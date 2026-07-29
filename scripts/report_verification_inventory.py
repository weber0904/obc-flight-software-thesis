#!/usr/bin/env python3

from __future__ import annotations

import json
import sys
from verification_inventory_lib import generate_report


def main() -> None:
    report = generate_report()

    if "--json" in sys.argv:
        print(json.dumps(report, indent=2, sort_keys=True))
        return

    print("Verification Inventory Report")
    print(f"- baseline gate: {report['baselineGate']}")
    print(f"- real F' components scanned: {len(report['componentCoverage'])}")
    print(f"- helper/support modules tracked: {len(report['helperSupportCoverage'])}")
    print(f"- standalone/integration tests discovered: {len(report['standaloneAndIntegrationTests'])}")
    print(f"- probe scripts discovered: {len(report['probeScripts'])}")
    print(f"- evidence directories discovered: {len(report['evidenceDirectories'])}")
    print("- components without classic F' L2 coverage:")
    for name in report["componentsMissingClassicL2"]:
        print(f"  - {name}")
    print("- helpers without direct L1 coverage:")
    for name in report["helpersMissingDirectL1"]:
        print(f"  - {name}")


if __name__ == "__main__":
    main()
