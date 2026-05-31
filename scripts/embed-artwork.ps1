[CmdletBinding()]
param(
    [string]$BackgroundPath = (Join-Path $PSScriptRoot '..\bg3.png'),
    [string]$ArrowPath = (Join-Path $PSScriptRoot '..\arrow.png'),
    [string]$OutputDir = (Join-Path $PSScriptRoot '..\src'),
    [int]$BackgroundWidth = 480,
    [int]$BackgroundHeight = 640,
    [int]$ArrowSize = 192,
    [int]$ArrowFrameCount = 31,
    [double]$ArrowSweepDegrees = 270.0,
    [int]$ArrowAlphaThreshold = 36,
    [double]$ArrowAlphaScale = 3.0
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

function Resolve-RequiredPath {
    param([string]$Path)

    $resolved = Resolve-Path -LiteralPath $Path -ErrorAction Stop
    return $resolved.Path
}

function New-BitmapCanvas {
    param(
        [int]$Width,
        [int]$Height
    )

    return [System.Drawing.Bitmap]::new(
        $Width,
        $Height,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
}

function Draw-ScaledImage {
    param(
        [System.Drawing.Bitmap]$Source,
        [System.Drawing.Bitmap]$Destination,
        [System.Drawing.Rectangle]$SourceRect
    )

    $graphics = [System.Drawing.Graphics]::FromImage($Destination)
    try {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

        $destinationRect = [System.Drawing.Rectangle]::new(
            0,
            0,
            $Destination.Width,
            $Destination.Height)

        $graphics.DrawImage(
            $Source,
            $destinationRect,
            $SourceRect,
            [System.Drawing.GraphicsUnit]::Pixel)
    }
    finally {
        $graphics.Dispose()
    }
}

function Get-CenterSquareRect {
    param([System.Drawing.Bitmap]$Bitmap)

    if ($Bitmap.Width -gt $Bitmap.Height) {
        $size = $Bitmap.Height
        $x = [int](($Bitmap.Width - $size) / 2)
        return [System.Drawing.Rectangle]::new($x, 0, $size, $size)
    }

    $size = $Bitmap.Width
    $y = [int](($Bitmap.Height - $size) / 2)
    return [System.Drawing.Rectangle]::new(0, $y, $size, $size)
}

function Convert-ArrowToTransparentChalk {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [int]$Threshold,
        [double]$AlphaScale
    )

    for ($y = 0; $y -lt $Bitmap.Height; $y++) {
        for ($x = 0; $x -lt $Bitmap.Width; $x++) {
            $color = $Bitmap.GetPixel($x, $y)
            $brightness = [int](0.2126 * $color.R + 0.7152 * $color.G + 0.0722 * $color.B)
            $alpha = [int](($brightness - $Threshold) * $AlphaScale)
            $alpha = [Math]::Max(0, [Math]::Min(255, $alpha))
            $Bitmap.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($alpha, 245, 245, 238))
        }
    }
}

function New-RotatedBitmap {
    param(
        [System.Drawing.Bitmap]$Source,
        [double]$Degrees
    )

    $destination = New-BitmapCanvas -Width $Source.Width -Height $Source.Height
    $graphics = [System.Drawing.Graphics]::FromImage($destination)

    try {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
        $graphics.TranslateTransform($Source.Width / 2.0, $Source.Height / 2.0)
        $graphics.RotateTransform([single]$Degrees)
        $graphics.TranslateTransform(-$Source.Width / 2.0, -$Source.Height / 2.0)
        $graphics.DrawImage($Source, 0, 0, $Source.Width, $Source.Height)
    }
    finally {
        $graphics.Dispose()
    }

    return $destination
}

function New-ArrowFilmstrip {
    param(
        [System.Drawing.Bitmap]$Arrow,
        [int]$FrameCount,
        [double]$SweepDegrees
    )

    if ($FrameCount -lt 3) {
        throw 'ArrowFrameCount must be at least 3.'
    }

    $filmstrip = New-BitmapCanvas -Width $Arrow.Width -Height ($Arrow.Height * $FrameCount)
    $graphics = [System.Drawing.Graphics]::FromImage($filmstrip)

    try {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $halfSweep = $SweepDegrees / 2.0

        for ($frame = 0; $frame -lt $FrameCount; $frame++) {
            $normalized = if ($FrameCount -eq 1) { 0.0 } else { $frame / ($FrameCount - 1.0) }
            $degrees = -$halfSweep + $normalized * $SweepDegrees
            $rotated = New-RotatedBitmap -Source $Arrow -Degrees $degrees

            try {
                $graphics.DrawImage($rotated, 0, $frame * $Arrow.Height, $Arrow.Width, $Arrow.Height)
            }
            finally {
                $rotated.Dispose()
            }
        }
    }
    finally {
        $graphics.Dispose()
    }

    return $filmstrip
}

function Write-BgraArtwork {
    param(
        [System.Drawing.Bitmap]$Bitmap,
        [string]$OutputDir,
        [string]$FileStem,
        [string]$Namespace,
        [string]$Symbol,
        [string]$HeaderGuard
    )

    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null

    $dataSize = $Bitmap.Width * $Bitmap.Height * 4
    $headerPath = Join-Path $OutputDir "$FileStem.hpp"
    $sourcePath = Join-Path $OutputDir "$FileStem.cpp"

    $header = @"
/* Auto-generated by scripts/embed-artwork.ps1. */

#ifndef $HeaderGuard
#define $HeaderGuard

namespace $Namespace
{
    extern const char* ${Symbol}Data;
    const unsigned int ${Symbol}DataSize = $dataSize;
    const unsigned int ${Symbol}Width = $($Bitmap.Width);
    const unsigned int ${Symbol}Height = $($Bitmap.Height);
}

#endif // $HeaderGuard
"@

    Set-Content -LiteralPath $headerPath -Value $header -Encoding ASCII

    $builder = [System.Text.StringBuilder]::new()
    [void]$builder.AppendLine('/* Auto-generated by scripts/embed-artwork.ps1. */')
    [void]$builder.AppendLine()
    [void]$builder.AppendLine("#include `"$FileStem.hpp`"")
    [void]$builder.AppendLine()
    [void]$builder.AppendLine("static const unsigned char temp_$Symbol[] = {")

    $column = 0

    for ($y = 0; $y -lt $Bitmap.Height; $y++) {
        [void]$builder.Append(' ')

        for ($x = 0; $x -lt $Bitmap.Width; $x++) {
            $color = $Bitmap.GetPixel($x, $y)
            [void]$builder.AppendFormat(
                ' {0,3}, {1,3}, {2,3}, {3,3},',
                $color.B,
                $color.G,
                $color.R,
                $color.A)

            $column++

            if ($column -ge 5) {
                [void]$builder.AppendLine()

                if (!($x -eq $Bitmap.Width - 1 -and $y -eq $Bitmap.Height - 1)) {
                    [void]$builder.Append(' ')
                }

                $column = 0
            }
        }

        if ($column -ne 0) {
            [void]$builder.AppendLine()
            $column = 0
        }
    }

    [void]$builder.AppendLine('};')
    [void]$builder.AppendLine("const char* ${Namespace}::${Symbol}Data = (const char*)temp_$Symbol;")

    Set-Content -LiteralPath $sourcePath -Value $builder.ToString() -Encoding ASCII

    return @($headerPath, $sourcePath)
}

$backgroundInput = Resolve-RequiredPath $BackgroundPath
$arrowInput = Resolve-RequiredPath $ArrowPath
$outputRoot = [System.IO.Path]::GetFullPath($OutputDir)

$backgroundSource = [System.Drawing.Bitmap]::new($backgroundInput)
$backgroundBitmap = New-BitmapCanvas -Width $BackgroundWidth -Height $BackgroundHeight

try {
    $backgroundRect = [System.Drawing.Rectangle]::new(
        0,
        0,
        $backgroundSource.Width,
        $backgroundSource.Height)

    Draw-ScaledImage -Source $backgroundSource -Destination $backgroundBitmap -SourceRect $backgroundRect

    $backgroundFiles = Write-BgraArtwork `
        -Bitmap $backgroundBitmap `
        -OutputDir $outputRoot `
        -FileStem 'BackgroundArtwork' `
        -Namespace 'BackgroundArtwork' `
        -Symbol 'bg' `
        -HeaderGuard 'BACKGROUND_ARTWORK_HPP_INCLUDED'
}
finally {
    $backgroundBitmap.Dispose()
    $backgroundSource.Dispose()
}

$arrowSource = [System.Drawing.Bitmap]::new($arrowInput)
$arrowBitmap = New-BitmapCanvas -Width $ArrowSize -Height $ArrowSize
$arrowFilmstrip = $null

try {
    Draw-ScaledImage -Source $arrowSource -Destination $arrowBitmap -SourceRect (Get-CenterSquareRect $arrowSource)
    Convert-ArrowToTransparentChalk `
        -Bitmap $arrowBitmap `
        -Threshold $ArrowAlphaThreshold `
        -AlphaScale $ArrowAlphaScale

    $arrowFilmstrip = New-ArrowFilmstrip `
        -Arrow $arrowBitmap `
        -FrameCount $ArrowFrameCount `
        -SweepDegrees $ArrowSweepDegrees

    $arrowFiles = Write-BgraArtwork `
        -Bitmap $arrowFilmstrip `
        -OutputDir $outputRoot `
        -FileStem 'ArrowArtwork' `
        -Namespace 'ArrowArtwork' `
        -Symbol 'arrow' `
        -HeaderGuard 'ARROW_ARTWORK_HPP_INCLUDED'
}
finally {
    if ($null -ne $arrowFilmstrip) {
        $arrowFilmstrip.Dispose()
    }

    $arrowBitmap.Dispose()
    $arrowSource.Dispose()
}

Write-Host "Embedded artwork:"
($backgroundFiles + $arrowFiles) | ForEach-Object {
    $item = Get-Item -LiteralPath $_
    Write-Host ("  {0} ({1:n0} bytes)" -f $item.FullName, $item.Length)
}
