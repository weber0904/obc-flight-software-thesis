#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
SEQGEN_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-seqgen"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${PYTHON_BIN}" || ! -x "${SEQGEN_BIN}" ]]; then
  echo "Required build outputs or fprime-venv python are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

PROBE_TMP_DIR="${PROBE_TMP_DIR:-$(mktemp -d "/tmp/uplink-authority-and-key-hardening-hosted.XXXXXX")}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${PROBE_TMP_DIR}/runtime-root}"
mkdir -p "${PROBE_TMP_DIR}"

ROOT_DIR="${ROOT_DIR}" \
BIN_DIR="${BIN_DIR}" \
DICT_PATH="${DICT_PATH}" \
PROBE_TMP_DIR="${PROBE_TMP_DIR}" \
RUNTIME_ROOT="${RUNTIME_ROOT}" \
PYTHON_BIN="${PYTHON_BIN}" \
SEQGEN_BIN="${SEQGEN_BIN}" \
"${PYTHON_BIN}" "${ROOT_DIR}/scripts/uplink_authority_and_key_hardening_hosted_probe.py"
