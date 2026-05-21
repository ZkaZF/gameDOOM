Add-Type -AssemblyName System.Drawing

$srcPng = "e:\UNDIP MATKUL\Smt 4\GTI\gameDOOM\prabowoPixel.png"
$dstBmp = "e:\UNDIP MATKUL\Smt 4\GTI\gameDOOM\prabowoPixel.bmp"

# Load PNG asli (ada alpha channel)
$srcImg = [System.Drawing.Bitmap]::new($srcPng)
$w = $srcImg.Width
$h = $srcImg.Height

# Buat BMP 24-bit baru dengan background putih
$dst = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)

for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) {
        $px = $srcImg.GetPixel($x, $y)
        if ($px.A -lt 128) {
            # Pixel transparan di PNG -> tulis putih (chroma-key untuk OpenGL)
            $dst.SetPixel($x, $y, [System.Drawing.Color]::White)
        } else {
            # Pixel solid -> tulis warna asli (tanpa alpha)
            $dst.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $px.R, $px.G, $px.B))
        }
    }
}

$srcImg.Dispose()
$dst.Save($dstBmp, [System.Drawing.Imaging.ImageFormat]::Bmp)
$dst.Dispose()
Write-Host "Done: ${w}x${h} - alpha diterapkan sebagai chroma-key putih"
