#include <opencv2/opencv.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct Config {
    bool useCamera = true;
    int cameraIndex = 0;
    std::string videoPath;

    double resizeFactor = 1.0;           // 0 < factor <= 1.0
    int frameSkip = 0;                    // Number of frames to skip between detections
    int gaussianKernel = 21;              // Must be odd

    int thresholdValue = 25;
    int dilateIterations = 2;
    int erodeIterations = 0;
    double minContourArea = 500.0;

    bool showMaskWindow = false;
    bool saveSnapshots = false;
    std::string snapshotDirectory = "snapshots";
    std::string logFilePath;

    std::string backgroundMethod = "mog2";  // Available: mog2, knn
    cv::Rect roi;                             // Empty rect => full frame
};

static void printUsage(const std::string& programName) {
    std::cout << "Usage: " << programName << " [options]\n\n"
              << "Options:\n"
              << "  --camera <index>          Use camera with given index (default 0).\n"
              << "  --video <path>            Use a video file instead of camera.\n"
              << "  --resize <factor>         Resize frames by factor (0 < factor <= 1).\n"
              << "  --skip <count>           Number of frames to skip between detections.\n"
              << "  --threshold <value>      Binary threshold value (default 25).\n"
              << "  --min-area <pixels>      Minimum contour area to treat as motion.\n"
              << "  --bg <mog2|knn>          Background subtractor implementation.\n"
              << "  --roi x,y,w,h            Region of interest for motion detection.\n"
              << "  --show-mask              Display the foreground mask window.\n"
              << "  --save-snapshots [dir]   Save frames when motion detected (default folder 'snapshots').\n"
              << "  --log <file>             Log detection events to a text file.\n"
              << "  --help                   Print this message.\n";
}

static bool parseRoi(const std::string& input, cv::Rect& roi) {
    std::stringstream ss(input);
    std::string token;
    std::vector<int> values;

    while (std::getline(ss, token, ',')) {
        try {
            values.emplace_back(std::stoi(token));
        } catch (const std::exception&) {
            return false;
        }
    }

    if (values.size() != 4) {
        return false;
    }

    roi = cv::Rect(values[0], values[1], values[2], values[3]);
    return roi.width > 0 && roi.height > 0;
}

static bool parseArguments(int argc, char** argv, Config& config) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help") {
            printUsage(argv[0]);
            return false;
        } else if (arg == "--camera" && i + 1 < argc) {
            config.useCamera = true;
            config.cameraIndex = std::stoi(argv[++i]);
        } else if (arg == "--video" && i + 1 < argc) {
            config.useCamera = false;
            config.videoPath = argv[++i];
        } else if (arg == "--resize" && i + 1 < argc) {
            config.resizeFactor = std::stod(argv[++i]);
            if (config.resizeFactor <= 0.0 || config.resizeFactor > 1.0) {
                std::cerr << "Invalid resize factor. Must be in (0, 1].\n";
                return false;
            }
        } else if (arg == "--skip" && i + 1 < argc) {
            config.frameSkip = std::max(0, std::stoi(argv[++i]));
        } else if (arg == "--threshold" && i + 1 < argc) {
            config.thresholdValue = std::stoi(argv[++i]);
        } else if (arg == "--min-area" && i + 1 < argc) {
            config.minContourArea = std::stod(argv[++i]);
        } else if (arg == "--bg" && i + 1 < argc) {
            config.backgroundMethod = argv[++i];
        } else if (arg == "--roi" && i + 1 < argc) {
            cv::Rect roiCandidate;
            if (!parseRoi(argv[++i], roiCandidate)) {
                std::cerr << "Invalid ROI format. Expected x,y,width,height.\n";
                return false;
            }
            config.roi = roiCandidate;
        } else if (arg == "--show-mask") {
            config.showMaskWindow = true;
        } else if (arg == "--save-snapshots") {
            config.saveSnapshots = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                config.snapshotDirectory = argv[++i];
            }
        } else if (arg == "--log" && i + 1 < argc) {
            config.logFilePath = argv[++i];
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return false;
        }
    }

    if (config.backgroundMethod != "mog2" && config.backgroundMethod != "knn") {
        std::cerr << "Unsupported background subtractor: " << config.backgroundMethod << "\n";
        return false;
    }

    if (!config.useCamera && config.videoPath.empty()) {
        std::cerr << "No video source provided. Use --camera or --video.\n";
        return false;
    }

    if (config.gaussianKernel % 2 == 0) {
        ++config.gaussianKernel;  // ensure odd for Gaussian blur
    }

    return true;
}

static std::string formatTimestampForDisplay(const std::chrono::system_clock::time_point& tp) {
    const auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

static std::string formatTimestampForFilename(const std::chrono::system_clock::time_point& tp) {
    const auto t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

int main(int argc, char** argv) {
    Config config;

    if (!parseArguments(argc, argv, config)) {
        return 0;
    }

    cv::VideoCapture capture;
    if (config.useCamera) {
        capture.open(config.cameraIndex, cv::CAP_ANY);
    } else {
        capture.open(config.videoPath, cv::CAP_ANY);
    }

    if (!capture.isOpened()) {
        std::cerr << "Failed to open video source." << std::endl;
        return 1;
    }

    cv::Ptr<cv::BackgroundSubtractor> subtractor;
    if (config.backgroundMethod == "knn") {
        subtractor = cv::createBackgroundSubtractorKNN();
    } else {
        subtractor = cv::createBackgroundSubtractorMOG2();
    }

    std::ofstream logStream;
    if (!config.logFilePath.empty()) {
        logStream.open(config.logFilePath, std::ios::app);
        if (!logStream) {
            std::cerr << "Failed to open log file: " << config.logFilePath << std::endl;
        }
    }

    if (config.saveSnapshots) {
        try {
            std::filesystem::create_directories(config.snapshotDirectory);
        } catch (const std::exception& ex) {
            std::cerr << "Failed to create snapshot directory: " << ex.what() << std::endl;
            return 1;
        }
    }

    if (config.showMaskWindow) {
        cv::namedWindow("Foreground Mask", cv::WINDOW_NORMAL);
    }
    cv::namedWindow("Motion Detection", cv::WINDOW_NORMAL);

    cv::Rect effectiveRoi;
    bool roiInitialized = false;
    bool motionDetected = false;
    int frameIndex = 0;
    std::chrono::system_clock::time_point lastSnapshotTime = std::chrono::system_clock::from_time_t(0);

    cv::Mat frame;
    while (capture.read(frame)) {
        if (frame.empty()) {
            break;
        }
        ++frameIndex;

        if (config.resizeFactor > 0.0 && config.resizeFactor < 1.0) {
            cv::resize(frame, frame, cv::Size(), config.resizeFactor, config.resizeFactor, cv::INTER_LINEAR);
        }

        if (!roiInitialized) {
            effectiveRoi = config.roi;
            if (effectiveRoi.width <= 0 || effectiveRoi.height <= 0) {
                effectiveRoi = cv::Rect(0, 0, frame.cols, frame.rows);
            } else {
                effectiveRoi &= cv::Rect(0, 0, frame.cols, frame.rows);
                if (effectiveRoi.empty()) {
                    effectiveRoi = cv::Rect(0, 0, frame.cols, frame.rows);
                }
            }
            roiInitialized = true;
        }

        if (config.frameSkip > 0 && (frameIndex % (config.frameSkip + 1) != 0)) {
            cv::imshow("Motion Detection", frame);
            if (config.showMaskWindow) {
                cv::Mat emptyMask = cv::Mat::zeros(frame.size(), CV_8UC1);
                cv::imshow("Foreground Mask", emptyMask);
            }
            const int key = cv::waitKey(1);
            if (key == 27 || key == 'q') {
                break;
            }
            continue;
        }

        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        cv::Mat blurred;
        const cv::Size kernelSize(config.gaussianKernel, config.gaussianKernel);
        cv::GaussianBlur(gray, blurred, kernelSize, 0);

        cv::Mat fgMask;
        subtractor->apply(blurred, fgMask);

        cv::Mat thresholdMask;
        cv::threshold(fgMask, thresholdMask, config.thresholdValue, 255, cv::THRESH_BINARY);

        if (config.erodeIterations > 0) {
            cv::erode(thresholdMask, thresholdMask, cv::Mat(), cv::Point(-1, -1), config.erodeIterations);
        }
        if (config.dilateIterations > 0) {
            cv::dilate(thresholdMask, thresholdMask, cv::Mat(), cv::Point(-1, -1), config.dilateIterations);
        }

        cv::Mat roiMask = cv::Mat::zeros(thresholdMask.size(), CV_8UC1);
        thresholdMask(effectiveRoi).copyTo(roiMask(effectiveRoi));

        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(roiMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        std::vector<cv::Rect> motionBoxes;
        for (const auto& contour : contours) {
            const double area = cv::contourArea(contour);
            if (area < config.minContourArea) {
                continue;
            }
            cv::Rect box = cv::boundingRect(contour);
            motionBoxes.emplace_back(box);
        }

        motionDetected = !motionBoxes.empty();

        cv::Mat displayFrame = frame.clone();
        for (const auto& box : motionBoxes) {
            cv::rectangle(displayFrame, box, cv::Scalar(0, 255, 0), 2);
        }

        const auto now = std::chrono::system_clock::now();
        const std::string timestamp = formatTimestampForDisplay(now);
        cv::putText(displayFrame, timestamp, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);

        if (motionDetected) {
            cv::putText(displayFrame, "MOTION DETECTED", cv::Point(10, 60), cv::FONT_HERSHEY_SIMPLEX, 0.9,
                        cv::Scalar(0, 0, 255), 2);

            if (config.saveSnapshots) {
                const auto sinceLastSnapshot = std::chrono::duration_cast<std::chrono::seconds>(now - lastSnapshotTime).count();
                if (sinceLastSnapshot >= 1) {
                    const std::string filename = config.snapshotDirectory + "/motion_" + formatTimestampForFilename(now) + ".png";
                    cv::imwrite(filename, frame);
                    lastSnapshotTime = now;

                    if (logStream) {
                        logStream << formatTimestampForDisplay(now) << ": snapshot saved to " << filename << '\n';
                        logStream.flush();
                    }
                }
            }
        }

        if (logStream && motionDetected) {
            logStream << timestamp << ": motion detected in " << motionBoxes.size() << " region(s)." << std::endl;
        }

        cv::imshow("Motion Detection", displayFrame);
        if (config.showMaskWindow) {
            cv::imshow("Foreground Mask", roiMask);
        }

        const int key = cv::waitKey(1);
        if (key == 27 || key == 'q') {
            break;
        }
    }

    capture.release();
    cv::destroyAllWindows();

    return 0;
}
