#!/usr/bin/env python3

from __future__ import annotations

import shutil
import struct
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SVG_PATH = ROOT / "assets" / "app-icon.svg"
PNG_DIR = ROOT / "assets" / "icons" / "png"
ICONSET_DIR = ROOT / "assets" / "icons" / "mac.iconset"
ICNS_PATH = ROOT / "assets" / "icons" / "help-me-print.icns"
ICO_PATH = ROOT / "packaging" / "windows" / "help-me-print.ico"
TMP_1024 = Path("/private/tmp/help-me-print-icon-1024.png")


def run(*args: str) -> None:
    subprocess.run(list(args), check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


def write_ico(path: Path, png_paths: list[Path]) -> None:
    images = [p.read_bytes() for p in png_paths]
    header = struct.pack("<HHH", 0, 1, len(images))
    entries = []
    offset = 6 + len(images) * 16
    for png_path, data in zip(png_paths, images):
        size = int(png_path.stem.split("-")[-1])
        width = 0 if size >= 256 else size
        height = 0 if size >= 256 else size
        entries.append(struct.pack("<BBBBHHII", width, height, 0, 0, 1, 32, len(data), offset))
        offset += len(data)
    path.write_bytes(header + b"".join(entries) + b"".join(images))


def main() -> None:
    PNG_DIR.mkdir(parents=True, exist_ok=True)
    ICONSET_DIR.mkdir(parents=True, exist_ok=True)
    ICO_PATH.parent.mkdir(parents=True, exist_ok=True)

    run("qlmanage", "-t", "-s", "1024", "-o", "/private/tmp", str(SVG_PATH))
    rendered = Path("/private/tmp") / f"{SVG_PATH.name}.png"
    shutil.copyfile(rendered, TMP_1024)

    sizes = [16, 32, 64, 128, 256, 512, 1024]
    for size in sizes:
        out_path = PNG_DIR / f"icon-{size}.png"
        if size == 1024:
            shutil.copyfile(TMP_1024, out_path)
            continue
        run("sips", "-z", str(size), str(size), str(TMP_1024), "--out", str(out_path))

    iconset_map = {
        "icon_16x16.png": 16,
        "icon_16x16@2x.png": 32,
        "icon_32x32.png": 32,
        "icon_32x32@2x.png": 64,
        "icon_128x128.png": 128,
        "icon_128x128@2x.png": 256,
        "icon_256x256.png": 256,
        "icon_256x256@2x.png": 512,
        "icon_512x512.png": 512,
        "icon_512x512@2x.png": 1024,
    }
    for name, size in iconset_map.items():
        shutil.copyfile(PNG_DIR / f"icon-{size}.png", ICONSET_DIR / name)

    write_ico(ICO_PATH, [PNG_DIR / f"icon-{size}.png" for size in [16, 32, 64, 128, 256]])

    try:
        run("iconutil", "-c", "icns", str(ICONSET_DIR), "-o", str(ICNS_PATH))
    except subprocess.CalledProcessError:
        pass


if __name__ == "__main__":
    main()
