#!/usr/bin/env python3
"""Record and verify Route 1 target revision provenance.

The target workspace is normally synchronized without ``.git``.  A successful
formal deployment therefore writes an explicit workspace marker after the
repository-owned sync/build step.  Before target C runs, this checker compares
that marker's Git-indexed content manifest and build-time hashes for every
remote-build input copied into the target package with the local revision,
live remote workspace, remote build metadata, installed release metadata,
release pointer, and every manifest-listed installed file.
"""

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import pathlib
import shlex
import stat
import subprocess
import sys
from typing import Any


TRACKED_CONTENT_FORMAT = "route1-git-indexed-workspace-v1"
OBC_COMM_SERVICE = "obc-comm-csp-stack.service"
EXPECTED_OBC_DROPINS = {
    "57-csp-socketcan-canfd.conf": (
        "[Service]\n"
        "Environment=COMM_CSP_SOCKETCAN_USE_CANFD=1\n"
        "Environment=COMM_CSP_SOCKETCAN_CANFD_DEST_ALLOWLIST=5,6\n"
        "Environment=COMM_CSP_SOCKETCAN_CANFD_DPORT_ALLOWLIST=40\n"
    ),
    "58-target-beacon-baseline.conf": (
        "[Service]\n"
        "Environment=UHF_BEACON_CSP_NODE=6\n"
    ),
}
PACKAGED_REMOTE_BUILD_INPUTS = {
    "bin/OBC": "build-artifacts/Linux/OBC/bin/OBC",
    "bin/payload_camera_backend_helper": (
        "build-fprime-automatic-native/bin/Linux/"
        "payload_camera_backend_helper"
    ),
    "bin/csp_zmqproxy": "build-fprime-automatic-native/bin/Linux/csp_zmqproxy",
    "bin/eps_simulator": "build-fprime-automatic-native/bin/Linux/eps_simulator",
    "bin/adcs_simulator": "build-fprime-automatic-native/bin/Linux/adcs_simulator",
    "bin/radio_mock_server": (
        "build-fprime-automatic-native/bin/Linux/radio_mock_server"
    ),
    "dict/AppTopologyDictionary.json": (
        "build-artifacts/Linux/OBC/dict/AppTopologyDictionary.json"
    ),
    "meta/version.json": "build-fprime-automatic-native/versions/version.json",
}


REMOTE_CAPTURE_SCRIPT = r"""
import hashlib
import json
import os
import pathlib
import shlex
import stat
import subprocess
import sys

workspace = pathlib.Path(sys.argv[1])
install_root = pathlib.Path(sys.argv[2])
packaged_build_inputs = json.loads(sys.argv[4])
tracked_content_format = "route1-git-indexed-workspace-v1"

def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception as exc:
        return {"_readError": f"{type(exc).__name__}: {exc}", "_path": str(path)}

def sha256(path):
    try:
        digest = hashlib.sha256()
        with path.open("rb") as handle:
            for chunk in iter(lambda: handle.read(1024 * 1024), b""):
                digest.update(chunk)
        return digest.hexdigest()
    except Exception as exc:
        return f"ERROR:{type(exc).__name__}:{exc}"

def tracked_content_summary(root, relative_paths):
    digest = hashlib.sha256()
    digest.update(tracked_content_format.encode("ascii") + b"\0")
    for value in relative_paths:
        if not isinstance(value, str):
            raise ValueError("tracked path is not a string")
        relative = pathlib.PurePosixPath(value)
        if (
            not value
            or relative.is_absolute()
            or any(part in {"", ".", ".."} for part in relative.parts)
        ):
            raise ValueError(f"unsafe tracked path: {value!r}")
        path = root.joinpath(*relative.parts)
        metadata = path.lstat()
        if stat.S_ISLNK(metadata.st_mode):
            mode = b"120000"
            content = os.fsencode(os.readlink(path))
        elif stat.S_ISREG(metadata.st_mode):
            mode = (
                b"100755"
                if metadata.st_mode & stat.S_IXUSR
                else b"100644"
            )
            content = path.read_bytes()
        else:
            raise ValueError(f"unsupported tracked path type: {value!r}")
        path_bytes = os.fsencode(value)
        content_sha256 = hashlib.sha256(content).digest()
        for item in (path_bytes, mode, content_sha256):
            digest.update(len(item).to_bytes(8, "big"))
            digest.update(item)
    return {
        "format": tracked_content_format,
        "algorithm": "sha256",
        "fileCount": len(relative_paths),
        "digest": digest.hexdigest(),
    }

def capture_tracked_content(root, marker):
    try:
        binding = marker.get("trackedContent")
        if not isinstance(binding, dict):
            raise ValueError("workspace marker trackedContent is missing")
        paths = binding.get("paths")
        if not isinstance(paths, list):
            raise ValueError("workspace marker tracked paths are missing")
        return tracked_content_summary(root, paths)
    except Exception as exc:
        return {"_readError": f"{type(exc).__name__}: {exc}"}

def capture_installed_files(root, manifest):
    try:
        files = manifest.get("files")
        if not isinstance(files, dict):
            raise ValueError("installed manifest files are missing")
        root = root.resolve(strict=True)
        captured = {}
        for value in sorted(files):
            if not isinstance(value, str):
                raise ValueError("installed manifest path is not a string")
            relative = pathlib.PurePosixPath(value)
            if (
                not value
                or relative.is_absolute()
                or any(part in {"", ".", ".."} for part in relative.parts)
            ):
                raise ValueError(f"unsafe installed manifest path: {value!r}")
            path = root.joinpath(*relative.parts)
            try:
                metadata = path.lstat()
                if not stat.S_ISREG(metadata.st_mode):
                    raise ValueError(
                        f"installed manifest path is not a regular file: {value!r}"
                    )
                if root not in path.resolve(strict=True).parents:
                    raise ValueError(
                        f"installed manifest path escapes release: {value!r}"
                    )
                captured[value] = {
                    "sizeBytes": metadata.st_size,
                    "sha256": sha256(path),
                }
            except Exception as exc:
                captured[value] = {
                    "_readError": f"{type(exc).__name__}: {exc}"
                }
        return {
            "format": "route1-installed-manifest-files-v1",
            "files": captured,
        }
    except Exception as exc:
        return {"_readError": f"{type(exc).__name__}: {exc}"}

def capture_service_launch(service):
    try:
        fields = (
            "FragmentPath",
            "DropInPaths",
            "ExecStart",
            "WorkingDirectory",
            "MainPID",
        )
        result = subprocess.run(
            [
                "systemctl",
                "show",
                service,
                *[f"--property={field}" for field in fields],
            ],
            capture_output=True,
            text=True,
        )
        if result.returncode != 0:
            raise RuntimeError(
                result.stderr.strip()
                or f"systemctl show exited {result.returncode}"
            )
        show = {}
        for line in result.stdout.splitlines():
            key, separator, value = line.partition("=")
            if separator:
                show[key] = value

        fragment_path = pathlib.Path(show.get("FragmentPath", ""))
        dropin_paths = sorted(shlex.split(show.get("DropInPaths", "")))
        files = {}
        for path in [str(fragment_path), *dropin_paths]:
            candidate = pathlib.Path(path)
            metadata = candidate.lstat()
            if not stat.S_ISREG(metadata.st_mode):
                raise ValueError(
                    f"service input is not a regular file: {path!r}"
                )
            files[path] = {
                "sizeBytes": metadata.st_size,
                "sha256": sha256(candidate),
            }

        main_pid = show.get("MainPID", "")
        if not main_pid.isdigit() or main_pid == "0":
            raise ValueError(f"service MainPID is not active: {main_pid!r}")
        parents = {}
        process_data = {}
        for entry in pathlib.Path("/proc").iterdir():
            if not entry.name.isdigit():
                continue
            try:
                stat_text = (entry / "stat").read_text(
                    encoding="utf-8", errors="replace"
                )
                parent = stat_text.rsplit(") ", 1)[1].split()[1]
                argv = [
                    os.fsdecode(value)
                    for value in (entry / "cmdline").read_bytes().split(b"\0")
                    if value
                ]
                executable = str((entry / "exe").resolve(strict=True))
            except (OSError, IndexError, ValueError):
                continue
            parents.setdefault(parent, []).append(entry.name)
            process_data[entry.name] = {
                "pid": int(entry.name),
                "argv": argv,
                "executable": executable,
                "executableSha256": sha256(pathlib.Path(executable)),
            }
        process_tree = []
        pending = [main_pid]
        seen = set()
        while pending:
            pid = pending.pop()
            if pid in seen:
                continue
            seen.add(pid)
            if pid in process_data:
                process_tree.append(process_data[pid])
            pending.extend(parents.get(pid, []))
        process_tree.sort(key=lambda record: record["pid"])
        return {
            "service": service,
            "show": show,
            "dropInPaths": dropin_paths,
            "files": files,
            "processTree": process_tree,
        }
    except Exception as exc:
        return {"_readError": f"{type(exc).__name__}: {exc}"}

git_head = None
git_error = None
if (workspace / ".git").exists():
    result = subprocess.run(
        ["git", "-C", str(workspace), "rev-parse", "HEAD"],
        capture_output=True,
        text=True,
    )
    if result.returncode == 0:
        git_head = result.stdout.strip()
    else:
        git_error = result.stderr.strip() or f"git exited {result.returncode}"

marker = read_json(workspace / ".route1-source-provenance.json")
current = install_root / "current"
release_pointer = str(current.resolve()) if current.exists() else None
installed_manifest_path = current / "manifest.json"
installed_manifest = read_json(installed_manifest_path)
remote_built_obc = workspace / "build-artifacts/Linux/OBC/bin/OBC"
installed_obc = current / "bin/OBC"
remote_build_inputs = {
    package_path: {
        "path": str(workspace / workspace_path),
        "sha256": sha256(workspace / workspace_path),
    }
    for package_path, workspace_path in sorted(packaged_build_inputs.items())
}
payload = {
    "workspaceMarker": marker,
    "workspaceTrackedContent": capture_tracked_content(workspace, marker),
    "workspaceGitHead": git_head,
    "workspaceGitError": git_error,
    "remoteBuildMetadata": read_json(
        workspace / "build-fprime-automatic-native/versions/version.json"
    ),
    "remoteBuiltObcPath": str(remote_built_obc),
    "remoteBuiltObcSha256": sha256(remote_built_obc),
    "remoteBuildInputs": remote_build_inputs,
    "installedReleasePointer": release_pointer,
    "installedManifest": installed_manifest,
    "installedManifestSha256": sha256(installed_manifest_path),
    "installedManifestFileAudit": capture_installed_files(
        current, installed_manifest
    ),
    "installedBuildMetadata": read_json(current / "meta/version.json"),
    "installedObcPath": str(installed_obc.resolve()) if installed_obc.exists() else None,
    "installedObcSha256": sha256(installed_obc),
    "serviceLaunchAudit": capture_service_launch(sys.argv[3]),
}
print(json.dumps(payload, sort_keys=True))
"""

REMOTE_MARKER_SCRIPT = r"""
import hashlib
import json
import pathlib
import sys

path = pathlib.Path(sys.argv[1])
build_inputs = json.loads(sys.argv[2])

def sha256(path):
    digest = hashlib.sha256()
    with pathlib.Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()

marker = json.loads(sys.stdin.read())
marker["builtInputSha256"] = {
    package_path: sha256(workspace_path)
    for package_path, workspace_path in sorted(build_inputs.items())
}
path.parent.mkdir(parents=True, exist_ok=True)
path.write_text(
    json.dumps(marker, indent=2, sort_keys=True) + "\n",
    encoding="utf-8",
)
print(json.dumps(marker, sort_keys=True))
"""


def run(
    command: list[str],
    *,
    input_text: str | None = None,
    timeout: int = 60,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        input=input_text,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
    )


def git_capture(repo_root: pathlib.Path, *args: str) -> str:
    result = run(["git", "-C", str(repo_root), *args])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"git {' '.join(args)} failed")
    return result.stdout.strip()


def git_indexed_paths(repo_root: pathlib.Path) -> list[str]:
    result = run(
        [
            "git",
            "-C",
            str(repo_root),
            "ls-files",
            "--recurse-submodules",
            "-z",
        ]
    )
    if result.returncode != 0:
        raise RuntimeError(
            result.stderr.strip() or "git ls-files --recurse-submodules failed"
        )
    paths = [value for value in result.stdout.split("\0") if value]
    if paths != sorted(paths) or len(paths) != len(set(paths)):
        raise RuntimeError("Git-indexed path list is not sorted and unique")
    return paths


def tracked_path_content(
    root: pathlib.Path, value: str
) -> tuple[str, bytes]:
    relative = pathlib.PurePosixPath(value)
    if (
        not value
        or relative.is_absolute()
        or any(part in {"", ".", ".."} for part in relative.parts)
    ):
        raise RuntimeError(f"unsafe Git-indexed path: {value!r}")
    path = root.joinpath(*relative.parts)
    try:
        metadata = path.lstat()
        if stat.S_ISLNK(metadata.st_mode):
            return "120000", os.fsencode(os.readlink(path))
        if stat.S_ISREG(metadata.st_mode):
            mode = "100755" if metadata.st_mode & stat.S_IXUSR else "100644"
            return mode, path.read_bytes()
        raise RuntimeError(f"unsupported tracked path type: {value!r}")
    except OSError as exc:
        raise RuntimeError(
            f"cannot read Git-indexed path {value!r}: {exc}"
        ) from exc


def git_blob_oid(content: bytes, expected_oid: str) -> str:
    object_bytes = f"blob {len(content)}\0".encode("ascii") + content
    if len(expected_oid) == 40:
        return hashlib.sha1(object_bytes).hexdigest()
    if len(expected_oid) == 64:
        return hashlib.sha256(object_bytes).hexdigest()
    raise RuntimeError(
        f"unsupported Git object id length: {len(expected_oid)}"
    )


def git_index_entries(
    repo_root: pathlib.Path,
) -> dict[str, tuple[str, str]]:
    result = run(
        [
            "git",
            "-C",
            str(repo_root),
            "ls-files",
            "--stage",
            "--recurse-submodules",
            "-z",
        ]
    )
    if result.returncode != 0:
        raise RuntimeError(
            result.stderr.strip()
            or "git ls-files --stage --recurse-submodules failed"
        )
    entries: dict[str, tuple[str, str]] = {}
    for raw_entry in result.stdout.split("\0"):
        if not raw_entry:
            continue
        metadata, separator, path = raw_entry.partition("\t")
        fields = metadata.split()
        if not separator or len(fields) != 3 or fields[2] != "0":
            raise RuntimeError(f"invalid Git index entry: {raw_entry!r}")
        mode, oid, _stage = fields
        if path in entries:
            raise RuntimeError(f"duplicate Git index path: {path!r}")
        entries[path] = (mode, oid)
    if list(entries) != sorted(entries):
        raise RuntimeError("Git index entries are not sorted")
    return entries


def require_index_matches_committed_tree(repo_root: pathlib.Path) -> None:
    cached = run(
        [
            "git",
            "-C",
            str(repo_root),
            "diff",
            "--cached",
            "--quiet",
            "--no-ext-diff",
            "HEAD",
            "--",
        ]
    )
    if cached.returncode != 0:
        raise RuntimeError(
            "Git index does not match the committed HEAD tree"
            + (f": {cached.stderr.strip()}" if cached.stderr.strip() else "")
        )

    submodule_status = run(
        [
            "git",
            "-C",
            str(repo_root),
            "submodule",
            "status",
            "--recursive",
        ]
    )
    if submodule_status.returncode != 0:
        raise RuntimeError(
            submodule_status.stderr.strip()
            or "cannot verify committed submodule revisions"
        )
    divergent = [
        line
        for line in submodule_status.stdout.splitlines()
        if line and not line.startswith(" ")
    ]
    if divergent:
        raise RuntimeError(
            "submodule revision does not match the committed HEAD tree: "
            + "; ".join(divergent)
        )

    submodule_indexes = run(
        [
            "git",
            "-C",
            str(repo_root),
            "submodule",
            "foreach",
            "--quiet",
            "--recursive",
            "git diff --cached --quiet --no-ext-diff HEAD --",
        ]
    )
    if submodule_indexes.returncode != 0:
        raise RuntimeError(
            "a submodule index does not match its committed HEAD tree"
            + (
                f": {submodule_indexes.stderr.strip()}"
                if submodule_indexes.stderr.strip()
                else ""
            )
        )


def tracked_content_summary(
    root: pathlib.Path,
    relative_paths: list[str],
    *,
    expected_index_entries: dict[str, tuple[str, str]] | None = None,
) -> dict[str, Any]:
    digest = hashlib.sha256()
    digest.update(TRACKED_CONTENT_FORMAT.encode("ascii") + b"\0")
    for value in relative_paths:
        mode, content = tracked_path_content(root, value)
        if expected_index_entries is not None:
            expected = expected_index_entries.get(value)
            if expected is None:
                raise RuntimeError(
                    f"Git-indexed path is absent from the committed index: {value!r}"
                )
            expected_mode, expected_oid = expected
            actual_oid = git_blob_oid(content, expected_oid)
            if mode != expected_mode or actual_oid != expected_oid:
                raise RuntimeError(
                    "serialized Git-indexed path does not match the committed "
                    f"HEAD tree: {value!r}"
                )
        path_bytes = os.fsencode(value)
        content_sha256 = hashlib.sha256(content).digest()
        for item in (path_bytes, mode.encode("ascii"), content_sha256):
            digest.update(len(item).to_bytes(8, "big"))
            digest.update(item)
    return {
        "format": TRACKED_CONTENT_FORMAT,
        "algorithm": "sha256",
        "fileCount": len(relative_paths),
        "digest": digest.hexdigest(),
    }


def tracked_workspace_binding(repo_root: pathlib.Path) -> dict[str, Any]:
    require_index_matches_committed_tree(repo_root)
    paths = git_indexed_paths(repo_root)
    index_entries = git_index_entries(repo_root)
    if paths != list(index_entries):
        raise RuntimeError(
            "Git-indexed sync paths do not match the committed index entries"
        )
    return {
        **tracked_content_summary(
            repo_root,
            paths,
            expected_index_entries=index_entries,
        ),
        "paths": paths,
    }


def tracked_content_without_paths(binding: dict[str, Any]) -> dict[str, Any]:
    return {
        field: binding.get(field)
        for field in ("format", "algorithm", "fileCount", "digest")
    }


def untracked_paths(repo_root: pathlib.Path) -> list[str]:
    result = run(
        [
            "git",
            "-C",
            str(repo_root),
            "status",
            "--porcelain=v1",
            "-z",
            "--untracked-files=all",
        ]
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "git status failed")
    return sorted(
        entry[3:]
        for entry in result.stdout.split("\0")
        if entry and entry[:2] == "??"
    )


def relative_allowed_roots(
    repo_root: pathlib.Path, allowed_untracked_roots: list[str]
) -> list[pathlib.PurePosixPath]:
    roots: list[pathlib.PurePosixPath] = []
    for value in allowed_untracked_roots:
        candidate = pathlib.Path(value)
        if not candidate.is_absolute():
            candidate = repo_root / candidate
        try:
            relative = candidate.resolve().relative_to(repo_root.resolve())
        except ValueError:
            continue
        roots.append(pathlib.PurePosixPath(relative.as_posix()))
    return roots


def local_identity(
    repo_root: pathlib.Path,
    allowed_untracked_roots: list[str],
    *,
    tracked_content_binding: dict[str, Any] | None = None,
) -> dict[str, Any]:
    untracked = untracked_paths(repo_root)
    allowed_roots = relative_allowed_roots(repo_root, allowed_untracked_roots)
    unexpected: list[str] = []
    for value in untracked:
        path = pathlib.PurePosixPath(value)
        if not any(path == root or root in path.parents for root in allowed_roots):
            unexpected.append(value)
    tracked_content = tracked_content_binding or tracked_workspace_binding(repo_root)
    return {
        "branch": git_capture(repo_root, "branch", "--show-current"),
        "head": git_capture(repo_root, "rev-parse", "HEAD"),
        "projectVersion": git_capture(
            repo_root, "describe", "--tags", "--always", "--dirty", "--broken"
        ),
        "status": git_capture(repo_root, "status", "--short"),
        "untrackedPaths": untracked,
        "allowedUntrackedRoots": [str(root) for root in allowed_roots],
        "unexpectedUntrackedPaths": unexpected,
        "trackedContent": tracked_content_without_paths(tracked_content),
    }


def require_markable_local_identity(identity: dict[str, Any]) -> None:
    if not identity["branch"]:
        raise RuntimeError("local branch is empty; refusing a detached revision")
    if str(identity["projectVersion"]).endswith("-dirty"):
        raise RuntimeError("tracked local changes make the intended revision ambiguous")
    if identity["unexpectedUntrackedPaths"]:
        raise RuntimeError(
            "untracked inputs are not excluded from target sync: "
            + ", ".join(identity["unexpectedUntrackedPaths"])
        )


def campaign_identity_payload(identity: dict[str, Any]) -> dict[str, Any]:
    return {
        "schemaVersion": 1,
        "branch": identity["branch"],
        "head": identity["head"],
        "projectVersion": identity["projectVersion"],
        "recordedAtUtc": dt.datetime.now(dt.timezone.utc)
        .replace(microsecond=0)
        .isoformat(),
    }


def campaign_identity_mismatches(
    recorded: dict[str, Any], current: dict[str, Any]
) -> list[str]:
    return [
        field
        for field in ("branch", "head", "projectVersion")
        if not recorded.get(field) or recorded.get(field) != current.get(field)
    ]


def write_campaign_identity(
    repo_root: pathlib.Path,
    output: pathlib.Path,
    allowed_untracked_roots: list[str],
) -> dict[str, Any]:
    identity = local_identity(repo_root, allowed_untracked_roots)
    require_markable_local_identity(identity)
    payload = campaign_identity_payload(identity)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return payload


def check_campaign_identity(
    repo_root: pathlib.Path,
    identity_path: pathlib.Path,
    allowed_untracked_roots: list[str],
    campaign_manifest_path: pathlib.Path | None = None,
) -> dict[str, Any]:
    current = local_identity(repo_root, allowed_untracked_roots)
    require_markable_local_identity(current)
    if not identity_path.is_file():
        raise RuntimeError(
            f"campaign source identity is missing: {identity_path}"
        )
    try:
        recorded = json.loads(identity_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RuntimeError(
            f"campaign source identity is invalid: {identity_path}: {exc}"
        ) from exc
    if not isinstance(recorded, dict):
        raise RuntimeError(
            f"campaign source identity must be an object: {identity_path}"
        )
    if recorded.get("schemaVersion") != 1:
        raise RuntimeError(
            f"campaign source identity schema is unsupported: {identity_path}"
        )
    mismatches = campaign_identity_mismatches(recorded, current)
    if mismatches:
        raise RuntimeError(
            "campaign source identity does not match the current revision: "
            + ", ".join(mismatches)
        )
    if campaign_manifest_path is not None:
        campaign = load_json_object(
            campaign_manifest_path, "campaign manifest"
        )
        retained_identity = campaign.get("campaignSourceIdentity")
        if (
            not isinstance(retained_identity, dict)
            or retained_identity.get("path")
            != "deployment/campaign-source-identity.json"
        ):
            raise RuntimeError(
                "campaign manifest source identity is missing or invalid: "
                f"{campaign_manifest_path}"
            )
        retained_mismatches = campaign_identity_mismatches(
            retained_identity, recorded
        )
        if retained_mismatches:
            raise RuntimeError(
                "campaign source identity does not match the retained "
                "campaign manifest: "
                + ", ".join(retained_mismatches)
            )
    return recorded


def ssh_python(
    ssh_target: str,
    script: str,
    args: list[str],
    *,
    timeout: int = 60,
) -> subprocess.CompletedProcess[str]:
    remote_command = shlex.join(["python3", "-", *args])
    return run(
        ["ssh", "-o", f"ConnectTimeout={min(timeout, 30)}", ssh_target, remote_command],
        input_text=script,
        timeout=timeout,
    )


def write_workspace_marker(
    repo_root: pathlib.Path,
    ssh_target: str,
    remote_workspace: str,
    timeout: int,
    allowed_untracked_roots: list[str],
) -> dict[str, Any]:
    tracked_content = tracked_workspace_binding(repo_root)
    identity = local_identity(
        repo_root,
        allowed_untracked_roots,
        tracked_content_binding=tracked_content,
    )
    require_markable_local_identity(identity)

    marker = {
        "schemaVersion": 3,
        "branch": identity["branch"],
        "head": identity["head"],
        "projectVersion": identity["projectVersion"],
        "recordedAtUtc": dt.datetime.now(dt.timezone.utc)
        .replace(microsecond=0)
        .isoformat(),
        "syncOwner": "scripts/bootstrap_rpi_workspace.sh",
        "trackedContent": tracked_content,
    }
    marker_text = json.dumps(marker, indent=2, sort_keys=True) + "\n"
    marker_path = str(
        pathlib.PurePosixPath(remote_workspace) / ".route1-source-provenance.json"
    )
    remote_build_inputs = {
        package_path: str(
            pathlib.PurePosixPath(remote_workspace) / workspace_path
        )
        for package_path, workspace_path in PACKAGED_REMOTE_BUILD_INPUTS.items()
    }
    remote_command = shlex.join(
        [
            "python3",
            "-c",
            REMOTE_MARKER_SCRIPT,
            marker_path,
            json.dumps(remote_build_inputs, sort_keys=True),
        ]
    )
    result = run(
        [
            "ssh",
            "-o",
            f"ConnectTimeout={min(timeout, 30)}",
            ssh_target,
            remote_command,
        ],
        input_text=marker_text,
        timeout=timeout,
    )
    if result.returncode != 0:
        raise RuntimeError(
            result.stderr.strip()
            or result.stdout.strip()
            or "remote workspace marker write failed"
        )
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    if not lines:
        raise RuntimeError("remote workspace marker write returned no JSON")
    try:
        written_marker = json.loads(lines[-1])
    except json.JSONDecodeError as exc:
        raise RuntimeError(
            f"remote workspace marker response is invalid: {exc}"
        ) from exc
    if not isinstance(written_marker, dict):
        raise RuntimeError("remote workspace marker response must be an object")
    return written_marker


def capture_remote(
    ssh_target: str,
    remote_workspace: str,
    install_root: str,
    timeout: int,
) -> dict[str, Any]:
    result = ssh_python(
        ssh_target,
        REMOTE_CAPTURE_SCRIPT,
        [
            remote_workspace,
            install_root,
            OBC_COMM_SERVICE,
            json.dumps(PACKAGED_REMOTE_BUILD_INPUTS, sort_keys=True),
        ],
        timeout=timeout,
    )
    if result.returncode != 0:
        return {
            "captureError": result.stderr.strip()
            or result.stdout.strip()
            or f"ssh exited {result.returncode}"
        }
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    if not lines:
        return {"captureError": "remote provenance capture returned no JSON"}
    try:
        return json.loads(lines[-1])
    except json.JSONDecodeError as exc:
        return {
            "captureError": f"remote provenance JSON decode failed: {exc}",
            "stdout": result.stdout,
        }


def json_value(payload: Any, key: str) -> Any:
    return payload.get(key) if isinstance(payload, dict) else None


def valid_tracked_content_summary(payload: Any) -> bool:
    if not isinstance(payload, dict):
        return False
    digest = payload.get("digest")
    file_count = payload.get("fileCount")
    return (
        payload.get("format") == TRACKED_CONTENT_FORMAT
        and payload.get("algorithm") == "sha256"
        and isinstance(file_count, int)
        and not isinstance(file_count, bool)
        and file_count >= 0
        and isinstance(digest, str)
        and len(digest) == 64
        and all(char in "0123456789abcdef" for char in digest.lower())
    )


def valid_tracked_paths(payload: Any) -> bool:
    if not isinstance(payload, dict):
        return False
    paths = payload.get("paths")
    if (
        not isinstance(paths, list)
        or paths != sorted(paths)
        or len(paths) != len(set(paths))
        or len(paths) != payload.get("fileCount")
    ):
        return False
    for value in paths:
        if not isinstance(value, str):
            return False
        relative = pathlib.PurePosixPath(value)
        if (
            not value
            or relative.is_absolute()
            or any(part in {"", ".", ".."} for part in relative.parts)
        ):
            return False
    return True


def valid_sha256(value: Any) -> bool:
    return (
        isinstance(value, str)
        and len(value) == 64
        and all(char in "0123456789abcdef" for char in value.lower())
    )


def trusted_manifest_binding(path: pathlib.Path) -> dict[str, Any]:
    try:
        manifest_bytes = path.read_bytes()
        manifest = json.loads(manifest_bytes.decode("utf-8"))
        if not isinstance(manifest, dict):
            raise ValueError("manifest must be a JSON object")
        return {
            "path": str(path),
            "sha256": hashlib.sha256(manifest_bytes).hexdigest(),
            "manifest": manifest,
        }
    except (OSError, UnicodeDecodeError, json.JSONDecodeError, ValueError) as exc:
        return {
            "path": str(path),
            "_readError": f"{type(exc).__name__}: {exc}",
        }


def trusted_file_binding(path: pathlib.Path) -> dict[str, Any]:
    try:
        content = path.read_bytes()
        return {
            "path": str(path),
            "sizeBytes": len(content),
            "sha256": hashlib.sha256(content).hexdigest(),
        }
    except OSError as exc:
        return {
            "path": str(path),
            "_readError": f"{type(exc).__name__}: {exc}",
        }


def safe_manifest_path(value: Any) -> bool:
    if not isinstance(value, str) or not value:
        return False
    relative = pathlib.PurePosixPath(value)
    return not relative.is_absolute() and not any(
        part in {"", ".", ".."} for part in relative.parts
    )


def evaluate_installed_manifest_files(
    manifest: dict[str, Any], remote: dict[str, Any]
) -> list[str]:
    failures: list[str] = []
    if not valid_sha256(remote.get("installedManifestSha256")):
        failures.append("installed-manifest-hash-missing")

    expected_files = manifest.get("files")
    if not isinstance(expected_files, dict) or not expected_files:
        return failures + ["installed-manifest-files-invalid"]

    expected: dict[str, dict[str, Any]] = {}
    for path, record in expected_files.items():
        if (
            not safe_manifest_path(path)
            or not isinstance(record, dict)
            or not isinstance(record.get("size_bytes"), int)
            or isinstance(record.get("size_bytes"), bool)
            or record.get("size_bytes") < 0
            or not valid_sha256(record.get("sha256"))
        ):
            failures.append("installed-manifest-files-invalid")
            break
        expected[path] = record

    audit = remote.get("installedManifestFileAudit")
    if (
        not isinstance(audit, dict)
        or audit.get("_readError")
        or audit.get("format") != "route1-installed-manifest-files-v1"
        or not isinstance(audit.get("files"), dict)
    ):
        return list(
            dict.fromkeys(failures + ["installed-manifest-file-audit-unreadable"])
        )

    actual_files = audit["files"]
    if set(actual_files) != set(expected_files):
        failures.append("installed-manifest-file-set-mismatch")
        return list(dict.fromkeys(failures))

    for path in sorted(expected):
        actual = actual_files.get(path)
        if not isinstance(actual, dict) or actual.get("_readError"):
            failures.append("installed-manifest-file-audit-unreadable")
            continue
        if (
            not isinstance(actual.get("sizeBytes"), int)
            or isinstance(actual.get("sizeBytes"), bool)
            or not valid_sha256(actual.get("sha256"))
        ):
            failures.append("installed-manifest-file-audit-unreadable")
            continue
        if actual["sizeBytes"] != expected[path]["size_bytes"]:
            failures.append("installed-manifest-file-size-mismatch")
        if actual["sha256"] != expected[path]["sha256"]:
            failures.append("installed-manifest-file-hash-mismatch")
    return list(dict.fromkeys(failures))


def evaluate_service_launch(
    local: dict[str, Any],
    remote: dict[str, Any],
    install_root: str,
) -> list[str]:
    failures: list[str] = []
    trusted = local.get("trustedServiceUnit")
    if (
        not isinstance(trusted, dict)
        or trusted.get("_readError")
        or not isinstance(trusted.get("sizeBytes"), int)
        or not valid_sha256(trusted.get("sha256"))
    ):
        return ["trusted-service-unit-missing"]

    audit = remote.get("serviceLaunchAudit")
    if not isinstance(audit, dict) or audit.get("_readError"):
        return ["service-launch-audit-unreadable"]
    if audit.get("service") != OBC_COMM_SERVICE:
        failures.append("service-launch-name-mismatch")

    expected_fragment = f"/etc/systemd/system/{OBC_COMM_SERVICE}"
    expected_dropins = {
        f"/etc/systemd/system/{OBC_COMM_SERVICE}.d/{name}": {
            "sizeBytes": len(content.encode("utf-8")),
            "sha256": hashlib.sha256(content.encode("utf-8")).hexdigest(),
        }
        for name, content in EXPECTED_OBC_DROPINS.items()
    }
    files = audit.get("files")
    if not isinstance(files, dict):
        return failures + ["service-launch-files-unreadable"]
    fragment = files.get(expected_fragment)
    if (
        not isinstance(fragment, dict)
        or fragment.get("sizeBytes") != trusted["sizeBytes"]
        or fragment.get("sha256") != trusted["sha256"]
    ):
        failures.append("service-unit-receipt-mismatch")
    actual_dropins = audit.get("dropInPaths")
    if (
        not isinstance(actual_dropins, list)
        or actual_dropins != sorted(expected_dropins)
    ):
        failures.append("service-dropin-set-mismatch")
    for path, expected in expected_dropins.items():
        if files.get(path) != expected:
            failures.append("service-dropin-content-mismatch")
            break
    if set(files) != {expected_fragment, *expected_dropins}:
        failures.append("service-launch-file-set-mismatch")

    show = audit.get("show")
    launch_path = f"{install_root}/current/launch/run_obc_comm_csp_stack.sh"
    if not isinstance(show, dict):
        failures.append("service-effective-properties-unreadable")
    else:
        if show.get("FragmentPath") != expected_fragment:
            failures.append("service-fragment-path-mismatch")
        if show.get("WorkingDirectory") != f"{install_root}/current":
            failures.append("service-working-directory-mismatch")
        exec_start = show.get("ExecStart")
        if not isinstance(exec_start, str) or launch_path not in exec_start:
            failures.append("service-exec-start-mismatch")

    release_pointer = str(remote.get("installedReleasePointer") or "")
    expected_executables = {
        f"{release_pointer}/bin/OBC",
        f"{release_pointer}/bin/radio_mock_server",
        "/usr/bin/bash",
        "/bin/bash",
    }
    process_tree = audit.get("processTree")
    if not isinstance(process_tree, list) or not process_tree:
        failures.append("service-process-tree-missing")
    else:
        launchers = 0
        obc_processes = 0
        radio_processes = 0
        manifest_files = json_value(remote.get("installedManifest"), "files")
        for record in process_tree:
            if (
                not isinstance(record, dict)
                or not isinstance(record.get("argv"), list)
                or not isinstance(record.get("executable"), str)
                or not valid_sha256(record.get("executableSha256"))
            ):
                failures.append("service-process-tree-unreadable")
                continue
            executable = record["executable"]
            argv = record["argv"]
            if executable not in expected_executables:
                failures.append("service-process-executable-unexpected")
            if launch_path in argv:
                launchers += 1
            if executable == f"{release_pointer}/bin/OBC":
                obc_processes += 1
                if record["executableSha256"] != remote.get(
                    "installedObcSha256"
                ):
                    failures.append("service-process-obc-hash-mismatch")
            if executable == f"{release_pointer}/bin/radio_mock_server":
                radio_processes += 1
                radio_manifest = json_value(
                    json_value(manifest_files, "bin/radio_mock_server"),
                    "sha256",
                )
                if record["executableSha256"] != radio_manifest:
                    failures.append("service-process-radio-hash-mismatch")
        if launchers != 1:
            failures.append("service-launcher-process-mismatch")
        if obc_processes != 1:
            failures.append("service-obc-process-mismatch")
        if radio_processes != 1:
            failures.append("service-radio-process-mismatch")
    return list(dict.fromkeys(failures))


def evaluate_provenance(
    local: dict[str, Any],
    remote: dict[str, Any],
    remote_workspace: str,
    install_root: str,
) -> list[str]:
    failures: list[str] = []
    branch = str(local.get("branch") or "")
    head = str(local.get("head") or "")
    project_version = str(local.get("projectVersion") or "")

    if not branch:
        failures.append("local-branch-missing")
    if len(head) != 40 or any(char not in "0123456789abcdef" for char in head.lower()):
        failures.append("local-head-invalid")
    if not project_version:
        failures.append("local-project-version-missing")
    elif project_version.endswith("-dirty"):
        failures.append("local-tracked-worktree-dirty")
    unexpected_untracked = local.get("unexpectedUntrackedPaths")
    if not isinstance(unexpected_untracked, list):
        failures.append("local-untracked-audit-missing")
    elif unexpected_untracked:
        failures.append("local-untracked-inputs-present")

    capture_error = remote.get("captureError")
    if capture_error:
        failures.append("remote-capture-failed")
        return failures

    marker = remote.get("workspaceMarker")
    if not isinstance(marker, dict) or marker.get("_readError"):
        failures.append("remote-workspace-marker-missing")
    else:
        if marker.get("schemaVersion") != 3:
            failures.append("remote-workspace-marker-schema-mismatch")
        if marker.get("branch") != branch:
            failures.append("remote-workspace-branch-mismatch")
        if marker.get("head") != head:
            failures.append("remote-workspace-head-mismatch")
        if marker.get("projectVersion") != project_version:
            failures.append("remote-workspace-version-mismatch")

    local_tracked_content = local.get("trackedContent")
    if not valid_tracked_content_summary(local_tracked_content):
        failures.append("local-tracked-content-audit-missing")

    marker_tracked_content = (
        marker.get("trackedContent") if isinstance(marker, dict) else None
    )
    if not (
        valid_tracked_content_summary(marker_tracked_content)
        and valid_tracked_paths(marker_tracked_content)
    ):
        failures.append("remote-workspace-content-manifest-invalid")
    elif (
        valid_tracked_content_summary(local_tracked_content)
        and tracked_content_without_paths(marker_tracked_content)
        != local_tracked_content
    ):
        failures.append("remote-workspace-content-marker-mismatch")

    remote_tracked_content = remote.get("workspaceTrackedContent")
    if not valid_tracked_content_summary(remote_tracked_content):
        failures.append("remote-workspace-content-unreadable")
    elif (
        valid_tracked_content_summary(local_tracked_content)
        and remote_tracked_content != local_tracked_content
    ):
        failures.append("remote-workspace-content-mismatch")

    workspace_git_error = remote.get("workspaceGitError")
    if workspace_git_error:
        failures.append("remote-workspace-git-unreadable")
    workspace_git_head = remote.get("workspaceGitHead")
    if workspace_git_head is not None and workspace_git_head != head:
        failures.append("remote-workspace-git-head-mismatch")

    remote_build = remote.get("remoteBuildMetadata")
    if (
        not isinstance(remote_build, dict)
        or remote_build.get("_readError")
        or json_value(remote_build, "project_version") != project_version
    ):
        failures.append("remote-build-version-mismatch")

    manifest = remote.get("installedManifest")
    if not isinstance(manifest, dict) or manifest.get("_readError"):
        failures.append("installed-manifest-missing")
        manifest = {}
    else:
        if manifest.get("project_version") != project_version:
            failures.append("installed-manifest-version-mismatch")
        if manifest.get("source_remote_dir") != remote_workspace:
            failures.append("installed-manifest-workspace-mismatch")
        trusted_manifest = local.get("trustedInstalledManifest")
        if (
            not isinstance(trusted_manifest, dict)
            or trusted_manifest.get("_readError")
            or not valid_sha256(trusted_manifest.get("sha256"))
            or not isinstance(trusted_manifest.get("manifest"), dict)
        ):
            failures.append("trusted-installed-manifest-missing")
        else:
            if (
                remote.get("installedManifestSha256")
                != trusted_manifest["sha256"]
            ):
                failures.append("installed-manifest-trusted-hash-mismatch")
            if manifest != trusted_manifest["manifest"]:
                failures.append("installed-manifest-trusted-content-mismatch")
        failures.extend(evaluate_installed_manifest_files(manifest, remote))

    installed_build = remote.get("installedBuildMetadata")
    if (
        not isinstance(installed_build, dict)
        or installed_build.get("_readError")
        or json_value(installed_build, "project_version") != project_version
    ):
        failures.append("installed-build-version-mismatch")

    release_id = str(manifest.get("release_id") or "")
    expected_pointer = str(
        pathlib.PurePosixPath(install_root) / "releases" / release_id
    )
    if not release_id or remote.get("installedReleasePointer") != expected_pointer:
        failures.append("installed-release-pointer-mismatch")

    installed_obc_path = str(remote.get("installedObcPath") or "")
    if not installed_obc_path.startswith(expected_pointer + "/"):
        failures.append("installed-obc-path-mismatch")
    installed_obc_sha = str(remote.get("installedObcSha256") or "")
    manifest_obc_sha = json_value(json_value(manifest, "files"), "bin/OBC")
    manifest_obc_sha = json_value(manifest_obc_sha, "sha256")
    if len(installed_obc_sha) != 64 or installed_obc_sha != manifest_obc_sha:
        failures.append("installed-obc-hash-mismatch")

    marker_build_inputs = (
        marker.get("builtInputSha256") if isinstance(marker, dict) else None
    )
    remote_build_inputs = remote.get("remoteBuildInputs")
    manifest_files = manifest.get("files")
    expected_input_paths = set(PACKAGED_REMOTE_BUILD_INPUTS)
    if (
        not isinstance(marker_build_inputs, dict)
        or set(marker_build_inputs) != expected_input_paths
        or any(
            not valid_sha256(value)
            for value in marker_build_inputs.values()
        )
    ):
        failures.append("remote-workspace-marker-build-inputs-invalid")
    if (
        not isinstance(remote_build_inputs, dict)
        or set(remote_build_inputs) != expected_input_paths
    ):
        failures.append("remote-build-input-audit-invalid")
    elif isinstance(marker_build_inputs, dict):
        for package_path, workspace_path in PACKAGED_REMOTE_BUILD_INPUTS.items():
            record = remote_build_inputs.get(package_path)
            expected_path = str(
                pathlib.PurePosixPath(remote_workspace) / workspace_path
            )
            if (
                not isinstance(record, dict)
                or record.get("path") != expected_path
                or not valid_sha256(record.get("sha256"))
            ):
                failures.append("remote-build-input-audit-invalid")
                continue
            if record["sha256"] != marker_build_inputs.get(package_path):
                failures.append("remote-build-input-marker-hash-mismatch")
            manifest_record = (
                manifest_files.get(package_path)
                if isinstance(manifest_files, dict)
                else None
            )
            if (
                not isinstance(manifest_record, dict)
                or manifest_record.get("sha256")
                != marker_build_inputs.get(package_path)
            ):
                failures.append("packaged-build-input-hash-mismatch")

    failures.extend(evaluate_service_launch(local, remote, install_root))
    return failures


def write_result(
    output: pathlib.Path,
    local: dict[str, Any],
    remote: dict[str, Any],
    remote_workspace: str,
    install_root: str,
) -> list[str]:
    failures = evaluate_provenance(local, remote, remote_workspace, install_root)
    payload = {
        "schemaVersion": 1,
        "checkedAtUtc": dt.datetime.now(dt.timezone.utc)
        .replace(microsecond=0)
        .isoformat(),
        "verdict": "PASS" if not failures else "FAIL",
        "failures": failures,
        "local": local,
        "remote": remote,
        "expected": {
            "remoteWorkspace": remote_workspace,
            "installRoot": install_root,
        },
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    return failures


def retained_target_resume_state(
    campaign: dict[str, Any],
    provenance: dict[str, Any],
    provenance_sha256: str,
) -> str:
    attempts = campaign.get("attempts")
    if not isinstance(attempts, list):
        raise RuntimeError("campaign attempts are missing or invalid")
    target_attempts = [
        record
        for record in attempts
        if isinstance(record, dict) and record.get("surface") == "target"
    ]
    if not target_attempts or target_attempts[-1].get("verdict") != "PASS":
        return "PENDING"

    latest = target_attempts[-1]
    if latest.get("authoritative") is not True:
        raise RuntimeError(
            "retained target PASS is not authoritative; refusing provenance rebinding"
        )
    retained_sha256 = latest.get("targetRevisionProvenanceSha256")
    if (
        not isinstance(retained_sha256, str)
        or len(retained_sha256) != 64
        or retained_sha256 != provenance_sha256
    ):
        raise RuntimeError(
            "retained target PASS provenance hash is missing or mismatched"
        )
    campaign_provenance = campaign.get("targetRevisionProvenance")
    if (
        not isinstance(campaign_provenance, dict)
        or campaign_provenance.get("verdict") != "PASS"
        or campaign_provenance.get("path")
        != "deployment/target-revision-provenance.json"
        or campaign_provenance.get("failures") != []
    ):
        raise RuntimeError(
            "retained target PASS lacks campaign-level PASS provenance"
        )
    if provenance.get("schemaVersion") != 1 or provenance.get("verdict") != "PASS":
        raise RuntimeError("retained target provenance is missing or not PASS")
    expected = provenance.get("expected")
    if not isinstance(expected, dict):
        raise RuntimeError("retained target provenance expected inputs are missing")
    failures = evaluate_provenance(
        provenance.get("local", {}),
        provenance.get("remote", {}),
        str(expected.get("remoteWorkspace") or ""),
        str(expected.get("installRoot") or ""),
    )
    if failures:
        raise RuntimeError(
            "retained target provenance no longer validates: "
            + ", ".join(failures)
        )
    return "RETAINED_AUTHORITATIVE_PASS"


def sha256_file(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json_object(path: pathlib.Path, label: str) -> dict[str, Any]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise RuntimeError(f"{label} is missing or invalid: {path}: {exc}") from exc
    if not isinstance(payload, dict):
        raise RuntimeError(f"{label} must contain a JSON object: {path}")
    return payload


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)
    local = subparsers.add_parser("check-local")
    local.add_argument("--repo-root", required=True)
    local.add_argument("--allow-untracked-root", action="append", default=[])
    for name in ("write-campaign-identity", "check-campaign-identity"):
        command = subparsers.add_parser(name)
        command.add_argument("--repo-root", required=True)
        command.add_argument("--identity", required=True)
        command.add_argument("--allow-untracked-root", action="append", default=[])
        if name == "check-campaign-identity":
            command.add_argument("--campaign-manifest")
    for name in ("mark-workspace", "check"):
        command = subparsers.add_parser(name)
        command.add_argument("--repo-root", required=True)
        command.add_argument("--ssh-target", required=True)
        command.add_argument("--remote-workspace", required=True)
        command.add_argument("--timeout", type=int, default=60)
        command.add_argument("--allow-untracked-root", action="append", default=[])
        if name == "check":
            command.add_argument("--install-root", required=True)
            command.add_argument("--trusted-installed-manifest", required=True)
            command.add_argument("--trusted-service-unit", required=True)
            command.add_argument("--output", required=True)
    resume_state = subparsers.add_parser("target-resume-state")
    resume_state.add_argument("--manifest", required=True)
    resume_state.add_argument("--provenance", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    if args.command == "target-resume-state":
        try:
            provenance_path = pathlib.Path(args.provenance).resolve()
            state = retained_target_resume_state(
                load_json_object(
                    pathlib.Path(args.manifest).resolve(), "campaign manifest"
                ),
                (
                    load_json_object(
                        provenance_path,
                        "target revision provenance",
                    )
                    if provenance_path.is_file()
                    else {}
                ),
                sha256_file(provenance_path) if provenance_path.is_file() else "",
            )
        except RuntimeError as exc:
            print(f"Route 1 target resume state failed: {exc}", file=sys.stderr)
            return 1
        print(state)
        return 0

    repo_root = pathlib.Path(args.repo_root).resolve()
    if args.command == "check-local":
        identity = local_identity(repo_root, args.allow_untracked_root)
        require_markable_local_identity(identity)
        print(json.dumps(identity, sort_keys=True))
        return 0
    if args.command == "write-campaign-identity":
        identity = write_campaign_identity(
            repo_root,
            pathlib.Path(args.identity).resolve(),
            args.allow_untracked_root,
        )
        print(json.dumps(identity, sort_keys=True))
        return 0
    if args.command == "check-campaign-identity":
        identity = check_campaign_identity(
            repo_root,
            pathlib.Path(args.identity).resolve(),
            args.allow_untracked_root,
            (
                pathlib.Path(args.campaign_manifest).resolve()
                if args.campaign_manifest
                else None
            ),
        )
        print(json.dumps(identity, sort_keys=True))
        return 0
    if args.command == "mark-workspace":
        marker = write_workspace_marker(
            repo_root,
            args.ssh_target,
            args.remote_workspace,
            args.timeout,
            args.allow_untracked_root,
        )
        print(json.dumps(marker, sort_keys=True))
        return 0

    local = local_identity(repo_root, args.allow_untracked_root)
    local["trustedInstalledManifest"] = trusted_manifest_binding(
        pathlib.Path(args.trusted_installed_manifest).resolve()
    )
    local["trustedServiceUnit"] = trusted_file_binding(
        pathlib.Path(args.trusted_service_unit).resolve()
    )
    remote = capture_remote(
        args.ssh_target, args.remote_workspace, args.install_root, args.timeout
    )
    output = pathlib.Path(args.output).resolve()
    failures = write_result(
        output, local, remote, args.remote_workspace, args.install_root
    )
    print(output)
    if failures:
        print(
            "Route 1 target revision provenance failed: " + ", ".join(failures),
            file=sys.stderr,
        )
        return 1
    print("Route 1 target revision provenance: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
