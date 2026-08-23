#!/usr/bin/env python3
"""Contract test for complete offline /web dependency closure."""
from __future__ import annotations

import importlib.util
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / "tools" / "check_web_bundle_offline.py"


def load_checker():
    spec = importlib.util.spec_from_file_location("web_bundle_offline", CHECKER)
    if spec is None or spec.loader is None:
        raise AssertionError(f"cannot load offline checker: {CHECKER}")
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
    checker = load_checker()
    assert list(checker.srcset_values(
        "data:image/png;base64,AAAA 1x, /assets/photo-2x.jpg 2x"
    )) == ["data:image/png;base64,AAAA", "/assets/photo-2x.jpg"]
    with tempfile.TemporaryDirectory() as temp:
        web = Path(temp) / "web"
        write(
            web / "index.html",
            """<!doctype html><html><head>
<link rel="stylesheet" href="/assets/app.css">
<link rel="icon" href="/assets/icon.svg">
</head><body background="/assets/bg.png">
<a href="https://example.com/manual">Allowed navigation</a>
<img src="/assets/photo.jpg" srcset="/assets/photo.jpg 1x, /assets/photo-2x.jpg 2x">
<img src="/assets/photo.jpg" srcset="data:image/png;base64,AAAA 1x">
<script src="/assets/app.js"></script>
<div style="background:url('/assets/bg.png')"></div>
<style>.logo{background:url("/assets/icon.svg")}</style>
</body></html>""",
        )
        write(
            web / "assets" / "app.css",
            '@import "nested.css";@font-face{src:url("font.woff2")}'
            '.x{background:url("bg.png")}',
        )
        write(web / "assets" / "nested.css", ".nested{color:#000}")
        write(web / "assets" / "app.js", "fetch('/api/status');")
        write(web / "assets" / "icon.svg", "<svg></svg>")
        write(web / "assets" / "bg.png", b"bg")
        write(web / "assets" / "photo.jpg", b"one")
        write(web / "assets" / "photo-2x.jpg", b"two")
        write(web / "assets" / "font.woff2", b"font")

        assert checker.validate_bundle(web) == []

        original = (web / "index.html").read_text(encoding="utf-8")
        write(web / "index.html", original.replace("/assets/photo.jpg", "https://cdn.example/photo.jpg", 1))
        assert any(
            "remote runtime resource" in error
            for error in checker.validate_bundle(web)
        )

        write(web / "index.html", original)
        (web / "assets" / "font.woff2").unlink()
        assert any(
            "missing runtime resource" in error and "font.woff2" in error
            for error in checker.validate_bundle(web)
        )

    print("web bundle offline dependency contracts: OK")


if __name__ == "__main__":
    main()
