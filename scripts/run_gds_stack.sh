#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

DICT_PATH="$(obc_find_dictionary_path "${ROOT_DIR}" OBC || true)"

if [[ -z "${DICT_PATH}" ]]; then
  echo "Dictionary not found. Run fprime-util build first." >&2
  exit 1
fi

export GDS_HOST="${GDS_HOST:-127.0.0.1}"
export GDS_PORT="${GDS_PORT:-50000}"

cat <<EOF
Start the ground side in another terminal with:
fprime-gds -n -g none --framing-selection space-packet-space-data-link --scid 68 --vcid 1 --frame-size 1024 --dictionary "${DICT_PATH}" --ip-address "${GDS_HOST}" --ip-port "${GDS_PORT}"

This script starts the internal CSP hub, EPS/ADCS/radio mocks, and the OBC runtime.
The OBC runtime uses the default hosted S-band CCSDS path through sband_comm_csp_node node 5 and ground_ttc_gateway raw relay.
EOF

export GROUND_LINK_MODE="${GROUND_LINK_MODE:-comm-csp}"
export COMM_CSP_NODE="${COMM_CSP_NODE:-5}"
export OBC_BINARY_NAME="${OBC_BINARY_NAME:-OBC}"

exec "${ROOT_DIR}/scripts/run_dev_stack.sh"
