#!/usr/bin/env python3
"""Contract tests for the report-only legacy reference migration audit."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUDIT = ROOT / "tools" / "audit_legacy_winding_reference.py"


def write(path: Path, value: str | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if isinstance(value, bytes):
        path.write_bytes(value)
    else:
        path.write_text(value, encoding="utf-8")


def main() -> None:
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "reference"
        report = Path(temp) / "audit.json"

        write(
            output / "desktop/pages/a.html",
            '<a href="/sites/reference/desktop/pages/b.html">B</a>'
            '<img src="/sites/reference/shared/assets/used.bin">'
            '<link rel="stylesheet" href="/sites/reference/desktop/assets/legacy.css">'
            '<video poster="/sites/reference/shared/assets/poster.bin"></video>',
        )
        write(output / "desktop/pages/b.html", "<p>B</p>")
        write(output / "mobile/pages/a.html", "<p>A mobile</p>")
        write(output / "mobile/pages/b.html", "<p>B mobile</p>")
        write(output / "desktop/index.html", "<p>Desktop</p>")
        write(output / "mobile/index.html", "<p>Mobile</p>")
        write(
            output / "shared/reference.css",
            'body{background:url("/sites/reference/shared/assets/css.bin")}',
        )
        write(output / "shared/assets/used.bin", b"used")
        write(output / "shared/assets/css.bin", b"css")
        write(output / "shared/assets/unused.bin", b"unused")
        write(output / "shared/assets/cp.bin", b"cp1251")
        write(output / "shared/assets/win.bin", b"windows")
        write(output / "shared/assets/poster.bin", b"poster")
        write(
            output / "desktop/assets/legacy.css",
            ('@font-face{src:url("../../shared/assets/cp.bin")}'
             '.win{background:url("..\\..\\shared\\assets\\win.bin")}/*М*/').encode("cp1251"),
        )
        write(output / "desktop/assets/duplicate.bin", b"same")
        write(output / "mobile/assets/duplicate-copy.bin", b"same")
        write(output / "mobile/assets/same-bytes.dat", b"same")

        subprocess.run(
            [sys.executable, str(AUDIT), "--output", str(output), "--report", str(report)],
            check=True,
        )
        data = json.loads(report.read_text(encoding="utf-8"))

        assert data["policy"] == "report-only; no files were removed"
        assert data["generated_pages"] == 4
        assert data["unreferenced_assets"]["count"] == 4
        assert data["unreferenced_assets"]["bytes"] == 18
        assert "shared/assets/poster.bin" not in data["unreferenced_assets"]["paths"]
        assert data["unreferenced_assets"]["by_extension"] == {
            ".bin": {"count": 3, "bytes": 14},
            ".dat": {"count": 1, "bytes": 4},
        }
        assert data["duplicate_asset_groups"]["count"] == 1
        assert data["duplicate_asset_groups"]["potential_savings_bytes"] == 4
        assert data["content_identical_page_groups"]["count"] == 0
        assert data["pages_without_legacy_incoming_links"]["desktop"]["count"] == 1
        assert data["pages_without_legacy_incoming_links"]["mobile"]["count"] == 2

    print("reference migration audit contracts: OK")


if __name__ == "__main__":
    main()
