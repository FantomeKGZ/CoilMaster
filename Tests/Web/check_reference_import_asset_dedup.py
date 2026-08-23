#!/usr/bin/env python3
"""Contract test for global content-addressed legacy asset deduplication."""
from __future__ import annotations

import hashlib
import importlib.util
import re
import subprocess
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
IMPORTER = ROOT / "tools" / "import_legacy_winding_reference.py"
CHECKER = ROOT / "tools" / "check_legacy_winding_reference.py"


def load_checker():
    spec = importlib.util.spec_from_file_location("reference_checker_contract", CHECKER)
    if spec is None or spec.loader is None:
        raise AssertionError(f"cannot load reference checker: {CHECKER}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


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
            '<link rel="stylesheet" href="styles/site.css">'
            '<div style="background:url(images/a.bin)"></div>'
            '<style>.inline{background:url("images/a.bin")}</style>'
            '<table background="images/a.bin"></table>'
            '<video poster="images/a.bin"></video>'
            '</body></html>',
        )
        write(
            mobile / "page.html",
            '<html><title>M</title><body><img src="images/b.bin">'
            '<link rel="stylesheet" href="styles/site.css">'
            '<div style="background:url(images/b.bin)"></div>'
            '<table background="images/b.bin"></table></body></html>',
        )
        write(desktop / "images/a.bin", b"same")
        write(desktop / "images/alias.bin", b"same")
        write(mobile / "images/b.bin", b"same")
        write(desktop / "images/same.dat", b"same")
        write(desktop / "images/unique.bin", b"unique")
        write(
            desktop / "styles/site.css",
            '@import "nested.css";body{background:url("../images/a.bin")}/*Стиль*/'.encode("cp1251"),
        )
        write(
            mobile / "styles/site.css",
            '@import "nested.css";body{background:url("../images/b.bin")}/*Стиль*/'.encode("cp1251"),
        )
        write(desktop / "styles/nested.css", '.nested{background:url("../images/a.bin")}')
        write(mobile / "styles/nested.css", '.nested{background:url("../images/b.bin")}')

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
        assert desktop_html.count(shared_url) == 6
        assert mobile_html.count(shared_url) == 3
        desktop_css = (output / "desktop/assets/styles/site.css").read_text(encoding="utf-8")
        mobile_css = (output / "mobile/assets/styles/site.css").read_text(encoding="utf-8")
        desktop_import = "/sites/reference/desktop/assets/styles/nested.css"
        mobile_import = "/sites/reference/mobile/assets/styles/nested.css"
        assert shared_url in desktop_css and desktop_import in desktop_css and "Стиль" in desktop_css
        assert shared_url in mobile_css and mobile_import in mobile_css and "Стиль" in mobile_css
        assert shared_url in (output / "desktop/assets/styles/nested.css").read_text(encoding="utf-8")
        assert shared_url in (output / "mobile/assets/styles/nested.css").read_text(encoding="utf-8")
        assert not list((output / "shared/assets").glob("*.css"))
        checker = load_checker()
        assert checker.validate_legacy_stylesheets(output) == []

        groups: dict[tuple[str, str], list[Path]] = defaultdict(list)
        for subtree in ("shared/assets", "desktop/assets", "mobile/assets"):
            for path in (output / subtree).rglob("*"):
                if path.is_file() and path.suffix.lower() != ".css":
                    groups[(hashlib.sha256(path.read_bytes()).hexdigest(), path.suffix.lower())].append(path)
        assert all(len(paths) == 1 for paths in groups.values())

    print("reference import asset dedup contracts: OK")


if __name__ == "__main__":
    main()
