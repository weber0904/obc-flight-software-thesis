from __future__ import annotations

import json
import pathlib
import re
import subprocess
import uuid
from dataclasses import dataclass
from typing import Any

from manual_ops.lib.common import ensure_dir, find_local_tool, utc_timestamp


def _slugify(value: str) -> str:
    text = re.sub(r"[^A-Za-z0-9._-]+", "-", value.strip()).strip("-")
    return text or "draft"


def _quote_seq_arg(value: str) -> str:
    text = str(value)
    if not text:
        return '""'
    if any(character.isspace() for character in text) or any(character in text for character in ['"', ";"]):
        return json.dumps(text)
    return text


def _resolved_draft_id(draft_id: str | None) -> str:
    if not draft_id:
        return f"draft-{uuid.uuid4().hex[:12]}"
    sanitized = re.sub(r"[^A-Za-z0-9_-]+", "-", str(draft_id).strip()).strip("-")
    return sanitized or f"draft-{uuid.uuid4().hex[:12]}"


def _find_seqgen_tool() -> str | None:
    try:
        return find_local_tool("fprime-seqgen")
    except RuntimeError:
        return None


def render_sequence_source(steps: list[dict[str, Any]]) -> str:
    lines = ["; Mission Console generated sequence"]
    for step in steps:
        offset = str(step.get("offset") or "R00:00:00")
        command_name = str(step.get("commandName") or "").strip()
        command_args = [_quote_seq_arg(str(value)) for value in step.get("commandArgs", [])]
        if not command_name:
            continue
        line = " ".join(part for part in [offset, command_name, *command_args] if part)
        lines.append(line)
    return "\n".join(lines).rstrip() + "\n"


@dataclass(frozen=True)
class SequenceDraftRecord:
    draft_id: str
    context_id: str
    title: str
    steps: list[dict[str, Any]]
    raw_source: str
    updated_at: str
    source_path: str | None = None
    compiled_path: str | None = None
    latest_compile_result: dict[str, Any] | None = None

    def to_json(self) -> dict[str, Any]:
        return {
            "draftId": self.draft_id,
            "contextId": self.context_id,
            "title": self.title,
            "steps": self.steps,
            "rawSource": self.raw_source,
            "updatedAt": self.updated_at,
            "sourcePath": self.source_path,
            "compiledPath": self.compiled_path,
            "latestCompileResult": self.latest_compile_result,
        }


class SequenceAuthoringService:
    def __init__(self, runtime_root: pathlib.Path) -> None:
        self.runtime_root = ensure_dir(runtime_root)
        self.drafts_root = ensure_dir(self.runtime_root / "sequence-drafts")
        self.seqgen_path: str | None = None

    def list_drafts(self, context_id: str) -> list[dict[str, Any]]:
        records: list[dict[str, Any]] = []
        for metadata_path in sorted(self.drafts_root.glob("*.json")):
            try:
                payload = json.loads(metadata_path.read_text(encoding="utf-8"))
            except Exception:
                continue
            if not isinstance(payload, dict):
                continue
            if payload.get("contextId") != context_id:
                continue
            records.append(payload)
        return sorted(records, key=lambda entry: entry.get("updatedAt", ""), reverse=True)

    def save_draft(
        self,
        *,
        context_id: str,
        title: str | None,
        steps: list[dict[str, Any]] | None,
        raw_source: str | None,
        draft_id: str | None = None,
    ) -> dict[str, Any]:
        normalized_steps = list(steps or [])
        normalized_title = str(title or "Untitled Sequence").strip() or "Untitled Sequence"
        source_text = str(raw_source or "").strip()
        if not source_text:
            source_text = render_sequence_source(normalized_steps)
        resolved_draft_id = _resolved_draft_id(draft_id)
        record = SequenceDraftRecord(
            draft_id=resolved_draft_id,
            context_id=context_id,
            title=normalized_title,
            steps=normalized_steps,
            raw_source=source_text,
            updated_at=utc_timestamp(),
        )
        self._write_metadata(record)
        return record.to_json()

    def compile_source(
        self,
        *,
        context_id: str,
        dictionary_path: pathlib.Path,
        title: str | None,
        steps: list[dict[str, Any]] | None,
        raw_source: str | None,
        draft_id: str | None = None,
    ) -> dict[str, Any]:
        normalized_steps = list(steps or [])
        normalized_title = str(title or "Untitled Sequence").strip() or "Untitled Sequence"
        source_text = str(raw_source or "").strip()
        if not source_text:
            source_text = render_sequence_source(normalized_steps)
        resolved_draft_id = _resolved_draft_id(draft_id)
        slug = _slugify(normalized_title)
        draft_dir = ensure_dir(self.drafts_root / resolved_draft_id)
        source_path = draft_dir / f"{slug}.seq"
        output_path = draft_dir / f"{slug}.bin"
        source_path.write_text(source_text, encoding="utf-8")
        if not self.seqgen_path:
            self.seqgen_path = _find_seqgen_tool()
        if not self.seqgen_path:
            result = {
                "draftId": resolved_draft_id,
                "contextId": context_id,
                "title": normalized_title,
                "sourceText": source_text,
                "sourcePath": str(source_path),
                "compiledPath": None,
                "success": False,
                "returnCode": -1,
                "diagnostics": "fprime-seqgen compiler tool not found in path.",
                "dictionaryPath": str(dictionary_path),
                "compiledAt": utc_timestamp(),
            }
            record = SequenceDraftRecord(
                draft_id=resolved_draft_id,
                context_id=context_id,
                title=normalized_title,
                steps=normalized_steps,
                raw_source=source_text,
                updated_at=utc_timestamp(),
                source_path=str(source_path),
                compiled_path=None,
                latest_compile_result=result,
            )
            self._write_metadata(record)
            return result
        args = [
            self.seqgen_path,
            "--dictionary",
            str(dictionary_path),
            str(source_path),
            str(output_path),
        ]
        try:
            completed = subprocess.run(
                args,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                check=False,
            )
            success = completed.returncode == 0 and output_path.exists()
            return_code = completed.returncode
            diagnostics = completed.stdout or ""
        except Exception as error:
            success = False
            return_code = -1
            diagnostics = f"Compilation failed to execute: {error}"
        result = {
            "draftId": resolved_draft_id,
            "contextId": context_id,
            "title": normalized_title,
            "sourceText": source_text,
            "sourcePath": str(source_path),
            "compiledPath": str(output_path) if output_path.exists() else None,
            "success": success,
            "returnCode": return_code,
            "diagnostics": diagnostics,
            "dictionaryPath": str(dictionary_path),
            "compiledAt": utc_timestamp(),
        }
        record = SequenceDraftRecord(
            draft_id=resolved_draft_id,
            context_id=context_id,
            title=normalized_title,
            steps=normalized_steps,
            raw_source=source_text,
            updated_at=utc_timestamp(),
            source_path=str(source_path),
            compiled_path=str(output_path) if output_path.exists() else None,
            latest_compile_result=result,
        )
        self._write_metadata(record)
        return result

    def _write_metadata(self, record: SequenceDraftRecord) -> None:
        draft_dir = ensure_dir(self.drafts_root / record.draft_id)
        metadata_path = self.drafts_root / f"{record.draft_id}.json"
        metadata_path.write_text(json.dumps(record.to_json(), indent=2, sort_keys=True) + "\n", encoding="utf-8")
        if record.source_path:
            ensure_dir(pathlib.Path(record.source_path).parent)
            pathlib.Path(record.source_path).write_text(record.raw_source, encoding="utf-8")
        else:
            source_path = draft_dir / f"{_slugify(record.title)}.seq"
            source_path.write_text(record.raw_source, encoding="utf-8")
