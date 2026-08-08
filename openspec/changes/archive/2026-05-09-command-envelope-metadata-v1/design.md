## Context

Checked-in F Prime v4.1.0 documents `Svc::CmdDispatcher` as accepting encoded `Fw::Com` command buffers, decoding opcodes, dispatching commands, and returning command status through the same source/status port index while transferring the original context value as the `cmdSeq` argument. Checked-in `Svc::FprimeRouter` routes F Prime packet types and notes that current F Prime protocol does not use router context.

The project already places `CommandIngressAuthority` between `FprimeRouter.commandOut` and `CmdDispatcher.seqCmdBuff`. That makes it the narrow integration point for mission envelope unwrap because it can preserve F Prime status semantics while continuing to use the existing configured ingress source index for authority.

## Design

Envelope v1 is an outer `Fw.CmdPacket` using project pseudo-opcode `0x0BC10001U`. Its arg buffer contains:

| Field | Type | Value |
|---|---:|---|
| magic | `U32` | `0x0BC0DE01` |
| version | `U8` | `1` |
| flags | `U8` | `0` |
| headerLength | `U16` | `20` |
| sessionId | `U32` | caller supplied, observed only |
| sequenceNumber | `U32` | caller supplied, observed only |
| innerLength | `U16` | complete serialized inner `Fw.CmdPacket` length |
| reserved | `U16` | `0` |
| innerCmd | bytes | serialized F Prime command packet |

`CommandIngressAuthority` behavior:

- Non-envelope commands keep the existing authority path.
- Envelope commands are parsed before command catalog authority evaluation.
- Valid envelopes increment envelope telemetry, emit an observed event, and replace the outbound command buffer with the inner command buffer.
- Existing authority policy evaluates the inner opcode using only the config for the input port index.
- Forwarded and synthetic status preserve original `Fw.Com.context` unchanged.
- Malformed recognized envelopes are rejected before `CmdDispatcher` with exactly one synthetic `FORMAT_ERROR` response.
- Malformed envelope synthetic status uses a safely decoded inner opcode when available, otherwise `0xFFFFFFFF`.

## Risks And Boundaries

- Envelope metadata is not trusted source identity. Source identity remains configured by `CommandIngressAuthority` input port index.
- `session_id` and `sequence_number` are parsed and observed but do not affect runtime acceptance in this change.
- Duplicate, lower, or wraparound sequence values are not rejected until a later `command-session-sequence-v1` change wires the sequence helper into runtime.
- The hosted probe proves envelope metadata over the current hosted ingress port `0` only.
- File and unknown packet routing stay out of scope.
