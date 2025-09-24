# Ringkasan Aplikasi Motion Detection CCTV dengan C dan OpenCV

## Alur Utama Aplikasi:

### 1. Inisialisasi
- Membuka sumber video (webcam/CCTV stream)
- Menginisialisasi background subtractor
- Setup window untuk menampilkan hasil

### 2. Pemrosesan Frame-per-Frame
- Membaca frame dari video source
- Konversi ke grayscale
- Aplikasi Gaussian blur untuk mengurangi noise
- Background subtraction untuk mendeteksi perubahan

### 3. Deteksi Gerakan
- Thresholding untuk mendapatkan area bergerak
- Operasi morfologi (dilasi/erosi) untuk membersihkan noise
- Menemukan kontur dari area yang bergerak
- Filter kontur berdasarkan ukuran (abaikan gerakan kecil)

### 4. Visualisasi & Output
- Gambar bounding box di sekitar objek bergerak
- Tampilkan teks "MOTION DETECTED" ketika terdeteksi gerakan
- Simpan frame ketika terdeteksi gerakan (opsional)
- Tampilkan video real-time dengan overlay deteksi

### 5. Terminasi
- Handle interrupt signal (Ctrl+C)
- Bebaskan resources
- Tutup semua window

## Fitur-Fitur Utama:

### ✅ **Fungsi Inti**
- **Real-time motion detection** dari webcam/CCTV
- **Background subtraction** adaptif untuk kondisi cahaya berubah
- **Noise filtering** dengan operasi morfologi
- **Threshold konfigurabel** untuk sensitivitas deteksi

### ✅ **Visualisasi**
- Bounding box warna hijau mengelilingi objek bergerak
- Display timestamp dan status deteksi
- Opsi menampilkan foreground mask untuk debugging
- Multiple view (original + processed)

### ✅ **Konfigurasi**
- Parameter yang dapat disesuaikan:
  - Threshold sensitivitas
  - Ukuran minimum objek yang dideteksi
  - Jenis background subtractor (MOG2/KNN)
  - Area ROI (Region of Interest)

### ✅ **Output & Logging**
- Save snapshot ketika motion terdeteksi
- Log events ke file teks
- Opsi recording video selama motion detected

### ✅ **Optimisasi**
- Frame skipping untuk performa
- Resize frame untuk processing yang lebih cepat
- Multi-threading (opsional advanced feature)

## Struktur Kode:
```c
main()
├── init_camera()
├── setup_background_subtractor()
├── while(running)
│   ├── capture_frame()
│   ├── preprocess_frame()
│   ├── background_subtraction()
│   ├── find_contours()
│   ├── filter_motions()
│   ├── draw_results()
│   └── handle_output()
└── cleanup()
```

Aplikasi ini akan memberikan dasar yang solid untuk sistem monitoring berbasis motion detection yang dapat dikembangkan lebih lanjut sesuai kebutuhan spesifik.