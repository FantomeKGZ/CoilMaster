#!/usr/bin/env python3
"""Contract test for deterministic risk-based reference smoke selection."""
from __future__ import annotations

import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILDER = ROOT / "tools" / "build_reference_smoke_plan.py"


def load_builder():
    spec = importlib.util.spec_from_file_location("reference_smoke_plan", BUILDER)
    if spec is None or spec.loader is None:
        raise AssertionError(f"cannot load smoke-plan builder: {BUILDER}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def write(path: Path, value: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(value, encoding="utf-8")


def generated(body: str) -> str:
    return (
        '<html><body><section class="cm-reference-card cm-reference-legacy-page">'
        + body
        + "</section></body></html>"
    )


def main() -> None:
    builder = load_builder()
    with tempfile.TemporaryDirectory() as temp:
        output = Path(temp) / "reference"
        entries = [
            ("4A.html", "4А", "<table><tr><td>4A</td></tr></table>"),
            ("tables.html", "Tables", "<table></table><table></table><table></table>"),
            ("images.html", "Images", "<img><img><img><img>"),
            ("большая страница.html", "Large", "<p>" + ("content " * 100) + "</p>"),
            ("nested/deep/page.html", "Deep", "<p>deep</p>"),
        ]
        catalog = []
        for relative, title, body in entries:
            catalog.append(
                {"path": relative, "title": title, "desktop": True, "mobile": True}
            )
            for mode in ("desktop", "mobile"):
                write(output / mode / "pages" / relative, generated(body))
        write(
            output / "shared" / "catalog.json",
            json.dumps(catalog, ensure_ascii=False),
        )

        plan = builder.build_plan(output)
        assert plan["schema_version"] == 1
        assert plan["catalog_entries"] == 5
        assert plan["selected_pages"] == 5
        pages = {page["path"]: page for page in plan["pages"]}
        assert "known-series-4A" in pages["4A.html"]["reasons"]
        assert "most-tables" in pages["tables.html"]["reasons"]
        assert "most-images" in pages["images.html"]["reasons"]
        assert "largest-visible-content" in pages["большая страница.html"]["reasons"]
        assert "deepest-path" in pages["nested/deep/page.html"]["reasons"]
        assert "%D0%B1" in pages["большая страница.html"]["desktop_url"]
        assert pages["tables.html"]["metrics_by_mode"]["desktop"]["tables"] == 3
        assert len(plan["checks"]) == 6

        (output / "mobile" / "pages" / "images.html").unlink()
        try:
            builder.build_plan(output)
        except ValueError as exc:
            assert "catalog smoke target missing" in str(exc)
        else:
            raise AssertionError("missing catalog target must fail smoke-plan generation")

    print("reference smoke plan contracts: OK")


if __name__ == "__main__":
    main()
