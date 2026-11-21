#!/usr/bin/env python3
"""Build a private, untracked configuration bundle for the Semantic_BI VPS runbook."""
from __future__ import annotations

import argparse
import json
import os
import re
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ENV = ROOT / "local-data/remote-graphdb-v2-20261115/graphdb-v2-parallel-2/.env"
DEFAULT_OUTPUT = Path("/tmp/semantic-bi-vps-configs.tar.gz")
S3_FIELDS = (
    "S3_BUCKET",
    "S3_PORTAL_ID",
    "S3_ENDPOINT_URL",
    "S3_REGION",
    "AWS_ACCESS_KEY_ID",
    "AWS_SECRET_ACCESS_KEY",
)


def read_env(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if line.startswith("export "):
            line = line[7:].lstrip()
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key, value = key.strip(), value.strip()
        if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
            value = value[1:-1]
        elif " #" in value:
            value = value.split(" #", 1)[0].rstrip()
        if re.fullmatch(r"[A-Z][A-Z0-9_]*", key):
            values[key] = value
    missing = [key for key in S3_FIELDS if not values.get(key)]
    if missing:
        raise ValueError("Source env is missing required S3 fields")
    return values


def write_private(path: Path, data: str | bytes) -> None:
    if isinstance(data, str):
        data = data.encode("utf-8")
    path.parent.mkdir(parents=True, exist_ok=True, mode=0o700)
    path.write_bytes(data)
    path.chmod(0o600)


def build_bundle(
    source_env: Path,
    license_file: Path,
    docker_config: Path,
    image_tag: str,
    output: Path,
) -> Path:
    source_env, license_file, docker_config = map(Path, (source_env, license_file, docker_config))
    output = output.expanduser().resolve()
    if output == ROOT or ROOT in output.parents:
        raise ValueError("Refusing to write a secrets bundle inside the repository")
    for required in (source_env, license_file, docker_config):
        if not required.is_file():
            raise FileNotFoundError("A required input file is missing")
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", image_tag):
        raise ValueError("Image tag contains unsupported characters")

    values = read_env(source_env)
    loader_template = (ROOT / "deploy/loader/config.vps.example.yaml").read_text(encoding="utf-8")
    monitor_template = (ROOT / "deploy/monitor/monitor.vps.env.example").read_text(encoding="utf-8")
    for key, template_value in (
        ("S3_BUCKET", values["S3_BUCKET"]),
        ("S3_PORTAL_ID", values["S3_PORTAL_ID"]),
        ("S3_ENDPOINT_URL", values["S3_ENDPOINT_URL"]),
        ("S3_REGION", values["S3_REGION"]),
    ):
        if template_value not in loader_template or template_value not in monitor_template:
            raise ValueError("Tracked loader/monitor templates do not match the source env")

    for key in ("AWS_ACCESS_KEY_ID", "AWS_SECRET_ACCESS_KEY"):
        if any(c in values[key] for c in "\r\n\0\"\\"):
            raise ValueError("An S3 credential cannot be represented safely in the templates")
    loader_template = loader_template.replace(
        "REPLACE_FROM_ACTIVE_LOCAL_ENV_AWS_ACCESS_KEY_ID", values["AWS_ACCESS_KEY_ID"]
    ).replace(
        "REPLACE_FROM_ACTIVE_LOCAL_ENV_AWS_SECRET_ACCESS_KEY", values["AWS_SECRET_ACCESS_KEY"]
    )
    monitor_template = monitor_template.replace(
        "AWS_ACCESS_KEY_ID=REPLACE_WITH_AWS_ACCESS_KEY_ID",
        f'AWS_ACCESS_KEY_ID="{values["AWS_ACCESS_KEY_ID"]}"',
    ).replace(
        "AWS_SECRET_ACCESS_KEY=REPLACE_WITH_AWS_SECRET_ACCESS_KEY",
        f'AWS_SECRET_ACCESS_KEY="{values["AWS_SECRET_ACCESS_KEY"]}"',
    )

    graphdb_env = (ROOT / "deploy/graphdb/.env.vps.example").read_text(encoding="utf-8")
    graphdb_env = graphdb_env.replace(
        "GRAPHDB_LICENSE_FILE=/secure/path/MISIS_GRAPHDB_FREE_v11.3.license",
        "GRAPHDB_LICENSE_FILE=/etc/semantic-bi/graphdb.license",
    )
    runtime_env = (ROOT / "deploy/graphdb/runtime.env.example").read_text(encoding="utf-8")
    runtime_env = runtime_env.replace("REPLACE_WITH_PUBLISHED_TAG", image_tag)

    docker_data = json.loads(docker_config.read_text(encoding="utf-8"))
    auth = docker_data.get("auths", {}).get("ghcr.io")
    if not auth or not (auth.get("auth") or auth.get("identitytoken")):
        raise ValueError("Docker config has no transferable ghcr.io authentication")
    if docker_data.get("credsStore") or docker_data.get("credHelpers", {}).get("ghcr.io"):
        raise ValueError("Docker config uses a credential helper and is not self-contained")
    ghcr_only_config = json.dumps({"auths": {"ghcr.io": auth}}, separators=(",", ":")) + "\n"

    output.parent.mkdir(parents=True, exist_ok=True)
    output_tmp_fd, output_tmp_name = tempfile.mkstemp(prefix=f".{output.name}.", dir=output.parent)
    os.close(output_tmp_fd)
    output_tmp = Path(output_tmp_name)
    try:
        output_tmp.chmod(0o600)
        with tempfile.TemporaryDirectory(prefix="semantic-bi-vps-bundle-") as temp:
            staging = Path(temp)
            staging.chmod(0o700)
            write_private(staging / "graphdb.env", graphdb_env)
            write_private(staging / "runtime.env", runtime_env)
            write_private(staging / "loader.yaml", loader_template)
            write_private(staging / "monitor.env", monitor_template)
            write_private(staging / "graphdb.license", license_file.read_bytes())
            write_private(staging / "docker-config/config.json", ghcr_only_config)
            with tarfile.open(output_tmp, "w:gz") as archive:
                for relative in (
                    "graphdb.env",
                    "runtime.env",
                    "loader.yaml",
                    "monitor.env",
                    "graphdb.license",
                    "docker-config",
                    "docker-config/config.json",
                ):
                    path = staging / relative
                    info = archive.gettarinfo(str(path), arcname=relative)
                    info.uid = info.gid = 0
                    info.uname = info.gname = ""
                    if path.is_dir():
                        info.mode = 0o700
                        archive.addfile(info)
                    else:
                        info.mode = 0o600
                        with path.open("rb") as source:
                            archive.addfile(info, source)
        os.replace(output_tmp, output)
        output.chmod(0o600)
    except Exception:
        output_tmp.unlink(missing_ok=True)
        raise
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-env", type=Path, default=DEFAULT_ENV)
    parser.add_argument("--license-file", type=Path, required=True)
    parser.add_argument("--docker-config", type=Path, required=True)
    parser.add_argument("--image-tag", default="v2.0.1")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()
    try:
        output = build_bundle(
            args.source_env, args.license_file, args.docker_config, args.image_tag, args.output
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    print(f"Private VPS config bundle created: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
