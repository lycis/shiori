Output "shiori-demo.gif"

Set Shell "cmd"
Set FontSize 20
Set Width 900
Set Height 520
Set TypingSpeed 35ms

Type ".\shiori.exe init"
Enter
Sleep 1s

Type ".\shiori.exe add --topic ideas `"build a tiny local-first notes tool`""
Enter
Sleep 1s

Type ".\shiori.exe todo add --due tomorrow `"write the first release notes`" `"#shiori`""
Enter
Sleep 1s

Type ".\shiori.exe today"
Enter
Sleep 4s