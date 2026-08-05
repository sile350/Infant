#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Normalize Date/Result/Note header cells to 200/471 (as in 1.7 / header.html)."""

import json
import re
import pathlib

ROOT = pathlib.Path(__file__).resolve().parents[1] / "assets" / "protocol_templates"
IDS = [
    "1.11", "1.12", "1.14", "1.15", "1.19", "1.20", "1.21", "1.22", "1.24",
    "1.27", "1.28", "1.29", "2.1", "2.2", "2.3", "2.11", "2.12",
    "3.1.7", "3.1.8", "3.1.15", "3.1.16", "3.1.20", "3.1.21", "3.1.23", "3.1.24",
    "3.3.1", "3.3.2", "3.3.3", "4.1.7",
]

DEFAULT_DATE = (
    "<tr><td width='200' valign='top'>Дата/специалист</td>"
    "<td width='471' valign='top'>{{DATE}}   {{USER}}</td></tr>"
)


def fix_summary_td_open(attrs: str, width: str) -> str:
    attrs = re.sub(r"\s*width\s*=\s*['\"][^'\"]*['\"]", "", attrs, flags=re.I)
    attrs = re.sub(r"\s*colspan\s*=\s*['\"][^'\"]*['\"]", "", attrs, flags=re.I)
    attrs = re.sub(r"\s*style\s*=\s*['\"][^'\"]*['\"]", "", attrs, flags=re.I)
    attrs = re.sub(r"\s*valign\s*=\s*['\"][^'\"]*['\"]", "", attrs, flags=re.I)
    return f" width='{width}' valign='top'{attrs}"


def cell_plain(html: str) -> str:
    text = re.sub(r"<[^>]+>", " ", html)
    text = re.sub(r"&nbsp;", " ", text, flags=re.I)
    return re.sub(r"\s+", " ", text).strip().lower()


def fix_date_row(html: str) -> str:
    if not html or not html.strip():
        return DEFAULT_DATE
    m = re.search(
        r"(<tr\b[^>]*>)\s*(<td\b)([^>]*>)([\s\S]*?</td>)\s*(<td\b)([^>]*>)([\s\S]*?</td>)\s*(</tr>)",
        html,
        re.I,
    )
    if not m:
        return html
    return (
        m.group(1)
        + m.group(2)
        + fix_summary_td_open(m.group(3), "200")
        + m.group(4)
        + m.group(5)
        + fix_summary_td_open(m.group(6), "471")
        + m.group(7)
        + m.group(8)
    )


def fix_initial_block(html: str) -> str:
    if not html:
        return html
    parts = re.split(r"(<!--s-->)", html, maxsplit=1, flags=re.I)
    summary = parts[0]
    rest = "".join(parts[1:]) if len(parts) > 1 else ""

    def repl_row(m: re.Match) -> str:
        label = m.group(4)
        plain = cell_plain(label)
        if "процесс выполнения" in plain:
            return (
                "<tr><td align='center' colspan='2' width='671' style='width:671px'>"
                + label
                + "</td></tr>"
            )
        if (
            ("дата" in plain and "специалист" in plain)
            or plain.startswith("результат")
            or plain.startswith("примечание")
        ):
            return (
                m.group(1)
                + m.group(2)
                + fix_summary_td_open(m.group(3), "200")
                + m.group(4)
                + m.group(5)
                + m.group(6)
                + fix_summary_td_open(m.group(7), "471")
                + m.group(8)
                + m.group(9)
                + m.group(10)
            )
        return m.group(0)

    single_process = re.compile(
        r"<tr\b[^>]*>\s*<td\b[^>]*>\s*"
        r"((?:<(?:p|div|span)\b[^>]*>\s*)*Процесс выполнения[\s\S]*?)</td>\s*</tr>",
        re.I,
    )
    summary = single_process.sub(
        lambda m: (
            "<tr><td align='center' colspan='2' width='671' style='width:671px'>"
            + m.group(1)
            + "</td></tr>"
        ),
        summary,
    )
    row_re = re.compile(
        r"(<tr\b[^>]*>)\s*(<td\b)([^>]*>)([\s\S]*?)(</td>)\s*"
        r"(<td\b)([^>]*>)([\s\S]*?)(</td>)\s*(</tr>)",
        re.I,
    )
    summary = row_re.sub(repl_row, summary)
    return summary + rest


def main() -> None:
    changed = []
    for id_ in IDS:
        path = ROOT / f"{id_}.json"
        data = json.loads(path.read_text(encoding="utf-8"))
        old_date = data.get("dateRow", "")
        old_init = data.get("initialBlock", "")
        data["dateRow"] = fix_date_row(old_date)
        data["initialBlock"] = fix_initial_block(old_init)
        if data["dateRow"] != old_date or data["initialBlock"] != old_init:
            path.write_text(
                json.dumps(data, ensure_ascii=False, indent=2) + "\n",
                encoding="utf-8",
            )
            changed.append(id_)
            print("FIXED", id_)
        else:
            print("SKIP", id_)
    print("Total fixed:", len(changed))


if __name__ == "__main__":
    main()
