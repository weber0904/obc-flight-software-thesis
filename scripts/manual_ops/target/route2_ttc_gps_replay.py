#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shlex
import subprocess
import sys
import time


DEFAULT_TARGET = "operator@obc.local"
DEFAULT_SERVICE = "obc-comm-csp-stack.service"
DEFAULT_DROPIN_NAME = "59-chapter5-ttc-gps-replay.conf"
DEFAULT_REMOTE_REPLAY = "/tmp/chapter5-route2-ttc-replay.nmea"


def shq(value: str) -> str:
    return shlex.quote(value)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Apply or remove the Route 2 target GPS replay override used by the TTC entry demo."
    )
    parser.add_argument(
        "--obc-target",
        default=DEFAULT_TARGET,
        help=f"SSH target for the OBC host. Default: {DEFAULT_TARGET}",
    )
    parser.add_argument(
        "--service",
        default=DEFAULT_SERVICE,
        help=f"Systemd service to restart after override changes. Default: {DEFAULT_SERVICE}",
    )
    parser.add_argument(
        "--dropin-name",
        default=DEFAULT_DROPIN_NAME,
        help=f"Override drop-in filename. Default: {DEFAULT_DROPIN_NAME}",
    )
    parser.add_argument(
        "--remote-replay",
        default=DEFAULT_REMOTE_REPLAY,
        help=f"Remote replay file path. Default: {DEFAULT_REMOTE_REPLAY}",
    )
    parser.add_argument(
        "--timeout-sec",
        type=float,
        default=120.0,
        help="Timeout for waiting on service active state.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    apply_parser = subparsers.add_parser("apply", help="Stage replay sentences and enable replay mode override.")
    apply_parser.add_argument(
        "--duration-sec",
        type=int,
        default=180,
        help="Replay duration in seconds. Default: 180",
    )
    apply_parser.add_argument(
        "--start-offset-sec",
        type=int,
        default=2,
        help="Seconds from now to start the replay timeline. Default: 2",
    )

    subparsers.add_parser("remove", help="Remove the replay override and restart the OBC service.")
    return parser


def run_ssh(target: str, command: str, *, input_text: str | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["ssh", target, command],
        input=input_text,
        text=True,
        capture_output=True,
        check=check,
    )


def wait_service_active(target: str, service: str, timeout_sec: float) -> None:
    deadline = time.time() + timeout_sec
    while time.time() < deadline:
        result = run_ssh(target, f"systemctl is-active {shq(service)}", check=False)
        if result.returncode == 0 and result.stdout.strip() == "active":
            return
        time.sleep(1.0)
    raise RuntimeError(f"timed out waiting for {service} to become active on {target}")


def make_sentence(payload: str) -> str:
    checksum = 0
    for ch in payload:
        checksum ^= ord(ch)
    return f"${payload}*{checksum:02X}"


def build_replay_nmea(*, duration_sec: int, start_offset_sec: int) -> str:
    remote_epoch = int(time.time()) + start_offset_sec
    lines: list[str] = []
    for step in range(duration_sec):
        moment = time.gmtime(remote_epoch + step)
        hhmmss = time.strftime("%H%M%S", moment)
        ddmmyy = time.strftime("%d%m%y", moment)
        payload = f"GPRMC,{hhmmss}.00,A,2503.7135,N,12133.5335,E,0.0,0.0,{ddmmyy},0.0,E"
        lines.append(make_sentence(payload))
    return "\n".join(lines) + "\n"


def apply_override(args: argparse.Namespace) -> None:
    replay_payload = build_replay_nmea(duration_sec=args.duration_sec, start_offset_sec=args.start_offset_sec)
    dropin_dir = f"/etc/systemd/system/{args.service}.d"
    dropin_path = f"{dropin_dir}/{args.dropin_name}"
    dropin_body = (
        "[Service]\n"
        "Environment=OBC_GPS_SOURCE_MODE=replay\n"
        f"Environment=OBC_GPS_REPLAY_FILE={args.remote_replay}\n"
    )

    run_ssh(args.obc_target, f"cat > {shq(args.remote_replay)}", input_text=replay_payload)
    run_ssh(args.obc_target, f"sudo -n mkdir -p {shq(dropin_dir)}")
    run_ssh(args.obc_target, f"sudo -n tee {shq(dropin_path)} >/dev/null", input_text=dropin_body)
    run_ssh(args.obc_target, "sudo -n systemctl daemon-reload")
    run_ssh(args.obc_target, f"sudo -n systemctl restart {shq(args.service)}")
    wait_service_active(args.obc_target, args.service, args.timeout_sec)
    print(f"OK applied gps replay override on {args.obc_target}")
    print(f"replay_file={args.remote_replay}")
    print(f"dropin={dropin_path}")


def remove_override(args: argparse.Namespace) -> None:
    dropin_dir = f"/etc/systemd/system/{args.service}.d"
    dropin_path = f"{dropin_dir}/{args.dropin_name}"
    run_ssh(
        args.obc_target,
        f"sudo -n rm -f {shq(dropin_path)} && rm -f {shq(args.remote_replay)}",
    )
    run_ssh(args.obc_target, "sudo -n systemctl daemon-reload")
    run_ssh(args.obc_target, f"sudo -n systemctl restart {shq(args.service)}")
    wait_service_active(args.obc_target, args.service, args.timeout_sec)
    print(f"OK removed gps replay override on {args.obc_target}")
    print(f"dropin={dropin_path}")


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    try:
        if args.command == "apply":
            apply_override(args)
        elif args.command == "remove":
            remove_override(args)
        else:
            parser.error(f"unsupported command {args.command}")
    except subprocess.CalledProcessError as exc:
        sys.stderr.write(
            f"SSH command failed (rc={exc.returncode})\nstdout={exc.stdout}\nstderr={exc.stderr}\n"
        )
        return 1
    except Exception as exc:
        sys.stderr.write(f"{exc}\n")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
