#!/usr/bin/env python3
"""Focused tests for the Route 1 target revision provenance gate."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import os
import pathlib
import re
import shutil
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
CHECKER = (
    ROOT
    / "scripts/chapter5_routes/check_route1_target_revision_provenance.py"
)
MANIFEST_WRITER = (
    ROOT / "scripts/chapter5_routes/write_route1_campaign_manifest.py"
)
FORMAL_RUNNER = (
    ROOT / "scripts/chapter5_routes/run_route1_sequence_formal_rerun.sh"
)
PACKAGE_SCRIPT = ROOT / "scripts/package_rpi_bundle.sh"
SPEC = importlib.util.spec_from_file_location("route1_target_provenance", CHECKER)
assert SPEC is not None and SPEC.loader is not None
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)
MANIFEST_SPEC = importlib.util.spec_from_file_location(
    "route1_campaign_manifest", MANIFEST_WRITER
)
assert MANIFEST_SPEC is not None and MANIFEST_SPEC.loader is not None
MANIFEST_MODULE = importlib.util.module_from_spec(MANIFEST_SPEC)
MANIFEST_SPEC.loader.exec_module(MANIFEST_MODULE)

HEAD = "1" * 40
VERSION = "route1-test-1-g1111111"
WORKSPACE = "/home/operator/lab/fprime/v0"
INSTALL_ROOT = "/home/operator/obc-deploy"
RELEASE_ID = "route1-test-1-g1111111"
OBC_SHA = "2" * 64
OBC_SIZE = 123
RADIO_SHA = "a" * 64
LAUNCH_SHA = "3" * 64
LAUNCH_SIZE = 456
TRACKED_CONTENT_SHA = "6" * 64
TRACKED_PATHS = [".gitignore", "tracked.txt"]
SERVICE_UNIT_SIZE = 789
SERVICE_UNIT_SHA = "7" * 64
BUILD_INPUT_SHA = {
    path: hashlib.sha256(path.encode("utf-8")).hexdigest()
    for path in MODULE.PACKAGED_REMOTE_BUILD_INPUTS
}
BUILD_INPUT_SHA["bin/OBC"] = OBC_SHA
BUILD_INPUT_SHA["bin/radio_mock_server"] = RADIO_SHA
CAMPAIGN_IDENTITY = {
    "schemaVersion": 1,
    "branch": "feature/route1-test",
    "head": HEAD,
    "projectVersion": VERSION,
    "recordedAtUtc": "2026-07-28T00:00:00+00:00",
}


def assert_packaged_remote_build_inputs_match_package_script() -> None:
    package_text = PACKAGE_SCRIPT.read_text(encoding="utf-8")
    copies = re.findall(
        r'cp "\$\{TMP_DIR\}/([^"]+)" "\$\{STAGE_DIR\}/([^"]+)"',
        package_text,
    )
    assert len(copies) == len(set(copies))
    package_inputs = {
        package_path: workspace_path
        for workspace_path, package_path in copies
    }
    assert package_inputs == MODULE.PACKAGED_REMOTE_BUILD_INPUTS


def valid_local() -> dict[str, object]:
    remote = valid_remote()
    return {
        "branch": "feature/route1-test",
        "head": HEAD,
        "projectVersion": VERSION,
        "status": "?? output/",
        "untrackedPaths": ["output/report.md"],
        "allowedUntrackedRoots": ["output"],
        "unexpectedUntrackedPaths": [],
        "trackedContent": {
            "format": MODULE.TRACKED_CONTENT_FORMAT,
            "algorithm": "sha256",
            "fileCount": len(TRACKED_PATHS),
            "digest": TRACKED_CONTENT_SHA,
        },
        "trustedInstalledManifest": {
            "path": "/trusted/manifest.json",
            "sha256": remote["installedManifestSha256"],
            "manifest": remote["installedManifest"],
        },
        "trustedServiceUnit": {
            "path": "/trusted/obc-comm-csp-stack.service",
            "sizeBytes": SERVICE_UNIT_SIZE,
            "sha256": SERVICE_UNIT_SHA,
        },
    }


def valid_remote() -> dict[str, object]:
    release = f"{INSTALL_ROOT}/releases/{RELEASE_ID}"
    service = MODULE.OBC_COMM_SERVICE
    fragment = f"/etc/systemd/system/{service}"
    dropins = {
        f"/etc/systemd/system/{service}.d/{name}": {
            "sizeBytes": len(content.encode("utf-8")),
            "sha256": hashlib.sha256(content.encode("utf-8")).hexdigest(),
        }
        for name, content in MODULE.EXPECTED_OBC_DROPINS.items()
    }
    launch = f"{INSTALL_ROOT}/current/launch/run_obc_comm_csp_stack.sh"
    return {
        "workspaceMarker": {
            "schemaVersion": 3,
            "branch": "feature/route1-test",
            "head": HEAD,
            "projectVersion": VERSION,
            "builtInputSha256": dict(BUILD_INPUT_SHA),
            "trackedContent": {
                "format": MODULE.TRACKED_CONTENT_FORMAT,
                "algorithm": "sha256",
                "fileCount": len(TRACKED_PATHS),
                "digest": TRACKED_CONTENT_SHA,
                "paths": TRACKED_PATHS,
            },
        },
        "workspaceTrackedContent": {
            "format": MODULE.TRACKED_CONTENT_FORMAT,
            "algorithm": "sha256",
            "fileCount": len(TRACKED_PATHS),
            "digest": TRACKED_CONTENT_SHA,
        },
        # A missing remote .git directory is valid because the governed sync
        # intentionally excludes it; the explicit marker is mandatory.
        "workspaceGitHead": None,
        "workspaceGitError": None,
        "remoteBuildMetadata": {"project_version": VERSION},
        "remoteBuiltObcPath": f"{WORKSPACE}/build-artifacts/Linux/OBC/bin/OBC",
        "remoteBuiltObcSha256": OBC_SHA,
        "remoteBuildInputs": {
            package_path: {
                "path": f"{WORKSPACE}/{workspace_path}",
                "sha256": BUILD_INPUT_SHA[package_path],
            }
            for package_path, workspace_path in (
                MODULE.PACKAGED_REMOTE_BUILD_INPUTS.items()
            )
        },
        "installedReleasePointer": release,
        "installedManifest": {
            "release_id": RELEASE_ID,
            "project_version": VERSION,
            "source_remote_dir": WORKSPACE,
            "files": {
                **{
                    package_path: {
                        "size_bytes": OBC_SIZE + index,
                        "sha256": BUILD_INPUT_SHA[package_path],
                    }
                    for index, package_path in enumerate(
                        MODULE.PACKAGED_REMOTE_BUILD_INPUTS
                    )
                },
                "launch/run_obc_comm_csp_stack.sh": {
                    "size_bytes": LAUNCH_SIZE,
                    "sha256": LAUNCH_SHA,
                },
            },
        },
        "installedManifestSha256": "9" * 64,
        "installedManifestFileAudit": {
            "format": "route1-installed-manifest-files-v1",
            "files": {
                **{
                    package_path: {
                        "sizeBytes": OBC_SIZE + index,
                        "sha256": BUILD_INPUT_SHA[package_path],
                    }
                    for index, package_path in enumerate(
                        MODULE.PACKAGED_REMOTE_BUILD_INPUTS
                    )
                },
                "launch/run_obc_comm_csp_stack.sh": {
                    "sizeBytes": LAUNCH_SIZE,
                    "sha256": LAUNCH_SHA,
                },
            },
        },
        "installedBuildMetadata": {"project_version": VERSION},
        "installedObcPath": f"{release}/bin/OBC",
        "installedObcSha256": OBC_SHA,
        "serviceLaunchAudit": {
            "service": service,
            "show": {
                "FragmentPath": fragment,
                "DropInPaths": " ".join(sorted(dropins)),
                "ExecStart": f"{{ path={launch} ; argv[]={launch} ; }}",
                "WorkingDirectory": f"{INSTALL_ROOT}/current",
                "MainPID": "100",
            },
            "dropInPaths": sorted(dropins),
            "files": {
                fragment: {
                    "sizeBytes": SERVICE_UNIT_SIZE,
                    "sha256": SERVICE_UNIT_SHA,
                },
                **dropins,
            },
            "processTree": [
                {
                    "pid": 100,
                    "argv": ["/bin/bash", launch],
                    "executable": "/usr/bin/bash",
                    "executableSha256": "8" * 64,
                },
                {
                    "pid": 101,
                    "argv": [f"{release}/bin/OBC", "--headless"],
                    "executable": f"{release}/bin/OBC",
                    "executableSha256": OBC_SHA,
                },
                {
                    "pid": 102,
                    "argv": [f"{release}/bin/radio_mock_server", "--port", "7000"],
                    "executable": f"{release}/bin/radio_mock_server",
                    "executableSha256": RADIO_SHA,
                },
            ],
        },
    }


def failures(local: dict[str, object], remote: dict[str, object]) -> list[str]:
    return MODULE.evaluate_provenance(local, remote, WORKSPACE, INSTALL_ROOT)


def valid_provenance() -> dict[str, object]:
    return {
        "schemaVersion": 1,
        "verdict": "PASS",
        "failures": [],
        "local": valid_local(),
        "remote": valid_remote(),
        "expected": {
            "remoteWorkspace": WORKSPACE,
            "installRoot": INSTALL_ROOT,
        },
    }


def check_committed_tree_binding_rejects_hidden_drift() -> None:
    for mode in ("assume-unchanged", "skip-worktree", "filemode"):
        with tempfile.TemporaryDirectory(
            prefix=f"route1-committed-tree-{mode}-test-"
        ) as temp_dir:
            repo = pathlib.Path(temp_dir)
            subprocess.run(["git", "init", "-q", str(repo)], check=True)
            subprocess.run(
                ["git", "-C", str(repo), "config", "user.email", "test@example.invalid"],
                check=True,
            )
            subprocess.run(
                ["git", "-C", str(repo), "config", "user.name", "Route1 Test"],
                check=True,
            )
            tracked = repo / "tracked.txt"
            tracked.write_text("committed\n", encoding="utf-8")
            subprocess.run(
                ["git", "-C", str(repo), "add", "tracked.txt"], check=True
            )
            subprocess.run(
                ["git", "-C", str(repo), "commit", "-q", "-m", "fixture"],
                check=True,
            )

            if mode == "assume-unchanged":
                subprocess.run(
                    [
                        "git",
                        "-C",
                        str(repo),
                        "update-index",
                        "--assume-unchanged",
                        "tracked.txt",
                    ],
                    check=True,
                )
                tracked.write_text("hidden mutation\n", encoding="utf-8")
            elif mode == "skip-worktree":
                subprocess.run(
                    [
                        "git",
                        "-C",
                        str(repo),
                        "update-index",
                        "--skip-worktree",
                        "tracked.txt",
                    ],
                    check=True,
                )
                tracked.write_text("hidden mutation\n", encoding="utf-8")
            else:
                subprocess.run(
                    ["git", "-C", str(repo), "config", "core.filemode", "false"],
                    check=True,
                )
                tracked.chmod(0o755)

            status = subprocess.run(
                ["git", "-C", str(repo), "status", "--short"],
                check=True,
                capture_output=True,
                text=True,
            ).stdout.strip()
            assert status == "", f"{mode} fixture unexpectedly appears dirty: {status}"
            try:
                MODULE.tracked_workspace_binding(repo)
            except RuntimeError as exc:
                assert "committed HEAD tree" in str(exc)
            else:
                raise AssertionError(
                    f"{mode} drift must not be attributed to committed HEAD"
                )

    with tempfile.TemporaryDirectory(
        prefix="route1-committed-tree-staged-index-test-"
    ) as temp_dir:
        repo = pathlib.Path(temp_dir)
        subprocess.run(["git", "init", "-q", str(repo)], check=True)
        subprocess.run(
            ["git", "-C", str(repo), "config", "user.email", "test@example.invalid"],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(repo), "config", "user.name", "Route1 Test"],
            check=True,
        )
        tracked = repo / "tracked.txt"
        tracked.write_text("committed\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(repo), "add", "tracked.txt"], check=True)
        subprocess.run(
            ["git", "-C", str(repo), "commit", "-q", "-m", "fixture"],
            check=True,
        )
        tracked.write_text("staged mutation\n", encoding="utf-8")
        subprocess.run(["git", "-C", str(repo), "add", "tracked.txt"], check=True)
        try:
            MODULE.tracked_workspace_binding(repo)
        except RuntimeError as exc:
            assert "index does not match the committed HEAD tree" in str(exc)
        else:
            raise AssertionError(
                "staged index drift must not be attributed to committed HEAD"
            )


def main() -> int:
    assert_packaged_remote_build_inputs_match_package_script()

    check_committed_tree_binding_rejects_hidden_drift()

    assert failures(valid_local(), valid_remote()) == []

    remote = valid_remote()
    remote["workspaceGitHead"] = "3" * 40
    assert "remote-workspace-git-head-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["workspaceMarker"] = {"_readError": "missing"}
    assert "remote-workspace-marker-missing" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["workspaceMarker"]["schemaVersion"] = 1
    assert "remote-workspace-marker-schema-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    del remote["workspaceMarker"]["builtInputSha256"]
    assert "remote-workspace-marker-build-inputs-invalid" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    del remote["workspaceMarker"]["builtInputSha256"]["bin/radio_mock_server"]
    assert "remote-workspace-marker-build-inputs-invalid" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    del remote["remoteBuildInputs"]["bin/radio_mock_server"]
    assert "remote-build-input-audit-invalid" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["workspaceMarker"]["trackedContent"]["digest"] = "7" * 64
    assert "remote-workspace-content-marker-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["workspaceMarker"]["trackedContent"]["paths"].pop()
    assert "remote-workspace-content-manifest-invalid" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["workspaceTrackedContent"]["digest"] = "7" * 64
    assert "remote-workspace-content-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["workspaceTrackedContent"] = {"_readError": "tracked file missing"}
    assert "remote-workspace-content-unreadable" in failures(valid_local(), remote)

    local = valid_local()
    del local["trackedContent"]
    assert "local-tracked-content-audit-missing" in failures(local, valid_remote())

    remote = valid_remote()
    remote["remoteBuildMetadata"] = {"project_version": "stale"}
    assert "remote-build-version-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["installedReleasePointer"] = f"{INSTALL_ROOT}/releases/stale"
    assert "installed-release-pointer-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["installedObcSha256"] = "4" * 64
    assert "installed-obc-hash-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["remoteBuildInputs"]["bin/radio_mock_server"]["sha256"] = "5" * 64
    assert "remote-build-input-marker-hash-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["remoteBuildInputs"]["bin/OBC"]["sha256"] = "5" * 64
    remote["installedObcSha256"] = "5" * 64
    remote["installedManifest"]["files"]["bin/OBC"]["sha256"] = "5" * 64
    remote["installedManifestFileAudit"]["files"]["bin/OBC"]["sha256"] = "5" * 64
    remote["serviceLaunchAudit"]["processTree"][1]["executableSha256"] = "5" * 64
    replacement_local = valid_local()
    replacement_local["trustedInstalledManifest"]["manifest"] = remote[
        "installedManifest"
    ]
    replacement_local["trustedInstalledManifest"]["sha256"] = remote[
        "installedManifestSha256"
    ]
    assert failures(replacement_local, remote) == [
        "remote-build-input-marker-hash-mismatch",
        "packaged-build-input-hash-mismatch",
    ]

    remote = valid_remote()
    remote["remoteBuildInputs"]["bin/radio_mock_server"]["sha256"] = "5" * 64
    remote["installedManifest"]["files"]["bin/radio_mock_server"][
        "sha256"
    ] = "5" * 64
    remote["installedManifestFileAudit"]["files"]["bin/radio_mock_server"][
        "sha256"
    ] = "5" * 64
    remote["serviceLaunchAudit"]["processTree"][2][
        "executableSha256"
    ] = "5" * 64
    replacement_local = valid_local()
    replacement_local["trustedInstalledManifest"]["manifest"] = remote[
        "installedManifest"
    ]
    replacement_local["trustedInstalledManifest"]["sha256"] = remote[
        "installedManifestSha256"
    ]
    coordinated_failures = failures(replacement_local, remote)
    assert "remote-build-input-marker-hash-mismatch" in coordinated_failures
    assert "packaged-build-input-hash-mismatch" in coordinated_failures

    remote = valid_remote()
    remote["serviceLaunchAudit"]["processTree"][2][
        "executableSha256"
    ] = "5" * 64
    assert "service-process-radio-hash-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["installedManifestFileAudit"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ]["sha256"] = "4" * 64
    assert "installed-manifest-file-hash-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    del remote["installedManifestFileAudit"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ]
    assert "installed-manifest-file-set-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["installedManifestFileAudit"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ]["sizeBytes"] += 1
    assert "installed-manifest-file-size-mismatch" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["installedManifestFileAudit"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ] = {"_readError": "missing"}
    assert "installed-manifest-file-audit-unreadable" in failures(
        valid_local(), remote
    )

    remote = valid_remote()
    remote["installedManifest"]["files"]["../escaped"] = {
        "size_bytes": 1,
        "sha256": "4" * 64,
    }
    remote["installedManifestFileAudit"]["files"]["../escaped"] = {
        "sizeBytes": 1,
        "sha256": "4" * 64,
    }
    assert "installed-manifest-files-invalid" in failures(valid_local(), remote)

    remote = valid_remote()
    del remote["installedManifestSha256"]
    assert "installed-manifest-hash-missing" in failures(valid_local(), remote)

    local = valid_local()
    del local["trustedInstalledManifest"]
    assert "trusted-installed-manifest-missing" in failures(
        local, valid_remote()
    )

    local = valid_local()
    del local["trustedServiceUnit"]
    assert "trusted-service-unit-missing" in failures(local, valid_remote())

    remote = valid_remote()
    service_fragment = f"/etc/systemd/system/{MODULE.OBC_COMM_SERVICE}"
    remote["serviceLaunchAudit"]["files"][service_fragment]["sha256"] = "4" * 64
    assert "service-unit-receipt-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    unknown_dropin = (
        f"/etc/systemd/system/{MODULE.OBC_COMM_SERVICE}.d/"
        "99-unreviewed.conf"
    )
    remote["serviceLaunchAudit"]["dropInPaths"].append(unknown_dropin)
    remote["serviceLaunchAudit"]["dropInPaths"].sort()
    remote["serviceLaunchAudit"]["files"][unknown_dropin] = {
        "sizeBytes": 10,
        "sha256": "4" * 64,
    }
    service_failures = failures(valid_local(), remote)
    assert "service-dropin-set-mismatch" in service_failures
    assert "service-launch-file-set-mismatch" in service_failures

    remote = valid_remote()
    known_dropin = remote["serviceLaunchAudit"]["dropInPaths"][0]
    remote["serviceLaunchAudit"]["files"][known_dropin]["sha256"] = "4" * 64
    assert "service-dropin-content-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["serviceLaunchAudit"]["show"]["ExecStart"] = (
        "{ path=/tmp/unreviewed ; argv[]=/tmp/unreviewed ; }"
    )
    assert "service-exec-start-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["serviceLaunchAudit"]["processTree"][1]["executable"] = (
        "/tmp/unreviewed-obc"
    )
    process_failures = failures(valid_local(), remote)
    assert "service-process-executable-unexpected" in process_failures
    assert "service-obc-process-mismatch" in process_failures

    remote = valid_remote()
    remote["serviceLaunchAudit"]["processTree"].pop(2)
    assert "service-radio-process-mismatch" in failures(valid_local(), remote)

    remote = valid_remote()
    remote["installedManifest"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ] = {
        "size_bytes": LAUNCH_SIZE + 10,
        "sha256": "4" * 64,
    }
    remote["installedManifestSha256"] = "8" * 64
    remote["installedManifestFileAudit"]["files"][
        "launch/run_obc_comm_csp_stack.sh"
    ] = {
        "sizeBytes": LAUNCH_SIZE + 10,
        "sha256": "4" * 64,
    }
    coordinated_failures = failures(valid_local(), remote)
    assert "installed-manifest-trusted-hash-mismatch" in coordinated_failures
    assert "installed-manifest-trusted-content-mismatch" in coordinated_failures

    assert failures(valid_local(), {"captureError": "ssh failed"}) == [
        "remote-capture-failed"
    ]

    local = valid_local()
    local["projectVersion"] = VERSION + "-dirty"
    assert "local-tracked-worktree-dirty" in failures(local, valid_remote())

    local = valid_local()
    local["unexpectedUntrackedPaths"] = ["OBC/unreviewed.cpp"]
    assert "local-untracked-inputs-present" in failures(local, valid_remote())

    with tempfile.TemporaryDirectory(prefix="route1-local-provenance-test-") as temp_dir:
        repo = pathlib.Path(temp_dir)
        subprocess.run(["git", "init", "-q", str(repo)], check=True)
        subprocess.run(
            ["git", "-C", str(repo), "config", "user.email", "test@example.invalid"],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(repo), "config", "user.name", "Route1 Test"],
            check=True,
        )
        (repo / "tracked.txt").write_text("tracked\n", encoding="utf-8")
        (repo / ".gitignore").write_text("ignored.cpp\n", encoding="utf-8")
        (repo / "tool.sh").write_text("#!/bin/sh\nexit 0\n", encoding="utf-8")
        (repo / "tool.sh").chmod(0o755)
        (repo / "tracked-link").symlink_to("tracked.txt")
        subprocess.run(
            [
                "git",
                "-C",
                str(repo),
                "add",
                "tracked.txt",
                ".gitignore",
                "tool.sh",
                "tracked-link",
            ],
            check=True,
        )
        subprocess.run(
            ["git", "-C", str(repo), "commit", "-q", "-m", "fixture"], check=True
        )
        (repo / "output").mkdir()
        (repo / "output/local.txt").write_text("local-only\n", encoding="utf-8")
        (repo / "ignored.cpp").write_text("int ignored;\n", encoding="utf-8")
        campaign_identity_path = repo / "output/campaign-source-identity.json"
        recorded_identity = MODULE.write_campaign_identity(
            repo, campaign_identity_path, [str(repo / "output")]
        )
        assert recorded_identity["head"] == subprocess.run(
            ["git", "-C", str(repo), "rev-parse", "HEAD"],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.strip()
        assert MODULE.check_campaign_identity(
            repo, campaign_identity_path, [str(repo / "output")]
        ) == recorded_identity
        campaign_manifest_path = repo / "output/campaign-manifest.json"
        campaign_manifest_path.write_text(
            json.dumps(
                {
                    "campaignSourceIdentity": {
                        "path": "deployment/campaign-source-identity.json",
                        "branch": recorded_identity["branch"],
                        "head": recorded_identity["head"],
                        "projectVersion": recorded_identity["projectVersion"],
                    }
                }
            )
            + "\n",
            encoding="utf-8",
        )
        assert MODULE.check_campaign_identity(
            repo,
            campaign_identity_path,
            [str(repo / "output")],
            campaign_manifest_path,
        ) == recorded_identity
        mismatched_campaign = json.loads(
            campaign_manifest_path.read_text(encoding="utf-8")
        )
        mismatched_campaign["campaignSourceIdentity"]["head"] = "f" * 40
        campaign_manifest_path.write_text(
            json.dumps(mismatched_campaign) + "\n", encoding="utf-8"
        )
        try:
            MODULE.check_campaign_identity(
                repo,
                campaign_identity_path,
                [str(repo / "output")],
                campaign_manifest_path,
            )
        except RuntimeError as exc:
            assert "retained campaign manifest: head" in str(exc)
        else:
            raise AssertionError(
                "mismatched retained campaign identity was accepted"
            )

        archive = repo / "output/tracked-only.tar.gz"
        tracked = subprocess.run(
            ["git", "-C", str(repo), "ls-files", "--recurse-submodules", "-z"],
            check=True,
            capture_output=True,
        ).stdout
        subprocess.run(
            [
                "tar",
                "-C",
                str(repo),
                "--null",
                "--files-from=-",
                "-czf",
                str(archive),
            ],
            input=tracked,
            check=True,
        )
        archive_members = subprocess.run(
            ["tar", "-tzf", str(archive)],
            check=True,
            capture_output=True,
            text=True,
        ).stdout.splitlines()
        assert "tracked.txt" in archive_members
        assert ".gitignore" in archive_members
        assert "tool.sh" in archive_members
        assert "tracked-link" in archive_members
        assert "ignored.cpp" not in archive_members
        assert "output/local.txt" not in archive_members

        tracked_binding = MODULE.tracked_workspace_binding(repo)
        assert tracked_binding["paths"] == [
            ".gitignore",
            "tool.sh",
            "tracked-link",
            "tracked.txt",
        ]
        assert tracked_binding["fileCount"] == 4
        assert len(tracked_binding["digest"]) == 64
        assert MODULE.local_identity(
            repo, [str(repo / "output")]
        )["trackedContent"] == MODULE.tracked_content_without_paths(
            tracked_binding
        )

        marker_write: dict[str, object] = {}
        original_run = MODULE.run

        def capture_marker_write(
            command: list[str],
            *,
            input_text: str | None = None,
            timeout: int = 60,
        ) -> subprocess.CompletedProcess[str]:
            if command[0] == "ssh":
                marker_write["command"] = command
                marker_write["inputText"] = input_text
                marker_write["timeout"] = timeout
                response = json.loads(input_text or "{}")
                response["builtInputSha256"] = BUILD_INPUT_SHA
                return subprocess.CompletedProcess(
                    command,
                    0,
                    json.dumps(response) + "\n",
                    "",
                )
            return original_run(
                command,
                input_text=input_text,
                timeout=timeout,
            )

        MODULE.run = capture_marker_write
        try:
            written_marker = MODULE.write_workspace_marker(
                repo,
                "test-target",
                WORKSPACE,
                60,
                [str(repo / "output")],
            )
        finally:
            MODULE.run = original_run
        assert written_marker["schemaVersion"] == 3
        assert written_marker["builtInputSha256"] == BUILD_INPUT_SHA
        assert written_marker["trackedContent"] == tracked_binding
        submitted_marker = json.loads(str(marker_write["inputText"]))
        assert "builtInputSha256" not in submitted_marker
        assert submitted_marker["trackedContent"] == tracked_binding
        assert tracked_binding["digest"] not in " ".join(
            marker_write["command"]
        )

        remote_workspace = repo / "output/remote-workspace"
        remote_install = repo / "output/remote-install"
        remote_workspace.mkdir()
        remote_install.mkdir()
        for relative_path in tracked_binding["paths"]:
            source = repo / relative_path
            destination = remote_workspace / relative_path
            destination.parent.mkdir(parents=True, exist_ok=True)
            if source.is_symlink():
                destination.symlink_to(os.readlink(source))
            else:
                shutil.copy2(source, destination)
        remote_built_obc = (
            remote_workspace / "build-artifacts/Linux/OBC/bin/OBC"
        )
        remote_built_obc.parent.mkdir(parents=True)
        remote_built_obc.write_bytes(b"remote-obc\n")
        remote_marker_path = (
            remote_workspace / ".route1-source-provenance.json"
        )
        remote_marker_result = subprocess.run(
            [
                "python3",
                "-c",
                MODULE.REMOTE_MARKER_SCRIPT,
                str(remote_marker_path),
                json.dumps({"bin/OBC": str(remote_built_obc)}),
            ],
            input=json.dumps(
                {
                    "schemaVersion": 3,
                    "trackedContent": tracked_binding,
                }
            ),
            check=True,
            capture_output=True,
            text=True,
        )
        remote_marker = json.loads(remote_marker_result.stdout)
        assert remote_marker["builtInputSha256"] == {
            "bin/OBC": hashlib.sha256(b"remote-obc\n").hexdigest()
        }
        assert json.loads(
            remote_marker_path.read_text(encoding="utf-8")
        ) == remote_marker

        release_root = remote_install / "releases/test-release"
        installed_obc = release_root / "bin/OBC"
        installed_launch = release_root / "launch/run_obc_comm_csp_stack.sh"
        installed_version = release_root / "meta/version.json"
        installed_obc.parent.mkdir(parents=True)
        installed_launch.parent.mkdir(parents=True)
        installed_version.parent.mkdir(parents=True)
        installed_obc.write_bytes(b"remote-obc\n")
        installed_launch.write_text("#!/bin/sh\nexec ./bin/OBC\n", encoding="utf-8")
        installed_version.write_text(
            json.dumps({"project_version": VERSION}) + "\n",
            encoding="utf-8",
        )
        installed_manifest = {
            "release_id": "test-release",
            "project_version": VERSION,
            "source_remote_dir": str(remote_workspace),
            "files": {
                "bin/OBC": {
                    "size_bytes": installed_obc.stat().st_size,
                    "sha256": hashlib.sha256(installed_obc.read_bytes()).hexdigest(),
                },
                "launch/run_obc_comm_csp_stack.sh": {
                    "size_bytes": installed_launch.stat().st_size,
                    "sha256": hashlib.sha256(
                        installed_launch.read_bytes()
                    ).hexdigest(),
                },
                "meta/version.json": {
                    "size_bytes": installed_version.stat().st_size,
                    "sha256": hashlib.sha256(
                        installed_version.read_bytes()
                    ).hexdigest(),
                },
            },
        }
        (release_root / "manifest.json").write_text(
            json.dumps(installed_manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        (remote_install / "current").symlink_to(release_root)

        def capture_workspace() -> dict[str, object]:
            result = subprocess.run(
                [
                    "python3",
                    "-",
                    str(remote_workspace),
                    str(remote_install),
                    MODULE.OBC_COMM_SERVICE,
                    json.dumps(MODULE.PACKAGED_REMOTE_BUILD_INPUTS),
                ],
                input=MODULE.REMOTE_CAPTURE_SCRIPT,
                check=True,
                capture_output=True,
                text=True,
            )
            return json.loads(result.stdout)

        captured = capture_workspace()
        assert captured["workspaceTrackedContent"] == (
            MODULE.tracked_content_without_paths(tracked_binding)
        )
        assert captured["installedManifestFileAudit"]["files"][
            "launch/run_obc_comm_csp_stack.sh"
        ]["sha256"] == installed_manifest["files"][
            "launch/run_obc_comm_csp_stack.sh"
        ]["sha256"]
        assert MODULE.valid_sha256(captured["installedManifestSha256"])

        installed_launch.write_text(
            "#!/bin/sh\nexec ./bin/OBC --unreviewed\n", encoding="utf-8"
        )
        captured = capture_workspace()
        assert captured["installedManifestFileAudit"]["files"][
            "launch/run_obc_comm_csp_stack.sh"
        ]["sha256"] != installed_manifest["files"][
            "launch/run_obc_comm_csp_stack.sh"
        ]["sha256"]
        installed_launch.write_text("#!/bin/sh\nexec ./bin/OBC\n", encoding="utf-8")

        (remote_workspace / "tracked.txt").write_text(
            "mutated remote source\n", encoding="utf-8"
        )
        captured = capture_workspace()
        assert captured["workspaceTrackedContent"]["digest"] != (
            tracked_binding["digest"]
        )

        (remote_workspace / ".gitignore").unlink()
        captured = capture_workspace()
        assert "_readError" in captured["workspaceTrackedContent"]

        mismatched_identity = dict(recorded_identity)
        mismatched_identity["head"] = "0" * 40
        assert MODULE.campaign_identity_mismatches(
            mismatched_identity, MODULE.local_identity(repo, [str(repo / "output")])
        ) == ["head"]
        campaign_identity_path.write_text(
            json.dumps(mismatched_identity) + "\n",
            encoding="utf-8",
        )
        try:
            MODULE.check_campaign_identity(
                repo, campaign_identity_path, [str(repo / "output")]
            )
        except RuntimeError as exc:
            assert "does not match" in str(exc)
        else:
            raise AssertionError("cross-revision campaign resume must be rejected")

        (repo / "unreviewed.cpp").write_text("int unreviewed;\n", encoding="utf-8")
        identity = MODULE.local_identity(repo, [str(repo / "output")])
        assert "unreviewed.cpp" in identity["untrackedPaths"]
        assert "ignored.cpp" not in identity["untrackedPaths"]
        assert identity["unexpectedUntrackedPaths"] == ["unreviewed.cpp"]
        try:
            MODULE.require_markable_local_identity(identity)
        except RuntimeError as exc:
            assert "untracked inputs" in str(exc)
        else:
            raise AssertionError("unexpected untracked source must block marking")

    provenance_payload = valid_provenance()
    provenance_text = json.dumps(provenance_payload, sort_keys=True) + "\n"
    provenance_sha256 = hashlib.sha256(
        provenance_text.encode("utf-8")
    ).hexdigest()
    valid_attempt_evidence = {
        "verdict": "PASS",
        "manifestPath": "route1/hosted/attempt-01/manifest.json",
        "manifestSha256": "a" * 64,
        "failures": [],
    }
    attempts = [
        {
            "surface": "hosted",
            "attempt": "attempt-01",
            "verdict": "PASS",
            "attemptEvidence": valid_attempt_evidence,
        },
        {
            "surface": "target",
            "attempt": "attempt-01",
            "verdict": "PASS",
            "targetRevisionProvenanceSha256": provenance_sha256,
            "attemptEvidence": {
                **valid_attempt_evidence,
                "manifestPath": "route1/target/attempt-01/manifest.json",
            },
        },
    ]
    campaign = MANIFEST_MODULE.build_campaign_manifest(
        attempts,
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        None,
        None,
        "2026-07-28T00:00:00+00:00",
    )
    assert campaign["authoritativeVerdict"] == "FAIL"
    assert campaign["attempts"][0]["authoritative"] is True
    assert campaign["attempts"][1]["authoritative"] is False

    campaign = MANIFEST_MODULE.build_campaign_manifest(
        attempts,
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "FAIL", "failures": ["remote-build-version-mismatch"]},
        provenance_sha256,
        "2026-07-28T00:00:00+00:00",
    )
    assert campaign["authoritativeVerdict"] == "FAIL"
    assert campaign["targetRevisionProvenance"]["failures"] == [
        "remote-build-version-mismatch"
    ]

    campaign = MANIFEST_MODULE.build_campaign_manifest(
        attempts,
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "PASS", "failures": []},
        provenance_sha256,
        "2026-07-28T00:00:00+00:00",
    )
    assert campaign["authoritativeVerdict"] == "PASS"
    assert all(record["authoritative"] for record in campaign["attempts"])
    assert campaign["campaignSourceIdentity"]["head"] == HEAD

    invalid_attempt_campaign = MANIFEST_MODULE.build_campaign_manifest(
        [
            {
                **attempts[0],
                "attemptEvidence": {
                    **valid_attempt_evidence,
                    "verdict": "FAIL",
                    "failures": ["attempt-artifact-hash-mismatch"],
                },
            },
            attempts[1],
        ],
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "PASS", "failures": []},
        provenance_sha256,
        "2026-07-28T00:00:00+00:00",
    )
    assert invalid_attempt_campaign["authoritativeVerdict"] == "FAIL"
    assert invalid_attempt_campaign["attempts"][0]["authoritative"] is False

    invalid_history_campaign = MANIFEST_MODULE.build_campaign_manifest(
        [
            {
                "surface": "hosted",
                "attempt": "attempt-00",
                "verdict": "FAIL",
                "attemptEvidence": {
                    **valid_attempt_evidence,
                    "verdict": "FAIL",
                    "failures": ["attempt-artifact-file-set-mismatch"],
                },
            },
            *attempts,
        ],
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "PASS", "failures": []},
        provenance_sha256,
        "2026-07-28T00:00:00+00:00",
    )
    assert invalid_history_campaign["attempts"][-2]["authoritative"] is True
    assert invalid_history_campaign["authoritativeVerdict"] == "FAIL"

    unbound_attempts = json.loads(json.dumps(attempts))
    del unbound_attempts[1]["targetRevisionProvenanceSha256"]
    unbound_campaign = MANIFEST_MODULE.build_campaign_manifest(
        unbound_attempts,
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "PASS", "failures": []},
        provenance_sha256,
        "2026-07-28T00:00:00+00:00",
    )
    assert unbound_campaign["authoritativeVerdict"] == "FAIL"
    assert unbound_campaign["attempts"][1]["authoritative"] is False

    mismatched_binding_campaign = MANIFEST_MODULE.build_campaign_manifest(
        attempts,
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        {"verdict": "PASS", "failures": []},
        "f" * 64,
        "2026-07-28T00:00:00+00:00",
    )
    assert mismatched_binding_campaign["authoritativeVerdict"] == "FAIL"
    assert mismatched_binding_campaign["attempts"][1]["authoritative"] is False

    assert (
        MODULE.retained_target_resume_state(
            campaign, provenance_payload, provenance_sha256
        )
        == "RETAINED_AUTHORITATIVE_PASS"
    )

    pending_campaign = MANIFEST_MODULE.build_campaign_manifest(
        [
            {
                "surface": "hosted",
                "attempt": "attempt-01",
                "verdict": "PASS",
                "attemptEvidence": valid_attempt_evidence,
            },
            {
                "surface": "target",
                "attempt": "attempt-01",
                "verdict": "FAIL",
                "attemptEvidence": {
                    **valid_attempt_evidence,
                    "manifestPath": "route1/target/attempt-01/manifest.json",
                },
            },
        ],
        "2026-07-28",
        CAMPAIGN_IDENTITY,
        None,
        None,
        "2026-07-28T00:00:00+00:00",
    )
    assert (
        MODULE.retained_target_resume_state(pending_campaign, {}, "")
        == "PENDING"
    )

    rebound_campaign = json.loads(json.dumps(campaign))
    rebound_campaign["attempts"][1]["authoritative"] = False
    try:
        MODULE.retained_target_resume_state(
            rebound_campaign, provenance_payload, provenance_sha256
        )
    except RuntimeError as exc:
        assert "not authoritative" in str(exc)
    else:
        raise AssertionError("non-authoritative target PASS must fail closed")

    try:
        MODULE.retained_target_resume_state(
            campaign, provenance_payload, "f" * 64
        )
    except RuntimeError as exc:
        assert "hash is missing or mismatched" in str(exc)
    else:
        raise AssertionError("retained target provenance hash mismatch must fail")

    changed_binary_provenance = valid_provenance()
    changed_binary_provenance["remote"]["installedObcSha256"] = "4" * 64
    try:
        MODULE.retained_target_resume_state(
            campaign, changed_binary_provenance, provenance_sha256
        )
    except RuntimeError as exc:
        assert "no longer validates" in str(exc)
    else:
        raise AssertionError("corrupt retained target provenance must fail closed")

    changed_source_provenance = valid_provenance()
    changed_source_provenance["remote"]["workspaceTrackedContent"]["digest"] = (
        "7" * 64
    )
    try:
        MODULE.retained_target_resume_state(
            campaign, changed_source_provenance, provenance_sha256
        )
    except RuntimeError as exc:
        assert "no longer validates" in str(exc)
    else:
        raise AssertionError(
            "mutated remote workspace content must fail retained provenance"
        )

    with tempfile.TemporaryDirectory(
        prefix="route1-target-resume-state-test-"
    ) as temp_dir:
        temp_root = pathlib.Path(temp_dir)
        manifest_path = temp_root / "campaign-manifest.json"
        provenance_path = temp_root / "target-revision-provenance.json"
        manifest_path.write_text(
            json.dumps(campaign) + "\n", encoding="utf-8"
        )
        provenance_path.write_text(
            provenance_text, encoding="utf-8"
        )
        result = subprocess.run(
            [
                "python3",
                str(CHECKER),
                "target-resume-state",
                "--manifest",
                str(manifest_path),
                "--provenance",
                str(provenance_path),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        assert result.stdout.strip() == "RETAINED_AUTHORITATIVE_PASS"

        manifest_path.write_text(
            json.dumps(pending_campaign) + "\n", encoding="utf-8"
        )
        provenance_path.unlink()
        result = subprocess.run(
            [
                "python3",
                str(CHECKER),
                "target-resume-state",
                "--manifest",
                str(manifest_path),
                "--provenance",
                str(provenance_path),
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        assert result.stdout.strip() == "PENDING"

    runner_text = FORMAL_RUNNER.read_text(encoding="utf-8")
    submodule_update_position = runner_text.index(
        "run_logged submodule-update"
    )
    fresh_identity_position = runner_text.index(
        '"${PROVENANCE_CHECKER}" write-campaign-identity'
    )
    resume_identity_position = runner_text.index(
        "check-campaign-identity"
    )
    post_hosted_identity_position = runner_text.index(
        "run_logged campaign-identity-post-hosted"
    )
    target_binding_position = runner_text.index(
        "Cannot bind target attempt without"
    )
    importer_position = runner_text.index(
        '"${ROOT_DIR}/fprime-venv/bin/python3" "${IMPORTER}"'
    )
    retained_attempt_position = runner_text.index(
        'ATTEMPT_RECORDS="$("${ROOT_DIR}/fprime-venv/bin/python3"'
    )
    local_gate_position = runner_text.index(
        "run_logged local-target-sync-provenance"
    )
    sync_position = runner_text.index(
        "run_logged target-workspace-bootstrap"
    )
    gate_position = runner_text.index(
        'if ! run_logged "target-revision-provenance-${attempt_label}"'
    )
    fresh_preflight_position = runner_text.index(
        "run_logged target-baseline-preflight"
    )
    ground_preflight_position = runner_text.index(
        "run_logged ground-baseline-preflight"
    )
    forced_restart_position = runner_text.index(
        "TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1"
    )
    resume_state_position = runner_text.index(
        "target-resume-state"
    )
    retained_artifact_gate_position = runner_text.index(
        '"${PYTHON_BIN}" "${ATTEMPT_CHECKER}"'
    )
    retained_skip_position = runner_text.index(
        "already has authoritative"
    )
    hosted_position = runner_text.index(
        'run_pending_surface hosted "${HOSTED_WRAPPER}"'
    )
    target_position = runner_text.index(
        'run_pending_surface target "${TARGET_WRAPPER}"'
    )
    prepare_function_position = runner_text.index(
        "prepare_target_attempt()"
    )
    prepare_hosted_function_position = runner_text.index(
        "prepare_hosted_attempt()"
    )
    run_surface_function_position = runner_text.index(
        "run_surface_attempt()"
    )
    hosted_attempt_guard_position = runner_text.index(
        'if [[ "${surface}" == "hosted" ]]',
        run_surface_function_position,
    )
    prepare_hosted_call_position = runner_text.index(
        'if ! prepare_hosted_attempt "${attempt_number}"; then',
        hosted_attempt_guard_position,
    )
    target_attempt_guard_position = runner_text.index(
        'elif [[ "${surface}" == "target" ]]',
        prepare_hosted_call_position,
    )
    prepare_call_position = runner_text.index(
        'if ! prepare_target_attempt "${attempt_number}"; then',
        target_attempt_guard_position,
    )
    wrapper_root_position = runner_text.index(
        "local wrapper_root",
        prepare_call_position,
    )
    wrapper_start_position = runner_text.index(
        'PROBE_ROOT="${wrapper_root}" bash "${wrapper}"',
        wrapper_root_position,
    )
    automatic_retry_position = runner_text.index(
        'run_surface_attempt "${surface}" 2 "attempt-01" "${wrapper}"',
        wrapper_start_position,
    )
    attempt_identity_position = runner_text.index(
        'if ! run_logged "campaign-identity-target-${attempt_label}"',
        prepare_function_position,
    )
    prepare_target_text = runner_text[
        prepare_function_position:prepare_hosted_function_position
    ]
    prepare_hosted_text = runner_text[
        prepare_hosted_function_position:run_surface_function_position
    ]
    assert submodule_update_position < fresh_identity_position
    assert local_gate_position < sync_position
    assert resume_identity_position < retained_attempt_position
    assert target_binding_position < importer_position
    assert (
        prepare_function_position
        < attempt_identity_position
        < forced_restart_position
        < gate_position
        < prepare_hosted_function_position
        < run_surface_function_position
    )
    assert (
        fresh_preflight_position
        < ground_preflight_position
        < hosted_position
    )
    assert (
        hosted_position
        < post_hosted_identity_position
        < target_position
    )
    assert hosted_position < resume_state_position < target_position
    assert retained_artifact_gate_position < retained_skip_position
    assert (
        run_surface_function_position
        < hosted_attempt_guard_position
        < prepare_hosted_call_position
        < target_attempt_guard_position
        < prepare_call_position
        < wrapper_root_position
        < wrapper_start_position
        < automatic_retry_position
    )
    assert "Every target attempt that reaches" in runner_text
    assert "target-revision-provenance.json" in runner_text
    assert "RPI_SYNC_EXTRA_EXCLUDE_PATH" in runner_text
    assert "RPI_SYNC_TRACKED_ONLY=1" in runner_text
    assert "RPI_SYNC_REPLACE_REMOTE=1" in runner_text
    assert "campaign-source-identity.json" in runner_text
    assert runner_text.count("check-campaign-identity") == 4
    assert (
        '--identity "${CAMPAIGN_IDENTITY_PATH}" \\\n'
        '    --campaign-manifest "${EVIDENCE_ROOT}/campaign-manifest.json"'
        in runner_text
    )
    assert (
        runner_text.index("--campaign-manifest")
        < runner_text.index('ATTEMPT_RECORDS="$(')
    )
    assert runner_text.count("TARGET_BASELINE_FORCE_OBC_COMM_RESTART=1") == 1
    assert prepare_target_text.count("return 1") == 3
    assert prepare_hosted_text.count("return 1") == 2
    assert (
        'if ! run_logged "target-baseline-pre-${attempt_label}"'
        in runner_text
    )
    assert (
        'run_logged "target-revision-provenance-${attempt_label}"'
        in runner_text
    )
    assert 'if ! prepare_target_attempt "${attempt_number}"; then' in runner_text
    assert (
        'if ! run_logged "campaign-identity-hosted-${attempt_label}"'
        in runner_text
    )
    assert (
        'if ! run_logged "native-build-hosted-${attempt_label}"'
        in runner_text
    )
    assert '--target "${HOSTED_BUILD_TARGETS[@]}" -- -j4' in runner_text
    for target in (
        "OBC",
        "payload_camera_backend_helper",
        "csp_zmqproxy",
        "eps_simulator",
        "adcs_simulator",
        "radio_mock_server",
        "pty_pair_bridge",
        "sband_comm_csp_node",
        "uhf_comm_csp_node",
        "ground_ttc_gateway",
    ):
        assert f"  {target}\n" in runner_text
    assert "run_logged native-build-target-deploy" in runner_text
    assert runner_text.count("run_logged submodule-update") == 1
    assert (
        'if [[ "${SKIP_DEPLOY}" != "1" && "${RESUME}" != "1" ]]; then'
        in runner_text[:fresh_identity_position]
    )
    assert 'if ! prepare_hosted_attempt "${attempt_number}"; then' in runner_text
    assert "RETAINED_AUTHORITATIVE_PASS" in runner_text
    assert "preserving retained target PASS provenance" in runner_text
    assert "targetRevisionProvenanceSha256" in runner_text
    assert "targetRevisionProvenancePath" in runner_text
    assert "target_provenance_attempt_path()" in runner_text
    assert "target-revision-provenance-%s.json" in runner_text
    assert (
        'cp "${target_provenance_attempt_path_value}" '
        '"${TARGET_PROVENANCE_PATH}"'
        in runner_text
    )
    assert "attemptManifestSha256" in runner_text
    assert 'manifest.get("authoritativeVerdict") != "PASS"' in runner_text
    assert "TRUSTED_INSTALLED_MANIFEST_PATH" in runner_text
    assert "TRUSTED_SERVICE_UNIT_PATH" in runner_text
    assert (
        'CANONICAL_OBC_COMM_CSP_SERVICE_NAME='
        '"obc-comm-csp-stack.service"'
    ) in runner_text
    assert (
        '"${OBC_COMM_CSP_SERVICE_NAME}" != '
        '"${CANONICAL_OBC_COMM_CSP_SERVICE_NAME}"'
    ) in runner_text
    assert (
        'export OBC_COMM_CSP_SERVICE_NAME='
        '"${CANONICAL_OBC_COMM_CSP_SERVICE_NAME}"'
    ) in runner_text
    assert '--trusted-installed-manifest "${INSTALLED_MANIFEST_RECEIPT_PATH}"' in runner_text
    assert '--trusted-service-unit "${SERVICE_UNIT_RECEIPT_PATH}"' in runner_text
    package_receipt_position = runner_text.index(
        'cp "${package_manifest_path}" "${INSTALLED_MANIFEST_RECEIPT_PATH}"'
    )
    install_position = runner_text.index("run_logged install-rpi-bundle")
    assert package_receipt_position < install_position
    service_install_position = runner_text.index(
        "run_logged install-rpi-comm-autostart"
    )
    assert install_position < service_install_position
    assert (
        'RENDERED_UNIT_RECEIPT_PATH="${SERVICE_UNIT_RECEIPT_PATH}"'
        in runner_text
    )
    assert "target_baseline_force_restart" not in runner_text

    installer_text = (
        ROOT / "scripts/install_rpi_comm_csp_autostart.sh"
    ).read_text(encoding="utf-8")
    assert 'RENDERED_UNIT_RECEIPT_PATH="${RENDERED_UNIT_RECEIPT_PATH:-}"' in (
        installer_text
    )
    assert (
        'install -m 0644 "${RENDERED_UNIT}" "${RENDERED_UNIT_RECEIPT_PATH}"'
        in installer_text
    )

    sync_text = (ROOT / "scripts/sync_rpi_workspace.sh").read_text(
        encoding="utf-8"
    )
    assert "--exclude='./output'" in sync_text
    assert "--exclude='./.codex_thesis_work'" in sync_text
    assert 'RPI_SYNC_EXTRA_EXCLUDE_PATH="${RPI_SYNC_EXTRA_EXCLUDE_PATH:-}"' in sync_text
    assert 'RPI_SYNC_TRACKED_ONLY="${RPI_SYNC_TRACKED_ONLY:-0}"' in sync_text
    assert 'RPI_SYNC_REPLACE_REMOTE="${RPI_SYNC_REPLACE_REMOTE:-0}"' in sync_text
    assert "git -C \"${ROOT_DIR}\" ls-files --recurse-submodules -z" in sync_text
    assert "route1-sync-previous" in sync_text

    print("Route 1 target revision provenance tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
