$rtf = 'd:\projects\DokitLab\infant\assets\htmls\anamnez.rtf'
try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $doc = $word.Documents.Open($rtf)
    $text = $doc.Content.Text
    $doc.Close($false)
    $word.Quit()
    Write-Output "LEN=$($text.Length)"
    Write-Output $text.Substring(0, [Math]::Min(800, $text.Length))
} catch {
    Write-Output "ERR=$($_.Exception.Message)"
}
