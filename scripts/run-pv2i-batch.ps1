[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateNotNullOrEmpty()]
    [string]$InputDirectory,

    [Parameter(Position = 1)]
    [ValidateRange(0, [int]::MaxValue)]
    [int]$Threshold,

    [Parameter(Position = 2)]
    [ValidateRange(1, [int]::MaxValue)]
    [int]$FrameSkip,

    [string]$OutputRoot = "",

    [switch]$Recurse
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $PWD.ProviderPath $Path))
}

function Get-SafeName {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    $invalidChars = [System.IO.Path]::GetInvalidFileNameChars()
    $safeName = $Name
    foreach ($char in $invalidChars) {
        $safeName = $safeName.Replace($char.ToString(), "_")
    }
    return $safeName
}

if (-not (Test-Path -LiteralPath $InputDirectory -PathType Container)) {
    throw "Input directory does not exist: $InputDirectory"
}

$inputDirectoryFullPath = Resolve-FullPath $InputDirectory
$Pv2iExe = Join-Path $PSScriptRoot "PV2i.exe"

if (-not (Test-Path -LiteralPath $Pv2iExe -PathType Leaf)) {
    throw "PV2i.exe does not exist next to this script: $Pv2iExe"
}

$videoExtensions = @(".mp4", ".mov", ".mkv", ".avi", ".wmv", ".flv", ".m4v", ".webm")
$childItemParams = @{
    LiteralPath = $inputDirectoryFullPath
    File = $true
}

if ($Recurse) {
    $childItemParams["Recurse"] = $true
}

$videos = Get-ChildItem @childItemParams |
    Where-Object { $videoExtensions -contains $_.Extension.ToLowerInvariant() } |
    Sort-Object FullName

if ($videos.Count -eq 0) {
    Write-Host "No supported video files found in: $inputDirectoryFullPath"
    exit 0
}

$hasThreshold = $PSBoundParameters.ContainsKey("Threshold")
$hasFrameSkip = $PSBoundParameters.ContainsKey("FrameSkip")
$hasOutputRoot = -not [string]::IsNullOrWhiteSpace($OutputRoot)

if ($hasOutputRoot) {
    $OutputRoot = Resolve-FullPath $OutputRoot
    if (-not (Test-Path -LiteralPath $OutputRoot -PathType Container)) {
        New-Item -ItemType Directory -Path $OutputRoot | Out-Null
    }
}

Write-Host "PV2i: $Pv2iExe"
Write-Host "Input: $inputDirectoryFullPath"
Write-Host "Files: $($videos.Count)"

$failed = 0

foreach ($video in $videos) {
    $arguments = @("--input", $video.FullName)

    if ($hasOutputRoot) {
        $outputFolderName = Get-SafeName $video.BaseName
        $outputFolder = Join-Path $OutputRoot $outputFolderName
        $arguments += @("--output", $outputFolder)
    }

    if ($hasThreshold) {
        $arguments += @("--threshold", $Threshold)
    }

    if ($hasFrameSkip) {
        $arguments += @("--frame-skip", $FrameSkip)
    }

    Write-Host ""
    Write-Host "Processing: $($video.FullName)"
    Write-Host "Args: $($arguments -join ' ')"

    & $Pv2iExe @arguments
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "PV2i failed with exit code $($LASTEXITCODE): $($video.FullName)"
        $failed++
    }
}

if ($failed -gt 0) {
    throw "$failed file(s) failed."
}

Write-Host ""
Write-Host "Done."
