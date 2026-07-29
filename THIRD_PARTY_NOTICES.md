# Third-Party Notices

This repository combines original OBC flight-software work with the following
open-source dependencies. The root Apache-2.0 license does not replace their
licenses.

| Dependency | Public source and pinned revision | License | Distribution notes |
|---|---|---|---|
| F Prime | `https://github.com/weber0904/fprime.git` at `54f02168c676d5b61990d7a48ea9c61b9a8d0b5f` | Apache-2.0 | Fork based on F Prime v4.1.0. The fork preserves `LICENSE.txt` and `NOTICE.txt`; files changed by this project carry modification notices. |
| libcsp / csp-es | `https://github.com/weber0904/csp-es.git` at `241b756a7fb5af1ba0967183b4a2b5843f77ebdb` | MIT | The dependency license is preserved at `lib/libcsp/LICENSE`. |
| TooJPEG | `OBC/Components/PayloadOpsController/third_party/toojpeg/` | zlib-style | Original author and distribution terms are preserved with the vendored source. |

## TooJPEG Integrity

- `toojpeg.cpp`: `d41b7f8469f1dd0341165294affafe138e2c6cb2ad5d15c1db3d4bb420966603`
- `toojpeg.h`: `cf815d4ccc6a827c9de83d1151c1140d86e5888dfc5fb2c177a16465fcd983e3`
- `LICENSE`: `e9e241fca73ec30e84b5538d5901584733fec60204ed13b9fd2a5db2925acbed`

Python packages declared by `requirements.txt`, build tools, and GitHub Actions
are development/runtime dependencies rather than relicensed project source.
Their resolved inventory is recorded in `SBOM.spdx.json`.
