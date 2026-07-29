#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/payload-target-backend-hardening-v1-hosted.XXXXXX")}"
mkdir -p "${PROBE_TMP_DIR}"

echo "Probe root: ${PROBE_TMP_DIR}"

COMPAT_ROOT="${PROBE_TMP_DIR}/compat-v1"
mkdir -p "${COMPAT_ROOT}"
PROBE_TMP_DIR="${COMPAT_ROOT}" bash "${ROOT_DIR}/scripts/run_payload_ops_contract_v1_hosted_probe.sh"

CSP_ROOT="${PROBE_TMP_DIR}/virtual-csp"
mkdir -p "${CSP_ROOT}"
PROBE_TMP_DIR="${CSP_ROOT}" bash "${ROOT_DIR}/scripts/run_payload_virtual_csp_node_v1_hosted_probe.sh"

echo "PASS ${PROBE_TMP_DIR}"
