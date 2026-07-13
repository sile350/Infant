from pathlib import Path
import re
text = Path(r"d:/projects/DokitLab/old_project/serv9 2025/WindowsFormsApp1/Упражнения1/puzzles.cs").read_text(encoding="utf-8", errors="ignore")
line = [l for l in text.splitlines() if "setSprite" in l and "4.1.5" in l][0]
pat = re.compile(r'setSprite\(Application\.StartupPath \+ "\\\\ex\\\\([^\\\\]+)\\\\([^"]+)"\)')
print(pat.search(line))
