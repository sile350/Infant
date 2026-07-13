#!/usr/bin/env python3
"""Extract puzzle sprite layouts from original puzzles.cs into JSON files."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PUZZLES_CS = (
    ROOT.parent
    / "old_project"
    / "serv9 2025"
    / "WindowsFormsApp1"
    / "Упражнения1"
    / "puzzles.cs"
)
OUT_BASE = ROOT / "assets" / "ex"

SET_SPRITE_RE = re.compile(
    r'setSprite\([^"]*"\\\\ex\\\\([^\\\\]+)\\\\([^"]+)"\)',
)
SET_SPRITE_DYN_RE = re.compile(
    r'setSprite\([^"]*"\\\\ex\\\\([^\\\\]+)\\\\"\s*\+\s*([^+]+)\s*\+\s*"([^"]+)"\)',
)
X_RE = re.compile(r"\.x\s*=\s*([^;]+);")
Y_RE = re.compile(r"\.y\s*=\s*([^;]+);")
NAME_BLOCK_RE = re.compile(
    r'if\s*\(name\s*==\s*"([^"]+)"(?:\s*&&\s*param\s*==\s*"([^"]+)")?\)'
)
PARAM_BLOCK_RE = re.compile(r'if\s*\(param\s*==\s*"([^"]+)"\)')
TRAF_SET_RE = re.compile(r'traf\.setSprite\([^"]*"\\\\ex\\\\([^\\\\]+)\\\\([^"]+)"\)')


def eval_coord(expr: str) -> int:
    expr = expr.strip()
    expr = expr.replace("dy", "0").replace("dx", "0")
    expr = re.sub(r"\s+", "", expr)
    allowed = set("0123456789+-*/().")
    if not expr or not all(ch in allowed for ch in expr):
        return 0
    try:
        return int(eval(expr, {"__builtins__": {}}, {}))
    except Exception:
        return 0


def sanitize_step(step: str) -> str:
    for ch in ('/', '\\', ':', '*', '?', '"', '<', '>', '|', ' '):
        step = step.replace(ch, "_")
    return step or "default"


def extract_blocks(text: str) -> list[tuple[str, str, str]]:
    blocks: list[tuple[str, str, str]] = []
    matches = list(NAME_BLOCK_RE.finditer(text))
    for i, match in enumerate(matches):
        name = match.group(1)
        base_param = match.group(2) or ""
        start = match.end()
        end = matches[i + 1].start() if i + 1 < len(matches) else len(text)
        body = text[start:end]

        param_matches = list(PARAM_BLOCK_RE.finditer(body))
        if not param_matches:
            blocks.append((name, base_param or "default", body))
            continue

        for j, param_match in enumerate(param_matches):
            param = param_match.group(1)
            sub_start = param_match.end()
            sub_end = (
                param_matches[j + 1].start()
                if j + 1 < len(param_matches)
                else len(body)
            )
            blocks.append((name, param, body[sub_start:sub_end]))
    return blocks


def parse_sprites_from_body(name: str, body: str) -> list[dict]:
    sprites: list[dict] = []
    lines = body.splitlines()
    pending_file: str | None = None
    pending_x: int | None = None

    for line in lines:
        static_match = SET_SPRITE_RE.search(line)
        dynamic_match = SET_SPRITE_DYN_RE.search(line)
        if static_match:
            ex_name, file_name = static_match.groups()
            if ex_name == name:
                pending_file = file_name
                pending_x = None
            continue
        if dynamic_match:
            ex_name, _expr, suffix = dynamic_match.groups()
            if ex_name == name and suffix == ".png":
                pending_file = None
            continue

        if pending_file:
            x_match = X_RE.search(line)
            if x_match:
                pending_x = eval_coord(x_match.group(1))
                continue
            y_match = Y_RE.search(line)
            if y_match and pending_x is not None:
                sprites.append(
                    {
                        "file": pending_file,
                        "x": pending_x,
                        "y": eval_coord(y_match.group(1)),
                    }
                )
                pending_file = None
                pending_x = None
    return sprites


def parse_template(name: str, body: str) -> dict | None:
    lines = body.splitlines()
    pending_file: str | None = None
    for line in lines:
        traf_match = TRAF_SET_RE.search(line)
        if traf_match:
            ex_name, file_name = traf_match.groups()
            if ex_name == name:
                pending_file = file_name
            continue
        if pending_file:
            x_match = X_RE.search(line)
            if x_match:
                x_val = eval_coord(x_match.group(1))
                continue
            y_match = Y_RE.search(line)
            if y_match:
                return {
                    "file": pending_file,
                    "x": x_val,
                    "y": eval_coord(y_match.group(1)),
                }
    return None


def parse_block(name: str, param: str, body: str) -> dict | None:
    sprites = parse_sprites_from_body(name, body)
    template = parse_template(name, body)
    if not sprites and not template:
        return None

    layout = {
        "rotate": name in {"1.19", "1.20", "1.21", "1.22", "3.1.8", "3.1.16"},
        "select": False,
        "showTemplate": template is not None,
        "sprites": sprites,
    }
    if template:
        layout["template"] = template
    return layout


def main() -> None:
    if not PUZZLES_CS.exists():
        raise SystemExit(f"Missing source file: {PUZZLES_CS}")

    text = PUZZLES_CS.read_text(encoding="utf-8", errors="ignore")
    blocks = extract_blocks(text)
    print(f"Found {len(blocks)} name blocks")
    written = 0
    for name, param, body in extract_blocks(text):
        layout = parse_block(name, param, body)
        if not layout:
            continue
        out_dir = OUT_BASE / name
        out_dir.mkdir(parents=True, exist_ok=True)
        step = param or "default"
        out_path = out_dir / f"puzzle_{sanitize_step(step)}.json"
        out_path.write_text(json.dumps(layout, ensure_ascii=False, indent=2), encoding="utf-8")
        written += 1
        print(f"Wrote {out_path.relative_to(ROOT)} ({len(layout['sprites'])} sprites)")

    print(f"Done: {written} layout files")


if __name__ == "__main__":
    main()
