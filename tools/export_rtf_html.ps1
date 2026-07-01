$rtf = 'd:\projects\DokitLab\infant\assets\htmls\anamnez.rtf'
$htmlOut = 'd:\projects\DokitLab\infant\assets\htmls\anamnez.html'
try {
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $doc = $word.Documents.Open($rtf)
    $doc.SaveAs([ref]$htmlOut, [ref]8)  # wdFormatHTML = 8
    $doc.Close($false)
    $word.Quit()
    Write-Output "OK $htmlOut"
} catch {
    Write-Output "ERR=$($_.Exception.Message)"
}
