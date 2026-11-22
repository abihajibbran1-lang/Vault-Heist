$ErrorActionPreference = 'Stop'
$repoPath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$releaseName = 'Vault-Heist-review-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
$releasePath = Join-Path $repoPath ('dist\' + $releaseName)
if (-not $releasePath.StartsWith((Join-Path $repoPath 'dist') + '\'))
{
    throw 'Release target is outside the intended dist directory.'
}
foreach ($required in @('build\vault-heist', 'docs\i253017-i252125.pdf', 'docs\benchmarks\trees.csv', 'docs\benchmarks\heaps.csv'))
{
    if (-not (Test-Path -LiteralPath (Join-Path $repoPath $required)))
    {
        throw "Missing release artifact: $required"
    }
}
New-Item -ItemType Directory -Path $releasePath | Out-Null
foreach ($directory in @('include', 'src', 'assets', 'docs', 'tests', '.vscode'))
{
    Copy-Item -LiteralPath (Join-Path $repoPath $directory) -Destination (Join-Path $releasePath $directory) -Recurse
}
foreach ($file in @('README.md', 'AGENTS.md', '.gitignore', '.gitattributes', 'run.sh'))
{
    Copy-Item -LiteralPath (Join-Path $repoPath $file) -Destination (Join-Path $releasePath $file)
}
New-Item -ItemType Directory -Path (Join-Path $releasePath 'data\saves') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoPath 'data\levels') -Destination (Join-Path $releasePath 'data\levels') -Recurse
Copy-Item -LiteralPath (Join-Path $repoPath 'data\saves\.gitkeep') -Destination (Join-Path $releasePath 'data\saves\.gitkeep')
New-Item -ItemType Directory -Path (Join-Path $releasePath 'build') | Out-Null
Copy-Item -LiteralPath (Join-Path $repoPath 'build\vault-heist') -Destination (Join-Path $releasePath 'build\vault-heist')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zipPath = $releasePath + '.zip'
[System.IO.Compression.ZipFile]::CreateFromDirectory($releasePath, $zipPath)
$archive = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try
{
    $levelCount = @($archive.Entries | Where-Object { $_.FullName -match '^data[/\\]levels[/\\]act[1-4]-level[1-5]\.json$' }).Count
    if ($levelCount -ne 20)
    {
        throw "Archive has $levelCount level definitions, expected 20."
    }
    foreach ($entry in $archive.Entries)
    {
        $stream = $entry.Open()
        try
        {
            $buffer = New-Object byte[] 65536
            while ($stream.Read($buffer, 0, $buffer.Length) -gt 0) { }
        }
        finally
        {
            $stream.Dispose()
        }
    }
}
finally
{
    $archive.Dispose()
}
Write-Output "Review archive: $zipPath"
Write-Output 'Verified readable entries and all 20 levels; player saves and Git history excluded.'
Write-Output 'Rename with the real two student IDs and section before official submission.'
