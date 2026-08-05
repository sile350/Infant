import sqlite3,re
c=sqlite3.connect(r"d:\projects\DokitLab\infant\build\Desktop_Qt_5_15_2_MinGW_64_bit-Debug\data\base.db")
for r in c.execute("SELECT id, uprid, length(pr) FROM protocols ORDER BY id DESC LIMIT 30"):
    print(r)
print("--- samples with Характер ---")
for id_, uprid, pr in c.execute("SELECT id, uprid, pr FROM protocols ORDER BY id DESC LIMIT 50"):
    if "Характер" in pr or "характер" in pr.lower():
        # find process table widths
        tables = re.findall(r"<table\b[^>]*>(.*?)</table>", pr, flags=re.I|re.S)
        print(f"\nid={id_} uprid={uprid} tables={len(tables)}")
        for i,t in enumerate(tables):
            if "Характер" in t or "характер" in t.lower() or "Баллы" in t:
                cols = re.findall(r"<col[^>]*width=['\"]?(\d+)", t, flags=re.I)
                tds = re.findall(r"<td([^>]*)>", t, flags=re.I)
                widths = re.findall(r"width=['\"](\d+)['\"]", t, flags=re.I)
                first_tr = re.search(r"<tr\b[^>]*>(.*?)</tr>", t, flags=re.I|re.S)
                hdr = re.sub(r"<[^>]+>"," ", first_tr.group(1) if first_tr else "")[:120]
                print(f"  t{i}: colgroup={cols} widths={widths[:8]} hdr={hdr!r}")
