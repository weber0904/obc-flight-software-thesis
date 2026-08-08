#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXAMPLE_PATH="${ROOT_DIR}/config/security/command-auth.example.ini"
LOCAL_PATH="${ROOT_DIR}/config/security/command-auth.ini"

if [[ ! -f "${EXAMPLE_PATH}" ]]; then
  echo "Missing public development example: ${EXAMPLE_PATH}" >&2
  exit 1
fi

if [[ -e "${LOCAL_PATH}" ]]; then
  echo "Local command-auth keystore already exists; leaving it unchanged: ${LOCAL_PATH}"
  exit 0
fi

install -m 0600 "${EXAMPLE_PATH}" "${LOCAL_PATH}"
echo "Created local hosted-development keystore: ${LOCAL_PATH}"
echo "This example is public and must not be used for target deployment."
