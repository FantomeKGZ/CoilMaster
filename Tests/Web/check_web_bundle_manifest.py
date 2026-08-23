#!/usr/bin/env python3
"""Contract test for the exact SD-ready /web payload manifest."""
from __future__ import annotations

import json
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / "tools" / "build_web_bundle_manifest.py"


def write(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def run(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [sys.executable, str(SCRIPT), *args],
        check=check,
        capture_output=True,
        text=True,
    )


def main() -> None:
    with tempfile.TemporaryDirectory() as temp:
        web = Path(temp) / "web"
        reference = web / "sites" / "reference"
        write(web / "index.html", "<html>CoilMaster</html>")
        write(reference / "desktop" / "index.html", "<html>Reference</html>")
        write(
            reference / "shared" / "catalog.json",
            json.dumps([{"title": "4A", "desktop": "desktop/pages/4A.html"}]),
        )
        for path in (
            reference / "shared" / "assets",
            reference / "desktop" / "assets",
            reference / "mobile" / "assets",
        ):
            path.mkdir(parents=True, exist_ok=True)

        run(
            "--web-bundle",
            str(web),
            "--coilmaster-commit",
            "a" * 40,
            "--branch",
            "cmp-protocol-v1",
            "--run-id",
            "42",
            "--legacy-commit",
            "b" * 40,
            "--generated-utc",
            "2026-08-23T00:00:00Z",
        )

        manifest_path = web / "web-bundle-manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        assert manifest["schema_version"] == 2
        assert manifest["workflow_run_id"] == 42
        assert manifest["reference"] == {
            "catalog_entries": 1,
            "files": 2,
            "bytes": sum(path.stat().st_size for path in reference.rglob("*") if path.is_file()),
            "html_files": 1,
            "shared_assets": 0,
            "desktop_assets": 0,
            "mobile_assets": 0,
        }
        assert manifest["web_payload"]["files"] == 3
        assert manifest["web_payload"]["bytes"] > 0
        assert len(manifest["web_payload"]["sha256"]) == 64
        assert run("--web-bundle", str(web), "--verify").returncode == 0

        write(web / "index.html", "<html>tampered</html>")
        failed = run("--web-bundle", str(web), "--verify", check=False)
        assert failed.returncode != 0
        assert "web_payload mismatch" in failed.stderr

    print("web bundle manifest contracts: OK")


if __name__ == "__main__":
    main()
