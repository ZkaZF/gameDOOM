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

## Rencana Verifikasi

1. **Compile** proyek
2. **Jalankan** game dan cek player bisa jalan melewati semua ruangan dan lorong
3. **Minimap** di HUD menampilkan bentuk cross dungeon dengan benar
4. **Enemy spawn** tidak berada di dalam dinding
5. **Collision detection** normal di lorong-lorong sempit
