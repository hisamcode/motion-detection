# Motion Detection CCTV Application

A C++17 real-time motion detection application using OpenCV, based on the workflow described in `app_summary.md`. It supports webcam or video input, background subtraction, contour-based motion detection, configurable parameters, optional snapshot saving, and event logging.

## Prerequisites

* CMake 3.16+
* A C++17-compatible compiler (MSVC, Clang, or GCC)
* OpenCV compiled with video and imgproc modules

### Installing OpenCV on Windows (suggested via vcpkg)

```powershell
# Install vcpkg if you do not have it yet
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat

# Install OpenCV
./vcpkg install opencv

# Integrate with MSVC (optional but handy)
./vcpkg integrate install
```

If you use another package manager or a pre-built OpenCV binary, ensure that `OpenCV_DIR` is visible to CMake (for example, `-DOpenCV_DIR="C:/path/to/opencv/build"`).

## Building

```powershell
# From the repository root
mkdir build
# cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE="C:/Users/Admin/github.com/microsoft/vcpkg/scripts/buildsystems/vcpkg.cmake"  # optional
cmake -S . -B build `
      -G "Visual Studio 17 2022" ` 
      -DCMAKE_TOOLCHAIN_FILE="C:/Users/Admin/github.com/microsoft/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

The resulting executable will be generated at `build/Release/motion_detection.exe` (on Windows) or `build/motion_detection` on other platforms.

## Running

```powershell
# Use default webcam (index 0)
./build/Release/motion_detection

# Or specify a video file
./build/Release/motion_detection --video "path/to/video.mp4"
```

### Command-Line Options

```
--camera <index>          Use camera with given index (default 0).
--video <path>            Use a video file instead of camera.
--resize <factor>         Resize frames by factor (0 < factor <= 1). Tujuan: mempercepat pemrosesan, 1.0 = ukuran asli.
--skip <count>            Number of frames to skip between detections. Tujuan: mengurangi beban CPU. count=0 = cek tiap frame.
--threshold <value>       Binary threshold value (default 25). Nilai ambang (binary threshold) untuk memisahkan foreground dari background. Nilai lebih rendah → lebih sensitif, bisa banyak noise. Nilai lebih tinggi → lebih ketat, hanya gerakan besar terdeteksi. Default: 25.
--min-area <pixels>       Minimum contour area to treat as motion. Berguna untuk mengabaikan “noise” kecil (seperti pixel bergetar).
--bg <mog2|knn>           Background subtractor implementation. mog2 → Mixture of Gaussians v2 (umum dipakai, sensitif pada pencahayaan). knn → K-Nearest Neighbors (lebih robust untuk scene tertentu).
--roi x,y,w,h             Region of interest for motion detection. Definisikan region of interest → area spesifik dalam frame yang dianalisis. x,y = koordinat pojok kiri atas. w,h = lebar dan tinggi area.
--show-mask               Display the foreground mask window. untuk debugging. Menampilkan jendela visualisasi foreground mask (hitam putih) hasil background subtraction.
--save-snapshots [dir]    Save frames when motion detected (default folder 'snapshots').
--log <file>              Log detection events to a text file.
--help                    Print usage information.


--roi x,y,w,h
→ gunakan kalau hanya area tertentu yang relevan.
Contoh: hanya pintu/gerbang, bukan seluruh frame (supaya pohon bergoyang karena angin tidak bikin false positive).

```

MOG2 biasanya lebih stabil untuk CCTV indoor/outdoor.
Kalau banyak perubahan cahaya → coba knn.



### Example

```powershell
./build/Release/motion_detection `
  --camera 1 `
  --resize 0.7 `
  --threshold 30 `
  --min-area 800 `
  --show-mask `
  --save-snapshots snapshots `
  --log motion.log
```

```powershell
./build/Release/motion_detection `
  --resize 0.5 `
  --skip 1 `
  --threshold 30 `
  --min-area 500 `
  --show-mask `
  --bg mog2
```



## Repository Structure

```
CMakeLists.txt
README.md
app_summary.md
src/
  main.cpp
```

`app_summary.md` retains the original design overview.
