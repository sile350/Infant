import pathlib
import re

src = pathlib.Path(__file__).resolve().parents[1] / "assets" / "htmls" / "anamnez.html"
text = src.read_text(encoding="utf-8")
match = re.search(r"(?is)<body[^>]*>(.*)</body>", text)
body = match.group(1) if match else text
patterns = [
    r"<!--\[if.*?\[endif\]-->",
    r"<o:p[^>]*>.*?</o:p>",
    r"\s+class=Mso\w+",
    r"\s+mso-[^:]+:[^;\"']+;?",
]
for pattern in patterns:
    body = re.sub(pattern, "", body, flags=re.I | re.S)
body = body.replace("Times New Roman CYR", "Times New Roman").replace("Calibri", "Times New Roman")
clean = (
    '<!DOCTYPE html><html><head><meta charset="utf-8"><style>'
    'body{font-family:"Times New Roman",serif;font-size:12pt;color:#000;background:#fff;margin:8px;}'
    "p{margin:0;line-height:85%;}"
    "</style></head><body>"
    + body
    + "</body></html>"
)
out = src.with_name("anamnez_clean.html")
out.write_text(clean, encoding="utf-8")
print(f"Wrote {out} ({len(clean)} bytes)")
