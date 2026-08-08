#!/usr/bin/env python3

from __future__ import annotations

import sys

from verification_inventory_lib import collect_component_inventory, collect_helper_inventory


def main() -> None:
    problems: list[str] = []
    components = collect_component_inventory()
    helpers = collect_helper_inventory()

    missing_l2 = [item["name"] for item in components if not item["hasClassicL2"]]
    if missing_l2:
        problems.append(
            "real components missing classic F' L2 harness: " + ", ".join(sorted(missing_l2))
        )

    missing_helper_l1 = [item["name"] for item in helpers if not item["hasDirectL1"]]
    if missing_helper_l1:
        problems.append(
            "helper/support modules missing direct L1 coverage: "
            + ", ".join(sorted(missing_helper_l1))
        )

    if problems:
        for problem in problems:
            print(f"error: {problem}", file=sys.stderr)
        raise SystemExit(1)

    print("component-test baseline check passed")
    print(f"- real components checked: {len(components)}")
    print(f"- helper/support modules checked: {len(helpers)}")


if __name__ == "__main__":
    main()
