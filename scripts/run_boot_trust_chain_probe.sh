#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

BIN_DIR="$(obc_find_native_bin_dir "${ROOT_DIR}")"
if [[ -z "${BIN_DIR}" ]]; then
  echo "Build output not found. Run fprime-util build first." >&2
  exit 1
fi

RUNTIME_ROOT="${RUNTIME_ROOT:-/tmp/boot-trust-chain-v1-probe}"
PERSISTENT_ROOT="${PERSISTENT_ROOT:-${RUNTIME_ROOT}/persistent-data}"
STAGING_ROOT="${STAGING_ROOT:-${RUNTIME_ROOT}/staging}"
LOG_ROOT="${LOG_ROOT:-${RUNTIME_ROOT}/logs}"
BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID:-repo-dev-boot-signer}"
BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT:-1}"
BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX:-424f4f545f54525553545f434841494e5f56315f4445565f4b4559}"
RADIO_PORT="${RADIO_PORT:-7020}"
CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT:-56320}"
CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT:-57320}"
SBAND_TCP_PORT="${SBAND_TCP_PORT:-18540}"

rm -rf "${RUNTIME_ROOT}"
mkdir -p "${PERSISTENT_ROOT}" "${STAGING_ROOT}" "${LOG_ROOT}"

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

write_manifest() {
  local image_name="$1"
  local image_size="$2"
  local image_digest="$3"
  local target_slot="$4"
  local software_version="$5"
  local valid_signature="${6:-1}"

  python3 - "$STAGING_ROOT/${image_name}.manifest-v1" \
    "$image_name" \
    "$image_size" \
    "$image_digest" \
    "$target_slot" \
    "$software_version" \
    "$BOOT_TRUST_SIGNER_ID" \
    "$BOOT_TRUST_KEY_SLOT" \
    "$BOOT_TRUST_KEY_HEX" \
    "$valid_signature" <<'PY'
import hashlib
import hmac
import sys

(
    path,
    image_name,
    image_size,
    image_digest,
    target_slot,
    software_version,
    signer_id,
    key_slot,
    key_hex,
    valid_signature,
) = sys.argv[1:]

payload = (
    "schema=boot_manifest_v1\n"
    f"image_path={image_name}\n"
    f"image_size={image_size}\n"
    f"image_digest_sha256={image_digest}\n"
    f"target_slot={target_slot}\n"
    "image_id=hosted-probe-image\n"
    f"software_version={software_version}\n"
    f"signer_id={signer_id}\n"
    f"key_slot={key_slot}\n"
    "signature_algorithm=hmac-sha256\n"
)
signature = hmac.new(bytes.fromhex(key_hex), payload.encode("utf-8"), hashlib.sha256).hexdigest()
if valid_signature == "0":
    signature = ("1" if signature[0] == "0" else "0") + signature[1:]

with open(path, "w", encoding="utf-8") as out:
    out.write(payload)
    out.write(f"signature={signature}\n")
PY
}

run_stack_commands() {
  local output_log="$1"
  shift

  {
    sleep 2
    for command in "$@"; do
      printf '%s\n' "$command"
      sleep 1
    done
    printf 'quit\n'
  } | env \
    RUNTIME_ROOT="${RUNTIME_ROOT}" \
    PERSISTENT_ROOT="${PERSISTENT_ROOT}" \
    STAGING_ROOT="${STAGING_ROOT}" \
    LOG_ROOT="${LOG_ROOT}" \
    RADIO_PORT="${RADIO_PORT}" \
    GDS_PORT=0 \
    GROUND_LINK_MODE=disabled \
    MANAGE_SBAND_COMM_NODE=0 \
    MANAGE_GROUND_TTC_GATEWAY=0 \
    CSP_HUB_SUB_PORT="${CSP_HUB_SUB_PORT}" \
    CSP_HUB_PUB_PORT="${CSP_HUB_PUB_PORT}" \
    SBAND_TCP_PORT="${SBAND_TCP_PORT}" \
    BOOT_TRUST=hmac-sha256 \
    BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID}" \
    BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT}" \
    BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX}" \
    bash "${ROOT_DIR}/scripts/run_dev_stack.sh" >"${output_log}" 2>&1
}

require_log_text() {
  local output_log="$1"
  local expected="$2"
  if ! grep -Fq "$expected" "$output_log"; then
    echo "Expected text not found in ${output_log}: ${expected}" >&2
    echo "--- ${output_log} ---" >&2
    cat "$output_log" >&2
    exit 1
  fi
}

write_image() {
  local image_name="$1"
  local contents="$2"
  printf '%s\n' "$contents" >"${STAGING_ROOT}/${image_name}"
}

VALID_IMAGE="trusted-v1.bin"
write_image "$VALID_IMAGE" "hosted boot trust chain image v1"
VALID_SIZE="$(wc -c <"${STAGING_ROOT}/${VALID_IMAGE}" | tr -d ' ')"
VALID_DIGEST="$(sha256_file "${STAGING_ROOT}/${VALID_IMAGE}")"
write_manifest "$VALID_IMAGE" "$VALID_SIZE" "$VALID_DIGEST" "SLOT_B" "1" "1"

VALID_LOG="${LOG_ROOT}/valid-activation.log"
run_stack_commands "$VALID_LOG" \
  "boot prepare ${VALID_SIZE} ${VALID_DIGEST}" \
  "boot verify ${VALID_IMAGE}" \
  "boot activate" \
  "boot status"
require_log_text "$VALID_LOG" "boot prepare response=0"
require_log_text "$VALID_LOG" "boot verify response=0"
require_log_text "$VALID_LOG" "boot activate response=0"
require_log_text "$VALID_LOG" "trustStatus=3"
require_log_text "$VALID_LOG" "lastAcceptedVersion=1"

CONFIRM_LOG="${LOG_ROOT}/confirm-reload.log"
run_stack_commands "$CONFIRM_LOG" \
  "boot status" \
  "boot confirm" \
  "boot status"
require_log_text "$CONFIRM_LOG" "pending=SLOT_B"
require_log_text "$CONFIRM_LOG" "boot confirm response=0"
require_log_text "$CONFIRM_LOG" "trustStatus=4"
require_log_text "$CONFIRM_LOG" "activeVersion=1"
require_log_text "$CONFIRM_LOG" "lastAcceptedVersion=1"

BAD_SIG_IMAGE="bad-signature-v2.bin"
write_image "$BAD_SIG_IMAGE" "hosted boot trust chain image v2 bad signature"
BAD_SIG_SIZE="$(wc -c <"${STAGING_ROOT}/${BAD_SIG_IMAGE}" | tr -d ' ')"
BAD_SIG_DIGEST="$(sha256_file "${STAGING_ROOT}/${BAD_SIG_IMAGE}")"
write_manifest "$BAD_SIG_IMAGE" "$BAD_SIG_SIZE" "$BAD_SIG_DIGEST" "SLOT_A" "2" "0"

BAD_SIG_LOG="${LOG_ROOT}/invalid-signature.log"
run_stack_commands "$BAD_SIG_LOG" \
  "boot prepare ${BAD_SIG_SIZE} ${BAD_SIG_DIGEST}" \
  "boot verify ${BAD_SIG_IMAGE}" \
  "boot status"
require_log_text "$BAD_SIG_LOG" "boot verify response=2"
require_log_text "$BAD_SIG_LOG" "trustStatus=2"
require_log_text "$BAD_SIG_LOG" "trustRejectReason=14"
require_log_text "$BAD_SIG_LOG" "lastAcceptedVersion=1"

DOWNGRADE_IMAGE="downgrade-v1.bin"
write_image "$DOWNGRADE_IMAGE" "hosted boot trust chain downgrade attempt"
DOWNGRADE_SIZE="$(wc -c <"${STAGING_ROOT}/${DOWNGRADE_IMAGE}" | tr -d ' ')"
DOWNGRADE_DIGEST="$(sha256_file "${STAGING_ROOT}/${DOWNGRADE_IMAGE}")"
write_manifest "$DOWNGRADE_IMAGE" "$DOWNGRADE_SIZE" "$DOWNGRADE_DIGEST" "SLOT_A" "1" "1"

DOWNGRADE_LOG="${LOG_ROOT}/downgrade.log"
run_stack_commands "$DOWNGRADE_LOG" \
  "boot prepare ${DOWNGRADE_SIZE} ${DOWNGRADE_DIGEST}" \
  "boot verify ${DOWNGRADE_IMAGE}" \
  "boot status"
require_log_text "$DOWNGRADE_LOG" "boot verify response=2"
require_log_text "$DOWNGRADE_LOG" "trustStatus=2"
require_log_text "$DOWNGRADE_LOG" "trustRejectReason=15"
require_log_text "$DOWNGRADE_LOG" "lastAcceptedVersion=1"

echo "boot-trust-chain probe PASS"
echo "runtime-root=${RUNTIME_ROOT}"
echo "valid-log=${VALID_LOG}"
echo "confirm-log=${CONFIRM_LOG}"
echo "invalid-signature-log=${BAD_SIG_LOG}"
echo "downgrade-log=${DOWNGRADE_LOG}"
