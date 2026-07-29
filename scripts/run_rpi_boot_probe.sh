#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "${ROOT_DIR}/scripts/_common.sh"

OBC_SSH_TARGET="${OBC_SSH_TARGET:-$(obc_resolve_obc_ssh_target)}"
RPI_REMOTE_DIR="${RPI_REMOTE_DIR:-/home/operator/lab/fprime/v0}"
RUNTIME_ROOT="${RUNTIME_ROOT:-${RPI_REMOTE_DIR}/runtime/boot-restart-probe}"
RADIO_PROTOCOL="${RADIO_PROTOCOL:-mock-text}"
BOOT_TRUST_SIGNER_ID="${BOOT_TRUST_SIGNER_ID:-repo-dev-boot-signer}"
BOOT_TRUST_KEY_SLOT="${BOOT_TRUST_KEY_SLOT:-1}"
BOOT_TRUST_KEY_HEX="${BOOT_TRUST_KEY_HEX:-424f4f545f54525553545f434841494e5f56315f4445565f4b4559}"

ssh "${OBC_SSH_TARGET}" /bin/bash <<EOF
set -euo pipefail

cd $(printf '%q' "${RPI_REMOTE_DIR}")
BIN_DIR=$(printf '%q' "${RPI_REMOTE_DIR}")/build-fprime-automatic-native/bin/Linux
RUNTIME_ROOT=$(printf '%q' "${RUNTIME_ROOT}")
PERSISTENT_ROOT="\${RUNTIME_ROOT}/persistent-data"
STAGING_ROOT="\${RUNTIME_ROOT}/staging"
METADATA_PATH="\${PERSISTENT_ROOT}/boot/metadata-v1.txt"

mkdir -p "\${PERSISTENT_ROOT}" "\${STAGING_ROOT}" "\${RUNTIME_ROOT}/logs"

pids=\$(ps -eo pid=,args= | awk '/build-fprime-automatic-native\\/bin\\/Linux\\/(csp_zmqproxy|eps_simulator|adcs_simulator|radio_mock_server|OBC)/ {print \$1}')
if [[ -n "\${pids}" ]]; then
  kill \${pids} >/dev/null 2>&1 || true
  sleep 1
fi

"\${BIN_DIR}/csp_zmqproxy" -s tcp://0.0.0.0:56400 -p tcp://0.0.0.0:57400 </dev/null >/tmp/obc-boot-probe-csp.log 2>&1 &
CSP_PID=\$!
CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=56400 CSP_HUB_PUB_PORT=57400 "\${BIN_DIR}/eps_simulator" --node-id 2 </dev/null >/tmp/obc-boot-probe-eps.log 2>&1 &
EPS_PID=\$!
CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=56400 CSP_HUB_PUB_PORT=57400 "\${BIN_DIR}/adcs_simulator" --node-id 3 </dev/null >/tmp/obc-boot-probe-adcs.log 2>&1 &
ADCS_PID=\$!
"\${BIN_DIR}/radio_mock_server" --port 7000 </dev/null >/tmp/obc-boot-probe-radio.log 2>&1 &
RADIO_PID=\$!

cleanup() {
  kill "\${CSP_PID}" "\${EPS_PID}" "\${ADCS_PID}" "\${RADIO_PID}" >/dev/null 2>&1 || true
}
trap cleanup EXIT

sleep 1

printf 'target-rpi-boot-image-v1\n' > "\${STAGING_ROOT}/staged-image.bin"
IMAGE_SIZE=\$(wc -c < "\${STAGING_ROOT}/staged-image.bin" | tr -d ' ')
IMAGE_DIGEST=\$(sha256sum "\${STAGING_ROOT}/staged-image.bin" | awk '{print \$1}')
BOOT_TRUST_SIGNER_ID=$(printf '%q' "${BOOT_TRUST_SIGNER_ID}")
BOOT_TRUST_KEY_SLOT=$(printf '%q' "${BOOT_TRUST_KEY_SLOT}")
BOOT_TRUST_KEY_HEX=$(printf '%q' "${BOOT_TRUST_KEY_HEX}")

python3 - "\${STAGING_ROOT}/staged-image.bin.manifest-v1" \
  "staged-image.bin" \
  "\${IMAGE_SIZE}" \
  "\${IMAGE_DIGEST}" \
  "SLOT_B" \
  "1" \
  "\${BOOT_TRUST_SIGNER_ID}" \
  "\${BOOT_TRUST_KEY_SLOT}" \
  "\${BOOT_TRUST_KEY_HEX}" <<'PY'
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
) = sys.argv[1:]

payload = (
    "schema=boot_manifest_v1\n"
    f"image_path={image_name}\n"
    f"image_size={image_size}\n"
    f"image_digest_sha256={image_digest}\n"
    f"target_slot={target_slot}\n"
    "image_id=rpi-boot-probe-image\n"
    f"software_version={software_version}\n"
    f"signer_id={signer_id}\n"
    f"key_slot={key_slot}\n"
    "signature_algorithm=hmac-sha256\n"
)
signature = hmac.new(bytes.fromhex(key_hex), payload.encode("utf-8"), hashlib.sha256).hexdigest()
with open(path, "w", encoding="utf-8") as out:
    out.write(payload)
    out.write(f"signature={signature}\n")
PY

echo "IMAGE_SIZE=\${IMAGE_SIZE}"
echo "IMAGE_DIGEST=\${IMAGE_DIGEST}"

{
  sleep 2
  printf 'boot prepare %s %s\n' "\${IMAGE_SIZE}" "\${IMAGE_DIGEST}"
  sleep 1
  printf 'boot verify staged-image.bin\n'
  sleep 1
  printf 'boot activate\n'
  sleep 1
  printf 'quit\n'
} | CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=56400 CSP_HUB_PUB_PORT=57400 EPS_CSP_NODE_ID=2 ADCS_CSP_NODE_ID=3 "\${BIN_DIR}/OBC" \
  --comm tcp \
  --comm-host 127.0.0.1 \
  --comm-port 7000 \
  --radio-protocol $(printf '%q' "${RADIO_PROTOCOL}") \
  --runtime-root "\${RUNTIME_ROOT}" \
  --persistent-root "\${PERSISTENT_ROOT}" \
  --staging-root "\${STAGING_ROOT}" \
  --boot-trust hmac-sha256 \
  --boot-trust-signer-id "\${BOOT_TRUST_SIGNER_ID}" \
  --boot-trust-key-slot "\${BOOT_TRUST_KEY_SLOT}" \
  --boot-trust-key-hex "\${BOOT_TRUST_KEY_HEX}"

echo '--- METADATA AFTER ACTIVATE ---'
cat "\${METADATA_PATH}"

set +e
{
  sleep 2
  printf 'boot status\n'
  sleep 1
  printf 'boot rollback\n'
  sleep 1
  printf 'status\n'
  sleep 1
  printf 'quit\n'
} | CSP_HUB_HOST=127.0.0.1 CSP_HUB_SUB_PORT=56400 CSP_HUB_PUB_PORT=57400 EPS_CSP_NODE_ID=2 ADCS_CSP_NODE_ID=3 "\${BIN_DIR}/OBC" \
  --comm tcp \
  --comm-host 127.0.0.1 \
  --comm-port 7000 \
  --radio-protocol $(printf '%q' "${RADIO_PROTOCOL}") \
  --runtime-root "\${RUNTIME_ROOT}" \
  --persistent-root "\${PERSISTENT_ROOT}" \
  --staging-root "\${STAGING_ROOT}" \
  --boot-trust hmac-sha256 \
  --boot-trust-signer-id "\${BOOT_TRUST_SIGNER_ID}" \
  --boot-trust-key-slot "\${BOOT_TRUST_KEY_SLOT}" \
  --boot-trust-key-hex "\${BOOT_TRUST_KEY_HEX}"
SECOND_RUN_STATUS=\$?
set -e

if [[ "\${SECOND_RUN_STATUS}" -ne 0 ]]; then
  echo "NOTE: post-rollback OBC run exited with status \${SECOND_RUN_STATUS}; collecting metadata anyway"
fi

echo '--- METADATA AFTER ROLLBACK ---'
cat "\${METADATA_PATH}"
EOF
