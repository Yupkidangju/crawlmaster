#!/usr/bin/env python3
"""설치 트리와 고정 component manifest를 결합한 SPDX 2.3 SBOM을 생성한다."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import re
import shutil
import subprocess
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def spdx_id(prefix: str, value: str) -> str:
    normalized = re.sub(r"[^A-Za-z0-9.-]", "-", value)
    return f"SPDXRef-{prefix}-{normalized}"


def runtime_dependencies(binary: Path, platform: str) -> list[str]:
    if platform.lower() == "linux":
        output = subprocess.run(
            ["readelf", "-d", str(binary)], check=True, capture_output=True, text=True
        ).stdout
        return sorted(set(re.findall(r"Shared library: \[([^]]+)]", output)))
    if platform.lower() == "windows":
        if shutil.which("dumpbin"):
            output = subprocess.run(
                ["dumpbin", "/dependents", str(binary)], check=True, capture_output=True, text=True
            ).stdout
            return sorted(set(re.findall(r"^\s+([^\s]+\.dll)\s*$", output, re.IGNORECASE | re.MULTILINE)))
        if shutil.which("x86_64-w64-mingw32-objdump"):
            output = subprocess.run(
                ["x86_64-w64-mingw32-objdump", "-p", str(binary)],
                check=True,
                capture_output=True,
                text=True,
            ).stdout
            return sorted(set(re.findall(r"DLL Name:\s+([^\s]+)", output)))
        raise RuntimeError("Windows runtime dependency inspector is unavailable")
    raise ValueError(f"unsupported platform: {platform}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", default="DEPENDENCY_MANIFEST.spdx.json")
    parser.add_argument("--install-root", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--platform", choices=("Linux", "Windows"), required=True)
    parser.add_argument(
        "--created", default=datetime.now(timezone.utc).replace(microsecond=0).isoformat()
    )
    args = parser.parse_args()

    root = Path(args.install_root).resolve()
    output_path = Path(args.output)
    document = json.loads(Path(args.manifest).read_text(encoding="utf-8"))
    executable = root / "bin" / ("Crawlmaster.exe" if args.platform == "Windows" else "Crawlmaster")
    if not executable.is_file():
        raise SystemExit(f"missing executable: {executable}")

    file_records: list[dict[str, object]] = []
    verification_hashes: list[str] = []
    relationships = list(document.get("relationships", []))
    for file_path in sorted(path for path in root.rglob("*") if path.is_file()):
        relative = file_path.relative_to(root).as_posix()
        digest = sha256(file_path)
        verification_hashes.append(digest)
        file_id = spdx_id("File", digest[:24])
        file_records.append(
            {
                "fileName": f"./{relative}",
                "SPDXID": file_id,
                "checksums": [{"algorithm": "SHA256", "checksumValue": digest}],
                "licenseConcluded": "NOASSERTION",
                "copyrightText": "NOASSERTION",
            }
        )
        relationships.append(
            {
                "spdxElementId": "SPDXRef-Package-Crawlmaster",
                "relationshipType": "CONTAINS",
                "relatedSpdxElement": file_id,
            }
        )

    root_package = next(
        package for package in document["packages"] if package["SPDXID"] == "SPDXRef-Package-Crawlmaster"
    )
    root_package["filesAnalyzed"] = True
    root_package["packageVerificationCode"] = {
        "packageVerificationCodeValue": hashlib.sha1(
            "".join(sorted(verification_hashes)).encode("ascii")
        ).hexdigest()
    }
    root_package["checksums"] = [{"algorithm": "SHA256", "checksumValue": sha256(executable)}]

    for dependency in runtime_dependencies(executable, args.platform):
        dependency_id = spdx_id("Runtime", dependency)
        document["packages"].append(
            {
                "name": dependency,
                "SPDXID": dependency_id,
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "comment": f"External {args.platform} runtime dependency; not bundled in the package.",
            }
        )
        relationships.append(
            {
                "spdxElementId": "SPDXRef-Package-Crawlmaster",
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": dependency_id,
            }
        )

    tree_digest = hashlib.sha256("".join(sorted(verification_hashes)).encode("ascii")).hexdigest()
    document["name"] = f"Crawlmaster-0.9.4-{args.platform}-artifact-sbom"
    document["documentNamespace"] = f"https://crawlmaster.local/spdx/artifact/{tree_digest}"
    document["creationInfo"]["created"] = args.created
    document["creationInfo"]["creators"] = ["Tool: scripts/generate_release_sbom.py"]
    document["documentComment"] = (
        "Artifact SPDX SBOM containing every shipped file, immutable source components, bundled fonts, "
        "and direct external runtime libraries. OS-provided libraries' own transitive graphs are outside the archive."
    )
    document["files"] = file_records
    document["relationships"] = relationships

    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(document, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
