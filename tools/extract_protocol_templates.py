#!/usr/bin/env python3
"""Extract per-exercise protocol HTML templates from protocols.cs."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PROTOCOLS_CS = (
    ROOT
    / "old_project"
    / "serv9 2025"
    / "WindowsFormsApp1"
    / "Служебные"
    / "protocols.cs"
)
OUT_DIR = Path(__file__).resolve().parents[1] / "assets" / "protocol_templates"


def split_blocks(text: str) -> dict[str, str]:
    pattern = re.compile(r'if\s*\(\s*uname\s*==\s*"([^"]+)"\s*\)', re.MULTILINE)
    matches = list(pattern.finditer(text))
    blocks: dict[str, str] = {}
    for i, match in enumerate(matches):
        start = match.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        blocks[match.group(1)] = text[start:end]
    return blocks


def extract_date_row(block: str) -> str:
    patterns = [
        r'add\s*=\s*"(<tr><td[^>]*>Дата/специалист</td><td[^>]*>)"\s*\+\s*dateTime\.ToString\([^)]*\)\s*\+\s*"([^"]*)"\s*\+\s*userfio\s*\+\s*"([^"]*)</td></tr>"',
        r'add\s*=\s*"(<tr><td[^>]*>Дата/специалист</td><td[^>]*>)"\s*\+\s*dateTime\.ToString\([^)]*\)\s*\+\s*"([^"]*)"\s*\+\s*userfio\s*\+\s*"([^"]*)"\s*</td></tr>"',
        r'add\s*=\s*"(<tr><td align=\'left\' >Дата/специалист</td><td >)"\s*\+\s*dateTime\.ToString\([^)]*\)\s*\+\s*"([^"]*)"\s*\+\s*userfio\s*\+\s*"([^"]*)"\s*</td></tr>"',
        r'add\s*=\s*"(<tr><td align=\'left\' >Дата/специалист</td><td >)"\s*\+\s*dateTime\.ToString\([^)]*\)\s*\+\s*"([^"]*)"\s*\+\s*userfio\s*\+\s*""\+\s*"([^"]*)</td></tr>"',
        r'add\s*=\s*"(<tr><td[^>]*>Дата/специалист</td><td[^>]*colspan[^>]*>)"\s*\+\s*dateTime\.ToString\([^)]*\)\s*\+\s*"([^"]*)"\s*\+\s*userfio\s*\+\s*"([^"]*)</td></tr>"',
    ]
    for pattern in patterns:
        m = re.search(pattern, block)
        if m:
            return m.group(1) + "{{DATE}}" + m.group(2) + "{{USER}}" + m.group(3) + "</td></tr>"
    return ""


def extract_partly_false(block: str) -> str:
    m = re.search(
        r"if\s*\(\s*partly\s*==\s*false\s*\)\s*\{(.*?)if\s*\(\s*partly\s*==\s*true\s*\)",
        block,
        re.DOTALL,
    )
    if not m:
        return ""
    section = m.group(1)
    parts: list[str] = []
    for line in section.splitlines():
        mm = re.search(r'add\s*=\s*add\s*\+\s*"(.+)"\s*;', line)
        if mm:
            parts.append(mm.group(1))
    return "".join(parts)


def collect_add_lines(section: str) -> str:
    parts: list[str] = []
    for line in section.splitlines():
        mm = re.search(r'add\s*=\s*add\s*\+\s*"(.+)"\s*;', line)
        if mm:
            parts.append(mm.group(1))
            continue
        mm2 = re.search(r'add\s*=\s*"(.+)"\s*;\s*$', line.strip())
        if mm2 and ("<tr" in mm2.group(1) or mm2.group(1).strip().startswith("<td")):
            parts.append(mm2.group(1))
    return "".join(parts)


def extract_row_variants(block: str) -> dict[str, str] | None:
    variants: dict[str, str] = {}
    for m in re.finditer(
        r'if\s*\(\s*tmp\[0\]\s*==\s*"([^"]+)"\s*\)\s*\{(.*?)(?=\s*if\s*\(\s*tmp\[0\]\s*==|\s*protocol\s*=)',
        block,
        re.DOTALL,
    ):
        key = m.group(1)
        variants[key] = collect_add_lines(m.group(2))
    return variants if variants else None


def extract_row_section(block: str) -> str:
    m = re.search(
        r"string ttext = minutes \+ \":\" \+ sec \+ \" сек\";\s*(.*?)protocol = add;",
        block,
        re.DOTALL,
    )
    if not m:
        return ""
    return collect_add_lines(m.group(1))


def detect_score_kind(block: str, ex_id: str) -> str:
    if ex_id == "1.4":
        return "timed14"
    if "if (time <= 25)" in block and "bls = 10" in block:
        return "timed11"
    if "if (time <= 20" in block and "time >=65" in block:
        return "timed18"
    if "count == \"0\"" in block or 'count=="0"' in block.replace(" ", ""):
        return "timed14"
    if "idd1" in block and "bls = 4" in block:
        return "or_checkbox_4"
    if "bls.ToString() + \"(10)/\"" in block.replace(" ", "") or '(10)/" + level' in block:
        return "timed11_result"
    if "bls.ToString() + \"/\" + level" in block or 'bls + "/" + level' in block:
        return "timed18_result"
    if "(10)" in extract_partly_false(block) and "Баллы" in block:
        return "balls_manual"
    return "none"


def detect_kind(block: str, row: str, initial: str, variants: dict[str, str] | None) -> str:
    if variants:
        return "tmp0_variants"
    if "answers = additional.Split" in block or "answers[0]" in block:
        return "picture_answers"
    if "tmp3[0]" in row or "tmp2[0]" in row:
        return "wolf_542"
    if "tmp[0]" in row and "tmp[1]" in row:
        return "numbered"
    if "additional + \"/\" + ttext" in row.replace(" ", "") or "{{ADDITIONAL}}/{{TIME}}" in row:
        return "additional_time"
    if "additional + \"</div>" in row or "{{ADDITIONAL}}</div>" in row:
        return "additional_or_hlp"
    if "align='left'>" in row and "additional" in row:
        return "additional_time"
    if "скачать" in initial and "№" in initial:
        return "done_time_scan"
    if "скачать" in initial:
        return "scan_slots"
    if "Факт выполнения" in initial and "№" in initial:
        return "numbered"
    if "Факт выполнения" in initial:
        return "done_time"
    if "Баллы" in initial or "баллы" in initial.lower():
        if "Кол-во цифр" in initial:
            return "digits_421"
        if "Картинка" in initial:
            return "picture_answers"
        return "or_hlp_balls"
    if "Характер деятельности" in initial:
        return "or_hlp"
    return "custom"


def normalize_placeholders(s: str) -> str:
    if not s:
        return s
    replacements = [
        (r'"\s*\+\s*or\s*\+\s*"', "{{OR}}"),
        (r'"\s*\+\s*hlp\s*\+\s*"', "{{HLP}}"),
        (r"\+\s*or\s*\+", "{{OR}}"),
        (r"\+\s*hlp\s*\+", "{{HLP}}"),
        (r"\+\s*ttext\s*\+", "{{TIME}}"),
        (r"\+\s*tmp\[0\]\s*\+", "{{STEP}}"),
        (r"\+\s*tmp\[1\]\s*\+", "{{DONE}}"),
        (r"\+\s*tmp\[1\]\+\s*", "{{DONE}}"),
        (r'"\s*\+\s*tmp\[(\d+)\]\s*\+\s*"', r"{{TMP\1}}"),
        (r'"\s*\+\s*tmp\[(\d+)\]\+\s*"', r"{{TMP\1}}"),
        (r'"\s*\+\s*tmp\[(\d+)\]\+\s*"', r"{{TMP\1}}"),
        (r"\+\s*additional\s*\+", "{{ADDITIONAL}}"),
        (r"\+\s*bls\s*\+", "{{SCORE}}"),
        (r"\+\s*bls\.ToString\(\)\s*\+", "{{SCORE}}"),
        (r"\+\s*level\s*\+", "{{LEVEL}}"),
        (r"\+\s*result\s*\+", "{{RESULT}}"),
        (r"\+\s*tmp2\[(\d+)\]\s*\+", r"{{HELP\1}}"),
        (r"\+\s*tmp3\[(\d+)\]\s*\+", r"{{ANSWER\1}}"),
    ]
    for pattern, repl in replacements:
        s = re.sub(pattern, repl, s)
    s = s.replace("скачать1 скачать2 скачать3", "{{SCAN_SLOTS}}")
    s = re.sub(r"скачать\s*\+\s*additional", "скачать{{ADDITIONAL}}", s)
    s = re.sub(r"\s+скачать\s+", " {{SCAN}} ", s)
    s = re.sub(r'"\s*\+\s*bls\s*\+\s*"\(4\)"', "{{SCORE}}(4)", s)
    s = re.sub(r"\+\s*bls\s*\+\s*\"\(4\)\"", "{{SCORE}}(4)", s)
    s = re.sub(r'"\s*\+\s*tmp\[(\d+)\]\+', r"{{TMP\1}}", s)
    s = re.sub(r'"\s*\+\s*tmp\[(\d+)\]\s*\+', r"{{TMP\1}}", s)
    s = re.sub(r'"\s*{{', "{{", s)
    s = re.sub(r'}}\s*"', "}}", s)
    return s


def main() -> int:
    text = PROTOCOLS_CS.read_text(encoding="utf-8")
    blocks = split_blocks(text)
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    index: dict[str, str] = {}
    for ex_id, block in sorted(blocks.items()):
        date_row = extract_date_row(block)
        initial = extract_partly_false(block)
        variants = extract_row_variants(block)
        row = extract_row_section(block)
        kind = detect_kind(block, row, initial, variants)
        score_kind = detect_score_kind(block, ex_id)

        template: dict = {
            "id": ex_id,
            "kind": kind,
            "scoreKind": score_kind,
            "dateRow": normalize_placeholders(date_row),
            "initialBlock": normalize_placeholders(initial),
        }
        if variants:
            template["rowVariants"] = {
                k: normalize_placeholders(v) for k, v in variants.items()
            }
            template["rowTemplate"] = ""
        else:
            template["rowTemplate"] = normalize_placeholders(row)

        out_path = OUT_DIR / f"{ex_id}.json"
        out_path.write_text(json.dumps(template, ensure_ascii=False, indent=2), encoding="utf-8")
        index[ex_id] = kind

    (OUT_DIR / "index.json").write_text(
        json.dumps(index, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(f"Extracted {len(blocks)} templates to {OUT_DIR}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
