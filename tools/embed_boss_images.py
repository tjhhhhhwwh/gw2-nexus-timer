#!/usr/bin/env python3
"""Embed the copied JPEG files; no network access or external image paths at runtime."""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def generate(output):
    bosses = json.loads((ROOT / "data/bosses.json").read_text(encoding="utf-8"))["bosses"]
    lines = ["// Generated from assets/bosses/*.jpg; do not edit.", "#pragma once",
             "#include <array>", "#include <cstddef>",
             "namespace world_timer {",
             "struct BossImage { const char* id; const unsigned char* data; std::size_t size; };"]
    ids = set()
    for boss in bosses:
        key = boss["id"]
        if not re.fullmatch(r"b[0-9]{2}", key) or key in ids:
            raise ValueError("Invalid or duplicate boss image ID")
        ids.add(key)
        data = (ROOT / "assets/bosses" / (key + ".jpg")).read_bytes()
        if not data.startswith(b"\xff\xd8\xff") or not data.endswith(b"\xff\xd9"):
            raise ValueError("Invalid JPEG: " + key)
        lines.append("inline constexpr unsigned char image_" + key + "[] = {")
        for start in range(0, len(data), 24):
            lines.append("    " + ",".join(str(b) for b in data[start:start + 24]) + ",")
        lines.append("};")
    lines.append("inline constexpr std::array<BossImage, " + str(len(bosses)) + "> kBossImages = {{")
    for boss in bosses:
        key = boss["id"]
        lines.append('    {"' + key + '", image_' + key + ', sizeof(image_' + key + ')},')
    lines.append("}};")
    miniapp = (ROOT / "assets/miniapp/entry.jpg").read_bytes()
    if not miniapp.startswith(b"\xff\xd8\xff") or not miniapp.endswith(b"\xff\xd9"):
        raise ValueError("Invalid mini-program code JPEG")
    lines.append("inline constexpr unsigned char image_miniapp[] = {")
    for start in range(0, len(miniapp), 24):
        lines.append("    " + ",".join(str(b) for b in miniapp[start:start + 24]) + ",")
    lines.extend(["};", 'inline constexpr BossImage kMiniAppCode = {"miniapp", image_miniapp, sizeof(image_miniapp)};',
                  "} // namespace world_timer", ""])
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True)
    generate(parser.parse_args().output)
