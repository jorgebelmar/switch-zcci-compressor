Add-Type -AssemblyName System.Drawing
$srcPath = "C:\Users\elcok\.gemini\antigravity-ide\brain\b63d2be9-4817-4d7c-a611-2e0602f9415a\switch_zcci_icon_1784696092124.png"
$dstPng = "c:\Users\elcok\OneDrive\Escritorio\azahar-2126.0-rc3\switch_zcci_compressor\icon.png"
$dstJpg = "c:\Users\elcok\OneDrive\Escritorio\azahar-2126.0-rc3\switch_zcci_compressor\icon.jpg"

$img = [System.Drawing.Image]::FromFile($srcPath)
$bmp = New-Object System.Drawing.Bitmap 256, 256
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.DrawImage($img, 0, 0, 256, 256)

$bmp.Save($dstPng, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Save($dstJpg, [System.Drawing.Imaging.ImageFormat]::Jpeg)

$g.Dispose()
$bmp.Dispose()
$img.Dispose()
Write-Host "ICON CONVERTED TO BOTH 256x256 PNG AND JPG SUCCESSFULLY!"
