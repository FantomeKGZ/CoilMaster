#!/usr/bin/env python3
"""Build or verify the provenance and payload manifest for an SD-ready /web bundle."""
from __future__ import annotations

import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path

MANIFEST_NAME = "web-bundle-manifest.json"
HTML_SUFFIXES = {".html", ".htm"}


def payload_files(root: Path, manifest: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if path.is_symlink():
            raise ValueError(f"symlink is not allowed in web bundle: {path}")
        if path.is_file() and path != manifest:
            files.append(path)
    return sorted(files, key=lambda item: item.relative_to(root).as_posix())


def file_digest(path: Path) -> tuple[int, str]:
    digest = hashlib.sha256()
    size = 0
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            size += len(chunk)
            digest.update(chunk)
    return size, digest.hexdigest()


def tree_metrics(root: Path, manifest: Path) -> dict[str, int | str]:
    tree = hashlib.sha256()
    total_bytes = 0
    files = payload_files(root, manifest)
    for path in files:
        relative = path.relative_to(root).as_posix()
        size, sha256 = file_digest(path)
        total_bytes += size
        tree.update(relative.encode("utf-8"))
        tree.update(b"\0")
        tree.update(str(size).encode("ascii"))
        tree.update(b"\0")
        tree.update(sha256.encode("ascii"))
        tree.update(b"\n")
    return {"files": len(files), "bytes": total_bytes, "sha256": tree.hexdigest()}


def directory_metrics(root: Path) -> tuple[int, int]:
    files = [path for path in root.rglob("*") if path.is_file() and not path.is_symlink()]
    return len(files), sum(path.stat().st_size for path in files)


def reference_metrics(reference: Path) -> dict[str, int]:
    catalog_path = reference / "shared" / "catalog.json"
    catalog = json.loads(catalog_path.read_text(encoding="utf-8"))
    if not isinstance(catalog, list) or not catalog:
        raise ValueError(f"reference catalog must be a non-empty list: {catalog_path}")

    files, total_bytes = directory_metrics(reference)
    html_files = sum(
        1
        for path in reference.rglob("*")
        if path.is_file() and path.suffix.lower() in HTML_SUFFIXES
    )
    shared_assets, _ = directory_metrics(reference / "shared" / "assets")
    desktop_assets, _ = directory_metrics(reference / "desktop" / "assets")
    mobile_assets, _ = directory_metrics(reference / "mobile" / "assets")
    return {
        "catalog_entries": len(catalog),
        "files": files,
        "bytes": total_bytes,
        "html_files": html_files,
        "shared_assets": shared_assets,
        "desktop_assets": desktop_assets,
        "mobile_assets": mobile_assets,
    }


def current_metrics(web_bundle: Path, manifest: Path) -> dict[str, object]:
    reference = web_bundle / "sites" / "reference"
    if not reference.is_dir():
        raise ValueError(f"reference output missing: {reference}")
    return {
        "reference": reference_metrics(reference),
        "web_payload": tree_metrics(web_bundle, manifest),
    }


def write_manifest(args: argparse.Namespace, manifest: Path) -> dict[str, object]:
    required = {
        "--coilmaster-commit": args.coilmaster_commit,
        "--branch": args.branch,
        "--run-id": args.run_id,
        "--legacy-commit": args.legacy_commit,
    }
    missing = [name for name, value in required.items() if value in (None, "")]
    if missing:
        raise ValueError("missing build metadata: " + ", ".join(missing))

    generated_utc = args.generated_utc or (
        datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
    )
    metrics = current_metrics(args.web_bundle, manifest)
    data: dict[str, object] = {
        "schema_version": 2,
        "coilmaster_commit": args.coilmaster_commit,
        "coilmaster_branch": args.branch,
        "workflow_run_id": args.run_id,
        "legacy_repository": "FantomeKGZ/motor-winding-reference",
        "legacy_commit": args.legacy_commit,
        "generated_utc": generated_utc,
        **metrics,
    }
    temporary = manifest.with_name(manifest.name + ".tmp")
    temporary.unlink(missing_ok=True)
    temporary.write_text(
        json.dumps(data, ensure_ascii=False, separators=(",", ":")),
        encoding="utf-8",
        newline="\n",
    )
    temporary.replace(manifest)
    return data


def verify_manifest(web_bundle: Path, manifest: Path) -> dict[str, object]:
    data = json.loads(manifest.read_text(encoding="utf-8"))
    if data.get("schema_version") != 2:
        raise ValueError(f"unsupported web bundle manifest schema: {data.get('schema_version')}")
    actual = current_metrics(web_bundle, manifest)
    for section in ("reference", "web_payload"):
        if data.get(section) != actual[section]:
            raise ValueError(
                f"{section} mismatch: manifest={data.get(section)!r}, actual={actual[section]!r}"
            )
    return data


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--web-bundle", required=True, type=Path)
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--coilmaster-commit")
    parser.add_argument("--branch")
    parser.add_argument("--run-id", type=int)
    parser.add_argument("--legacy-commit")
    parser.add_argument("--generated-utc")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    args.web_bundle = args.web_bundle.resolve()
    manifest = args.web_bundle / MANIFEST_NAME
    if args.verify:
        data = verify_manifest(args.web_bundle, manifest)
        print(
            "web bundle manifest verified: "
            f"{data['web_payload']['files']} files, "
            f"{data['web_payload']['bytes']} bytes, "
            f"sha256={data['web_payload']['sha256']}"
        )
    else:
        data = write_manifest(args, manifest)
        print(f"web bundle manifest written: {manifest}")
        print(json.dumps(data, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
