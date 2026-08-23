#!/usr/bin/env python3
"""Import the legacy winding reference into the CoilMaster static-site layout.

The source is the old Windows-1251 HTML reference stored in
FantomeKGZ/motor-winding-reference/sourse/{desktop,mobile}. This importer:

* preserves page bodies, tables, descriptions, images and internal HTML links;
* removes only the legacy top banner/logo block (``div.verh``);
* converts generated HTML to UTF-8;
* gives desktop/mobile the same CoilMaster shell and shared stylesheet/JS;
* rewrites internal links so each UI mode stays inside its own page tree;
* stores byte-identical desktop/mobile assets once in ``shared/assets``;
* keeps mode-specific assets below ``desktop/assets`` or ``mobile/assets``;
* excludes Microsoft FrontPage ``_vti_*`` metadata from the published site.

It intentionally does not modify ESP32/Arduino runtime code.
"""
from __future__ import annotations

import argparse
import hashlib
import html
import os
import re
import shutil
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import unquote, urlsplit, urlunsplit

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "firmware" / "esp32" / "web" / "sites" / "reference"
HTML_SUFFIXES = {".html", ".htm"}
SKIP_ASSETS = {"images/verh.jpg"}

BODY_RE = re.compile(r"<body\b[^>]*>(?P<body>.*)</body\s*>", re.I | re.S)
TITLE_RE = re.compile(r"<title\b[^>]*>(?P<title>.*?)</title\s*>", re.I | re.S)
VERH_RE = re.compile(r"<div\b[^>]*class=[\"'][^\"']*\bverh\b[^\"']*[\"'][^>]*>.*?</div\s*>", re.I | re.S)
CHARSET_META_RE = re.compile(r"<meta\b[^>]*(?:charset\s*=|http-equiv=[\"']Content-Type[\"'])[^>]*>", re.I)
ATTR_RE = re.compile(r"(?P<prefix>\b(?:href|src)\s*=\s*)(?P<quote>[\"'])(?P<value>.*?)(?P=quote)", re.I)


@dataclass(frozen=True)
class ModeSource:
    mode: str
    root: Path


def decode_legacy(path: Path) -> str:
    data = path.read_bytes()
    for encoding in ("utf-8-sig", "cp1251"):
        try:
            return data.decode(encoding)
        except UnicodeDecodeError:
            pass
    return data.decode("cp1251", errors="replace")


def rel_posix(path: Path, root: Path) -> str:
    return path.relative_to(root).as_posix()


def is_frontpage_metadata(path: Path, root: Path) -> bool:
    """Return True for Microsoft FrontPage bookkeeping trees, never site content."""
    try:
        parts = path.relative_to(root).parts
    except ValueError:
        return False
    return any(part.lower().startswith("_vti_") for part in parts)


def asset_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def collect_assets(source: ModeSource) -> dict[str, Path]:
    assets: dict[str, Path] = {}
    for path in source.root.rglob("*"):
        if (
            not path.is_file()
            or path.suffix.lower() in HTML_SUFFIXES
            or is_frontpage_metadata(path, source.root)
        ):
            continue
        rel = rel_posix(path, source.root)
        if rel.lower() in SKIP_ASSETS:
            continue
        assets[rel] = path
    return assets


def shared_asset_map(desktop: ModeSource, mobile: ModeSource) -> tuple[dict[str, str], dict[str, str]]:
    desktop_assets = collect_assets(desktop)
    mobile_assets = collect_assets(mobile)
    mobile_by_hash: dict[str, list[str]] = {}
    for rel, path in mobile_assets.items():
        mobile_by_hash.setdefault(asset_hash(path), []).append(rel)

    desktop_shared: dict[str, str] = {}
    mobile_shared: dict[str, str] = {}
    for rel, path in desktop_assets.items():
        digest = asset_hash(path)
        matches = mobile_by_hash.get(digest, [])
        if not matches:
            continue
        canonical_name = Path(rel).name
        target = f"{digest[:16]}-{canonical_name}"
        desktop_shared[rel] = target
        for mobile_rel in matches:
            mobile_shared[mobile_rel] = target
    return desktop_shared, mobile_shared


def clean_output(output: Path) -> None:
    for mode in ("desktop", "mobile"):
        for child in (output / mode / "pages", output / mode / "assets"):
            if child.exists():
                shutil.rmtree(child)
    shared_assets = output / "shared" / "assets"
    if shared_assets.exists():
        shutil.rmtree(shared_assets)


def copy_assets(source: ModeSource, output: Path, shared_for_mode: dict[str, str]) -> None:
    for rel, path in collect_assets(source).items():
        shared_name = shared_for_mode.get(rel)
        if shared_name:
            target = output / "shared" / "assets" / shared_name
        else:
            target = output / source.mode / "assets" / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists():
            shutil.copy2(path, target)


def external_or_special(value: str) -> bool:
    stripped = value.strip()
    if not stripped or stripped.startswith(("#", "/", "?", "//")):
        return stripped.startswith(("/", "//"))
    scheme = urlsplit(stripped).scheme.lower()
    return scheme in {"http", "https", "mailto", "tel", "javascript", "data"}


def resolve_rel(current_rel: str, target: str) -> str:
    current_dir = Path(current_rel).parent
    normalized = os.path.normpath((current_dir / target).as_posix()).replace("\\", "/")
    while normalized.startswith("../"):
        normalized = normalized[3:]
    return normalized.lstrip("./")


def rewrite_url(value: str, current_rel: str, mode: str, shared_for_mode: dict[str, str]) -> str:
    if external_or_special(value):
        return value
    parts = urlsplit(value)
    if not parts.path:
        return value
    decoded_path = unquote(parts.path)
    resolved = resolve_rel(current_rel, decoded_path)
    suffix = Path(resolved).suffix.lower()
    if suffix in HTML_SUFFIXES:
        # The exported legacy bundle references index.html from almost every page,
        # but that file is absent from the source export. Route those legacy
        # "home" links to the CoilMaster reference entry page instead of
        # preserving a known-broken target.
        if resolved.lower() == "index.html":
            new_path = f"/sites/reference/{mode}/"
        else:
            new_path = f"/sites/reference/{mode}/pages/{resolved}"
    else:
        shared_name = shared_for_mode.get(resolved)
        if shared_name:
            new_path = f"/sites/reference/shared/assets/{shared_name}"
        else:
            new_path = f"/sites/reference/{mode}/assets/{resolved}"
    return urlunsplit(("", "", new_path, parts.query, parts.fragment))


def rewrite_links(fragment: str, current_rel: str, mode: str, shared_for_mode: dict[str, str]) -> str:
    def replace(match: re.Match[str]) -> str:
        value = match.group("value")
        rewritten = rewrite_url(value, current_rel, mode, shared_for_mode)
        return f"{match.group('prefix')}{match.group('quote')}{rewritten}{match.group('quote')}"
    return ATTR_RE.sub(replace, fragment)


def extract_title(source: str) -> str:
    match = TITLE_RE.search(source)
    if not match:
        return "Справочник обмотчика"
    return html.unescape(re.sub(r"<[^>]+>", "", match.group("title"))).strip() or "Справочник обмотчика"


def extract_body(source: str) -> str:
    body_match = BODY_RE.search(source)
    body = body_match.group("body") if body_match else source
    body = VERH_RE.sub("", body)
    return body.strip()


def nav_links(mode: str) -> str:
    prefix = f"/{mode}"
    items = [
        ("🏠", "Главная", f"{prefix}/"),
        ("🧰", "Ремонты", f"{prefix}/repairs.html"),
        ("👤", "Клиенты", f"{prefix}/clients.html"),
        ("📊", "Двигатели", f"{prefix}/motors.html"),
        ("🔌", "Arduino архив", f"{prefix}/arduino-windings.html"),
        ("🧮", "Калькулятор", f"{prefix}/calculator.html"),
        ("📦", "Склад", f"{prefix}/warehouse.html"),
        ("💰", "Калькуляция", f"{prefix}/costing.html"),
        ("📈", "Отчёты", f"{prefix}/reports.html"),
        ("💾", "Резервная копия", f"{prefix}/backup.html"),
        ("⚙️", "Настройки", f"{prefix}/settings.html"),
    ]
    return "\n".join(f'<a href="{href}">{icon} {label}</a>' for icon, label, href in items)


def mobile_nav_links(mode: str) -> str:
    prefix = f"/{mode}"
    items = [
        ("🏠", "Главная", f"{prefix}/"),
        ("🧰", "Ремонты", f"{prefix}/repairs.html"),
        ("👤", "Клиенты", f"{prefix}/clients.html"),
        ("📊", "Двигатели", f"{prefix}/motors.html"),
        ("🔌", "Arduino", f"{prefix}/arduino-windings.html"),
        ("🧮", "Калькулятор", f"{prefix}/calculator.html"),
        ("📦", "Склад", f"{prefix}/warehouse.html"),
        ("💰", "Калькуляция", f"{prefix}/costing.html"),
        ("📈", "Отчёты", f"{prefix}/reports.html"),
        ("💾", "Backup", f"{prefix}/backup.html"),
        ("⚙️", "Настройки", f"{prefix}/settings.html"),
    ]
    return "".join(f'<a href="{href}">{icon} {label}</a>' for icon, label, href in items)


def shell(title: str, body: str, mode: str) -> str:
    other = "mobile" if mode == "desktop" else "desktop"
    other_label = "📱 Мобильная версия" if other == "mobile" else "🖥 Версия для ПК"
    other_short = "📱" if other == "mobile" else "🖥"
    return f'''<!doctype html>
<html lang="ru" data-reference-mode="{mode}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>{html.escape(title)} · CoilMaster</title>
<link rel="stylesheet" href="/sites/reference/shared/reference.css">
</head>
<body>
<div class="cm-reference-layout">
<aside class="cm-reference-nav">
<h1>CoilMaster</h1>
{nav_links(mode)}
<a class="active" href="/sites/reference/{mode}/">📚 Справочник обмотчика</a>
<div class="cm-reference-switch"><a href="/sites/reference/{other}/" data-reference-mode="{other}">{other_label}</a></div>
</aside>
<header class="cm-reference-mobile-header"><a href="/{mode}/">←</a><b>Справочник обмотчика</b><a href="/sites/reference/{other}/" data-reference-mode="{other}">{other_short}</a></header>
<nav class="cm-reference-mobile-nav" aria-label="Разделы CoilMaster">{mobile_nav_links(mode)}<a class="active" href="/sites/reference/{mode}/">📚 Справочник</a></nav>
<main class="cm-reference-main cm-reference-content">
<section class="cm-reference-card cm-reference-legacy-page">
{body}
</section>
</main>
</div>
<script src="/sites/reference/shared/reference.js"></script>
</body>
</html>
'''


def convert_pages(source: ModeSource, output: Path, shared_for_mode: dict[str, str]) -> int:
    count = 0
    for path in sorted(source.root.rglob("*")):
        if (
            not path.is_file()
            or path.suffix.lower() not in HTML_SUFFIXES
            or is_frontpage_metadata(path, source.root)
        ):
            continue
        rel = rel_posix(path, source.root)
        raw = decode_legacy(path)
        title = extract_title(raw)
        body = extract_body(raw)
        body = CHARSET_META_RE.sub("", body)
        body = rewrite_links(body, rel, source.mode, shared_for_mode)
        target = output / source.mode / "pages" / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(shell(title, body, source.mode), encoding="utf-8", newline="\n")
        count += 1
    return count


def write_entry_link(output: Path, mode: str, source: ModeSource) -> None:
    entry = output / mode / "index.html"
    if not entry.exists():
        return
    # The source export has no index.html. Use the legacy 4A table as a stable
    # first material when present; otherwise fall back to the first real HTML file.
    preferred = source.root / "4A.html"
    if preferred.exists():
        first_rel = "4A.html"
    else:
        candidates = sorted(
            path for path in source.root.rglob("*")
            if (
                path.is_file()
                and path.suffix.lower() in HTML_SUFFIXES
                and not is_frontpage_metadata(path, source.root)
            )
        )
        if not candidates:
            return
        first_rel = rel_posix(candidates[0], source.root)
    current = entry.read_text(encoding="utf-8")
    href = f"/sites/reference/{mode}/pages/{first_rel}"
    marker = f'<a class="cm-reference-link" href="{href}">'
    if marker in current:
        return
    card = (
        '<section class="cm-reference-card"><h3>Материалы справочника</h3>'
        f'<a class="cm-reference-link" href="{href}">Открыть справочник →</a></section>'
    )
    if "</main>" in current:
        current = current.replace("</main>", card + "\n</main>", 1)
        entry.write_text(current, encoding="utf-8", newline="\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--desktop-source", required=True, type=Path)
    parser.add_argument("--mobile-source", required=True, type=Path)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    desktop = ModeSource("desktop", args.desktop_source.resolve())
    mobile = ModeSource("mobile", args.mobile_source.resolve())
    for source in (desktop, mobile):
        if not source.root.is_dir():
            raise SystemExit(f"source directory not found: {source.root}")

    output = args.output.resolve()
    clean_output(output)
    desktop_shared, mobile_shared = shared_asset_map(desktop, mobile)
    copy_assets(desktop, output, desktop_shared)
    copy_assets(mobile, output, mobile_shared)
    desktop_pages = convert_pages(desktop, output, desktop_shared)
    mobile_pages = convert_pages(mobile, output, mobile_shared)
    write_entry_link(output, "desktop", desktop)
    write_entry_link(output, "mobile", mobile)

    print(f"desktop pages: {desktop_pages}")
    print(f"mobile pages: {mobile_pages}")
    print(f"shared desktop assets: {len(desktop_shared)}")
    print(f"shared mobile assets: {len(mobile_shared)}")
    print(f"output: {output}")


if __name__ == "__main__":
    main()
