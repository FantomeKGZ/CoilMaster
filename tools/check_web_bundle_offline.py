#!/usr/bin/env python3
"""Validate that an SD-ready /web bundle has no missing or remote runtime resources."""
from __future__ import annotations

import argparse
import posixpath
import re
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit

CSS_URL_RE = re.compile(r"\burl\(\s*[\"']?(?P<value>[^\"')]+)", re.I)
CSS_IMPORT_RE = re.compile(r"@import\s+[\"'](?P<value>.*?)[\"']", re.I)
HTML_SUFFIXES = {".html", ".htm"}
DYNAMIC_PREFIXES = ("/api/",)
LINK_RESOURCE_RELS = {
    "stylesheet",
    "icon",
    "preload",
    "modulepreload",
    "manifest",
    "apple-touch-icon",
}
TAG_RESOURCE_ATTRS = {
    "audio": ("src",),
    "embed": ("src",),
    "iframe": ("src",),
    "img": ("src",),
    "input": ("src",),
    "object": ("data",),
    "script": ("src",),
    "source": ("src",),
    "track": ("src",),
    "video": ("src", "poster"),
}


def css_values(text: str):
    for pattern in (CSS_URL_RE, CSS_IMPORT_RE):
        for match in pattern.finditer(text):
            yield match.group("value").strip()


def srcset_values(value: str):
    """Parse srcset candidates without splitting the comma inside data URLs."""
    index = 0
    length = len(value)
    while index < length:
        while index < length and (value[index].isspace() or value[index] == ","):
            index += 1
        if index >= length:
            break
        start = index
        if value[index:].casefold().startswith("data:"):
            while index < length and not value[index].isspace():
                index += 1
        else:
            while index < length and not value[index].isspace() and value[index] != ",":
                index += 1
        candidate = value[start:index]
        if candidate:
            yield candidate
        while index < length and value[index] != ",":
            index += 1
        if index < length:
            index += 1


class ResourceParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.resources: list[tuple[str, str]] = []
        self._style_depth = 0

    def handle_starttag(self, tag: str, attrs) -> None:
        tag = tag.lower()
        values = {str(name).lower(): value for name, value in attrs if name}
        for attribute in TAG_RESOURCE_ATTRS.get(tag, ()):
            value = values.get(attribute)
            if value:
                self.resources.append((f"{tag}[{attribute}]", value))
        background = values.get("background")
        if background:
            self.resources.append((f"{tag}[background]", background))
        style = values.get("style")
        if style:
            self.resources.extend(("inline-style", value) for value in css_values(style))
        srcset = values.get("srcset")
        if srcset:
            self.resources.extend((f"{tag}[srcset]", value) for value in srcset_values(srcset))
        if tag == "link":
            rel = {item.casefold() for item in (values.get("rel") or "").split()}
            href = values.get("href")
            if href and rel & LINK_RESOURCE_RELS:
                self.resources.append(("link[href]", href))
        if tag == "style":
            self._style_depth += 1

    def handle_startendtag(self, tag: str, attrs) -> None:
        self.handle_starttag(tag, attrs)

    def handle_endtag(self, tag: str) -> None:
        if tag.lower() == "style" and self._style_depth:
            self._style_depth -= 1

    def handle_data(self, data: str) -> None:
        if self._style_depth:
            self.resources.extend(("style-block", value) for value in css_values(data))


def classify_target(value: str) -> tuple[str, str]:
    stripped = value.strip()
    if not stripped or stripped.startswith("#"):
        return "ignore", stripped
    if "\\" in stripped:
        return "invalid", stripped
    if stripped.startswith("//"):
        return "remote", stripped
    parts = urlsplit(stripped)
    scheme = parts.scheme.casefold()
    if scheme in {"data", "blob"}:
        return "ignore", stripped
    if scheme or parts.netloc:
        return "remote", stripped
    decoded = unquote(parts.path)
    if not decoded:
        return "ignore", stripped
    if any(decoded.startswith(prefix) for prefix in DYNAMIC_PREFIXES):
        return "dynamic", decoded
    return "local", decoded


def resolve_local(web_root: Path, source: Path, url_path: str) -> Path | None:
    if url_path.startswith("/"):
        relative = posixpath.normpath(url_path.lstrip("/"))
    else:
        source_dir = source.relative_to(web_root).parent.as_posix()
        relative = posixpath.normpath(posixpath.join(source_dir, url_path))
    if relative == ".." or relative.startswith("../"):
        return None
    target = web_root.joinpath(*Path(relative).parts)
    if target.is_dir():
        target = target / "index.html"
    return target


def validate_resource(
    web_root: Path,
    source: Path,
    kind: str,
    value: str,
) -> str | None:
    classification, path = classify_target(value)
    relative_source = source.relative_to(web_root).as_posix()
    if classification == "remote":
        return f"remote runtime resource: {relative_source}: {kind} -> {value}"
    if classification == "invalid":
        return f"invalid runtime resource URL: {relative_source}: {kind} -> {value}"
    if classification != "local":
        return None
    target = resolve_local(web_root, source, path)
    if target is None:
        return f"runtime resource escapes /web: {relative_source}: {kind} -> {value}"
    if not target.is_file():
        return f"missing runtime resource: {relative_source}: {kind} -> {value}"
    return None


def validate_bundle(web_root: Path) -> list[str]:
    errors: list[str] = []
    for source in sorted(path for path in web_root.rglob("*") if path.is_file()):
        suffix = source.suffix.casefold()
        resources: list[tuple[str, str]] = []
        if suffix in HTML_SUFFIXES:
            try:
                text = source.read_text(encoding="utf-8")
            except UnicodeDecodeError as exc:
                errors.append(f"HTML is not UTF-8: {source.relative_to(web_root)}: {exc}")
                continue
            parser = ResourceParser()
            parser.feed(text)
            parser.close()
            resources = parser.resources
        elif suffix == ".css":
            try:
                text = source.read_text(encoding="utf-8")
            except UnicodeDecodeError as exc:
                errors.append(f"CSS is not UTF-8: {source.relative_to(web_root)}: {exc}")
                continue
            resources = [("css", value) for value in css_values(text)]
        for kind, value in resources:
            error = validate_resource(web_root, source, kind, value)
            if error:
                errors.append(error)
    return errors


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--web-bundle", required=True, type=Path)
    return parser.parse_args()


def main() -> None:
    web_root = parse_args().web_bundle.resolve()
    if not web_root.is_dir():
        raise SystemExit(f"web bundle directory missing: {web_root}")
    errors = validate_bundle(web_root)
    if errors:
        for error in errors[:100]:
            print(f"ERROR: {error}")
        if len(errors) > 100:
            print(f"ERROR: ... and {len(errors) - 100} more")
        raise SystemExit(1)
    files = sum(1 for path in web_root.rglob("*") if path.is_file())
    print(f"offline dependency closure: {files} files OK")


if __name__ == "__main__":
    main()
