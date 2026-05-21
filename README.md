# 🔫 DOOM GTI — Raycasting FPS Game

> **Tugas Besar Game Technology and Innovation — Semester 4**  
> Universitas Diponegoro · Dibangun dengan **C++ / OpenGL / FreeGLUT**

![Language](https://img.shields.io/badge/Language-C%2B%2B-blue?style=flat-square)
![Renderer](https://img.shields.io/badge/Renderer-OpenGL-brightgreen?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey?style=flat-square)
![Status](https://img.shields.io/badge/Status-V5%20Arena%20Mode-success?style=flat-square)

---

## 📖 Deskripsi

Game **First-Person Shooter (FPS)** bergaya retro terinspirasi dari DOOM Classic (1993), dibangun dari nol menggunakan teknik **Raycasting** untuk merender dunia 3D dari peta 2D berbasis grid.

Engine raycasting menggunakan algoritma **DDA (Digital Differential Analyzer)** untuk menembakkan sinar per-kolom piksel, menghasilkan ilusi perspektif 3D yang autentik. Sementara itu, objek dalam dunia (senjata, musuh, item) dirender menggunakan **OpenGL 3D primitives** dengan frustum kamera yang disesuaikan.

**Fitur unggulan terbaru (V5 - Arena Update):**
- ✅ **Wave-Based Arena Combat**: Musuh muncul dalam 5 wave di ruangan tertentu. Pintu terkunci secara otomatis saat masuk, dan terbuka setelah semua wave selesai.
- ✅ **Tekstur Lantai (BMP)**: Menggunakan file `Floor.bmp` dengan rendering lantai *single-zone* dan *distance fog* yang agresif untuk visual memukau tanpa aliasing (efek burik).
- ✅ **Dungeon Cross Layout**: Peta skala besar (3x lebih luas) berbentuk *cross/plus* bergaya RPG.
- ✅ **Raycasting Engine**: Anti-fisheye correction, *distance fog*, tembok tinggi untuk imersi.
- ✅ **3 Jenis Musuh**: Imp (merah), Demon (tank hijau), Spectre (hantu biru) dengan AI state machine.
- ✅ **Sistem Senjata 3D**: Pistol & Shotgun dengan recoil, muzzle flash, reload, dan peluru.
- ✅ **HUD Interaktif**: Indikator Wave, status Arena, Crosshair, Health/Armor bar, Minimap, Kill Counter.

### ⚡ Optimasi Performa (Anti-Lag)
Game ini mengimplementasikan teknik rendering yang dioptimasi agar tetap berjalan lancar (30-60 FPS) meskipun peta dunia berukuran sangat besar:
1. **Adaptive Skip Rendering**: Algoritma lantai (*floor raycasting*) pintar yang mengambil sampel piksel setiap langkah 2x2 atau 4x4 secara adaptif berdasarkan kedalaman (z-depth) agar tidak memberatkan CPU.
2. **Batching Draw Calls (`GL_POINTS` & `GL_LINES`)**: Menggabungkan perhitungan piksel lantai dan tembok ke dalam satu instruksi.
3. **Occlusion Culling**: Sistem hanya menggambar (*render*) objek dan musuh yang tidak tertutup dinding berkat Z-Buffer 1D array.
4. **Compiler Optimization (`-O2`)**: Memanfaatkan pengaturan kompiler C++ untuk merampingkan matematika mentah.

---

## 🗺️ Layout Peta (Dungeon Cross)

Peta berukuran skala besar (world size 120x96) dengan beberapa zona:
- **Pusat (Hub)**: Area kayu perempatan di tengah.
- **Top & Bottom Room**: *Arena Room* dengan mekanisme Wave. Saat dimasuki, pintu terkunci dan Anda harus bertahan dari 5 wave musuh!
- **Left Room**: Ruang spawn / awal pemain.
- **Right Room**: Ruang ekstra luas untuk mencari *item pickup* (Health/Ammo/Armor).

---

## 🎮 Kontrol

| Input | Aksi |
|-------|------|
| `W` / `S` | Maju / Mundur |
| `A` / `D` | Strafe Kiri / Kanan |
| **Mouse (Horizontal)** | Putar kamera kiri/kanan |
| **Mouse (Vertical)** | Atur pitch (lihat atas/bawah) |
| **LMB** / `Space` | Tembak |
| `R` | Reload senjata |
| `1` | Switch ke Pistol |
| `2` | Switch ke Shotgun |
| **Scroll Mouse** | Ganti senjata |
| `F5` | Respawn / Restart level & Reset Arena |
| `ESC` | Keluar |

---

## 🚀 Cara Build & Jalankan

### ✅ Menggunakan Dev-C++ GTI MOD (Direkomendasikan)

1. Buka **Dev-C++ GTI MOD**
2. **File → Open Project** → pilih file `tubesGame.dev`
3. Tekan **F9** untuk Compile & Run langsung

---

### ⚡ Menggunakan Terminal (PowerShell / CMD / VS Code)

Sangat direkomendasikan jika Anda ingin kompilasi cepat via terminal tanpa membuka IDE Dev-C++.

#### 📌 Cara 1: Menggunakan `mingw32-make` (Paling Mudah)
Jika Anda menggunakan **Dev-C++ GTI MOD**, utility `mingw32-make` sudah terinstall secara bawaan di folder instalasi.

**Di PowerShell / CMD:**
```powershell
# Jalankan perintah ini untuk compile secara otomatis menggunakan Makefile.win
& "C:\Program Files (x86)\GibsTeam\Dev-C++ GTI MOD\Dev-Cpp\Dev-Cpp\MinGW32\bin\mingw32-make.exe" -f Makefile.win

# Jalankan game
.\tubesGame.exe
```

*Jika Anda ingin membersihkan file objek (`.o`) dan hasil build lama sebelum compile ulang:*
```powershell
& "C:\Program Files (x86)\GibsTeam\Dev-C++ GTI MOD\Dev-Cpp\Dev-Cpp\MinGW32\bin\mingw32-make.exe" -f Makefile.win clean
```

---

#### 📌 Cara 2: Menggunakan perintah manual `g++` (Cadangan)
Jika Anda ingin melakukan kompilasi manual baris demi baris menggunakan compiler G++ bawaan Dev-C++:

**Di PowerShell:**
```powershell
# 1. Definisikan path ke compiler dan folder include/lib
$cpp = "C:/Program Files (x86)/GibsTeam/Dev-C++ GTI MOD/Dev-Cpp/Dev-Cpp/MinGW32/bin/g++.exe"
$inc = "C:/Program Files (x86)/GibsTeam/Dev-C++ GTI MOD/Dev-Cpp/Dev-Cpp/MinGW32/include"
$lib = "C:/Program Files (x86)/GibsTeam/Dev-C++ GTI MOD/Dev-Cpp/Dev-Cpp/MinGW32/lib"

# 2. Compile main.cpp menjadi object file main.o
& $cpp -c main.cpp -o main.o -I"$inc" -O2 -fpermissive

# 3. Link file main.o dengan library OpenGL & FreeGLUT menjadi executable
& $cpp main.o -o tubesGame.exe -L"$lib" -static-libstdc++ -static-libgcc -mwindows -lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32

# 4. Jalankan game
.\tubesGame.exe
```

---

## 📂 Struktur File

```
gameDOOM/
├── main.cpp          # Entry point, game loop, input handler (keyboard & mouse)
├── map.h             # Data level — Dungeon layout, definisi tipe sel & warna dinding
├── player.h          # State player, movement WASD, collision, mouse look
├── raycaster.h       # Engine raycasting DDA — render dinding, lantai tekstur (BMP), langit-langit
├── texture.h         # Utilitas pemuatan tekstur BMP (Floor.bmp) dan helper warna
├── weapon.h          # Sistem senjata 3D, projectile, recoil, muzzle flash
├── enemy.h           # Sistem Wave Arena, AI musuh 3D (Imp, Demon, Spectre), collision
├── item.h            # Item pickup 3D (Health, Ammo, Armor)
├── hud.h             # Overlay HUD: Arena wave panel, crosshair, minimap, score
├── Makefile.win      # Build configuration untuk Dev-C++ / MinGW32
├── layoutMap.png     # Visualisasi layout peta asli
├── Floor.bmp         # File gambar tekstur lantai
├── README.md         # Dokumen ini
└── DOCS.md           # Dokumentasi teknis lengkap per-file
```

---

## 📈 Versi & Roadmap

| Versi | Status | Fitur Utama |
|-------|--------|-------------|
| **V1** | ✅ Selesai | Core engine, raycasting DDA, peta grid, movement WASD, mouse look |
| **V2** | ✅ Selesai | Senjata 3D (Pistol + Shotgun), projectile, recoil, reload |
| **V3** | ✅ Selesai | 3 tipe musuh 3D, AI state machine, hit detection, kill counter |
| **V4** | ✅ Selesai | Item drop (Health/Ammo/Armor), HUD lengkap, minimap |
| **V5** | ✅ Selesai | Arena Wave system, dungeon map scale 3x, texturing lantai (BMP), optimasi |

---

## 🛠️ Teknologi

| Komponen | Detail |
|----------|--------|
| Bahasa | C++ |
| Compiler | GCC 4.8.1 via MinGW32 |
| IDE | Dev-C++ GTI MOD |
| Windowing | FreeGLUT (`glut32`) |
| Rendering | OpenGL fixed-function pipeline |
| Math | `math.h` standar (sin, cos, atan2, sqrtf) |

---

## 👥 Anggota Tim

| Nama | NIM |
|------|-----|
| *Azka Wayasy Al Hafizh* | *24060124140161* | 
| *Menza Isaiah Tampubolon* | *24060124140138* | 
| *Ikrar Maheswara Rabbani Wibowo* | *24060124140202* | 
| *Ali Maskan Ferry Purwanto* | *24060124130072* | 

---

## 🌿 Panduan Kolaborasi Git (4 Orang)

### Struktur Branch

```
main                  ← Branch utama (STABIL, selalu bisa dijalankan)
└── dev               ← Branch integrasi (semua fitur dikumpulkan di sini)
    ├── dev/orang1    ← Branch milik anggota 1
    ├── dev/orang2    ← Branch milik anggota 2
    ├── dev/orang3    ← Branch milik anggota 3
    └── dev/orang4    ← Branch milik anggota 4
```

> **Aturan Utama:**
> - ❌ **JANGAN push langsung ke `main`**
> - ✅ Kerjakan di branch sendiri, lalu merge ke `dev`, baru ke `main`

---

### 🔧 Setup Awal (Lakukan Sekali)

**Clone repo dan buat branch masing-masing:**

```bash
# Clone repo
git clone https://github.com/ZkaZF/gameDOOM.git
cd gameDOOM

# Buat branch dev (jika belum ada)
git checkout -b dev
git push origin dev

# Buat branch pribadi dari dev (ganti "orang1" sesuai nama/nim)
git checkout -b dev/orang1
git push origin dev/orang1
```

---

### 📅 Alur Kerja Harian

**Setiap kali mau mulai coding:**

```bash
# 1. Pastikan branch kamu aktif
git checkout dev/orang1

# 2. Ambil update terbaru dari dev (biar tidak ketinggalan)
git fetch origin
git merge origin/dev

# 3. Mulai koding...

# 4. Simpan perubahan
git add .
git commit -m "feat: tambah fitur XYZ"

# 5. Push ke branch sendiri
git push origin dev/orang1
```

---

### 🔀 Merge ke `dev` (Setelah Fitur Selesai)

```bash
# 1. Pindah ke branch dev
git checkout dev

# 2. Ambil update terbaru dari remote
git pull origin dev

# 3. Merge branch kamu ke dev
git merge dev/orang1

# 4. Jika ada konflik, selesaikan dulu lalu:
git add .
git commit -m "merge: gabung fitur orang1 ke dev"

# 5. Push ke remote
git push origin dev
```

---

### 🚀 Merge ke `main` (Setelah Semua Fitur Stabil)

> Lakukan ini bersama-sama saat milestone (V2, V3, dst.) sudah siap.

```bash
# 1. Pindah ke main
git checkout main

# 2. Merge dari dev
git merge dev

# 3. Push ke remote
git push origin main
```