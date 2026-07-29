#!/usr/bin/env python3

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
COMPONENTS_DIR = ROOT / "OBC" / "Components"
SIMULATORS_DIR = ROOT / "simulators"
TOP_DIR = ROOT / "OBC" / "TopCcsds"
SCRIPTS_DIR = ROOT / "scripts"
TEST_RECORDS_DIR = ROOT / "docs" / "test-records"
BASELINE_GATE = "scripts/run_verification_ci.sh"

COMPONENT_CAPABILITY = {
    "AdcsBridge": "adcs-subsystem",
    "BootManager": "boot-update",
    "CommController": "comm-subsystem",
    "CspBridge": "core-system-contracts",
    "EpsBridge": "eps-subsystem",
    "GpsBridge": "gps-subsystem",
    "HealthMonitor": "core-system-contracts",
    "MissionExecutive": "mission-autonomy",
    "ModeManager": "core-system-contracts",
    "BeaconPublisher": "onboard-data-products-and-live-beacon",
    "DpCatalogFileDownlinkGate": "onboard-data-products-and-live-beacon",
    "HkTrendProductProducer": "onboard-data-products-and-live-beacon",
    "OnboardStateMonitor": "onboard-data-products-and-live-beacon",
    "RadioController": "comm-subsystem",
    "StorageHealthBridge": "storage-health",
    "UartDriver": "comm-subsystem",
}

HELPER_SUPPORT_MODULES = [
    {
        "name": "StorageScanner",
        "paths": ["OBC/Components/StorageHealthBridge/StorageScanner.hpp"],
        "ownerComponent": "StorageHealthBridge",
        "capability": "storage-health",
        "tests": ["storage_scanner_unit_test"],
    },
    {
        "name": "GpsSource/NmeaParser",
        "paths": [
            "simulators/gps/GpsSource.hpp",
            "simulators/gps/NmeaParser.hpp",
        ],
        "ownerComponent": "GpsBridge",
        "capability": "gps-subsystem",
        "tests": ["gps_support_unit_test"],
    },
    {
        "name": "TransparentLinkFraming",
        "paths": ["simulators/comm/TransparentLinkFraming.hpp"],
        "ownerComponent": "CommController",
        "capability": "comm-subsystem",
        "tests": ["transparent_link_framing_unit_test"],
    },
    {
        "name": "OnboardStateData",
        "paths": ["OBC/Components/OnboardStateData/OnboardStateData.hpp"],
        "ownerComponent": "OnboardStateMonitor",
        "capability": "onboard-data-products-and-live-beacon",
        "tests": ["onboard_state_data_unit_test"],
    },
    {
        "name": "OnboardStateSnapshotSource",
        "paths": ["OBC/TopCcsds/OnboardStateSnapshotSource.hpp"],
        "ownerComponent": "OnboardStateMonitor",
        "capability": "onboard-data-products-and-live-beacon",
        "tests": ["onboard_state_snapshot_source_unit_test"],
    },
]


def classify_test(name: str) -> str:
    lowered = name.lower()
    if "unit" in lowered:
        return "L1"
    if "contract" in lowered:
        return "L2"
    if "integration" in lowered:
        return "L3"
    if lowered.endswith("_ut_exe"):
        return "L2"
    return "unknown"


def find_add_tests(cmake_text: str) -> list[str]:
    return re.findall(r"add_test\(NAME\s+([A-Za-z0-9_:-]+)", cmake_text)


def collect_all_add_tests() -> dict[str, str]:
    tests: dict[str, str] = {}
    for root_dir in (COMPONENTS_DIR, SIMULATORS_DIR, TOP_DIR):
        for cmake_path in root_dir.rglob("CMakeLists.txt"):
            rel = str(cmake_path.relative_to(ROOT))
            text = cmake_path.read_text(encoding="utf-8")
            for name in find_add_tests(text):
                tests[name] = rel
    return tests


def is_real_component_header(header_path: Path) -> bool:
    if not header_path.is_file():
        return False
    text = header_path.read_text(encoding="utf-8")
    return bool(
        re.search(
            r"class\s+\w+\s*(?:final\s*)?:\s*public\s+\w+ComponentBase\b",
            text,
        )
    )


def collect_component_inventory() -> list[dict]:
    inventory = []
    for component_dir in sorted(COMPONENTS_DIR.iterdir()):
        if not component_dir.is_dir():
            continue
        name = component_dir.name
        header_path = component_dir / f"{name}.hpp"
        if not is_real_component_header(header_path):
            continue
        cmake_path = component_dir / "CMakeLists.txt"
        cmake_text = cmake_path.read_text(encoding="utf-8") if cmake_path.is_file() else ""
        has_classic = "register_fprime_ut()" in cmake_text or "register_fprime_ut(" in cmake_text
        classic_target = f"OBC_Components_{name}_ut_exe" if has_classic else None
        inventory.append(
            {
                "name": name,
                "capability": COMPONENT_CAPABILITY.get(name, "unknown"),
                "modulePath": str(component_dir.relative_to(ROOT)),
                "headerPath": str(header_path.relative_to(ROOT)),
                "cmakePath": str(cmake_path.relative_to(ROOT)) if cmake_path.is_file() else None,
                "hasClassicL2": has_classic,
                "classicHarnessTarget": classic_target,
                "registeredAddTests": find_add_tests(cmake_text),
            }
        )
    return inventory


def collect_helper_inventory() -> list[dict]:
    add_tests = collect_all_add_tests()
    inventory = []
    for helper in HELPER_SUPPORT_MODULES:
        registered = [name for name in helper["tests"] if name in add_tests]
        inventory.append(
            {
                "name": helper["name"],
                "capability": helper["capability"],
                "ownerComponent": helper["ownerComponent"],
                "paths": helper["paths"],
                "registeredTests": registered,
                "hasDirectL1": bool(registered),
            }
        )
    return inventory


def collect_probe_scripts() -> list[str]:
    return [
        str(script_path.relative_to(ROOT))
        for script_path in sorted(SCRIPTS_DIR.glob("run_*_probe.sh"))
    ]


def collect_evidence_dirs() -> list[str]:
    return [
        str(path.relative_to(ROOT))
        for path in sorted(TEST_RECORDS_DIR.iterdir())
        if path.is_dir() and path.name != "templates"
    ]


def generate_report() -> dict:
    components = collect_component_inventory()
    helpers = collect_helper_inventory()
    add_tests = collect_all_add_tests()
    standalone_tests = [
        {
            "name": name,
            "layer": classify_test(name),
            "source": source,
        }
        for name, source in sorted(add_tests.items())
    ]
    return {
        "baselineGate": BASELINE_GATE,
        "componentCoverage": components,
        "helperSupportCoverage": helpers,
        "standaloneAndIntegrationTests": standalone_tests,
        "probeScripts": collect_probe_scripts(),
        "evidenceDirectories": collect_evidence_dirs(),
        "componentsMissingClassicL2": sorted(
            item["name"] for item in components if not item["hasClassicL2"]
        ),
        "helpersMissingDirectL1": sorted(
            item["name"] for item in helpers if not item["hasDirectL1"]
        ),
    }
