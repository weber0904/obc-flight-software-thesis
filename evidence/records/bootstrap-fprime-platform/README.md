# Bootstrap F' Platform Evidence

## Scope

This record captures the repository bootstrap results for the `bootstrap-fprime-platform` OpenSpec change.

## Environment

- Date: `2026-03-20`
- Workspace: `$REPO_ROOT`
- Framework baseline: `lib/fprime` pinned to `v4.1.0`
- Verified virtual environment interpreter: `python3.13`

## Commands Run

1. `fprime-bootstrap project --populate --path . --tag v4.1.0`
2. `git submodule add --depth 1 https://github.com/nasa/fprime.git lib/fprime`
3. `git -C lib/fprime checkout v4.1.0`
4. `python3.13 -m venv fprime-venv`
5. `fprime-venv/bin/pip install --no-cache-dir -Ur requirements.txt`
6. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util generate -f`
7. `PATH=$REPO_ROOT/fprime-venv/bin:$PATH $REPO_ROOT/fprime-venv/bin/fprime-util build`

## Results

- The repository contains the generated root F' scaffold files: `CMakeLists.txt`, `CMakePresets.json`, `settings.ini`, `requirements.txt`, and `README.md`.
- The project-local module path `lib/fprime/` is present as a git submodule and is pinned to `v4.1.0`.
- The baseline working directories `OBC/`, `simulators/`, `scripts/`, `docs/`, and `.github/` exist in the governed workspace.
- `fprime-util generate -f` completed successfully from `fprime-venv/`.
- `fprime-util build` completed successfully from `fprime-venv/`.

## Bootstrap-Specific Notes

- The initial `fprime-bootstrap project --populate ...` run partially generated the project tree but failed on the final template rename because the target `OBC/` directory already existed in this non-empty governed workspace. The generated root files were preserved and reconciled in place.
- A `python3.14` virtual environment was not viable for the `v4.1.0` dependency set because `pydantic_core` could not build against PyO3's supported range. Recreating `fprime-venv/` with `python3.13` resolved the issue.
- During `fprime-util build`, the generated version step reported `fatal: bad revision 'HEAD'` because the repository does not yet have an initial commit. The build still completed successfully, and `build-fprime-automatic-native/versions/version.json` records `"framework_version": "v4.1.0"`.
- `syft` is not installed on the host, so software bill-of-material generation was skipped during `generate`.

## Follow-Up

- Keep future bootstrap and CI jobs on a Python interpreter compatible with the F' `v4.1.0` dependency set.
- Expect the `HEAD` warning to disappear once the repository has its first commit.
