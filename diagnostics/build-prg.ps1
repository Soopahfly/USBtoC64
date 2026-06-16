param(
  [string]$Source = "$PSScriptRoot\USBtoC64ModeTest.bas",
  [string]$Output = "$PSScriptRoot\U64TEST.prg"
)

$ErrorActionPreference = "Stop"

$tokens = [ordered]@{
  "PRINT#" = 0x98
  "PRINT" = 0x99
  "GOTO" = 0x89
  "IF" = 0x8B
  "POKE" = 0x97
  "THEN" = 0xA7
  "GET" = 0xA1
  "END" = 0x80
  "FOR" = 0x81
  "TO" = 0xA4
  "NEXT" = 0x82
  "+" = 0xAA
  "-" = 0xAB
  "*" = 0xAC
  "/" = 0xAD
  "^" = 0xAE
  "AND" = 0xAF
  ">" = 0xB1
  "=" = 0xB2
  "<" = 0xB3
  "ABS" = 0xB6
  "PEEK" = 0xC2
  "CHR$" = 0xC7
}

function Convert-BasicText {
  param([string]$Text)

  $bytes = New-Object System.Collections.Generic.List[byte]
  $inString = $false
  $i = 0

  while ($i -lt $Text.Length) {
    $ch = $Text[$i]
    if ($ch -eq '"') {
      $inString = -not $inString
      $bytes.Add([byte][char]$ch)
      $i++
      continue
    }

    if (-not $inString) {
      $matched = $false
      foreach ($key in $tokens.Keys) {
        if (($i + $key.Length) -le $Text.Length) {
          $part = $Text.Substring($i, $key.Length).ToUpperInvariant()
          if ($part -eq $key) {
            $bytes.Add([byte]$tokens[$key])
            $i += $key.Length
            $matched = $true
            break
          }
        }
      }
      if ($matched) { continue }
    }

    $code = [int][char]$ch
    if ($code -gt 255) {
      throw "Unsupported non-ASCII character '$ch' in BASIC source."
    }
    $bytes.Add([byte]$code)
    $i++
  }

  return $bytes.ToArray()
}

$sourceLines = Get-Content -LiteralPath $Source
$program = New-Object System.Collections.Generic.List[byte]
$loadAddress = 0x0801
$program.Add([byte]($loadAddress -band 0xFF))
$program.Add([byte](($loadAddress -shr 8) -band 0xFF))
$currentAddress = $loadAddress

foreach ($raw in $sourceLines) {
  $line = $raw.TrimEnd()
  if ($line -eq "") { continue }
  if ($line -notmatch '^(\d+)\s*(.*)$') {
    throw "Line does not start with a BASIC line number: $line"
  }

  $lineNumber = [int]$Matches[1]
  $body = Convert-BasicText $Matches[2]
  $nextAddress = $currentAddress + 5 + $body.Length

  $program.Add([byte]($nextAddress -band 0xFF))
  $program.Add([byte](($nextAddress -shr 8) -band 0xFF))
  $program.Add([byte]($lineNumber -band 0xFF))
  $program.Add([byte](($lineNumber -shr 8) -band 0xFF))
  foreach ($b in $body) {
    $program.Add([byte]$b)
  }
  $program.Add(0)

  $currentAddress = $nextAddress
}

$program.Add(0)
$program.Add(0)

[System.IO.File]::WriteAllBytes($Output, $program.ToArray())
Write-Host "Wrote $Output ($($program.Count) bytes)"
