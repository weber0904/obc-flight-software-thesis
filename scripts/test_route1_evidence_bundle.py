#!/usr/bin/env python3
"""Focused positive and corruption-path tests for Route 1 evidence checking."""

from __future__ import annotations

import hashlib
import importlib.util
import json
import pathlib
import shutil
import subprocess
import sys
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[1]
CHECKER = ROOT / "scripts/check_route1_evidence_bundle.py"
CAMPAIGN = (
    ROOT
    / "docs/test-records/chapter5-integrated-route-closure-v1/artifacts"
    / "2026-07-20-route1-target-abc-rerun"
)
TARGET_WRAPPERS = (
    ROOT / "scripts/chapter5_routes/target/route1_prepare_and_capture.sh",
    ROOT / "scripts/chapter5_routes/target/route1_soc_fallback_check.sh",
)
FORBIDDEN_C_OWNED_SERVICE_OVERRIDES = (
    "COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS",
    "COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS",
    "COMM_GROUNDLINK_HEALTH_TIMEOUT_MS",
    "COMM_GROUNDLINK_DOWNLINK_V3_MAX_DATA_BYTES",
    "COMM_GROUNDLINK_DOWNLINK_V3_WINDOW_FRAMES_OVERRIDE",
    "COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_WAIT_MS",
    "COMM_GROUNDLINK_DOWNLINK_V3_SEND_PROBE_RETRY_SLEEP_MS",
    "COMM_GROUNDLINK_DOWNLINK_V3_INTERFRAME_DELAY_USEC",
    "COMM_CSP_SOCKETCAN_TX_FRAME_DELAY_USEC",
)
FORBIDDEN_CALLER_MANUAL_TIMEOUT_OVERRIDES = (
    "MANUAL_TARGET_COMM_GROUNDLINK_UPLINK_POLL_TIMEOUT_MS",
    "MANUAL_TARGET_COMM_GROUNDLINK_DOWNLINK_WRITE_TIMEOUT_MS",
    "MANUAL_TARGET_COMM_GROUNDLINK_HEALTH_TIMEOUT_MS",
    "MANUAL_TARGET_OBC_GROUNDLINK_TIMEOUTS_DROPIN_NAME",
    "MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS_DROPIN_NAME",
    "MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS_DROPIN_NAME",
)
ROUTE1_SERVICE_PROFILE = {
    "TARGET_SERVICE_PROFILE": "sband",
    "TARGET_SERVICE_COMMAND_AUTHORITY_PROFILE": "sband-primary",
    "TARGET_SERVICE_INITIAL_COMM_BAND": "sband",
    "TARGET_SERVICE_ENABLE_PRIMARY_GROUND_LINK_DRIVER": "1",
}
RAW_GROUND_CAPTURES = (
    "prepare-and-capture/scenario/sband-ground/native-cli/recv.bin",
    "prepare-and-capture/scenario/sband-ground/pipeline-logs/recv.bin",
    "soc-fallback/sband-ground/cli-logs/events/recv.bin",
)
PIPELINE_OBSERVATION_LOGS = (
    "prepare-and-capture/scenario/sband-ground/pipeline-logs/channel.log",
    "prepare-and-capture/scenario/sband-ground/pipeline-logs/event.log",
)
SOC_FALLBACK_RAW_GROUND_CAPTURE = (
    "soc-fallback/sband-ground/cli-logs/events/recv.bin"
)
RETAINED_SEQUENCE_ARTIFACTS = (
    "prepare-and-capture/scenario/sequence-src/route1-sequence-demo.seq",
    "prepare-and-capture/scenario/sequence-bin/route1-sequence-demo.bin",
)
PIPELINE_RECEIVED_FDP = (
    "prepare-and-capture/scenario/sband-ground/pipeline-store/fprime-downlink/"
    "_home_youjun_obc-deploy_runtime_comm-csp-lab-obc_data-products_"
    "Dp_268673025_1784485107_00846579.fdp"
)
CAMPAIGN_README = "README.md"
GOVERNANCE_STATUS_CORRUPTIONS = (
    (
        "docs/test-records/route1-sequence-verification-v1/README.md",
        "2026-07-12 proof remains a historical governed functional proof, not current target authority",
        "2026-07-12 proof remains the authoritative target A/B/C closure",
    ),
    (
        "docs/architecture/current-development-architecture.md",
        "2026-07-20 observation is not target A/B/C authority",
        "2026-07-20 observation is authoritative target A/B/C closure",
    ),
    (
        "docs/verification-path-registry.md",
        "Route 1 target requalification remains pending",
        "Route 1 target requalification is complete",
    ),
)


def run_checker(
    campaign_root: pathlib.Path,
    expected_returncode: int,
    repo_root: pathlib.Path = ROOT,
) -> None:
    result = subprocess.run(
        [
            "python3",
            str(CHECKER),
            "--repo-root",
            str(repo_root),
            "--campaign-root",
            str(campaign_root),
        ],
        check=False,
        capture_output=True,
        text=True,
    )
    if result.returncode != expected_returncode:
        raise AssertionError(
            f"checker returned {result.returncode}, expected {expected_returncode}\n"
            f"stdout:\n{result.stdout}\nstderr:\n{result.stderr}"
        )


def copy_governing_documents(destination_root: pathlib.Path) -> None:
    manifest = json.loads(
        (CAMPAIGN / "dedup-manifest.json").read_text(encoding="utf-8")
    )
    for relative_path in manifest["governingDocuments"]:
        destination = destination_root / relative_path
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / relative_path, destination)


def load_soc_fallback_module():
    module_path = (
        ROOT / "scripts/chapter5_routes/target/route1_soc_fallback_check.py"
    )
    spec = importlib.util.spec_from_file_location(
        "route1_soc_fallback_check_for_test", module_path
    )
    if spec is None or spec.loader is None:
        raise AssertionError(f"cannot load {module_path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def check_soc_fallback_restores_baseline() -> None:
    module = load_soc_fallback_module()

    class FakeScenario(module.Route1SocFallbackScenario):
        def __init__(
            self,
            fail_after_low_soc: bool = False,
            fail_restore: bool = False,
        ) -> None:
            self.mode = "test"
            self.summary_path = pathlib.Path("/tmp/unused-route1-summary.json")
            self.diagnostics_dir = pathlib.Path("/tmp")
            self.opcodes = {
                "OBCApp.epsBridge.EPS_GET_STATUS": 1,
            }
            self.sband = object()
            self.obc_target = "obc"
            self.obc_service = "obc.service"
            self.soc_values: list[float] = []
            self.checkpoints: list[tuple[str, str]] = []
            self.payloads: list[dict[str, object]] = []
            self.cleanup_errors: list[Exception | None] = []
            self.journal_waits = 0
            self.fail_after_low_soc = fail_after_low_soc
            self.fail_restore = fail_restore

        def begin_profile(self) -> None:
            pass

        def record_secure_auth_provenance(self) -> None:
            pass

        def load_route_opcodes(self) -> None:
            pass

        def resolve_eps_control_socket(self) -> str:
            return "/tmp/eps.sock"

        def start_security_server(self) -> None:
            pass

        def wait_for_sband_tcp_reachability(self, timeout: float) -> None:
            pass

        def start_ground_paths(self, **kwargs) -> None:
            pass

        def ensure_sband_ground_ready(self) -> str:
            return "ready"

        def prepare_ground_window(self, *args, **kwargs) -> None:
            pass

        def authenticate_secure_service(self, *args, **kwargs):
            return object()

        def remote_set_soc(
            self, socket_path: str, value: float, transition_sec: float = 0.0
        ) -> str:
            self.soc_values.append(value)
            if (
                self.fail_restore
                and value == self.BASELINE_SOC_PERCENT
                and len(self.soc_values) >= 4
            ):
                raise module.ProbeFailure("injected SoC restore failure")
            return (
                f"OK current_soc={value:.2f} target_soc={value:.2f} "
                f"transition_sec={transition_sec:.2f}"
            )

        def send_secure_command_name(self, *args, **kwargs):
            return object(), "test"

        def wait_for_target_journal(self, *args, **kwargs) -> None:
            self.journal_waits += 1
            if self.fail_after_low_soc and self.journal_waits == 2:
                raise module.ProbeFailure("injected low-SoC failure")

        def write_json_artifact(
            self, path: pathlib.Path, payload: dict[str, object]
        ) -> None:
            self.payloads.append(payload)

        def checkpoint(self, name: str, status: str = "info", **kwargs) -> None:
            self.checkpoints.append((name, status))

        def cleanup(self, run_error: Exception | None) -> None:
            self.cleanup_errors.append(run_error)

    original_ssh_capture = module.ssh_capture
    module.ssh_capture = lambda target, command: "2026-07-29 00:00:00"
    success = FakeScenario()
    try:
        summary = success.run()
    finally:
        module.ssh_capture = original_ssh_capture
    if success.soc_values != [80.0, 59.0, 39.0, 80.0]:
        raise AssertionError(
            f"successful Route 1 SoC scenario did not restore 80%: "
            f"{success.soc_values}"
        )
    if not any(line.startswith("soc-restore-response=OK ") for line in summary):
        raise AssertionError("successful Route 1 summary omits SoC restore")
    if success.payloads[-1]["verdict"] != "PASS":
        raise AssertionError("successful Route 1 scenario did not retain PASS")
    if success.cleanup_errors != [None]:
        raise AssertionError("successful Route 1 cleanup received an error")

    failed = FakeScenario(fail_after_low_soc=True)
    module.ssh_capture = lambda target, command: "2026-07-29 00:00:00"
    try:
        try:
            failed.run()
        except module.ProbeFailure:
            pass
        else:
            raise AssertionError("injected Route 1 failure unexpectedly passed")
    finally:
        module.ssh_capture = original_ssh_capture
    if failed.soc_values != [80.0, 59.0, 39.0, 80.0]:
        raise AssertionError(
            f"failed Route 1 SoC scenario did not restore 80%: "
            f"{failed.soc_values}"
        )
    if failed.payloads[-1]["verdict"] != "FAIL":
        raise AssertionError("failed Route 1 scenario did not retain FAIL")
    if not isinstance(failed.cleanup_errors[-1], module.ProbeFailure):
        raise AssertionError("failed Route 1 cleanup lost the original error")

    restore_failed = FakeScenario(fail_restore=True)
    module.ssh_capture = lambda target, command: "2026-07-29 00:00:00"
    try:
        try:
            restore_failed.run()
        except module.ProbeFailure:
            pass
        else:
            raise AssertionError("Route 1 scenario passed despite SoC restore failure")
    finally:
        module.ssh_capture = original_ssh_capture
    if restore_failed.soc_values != [80.0, 59.0, 39.0, 80.0, 80.0]:
        raise AssertionError(
            "Route 1 scenario did not retry a failed 80% SoC restore: "
            f"{restore_failed.soc_values}"
        )
    if restore_failed.payloads[-1]["verdict"] != "FAIL":
        raise AssertionError("SoC restore failure did not replace the PASS summary")


def main() -> int:
    run_checker(CAMPAIGN, 0)
    check_soc_fallback_restores_baseline()

    for wrapper in TARGET_WRAPPERS:
        wrapper_text = wrapper.read_text(encoding="utf-8")
        if (
            'if [[ "${OBC_COMM_CSP_SERVICE_NAME}" != '
            '"obc-comm-csp-stack.service" ]]' not in wrapper_text
        ):
            raise AssertionError(
                f"{wrapper.name} must reject a noncanonical OBC service"
            )
        if "TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1" not in wrapper_text:
            raise AssertionError(
                f"{wrapper.name} must delegate manual-auth profile ownership to A"
            )
        if "target-manual-auth-preflight-apply" in wrapper_text:
            raise AssertionError(
                f"{wrapper.name} must not apply shared-service overrides after A/B"
            )
        if wrapper_text.index(
            "TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1"
        ) > wrapper_text.index('bash "${ENSURE_GROUND_BASELINE}"'):
            raise AssertionError(
                f"{wrapper.name} must request the manual-auth profile from A before B"
            )
        manual_unset_position = wrapper_text.index("unset")
        a_preflight_position = wrapper_text.index(
            "TARGET_BASELINE_INCLUDE_MANUAL_AUTH_PREFLIGHT=1"
        )
        if manual_unset_position > a_preflight_position:
            raise AssertionError(
                f"{wrapper.name} must clear caller manual timeouts before A"
            )
        for name in FORBIDDEN_CALLER_MANUAL_TIMEOUT_OVERRIDES:
            if name not in wrapper_text[manual_unset_position:a_preflight_position]:
                raise AssertionError(
                    f"{wrapper.name} must clear inherited {name} before A"
                )
        for name, value in (
            ("MANUAL_TARGET_OBC_GROUNDLINK_DIAGNOSTICS", "0"),
            ("MANUAL_TARGET_SBAND_INGRESS_DIAGNOSTICS", "1"),
        ):
            if f"export {name}={value}" not in wrapper_text[
                manual_unset_position:a_preflight_position
            ]:
                raise AssertionError(
                    f"{wrapper.name} must pin {name}={value} before A"
                )
        for name in FORBIDDEN_C_OWNED_SERVICE_OVERRIDES:
            if f"-u {name}" not in wrapper_text:
                raise AssertionError(
                    f"{wrapper.name} must explicitly drop inherited {name}"
                )
            if f'{name}="${{' in wrapper_text:
                raise AssertionError(
                    f"{wrapper.name} must not forward {name} into the target probe"
                )
        for name, value in ROUTE1_SERVICE_PROFILE.items():
            if f"{name}={value}" not in wrapper_text:
                raise AssertionError(
                    f"{wrapper.name} must pin {name} to the Route 1 baseline"
                )
            if f'{name}="${{' in wrapper_text:
                raise AssertionError(
                    f"{wrapper.name} must not inherit {name} from the caller"
                )

    with tempfile.TemporaryDirectory(prefix="route1-evidence-manifest-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        manifest_path = copied_campaign / "dedup-manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["canonicalizations"][0]["sha256"] = "0" * 64
        manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
        run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(prefix="route1-evidence-capture-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        missing_artifact = (
            copied_campaign
            / "prepare-and-capture/scenario/captures/sband/gds-to-southbound.bin"
        )
        missing_artifact.unlink()
        run_checker(copied_campaign, 1)

    for relative_path in RAW_GROUND_CAPTURES:
        with tempfile.TemporaryDirectory(
            prefix="route1-evidence-raw-ground-capture-test-"
        ) as temp_dir:
            copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
            shutil.copytree(CAMPAIGN, copied_campaign)
            altered_capture = copied_campaign / relative_path
            payload = bytearray(altered_capture.read_bytes())
            payload[-1] ^= 0x01
            altered_capture.write_bytes(payload)
            run_checker(copied_campaign, 1)

    for relative_path in PIPELINE_OBSERVATION_LOGS:
        for mode in ("mutate", "delete"):
            with tempfile.TemporaryDirectory(
                prefix=f"route1-evidence-pipeline-observation-{mode}-test-"
            ) as temp_dir:
                copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
                shutil.copytree(CAMPAIGN, copied_campaign)
                observation = copied_campaign / relative_path
                if mode == "mutate":
                    observation.write_text(
                        observation.read_text(encoding="utf-8") + "\nmutation\n",
                        encoding="utf-8",
                    )
                else:
                    observation.unlink()
                run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(
        prefix="route1-evidence-soc-fallback-recv-deletion-test-"
    ) as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        (copied_campaign / SOC_FALLBACK_RAW_GROUND_CAPTURE).unlink()
        run_checker(copied_campaign, 1)

    for relative_path in RETAINED_SEQUENCE_ARTIFACTS:
        with tempfile.TemporaryDirectory(
            prefix="route1-evidence-sequence-input-mutation-test-"
        ) as temp_dir:
            copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
            shutil.copytree(CAMPAIGN, copied_campaign)
            altered_input = copied_campaign / relative_path
            payload = bytearray(altered_input.read_bytes())
            payload[-1] ^= 0x01
            altered_input.write_bytes(payload)
            run_checker(copied_campaign, 1)

        with tempfile.TemporaryDirectory(
            prefix="route1-evidence-sequence-input-deletion-test-"
        ) as temp_dir:
            copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
            shutil.copytree(CAMPAIGN, copied_campaign)
            (copied_campaign / relative_path).unlink()
            run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(
        prefix="route1-evidence-sequence-input-manifest-test-"
    ) as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        manifest_path = copied_campaign / "dedup-manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        manifest["retainedArtifacts"] = [
            record
            for record in manifest["retainedArtifacts"]
            if record["path"] != RETAINED_SEQUENCE_ARTIFACTS[0]
        ]
        manifest_path.write_text(
            json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
        )
        run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(prefix="route1-evidence-baseline-hash-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        altered_baseline = (
            copied_campaign / "prepare-and-capture/baseline/target-before.json"
        )
        altered_baseline.write_text("{}\n", encoding="utf-8")
        run_checker(copied_campaign, 1)

    for mode in ("mutate", "delete", "promote-authority"):
        with tempfile.TemporaryDirectory(
            prefix=f"route1-evidence-campaign-readme-{mode}-test-"
        ) as temp_dir:
            copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
            shutil.copytree(CAMPAIGN, copied_campaign)
            readme = copied_campaign / CAMPAIGN_README
            if mode == "mutate":
                readme.write_text(
                    readme.read_text(encoding="utf-8") + "\nmutation\n",
                    encoding="utf-8",
                )
            elif mode == "delete":
                readme.unlink()
            else:
                text = readme.read_text(encoding="utf-8")
                readme.write_text(
                    text
                    + "\nThe 2026-07-20 campaign is authoritative target closure.\n",
                    encoding="utf-8",
                )
                manifest_path = copied_campaign / "dedup-manifest.json"
                manifest = json.loads(
                    manifest_path.read_text(encoding="utf-8")
                )
                for record in manifest["retainedArtifacts"]:
                    if record["path"] == CAMPAIGN_README:
                        record["sha256"] = hashlib.sha256(
                            readme.read_bytes()
                        ).hexdigest()
                        break
                manifest_path.write_text(
                    json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8",
                )
            run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(prefix="route1-evidence-fdp-hash-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        altered_fdp = (
            copied_campaign
            / "prepare-and-capture/scenario/source-artifacts/auto/data-products"
            / "Dp_268673025_1784485107_00846579.fdp"
        )
        payload = bytearray(altered_fdp.read_bytes())
        payload[-1] ^= 0x01
        altered_fdp.write_bytes(payload)
        run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(
        prefix="route1-evidence-received-fdp-test-"
    ) as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        manifest_path = copied_campaign / "dedup-manifest.json"
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        deterministic = next(
            record
            for record in manifest["canonicalizations"]
            if record["policy"] == "CAPTURE_DETERMINISTIC"
        )
        received_path = copied_campaign / deterministic["receivedFdp"]["path"]
        deterministic["receivedFdp"] = None
        manifest_path.write_text(
            json.dumps(manifest, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
        received_path.unlink()
        run_checker(copied_campaign, 1)

    for mode in ("mutate", "delete", "remove-manifest-entry"):
        with tempfile.TemporaryDirectory(
            prefix=f"route1-evidence-pipeline-fdp-{mode}-test-"
        ) as temp_dir:
            copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
            shutil.copytree(CAMPAIGN, copied_campaign)
            pipeline_fdp = copied_campaign / PIPELINE_RECEIVED_FDP
            if mode == "mutate":
                payload = bytearray(pipeline_fdp.read_bytes())
                payload[-1] ^= 0x01
                pipeline_fdp.write_bytes(payload)
            elif mode == "delete":
                pipeline_fdp.unlink()
            else:
                manifest_path = copied_campaign / "dedup-manifest.json"
                manifest = json.loads(
                    manifest_path.read_text(encoding="utf-8")
                )
                manifest["retainedArtifacts"] = [
                    record
                    for record in manifest["retainedArtifacts"]
                    if record["path"] != PIPELINE_RECEIVED_FDP
                ]
                manifest_path.write_text(
                    json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                    encoding="utf-8",
                )
            run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(prefix="route1-evidence-provenance-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        missing_provenance = (
            copied_campaign
            / "prepare-and-capture/scenario/diagnostics/secure-auth-keystore-provenance.json"
        )
        missing_provenance.unlink()
        run_checker(copied_campaign, 1)

    with tempfile.TemporaryDirectory(prefix="route1-evidence-provenance-hash-test-") as temp_dir:
        copied_campaign = pathlib.Path(temp_dir) / CAMPAIGN.name
        shutil.copytree(CAMPAIGN, copied_campaign)
        altered_provenance = (
            copied_campaign
            / "soc-fallback/diagnostics/secure-auth-keystore-provenance.json"
        )
        altered_provenance.write_text("{}\n", encoding="utf-8")
        run_checker(copied_campaign, 1)

    for relative_path, original, replacement in GOVERNANCE_STATUS_CORRUPTIONS:
        with tempfile.TemporaryDirectory(
            prefix="route1-evidence-governing-authority-test-"
        ) as temp_dir:
            copied_repo = pathlib.Path(temp_dir) / "repo"
            copied_campaign = copied_repo / "campaign"
            shutil.copytree(CAMPAIGN, copied_campaign)
            copy_governing_documents(copied_repo)
            document = copied_repo / relative_path
            text = document.read_text(encoding="utf-8")
            if original not in text:
                raise AssertionError(
                    f"governance status test fixture missing: {original}"
                )
            document.write_text(
                text.replace(original, replacement, 1), encoding="utf-8"
            )
            run_checker(copied_campaign, 1, copied_repo)

    for field, replacement in (
        (
            "historicalProofContains",
            "2026-07-12 is notably authoritative",
        ),
        (
            "nonAuthoritativeObservationContains",
            "2026-07-20 is notably authoritative",
        ),
    ):
        with tempfile.TemporaryDirectory(
            prefix="route1-evidence-false-negation-test-"
        ) as temp_dir:
            copied_repo = pathlib.Path(temp_dir) / "repo"
            copied_campaign = copied_repo / "campaign"
            shutil.copytree(CAMPAIGN, copied_campaign)
            copy_governing_documents(copied_repo)
            manifest_path = copied_campaign / "dedup-manifest.json"
            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            for assertion in manifest["governanceStatusAssertions"]:
                document = copied_repo / assertion["path"]
                text = document.read_text(encoding="utf-8")
                original = assertion[field]
                if original not in text:
                    raise AssertionError(
                        f"false-negation fixture missing: {original}"
                    )
                document.write_text(
                    text.replace(original, replacement, 1),
                    encoding="utf-8",
                )
                assertion[field] = replacement
            manifest_path.write_text(
                json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                encoding="utf-8",
            )
            run_checker(copied_campaign, 1, copied_repo)

    with tempfile.TemporaryDirectory(
        prefix="route1-evidence-contradictory-authority-test-"
    ) as temp_dir:
        copied_repo = pathlib.Path(temp_dir) / "repo"
        copied_campaign = copied_repo / "campaign"
        shutil.copytree(CAMPAIGN, copied_campaign)
        copy_governing_documents(copied_repo)
        manifest = json.loads(
            (copied_campaign / "dedup-manifest.json").read_text(
                encoding="utf-8"
            )
        )
        document = (
            copied_repo
            / manifest["governanceStatusAssertions"][0]["path"]
        )
        document.write_text(
            document.read_text(encoding="utf-8")
            + "\nThe 2026-07-20 observation is authoritative.\n",
            encoding="utf-8",
        )
        run_checker(copied_campaign, 1, copied_repo)

    for label, statement in (
        (
            "double-negation",
            "The 2026-07-20 observation is not non-authoritative; "
            "it is authoritative.",
        ),
        (
            "later-clause",
            "The 2026-07-20 observation is non-authoritative: "
            "it is authoritative.",
        ),
        (
            "coordinated-but-clause",
            "The 2026-07-20 observation is not authoritative, "
            "but it is authoritative.",
        ),
        (
            "coordinated-and-clause",
            "The 2026-07-20 observation is non-authoritative "
            "and it is authoritative.",
        ),
        (
            "positive-before-rejection-context",
            "The 2026-07-20 observation is not authoritative, "
            "but it is authoritative, which should fail validation.",
        ),
        (
            "while-remains-positive",
            "The 2026-07-20 observation is not authoritative, "
            "while it remains authoritative.",
        ),
        (
            "although-continues-positive",
            "The 2026-07-20 observation is not authoritative, "
            "although it continues to be authoritative.",
        ),
        (
            "natural-language-date",
            "The July 20, 2026 observation is authoritative.",
        ),
        (
            "day-first-natural-language-date",
            "The 20 July 2026 observation remains authoritative.",
        ),
        (
            "chinese-date",
            "The 2026年7月20日 observation is authoritative.",
        ),
    ):
        with tempfile.TemporaryDirectory(
            prefix=f"route1-evidence-{label}-authority-test-"
        ) as temp_dir:
            copied_repo = pathlib.Path(temp_dir) / "repo"
            copied_campaign = copied_repo / "campaign"
            shutil.copytree(CAMPAIGN, copied_campaign)
            copy_governing_documents(copied_repo)
            manifest = json.loads(
                (copied_campaign / "dedup-manifest.json").read_text(
                    encoding="utf-8"
                )
            )
            document = (
                copied_repo
                / manifest["governanceStatusAssertions"][0]["path"]
            )
            document.write_text(
                document.read_text(encoding="utf-8")
                + f"\n{statement}\n",
                encoding="utf-8",
            )
            run_checker(copied_campaign, 1, copied_repo)

    print("Route 1 evidence checker tests: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
