#!/usr/bin/env python3
"""Contract test for global content-addressed legacy asset deduplication."""
from __future__ import annotations

import hashlib
import re
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IMPORTER = ROOT / "tools" / "import_legacy_winding_reference.py"


def write(path: Path, value: str | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(value, bytes):
        path.write_bytes(value)
    else:
        path.write_text(value, encoding="utf-8")


def main() -> None:
    with tempfile.TemporaryDirectory() as temp:
        base = Path(temp)
        desktop = base / "desktop"
        mobile = base / "mobile"
        output = base / "reference"

        write(
            desktop / "page.html",
            '<html><title>D</title><body>'
            '<img src="images/a.bin"><img src="images/alias.bin">'
            '<img src="images/unique.bin"><a href="images/same.dat">data</a>'
            '</body></html>',
        )
        write(
            mobile / "page.html",
            '<html><title>M</title><body><img src="images/b.bin"></body></html>',
        )
        write(desktop / "images/a.bin", b"same")
        write(desktop / "images/alias.bin", b"same")
        write(mobile / "images/b.bin", b"same")
        write(desktop / "images/same.dat", b"same")
        write(desktop / "images/unique.bin", b"unique")

        subprocess.run(
            [
                sys.executable,
                str(IMPORTER),
                "--desktop-source",
                str(desktop),
                "--mobile-source",
                str(mobile),
                "--output",
                str(output),
            ],
            check=True,
        )

        shared = sorted(path for path in (output / "shared/assets").rglob("*") if path.is_file())
        assert len(shared) == 1
        assert re.fullmatch(r"[0-9a-f]{64}\.bin", shared[0].name)
        assert shared[0].read_bytes() == b"same"
        assert (output / "desktop/assets/images/unique.bin").read_bytes() == b"unique"
        assert (output / "desktop/assets/images/same.dat").read_bytes() == b"same"

        shared_url = f"/sites/reference/shared/assets/{shared[0].name}"
        desktop_html = (output / "desktop/pages/page.html").read_text(encoding="utf-8")
        mobile_html = (output / "mobile/pages/page.html").read_text(encoding="utf-8")
        assert desktop_html.count(shared_url) == 2
        assert shared_url in mobile_html

        groups: dict[tuple[str, str], list[Path]] = defaultdict(list)
        for subtree in ("shared/assets", "desktop/assets", "mobile/assets"):
            for path in (output / subtree).rglob("*"):
                if path.is_file():
                    groups[(hashlib.sha256(path.read_bytes()).hexdigest(), path.suffix.lower())].append(path)
        assert all(len(paths) == 1 for paths in groups.values())

    print("reference import asset dedup contracts: OK")


if __name__ == "__main__":
    main()
