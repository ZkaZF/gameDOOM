# Implementasi Layout Map Baru — gameDOOM

## Deskripsi

Permintaan: Mengubah `worldMap[24][24]` di `map.h` agar sesuai dengan layout dungeon baru yang ada di `layoutMap.png`.

---

## Analisis Layout Baru (layoutMap.png)

Layout baru adalah **dungeon cross/plus** bergaya RPG dengan 4 ruangan yang terhubung ke pusat via lorong-lorong sempit:

```
         [Dapur Makanan (atas)]
                  |
[Dapur Makanan]--[Pusat]--[Cairi Bahan Makanan]
  (kiri/besar)     |
               [Storit]
                (bawah)
```

### Detail Ruangan:
| Ruangan             | Posisi (relatif ke peta)      | Keterangan                       |
|---------------------|-------------------------------|----------------------------------|
| Dapur Makanan (atas)| Tengah-atas                   | Ruangan persegi, berukuran medium|
| Dapur Makanan (kiri)| Kiri, agak lebih rendah       | Ruangan besar, memanjang         |
| Cairi Bahan Makanan | Kanan                         | Ruangan medium                   |
| Storit (bawah)      | Bawah-tengah                  | Ruangan kecil                    |
| Pusat / Hub         | Tengah                        | Ruangan kecil + simbol bulan sabit (item?) |

---

## Rencana Grid 24x24

Map tetap berukuran **24x24** (tidak perlu mengubah MAP_W/MAP_H).

**Legenda tile:**
- `0` = ruang kosong (walkable)
- `1` = dinding luar / solid boundary
- `2` = dinding ruangan Dapur Makanan atas (brick)
- `3` = dinding ruangan Cairi Bahan Makanan (blue-gray)
- `4` = dinding ruangan Storit (olive)
- `5` = dinding ruangan pusat / hub (wood)
- `6` = dinding ruangan Dapur Makanan kiri (metal dark)

**Layout Cross (diukur dari gambar):**
- Outer boundary: baris 0, 23 dan kolom 0, 23 penuh dinding `1`
- Dapur Makanan atas: kira-kira rows 1-5, cols 8-14
- Pusat/hub: kira-kira rows 7-10, cols 8-13
- Dapur Makanan kiri: kira-kira rows 6-11, cols 1-6
- Cairi Bahan Makanan: kira-kira rows 8-13, cols 16-22
- Storit: kira-kira rows 13-18, cols 8-14
- Lorong atas (hub ke Dapur Makanan atas): cols 10-11, rows 5-7
- Lorong kiri (hub ke Dapur Makanan kiri): rows 8-9, cols 6-8
- Lorong kanan (hub ke Cairi): rows 9-10, cols 13-16
- Lorong bawah (hub ke Storit): cols 10-11, rows 10-13

---

## Perubahan yang Direncanakan

### [MODIFY] map.h

1. **Ganti seluruh `worldMap[24][24]`** mengikuti layout cross dungeon baru
2. **Update komentar header** — ganti dari *Nuketown-inspired* ke *Dungeon Cross layout*
3. **Update PLAYER_START_X/Y** — pindah ke pusat (hub), kira-kira (10.5, 8.5)
4. **Update PLAYER_START_ANGLE** — menghadap ke kanan/timur (angle = 0.0f)

---

### [MODIFY] enemy.h — fungsi enemyInitLevel

Posisi spawn enemy direlokasi ke ruangan yang valid:

| Enemy          | Posisi Baru             | Ruangan                        |
|----------------|-------------------------|--------------------------------|
| Imp #1         | Dapur Makanan atas       | ~(11.0, 3.0)                  |
| Imp #2         | Cairi Bahan Makanan      | ~(19.5, 10.5)                 |
| Imp #3         | Storit                   | ~(11.0, 16.0)                 |
| Imp #4         | Dapur Makanan kiri       | ~(3.0, 8.5)                   |
| Imp #5         | Lorong kanan             | ~(15.0, 9.5)                  |
| Demon #1       | Pusat (hub)              | ~(10.0, 8.0)                  |
| Demon #2       | Pusat (hub)              | ~(11.5, 9.5)                  |
| Spectre #1     | Lorong atas              | ~(10.5, 6.0)                  |
| Spectre #2     | Lorong bawah             | ~(10.5, 12.5)                 |
| Spectre #3     | Lorong kiri              | ~(6.5, 8.5)                   |

---

## Open Questions

> [!IMPORTANT]
> Apakah **simbol bulan sabit di pusat** pada gambar merupakan item pickup atau hanya dekorasi/ornamen visual saja?

> [!NOTE]
> Lebar lorong di gambar terlihat sekitar 1-2 tile. Karena player butuh minimal 1 tile clearance (PLAYER_RADIUS = 0.2f), apakah lorong lebar 2 tile sudah cukup atau perlu lebih lebar?

---

## Rencana Verifikasi (Phase 1)

1. **Compile** proyek
2. **Jalankan** game dan cek player bisa jalan melewati semua ruangan dan lorong
3. **Minimap** di HUD menampilkan bentuk cross dungeon dengan benar
4. **Enemy spawn** tidak berada di dalam dinding
5. **Collision detection** normal di lorong-lorong sempit

---
---

# Phase 2 — Perbesar Map, Tekstur Lantai, dan Tembok Lebih Tinggi

## Deskripsi

Permintaan:
1. **Perbesar ukuran map ~3x** — Skala keseluruhan diperbesar agar ruangan terasa lebih luas dan imersif
2. **Lantai bertekstur ubin/kayu** — Bukan flat color, tapi procedural pattern seperti ubin keramik atau papan kayu
3. **Pertinggi tembok** — Tembok terasa lebih tinggi saat dilihat dari sudut pandang pemain

---

## 2.1 Perbesar Ukuran Map (3x Scale)

### Kondisi Saat Ini
- Map: `worldMap[32][40]` → `MAP_W = 40`, `MAP_H = 32`
- Setiap sel = 1 unit dunia
- Ruangan terbesar (Right/Cairi): ~16x16 sel
- Lorong: lebar 2 sel

### Rencana
- Map baru: `worldMap[96][120]` → `MAP_W = 120`, `MAP_H = 96`
- Setiap ruangan, lorong, dan dinding dikalikan 3x
- Lorong baru: lebar 6 sel (lebih nyaman untuk navigasi)
- Ruangan terbesar baru: ~48x48 sel

### File yang Perlu Diubah

#### [MODIFY] `map.h`
1. Ubah `MAP_W` dari `40` → `120`
2. Ubah `MAP_H` dari `32` → `96`
3. **Rebuild seluruh `worldMap[96][120]`** — setiap sel lama menjadi blok 3x3 sel
4. Update `PLAYER_START_X` dari `4.5f` → `~13.5f` (× 3)
5. Update `PLAYER_START_Y` dari `15.5f` → `~46.5f` (× 3)

#### [MODIFY] `enemy.h` — `enemyInitLevel()`
- Semua posisi spawn enemy × 3
- Contoh: `(11.0, 3.0)` → `(33.0, 9.0)`

#### [MODIFY] `item.h` — posisi spawn item
- Semua posisi item pickup × 3

#### [MODIFY] `raycaster.h`
- Update jarak fog: `perpWallDist / 16.0f` → `perpWallDist / 48.0f` (× 3, karena dunia lebih besar)

#### [MODIFY] `hud.h` — Minimap
- Skala minimap mungkin perlu di-adjust supaya tetap muat di layar
- Jika minimap menggambar 1 pixel per sel, perlu di-scale down

#### [MODIFY] `player.h`
- Cek `PLAYER_MOVE_SPEED` — mungkin perlu × 3 agar kecepatan relatif tetap sama
- Atau biarkan supaya map terasa lebih luas dan eksplorasi lebih lama

---

## 2.2 Tekstur Lantai (Ubin / Kayu)

### Kondisi Saat Ini
- Lantai di-render sebagai **flat color gradient** (dari `raycaster.h`, baris 58-72)
- Warna: dark gray-green (`0.18, 0.18, 0.16` ke `0.10, 0.10, 0.08`)
- Tidak ada floor raycasting / texturing

### Rencana
Implementasi **floor raycasting** dengan **procedural pattern** (tanpa file gambar eksternal).

#### Opsi A: Ubin Keramik (Tile Pattern)
- Pattern kotak-kotak (checkerboard) dengan 2 warna berbeda
- Warna ubin terang: `(0.35, 0.32, 0.28)` — krem/beige
- Warna ubin gelap: `(0.22, 0.20, 0.18)` — coklat tua
- Ditentukan oleh `(floorX + floorY) % 2`
- Tambah garis nat (grout line) tipis di antara ubin

#### Opsi B: Lantai Kayu (Wood Plank Pattern)
- Pattern garis horizontal/vertikal (papan kayu)
- Warna dasar: `(0.45, 0.30, 0.15)` — coklat kayu
- Variasi per papan berdasarkan `floor(floorY * 2) % 3`
- Tambahkan noise sederhana (hash function) untuk grain kayu

#### Opsi C: Hybrid per Ruangan
- Ubin untuk hub/pusat dan lorong
- Kayu untuk ruangan-ruangan
- Ditentukan oleh wall type area terdekat

### File yang Perlu Diubah

#### [MODIFY] `raycaster.h`
1. **Hapus** blok render floor flat color (baris 58-72)
2. **Tambahkan** floor raycasting loop:
   - Untuk setiap baris di bawah horizon → hitung jarak lantai → hitung koordinat dunia `(floorX, floorY)`
   - Tentukan warna berdasarkan pattern (ubin/kayu)
   - Terapkan distance fog
   - Render pixel per pixel (atau per-2-pixel untuk performa)
3. **Ceiling** bisa tetap flat gradient (langit) atau ditambah pattern juga

#### [MODIFY] `texture.h`
- Tambahkan fungsi helper:
  - `floorTileColor(float worldX, float worldY, float* r, float* g, float* b)` — menentukan warna lantai berdasarkan posisi dunia
  - `simpleHash(int x, int y)` — hash function untuk variasi warna kayu

> [!WARNING]
> Floor raycasting per-pixel bisa **berat secara performa** (800×300 pixel per frame). Optimasi:
> - Render setiap 2 pixel horizontal dan isi celahnya
> - Skip pixel yang terlalu jauh (fog = hitam total)
> - Gunakan lookup table untuk divisi

---

## 2.3 Pertinggi Tembok

### Kondisi Saat Ini
- Tinggi tembok: `lineHeight = (int)(SCREEN_H / perpWallDist)` (raycaster.h baris 136)
- Artinya: tembok setinggi 1 unit dunia = `600 / jarak` pixel di layar
- Tembok terlihat "pendek" terutama dari jauh

### Rencana
Tambahkan faktor pengali tinggi tembok:

```c
#define WALL_HEIGHT_SCALE 1.8f
lineHeight = (int)((SCREEN_H * WALL_HEIGHT_SCALE) / perpWallDist);
```

Dengan `WALL_HEIGHT_SCALE = 1.8f`:
- Tembok terasa ~2x lebih tinggi secara visual
- Tidak terlalu extrem sehingga ceiling/floor masih terlihat
- Jika terlalu tinggi, adjust ke `1.5f`; jika kurang, naikkan ke `2.0f`

### File yang Perlu Diubah

#### [MODIFY] `raycaster.h`
1. Tambahkan `#define WALL_HEIGHT_SCALE 1.8f` di bagian constants
2. Ubah baris 136: `lineHeight = (int)((SCREEN_H * WALL_HEIGHT_SCALE) / perpWallDist);`

---

## Urutan Implementasi (Phase 2)

| Step | Task                         | Prioritas | Estimasi |
|------|------------------------------|-----------|----------|
| 1    | Pertinggi tembok (WALL_HEIGHT_SCALE) | Tinggi | Cepat — 1 baris kode |
| 2    | Perbesar map 3x              | Tinggi    | Medium — rebuild worldMap + update semua posisi |
| 3    | Update posisi enemy & item   | Tinggi    | Cepat — kalikan semua × 3 |
| 4    | Update fog distance & minimap| Medium    | Cepat |
| 5    | Implementasi floor raycasting| Medium    | Berat — logic baru + optimasi performa |

---

## Rencana Verifikasi (Phase 2)

1. **Tembok tinggi** — Jalankan game, tembok terlihat lebih tinggi & imersif
2. **Map besar** — Semua ruangan & lorong walkable, tidak ada stuck di dinding
3. **Player spawn** benar di posisi baru (× 3)
4. **Enemy & item** spawn di posisi valid (× 3)
5. **Minimap** tetap terbaca meski map lebih besar
6. **Lantai bertekstur** — Pattern ubin/kayu terlihat jelas dari dekat, fade dengan jarak
7. **Performa** — FPS tetap stabil ≥ 30 fps dengan floor raycasting
