# Dokumentasi Teknis — DOOM GTI FPS

> Dokumentasi teknis lengkap untuk setiap file dalam project game DOOM GTI.

---

## Daftar Isi

1. [Arsitektur Umum](#arsitektur-umum)
2. [main.cpp](#maincpp)
3. [map.h](#maph)
4. [player.h](#playerh)
5. [raycaster.h](#raycasterh)
6. [texture.h](#textureh)
7. [hud.h](#hudh)
8. [Makefile.win](#makefilewin)
9. [Alur Eksekusi](#alur-eksekusi)
10. [Roadmap V2 ke Atas](#roadmap-v2-ke-atas)

---

## Arsitektur Umum

Game ini menggunakan arsitektur **header-only** — semua modul ditulis sebagai file `.h` dan di-include oleh `main.cpp`. Pendekatan ini menyederhanakan proses build karena hanya ada **satu compilation unit** (`main.cpp`).

```
┌─────────────────────────────────────────────────────────┐
│                        main.cpp                         │
│  ┌─────────┐  ┌──────────┐  ┌───────────┐  ┌────────┐  │
│  │  map.h  │  │ player.h │  │raycaster.h│  │ hud.h  │  │
│  └─────────┘  └──────────┘  └───────────┘  └────────┘  │
│                    ↓                ↓                    │
│             ┌─────────────────────────────┐             │
│             │         texture.h           │             │
│             └─────────────────────────────┘             │
└─────────────────────────────────────────────────────────┘
```

### Dependency Graph

| File | Depends On |
|------|-----------|
| `main.cpp` | `map.h`, `player.h`, `raycaster.h`, `hud.h`, `texture.h` |
| `raycaster.h` | `map.h`, `player.h`, `texture.h` |
| `hud.h` | `map.h`, `player.h` |
| `player.h` | `map.h` (via `isWalkable`) |
| `map.h` | *(tidak ada)* |
| `texture.h` | *(tidak ada)* |

---

## main.cpp

**Peran**: Entry point dan orchestrator utama game.

### Fungsi-Fungsi

#### `display(void)`
Callback utama rendering GLUT. Dipanggil setiap frame.

```
1. Bersihkan color buffer dan depth buffer
2. Set proyeksi Ortho 2D (untuk raycasting & HUD)
3. Panggil renderRaycastView(&player)   ← render dunia 3D
4. Panggil drawHUD(&player)             ← render overlay HUD
5. Swap buffer (double buffering)
```

#### `reshape(int width, int height)`
Callback resize window. Memperbarui `glViewport` dan posisi center mouse.

#### `keyDown(unsigned char key, int x, int y)`
Callback keyboard ditekan. Mengaktifkan flag movement pada player struct.

| Key | Flag yang diset |
|-----|-----------------|
| `W` | `player.moveForward = 1` |
| `S` | `player.moveBackward = 1` |
| `A` | `player.strafeLeft = 1` |
| `D` | `player.strafeRight = 1` |
| `ESC` | `exit(0)` |

#### `keyUp(unsigned char key, int x, int y)`
Callback keyboard dilepas. Me-reset flag movement ke `0`.

#### `mouseMotion(int x, int y)`
Callback gerakan mouse. Menghitung delta X lalu memanggil `playerRotate()`.

**Teknik Mouse Capture**:
```
1. Terima posisi mouse (x, y)
2. Jika mouseWarping == 1: abaikan event ini, reset flag, return
3. Hitung dx = x - windowCenterX
4. Panggil playerRotate(dx * MOUSE_SENS)
5. Set mouseWarping = 1
6. Warp kursor ke tengah window via glutWarpPointer()
```
> Flag `mouseWarping` mencegah loop tak terbatas — ketika kursor di-warp ke tengah, GLUT mengirim event motion baru yang harus diabaikan.

#### `idle(void)`
Callback game loop. Dipanggil berulang saat tidak ada event lain.
- Memanggil `playerMove(&player)` untuk mengaplikasikan movement
- Memanggil `glutPostRedisplay()` untuk request frame baru

#### `main(int argc, char *argv[])`
Inisialisasi GLUT, player, register semua callback, sembunyikan cursor.

---

## map.h

**Peran**: Mendefinisikan data level dalam bentuk grid 2D.

### Konstanta

| Konstanta | Nilai | Keterangan |
|-----------|-------|------------|
| `MAP_W` | 24 | Lebar peta (kolom) |
| `MAP_H` | 24 | Tinggi peta (baris) |
| `PLAYER_START_X` | 12.0 | Posisi awal X player |
| `PLAYER_START_Y` | 22.0 | Posisi awal Y player |
| `PLAYER_START_ANGLE` | 4.712 | Sudut awal (≈ -π/2, menghadap utara/atas peta) |

### Tipe Sel Peta

| Nilai | Jenis | Warna |
|-------|-------|-------|
| `0` | Ruang kosong (bisa dilewati) | — |
| `1` | Dinding luar (outer boundary) | Abu-abu gelap |
| `2` | Bangunan A (kiri) | Merah bata |
| `3` | Bangunan B (kanan) | Biru-abu |
| `4` | Cover/barrier (area tengah) | Olive/sandbag |
| `5` | Dinding interior (dalam bangunan) | Coklat kayu |

### Layout Peta (Nuketown-Inspired)

```
00: ████████████████████████  ← Batas luar
01: █                        █
02: █                        █
03: █   ██████      █████    █  ← Bangunan A (kiri) & B (kanan)
04: █   █    █      █   █    █
05: █   █ ▪  █      █ ▪ █    █  ← Interior (▪ = type 5)
06: █   █ ▪         . ▪ █    █  ← Pintu bangunan (gap)
07: █   █    █      █   █    █
08: █   ████.██    ██.████   █  ← Pintu depan
09: █           ♦   ♦        █  ← Cover tengah (♦ = type 4)
10: █          ♦       ♦     █
11: █                        █  ← Jalur bebas tengah
12: █                        █
13: █          ♦       ♦     █
14: █           ♦   ♦        █
15: █   ████.██    ██.████   █  ← Bangunan bawah (mirror)
...
23: ████████████████████████
```

> Layout ini simetris vertikal — bangunan atas dan bawah adalah mirror image satu sama lain, persis seperti Nuketown di CoD.

### Fungsi Helper

#### `getMap(int x, int y) → int`
Mengembalikan nilai sel peta di koordinat (x, y). Mengembalikan `1` (solid wall) untuk koordinat di luar batas — mencegah akses out-of-bounds.

#### `isWalkable(float x, float y) → int`
Mengembalikan `1` jika posisi (x, y) bisa dilewati (nilai sel = 0), `0` jika tidak.

---

## player.h

**Peran**: Mendefinisikan state player dan logika movement + collision.

### Konstanta

| Konstanta | Nilai | Keterangan |
|-----------|-------|------------|
| `MOVE_SPEED` | 0.05 | Unit per frame untuk gerak |
| `ROT_SPEED` | 0.03 | Radian per frame untuk rotasi keyboard |
| `MOUSE_SENS` | 0.003 | Multiplier sensitivitas mouse |
| `PLAYER_RADIUS` | 0.2 | Radius collision player (dalam unit peta) |

### Struct `Player`

```c
typedef struct {
    float x, y;          // Posisi di map coordinates
    float angle;          // Sudut pandang (radian)
    float dirX, dirY;     // Direction vector (unit vector)
    float planeX, planeY; // Camera plane (FOV)
    int health;           // HP pemain (0-100)
    int armor;            // Armor pemain
    int alive;            // Flag hidup/mati
    int moveForward;      // Flag gerakan (set oleh keyboard)
    int moveBackward;
    int strafeLeft;
    int strafeRight;
} Player;
```

### Sistem Kamera (Direction + Plane)

Kamera di raycasting didefinisikan oleh dua vektor:
- **`dir`** — ke mana player menghadap
- **`plane`** — vektor tegak lurus ke `dir`, merepresentasikan layar kamera

```
         dirX = cos(angle)
         dirY = sin(angle)
         
     planeX = -dirY * 0.66
     planeY =  dirX * 0.66
     
     Panjang plane = 0.66 → FOV = 2 * atan(0.66) ≈ 66°
```

### Fungsi

#### `playerInit(Player* p)`
Inisialisasi player: posisi awal dari konstanta map, hitung `dirX/Y` dan `planeX/Y` dari angle awal, set HP = 100.

#### `playerUpdateDirection(Player* p)`
Recalculate `dirX`, `dirY`, `planeX`, `planeY` dari `angle` saat ini. Dipanggil setelah rotasi.

#### `playerRotate(Player* p, float deltaAngle)`
Tambah `deltaAngle` ke `angle`, normalize ke range `[0, 2π]`, lalu panggil `playerUpdateDirection()`.

#### `playerMove(Player* p)`
**Wall-Sliding Collision Detection**:
```
1. Hitung moveX, moveY dari flag movement aktif
2. Coba newX = x + moveX
3. Jika isWalkable(newX ± radius, y) → update x
4. Coba newY = y + moveY  
5. Jika isWalkable(x, newY ± radius) → update y
```
Dengan memeriksa X dan Y secara **terpisah**, pemain bisa "slide" menyusuri dinding tanpa tersangkut di sudut.

---

## raycaster.h

**Peran**: Engine rendering utama — mengubah peta 2D menjadi tampilan 3D menggunakan raycasting.

### Konstanta

| Konstanta | Nilai | Keterangan |
|-----------|-------|------------|
| `SCREEN_W` | 800 | Lebar layar (pixel) |
| `SCREEN_H` | 600 | Tinggi layar (pixel) |

### Tabel Warna Dinding

```c
static float wallColors[][3] = {
    {0.00, 0.00, 0.00},  // 0: unused
    {0.45, 0.45, 0.48},  // 1: concrete (abu-abu)
    {0.65, 0.25, 0.18},  // 2: brick (merah bata)
    {0.40, 0.50, 0.60},  // 3: blue-gray
    {0.50, 0.48, 0.30},  // 4: sandbag (olive)
    {0.55, 0.38, 0.22},  // 5: wood (coklat)
    {0.30, 0.30, 0.35},  // 6: metal (gelap)
};
```

### Z-Buffer

```c
static float zBuffer[SCREEN_W];
```
Menyimpan jarak perpendicular ke dinding untuk setiap kolom layar. Akan digunakan di V3+ untuk sprite sorting (musuh & item di-render di depan/belakang dinding dengan benar).

### Fungsi Utama: `renderRaycastView(Player* player)`

#### Tahap 1: Background (Ceiling & Floor)

```
Ceiling: Gradient gelap (biru sangat gelap di atas → ungu gelap di horizon)
Floor:   Gradient gelap (abu di horizon → hitam di bawah)
Render dengan glBegin(GL_QUADS)
```

#### Tahap 2: Ray Casting Loop (per kolom x = 0..799)

```
Untuk setiap kolom pixel x:

A. Hitung arah ray:
   cameraX = (2 * x / SCREEN_W) - 1       → [-1.0, +1.0]
   rayDir  = playerDir + playerPlane * cameraX

B. Setup DDA:
   - Posisi grid saat ini: mapX = (int)player.x, mapY = (int)player.y
   - deltaDistX = |1 / rayDirX|            → jarak antar intersection X
   - deltaDistY = |1 / rayDirY|            → jarak antar intersection Y
   - Tentukan stepX/Y (+1 atau -1)
   - sideDistX/Y = jarak ke intersection pertama

C. Loop DDA (march through grid):
   WHILE not hit:
     IF sideDistX < sideDistY:
       sideDistX += deltaDistX
       mapX += stepX
       side = 0 (hit X-side, dinding vertikal)
     ELSE:
       sideDistY += deltaDistY
       mapY += stepY
       side = 1 (hit Y-side, dinding horizontal)
     IF map[mapX][mapY] > 0: hit = TRUE

D. Hitung jarak perpendicular (anti-fisheye):
   IF side == 0: perpDist = (mapX - playerX + (1-stepX)/2) / rayDirX
   IF side == 1: perpDist = (mapY - playerY + (1-stepY)/2) / rayDirY

E. Hitung tinggi strip:
   lineHeight = SCREEN_H / perpDist
   drawStart  = -lineHeight/2 + SCREEN_H/2   (clamped ke 0)
   drawEnd    = +lineHeight/2 + SCREEN_H/2   (clamped ke SCREEN_H-1)

F. Tentukan warna:
   wallType = map[mapX][mapY]
   (R,G,B)  = wallColors[wallType]
   IF side == 1: warna *= 0.7    (Y-side lebih gelap)
   shade    = 1.0 - (perpDist/16.0)  (distance fog)
   (R,G,B)  *= shade

G. Render strip vertikal:
   glBegin(GL_LINES)
     glVertex2f(x, drawStart)
     glVertex2f(x, drawEnd)
   glEnd()

H. Simpan di Z-Buffer:
   zBuffer[x] = perpDist
```

#### Mengapa Anti-Fisheye?

Jika menggunakan jarak Euclidean biasa (dari player ke titik tumbukan), dinding di pinggir layar akan tampak lebih jauh sehingga gambar terlihat "bengkok" seperti lensa fisheye. Jarak **perpendicular** (tegak lurus ke camera plane) menghindari distorsi ini.

---

## texture.h

**Peran**: Utilitas warna dan shading yang dipakai modul lain.

### Fungsi

#### `clampf(float v, float lo, float hi) → float`
Membatasi nilai `v` antara `lo` dan `hi`.

#### `lerpf(float a, float b, float t) → float`
Linear interpolasi antara `a` dan `b` dengan factor `t` (0.0 - 1.0).

#### `applyFog(float* r, float* g, float* b, float distance, float maxDist)`
Menggelapkan warna berdasarkan jarak. `fogFactor = clamp(1 - dist/maxDist, 0.1, 1.0)`. Setiap komponen RGB dikali `fogFactor`.

---

## hud.h

**Peran**: Semua elemen Heads-Up Display yang dirender di atas tampilan 3D.

### Teknik Rendering HUD

HUD di-render dalam koordinat layar 2D menggunakan **Orthographic projection**:

```c
// Simpan state matrix, switch ke 2D ortho
glMatrixMode(GL_PROJECTION);
glPushMatrix();
glLoadIdentity();
glOrtho(0, SCREEN_W, SCREEN_H, 0, -1, 1);  // origin di kiri-atas
glMatrixMode(GL_MODELVIEW);
glPushMatrix();
glLoadIdentity();

// ... render HUD elements ...

// Restore matrix
glMatrixMode(GL_PROJECTION);
glPopMatrix();
glMatrixMode(GL_MODELVIEW);
glPopMatrix();
```

Selama fase HUD, `GL_DEPTH_TEST` dinonaktifkan agar HUD selalu di depan.

### Fungsi

#### `drawText(float x, float y, const char* text, void* font)`
Render string teks menggunakan `glutBitmapCharacter`. Font yang tersedia: `GLUT_BITMAP_HELVETICA_12`, `GLUT_BITMAP_HELVETICA_18`, `GLUT_BITMAP_TIMES_ROMAN_24`.

#### `drawCrosshair(void)`
Menggambar crosshair (+) di tengah layar.
- **Warna**: Hijau cerah (`0.0, 1.0, 0.3`) dengan shadow hitam di belakang untuk keterbacaan
- **Bentuk**: 4 garis dengan gap di tengah + titik kecil
- **Posisi**: `(SCREEN_W/2, SCREEN_H/2)`

#### `drawHealthBar(int health)`
Menggambar health bar di sudut kiri bawah.
- **Warna bar** berubah berdasarkan HP: Hijau (>50%), Kuning (25-50%), Merah (<25%)
- Angka HP ditampilkan sebagai teks di dalam bar

#### `drawMinimap(Player* player)`
Menggambar peta kecil di sudut kanan atas.
- Setiap sel dinding digambar sebagai kotak berwarna
- Posisi player ditunjukkan dengan **titik hijau**
- Arah pandang player ditunjukkan dengan **garis pendek**
- Background semi-transparan menggunakan alpha blending

#### `drawHUD(Player* player)`
Fungsi utama yang memanggil semua elemen HUD secara berurutan: crosshair → health bar → minimap → debug text.

---

## Makefile.win

**Peran**: Build configuration untuk Dev-C++ GTI MOD.

### Variabel Penting

| Variabel | Nilai |
|----------|-------|
| `CPP` | `g++.exe` (compiler C++) |
| `BIN` | `tubesGame.exe` (output) |
| `LIBS` | `-lglut32 -lglu32 -lopengl32 -lwinmm -lgdi32` |
| `CXXFLAGS` | `-g3` (debug info) |

### Dependency Rule

```makefile
main.o: main.cpp map.h player.h raycaster.h texture.h hud.h
```
Jika **salah satu header diubah**, `main.cpp` akan **di-compile ulang** secara otomatis.

---

## Alur Eksekusi

```
main()
 ├─ glutInit()                     ← Init GLUT
 ├─ glutCreateWindow()             ← Buat window 800×600
 ├─ playerInit()                   ← Set posisi & arah awal player
 ├─ glutReshapeFunc(reshape)       
 ├─ glutDisplayFunc(display)       ← Register render callback
 ├─ glutKeyboardFunc(keyDown/Up)   ← Register input callbacks
 ├─ glutPassiveMotionFunc(mouse)   
 ├─ glutIdleFunc(idle)             
 ├─ glutSetCursor(NONE)            ← Sembunyikan cursor
 └─ glutMainLoop()                 ← Masuk event loop
 
Setiap frame:
idle()
 └─ playerMove()     ← Terapkan movement berdasarkan flag keyboard
 └─ glutPostRedisplay()
 
display()
 ├─ glClear()
 ├─ renderRaycastView()   ← Raycasting 800 rays → render 3D world
 └─ drawHUD()             ← Overlay crosshair, HP bar, minimap
 
Input events (async):
keyDown/keyUp  → set/reset flag (moveForward, strafeLeft, dll)
mouseMotion    → playerRotate(dx * sensitivity) + warp cursor
```

---

## Roadmap V2 ke Atas

### V2 — Senjata & Sistem Tembak
File baru: `weapon.h`

```c
typedef struct {
    int type;          // WEAPON_SHOTGUN, WEAPON_PISTOL
    int ammo;
    float recoilY;     // Animasi recoil
    float fireTimer;   // Cooldown antar tembakan
    int isFiring;
} Weapon;
```

Senjata di-render dalam **viewport 3D terpisah** di kanan bawah layar menggunakan `gluPerspective`, terlepas dari raycasting.

### V3 — Musuh (3 Tipe)

File baru: `enemy.h`

| Tipe | HP | Speed | Behavior |
|------|-----|-------|----------|
| Imp | 50 | Medium | Chase + melee attack |
| Demon | 150 | Slow | Tank, damage besar |
| Spectre | 30 | Fast | Hit & run |

Musuh di-render sebagai **billboard** (selalu menghadap kamera) menggunakan transformasi:
```
glRotatef(atan2(player.x - enemy.x, player.y - enemy.y) * RAD2DEG, 0, 1, 0)
```

Z-buffer dari raycasting digunakan untuk menentukan apakah musuh terlihat atau tersembunyi di balik dinding.

### V4 — Item System

File baru: `item.h`

Item floating menggunakan animasi sinusoidal:
```c
glTranslatef(item.x, 0.3f + sinf(time * 2.0f) * 0.1f, item.y);
glRotatef(time * 90.0f, 0, 1, 0);
```

### V5 — Polish

- Menu screen menggunakan `glOrtho` 2D dengan `glutBitmapCharacter`
- Sound via `PlaySound()` dari `<windows.h>` / WinMM API
- Minimap ditingkatkan dengan indicator musuh (titik merah)
