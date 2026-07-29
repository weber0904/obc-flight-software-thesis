#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}" || true)"
DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"
PYTHON_BIN="${ROOT_DIR}/fprime-venv/bin/python"
FPRIME_CLI_BIN="${ROOT_DIR}/fprime-venv/bin/fprime-cli"

if [[ -z "${BIN_DIR}" || -z "${DICT_PATH}" || ! -x "${PYTHON_BIN}" || ! -x "${FPRIME_CLI_BIN}" ]]; then
  echo "Required build outputs or fprime-venv tools are missing. Run PATH=\"\$PWD/fprime-venv/bin:\$PATH\" fprime-util build first." >&2
  exit 1
fi

exec "${PYTHON_BIN}" "${ROOT_DIR}/scripts/per_band_stock_ground_stacks.py" \
  --mode uhf \
  --root-dir "${ROOT_DIR}" \
  --bin-dir "${BIN_DIR}" \
  --dictionary-path "${DICT_PATH}" \
  --cli-path "${FPRIME_CLI_BIN}"
