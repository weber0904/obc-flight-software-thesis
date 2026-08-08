## Context

The current active verification path already defaults to `OBC` and `TopCcsds`,
but its probe lifecycle management is inconsistent.

The failure mode repeats across several scripts:

- Python probes launch local `fprime-gds`, stack wrappers, or helper listeners
  via `subprocess.Popen(...)` without `start_new_session=True`
- cleanup often calls `process.terminate()` only on direct children
- macOS `fprime-gds` helpers can survive outside the parent process group
- shell launchers only run `jobs -p | xargs kill`, which cannot reach orphaned
  or reparented helpers after an interrupted wrapper session

The result is a class of rerun failures where active probes appear broken until
someone manually kills high-CPU Python leftovers.

## Design

### Python probe subprocess ownership

This change adds one repo-internal helper module under `scripts/` that owns the
active Python probe subprocess contract.

The helper SHALL provide:

- `start_new_session=True` launches for local subprocesses
- logged command startup so probe logs still show exact commands used
- reverse-order `SIGTERM -> wait -> SIGKILL` cleanup against the full process
  group
- narrowly-scoped stale helper reaping based on exact ownership fragments

The stale-process sweep SHALL inspect `ps` output and kill only processes whose
command lines match both:

1. an allowed helper family such as:
   - `fprime_gds.executables.comm`
   - `fprime_gds.executables.tcpserver`
   - `CustomDataHandlers`
   - probe-owned wrapper commands when the caller explicitly asks for them
2. concrete ownership fragments for the current probe run, such as:
   - `--ip-port <port>`
   - `--tts-port <port>`
   - `--file-storage-directory <path>`
   - the probe temp root
   - the runtime root
   - the exact stack script path

This keeps the reap logic narrow enough to avoid killing unrelated user
processes while still cleaning up prior interrupted runs that reused the same
owned roots or ports.

### Active Python probe refactor boundary

The active Python probes touched by this wave SHALL all use the same helper
contract for local subprocesses:

- start each local `fprime-gds`, listener, stack script, or auxiliary child in
  its own managed process group
- reap owned stale helpers before startup
- clean up managed processes in reverse order
- run a final stale-helper sweep during shutdown so macOS helper children do
  not survive the parent wrapper exit

The acceptance boundary is limited to the governed active path:

- command envelope/auth/session/freshness probes
- the active Raspberry Pi command-persistence probe
- the selected CCSDS mode/TT&C hosted probes in this plan

Legacy-only probes remain out of scope unless they are explicitly reused by one
of the active verification surfaces above.

### Shell launcher hardening

Active shell launchers keep their current direct-child background launch style,
but gain targeted stale-process sweeps in `scripts/_common.sh`.

The shell helper layer SHALL:

- sweep owned stale processes before startup
- sweep again during trap-based cleanup
- match only narrowly-scoped command fragments derived from the current launch
  parameters
- avoid depending on `setsid`, because the local macOS path cannot assume it

`run_dev_stack.sh` SHALL sweep exact owned stack processes using command
fragments already visible on the process command line, including runtime roots
and explicit ports. `run_remote_csp_gds_stack.sh` SHALL additionally sweep
owned `fprime-gds` helper children keyed by its GDS ports and file-storage
directory.

`run_rpi_stack.sh` and `run_rpi_installed_stack.sh` SHALL propagate cleanup
ownership tokens into the remote environment so the remote `run_dev_stack.sh`
can self-heal the same runtime root on rerun after interruption.

### Verification boundary

This change adds a repository-owned cleanup-hardening verification script that
proves three bounded scenarios:

1. an active hosted probe is interrupted mid-run, leaves no owned stale
   `fprime-gds` helpers behind, and can be rerun immediately
2. the active Raspberry Pi command-persistence probe is interrupted mid-run,
   leaves no owned local helper leftovers behind, and can be rerun immediately
3. `run_remote_csp_gds_stack.sh` is interrupted and relaunched without manual
   cleanup or owned stale helper leftovers

The evidence claim is rerun safety for governed active verification scripts. It
does not claim generic workstation process hygiene, system-wide orphan
prevention, or legacy-path closure.

## Risks And Boundaries

- Matching must stay narrow; a broad `pkill -f python` or `pkill -f fprime-gds`
  style sweep would be unsafe and is explicitly out of scope.
- Remote cleanup on Raspberry Pi is bounded to the governed runtime root and
  stack-launch command fragments passed by the active launcher.
- The change hardens process lifecycle management only; it does not change any
  flight behavior or verification-path semantics beyond making reruns reliable.
