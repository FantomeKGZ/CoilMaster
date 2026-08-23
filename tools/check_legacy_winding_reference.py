#!/usr/bin/env python3
"""Validate a generated CoilMaster legacy winding-reference import.

Broken generated links remain fatal. Links whose exact target is already absent from
the legacy source are preserved as explicit SOURCE WARNING evidence instead of being
silently treated as generated-site defects.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "firmware" / "esp32" / "web" / "sites" / "reference"
HTML_SUFFIXES = {".html", ".htm"}
ATTR_RE = re.compile(r"\b(?:href|src)\s*=\s*[\"'](?P<value>.*?)[\"']", re.I)
CSS_URL_RE = re.compile(r"\burl\(\s*[\"']?(?P<value>[^\"')]+)", re.I)
CSS_IMPORT_RE = re.compile(r"@import\s+[\"'](?P<value>.*?)[\"']", re.I)
REFERENCE_PREFIX = "/sites/reference/"


def css_reference_values(text: str):
    for pattern in (CSS_URL_RE, CSS_IMPORT_RE):
        for match in pattern.finditer(text):
            yield match.group("value").strip()


def digest(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def is_frontpage_metadata(path: Path, root: Path) -> bool:
    try:
        parts = path.relative_to(root).parts
    except ValueError:
        return False
    return any(part.lower().startswith("_vti_") for part in parts)


def html_count(root: Path) -> int:
    return sum(
        1
        for path in root.rglob("*")
        if (
            path.is_file()
            and path.suffix.lower() in HTML_SUFFIXES
            and not is_frontpage_metadata(path, root)
        )
    )


def generated_page_count(output: Path, mode: str) -> int:
    pages = output / mode / "pages"
    return html_count(pages) if pages.exists() else 0


def available_reference_urls(output: Path) -> set[str]:
    urls: set[str] = set()
    for path in output.rglob("*"):
        if not path.is_file():
            continue
        relative = path.relative_to(output).as_posix()
        url = REFERENCE_PREFIX + relative
        urls.add(url)
        if path.name.lower() == "index.html":
            directory = url[: -len("index.html")]
            urls.add(directory)
            urls.add(directory.rstrip("/"))
    return urls


def normalized_reference_url(value: str) -> str | None:
    parts = urlsplit(value)
    if parts.scheme or parts.netloc or not parts.path.startswith(REFERENCE_PREFIX):
        return None
    path = unquote(parts.path)
    segments: list[str] = []
    for segment in path.split("/"):
        if not segment or segment == ".":
            continue
        if segment == "..":
            if not segments:
                return "/__invalid_reference_escape__"
            segments.pop()
            continue
        segments.append(segment)
    return "/" + "/".join(segments) + ("/" if path.endswith("/") else "")


def source_file_index(root: Path) -> set[str]:
    return {
        path.relative_to(root).as_posix().casefold()
        for path in root.rglob("*")
        if path.is_file() and not is_frontpage_metadata(path, root)
    }


def is_preexisting_source_gap(
    target: str,
    mode: str,
    source_files: set[str],
) -> bool:
    """True only when a missing generated URL maps to a file absent in source."""
    asset_prefix = f"/sites/reference/{mode}/assets/"
    page_prefix = f"/sites/reference/{mode}/pages/"
    if target.startswith(asset_prefix):
        relative = target[len(asset_prefix):]
    elif target.startswith(page_prefix):
        relative = target[len(page_prefix):]
    else:
        return False
    return relative.casefold() not in source_files


def validate_catalog(output: Path, sources: dict[str, Path]) -> list[str]:
    errors: list[str] = []
    path = output / "shared" / "catalog.json"
    if not path.is_file():
        return [f"shared catalog missing: {path}"]
    try:
        catalog = json.loads(path.read_text(encoding="utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        return [f"invalid shared catalog: {path}: {exc}"]
    if not isinstance(catalog, list):
        return [f"shared catalog must be a list: {path}"]

    expected_paths = {
        mode: {
            p.relative_to(root).as_posix().casefold()
            for p in root.rglob("*")
            if p.is_file()
            and p.suffix.lower() in HTML_SUFFIXES
            and not is_frontpage_metadata(p, root)
        }
        for mode, root in sources.items()
    }
    seen: set[str] = set()
    for index, entry in enumerate(catalog):
        if not isinstance(entry, dict):
            errors.append(f"catalog entry {index} is not an object")
            continue
        rel = entry.get("path")
        title = entry.get("title")
        if not isinstance(rel, str) or not rel:
            errors.append(f"catalog entry {index} has invalid path")
            continue
        key = rel.casefold()
        if key in seen:
            errors.append(f"duplicate catalog path: {rel}")
        seen.add(key)
        if not isinstance(title, str) or not title.strip():
            errors.append(f"catalog entry {rel} has empty title")
        desktop = entry.get("desktop") is True
        mobile = entry.get("mobile") is True
        if desktop != (key in expected_paths["desktop"]):
            errors.append(f"catalog desktop availability mismatch: {rel}")
        if mobile != (key in expected_paths["mobile"]):
            errors.append(f"catalog mobile availability mismatch: {rel}")
        if desktop and not (output / "desktop" / "pages" / rel).is_file():
            errors.append(f"catalog desktop target missing: {rel}")
        if mobile and not (output / "mobile" / "pages" / rel).is_file():
            errors.append(f"catalog mobile target missing: {rel}")

    expected_union = expected_paths["desktop"] | expected_paths["mobile"]
    if seen != expected_union:
        errors.append(
            f"catalog coverage mismatch: expected={len(expected_union)}, catalog={len(seen)}"
        )
    return errors


def validate_entry_pages(output: Path) -> list[str]:
    errors: list[str] = []
    available_urls = available_reference_urls(output)
    required = (
        'data-reference-search',
        'data-reference-search-clear',
        'data-reference-query="4А"',
        'data-reference-query="АИР"',
        'data-reference-query="АО2"',
        'data-reference-query="5А"',
        'role="status"',
        'aria-live="polite"',
        'role="list"',
        'data-reference-results',
    )
    for mode in ("desktop", "mobile"):
        entry = output / mode / "index.html"
        if not entry.is_file():
            errors.append(f"reference entry page missing: {entry}")
            continue
        try:
            text = entry.read_text(encoding="utf-8")
        except UnicodeDecodeError as exc:
            errors.append(f"reference entry page not utf-8: {entry}: {exc}")
            continue
        for token in required:
            if token not in text:
                errors.append(f"reference entry search contract missing {token}: {entry}")
        for match in ATTR_RE.finditer(text):
            value = match.group("value")
            target = normalized_reference_url(value)
            if target is not None and target not in available_urls:
                errors.append(f"broken reference entry link: {entry} -> {value}")
    return errors


def validate_pages(
    output: Path,
    sources: dict[str, Path],
) -> tuple[list[str], list[str]]:
    errors: list[str] = []
    warnings: set[str] = set()
    shared_css = output / "shared" / "reference.css"
    shared_js = output / "shared" / "reference.js"
    if not shared_css.is_file():
        errors.append(f"shared stylesheet file missing: {shared_css}")
    if not shared_js.is_file():
        errors.append(f"shared script file missing: {shared_js}")

    available_urls = available_reference_urls(output)
    source_indexes = {mode: source_file_index(root) for mode, root in sources.items()}

    for mode in ("desktop", "mobile"):
        mode_pages = output / mode / "pages"
        if not mode_pages.is_dir():
            errors.append(f"generated page directory missing: {mode_pages}")
            continue
        for page in mode_pages.rglob("*"):
            if not page.is_file() or page.suffix.lower() not in HTML_SUFFIXES:
                continue
            if is_frontpage_metadata(page, mode_pages):
                errors.append(f"FrontPage metadata leaked into generated pages: {page}")
                continue
            try:
                text = page.read_text(encoding="utf-8")
            except UnicodeDecodeError as exc:
                errors.append(f"not utf-8: {page}: {exc}")
                continue
            lowered = text.lower()
            if "class=\"verh\"" in lowered or "class='verh'" in lowered or "images/verh.jpg" in lowered:
                errors.append(f"legacy top logo remains: {page}")
            if "charset=windows-1251" in lowered:
                errors.append(f"legacy charset remains: {page}")
            if "/sites/reference/shared/reference.css" not in text:
                errors.append(f"shared stylesheet missing: {page}")
            if "/sites/reference/shared/reference.js" not in text:
                errors.append(f"shared script missing: {page}")
            if 'class="cm-reference-page-toolbar"' not in text:
                errors.append(f"generated page top navigation missing: {page}")
            if 'class="cm-reference-page-footer"' not in text:
                errors.append(f"generated page bottom navigation missing: {page}")
            mode_home = f'href="/sites/reference/{mode}/"'
            if mode_home not in text:
                errors.append(f"generated page reference-home target missing: {page}")
            other_mode = "mobile" if mode == "desktop" else "desktop"
            relative_page = page.relative_to(mode_pages).as_posix()
            expected_switch = (
                f'href="/sites/reference/{other_mode}/pages/{relative_page}" '
                f'data-reference-mode="{other_mode}"'
            )
            if expected_switch not in text:
                errors.append(f"generated page same-page mode switch missing: {page}")
            for match in ATTR_RE.finditer(text):
                value = match.group("value")
                target = normalized_reference_url(value)
                if target is None or target in available_urls:
                    continue
                if is_preexisting_source_gap(target, mode, source_indexes[mode]):
                    warnings.add(f"{mode}: source file missing for preserved link {target}")
                else:
                    errors.append(f"broken reference link: {page} -> {value}")
            for value in css_reference_values(text):
                parts = urlsplit(value)
                if (
                    parts.scheme
                    or parts.netloc
                    or value.startswith(("#", "//", "data:"))
                ):
                    continue
                target = normalized_reference_url(value)
                if target is None:
                    errors.append(f"generated page CSS URL not rewritten: {page} -> {value}")
                elif target in available_urls:
                    continue
                elif is_preexisting_source_gap(target, mode, source_indexes[mode]):
                    warnings.add(f"{mode}: source file missing for preserved CSS link {target}")
                else:
                    errors.append(f"broken generated page CSS URL: {page} -> {value}")
    return errors, sorted(warnings)


def validate_legacy_stylesheets(output: Path) -> list[str]:
    errors: list[str] = []
    available_urls = available_reference_urls(output)
    shared_assets = output / "shared" / "assets"
    if shared_assets.exists():
        for path in shared_assets.rglob("*.css"):
            if path.is_file():
                errors.append(f"legacy stylesheet must remain mode-specific: {path}")

    for mode in ("desktop", "mobile"):
        asset_root = output / mode / "assets"
        if not asset_root.exists():
            continue
        for path in asset_root.rglob("*.css"):
            if not path.is_file():
                continue
            try:
                text = path.read_text(encoding="utf-8")
            except UnicodeDecodeError as exc:
                errors.append(f"legacy stylesheet not utf-8: {path}: {exc}")
                continue
            for value in css_reference_values(text):
                parts = urlsplit(value)
                if (
                    parts.scheme
                    or parts.netloc
                    or value.startswith(("#", "//", "data:"))
                ):
                    continue
                target = normalized_reference_url(value)
                if target is None:
                    errors.append(f"legacy stylesheet local URL not rewritten: {path} -> {value}")
                elif target not in available_urls:
                    errors.append(f"broken legacy stylesheet URL: {path} -> {value}")
    return errors


def duplicate_assets(output: Path) -> list[str]:
    groups: dict[tuple[str, str], list[Path]] = {}
    for subtree in (
        output / "shared" / "assets",
        output / "desktop" / "assets",
        output / "mobile" / "assets",
    ):
        if not subtree.exists():
            continue
        for path in subtree.rglob("*"):
            if path.is_file() and path.suffix.lower() != ".css":
                groups.setdefault((digest(path), path.suffix.lower()), []).append(path)

    errors: list[str] = []
    for paths in groups.values():
        if len(paths) > 1:
            errors.append(
                "byte-identical assets with the same suffix should be shared once: "
                + " == ".join(str(path) for path in paths)
            )
    return errors


def frontpage_output_leaks(output: Path) -> list[str]:
    errors: list[str] = []
    for mode in ("desktop", "mobile"):
        for subtree in (output / mode / "pages", output / mode / "assets"):
            if not subtree.exists():
                continue
            for path in subtree.rglob("*"):
                if path.is_file() and is_frontpage_metadata(path, subtree):
                    errors.append(f"FrontPage metadata leaked into output: {path}")
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--desktop-source", required=True, type=Path)
    parser.add_argument("--mobile-source", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    output = args.output.resolve()
    desktop_source = args.desktop_source.resolve()
    mobile_source = args.mobile_source.resolve()
    sources = {"desktop": desktop_source, "mobile": mobile_source}

    errors: list[str] = []
    for mode, source in sources.items():
        source_count = html_count(source)
        output_count = generated_page_count(output, mode)
        if source_count != output_count:
            errors.append(f"{mode} page count mismatch: source={source_count}, generated={output_count}")

    errors.extend(validate_catalog(output, sources))
    errors.extend(validate_entry_pages(output))
    page_errors, warnings = validate_pages(output, sources)
    errors.extend(page_errors)
    errors.extend(frontpage_output_leaks(output))
    errors.extend(validate_legacy_stylesheets(output))
    errors.extend(duplicate_assets(output))

    for warning in warnings:
        print(f"SOURCE WARNING: {warning}")

    if errors:
        for error in errors[:100]:
            print(f"ERROR: {error}")
        if len(errors) > 100:
            print(f"ERROR: ... and {len(errors) - 100} more")
        raise SystemExit(1)

    print(f"desktop pages: {generated_page_count(output, 'desktop')}")
    print(f"mobile pages: {generated_page_count(output, 'mobile')}")
    print(f"catalog entries: {len(json.loads((output / 'shared' / 'catalog.json').read_text(encoding='utf-8')))}")
    print(f"pre-existing source gaps: {len(warnings)}")
    print("legacy winding reference check: OK")


if __name__ == "__main__":
    main()
