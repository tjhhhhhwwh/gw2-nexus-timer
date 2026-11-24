#!/usr/bin/env python3
"""Optional macOS/Linux -> Windows x64 DLL build using uv + pinned Zig."""
import argparse
import hashlib
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument("--github-repository", default="tjhhhhhwwh/gw2-nexus-timer", help="Optional owner/repository; empty disables updates")
args = parser.parse_args()
if args.github_repository and not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", args.github_repository):
    parser.error("GitHub repository must be owner/repository")
subprocess.run(["python3", "tools/generate_data.py", "--check"], cwd=ROOT, check=True)
(ROOT / "dist").mkdir(exist_ok=True)
subprocess.run(["python3", "tools/embed_boss_images.py", "--output",
                "build/generated/boss_images.generated.h"], cwd=ROOT, check=True)
command = ["uv", "run", "--no-project", "--with", "ziglang==0.14.1", "python", "-m", "ziglang", "c++",
           "-target", "x86_64-windows-gnu", "-std=c++17", "-O2", "-shared",
           "-DWIN32_LEAN_AND_MEAN", "-DNOMINMAX", "-Isrc", "-Ithird_party/nexus", "-Ithird_party/imgui",
           "-Ibuild/generated"]
if args.github_repository:
    command.append('-DGW2_TIMER_GITHUB_URL="https://github.com/' + args.github_repository + '"')
command.extend(["src/addon.cpp", "src/clipboard_win.cpp", "src/schedule.cpp",
                "third_party/imgui/imgui.cpp", "third_party/imgui/imgui_draw.cpp",
                "third_party/imgui/imgui_tables.cpp", "third_party/imgui/imgui_widgets.cpp",
                "-luser32", "-o", "dist/gw2_nexus_timer.dll"])
subprocess.run(command, cwd=ROOT, check=True)
output = ROOT / "dist/gw2_nexus_timer.dll"
print("Built", output)
print("SHA256", hashlib.sha256(output.read_bytes()).hexdigest())
