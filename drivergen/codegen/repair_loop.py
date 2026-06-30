"""Fail-fast repair loop for driver synthesis verification."""
from __future__ import annotations

import ast
import dataclasses
import json
import logging
import operator as _op
import re
import time
from fractions import Fraction
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

from ..llm.providers import BaseProvider
from .adapter_generator import (
    AdapterContractError,
    GeneratedAdapter,
    generate_adapter,
)
from .channel_alias import build_or_load_channel_alias_map
from .classify_device import (
    ClassifyResult,
    EVAL_CLASS_DISPLAY,
    EVAL_CLASS_MEMORY,
    EVAL_CLASS_MULTI_CHANNEL,
    EVAL_CLASS_RTC,
    EVAL_CLASS_SINGLE_CHANNEL,
)
from .consistency_check import ConsistencyReport, _extract_bytes, check_consistency
from .diagnosis_renderer import compose_diagnosis
from .invariants_tracker import InvariantsTracker
from .ir_to_expected_transactions import (
    ExpectedTransaction,
    _literal_prefix_from_transaction,
    _select_codegen_operation_flows,
)
from .merge_test_plan import CoverageReport, check_bundle_coverage
from .repair_context_selector import build_repair_context
from .output_semantics import (
    build_or_load_output_semantics_map,
    primary_semantic_kind,
    semantic_kind_is_physical,
)
from .route import RoutingResult, SPI_SUB_COMMAND, SPI_SUB_STREAM
from .spi_protocol import spi_protocol_hints
from .runtime_probe import (
    ProbeError,
    ProbeMeta,
    ProbeOutcome,
    ProbeStimulus,
    build_probe_meta,
    build_probe_stimuli,
    check_probe_expectations,
    probe_all_stimuli,
)
from .rtos_surface import (
    forbidden_surface_usage_errors,
    sanitize_rtos_contract_for_codegen,
    signature_config_pointer_args,
    stub_headers_declaring_symbol,
    struct_field_validation_errors,
)
from .stub_compiler import StubCompileResult, available_stub_headers, stub_compile
from .code_generator import (
    DriverCodeBundle,
    PlanBundle,
    SynthesisBundle,
    SynthesisError,
    generate_contract_test_plan,
    generate_driver_code,
    generate_synthesis,
)

# Keep compile imports limited to runtime evaluation helpers.
from evaluation.runtime.compile import CompileResult, link_mode_compile

logger = logging.getLogger(__name__)


# Layer tags (string constants for observability & JSON)

LAYER_SYNTHESIS = "synthesis"
LAYER_SYNTAX = "syntax"
LAYER_SELF_CHECK = "self_check"       # advisory only
LAYER_LINK = "link"
LAYER_PROBE = "probe"
LAYER_NONE = ""                        # success: no layer failed

ALL_LAYERS: Tuple[str, ...] = (
    LAYER_SYNTHESIS, LAYER_SYNTAX, LAYER_SELF_CHECK, LAYER_LINK, LAYER_PROBE,
)

# Ordered severity for "best attempt" scoring when loop exhausts.
# Higher score = closer to success.
_LAYER_SCORE = {
    LAYER_SYNTHESIS: 0,   # didn't even get a bundle
    LAYER_SYNTAX:    1,   # got bundle but driver doesn't compile
    LAYER_LINK:      2,   # driver compiles but adapter/harness don't link
    LAYER_PROBE:     3,   # linked but runtime probe rejects behaviour
    LAYER_NONE:      4,   # success (layer_failed == LAYER_NONE)
}


# Result dataclasses

@dataclasses.dataclass(frozen=True)
class AttemptRecord:
    """Immutable record of one repair attempt."""
    attempt: int
    layer_failed: str                              # "" = success
    success: bool
    total_time_s: float

    synthesis_error: Optional[SynthesisError] = None
    bundle: Optional[SynthesisBundle] = None

    syntax_result: Optional[StubCompileResult] = None
    consistency_report: Optional[ConsistencyReport] = None
    coverage_report: Optional[CoverageReport] = None
    adapter: Optional[GeneratedAdapter] = None
    adapter_error: Optional[str] = None            # AdapterContractError msg
    link_result: Optional[CompileResult] = None
    probe_outcomes: Tuple[ProbeOutcome, ...] = ()
    probe_stimuli: Tuple[ProbeStimulus, ...] = ()

    feedback_for_next: str = ""                    # diagnosis for next prompt
    repair_context_for_next: Optional[Mapping[str, Any]] = None

    def to_dict(self) -> dict:
        """JSON-safe projection (skips raw response bodies and ELF paths)."""
        d: dict = {
            "attempt": self.attempt,
            "layer_failed": self.layer_failed,
            "success": self.success,
            "total_time_s": round(self.total_time_s, 3),
        }
        if self.synthesis_error is not None:
            d["synthesis_error"] = {
                "source": self.synthesis_error.source,
                "message": str(self.synthesis_error),
                "errors": list(self.synthesis_error.errors),
                "raw_response_len": (
                    len(self.synthesis_error.raw_response)
                    if self.synthesis_error.raw_response else 0
                ),
            }
        if self.bundle is not None:
            d["bundle_summary"] = {
                "device_id": self.bundle.device_id,
                "rtos_id": self.bundle.rtos_id,
                "eval_class": self.bundle.eval_class,
                "bus_kind": self.bundle.bus_kind,
                "header_chars": len(self.bundle.driver_header),
                "source_chars": len(self.bundle.driver_source),
                "plan_hash": self.bundle.plan_hash,
                "api_contract_keys": sorted(self.bundle.api_contract.keys()),
                "stimuli_count": len(
                    self.bundle.test_plan.get("test_stimuli", []) or []
                ),
                "model": self.bundle.model,
                "generation_time_s": self.bundle.generation_time_s,
            }
        if self.syntax_result is not None:
            d["syntax"] = {
                "success": self.syntax_result.success,
                "errors": len(self.syntax_result.errors),
                "warnings": len(self.syntax_result.warnings),
            }
        if self.consistency_report is not None:
            d["consistency"] = self.consistency_report.to_dict()
        if self.coverage_report is not None:
            d["coverage"] = self.coverage_report.to_dict()
        if self.adapter_error is not None:
            d["adapter_error"] = self.adapter_error
        if self.adapter is not None:
            d["adapter_summary"] = {
                "device_id": self.adapter.device_id,
                "eval_class": self.adapter.eval_class,
                "bus_kind": self.adapter.bus_kind,
                "source_chars": len(self.adapter.source_c),
                "warnings": list(self.adapter.warnings),
            }
        if self.link_result is not None:
            d["link"] = {
                "success": self.link_result.success,
                "errors": len(self.link_result.errors),
                "warnings": len(self.link_result.warnings),
                "elf_path": (
                    str(self.link_result.elf_path)
                    if self.link_result.elf_path else None
                ),
                "text_bytes": self.link_result.text_bytes,
                "data_bytes": self.link_result.data_bytes,
                "bss_bytes": self.link_result.bss_bytes,
            }
        if self.probe_outcomes:
            stim_map = {s.name: s for s in self.probe_stimuli}
            d["probe"] = {
                "total": len(self.probe_outcomes),
                "passed": sum(
                    1 for o in self.probe_outcomes
                    if _probe_outcome_passed(o, stim_map.get(o.stimulus_name))
                ),
                "expectation_failures": sum(
                    1 for o in self.probe_outcomes
                    if not check_probe_expectations(
                        o, stim_map.get(o.stimulus_name)
                    ).ok
                ),
                "outcomes": [o.to_dict() for o in self.probe_outcomes],
            }
        if self.feedback_for_next:
            d["feedback_chars"] = len(self.feedback_for_next)
        if self.repair_context_for_next:
            d["repair_context_for_next"] = dict(self.repair_context_for_next)
        return d


@dataclasses.dataclass(frozen=True)
class PlanAttemptRecord:
    """Immutable record of one split planner attempt."""
    attempt: int
    success: bool
    total_time_s: float

    plan_bundle: Optional[PlanBundle] = None
    synthesis_error: Optional[SynthesisError] = None
    consistency_report: Optional[ConsistencyReport] = None
    coverage_report: Optional[CoverageReport] = None
    validation_errors: Tuple[str, ...] = ()
    feedback_for_next: str = ""

    def to_dict(self) -> dict:
        d: dict = {
            "attempt": self.attempt,
            "success": self.success,
            "total_time_s": round(self.total_time_s, 3),
        }
        if self.plan_bundle is not None:
            d["plan_summary"] = {
                "device_id": self.plan_bundle.device_id,
                "rtos_id": self.plan_bundle.rtos_id,
                "eval_class": self.plan_bundle.eval_class,
                "bus_kind": self.plan_bundle.bus_kind,
                "plan_hash": self.plan_bundle.plan_hash,
                "api_contract_keys": sorted(
                    self.plan_bundle.api_contract.keys()
                ),
                "stimuli_count": len(
                    self.plan_bundle.test_plan.get("test_stimuli", []) or []
                ),
                "model": self.plan_bundle.model,
                "generation_time_s": self.plan_bundle.generation_time_s,
            }
        if self.synthesis_error is not None:
            d["synthesis_error"] = {
                "source": self.synthesis_error.source,
                "message": str(self.synthesis_error),
                "errors": list(self.synthesis_error.errors),
                "raw_response_len": (
                    len(self.synthesis_error.raw_response)
                    if self.synthesis_error.raw_response else 0
                ),
            }
        if self.consistency_report is not None:
            d["consistency"] = self.consistency_report.to_dict()
        if self.coverage_report is not None:
            d["coverage"] = self.coverage_report.to_dict()
        if self.validation_errors:
            d["validation_errors"] = list(self.validation_errors)
        if self.feedback_for_next:
            d["feedback_chars"] = len(self.feedback_for_next)
        return d


@dataclasses.dataclass
class RepairLoopResult:
    """Aggregate outcome of ``run_repair_loop``."""
    success: bool
    final_attempt: int                             # 1-based attempt index
    total_time_s: float

    device_id: str
    rtos_id: str
    eval_class: str
    bus_kind: str

    plan_bundle: Optional[PlanBundle] = None
    final_bundle: Optional[SynthesisBundle] = None
    final_adapter: Optional[GeneratedAdapter] = None
    final_elf_path: Optional[Path] = None

    plan_attempts: List[PlanAttemptRecord] = dataclasses.field(default_factory=list)
    attempts: List[AttemptRecord] = dataclasses.field(default_factory=list)
    invariants: Tuple = dataclasses.field(default_factory=tuple)   # Invariant
    layer_failed: str = ""                         # last attempt's layer_failed

    def to_dict(self) -> dict:
        """JSON-safe aggregate view."""
        plan_summary = None
        if self.plan_bundle is not None:
            plan_summary = {
                "plan_hash": self.plan_bundle.plan_hash,
                "api_contract_keys": sorted(
                    self.plan_bundle.api_contract.keys()
                ),
                "stimuli_count": len(
                    self.plan_bundle.test_plan.get("test_stimuli", []) or []
                ),
            }
        return {
            "split_plan_enabled": True,
            "success": self.success,
            "final_attempt": self.final_attempt,
            "total_time_s": round(self.total_time_s, 3),
            "device_id": self.device_id,
            "rtos_id": self.rtos_id,
            "eval_class": self.eval_class,
            "bus_kind": self.bus_kind,
            "layer_failed": self.layer_failed,
            "plan_summary": plan_summary,
            "final_elf_path": (
                str(self.final_elf_path) if self.final_elf_path else None
            ),
            "plan_attempts": [a.to_dict() for a in self.plan_attempts],
            "attempts": [a.to_dict() for a in self.attempts],
            "invariants": [inv.to_dict() for inv in self.invariants],
        }


# Per-layer helpers

def _run_synthesis(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction],
    artifact: Optional[Mapping[str, Any]],
    channel_alias_map: Optional[Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
    prior_feedback: Optional[str],
    attempt: int,
    prefer_json_mode: bool,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> Tuple[Optional[SynthesisBundle], Optional[SynthesisError]]:
    """Call synthesis once."""
    try:
        bundle = generate_synthesis(
            provider,
            device_ir,
            rtos_contract,
            classify_result=classify_result,
            routing=routing,
            expected_transactions=expected_transactions,
            artifact=artifact,
            channel_alias_map=channel_alias_map,
            output_semantics_map=output_semantics_map,
            prior_feedback=prior_feedback,
            attempt=attempt,
            prefer_json_mode=prefer_json_mode,
            extra_metadata=extra_metadata,
        )
        return bundle, None
    except SynthesisError as e:
        logger.warning(
            "Synthesis failed (attempt %d, source=%s): %s",
            attempt, e.source, e,
        )
        return None, e


def _run_contract_test_plan(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction],
    artifact: Optional[Mapping[str, Any]],
    channel_alias_map: Optional[Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
    prior_feedback: Optional[str],
    attempt: int,
    prefer_json_mode: bool,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> Tuple[Optional[PlanBundle], Optional[SynthesisError]]:
    """_run_contract_test_plan helper."""
    try:
        plan = generate_contract_test_plan(
            provider,
            device_ir,
            rtos_contract,
            classify_result=classify_result,
            routing=routing,
            expected_transactions=expected_transactions,
            artifact=artifact,
            channel_alias_map=channel_alias_map,
            output_semantics_map=output_semantics_map,
            prior_feedback=prior_feedback,
            attempt=attempt,
            prefer_json_mode=prefer_json_mode,
            extra_metadata=extra_metadata,
        )
        return plan, None
    except SynthesisError as e:
        logger.warning(
            "Contract/test-plan generation failed (attempt %d, source=%s): %s",
            attempt, e.source, e,
        )
        return None, e


def _run_driver_code(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    plan_bundle: PlanBundle,
    expected_transactions: Sequence[ExpectedTransaction],
    artifact: Optional[Mapping[str, Any]],
    channel_alias_map: Optional[Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
    prior_feedback: Optional[str],
    repair_context: Optional[Mapping[str, Any]],
    attempt: int,
    prefer_json_mode: bool,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> Tuple[Optional[DriverCodeBundle], Optional[SynthesisError]]:
    """_run_driver_code helper."""
    frozen_plan = {
        "api_contract": dict(plan_bundle.api_contract),
        "test_plan": dict(plan_bundle.test_plan),
        "plan_hash": plan_bundle.plan_hash,
    }
    try:
        code = generate_driver_code(
            provider,
            device_ir,
            rtos_contract,
            classify_result=classify_result,
            routing=routing,
            frozen_plan=frozen_plan,
            plan_hash=plan_bundle.plan_hash,
            expected_transactions=expected_transactions,
            artifact=artifact,
            channel_alias_map=channel_alias_map,
            output_semantics_map=output_semantics_map,
            prior_feedback=prior_feedback,
            repair_context=repair_context,
            attempt=attempt,
            prefer_json_mode=prefer_json_mode,
            extra_metadata=extra_metadata,
        )
        return code, None
    except SynthesisError as e:
        logger.warning(
            "Driver-code generation failed (attempt %d, source=%s): %s",
            attempt, e.source, e,
        )
        return None, e


def _assemble_synthesis_bundle(
    plan_bundle: PlanBundle,
    code_bundle: DriverCodeBundle,
    routing: RoutingResult,
) -> SynthesisBundle:
    """Combine frozen plan and code-only output for verification."""
    return SynthesisBundle(
        device_id=code_bundle.device_id,
        rtos_id=code_bundle.rtos_id,
        eval_class=code_bundle.eval_class,
        bus_kind=code_bundle.bus_kind,
        driver_header=code_bundle.driver_header,
        driver_source=code_bundle.driver_source,
        api_contract=dict(plan_bundle.api_contract),
        test_plan=dict(plan_bundle.test_plan),
        runtime_path=routing.runtime_path,
        slave_kind=routing.slave_kind,
        spi_sub_mode=routing.spi_sub_mode,
        attempt=code_bundle.attempt,
        model=code_bundle.model,
        provider_name=code_bundle.provider_name,
        generation_time_s=code_bundle.generation_time_s,
        prompt_chars=code_bundle.prompt_chars,
        raw_response=code_bundle.raw_response,
        plan_hash=plan_bundle.plan_hash,
    )


def _run_syntax_check(
    bundle: SynthesisBundle,
    *,
    timeout: int = 30,
) -> StubCompileResult:
    """Compile driver .h/.c standalone without linking."""
    return stub_compile(
        header_text=bundle.driver_header,
        source_text=bundle.driver_source,
        sample_text="",
        device_id=bundle.device_id,
        rtos_id=bundle.rtos_id,
        compile_level="syntax",
        timeout=timeout,
    )


def _run_self_check(
    bundle: SynthesisBundle,
    expected_transactions: Sequence[ExpectedTransaction],
) -> Tuple[ConsistencyReport, CoverageReport]:
    """Run advisory consistency and transaction coverage checks."""
    cons = check_consistency(bundle.test_plan, bundle.eval_class)
    cov = check_bundle_coverage(bundle.test_plan, expected_transactions)
    return cons, cov


def _stimulus_has_expected_value(stimulus: Mapping[str, Any]) -> bool:
    for key, value in stimulus.items():
        if key.startswith("expected_") and value is not None:
            return True
    return False


_DERIVATION_SELF_CORRECTION_RE = re.compile(
    r"""
    \b(?:wait|recalc(?:ulate|ulated|ulating)?|recompute|
       correcting|corrected|correction|let'?s|let\s+us|
       to\s+get|prior\s+feedback|but\s+expected|however)\b
    |
    \b(?:choose|set|change|update|fix|correct)\s+
      (?:the\s+)?(?:mock_preload|preload|bytes?|high_byte|low_byte|
                   register|0x[0-9a-fA-F]+)\b
    """,
    re.IGNORECASE | re.VERBOSE,
)


def _plan_derivation_static_errors(plan_bundle: PlanBundle) -> Tuple[str, ...]:
    """Return blocking errors for draft/self-correction text in derivations."""
    stimuli = plan_bundle.test_plan.get("test_stimuli") or []
    if not isinstance(stimuli, Sequence) or isinstance(stimuli, (str, bytes)):
        return ()

    errors: List[str] = []
    for index, stim in enumerate(stimuli):
        if not isinstance(stim, Mapping):
            continue
        derivation = stim.get("derivation")
        if derivation is None:
            continue
        text = str(derivation)
        match = _DERIVATION_SELF_CORRECTION_RE.search(text)
        if not match:
            continue
        name = str(stim.get("name") or f"#{index}")
        errors.append(
            f"test_plan.test_stimuli[{index}] {name!r} derivation contains "
            f"draft/self-correction text near {match.group(0)!r}; rewrite "
            "mock_preload bytes, expected_* values, and derivation as one "
            "final calculation instead of narrating a correction."
        )
    return tuple(errors)


def _twos_complement_checksum_excluding_start(frame: Sequence[int]) -> int:
    """Common UART packet checksum: two's complement of bytes[1:-1]."""
    return (-sum(int(b) & 0xFF for b in frame[1:-1])) & 0xFF


def _looks_like_twos_complement_uart_frame(frame: Sequence[int]) -> bool:
    if len(frame) < 4:
        return False
    return (int(frame[-1]) & 0xFF) == _twos_complement_checksum_excluding_start(frame)


def _iter_uart_write_prefix_frames(plan_bundle: PlanBundle) -> Tuple[Tuple[int, ...], ...]:
    txs = plan_bundle.test_plan.get("expected_transactions") or []
    if not isinstance(txs, Sequence) or isinstance(txs, (str, bytes)):
        return ()

    frames: List[Tuple[int, ...]] = []
    for tx in txs:
        if not isinstance(tx, Mapping):
            continue
        prefixes = tx.get("write_prefix_any_of") or []
        if not isinstance(prefixes, Sequence) or isinstance(prefixes, (str, bytes)):
            continue
        for prefix in prefixes:
            bs = _extract_bytes(prefix)
            if bs is not None and len(bs) >= 4:
                frames.append(tuple(int(b) & 0xFF for b in bs))
    return tuple(frames)


def _iter_uart_response_frames(stim: Mapping[str, Any]) -> Tuple[Tuple[str, Tuple[int, ...]], ...]:
    preload = stim.get("mock_preload") or {}
    if not isinstance(preload, Mapping):
        return ()

    frames: List[Tuple[str, Tuple[int, ...]]] = []
    for key, value in preload.items():
        key_s = str(key)
        if key_s not in {"read_bytes", "payload", "response", "resp"}:
            continue
        bs = _extract_bytes(value)
        if bs is not None and len(bs) >= 4:
            frames.append((key_s, tuple(int(b) & 0xFF for b in bs)))
    return tuple(frames)


def _format_uart_frame(frame: Sequence[int]) -> str:
    return "[" + ", ".join(f"0x{(int(b) & 0xFF):02X}" for b in frame) + "]"


def _uart_packet_checksum_static_errors(plan_bundle: PlanBundle) -> Tuple[str, ...]:
    """Block UART response frames whose checksum contradicts inferred framing."""
    if str(plan_bundle.bus_kind).lower() != "uart":
        return ()

    checksum_prefixes = tuple(
        frame for frame in _iter_uart_write_prefix_frames(plan_bundle)
        if _looks_like_twos_complement_uart_frame(frame)
    )
    if not checksum_prefixes:
        return ()

    errors: List[str] = []
    stimuli = plan_bundle.test_plan.get("test_stimuli") or []
    if not isinstance(stimuli, Sequence) or isinstance(stimuli, (str, bytes)):
        return ()

    for index, stim in enumerate(stimuli):
        if not isinstance(stim, Mapping):
            continue
        name = str(stim.get("name") or f"#{index}")
        for key, frame in _iter_uart_response_frames(stim):
            inferred = next(
                (
                    prefix for prefix in checksum_prefixes
                    if len(prefix) == len(frame) and prefix[0] == frame[0]
                ),
                None,
            )
            if inferred is None:
                continue
            expected = _twos_complement_checksum_excluding_start(frame)
            got = frame[-1] & 0xFF
            if got == expected:
                continue
            errors.append(
                f"UART packet stimulus {name!r} mock_preload.{key} has "
                f"checksum 0x{got:02X}, but the generated write prefix "
                f"{_format_uart_frame(inferred)} establishes a "
                "two's-complement checksum over frame bytes[1:-1]. "
                f"For response frame {_format_uart_frame(frame)}, "
                f"the final byte should be 0x{expected:02X}. Recompute the "
                "checksum for the response frame; do not reuse the request "
                "checksum."
            )
    return tuple(errors)


_C_DECL_RE = re.compile(
    r"""
    (?:
        (?:struct|union|enum)\s+[A-Za-z_][A-Za-z0-9_]* |
        (?:const|volatile|static|unsigned|signed|long|short)\s+ |
        [A-Za-z_][A-Za-z0-9_]*_t |
        [A-Za-z_][A-Za-z0-9_]*
    )
    (?:\s+(?:const|volatile|unsigned|signed|long|short))*\s*
    \*?\s*
    (?P<name>[A-Za-z_][A-Za-z0-9_]*)
    \s*(?:=|;|\[)
    """,
    re.VERBOSE,
)


def _declared_c_identifiers(statement_block: str) -> set[str]:
    """Return likely local variable names declared in a small C statement block."""
    out: set[str] = set()
    for statement in str(statement_block or "").split(";"):
        text = statement.strip()
        if not text or "(" in text:
            continue
        match = _C_DECL_RE.search(text + ";")
        if match:
            out.add(match.group("name"))
    return out


def _call_function_name(call: str) -> str:
    match = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(", str(call or ""))
    return match.group(1) if match else ""


def _display_contract_static_errors(plan_bundle: PlanBundle) -> Tuple[str, ...]:
    if str(plan_bundle.eval_class) != EVAL_CLASS_DISPLAY:
        return tuple()
    api = plan_bundle.api_contract
    call = str(api.get("output_frame_call") or "").strip()
    if not call:
        return tuple()

    errors: List[str] = []
    func_name = _call_function_name(call).lower()
    if re.search(r"(?:^|_)(?:read|get|status|query|probe)(?:_|$)", func_name):
        errors.append(
            "display api_contract.output_frame_call appears to be a read/status "
            f"API ({call!r}). For eval_class=display it must transmit the "
            "provided frame buffer, not read pixels/status from the display."
        )
    if not re.search(r"\b(?:data|buf|frame|buffer)\b", call):
        errors.append(
            "display api_contract.output_frame_call must pass the adapter "
            "frame buffer argument (`data`/`buf`) to the driver."
        )
    if not re.search(r"\b(?:len|length|size)\b", call):
        errors.append(
            "display api_contract.output_frame_call must pass the adapter "
            "frame length argument (`len`) to the driver."
        )
    return tuple(errors)


def _rtc_contract_static_errors(plan_bundle: PlanBundle) -> Tuple[str, ...]:
    if str(plan_bundle.eval_class) != EVAL_CLASS_RTC:
        return tuple()
    api = plan_bundle.api_contract
    get_call = str(api.get("get_time_call") or "").strip()
    set_call = str(api.get("set_time_call") or "").strip()
    if not get_call and not set_call:
        return tuple()

    get_decl = str(api.get("time_struct_decl") or "")
    decl = str(api.get("time_struct_from_in") or "drivergen_eval_time_t t = *in;")
    get_declared = _declared_c_identifiers(get_decl)
    declared = _declared_c_identifiers(decl)
    errors: List[str] = []

    valid_eval_time_fields = {
        "year", "month", "day", "hour", "minute", "second", "weekday", "reserved",
    }
    bad_input_fields = sorted(
        {
            field for field in re.findall(
                r"\bin\s*->\s*([A-Za-z_][A-Za-z0-9_]*)",
                decl + "\n" + set_call,
            )
            if field not in valid_eval_time_fields
        }
    )
    if bad_input_fields:
        errors.append(
            "rtc api_contract references invalid drivergen_eval_time_t input "
            f"field(s) {bad_input_fields!r}; use one of "
            f"{sorted(valid_eval_time_fields)!r}."
        )

    get_addr_of_args = {
        name for name in re.findall(r"&\s*([A-Za-z_][A-Za-z0-9_]*)", get_call)
        if name not in {"in", "out", "g_eval_dev"}
    }
    get_missing = sorted(name for name in get_addr_of_args if name not in get_declared)
    if get_missing:
        errors.append(
            "rtc api_contract.get_time_call references address-of variable(s) "
            f"{get_missing!r} that are not declared by time_struct_decl. Keep "
            "the local time variable name consistent across time_struct_decl, "
            "get_time_call, and time_fields."
        )
    if get_declared and not any(re.search(rf"\b{re.escape(name)}\b", get_call)
                                for name in get_declared):
        errors.append(
            "rtc api_contract.time_struct_decl declares "
            f"{sorted(get_declared)!r}, but get_time_call does not pass any "
            "of those variables."
        )

    if not set_call:
        return tuple(errors)

    set_addr_of_args = {
        name for name in re.findall(r"&\s*([A-Za-z_][A-Za-z0-9_]*)", set_call)
        if name not in {"in", "out", "g_eval_dev"}
    }
    missing = sorted(name for name in set_addr_of_args if name not in declared)
    if missing:
        errors.append(
            "rtc api_contract.set_time_call references address-of variable(s) "
            f"{missing!r} that are not declared by time_struct_from_in. Keep "
            "the local time variable name consistent across time_struct_from_in "
            "and set_time_call."
        )
    if declared and not any(re.search(rf"\b{re.escape(name)}\b", set_call)
                            for name in declared):
        # Ensure the set call uses the declared native time object.
        if not re.search(r"\bin\s*->|\*\s*in\b", set_call):
            errors.append(
                "rtc api_contract.time_struct_from_in declares "
                f"{sorted(declared)!r}, but set_time_call does not pass any "
                "of those variables or the adapter `in` time object."
            )
    return tuple(errors)


def _parse_int_literal(raw: Any) -> Optional[int]:
    if isinstance(raw, bool) or raw is None:
        return None
    if isinstance(raw, int):
        return raw
    text = str(raw).strip()
    if not text:
        return None
    try:
        return int(text, 0)
    except ValueError:
        match = re.search(r"0[xX][0-9A-Fa-f]+|\b\d+\b", text)
        if match:
            try:
                return int(match.group(0), 0)
            except ValueError:
                return None
    return None


def _hex_byte(value: int) -> str:
    return f"0x{int(value) & 0xFF:02X}"


def _signed8(value: int) -> int:
    value = int(value) & 0xFF
    return value - 0x100 if value & 0x80 else value


def _c_trunc_div(left: Any, right: Any) -> int:
    """Integer division with C99 trunc-toward-zero semantics."""
    lhs = int(left)
    rhs = int(right)
    if rhs == 0:
        raise ZeroDivisionError("division by zero")
    magnitude = abs(lhs) // abs(rhs)
    return -magnitude if (lhs < 0) ^ (rhs < 0) else magnitude


_MECH_BINOPS: Dict[type, Any] = {
    ast.Add: _op.add,
    ast.Sub: _op.sub,
    ast.Mult: _op.mul,
    ast.Div: _op.truediv,
    ast.FloorDiv: _c_trunc_div,
    ast.Mod: _op.mod,
    ast.LShift: _op.lshift,
    ast.RShift: _op.rshift,
    ast.BitAnd: _op.and_,
    ast.BitOr: _op.or_,
    ast.BitXor: _op.xor,
}
_MECH_UNARYOPS: Dict[type, Any] = {
    ast.USub: _op.neg,
    ast.UAdd: _op.pos,
    ast.Invert: _op.invert,
}


def _safe_eval_with_names(node: ast.AST, names: Mapping[str, Any]) -> Any:
    if isinstance(node, ast.Expression):
        return _safe_eval_with_names(node.body, names)
    if isinstance(node, ast.Constant):
        if isinstance(node.value, bool):
            raise ValueError("bool literal not allowed")
        if isinstance(node.value, (int, float)):
            return node.value
        raise ValueError(f"constant {type(node.value).__name__} not allowed")
    if isinstance(node, ast.Name):
        if node.id not in names:
            raise ValueError(f"name {node.id!r} not allowed")
        return names[node.id]
    if isinstance(node, ast.UnaryOp):
        fn = _MECH_UNARYOPS.get(type(node.op))
        if fn is None:
            raise ValueError(f"unary op {type(node.op).__name__} not allowed")
        return fn(_safe_eval_with_names(node.operand, names))
    if isinstance(node, ast.BinOp):
        fn = _MECH_BINOPS.get(type(node.op))
        if fn is None:
            raise ValueError(f"binary op {type(node.op).__name__} not allowed")
        return fn(
            _safe_eval_with_names(node.left, names),
            _safe_eval_with_names(node.right, names),
        )
    raise ValueError(f"ast node {type(node).__name__} not allowed")


def _eval_mechanical_expression(expr: str, names: Mapping[str, Any]) -> Optional[int]:
    try:
        value = _safe_eval_with_names(ast.parse(expr, mode="eval"), names)
    except (SyntaxError, ValueError, TypeError, ZeroDivisionError):
        return None
    if isinstance(value, bool):
        return None
    if isinstance(value, float):
        if not value.is_integer():
            return None
        value = int(value)
    return int(value)


def _formula_by_name(device_ir: Mapping[str, Any]) -> Dict[str, Mapping[str, Any]]:
    out: Dict[str, Mapping[str, Any]] = {}
    formulas = device_ir.get("conversion_formulae")
    if not isinstance(formulas, Sequence) or isinstance(formulas, (str, bytes)):
        formulas = device_ir.get("conversion_formulas")
    formulas = formulas or []
    if not isinstance(formulas, Sequence) or isinstance(formulas, (str, bytes)):
        return out
    for formula in formulas:
        if not isinstance(formula, Mapping):
            continue
        name = str(formula.get("name") or "")
        if name:
            out[name] = formula
    return out


def _channel_formula_ids(device_ir: Mapping[str, Any]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    channels = device_ir.get("read_channels") or []
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        ch_id = str(channel.get("id") or "")
        formula_id = str(channel.get("formula_id") or "")
        if ch_id and formula_id:
            out[ch_id] = formula_id
    return out


def _channel_units(device_ir: Mapping[str, Any]) -> Dict[str, str]:
    out: Dict[str, str] = {}
    channels = device_ir.get("read_channels") or []
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        ch_id = str(channel.get("id") or "")
        unit = str(channel.get("physical_unit") or channel.get("unit") or "")
        if ch_id and unit:
            out[ch_id] = unit
    return out


def _channel_high_low_registers(device_ir: Mapping[str, Any]) -> Dict[str, Tuple[int, int]]:
    """Return channel -> (high pointer, low pointer) from IR operation flows."""
    out: Dict[str, Tuple[int, int]] = {}
    flows = device_ir.get("operation_flows") or []
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return out
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        channels = flow.get("channels") or []
        if (
            not isinstance(channels, Sequence)
            or isinstance(channels, (str, bytes))
            or len(channels) != 1
        ):
            continue
        ch_id = str(channels[0])
        high: Optional[int] = None
        low: Optional[int] = None
        steps = flow.get("steps") or []
        if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
            continue
        for step in steps:
            if not isinstance(step, Mapping):
                continue
            transaction = step.get("transaction")
            if not isinstance(transaction, Mapping):
                continue
            if str(transaction.get("kind") or "") != "write_then_read":
                continue
            raw_bytes = transaction.get("bytes") or []
            if not isinstance(raw_bytes, Sequence) or isinstance(raw_bytes, (str, bytes)):
                continue
            if not raw_bytes:
                continue
            pointer = _parse_int_literal(raw_bytes[0])
            if pointer is None:
                continue
            label = " ".join(
                str(part or "")
                for part in (
                    transaction.get("pointer_target"),
                    transaction.get("notes"),
                    step.get("notes"),
                )
            ).lower()
            if "high" in label and high is None:
                high = pointer
            elif "low" in label and low is None:
                low = pointer
        if high is not None and low is not None:
            out[ch_id] = (high, low)
    return out


_MECHANICAL_BYTE_PATTERNS: Tuple[Tuple[int, int], ...] = (
    (0x1A, 0x80),
    (0x64, 0x00),
    (0xF6, 0x00),
    (0x00, 0x00),
    (0x19, 0xA0),
)

_MECHANICAL_UNSIGNED_PHYSICAL_BYTE_PATTERNS: Tuple[Tuple[int, int], ...] = (
    (0x1A, 0x80),
    (0x64, 0x00),
    (0x7F, 0xE0),
    (0x00, 0x00),
    (0x19, 0xA0),
)


@dataclasses.dataclass(frozen=True)
class _RegisterMappedChannelSpec:
    channel_id: str
    source_channel_id: str
    channel: Mapping[str, Any]
    source_regs: Tuple[Tuple[str, int], ...]
    expression: str
    input_names: Tuple[str, ...]
    semantic_kind: str
    raw_encoding: Mapping[str, Any]


@dataclasses.dataclass(frozen=True)
class _RegisterMappedChannelSample:
    register_bytes: Tuple[Tuple[str, int, int], ...]
    expected: int
    derivation: str


@dataclasses.dataclass(frozen=True)
class _StreamBitfieldChannelSpec:
    public_id: str
    source_id: str
    channel: Mapping[str, Any]
    bit_segments: Tuple[Tuple[int, int], ...]
    input_name: str
    expression: str
    semantic_kind: str


_MEMORY_PROBE_LEN = 16
_MEMORY_PROBE_PATTERNS: Tuple[Tuple[int, ...], ...] = (
    (
        0xDE, 0xAD, 0xBE, 0xEF,
        0x10, 0x32, 0x54, 0x76,
        0x89, 0xAB, 0xCD, 0xEF,
        0x01, 0x23, 0x45, 0x67,
    ),
    (
        0x00, 0xFF, 0x55, 0xAA,
        0x12, 0x34, 0x56, 0x78,
        0x9A, 0xBC, 0xFE, 0xDC,
        0x11, 0x22, 0x33, 0x44,
    ),
)


def _generate_mechanical_memory_stimuli(
    api_contract: Mapping[str, Any],
) -> Tuple[Mapping[str, Any], ...]:
    """Generate memory-class self-test stimuli for the fixed read harness."""
    if str(api_contract.get("eval_class") or "") != EVAL_CLASS_MEMORY:
        return ()
    size = _parse_int_literal(api_contract.get("memory_size_bytes"))
    if size is None or size < _MEMORY_PROBE_LEN:
        return ()

    stimuli: List[Mapping[str, Any]] = []
    for index, pattern in enumerate(_MEMORY_PROBE_PATTERNS, start=1):
        payload = tuple(int(byte) & 0xFF for byte in pattern[:_MEMORY_PROBE_LEN])
        stimuli.append({
            "name": f"mechanical_memory_probe_{index}",
            "mock_preload": {
                "0x0000": [_hex_byte(byte) for byte in payload],
            },
            "expected_mem_bytes": "".join(f"{byte:02X}" for byte in payload),
            "derivation": (
                "memory harness reads 16 bytes from device-internal address "
                "0x0000; mock_preload['0x0000'] provides exactly these 16 "
                "bytes, so expected_mem_bytes is their concatenated hex string"
            ),
        })
    return tuple(stimuli)


def _normalise_identifier(value: str) -> str:
    return re.sub(r"[^a-z0-9]+", "_", str(value or "").lower()).strip("_")


def _channel_id_alias_tokens(value: str) -> Tuple[str, ...]:
    key = _normalise_identifier(value)
    if not key:
        return ()
    aliases = {key}
    replacements = {
        "temperature": "temp",
        "temp": "temperature",
        "pressure": "press",
        "press": "pressure",
        "humidity": "hum",
        "hum": "humidity",
    }
    parts = [part for part in key.split("_") if part]
    for part in parts:
        aliases.add(part)
        replacement = replacements.get(part)
        if replacement:
            aliases.add(replacement)
    for src, dst in replacements.items():
        if src in key:
            aliases.add(key.replace(src, dst))
    return tuple(sorted(aliases))


def _read_channel_by_id(device_ir: Mapping[str, Any]) -> Dict[str, Mapping[str, Any]]:
    out: Dict[str, Mapping[str, Any]] = {}
    channels = device_ir.get("read_channels") or []
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return out
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        ch_id = _normalise_identifier(str(channel.get("id") or ""))
        if ch_id:
            out[ch_id] = channel
    return out


def _output_semantics_rows_by_public_id(
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Dict[str, Mapping[str, Any]]:
    rows = output_semantics_map.get("channels") if isinstance(output_semantics_map, Mapping) else []
    if not isinstance(rows, Sequence) or isinstance(rows, (str, bytes)):
        return {}
    out: Dict[str, Mapping[str, Any]] = {}
    for row in rows:
        if not isinstance(row, Mapping):
            continue
        public_id = _normalise_identifier(str(row.get("public_id") or ""))
        if public_id:
            out[public_id] = row
    return out


def _output_semantics_rows_by_source_id(
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Dict[str, Mapping[str, Any]]:
    rows = output_semantics_map.get("channels") if isinstance(output_semantics_map, Mapping) else []
    if not isinstance(rows, Sequence) or isinstance(rows, (str, bytes)):
        return {}
    out: Dict[str, Mapping[str, Any]] = {}
    for row in rows:
        if not isinstance(row, Mapping):
            continue
        source_id = _normalise_identifier(str(row.get("source_channel") or ""))
        if source_id:
            out[source_id] = row
    return out


def _semantic_kind_for_mechanical_channel(
    output_semantics_map: Optional[Mapping[str, Any]],
    *,
    public_id: str,
    source_id: str = "",
) -> str:
    by_public = _output_semantics_rows_by_public_id(output_semantics_map)
    row = by_public.get(_normalise_identifier(public_id))
    if row is None and source_id:
        by_source = _output_semantics_rows_by_source_id(output_semantics_map)
        row = by_source.get(_normalise_identifier(source_id))
    kind = str((row or {}).get("semantic_kind") or "").strip().lower()
    return kind


def _device_uses_spi_stream(device_ir: Mapping[str, Any]) -> bool:
    bus = str(device_ir.get("bus_type") or device_ir.get("bus") or "").strip().lower()
    if "spi" not in bus:
        return False
    for key in ("spi_proto", "spi_sub_mode"):
        if str(device_ir.get(key) or "").strip().lower() == SPI_SUB_STREAM:
            return True
    access_model = device_ir.get("access_model")
    if isinstance(access_model, Mapping):
        kind = str(access_model.get("kind") or "").strip().lower()
        if kind == "stream" or "stream" in kind:
            return True
    return False


def _primary_stream_frame_nbytes(device_ir: Mapping[str, Any]) -> Optional[int]:
    """Infer SPI stream frame length in bytes from IR bit/byte facts."""
    access_model = device_ir.get("access_model")
    if isinstance(access_model, Mapping):
        for key in ("frame_bytes", "read_bytes", "payload_bytes", "byte_length", "nbytes"):
            value = _parse_int_literal(access_model.get(key))
            if value is not None and value > 0:
                return int(value)
        for key in ("frame_bits", "bit_length", "size_bits", "bits"):
            value = _parse_int_literal(access_model.get(key))
            if value is not None and value > 0:
                return max(1, (int(value) + 7) // 8)

    max_source_bit = _max_stream_source_bit(device_ir)
    if max_source_bit is not None:
        return max(1, (max_source_bit + 8) // 8)

    flows = device_ir.get("operation_flows") or []
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return None
    target_flow_ids = _read_channel_flow_ids(device_ir)
    access_text = json.dumps(access_model or {}, ensure_ascii=False).lower()
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        flow_id = str(flow.get("flow_id") or "")
        if target_flow_ids and flow_id not in target_flow_ids:
            continue
        steps = flow.get("steps")
        if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
            continue
        for step in steps:
            if not isinstance(step, Mapping):
                continue
            tx = step.get("transaction")
            if not isinstance(tx, Mapping):
                continue
            if str(tx.get("kind") or "").strip().lower() != "read":
                continue
            length = _parse_int_literal(tx.get("length"))
            if length is None:
                length = _parse_int_literal(step.get("length"))
            if length is None or length <= 0:
                continue
            context = " ".join(
                str(part or "")
                for part in (
                    access_text,
                    flow.get("kind"),
                    flow.get("notes"),
                    step.get("op"),
                    step.get("notes"),
                    tx.get("notes"),
                )
            ).lower()
            if "byte" in context or "octet" in context:
                return int(length)
            if (
                int(length) % 8 == 0
                and any(token in context for token in ("bit", "clock", "cycle", "sck"))
            ):
                return max(1, int(length) // 8)
            if int(length) <= 8:
                return int(length)
    return None


_STREAM_BIT_RANGE_RE = re.compile(
    r"\bD\s*\[?\s*(\d+)\s*(?::|-|\.\.|to)\s*(\d+)\s*\]?",
    re.IGNORECASE,
)
_STREAM_SINGLE_BIT_RE = re.compile(r"\bD\s*\[?\s*(\d+)\s*\]?", re.IGNORECASE)


def _normalise_stream_bit_pairs(
    pairs: Sequence[Tuple[int, int]],
) -> Tuple[Tuple[int, int], ...]:
    out: List[Tuple[int, int]] = []
    for idx, (first, second) in enumerate(pairs):
        high = max(int(first), int(second))
        low = min(int(first), int(second))
        # Handle one-bit sign segments adjacent to the next range.
        if (
            int(second) == 1
            and int(first) > 1
            and idx + 1 < len(pairs)
            and max(int(pairs[idx + 1][0]), int(pairs[idx + 1][1])) == int(first) - 1
        ):
            high = low = int(first)
        out.append((high, low))
    return tuple(out)


def _stream_bit_segments_from_text(value: Any) -> Tuple[Tuple[int, int], ...]:
    text = str(value or "")
    if not text:
        return ()
    pairs = [
        (int(match.group(1)), int(match.group(2)))
        for match in _STREAM_BIT_RANGE_RE.finditer(text)
    ]
    if pairs:
        return _normalise_stream_bit_pairs(pairs)
    singles = [
        (int(match.group(1)), int(match.group(1)))
        for match in _STREAM_SINGLE_BIT_RE.finditer(text)
    ]
    return tuple(singles)


def _stream_bit_segments_for_channel(
    channel: Mapping[str, Any],
    formula_input: Optional[Mapping[str, Any]],
) -> Tuple[Tuple[int, int], ...]:
    candidates: List[Any] = []
    if isinstance(formula_input, Mapping):
        candidates.extend(
            formula_input.get(key)
            for key in ("byte_source", "bit_source", "source_bits", "description")
        )
    sources = channel.get("source_bytes")
    if isinstance(sources, Sequence) and not isinstance(sources, (str, bytes)):
        candidates.append(" || ".join(str(item or "") for item in sources))
    for candidate in candidates:
        segments = _stream_bit_segments_from_text(candidate)
        if segments:
            return segments
    return ()


def _max_stream_source_bit(device_ir: Mapping[str, Any]) -> Optional[int]:
    max_bit: Optional[int] = None
    channels = device_ir.get("read_channels") or []
    if isinstance(channels, Sequence) and not isinstance(channels, (str, bytes)):
        for channel in channels:
            if not isinstance(channel, Mapping):
                continue
            segments = _stream_bit_segments_for_channel(channel, None)
            for high, _low in segments:
                max_bit = high if max_bit is None else max(max_bit, high)
    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            outputs = flow.get("outputs") or []
            if not isinstance(outputs, Sequence) or isinstance(outputs, (str, bytes)):
                continue
            for output in outputs:
                if not isinstance(output, Mapping):
                    continue
                for key in ("byte_source", "bit_source", "source_bits", "notes"):
                    segments = _stream_bit_segments_from_text(output.get(key))
                    for high, _low in segments:
                        max_bit = high if max_bit is None else max(max_bit, high)
    return max_bit


def _register_addresses_by_name(device_ir: Mapping[str, Any]) -> Dict[str, int]:
    out: Dict[str, int] = {}
    entries = device_ir.get("registers_or_commands") or []
    if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
        return out
    for entry in entries:
        if not isinstance(entry, Mapping):
            continue
        name = _normalise_identifier(str(entry.get("name") or ""))
        value = _parse_int_literal(entry.get("value"))
        if name and value is not None:
            out[name] = int(value) & 0xFFFF
    return out


_REGISTER_MATCH_FILLER_TOKENS = {
    "byte", "bytes", "bit", "bits", "data", "register", "reg", "out",
    "output", "value", "measurement", "raw",
}


def _register_source_match_score(source_name: str, register_name: str) -> int:
    source_tokens = {
        tok for tok in _normalise_identifier(source_name).split("_") if tok
    }
    register_tokens = {
        tok for tok in _normalise_identifier(register_name).split("_") if tok
    }
    if not source_tokens or not register_tokens:
        return 0

    role_tokens = {"high", "low", "msb", "lsb", "b0", "b1", "b2"}
    for role in role_tokens & source_tokens:
        if role not in register_tokens:
            return 0

    meaningful = source_tokens - _REGISTER_MATCH_FILLER_TOKENS - role_tokens
    if meaningful and not meaningful <= register_tokens:
        return 0

    score = len(meaningful) * 4 + len(source_tokens & register_tokens)
    if "data" in register_tokens:
        score += 2
    if "limit" in register_tokens and "limit" not in source_tokens:
        score -= 4
    return max(score, 0)


def _register_address_for_source(
    source_name: str,
    register_addrs: Mapping[str, int],
) -> Optional[int]:
    direct = _parse_int_literal(source_name)
    if direct is not None:
        return int(direct) & 0xFFFF
    key = _normalise_identifier(source_name)
    if key in register_addrs:
        return register_addrs[key]
    # Be tolerant of IR source labels that include suffix/prefix text around
    # the register mnemonic, but only accept an unambiguous match.
    matches = [
        addr for reg_name, addr in register_addrs.items()
        if key and (key in reg_name or reg_name in key)
    ]
    unique = sorted(set(matches))
    if len(unique) == 1:
        return unique[0]

    scored = sorted(
        (
            (_register_source_match_score(source_name, reg_name), addr)
            for reg_name, addr in register_addrs.items()
        ),
        reverse=True,
    )
    if not scored or scored[0][0] <= 0:
        return None
    if len(scored) > 1 and scored[0][0] == scored[1][0]:
        return None
    return scored[0][1]


_HEX_LITERAL_RE = re.compile(r"0[xX][0-9A-Fa-f]+|\b\d+\b")


def _canonical_i2c_addr(value: Any) -> Optional[str]:
    parsed = _parse_int_literal(value)
    if parsed is not None and 0 <= int(parsed) <= 0x7F:
        return _hex_byte(parsed)
    if isinstance(value, str):
        match = _HEX_LITERAL_RE.search(value)
        if match:
            parsed = _parse_int_literal(match.group(0))
            if parsed is not None and 0 <= int(parsed) <= 0x7F:
                return _hex_byte(parsed)
    return None


def _address_candidate_value(entry: Mapping[str, Any]) -> Optional[str]:
    for key in ("addr", "address", "value", "address_7bit"):
        addr = _canonical_i2c_addr(entry.get(key))
        if addr is not None:
            return addr
    return None


def _semantic_tokens(value: Any) -> set[str]:
    base = _normalise_identifier(str(value or ""))
    if not base:
        return set()
    tokens = {tok for tok in base.split("_") if tok}
    expanded = set(tokens)
    for tok in list(tokens):
        if tok in {"a", "acc", "accel", "accelerometer", "acceleration"}:
            expanded.update({"accel", "accelerometer", "acceleration"})
        if tok in {"m", "mag", "magn", "magnetometer", "magnetic"}:
            expanded.update({"mag", "magnetometer", "magnetic"})
        if tok in {"g", "gyro", "gyroscope"}:
            expanded.update({"gyro", "gyroscope"})
        if tok in {"prs", "press", "pressure"}:
            expanded.update({"prs", "pressure"})
        if tok in {"tmp", "temp", "temperature"}:
            expanded.update({"tmp", "temp", "temperature"})
    return expanded


def _i2c_address_candidates_for_mock(
    device_ir: Mapping[str, Any],
) -> Tuple[Tuple[str, set[str]], ...]:
    addr_rule = device_ir.get("address_rule")
    if not isinstance(addr_rule, Mapping):
        return tuple()
    entries = addr_rule.get("addresses")
    if not isinstance(entries, Sequence) or isinstance(entries, (str, bytes)):
        return tuple()

    out: List[Tuple[str, set[str]]] = []
    seen: set[str] = set()
    for entry in entries:
        if not isinstance(entry, Mapping):
            continue
        addr = _address_candidate_value(entry)
        if addr is None or addr in seen:
            continue
        seen.add(addr)
        tokens: set[str] = set()
        for key in ("name", "description", "condition", "role"):
            tokens.update(_semantic_tokens(entry.get(key)))
        out.append((addr, tokens))
    return tuple(out)


def _source_i2c_context_tokens(
    source_name: str,
    channel_id: str,
    channel: Mapping[str, Any],
) -> set[str]:
    tokens: set[str] = set()
    for value in (
        source_name,
        channel_id,
        channel.get("description"),
        channel.get("physical_unit"),
        channel.get("formula_id"),
        channel.get("flow_id"),
    ):
        tokens.update(_semantic_tokens(value))

    norm = _normalise_identifier(" ".join(str(v or "") for v in (
        source_name, channel_id, channel.get("description"),
    )))
    # Register suffixes commonly identify the logical die/sub-device:
    # OUT_X_L_A / OUT_X_L_M, MAG_X_H, ACCEL_XOUT_H, etc.
    if re.search(r"(?:^|_)a(?:$|_)", norm) or "accel" in norm:
        tokens.update({"accel", "accelerometer", "acceleration"})
    if re.search(r"(?:^|_)m(?:$|_)", norm) or "mag" in norm:
        tokens.update({"mag", "magnetometer", "magnetic"})
    return tokens


def _infer_i2c_addr_for_source(
    device_ir: Mapping[str, Any],
    *,
    source_name: str,
    channel_id: str,
    channel: Mapping[str, Any],
) -> Optional[str]:
    candidates = _i2c_address_candidates_for_mock(device_ir)
    if len(candidates) <= 1:
        return None

    context_tokens = _source_i2c_context_tokens(source_name, channel_id, channel)
    if not context_tokens:
        return candidates[0][0]

    scored: List[Tuple[int, str]] = []
    for addr, addr_tokens in candidates:
        score = len(context_tokens & addr_tokens)
        for token in addr_tokens:
            if len(token) >= 4 and any(
                token in ctx or ctx in token
                for ctx in context_tokens
                if len(ctx) >= 3
            ):
                score += 1
        if score > 0:
            scored.append((score, addr))

    if not scored:
        return candidates[0][0]
    scored.sort(reverse=True)
    if len(scored) > 1 and scored[0][0] == scored[1][0]:
        return candidates[0][0]
    return scored[0][1]


def _preload_key_for_source_register(
    device_ir: Mapping[str, Any],
    *,
    source_name: str,
    channel_id: str,
    channel: Mapping[str, Any],
    reg: int,
) -> str:
    addr = _infer_i2c_addr_for_source(
        device_ir,
        source_name=source_name,
        channel_id=channel_id,
        channel=channel,
    )
    reg_key = _hex_byte(reg)
    return f"{addr}:{reg_key}" if addr else reg_key


_IDENTITY_REGISTER_NAMES = {
    "id",
    "devid",
    "device_id",
    "chip_id",
    "chipid",
    "part_id",
    "product_id",
    "prod_id",
    "model_id",
    "who_am_i",
    "whoami",
}

_IDENTITY_TEXT_RE = re.compile(
    r"\b(?:device|chip|part|product|model|manufacturer)\s+id\b|"
    r"\bwho[\s_-]*am[\s_-]*i\b|\bdevid\b",
    re.IGNORECASE,
)
_FIXED_VALUE_RE = re.compile(
    r"\b(?:fixed(?:\s+value)?|expected(?:\s+value)?|expects?|"
    r"constant(?:\s+value)?|reset(?:\s+value)?|power[-\s]?on(?:\s+value)?|"
    r"reads?\s+as|returns?)\b.{0,80}?"
    r"(0[xX][0-9A-Fa-f]{1,2}|\b\d{1,3}\b)",
    re.IGNORECASE,
)


def _is_identity_register_name(name: Any, description: Any = "") -> bool:
    key = _normalise_identifier(str(name or ""))
    if not key:
        return False
    if key in _IDENTITY_REGISTER_NAMES:
        return True
    if "who_am_i" in key or key.endswith("_id"):
        return True
    return bool(_IDENTITY_TEXT_RE.search(f"{name or ''} {description or ''}"))


_HEX_SUFFIX_RE = re.compile(r"\b([0-9A-Fa-f]{1,2})h\b")


def _identity_expected_value_from_text(*values: Any) -> Optional[int]:
    text = " ".join(str(value or "") for value in values if value is not None)
    if not text.strip():
        return None
    for match in _FIXED_VALUE_RE.finditer(text):
        parsed = _parse_int_literal(match.group(1))
        if parsed is not None and 0 <= int(parsed) <= 0xFF:
            return int(parsed) & 0xFF

    # Intel hex suffix: "5Dh", "21h", "A0h" (not matched by 0xNN regexes)
    for match in _HEX_SUFFIX_RE.finditer(text):
        try:
            value = int(match.group(1), 16)
            if 0 <= value <= 0xFF:
                return value
        except ValueError:
            continue

    lowered = text.lower()
    if not any(token in lowered for token in ("fixed", "expected", "constant", "reset")):
        return None
    literals = _HEX_LITERAL_RE.findall(text)
    for literal in reversed(literals):
        parsed = _parse_int_literal(literal)
        if parsed is not None and 0 <= int(parsed) <= 0xFF:
            return int(parsed) & 0xFF
    return None


def _register_entry_text(entry: Mapping[str, Any]) -> str:
    parts: List[str] = []
    for key in (
        "name",
        "description",
        "notes",
        "reset_value",
        "default_value",
        "fixed_value",
        "expected_value",
    ):
        value = entry.get(key)
        if value is not None:
            parts.append(str(value))
    return " ".join(parts)


def _identity_probe_register_preloads(
    device_ir: Mapping[str, Any],
) -> Dict[str, List[str]]:
    """Build init/probe identity-register preloads from Device IR only."""

    register_addrs = _register_addresses_by_name(device_ir)
    register_entries: Dict[str, Mapping[str, Any]] = {}
    entries = device_ir.get("registers_or_commands") or []
    if isinstance(entries, Sequence) and not isinstance(entries, (str, bytes)):
        for entry in entries:
            if not isinstance(entry, Mapping):
                continue
            key = _normalise_identifier(str(entry.get("name") or ""))
            if key:
                register_entries[key] = entry

    found: Dict[int, int] = {}
    for key, entry in register_entries.items():
        text = _register_entry_text(entry)
        if not _is_identity_register_name(entry.get("name"), text):
            continue
        addr = _parse_int_literal(entry.get("value"))
        value = _identity_expected_value_from_text(text)
        if addr is not None and value is not None:
            found[int(addr) & 0xFF] = int(value) & 0xFF

    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            flow_text = " ".join(
                str(flow.get(key) or "")
                for key in ("flow_id", "name", "kind", "description", "notes")
            )
            flow_mentions_identity = (
                str(flow.get("kind") or "").strip().lower() == "probe"
                or bool(_IDENTITY_TEXT_RE.search(flow_text))
            )
            steps = flow.get("steps")
            if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
                continue
            for step in steps:
                if not isinstance(step, Mapping):
                    continue
                tx = step.get("transaction")
                if not isinstance(tx, Mapping):
                    tx = {}
                step_text = " ".join((
                    flow_text,
                    json.dumps(step, ensure_ascii=False, sort_keys=True),
                ))
                pointer = (
                    tx.get("pointer_target")
                    or tx.get("register")
                    or tx.get("reg")
                    or tx.get("command")
                )
                pointer_key = _normalise_identifier(str(pointer or ""))
                pointer_entry = register_entries.get(pointer_key)
                addr = _register_address_for_source(str(pointer or ""), register_addrs)
                identity_like = flow_mentions_identity or _is_identity_register_name(
                    pointer,
                    _register_entry_text(pointer_entry) if isinstance(pointer_entry, Mapping) else step_text,
                )
                if not identity_like or addr is None:
                    continue
                value = _identity_expected_value_from_text(
                    step_text,
                    _register_entry_text(pointer_entry) if isinstance(pointer_entry, Mapping) else "",
                )
                if value is not None:
                    found[int(addr) & 0xFF] = int(value) & 0xFF

    return {
        _hex_byte(reg): [_hex_byte(value)]
        for reg, value in sorted(found.items())
    }


def _merge_identity_probe_preloads(
    device_ir: Mapping[str, Any],
    preload: Mapping[str, List[str]],
) -> Dict[str, List[str]]:
    merged = {str(key): list(value) for key, value in preload.items()}
    for key, value in _identity_probe_register_preloads(device_ir).items():
        merged.setdefault(key, list(value))
    return merged


def _spi_register_burst_start_register(
    device_ir: Mapping[str, Any],
    regs: Sequence[int],
) -> Optional[int]:
    if str(device_ir.get("bus_type") or "").strip().lower() != "spi":
        return None
    unique_regs = sorted({int(reg) & 0xFF for reg in regs})
    if not unique_regs:
        return None
    min_reg = unique_regs[0]
    if unique_regs != list(range(min_reg, min_reg + len(unique_regs))):
        return None

    register_addrs = _register_addresses_by_name(device_ir)
    target_flow_ids = _read_channel_flow_ids(device_ir)
    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            flow_id = str(flow.get("flow_id") or "")
            if target_flow_ids and flow_id and flow_id not in target_flow_ids:
                continue
            steps = flow.get("steps")
            if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
                continue
            for step in steps:
                if not isinstance(step, Mapping):
                    continue
                tx = step.get("transaction")
                if not isinstance(tx, Mapping):
                    continue
                if str(tx.get("kind") or "") != "write_then_read":
                    continue
                pointer = (
                    tx.get("pointer_target")
                    or tx.get("register")
                    or tx.get("reg")
                    or tx.get("command")
                )
                start = _register_address_for_source(str(pointer or ""), register_addrs)
                length = _parse_int_literal(tx.get("read_length") or tx.get("length"))
                if start is None:
                    continue
                if int(start) & 0xFF != min_reg:
                    continue
                if length is not None and int(length) < len(unique_regs):
                    continue
                return min_reg
    return min_reg


def _pack_spi_register_burst_preload(
    device_ir: Mapping[str, Any],
    preload: Mapping[str, List[str]],
    register_bytes: Sequence[Tuple[str, int, int]],
) -> Dict[str, List[str]]:
    if str(device_ir.get("bus_type") or "").strip().lower() != "spi":
        return {str(key): list(value) for key, value in preload.items()}
    by_reg: Dict[int, int] = {}
    for _source_name, reg, byte in register_bytes:
        reg8 = int(reg) & 0xFF
        if reg8 in by_reg and by_reg[reg8] != (int(byte) & 0xFF):
            return {str(key): list(value) for key, value in preload.items()}
        by_reg[reg8] = int(byte) & 0xFF
    if len(by_reg) <= 1:
        return {str(key): list(value) for key, value in preload.items()}

    start = _spi_register_burst_start_register(device_ir, tuple(by_reg))
    if start is None:
        return {str(key): list(value) for key, value in preload.items()}
    regs = sorted(by_reg)
    if regs[0] != start or regs != list(range(start, start + len(regs))):
        return {str(key): list(value) for key, value in preload.items()}

    packed = {str(key): list(value) for key, value in preload.items()}
    for reg in regs:
        packed.pop(_hex_byte(reg), None)
    packed[_hex_byte(start)] = [_hex_byte(by_reg[reg]) for reg in regs]
    return packed


def _channel_bit_width(channel: Mapping[str, Any], source_count: int) -> int:
    raw_type = str(channel.get("raw_type") or "")
    match = re.search(r"(\d+)", raw_type)
    if match:
        width = int(match.group(1))
        if width > 0:
            return width
    return max(8, int(source_count) * 8)


def _channel_is_signed(channel: Mapping[str, Any]) -> bool:
    raw_type = str(channel.get("raw_type") or "").strip().lower()
    return raw_type.startswith("int") and not raw_type.startswith("uint")


def _raw_pattern_for_channel(
    *,
    width: int,
    signed: bool,
    stim_index: int,
    channel_index: int,
) -> int:
    magnitude = 1000 + channel_index * 73
    if width <= 12:
        magnitude = min(magnitude, (1 << (width - 1)) - 1 if signed else (1 << width) - 1)
    elif width <= 16:
        magnitude = min(magnitude, 12000)
    else:
        magnitude = min(magnitude, 0x001000 + channel_index * 0x101)
    if stim_index == 1 and signed:
        return -magnitude
    return magnitude


def _raw_to_bytes(raw: int, width: int, nbytes: int) -> Tuple[int, ...]:
    mask = (1 << (nbytes * 8)) - 1
    encoded = int(raw) & mask
    return tuple((encoded >> (8 * (nbytes - 1 - i))) & 0xFF for i in range(nbytes))


def _raw_encoding_little_endian(raw_encoding: Mapping[str, Any]) -> Optional[bool]:
    order = str(raw_encoding.get("byte_order") or "").strip().lower()
    if "little" in order or "lsb" in order:
        return True
    if "big" in order or "msb" in order:
        return False
    return None


def _register_raw_to_source_order_bytes(
    raw: int,
    width: int,
    nbytes: int,
    raw_encoding: Mapping[str, Any],
) -> Tuple[int, ...]:
    little = _raw_encoding_little_endian(raw_encoding)
    if little is None or not little:
        return _raw_to_bytes(raw, width, nbytes)
    mask = (1 << (nbytes * 8)) - 1
    encoded = int(raw) & mask
    return tuple((encoded >> (8 * i)) & 0xFF for i in range(nbytes))


def _source_byte_role(source_name: str, position: int, total: int) -> str:
    key = _normalise_identifier(source_name)
    if any(tok in key for tok in ("high", "_h", "msb", "b2")):
        return "high"
    if any(tok in key for tok in ("low", "_l", "lsb", "b0")):
        return "low"
    if total == 2:
        tokens = [tok for tok in key.split("_") if tok]
        tail = tokens[-1] if tokens else key
        if tail.endswith(("0", "l", "lo", "lsb", "low")):
            return "low"
        if tail.endswith(("1", "h", "hi", "msb", "high")):
            return "high"
    if "b1" in key and total == 3:
        return "middle"
    if total == 1:
        return "only"
    if total == 2:
        return "high" if position == 0 else "low"
    if total == 3:
        return ("high", "middle", "low")[min(position, 2)]
    return f"byte{position}"


def _assign_raw_bytes_to_sources(
    source_regs: Sequence[Tuple[str, int]],
    raw_bytes: Sequence[int],
) -> Tuple[Tuple[str, int, int], ...]:
    total = len(source_regs)
    if total != len(raw_bytes):
        return ()
    out: List[Tuple[str, int, int]] = []
    for idx, (source_name, reg) in enumerate(source_regs):
        role = _source_byte_role(source_name, idx, total)
        if total == 1:
            byte = raw_bytes[0]
        elif total == 2 and role == "low":
            byte = raw_bytes[1]
        elif total == 2:
            byte = raw_bytes[0]
        elif total == 3 and role == "high":
            byte = raw_bytes[0]
        elif total == 3 and role == "middle":
            byte = raw_bytes[1]
        elif total == 3 and role == "low":
            byte = raw_bytes[2]
        else:
            byte = raw_bytes[idx]
        out.append((source_name, reg, int(byte) & 0xFF))
    return tuple(out)


def _assign_encoded_raw_bytes_to_sources(
    source_regs: Sequence[Tuple[str, int]],
    raw_bytes: Sequence[int],
    raw_encoding: Mapping[str, Any],
) -> Tuple[Tuple[str, int, int], ...]:
    total = len(source_regs)
    if total != len(raw_bytes):
        return ()
    little = _raw_encoding_little_endian(raw_encoding)
    if little is None:
        return _assign_raw_bytes_to_sources(source_regs, raw_bytes)

    def index_for_role(role: str, position: int) -> int:
        if total == 1:
            return 0
        if role == "low":
            return 0 if little else total - 1
        if role == "high":
            return total - 1 if little else 0
        if role == "middle" and total == 3:
            return 1
        return position

    out: List[Tuple[str, int, int]] = []
    for idx, (source_name, reg) in enumerate(source_regs):
        role = _source_byte_role(source_name, idx, total)
        byte_idx = index_for_role(role, idx)
        if byte_idx < 0 or byte_idx >= len(raw_bytes):
            return ()
        out.append((source_name, reg, int(raw_bytes[byte_idx]) & 0xFF))
    return tuple(out)


def _expr_uses_name(expr: str, name: str) -> bool:
    return bool(re.search(rf"\b{re.escape(name)}\b", expr))


def _formula_input_mode(expr: str, input_names: Sequence[str]) -> str:
    names = {_normalise_identifier(name) for name in input_names}
    if _expr_uses_name(expr, "raw") or any(name.startswith("raw") for name in names):
        return "raw"
    if _expr_uses_name(expr, "high_byte") and _expr_uses_name(expr, "low_byte"):
        return "high_low"
    return ""


def _replace_name(expr: str, name: str, replacement: str) -> str:
    return re.sub(rf"\b{re.escape(name)}\b", replacement, expr)


def _byte_patterns_for_register_mapped_sample(
    spec: _RegisterMappedChannelSpec,
) -> Tuple[Tuple[int, int], ...]:
    """Choose deterministic high/low probes that respect explicit unsigned IR."""
    if semantic_kind_is_physical(str(spec.semantic_kind or "")):
        if not _raw_type_is_signed(spec.channel.get("raw_type"), spec.raw_encoding):
            return _MECHANICAL_UNSIGNED_PHYSICAL_BYTE_PATTERNS
    return _MECHANICAL_BYTE_PATTERNS


def _register_mapped_channel_sample(
    spec: _RegisterMappedChannelSpec,
    stim_index: int,
    channel_index: int,
) -> Optional[_RegisterMappedChannelSample]:
    width = _channel_bit_width(spec.channel, len(spec.source_regs))
    signed = _channel_is_signed(spec.channel)
    expr = spec.expression
    mode = _formula_input_mode(expr, spec.input_names)
    semantic_kind = str(spec.semantic_kind or "").strip().lower()

    if mode == "high_low":
        byte_patterns = _byte_patterns_for_register_mapped_sample(spec)
        high_byte, low_byte = byte_patterns[
            (stim_index + channel_index) % len(byte_patterns)
        ]
        register_bytes: List[Tuple[str, int, int]] = []
        for idx, (source_name, reg) in enumerate(spec.source_regs):
            role = _source_byte_role(source_name, idx, len(spec.source_regs))
            if role == "high":
                byte = high_byte
            elif role == "low":
                byte = low_byte
            else:
                byte = high_byte if idx == 0 else low_byte
            register_bytes.append((source_name, reg, byte))
        raw_bytes = tuple(byte for _source_name, _reg, byte in register_bytes)
        raw_expr = _raw_expression_from_bytes(raw_bytes, signed=signed, width=width)
        raw_value = _raw_value_from_bytes(raw_bytes, signed=signed, width=width)
        if semantic_kind in {"raw_count", "status_or_code"}:
            return _RegisterMappedChannelSample(
                register_bytes=tuple(register_bytes),
                expected=int(raw_value),
                derivation=(
                    f"{spec.channel_id}: SECTION B3 semantic_kind={semantic_kind}; "
                    f"public output is unconverted raw/code value; "
                    f"{raw_expr} = {int(raw_value)}"
                ),
            )
        expected = _eval_mechanical_expression(
            expr,
            {"high_byte": int(high_byte) & 0xFF, "low_byte": int(low_byte) & 0xFF},
        )
        if expected is None:
            if semantic_kind_is_physical(semantic_kind):
                return None
            return None
        rendered = _replace_name(expr, "high_byte", f"0x{high_byte:02X}")
        rendered = _replace_name(rendered, "low_byte", f"0x{low_byte:02X}")
        return _RegisterMappedChannelSample(
            register_bytes=tuple(register_bytes),
            expected=int(expected),
            derivation=(
                f"{spec.channel_id}: {rendered} = {int(expected)}"
            ),
        )

    raw = _raw_pattern_for_channel(
        width=width,
        signed=signed,
        stim_index=stim_index,
        channel_index=channel_index,
    )
    raw_bytes = _register_raw_to_source_order_bytes(
        raw,
        width,
        len(spec.source_regs),
        spec.raw_encoding,
    )
    register_bytes = _assign_encoded_raw_bytes_to_sources(
        spec.source_regs,
        raw_bytes,
        spec.raw_encoding,
    )
    if not register_bytes:
        return None

    raw_expr = _raw_expression_from_bytes(
        raw_bytes,
        signed=signed,
        width=width,
        raw_encoding=spec.raw_encoding,
    )
    raw_value = _raw_value_from_bytes(
        raw_bytes,
        signed=signed,
        width=width,
        raw_encoding=spec.raw_encoding,
    )
    if semantic_kind in {"raw_count", "status_or_code"}:
        return _RegisterMappedChannelSample(
            register_bytes=register_bytes,
            expected=int(raw_value),
            derivation=(
                f"{spec.channel_id}: SECTION B3 semantic_kind={semantic_kind}; "
                f"public output is unconverted raw/code value; "
                f"{raw_expr} = {int(raw_value)}"
            ),
        )

    if mode == "raw":
        # Prefer the explicit formula input name when it is not literally
        # ``raw`` (for example ``raw_temp``), otherwise use ``raw``.
        raw_name = "raw"
        for name in spec.input_names:
            if _expr_uses_name(expr, name):
                raw_name = name
                break
        expected = _eval_mechanical_expression(expr, {raw_name: int(raw_value)})
        if expected is not None:
            rendered = _replace_name(expr, raw_name, f"({raw_expr})")
            derivation = f"{spec.channel_id}: {raw_expr} = {int(raw_value)}; {rendered} = {int(expected)}"
            return _RegisterMappedChannelSample(
                register_bytes=register_bytes,
                expected=int(expected),
                derivation=derivation,
            )

    if semantic_kind_is_physical(semantic_kind):
        return None

    return _RegisterMappedChannelSample(
        register_bytes=register_bytes,
        expected=int(raw_value),
        derivation=(
            f"{spec.channel_id}: raw fallback because conversion expression "
            f"is not executable from source bytes; {raw_expr} = {int(raw_value)}"
        ),
    )


def _raw_expression_from_bytes(
    raw_bytes: Sequence[int],
    *,
    signed: bool,
    width: int,
    raw_encoding: Optional[Mapping[str, Any]] = None,
) -> str:
    nbytes = len(raw_bytes)
    little = _raw_encoding_little_endian(raw_encoding or {}) is True
    if little:
        terms = [
            f"0x{int(byte) & 0xFF:02X}" if idx == 0
            else f"(0x{int(byte) & 0xFF:02X} << {8 * idx})"
            for idx, byte in enumerate(raw_bytes)
        ]
        unsigned = sum((int(byte) & 0xFF) << (8 * idx) for idx, byte in enumerate(raw_bytes))
    else:
        terms = [
            f"0x{int(byte) & 0xFF:02X}" if idx == nbytes - 1
            else f"(0x{int(byte) & 0xFF:02X} << {8 * (nbytes - 1 - idx)})"
            for idx, byte in enumerate(raw_bytes)
        ]
        unsigned = 0
        for byte in raw_bytes:
            unsigned = (unsigned << 8) | (int(byte) & 0xFF)
    expr = "(" + " | ".join(terms) + ")"
    effective_width = width if width and width > 0 else len(raw_bytes) * 8
    if raw_bytes:
        effective_width = min(effective_width, len(raw_bytes) * 8)
    if signed and effective_width > 0:
        sign_bit = 1 << (effective_width - 1)
        if unsigned & sign_bit:
            expr = f"({expr} - {1 << effective_width})"
    return expr


def _raw_value_from_bytes(
    raw_bytes: Sequence[int],
    *,
    signed: bool,
    width: int,
    raw_encoding: Optional[Mapping[str, Any]] = None,
) -> int:
    little = _raw_encoding_little_endian(raw_encoding or {}) is True
    if little:
        unsigned = sum((int(byte) & 0xFF) << (8 * idx) for idx, byte in enumerate(raw_bytes))
    else:
        unsigned = 0
        for byte in raw_bytes:
            unsigned = (unsigned << 8) | (int(byte) & 0xFF)
    effective_width = width if width and width > 0 else len(raw_bytes) * 8
    if raw_bytes:
        effective_width = min(effective_width, len(raw_bytes) * 8)
    if signed and effective_width > 0:
        sign_bit = 1 << (effective_width - 1)
        if unsigned & sign_bit:
            unsigned -= 1 << effective_width
    return int(unsigned)


def _single_channel_read_nbytes(device_ir: Mapping[str, Any], channel: Mapping[str, Any]) -> int:
    read_sequence = device_ir.get("read_sequence") or []
    if isinstance(read_sequence, Sequence) and not isinstance(read_sequence, (str, bytes)):
        for step in read_sequence:
            if not isinstance(step, Mapping):
                continue
            tx = step.get("transaction")
            if not isinstance(tx, Mapping):
                continue
            length = _parse_int_literal(tx.get("length"))
            if length is not None and length > 0:
                return int(length)
    source_bytes = channel.get("source_bytes")
    if isinstance(source_bytes, Sequence) and not isinstance(source_bytes, (str, bytes)) and source_bytes:
        return len(source_bytes)
    raw_encoding = device_ir.get("raw_encoding") if isinstance(device_ir.get("raw_encoding"), Mapping) else {}
    bit_width = _parse_int_literal(raw_encoding.get("bit_width"))
    if bit_width is None:
        raw_type = str(channel.get("raw_type") or "")
        match = re.search(r"(\d+)", raw_type)
        if match:
            bit_width = int(match.group(1))
    return max(1, ((bit_width or 8) + 7) // 8)


def _access_model_prefers_direct_read(device_ir: Mapping[str, Any]) -> bool:
    access_model = device_ir.get("access_model")
    if not isinstance(access_model, Mapping):
        return False
    read_requires_pointer = access_model.get("read_requires_pointer")
    if isinstance(read_requires_pointer, bool):
        return read_requires_pointer is False
    text = " ".join(
        str(access_model.get(key) or "")
        for key in ("kind", "mode", "notes", "description")
    ).lower()
    return any(token in text for token in ("direct_read", "direct read", "command-mode", "command mode"))


def _step_transaction_text(step: Mapping[str, Any], tx: Mapping[str, Any]) -> str:
    return " ".join(
        str(part or "")
        for part in (
            step.get("op"),
            step.get("kind"),
            step.get("notes"),
            tx.get("kind"),
            tx.get("notes"),
            tx.get("pointer_target"),
            tx.get("register"),
            tx.get("reg"),
            tx.get("command"),
        )
    ).lower()


def _transaction_first_byte(tx: Mapping[str, Any]) -> Optional[int]:
    raw_bytes = tx.get("bytes") or []
    if not isinstance(raw_bytes, Sequence) or isinstance(raw_bytes, (str, bytes)) or not raw_bytes:
        return None
    return _parse_int_literal(raw_bytes[0])


def _transaction_declares_register_pointer(step: Mapping[str, Any], tx: Mapping[str, Any]) -> bool:
    kind = str(tx.get("kind") or step.get("op") or step.get("kind") or "").strip().lower()
    if kind in {"write_then_read", "read_register", "register_read", "reg_read", "mem_read"}:
        return True
    if any(token in kind for token in ("write_then_read", "register_read", "reg_read", "mem_read")):
        return True
    if tx.get("pointer_target") or tx.get("register") or tx.get("reg"):
        return True
    text = _step_transaction_text(step, tx)
    return bool(re.search(r"\b(pointer|register)\b", text))


def _transaction_looks_like_command_write(step: Mapping[str, Any], tx: Mapping[str, Any]) -> bool:
    if _transaction_first_byte(tx) is None:
        return False
    kind = str(tx.get("kind") or step.get("op") or step.get("kind") or "").strip().lower()
    text = _step_transaction_text(step, tx)
    return (
        kind in {"command", "cmd", "write_command"}
        or "command" in text
        or "opcode" in text
        or tx.get("command") is not None
    )


def _transaction_is_unaddressed_read(step: Mapping[str, Any], tx: Mapping[str, Any]) -> bool:
    if _transaction_first_byte(tx) is not None:
        return False
    length = _parse_int_literal(tx.get("length") or tx.get("read_length") or step.get("length"))
    if length is None or length <= 0:
        return False
    kind = str(tx.get("kind") or step.get("op") or step.get("kind") or "").strip().lower()
    return not kind or "read" in kind


def _single_channel_preload_key_from_steps(
    steps: Any,
    *,
    direct_read_preferred: bool,
    assume_unspecified_write_is_pointer: bool,
) -> Optional[str]:
    if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
        return None

    fallback_pointer: Optional[int] = None
    saw_command_write = False
    saw_unaddressed_read = False
    for step in steps:
        if not isinstance(step, Mapping):
            continue
        tx = step.get("transaction")
        if not isinstance(tx, Mapping):
            continue
        pointer = _transaction_first_byte(tx)
        if pointer is not None:
            if _transaction_declares_register_pointer(step, tx):
                return _hex_byte(pointer)
            if _transaction_looks_like_command_write(step, tx):
                saw_command_write = True
                continue
            if assume_unspecified_write_is_pointer and fallback_pointer is None:
                fallback_pointer = pointer
            continue
        if _transaction_is_unaddressed_read(step, tx):
            saw_unaddressed_read = True

    if saw_unaddressed_read and (direct_read_preferred or saw_command_write or fallback_pointer is None):
        return "read_bytes"
    if fallback_pointer is not None:
        return _hex_byte(fallback_pointer)
    return None


def _single_channel_preload_key(device_ir: Mapping[str, Any]) -> str:
    direct_read_preferred = _access_model_prefers_direct_read(device_ir)
    read_sequence = device_ir.get("read_sequence") or []
    key = _single_channel_preload_key_from_steps(
        read_sequence,
        direct_read_preferred=direct_read_preferred,
        assume_unspecified_write_is_pointer=True,
    )
    if key is not None:
        return key
    channel = _first_read_channel(device_ir) or {}
    channel_id = str(channel.get("id") or "")
    flow_id = str(channel.get("flow_id") or channel.get("read_flow_id") or "")
    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            if str(flow.get("kind") or "").strip().lower() not in {"read", "probe"}:
                continue
            flow_channels = flow.get("channels") or []
            channel_match = (
                flow_id and str(flow.get("flow_id") or "") == flow_id
            ) or (
                channel_id
                and isinstance(flow_channels, Sequence)
                and not isinstance(flow_channels, (str, bytes))
                and channel_id in {str(item) for item in flow_channels}
            )
            if not channel_match:
                continue
            key = _single_channel_preload_key_from_steps(
                flow.get("steps") or [],
                direct_read_preferred=direct_read_preferred,
                assume_unspecified_write_is_pointer=True,
            )
            if key is not None:
                return key
    return "read_bytes"


def _resolve_public_channel_source(
    public_id: str,
    channels_by_id: Mapping[str, Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Tuple[str, Optional[Mapping[str, Any]], Mapping[str, Any]]:
    sem_by_public = _output_semantics_rows_by_public_id(output_semantics_map)
    sem_by_source = _output_semantics_rows_by_source_id(output_semantics_map)
    public_key = _normalise_identifier(public_id)
    sem = sem_by_public.get(public_key) or sem_by_source.get(public_key) or {}
    source_id = str((sem or {}).get("source_channel") or public_id).strip()
    source_key = _normalise_identifier(source_id)
    channel = channels_by_id.get(source_key)
    if channel is not None:
        return source_id, channel, sem
    channel = channels_by_id.get(public_key)
    if channel is not None:
        return str(channel.get("id") or public_id), channel, sem

    substring_matches = [
        (key, value)
        for key, value in channels_by_id.items()
        if public_key and (public_key in key or key in public_key)
    ]
    if len(substring_matches) == 1:
        _key, channel = substring_matches[0]
        return str(channel.get("id") or public_id), channel, sem

    public_tokens = {tok for tok in public_key.split("_") if tok}
    scored: List[Tuple[int, str, Mapping[str, Any]]] = []
    for key, channel in channels_by_id.items():
        source_tokens = {tok for tok in key.split("_") if tok}
        score = len(public_tokens & source_tokens)
        if "temp" in public_tokens and "temperature" in source_tokens:
            score += 1
        if "temperature" in public_tokens and "temp" in source_tokens:
            score += 1
        if score > 0:
            scored.append((score, key, channel))
    scored.sort(reverse=True)
    if scored and (len(scored) == 1 or scored[0][0] > scored[1][0]):
        channel = scored[0][2]
        return str(channel.get("id") or public_id), channel, sem

    return source_id, None, sem


def _stream_formula_input(
    formula: Optional[Mapping[str, Any]],
) -> Tuple[str, str, Optional[Mapping[str, Any]]]:
    if not isinstance(formula, Mapping):
        return "", "", None
    expr_obj = formula.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return "", "", None
    expr = str(expr_obj.get("expression") or "").strip()
    inputs = expr_obj.get("inputs") or []
    if not isinstance(inputs, Sequence) or isinstance(inputs, (str, bytes)):
        return expr, "", None
    usable = [
        item for item in inputs
        if isinstance(item, Mapping) and str(item.get("name") or "").strip()
    ]
    if len(usable) != 1:
        return expr, "", None
    input_row = usable[0]
    return expr, str(input_row.get("name") or "").strip(), input_row


def _stream_bitfield_width(segments: Sequence[Tuple[int, int]]) -> int:
    return sum(max(0, int(high) - int(low) + 1) for high, low in segments)


def _stream_raw_pattern(
    *,
    width: int,
    signed: bool,
    stim_index: int,
    channel_index: int,
) -> int:
    if width <= 0:
        return 0
    max_raw = (1 << width) - 1
    if signed and width > 1:
        max_raw = (1 << (width - 1)) - 1
    if max_raw <= 0:
        return 0
    raw = 1000 + stim_index * 257 + channel_index * 73
    return min(raw, max_raw)


def _stream_negative_signed_sample(width: int, _channel_index: int) -> Tuple[int, int]:
    """Return (encoded_bits, signed_value) for a conservative negative sample."""
    if width <= 1:
        return 0, 0
    max_magnitude = (1 << (width - 1)) - 1
    # 48 is small enough for narrow signed fields and avoids many common
    # binary-fraction scale leftovers (0.25, 0.0625) in negative tests.
    magnitude = min(max_magnitude, 48)
    if magnitude <= 0:
        magnitude = 1
    encoded = ((1 << width) - magnitude) & ((1 << width) - 1)
    return int(encoded), -int(magnitude)


def _stream_formula_expects_encoded_raw(
    expr: str,
    input_name: str,
    *,
    width: int,
) -> bool:
    """Heuristic: true when the formula performs its own sign-bit decode."""
    text = str(expr or "")
    if not text or not input_name:
        return False
    sign_bit = 1 << (int(width) - 1) if width > 0 else 0
    full_range = 1 << int(width) if width > 0 else 0
    name = re.escape(input_name)
    if re.search(rf"\b{name}\b\s*&", text) or re.search(rf"&\s*\b{name}\b", text):
        return True
    constants = {sign_bit, full_range}
    constants.update({hex(v) for v in constants if v})
    lowered = text.lower()
    if "sign" in lowered and input_name.lower() in lowered:
        return True
    return any(str(c).lower() in lowered for c in constants if c)


def _infer_positive_linear_scale(
    expr: str,
    input_name: str,
    *,
    width: int,
) -> Optional[Fraction]:
    if width <= 1:
        return None
    max_pos = (1 << (width - 1)) - 1
    if max_pos <= 0:
        return None
    candidates = [
        min(max_pos, 1 << max(0, width - 2)),
        min(max_pos, 1024),
        min(max_pos, 1000),
        min(max_pos, 257),
    ]
    seen: List[int] = []
    for raw in candidates:
        raw = int(raw)
        if raw <= 0 or raw in seen:
            continue
        seen.append(raw)
        value = _eval_mechanical_expression(expr, {input_name: raw})
        if value is None or value == 0:
            continue
        scale = Fraction(int(value), raw)
        ok = True
        for check_raw in seen:
            check_value = _eval_mechanical_expression(expr, {input_name: check_raw})
            if check_value is None:
                ok = False
                break
            predicted = int(scale * check_raw)
            if abs(int(check_value) - predicted) > 1:
                ok = False
                break
        if ok:
            return scale
    return None


def _signed_stream_negative_sanity_value(
    expr: str,
    input_name: str,
    *,
    width: int,
    raw_signed: int,
    computed_value: int,
) -> Optional[Tuple[int, Fraction]]:
    if raw_signed >= 0 or computed_value < 0:
        return None
    scale = _infer_positive_linear_scale(expr, input_name, width=width)
    if scale is None or scale <= 0:
        return None
    return int(scale * raw_signed), scale


_FAULT_BIT_TOKENS = (
    "fault", "error", "err", "invalid", "open", "short", "overflow",
    "overrange", "underrange", "crc", "parity", "alarm",
)


def _text_suggests_fault_bit(*parts: Any) -> bool:
    text = " ".join(str(part or "") for part in parts).lower()
    return any(tok in text for tok in _FAULT_BIT_TOKENS)


def _bit_position_candidates(value: Any) -> Tuple[int, ...]:
    if isinstance(value, bool) or value is None:
        return ()
    if isinstance(value, int):
        return (int(value),)
    if isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        out: List[int] = []
        for item in value:
            if isinstance(item, int) and not isinstance(item, bool):
                out.append(int(item))
            else:
                parsed = _parse_int_literal(item)
                if parsed is not None:
                    out.append(int(parsed))
        return tuple(out)
    parsed = _parse_int_literal(value)
    if parsed is not None:
        return (int(parsed),)
    return tuple(
        bit
        for high, low in _stream_bit_segments_from_text(value)
        if high == low
        for bit in (high,)
    )


def _stream_fault_bit_candidates(
    device_ir: Mapping[str, Any],
    *,
    frame_bits: int,
) -> Tuple[Tuple[int, str], ...]:
    candidates: List[Tuple[int, str]] = []

    bitfields = device_ir.get("bitfields") or []
    if isinstance(bitfields, Sequence) and not isinstance(bitfields, (str, bytes)):
        for bf in bitfields:
            if not isinstance(bf, Mapping):
                continue
            if not _text_suggests_fault_bit(
                bf.get("name"), bf.get("description"), bf.get("notes"),
                bf.get("meaning"), bf.get("enum_values"),
            ):
                continue
            bits: List[int] = []
            for key in ("bit_position", "bit", "position", "source_bits", "bits"):
                bits.extend(_bit_position_candidates(bf.get(key)))
            for key in ("name", "description", "notes", "meaning"):
                bits.extend(_bit_position_candidates(bf.get(key)))
            for bit in bits:
                if 0 <= int(bit) < frame_bits:
                    label = str(bf.get("name") or bf.get("condition") or f"D{bit}")
                    candidates.append((int(bit), label))

    errors = device_ir.get("error_conditions") or []
    if isinstance(errors, Sequence) and not isinstance(errors, (str, bytes)):
        for err in errors:
            if not isinstance(err, Mapping):
                continue
            if not _text_suggests_fault_bit(
                err.get("name"), err.get("condition"), err.get("description"),
                err.get("detection"), err.get("notes"), err.get("driver_action"),
            ):
                continue
            bits: List[int] = []
            for key in ("bit_position", "bit", "position", "source_bits", "detection", "notes", "description"):
                bits.extend(_bit_position_candidates(err.get(key)))
            for bit in bits:
                if 0 <= int(bit) < frame_bits:
                    label = str(err.get("name") or err.get("condition") or f"D{bit}")
                    candidates.append((int(bit), label))

    dedup: Dict[int, str] = {}
    for bit, label in candidates:
        dedup.setdefault(int(bit), label)
    return tuple(sorted(dedup.items(), reverse=True))


def _pack_stream_bit_segments(
    frame_value: int,
    *,
    frame_bits: int,
    segments: Sequence[Tuple[int, int]],
    raw_value: int,
    raw_width: int,
    used_bits: set[int],
) -> Optional[int]:
    bit_cursor = raw_width - 1
    for high, low in segments:
        if high >= frame_bits or low < 0 or high < low:
            return None
        for frame_bit in range(high, low - 1, -1):
            if frame_bit in used_bits:
                return None
            used_bits.add(frame_bit)
            bit = (int(raw_value) >> bit_cursor) & 1 if bit_cursor >= 0 else 0
            if bit:
                frame_value |= 1 << frame_bit
            bit_cursor -= 1
    if bit_cursor != -1:
        return None
    return frame_value


def _stream_frame_bytes(frame_value: int, frame_nbytes: int) -> Tuple[int, ...]:
    return tuple(
        (int(frame_value) >> (8 * (frame_nbytes - 1 - idx))) & 0xFF
        for idx in range(frame_nbytes)
    )


def _build_stream_bitfield_specs(
    device_ir: Mapping[str, Any],
    channel_ids: Sequence[str],
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Tuple[_StreamBitfieldChannelSpec, ...]:
    channels_by_id = _read_channel_by_id(device_ir)
    formulas = _formula_by_name(device_ir)
    formula_ids = _channel_formula_ids(device_ir)
    specs: List[_StreamBitfieldChannelSpec] = []
    for ch_id in channel_ids:
        source_id, channel, sem = _resolve_public_channel_source(
            ch_id,
            channels_by_id,
            output_semantics_map,
        )
        if channel is None:
            return ()
        semantic_kind = str((sem or {}).get("semantic_kind") or "").strip().lower()
        formula = formulas.get(str(channel.get("formula_id") or "")) or formulas.get(
            formula_ids.get(source_id, "")
        )
        expr, input_name, input_row = _stream_formula_input(formula)
        if semantic_kind not in {"raw_count", "status_or_code"}:
            if not expr or not input_name:
                return ()
            try:
                ast.parse(expr, mode="eval")
            except SyntaxError:
                return ()
        segments = _stream_bit_segments_for_channel(channel, input_row)
        if not segments:
            return ()
        specs.append(_StreamBitfieldChannelSpec(
            public_id=ch_id,
            source_id=str(channel.get("id") or source_id),
            channel=channel,
            bit_segments=segments,
            input_name=input_name or "raw",
            expression=expr,
            semantic_kind=semantic_kind,
        ))
    return tuple(specs)


def _generate_spi_stream_multi_channel_stimuli(
    device_ir: Mapping[str, Any],
    channel_ids: Sequence[str],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    if not _device_uses_spi_stream(device_ir):
        return ()
    frame_nbytes = _primary_stream_frame_nbytes(device_ir)
    if frame_nbytes is None or frame_nbytes <= 0:
        return ()
    specs = _build_stream_bitfield_specs(device_ir, channel_ids, output_semantics_map)
    if len(specs) != len(channel_ids):
        return ()

    frame_bits = int(frame_nbytes) * 8
    stimuli: List[Mapping[str, Any]] = []
    signed_widths = [
        _stream_bitfield_width(spec.bit_segments)
        for spec in specs
        if _channel_is_signed(spec.channel)
    ]
    sample_kinds = ["positive_1", "positive_2"]
    if any(width > 1 for width in signed_widths):
        sample_kinds.append("signed_negative")

    for stim_index, sample_kind in enumerate(sample_kinds):
        frame_value = 0
        used_bits: set[int] = set()
        expected: Dict[str, int] = {}
        channel_preload_bytes: Dict[str, List[str]] = {}
        derivation_parts: List[str] = []
        for channel_index, spec in enumerate(specs):
            width = _stream_bitfield_width(spec.bit_segments)
            if width <= 0:
                return ()
            signed = _channel_is_signed(spec.channel)
            raw_signed: Optional[int] = None
            if sample_kind == "signed_negative" and signed and width > 1:
                raw, raw_signed = _stream_negative_signed_sample(width, channel_index)
            else:
                raw = _stream_raw_pattern(
                    width=width,
                    signed=signed,
                    stim_index=stim_index,
                    channel_index=channel_index,
                )
                raw_signed = int(raw)
            packed = _pack_stream_bit_segments(
                frame_value,
                frame_bits=frame_bits,
                segments=spec.bit_segments,
                raw_value=raw,
                raw_width=width,
                used_bits=used_bits,
            )
            if packed is None:
                return ()
            frame_value = packed
            raw_bytes = _raw_to_bytes(raw, width, max(1, (width + 7) // 8))
            channel_preload_bytes[spec.public_id] = [
                _hex_byte(byte) for byte in raw_bytes
            ]

            if spec.semantic_kind in {"raw_count", "status_or_code"}:
                value = int(raw)
                derivation_parts.append(
                    f"{spec.public_id}: SECTION B3 semantic_kind={spec.semantic_kind}; "
                    f"stream bits {spec.bit_segments} encode raw={int(raw)}"
                )
            else:
                formula_raw = int(raw)
                if (
                    sample_kind == "signed_negative"
                    and signed
                    and raw_signed is not None
                    and not _stream_formula_expects_encoded_raw(
                        spec.expression,
                        spec.input_name,
                        width=width,
                    )
                ):
                    formula_raw = int(raw_signed)
                value = _eval_mechanical_expression(
                    spec.expression,
                    {spec.input_name: int(formula_raw)},
                )
                if value is None:
                    return ()
                computed_value = int(value)
                sanity_note = ""
                if sample_kind == "signed_negative" and signed and raw_signed is not None:
                    sanity = _signed_stream_negative_sanity_value(
                        spec.expression,
                        spec.input_name,
                        width=width,
                        raw_signed=int(raw_signed),
                        computed_value=int(value),
                    )
                    if sanity is not None:
                        value, scale = sanity
                        scale_expr = (
                            str(scale.numerator)
                            if scale.denominator == 1
                            else f"{scale.numerator} // {scale.denominator}"
                        )
                        expected_expr = (
                            f"({int(raw_signed)} * {scale.numerator})"
                            if scale.denominator == 1
                            else f"({int(raw_signed)} * {scale.numerator}) // {scale.denominator}"
                        )
                        sanity_note = (
                            f" (formula gives suspect {computed_value}); "
                            f"signed two's-complement sanity: encoded "
                            f"0x{int(raw):X} represents {int(raw_signed)}, "
                            f"positive-path scale {scale_expr}; "
                            f"{expected_expr} = {int(value)}"
                        )
                rendered = _replace_name(
                    spec.expression, spec.input_name, str(int(formula_raw))
                )
                expr_result_text = f"{rendered} = {computed_value}{sanity_note}"
                raw_text = f"{spec.input_name}={int(raw)}"
                if int(formula_raw) != int(raw):
                    raw_text = (
                        f"{spec.input_name}_bits=0x{int(raw):X} encodes "
                        f"{spec.input_name}={int(formula_raw)}"
                    )
                derivation_parts.append(
                    f"{spec.public_id}: stream bits {spec.bit_segments} "
                    f"from source_channel={spec.source_id} encode "
                    f"{raw_text}; {expr_result_text}"
                )
            expected[spec.public_id] = int(value)

        stream_bytes = _stream_frame_bytes(frame_value, int(frame_nbytes))
        stim_name = (
            "mechanical_spi_stream_bitfield_signed_negative"
            if sample_kind == "signed_negative"
            else f"mechanical_spi_stream_bitfield_{stim_index + 1}"
        )
        stimuli.append({
            "name": stim_name,
            "mock_preload": {"stream": [_hex_byte(byte) for byte in stream_bytes]},
            "channel_preload_bytes": channel_preload_bytes,
            "expected_channels": expected,
            "raw_tolerance": 0,
            "derivation": (
                f"SPI stream frame is MSB-first bytes "
                f"{' '.join(_hex_byte(byte) for byte in stream_bytes)}; "
                + "; ".join(derivation_parts)
            ),
        })
    fault_bits = _stream_fault_bit_candidates(device_ir, frame_bits=frame_bits)
    if fault_bits:
        fault_bit, fault_label = fault_bits[0]
        frame_value = 1 << int(fault_bit)
        stream_bytes = _stream_frame_bytes(frame_value, int(frame_nbytes))
        stimuli.append({
            "name": "mechanical_spi_stream_fault_status",
            "mock_preload": {"stream": [_hex_byte(byte) for byte in stream_bytes]},
            "expected_err": 1,
            "derivation": (
                f"Device IR marks stream bit D{int(fault_bit)} ({fault_label}) "
                "as a runtime-detectable fault/status condition; public "
                "driver read should return a nonzero error."
            ),
        })
    return tuple(stimuli)


def _single_channel_formula_scale(
    formula: Mapping[str, Any],
    public_unit: str,
    *,
    raw_encoding: Optional[Mapping[str, Any]] = None,
) -> Optional[Fraction]:
    """Return public-unit scale for simple ``raw * constant`` formula text."""
    expr_obj = formula.get("integer_approximation_expression")
    if isinstance(expr_obj, Mapping):
        expr = str(expr_obj.get("expression") or "").strip()
        input_name = _single_formula_input_name(expr_obj)
        default_bindings: Dict[str, Any] = {}
        inputs = expr_obj.get("inputs")
        if isinstance(inputs, Sequence) and not isinstance(inputs, (str, bytes)):
            expr_names = set(_expression_names(expr))
            rawish_names: List[str] = []
            for item in inputs:
                if not isinstance(item, Mapping):
                    continue
                name = str(item.get("name") or "").strip()
                if not name or (expr_names and name not in expr_names):
                    continue
                default_value = _parse_int_literal(item.get("default_value"))
                if default_value is not None:
                    default_bindings[name] = default_value
                if (
                    item.get("byte_source") is not None
                    or item.get("source_signal") is not None
                    or re.search(r"\b(raw|count|code|sample|adc)\b", name, re.IGNORECASE)
                ):
                    rawish_names.append(name)
            if not input_name and len(rawish_names) == 1:
                input_name = rawish_names[0]
        input_name = input_name or "raw"
        # Avoid double-shifting candidates when the formula already shifts input.
        _right_shift = (
            _parse_int_literal(raw_encoding.get("right_shift"))
            if isinstance(raw_encoding, Mapping)
            else None
        )
        _expr_has_input_shift = bool(
            _right_shift
            and _right_shift > 0
            and re.search(
                r"\b" + re.escape(input_name) + r"\b\s*>>",
                expr,
            )
        )
        _pre_shift = (
            (1 << _right_shift)
            if _expr_has_input_shift
            else 1
        )
        for candidate in (400, 160, 100, 16, 1):
            bindings = dict(default_bindings)
            bindings[input_name] = candidate * _pre_shift
            expected = _eval_mechanical_expression(expr, bindings)
            if expected is not None:
                return Fraction(int(expected), candidate)

    text = str(formula.get("formula") or "")
    match = re.search(
        r"\braw(?:_[A-Za-z0-9]+)*\b\s*\*\s*([0-9]+(?:\.[0-9]+)?)",
        text,
        re.IGNORECASE,
    )
    if match is None:
        match = re.search(
            r"([0-9]+(?:\.[0-9]+)?)\s*\*\s*\braw(?:_[A-Za-z0-9]+)*\b",
            text,
            re.IGNORECASE,
        )
    if match is None:
        return None
    try:
        scale = Fraction(match.group(1))
    except ValueError:
        return None
    unit_key = _unit_key(public_unit)
    text_key = _unit_key(text)
    if unit_key.startswith("milli_") and (
        "degc" in text_key or "degree_c" in text_key or "_c_" in f"_{text_key}_"
    ):
        scale *= 1000
    elif unit_key.startswith("micro_"):
        scale *= 1000000
    return scale


def _single_channel_formula_from_output_semantics(
    row: Optional[Mapping[str, Any]],
) -> Optional[Mapping[str, Any]]:
    if not isinstance(row, Mapping):
        return None
    parts = [
        row.get("formula"),
        row.get("conversion_formula"),
        row.get("formula_text"),
        row.get("evidence"),
        row.get("notes"),
    ]
    text = " ".join(str(part or "") for part in parts).strip()
    if not text:
        return None
    if re.search(r"\braw(?:_[A-Za-z0-9]+)*\b\s*\*", text, re.IGNORECASE) is None:
        return None
    return {"formula": text}


def _single_channel_context_text(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
) -> str:
    parts: List[Any] = [channel]
    channel_id = str(channel.get("id") or "")
    flow_id = str(channel.get("flow_id") or channel.get("read_flow_id") or "")
    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            flow_channels = flow.get("channels") or []
            channel_match = (
                flow_id and str(flow.get("flow_id") or "") == flow_id
            ) or (
                channel_id
                and isinstance(flow_channels, Sequence)
                and not isinstance(flow_channels, (str, bytes))
                and channel_id in {str(item) for item in flow_channels}
            )
            if channel_match:
                parts.append(flow)
    registers = device_ir.get("registers_or_commands") or []
    if isinstance(registers, Sequence) and not isinstance(registers, (str, bytes)):
        parts.extend(item for item in registers if isinstance(item, Mapping))
    return json.dumps(parts, ensure_ascii=False).lower()


def _infer_single_channel_raw_encoding(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
    *,
    nbytes: int,
) -> Dict[str, Any]:
    inferred: Dict[str, Any] = {}
    bit_width = max(1, int(nbytes) * 8)
    source = " ".join(str(item or "") for item in (channel.get("source_bytes") or [])).lower()
    if "msb" in source and "lsb" in source:
        inferred["byte_order"] = (
            "big_endian"
            if source.find("msb") < source.find("lsb")
            else "little_endian"
        )
    elif "high" in source and "low" in source:
        inferred["byte_order"] = (
            "big_endian"
            if source.find("high") < source.find("low")
            else "little_endian"
        )

    text = _single_channel_context_text(device_ir, channel)
    if "msb first" in text or "high byte then low byte" in text:
        inferred["byte_order"] = "big_endian"
    elif "lsb first" in text or "low byte then high byte" in text:
        inferred["byte_order"] = "little_endian"

    if re.search(r"(?:two'?s|2'?s)\s+complement", text):
        inferred["signed"] = True

    effective_bits: Optional[int] = None
    msb_match = re.search(
        r"(?:only\s+)?(\d{1,2})\s*(?:msb|most\s+significant)\s+bits?\s+significant",
        text,
    )
    if msb_match is not None:
        effective_bits = int(msb_match.group(1))
    else:
        twos_match = re.search(
            r"\b(\d{1,2})\s*[- ]?bits?\s+(?:two'?s|2'?s)\s+complement",
            text,
        )
        if twos_match is not None:
            effective_bits = int(twos_match.group(1))

    if effective_bits is not None and 0 < effective_bits <= bit_width:
        inferred["bit_width"] = bit_width
        inferred["effective_bits"] = effective_bits
        inferred["sign_extend_from_bit"] = effective_bits - 1
        if (
            effective_bits < bit_width
            and (
                "msb" in text
                or "most significant" in text
                or "left justified" in text
                or "left-justified" in text
                or "left aligned" in text
                or "left-aligned" in text
            )
        ):
            inferred["right_shift"] = bit_width - effective_bits
    elif bit_width > 0:
        inferred["bit_width"] = bit_width
    return inferred


def _effective_single_channel_raw_encoding(
    raw_encoding: Mapping[str, Any],
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
    *,
    nbytes: int,
) -> Dict[str, Any]:
    out = dict(raw_encoding)
    inferred = _infer_single_channel_raw_encoding(
        device_ir,
        channel,
        nbytes=nbytes,
    )
    for key, value in inferred.items():
        out.setdefault(key, value)
    return out


def _single_channel_encoded_bytes(
    *,
    raw_value: int,
    raw_encoding: Mapping[str, Any],
    nbytes: int,
) -> Tuple[int, ...]:
    effective_bits = _parse_int_literal(raw_encoding.get("effective_bits"))
    if effective_bits is None:
        effective_bits = _parse_int_literal(raw_encoding.get("bit_width"))
    if effective_bits is None or effective_bits <= 0:
        effective_bits = nbytes * 8
    right_shift = _parse_int_literal(raw_encoding.get("right_shift")) or 0
    raw_mask = (1 << effective_bits) - 1
    encoded_field = int(raw_value) & raw_mask
    register_value = (encoded_field << max(0, right_shift)) & ((1 << (nbytes * 8)) - 1)
    order = str(raw_encoding.get("byte_order") or "").lower()
    if "little" in order or "lsb" in order:
        return tuple((register_value >> (8 * idx)) & 0xFF for idx in range(nbytes))
    return tuple(
        (register_value >> (8 * (nbytes - 1 - idx))) & 0xFF
        for idx in range(nbytes)
    )


_GPIO_PULSE_DURATION_US_CANDIDATES: Tuple[int, ...] = (
    400,
    580,
    1000,
    1600,
    5800,
)


def _expression_names(expr: str) -> Tuple[str, ...]:
    try:
        tree = ast.parse(expr, mode="eval")
    except SyntaxError:
        return ()
    names = sorted({
        node.id for node in ast.walk(tree)
        if isinstance(node, ast.Name) and node.id
    })
    return tuple(names)


def _single_formula_input_name_for_mechanical(expr_obj: Mapping[str, Any]) -> str:
    explicit = _single_formula_input_name(expr_obj)
    if explicit:
        return explicit
    expr = str(expr_obj.get("expression") or "").strip()
    names = _expression_names(expr)
    return names[0] if len(names) == 1 else ""


def _device_uses_gpio_pulse_measurement(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
) -> bool:
    bus_text = " ".join(
        str(part or "")
        for part in (
            device_ir.get("bus_type"),
            (device_ir.get("access_model") or {}).get("kind")
            if isinstance(device_ir.get("access_model"), Mapping) else "",
            (device_ir.get("access_model") or {}).get("notes")
            if isinstance(device_ir.get("access_model"), Mapping) else "",
        )
    ).lower()
    if "gpio" not in bus_text:
        return False

    source_signal = str(channel.get("source_signal") or "").lower()
    if any(token in source_signal for token in ("pulse", "duration", "width", "echo")):
        return True

    flow_ids = {
        str(channel.get("flow_id") or ""),
        str(channel.get("read_flow_id") or ""),
    }
    flows = device_ir.get("operation_flows") or []
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return False
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        flow_id = str(flow.get("flow_id") or "")
        flow_channels = flow.get("channels") or []
        channel_match = (
            (flow_id and flow_id in flow_ids)
            or (
                isinstance(flow_channels, Sequence)
                and not isinstance(flow_channels, (str, bytes))
                and str(channel.get("id") or "") in {str(ch) for ch in flow_channels}
            )
        )
        if not channel_match:
            continue
        text = json.dumps(flow, ensure_ascii=False).lower()
        if "measure_pulse" in text:
            return True
        if "pulse" in text and any(token in text for token in ("width", "duration", "echo", "high")):
            return True
    return False


def _generate_gpio_pulse_single_channel_stimuli(
    device_ir: Mapping[str, Any],
    api_contract: Mapping[str, Any],
) -> Tuple[Mapping[str, Any], ...]:
    """Generate GPIO pulse-width stimuli from executable IR timing formulae."""
    if str(api_contract.get("eval_class") or "") != EVAL_CLASS_SINGLE_CHANNEL:
        return ()
    channel = _first_read_channel(device_ir)
    if not isinstance(channel, Mapping):
        return ()
    if not _device_uses_gpio_pulse_measurement(device_ir, channel):
        return ()

    formula = _formula_by_name(device_ir).get(str(channel.get("formula_id") or ""))
    if not isinstance(formula, Mapping):
        return ()
    expr_obj = formula.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return ()
    expr = str(expr_obj.get("expression") or "").strip()
    input_name = _single_formula_input_name_for_mechanical(expr_obj)
    if not expr or not input_name:
        return ()

    chosen: Optional[Tuple[int, int]] = None
    for duration_us in _GPIO_PULSE_DURATION_US_CANDIDATES:
        expected = _eval_mechanical_expression(expr, {input_name: duration_us})
        if expected is None or expected <= 0:
            continue
        if abs(expected) > 10_000_000:
            continue
        chosen = (duration_us, expected)
        break
    if chosen is None:
        return ()

    duration_us, expected = chosen
    expr_with_value = re.sub(
        rf"\b{re.escape(input_name)}\b",
        str(duration_us),
        expr,
    )
    signal = str(channel.get("source_signal") or "pulse_width_us")
    return ({
        "name": "mechanical_gpio_pulse_width_positive",
        "mock_preload": {
            "schedule": [[1, duration_us], [0, 1]],
        },
        "expected_read_raw": expected,
        "raw_tolerance": 3,
        "derivation": (
            f"GPIO pulse schedule drives {signal} high for {duration_us} us; "
            f"formula input {input_name}={duration_us}; "
            f"{expr_with_value} = {expected}."
        ),
    },)


def _device_uses_gpio_byte_frame(device_ir: Mapping[str, Any]) -> bool:
    bus_text = " ".join(
        str(part or "")
        for part in (
            device_ir.get("bus_type"),
            device_ir.get("bus"),
            device_ir.get("gpio_protocol_hint"),
            (device_ir.get("access_model") or {}).get("kind")
            if isinstance(device_ir.get("access_model"), Mapping) else "",
            (device_ir.get("access_model") or {}).get("notes")
            if isinstance(device_ir.get("access_model"), Mapping) else "",
        )
    ).lower()
    if "gpio" not in bus_text and "one-wire" not in bus_text and "1-wire" not in bus_text:
        return False

    text = json.dumps(device_ir, ensure_ascii=False).lower()
    has_frame_hint = any(
        token in text
        for token in (
            "byte frame",
            "data frame",
            "scratchpad",
            "payload",
            "40-bit",
            "40 bit",
            "source_bytes",
            "byte_source",
            "checksum",
        )
    )
    if not has_frame_hint:
        return False
    if "measure_pulse" in text and not any(
        token in text for token in ("byte_source", "source_bytes", "checksum", "40-bit", "40 bit")
    ):
        return False
    return True


def _source_names_from_byte_source(byte_source: Any) -> Tuple[str, ...]:
    if not isinstance(byte_source, str) or not byte_source.strip():
        return ()
    names: List[str] = []
    for part in re.split(r"\|\||[,;/]+", byte_source):
        match = re.search(r"\b([A-Za-z_][A-Za-z0-9_]*)\b\s*(?::\s*\d+)?", part)
        if not match:
            continue
        name = match.group(1).strip()
        if name and _normalise_identifier(name) not in {"bit", "bits", "byte", "bytes"}:
            names.append(name)
    return tuple(dict.fromkeys(names))


def _flow_output_source_names_for_channel(
    device_ir: Mapping[str, Any],
    channel_id: str,
    flow_id: str = "",
) -> Tuple[str, ...]:
    target = _normalise_identifier(channel_id)
    flows = device_ir.get("operation_flows") or []
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return ()
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        if flow_id and str(flow.get("flow_id") or "").strip() != flow_id:
            continue
        outputs = flow.get("outputs") or []
        if not isinstance(outputs, Sequence) or isinstance(outputs, (str, bytes)):
            continue
        for output in outputs:
            if not isinstance(output, Mapping):
                continue
            if _normalise_identifier(str(output.get("channel") or "")) != target:
                continue
            names = _source_names_from_byte_source(output.get("byte_source"))
            if names:
                return names
    return ()


def _formula_input_specs(
    formula: Optional[Mapping[str, Any]],
) -> Tuple[str, Tuple[Mapping[str, Any], ...]]:
    if not isinstance(formula, Mapping):
        return "", ()
    expr_obj = formula.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return "", ()
    expr = str(expr_obj.get("expression") or "").strip()
    raw_inputs = expr_obj.get("inputs") or []
    inputs: List[Mapping[str, Any]] = []
    if isinstance(raw_inputs, Sequence) and not isinstance(raw_inputs, (str, bytes)):
        inputs = [item for item in raw_inputs if isinstance(item, Mapping)]
    return expr, tuple(inputs)


def _formula_source_names(formula: Optional[Mapping[str, Any]]) -> Tuple[str, ...]:
    _expr, inputs = _formula_input_specs(formula)
    names: List[str] = []
    fallback_names: List[str] = []
    for item in inputs:
        parsed = _source_names_from_byte_source(item.get("byte_source"))
        names.extend(parsed)
        input_name = str(item.get("name") or "").strip()
        if input_name and not _normalise_identifier(input_name).startswith("raw"):
            fallback_names.append(input_name)
    return tuple(dict.fromkeys(names or fallback_names))


def _gpio_byte_frame_source_names_for_channel(
    device_ir: Mapping[str, Any],
    channel: Mapping[str, Any],
    formula: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    sources = channel.get("source_bytes")
    if isinstance(sources, Sequence) and not isinstance(sources, (str, bytes)):
        names = [str(src or "").strip() for src in sources if str(src or "").strip()]
        if names:
            return tuple(names)

    channel_id = str(channel.get("id") or "").strip()
    flow_id = str(channel.get("flow_id") or "").strip()
    names = _flow_output_source_names_for_channel(device_ir, channel_id, flow_id)
    if names:
        return names
    return _formula_source_names(formula)


def _gpio_byte_frame_checksum_byte(
    device_ir: Mapping[str, Any],
    payload: Sequence[int],
) -> Optional[int]:
    text = json.dumps(device_ir, ensure_ascii=False).lower()
    if "checksum" not in text:
        return None
    if "crc" in text and "sum" not in text:
        return None
    if not any(token in text for token in ("sum", "add", "modulo", "mod 256", "8-bit")):
        return None
    if not payload:
        return None
    return sum(int(byte) & 0xFF for byte in payload) & 0xFF


def _gpio_byte_frame_channel_order(
    device_ir: Mapping[str, Any],
    channel_ids: Sequence[str],
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    channels_by_id = _read_channel_by_id(device_ir)
    source_to_public: Dict[str, str] = {}
    public_to_public: Dict[str, str] = {}
    for public_id in channel_ids:
        source_id, channel, _sem = _resolve_public_channel_source(
            public_id,
            channels_by_id,
            output_semantics_map,
        )
        public_to_public[_normalise_identifier(public_id)] = public_id
        source_to_public[_normalise_identifier(source_id or public_id)] = public_id
        if isinstance(channel, Mapping):
            source_to_public[_normalise_identifier(str(channel.get("id") or public_id))] = public_id

    ordered: List[str] = []
    flows = device_ir.get("operation_flows") or []
    if isinstance(flows, Sequence) and not isinstance(flows, (str, bytes)):
        for flow in flows:
            if not isinstance(flow, Mapping):
                continue
            for output in flow.get("outputs") or []:
                if not isinstance(output, Mapping):
                    continue
                key = _normalise_identifier(str(output.get("channel") or ""))
                public_id = source_to_public.get(key) or public_to_public.get(key)
                if public_id and public_id not in ordered:
                    ordered.append(public_id)
            for flow_channel in flow.get("channels") or []:
                key = _normalise_identifier(str(flow_channel or ""))
                public_id = source_to_public.get(key) or public_to_public.get(key)
                if public_id and public_id not in ordered:
                    ordered.append(public_id)

    for public_id in channel_ids:
        if public_id not in ordered:
            ordered.append(public_id)
    return tuple(ordered)


def _gpio_byte_frame_raw_sample(
    *,
    width: int,
    stim_index: int,
    channel_index: int,
) -> int:
    max_raw = (1 << max(1, min(width, 30))) - 1
    base = 600 if stim_index == 0 else 455
    raw = base + channel_index * 37
    return max(0, min(raw, max_raw))


def _formula_result_public_unit_scale(
    formula: Optional[Mapping[str, Any]],
    output_semantics: Optional[Mapping[str, Any]],
) -> Fraction:
    if not isinstance(formula, Mapping) or not isinstance(output_semantics, Mapping):
        return Fraction(1, 1)
    kind = str(output_semantics.get("semantic_kind") or "").strip().lower()
    if not semantic_kind_is_physical(kind):
        return Fraction(1, 1)

    public_unit = str(output_semantics.get("public_unit") or "").strip()
    public_key = _unit_key(public_unit)
    if not public_key:
        return Fraction(1, 1)

    expr_obj = formula.get("integer_approximation_expression")
    output: Mapping[str, Any] = {}
    expr = ""
    if isinstance(expr_obj, Mapping):
        expr = str(expr_obj.get("expression") or "").strip()
        raw_output = expr_obj.get("output")
        if isinstance(raw_output, Mapping):
            output = raw_output
    formula_unit = str(output.get("unit") or formula.get("unit") or "").strip()
    formula_key = _unit_key(formula_unit)
    if formula_key and formula_key == public_key:
        return Fraction(1, 1)

    descriptor = " ".join(
        str(part or "").lower()
        for part in (
            output.get("name"),
            output.get("unit"),
            formula.get("formula"),
            expr,
        )
    )
    is_tenths = any(token in descriptor for token in ("tenth", "tenths", "_x10", " x10", "/10"))
    is_centi = any(token in descriptor for token in ("centi", "hundredth", "hundredths", "_x100", " x100", "/100"))

    if public_key.startswith("milli_") and not formula_key.startswith("milli_"):
        if is_centi:
            return Fraction(10, 1)
        if is_tenths:
            return Fraction(100, 1)
        return Fraction(1000, 1)
    if public_key.startswith("micro_") and not formula_key.startswith("micro_"):
        if is_centi:
            return Fraction(10000, 1)
        if is_tenths:
            return Fraction(100000, 1)
        return Fraction(1000000, 1)
    return Fraction(1, 1)


def _render_scaled_expression(expr: str, scale: Fraction) -> str:
    if scale == 1:
        return expr
    if scale.denominator == 1:
        return f"({expr}) * {scale.numerator}"
    return f"(({expr}) * {scale.numerator}) // {scale.denominator}"


def _eval_gpio_byte_frame_channel(
    *,
    channel_id: str,
    channel: Mapping[str, Any],
    formula: Optional[Mapping[str, Any]],
    source_names: Sequence[str],
    raw_bytes: Sequence[int],
    semantic_kind: str,
    output_semantics: Optional[Mapping[str, Any]],
) -> Optional[Tuple[int, str]]:
    width = _channel_bit_width(channel, len(source_names))
    signed = _channel_is_signed(channel)
    raw_expr = _raw_expression_from_bytes(raw_bytes, signed=signed, width=width)
    raw_value = _raw_value_from_bytes(raw_bytes, signed=signed, width=width)
    encoded_raw_expr = _raw_expression_from_bytes(raw_bytes, signed=False, width=width)
    encoded_raw_value = _raw_value_from_bytes(raw_bytes, signed=False, width=width)

    if semantic_kind in {"raw_count", "status_or_code"}:
        return int(raw_value), (
            f"{channel_id}: SECTION B3 semantic_kind={semantic_kind}; "
            f"public output is unconverted raw/code value; {raw_expr} = {int(raw_value)}"
        )

    expr, inputs = _formula_input_specs(formula)
    if not expr:
        if semantic_kind_is_physical(semantic_kind):
            return None
        return int(raw_value), (
            f"{channel_id}: raw fallback because conversion expression is missing; "
            f"{raw_expr} = {int(raw_value)}"
        )

    names: Dict[str, int] = {
        "raw": int(raw_value),
        "raw_value": int(raw_value),
    }
    replacements: Dict[str, str] = {
        "raw": raw_expr,
        "raw_value": raw_expr,
    }
    if len(raw_bytes) >= 2:
        names["high_byte"] = int(raw_bytes[0]) & 0xFF
        names["low_byte"] = int(raw_bytes[-1]) & 0xFF
        replacements["high_byte"] = _hex_byte(raw_bytes[0])
        replacements["low_byte"] = _hex_byte(raw_bytes[-1])

    source_values = {
        _normalise_identifier(name): int(raw_bytes[idx]) & 0xFF
        for idx, name in enumerate(source_names)
        if idx < len(raw_bytes)
    }
    source_display = {
        _normalise_identifier(name): _hex_byte(raw_bytes[idx])
        for idx, name in enumerate(source_names)
        if idx < len(raw_bytes)
    }
    for name in source_names:
        key = _normalise_identifier(name)
        if key in source_values:
            names[name] = source_values[key]
            replacements[name] = source_display[key]

    for item in inputs:
        input_name = str(item.get("name") or "").strip()
        if not input_name:
            continue
        byte_names = _source_names_from_byte_source(item.get("byte_source"))
        selected = [
            source_values[_normalise_identifier(name)]
            for name in byte_names
            if _normalise_identifier(name) in source_values
        ]
        if len(selected) >= 2:
            value = _raw_value_from_bytes(selected, signed=signed, width=len(selected) * 8)
            unsigned_value = _raw_value_from_bytes(selected, signed=False, width=len(selected) * 8)
            value_expr = _raw_expression_from_bytes(selected, signed=signed, width=len(selected) * 8)
            unsigned_expr = _raw_expression_from_bytes(selected, signed=False, width=len(selected) * 8)
            if any(token in expr for token in ("&", "|", "^", "<<", ">>", "~")):
                value = unsigned_value
                value_expr = unsigned_expr
            names[input_name] = int(value)
            replacements[input_name] = value_expr
            continue
        if len(selected) == 1:
            names[input_name] = int(selected[0]) & 0xFF
            replacements[input_name] = _hex_byte(selected[0])
            continue
        input_key = _normalise_identifier(input_name)
        if input_key.startswith("raw"):
            if any(token in expr for token in ("&", "|", "^", "<<", ">>", "~")):
                names[input_name] = int(encoded_raw_value)
                replacements[input_name] = encoded_raw_expr
            else:
                names[input_name] = int(raw_value)
                replacements[input_name] = raw_expr
            continue
        if input_key in source_values:
            names[input_name] = source_values[input_key]
            replacements[input_name] = source_display[input_key]
            continue
        if len(raw_bytes) >= 2 and any(token in input_key for token in ("high", "hi", "msb")):
            names[input_name] = int(raw_bytes[0]) & 0xFF
            replacements[input_name] = _hex_byte(raw_bytes[0])
            continue
        if len(raw_bytes) >= 2 and any(token in input_key for token in ("low", "lo", "lsb")):
            names[input_name] = int(raw_bytes[-1]) & 0xFF
            replacements[input_name] = _hex_byte(raw_bytes[-1])
            continue

    try:
        ast.parse(expr, mode="eval")
    except SyntaxError:
        return None
    value = _eval_mechanical_expression(expr, names)
    if value is None:
        return None

    rendered = expr
    for name in sorted(replacements, key=len, reverse=True):
        if _expr_uses_name(rendered, name):
            rendered = _replace_name(rendered, name, replacements[name])

    scale = _formula_result_public_unit_scale(formula, output_semantics)
    if scale != 1:
        scaled = Fraction(int(value), 1) * scale
        if scaled.denominator != 1:
            return None
        scaled_value = int(scaled)
        scaled_expr = _render_scaled_expression(rendered, scale)
        public_unit = str((output_semantics or {}).get("public_unit") or "").strip()
        return scaled_value, (
            f"{channel_id}: {raw_expr} = {int(raw_value)}; {rendered} = {int(value)}; "
            f"SECTION B3 public_unit {public_unit}: {scaled_expr} = {scaled_value}"
        )
    return int(value), (
        f"{channel_id}: {raw_expr} = {int(raw_value)}; {rendered} = {int(value)}"
    )


def _generate_gpio_byte_frame_multi_channel_stimuli(
    device_ir: Mapping[str, Any],
    channel_ids: Sequence[str],
    formula_ids: Mapping[str, str],
    formulas: Mapping[str, Mapping[str, Any]],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    if not channel_ids or not _device_uses_gpio_byte_frame(device_ir):
        return ()

    channels_by_id = _read_channel_by_id(device_ir)
    ordered_channel_ids = _gpio_byte_frame_channel_order(
        device_ir,
        channel_ids,
        output_semantics_map,
    )

    stimuli: List[Mapping[str, Any]] = []
    for stim_index in range(2):
        payload: List[int] = []
        expected: Dict[str, int] = {}
        channel_preload_bytes: Dict[str, List[str]] = {}
        derivation_parts: List[str] = []
        for channel_index, public_id in enumerate(ordered_channel_ids):
            source_id, channel, sem = _resolve_public_channel_source(
                public_id,
                channels_by_id,
                output_semantics_map,
            )
            if channel is None:
                return ()
            formula = formulas.get(formula_ids.get(source_id, "")) or formulas.get(formula_ids.get(public_id, ""))
            source_names = _gpio_byte_frame_source_names_for_channel(
                device_ir,
                channel,
                formula,
            )
            if not source_names:
                return ()
            width = _channel_bit_width(channel, len(source_names))
            raw = _gpio_byte_frame_raw_sample(
                width=width,
                stim_index=stim_index,
                channel_index=channel_index,
            )
            raw_bytes = _raw_to_bytes(raw, width, len(source_names))
            semantic_kind = _semantic_kind_for_mechanical_channel(
                output_semantics_map,
                public_id=public_id,
                source_id=source_id,
            )
            evaluated = _eval_gpio_byte_frame_channel(
                channel_id=public_id,
                channel=channel,
                formula=formula,
                source_names=source_names,
                raw_bytes=raw_bytes,
                semantic_kind=semantic_kind,
                output_semantics=sem,
            )
            if evaluated is None:
                return ()
            value, derivation = evaluated
            payload.extend(int(byte) & 0xFF for byte in raw_bytes)
            channel_preload_bytes[public_id] = [_hex_byte(byte) for byte in raw_bytes]
            expected[public_id] = int(value)
            derivation_parts.append(derivation)

        checksum = _gpio_byte_frame_checksum_byte(device_ir, payload)
        if checksum is not None:
            payload.append(checksum)
            derivation_parts.append(
                "checksum: sum payload data bytes modulo 256 = "
                f"{_hex_byte(checksum)}"
            )
        if not payload:
            return ()
        stimuli.append({
            "name": f"mechanical_gpio_byte_frame_{stim_index + 1}",
            "mock_preload": {"payload": [_hex_byte(byte) for byte in payload]},
            "channel_preload_bytes": channel_preload_bytes,
            "expected_channels": expected,
            "raw_tolerance": 0,
            "derivation": "; ".join(derivation_parts),
        })
    return tuple(stimuli)


def _generate_mechanical_uart_stimuli(
    device_ir: Mapping[str, Any],
    api_contract: Mapping[str, Any],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    """Generate self-test stimuli for UART packet-based sensors."""
    # Only for UART bus type
    bus = str(device_ir.get("bus_type") or "").strip().lower()
    if bus != "uart":
        return ()

    eval_class = str(api_contract.get("eval_class") or "")
    if eval_class not in (EVAL_CLASS_SINGLE_CHANNEL, EVAL_CLASS_MULTI_CHANNEL):
        return ()

    flows = device_ir.get("operation_flows")
    if not isinstance(flows, list) or not flows:
        return ()

    # Find write+read flows (UART command/response pairs)
    read_flows: List[Mapping[str, Any]] = []
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        steps = flow.get("steps")
        if not isinstance(steps, list):
            continue
        has_write = False
        has_read = False
        for s in steps:
            if not isinstance(s, Mapping):
                continue
            txn = s.get("transaction")
            if isinstance(txn, Mapping) and str(txn.get("kind") or "").strip() == "write":
                has_write = True
            if isinstance(txn, Mapping) and str(txn.get("kind") or "").strip() == "read":
                has_read = True
        if has_write and has_read:
            read_flows.append(flow)

    if not read_flows:
        return ()

    # Get formula info
    channels = device_ir.get("read_channels") or []
    formulas = _formula_by_name(device_ir)
    fid_map = _channel_formula_ids(device_ir)

    # Find usable channels with integer formulas
    usable: List[Tuple[str, str, str]] = []  # (ch_id, formula_expr, input_name)
    for ch in channels:
        if not isinstance(ch, Mapping):
            continue
        ch_id = str(ch.get("id") or "")
        fid = fid_map.get(ch_id, "") or ch.get("formula_id", "")
        formula = formulas.get(fid)
        if not isinstance(formula, Mapping):
            continue
        ie = formula.get("integer_approximation_expression")
        if not isinstance(ie, Mapping):
            continue
        expr = str(ie.get("expression") or "").strip()
        if not expr:
            continue
        # Collect formula input names, preferring high_byte/low_byte split
        # over the generic "raw" label when they exist.
        input_names = [
            str(inp.get("name") or "").strip()
            for inp in (ie.get("inputs") or [])
            if isinstance(inp, Mapping) and inp.get("name")
        ]
        if not input_names:
            input_name = _single_formula_input_name(ie) or "raw"
            input_names = [input_name]
        usable.append((ch_id, expr, input_names))

    if not usable:
        return ()

    # Use the response frame from the first read flow as template
    flow_stimuli: List[dict] = []
    for flow in read_flows:
        steps = flow.get("steps") or []
        write_bytes: Optional[List[int]] = None
        resp_len: Optional[int] = None

        for s in steps:
            if not isinstance(s, Mapping):
                continue
            txn = s.get("transaction")
            if not isinstance(txn, Mapping):
                continue
            kind = str(txn.get("kind") or "").strip()
            if kind == "write":
                bts = _literal_prefix_from_transaction(txn)
                if bts:
                    write_bytes = [int(b, 0) for b in bts] if isinstance(bts[0], str) else list(bts)
            elif kind == "read":
                resp_len = txn.get("length")

        if write_bytes is None or resp_len is None or resp_len < 4:
            continue

        # Check that write frame uses two's-complement checksum
        if not _looks_like_twos_complement_uart_frame(write_bytes):
            continue

        # Build representative byte-pattern stimuli mechanically.
        ch_id, expr, input_names = usable[0]
        stimuli: List[dict] = []

        # Payload byte pairs cover low, middle, and high ranges.
        payloads = [(0x01, 0xF4), (0x07, 0xD0), (0x13, 0x88)]
        labels = ["low", "mid", "high"]

        for idx, ((hi, lo), label) in enumerate(zip(payloads, labels)):
            # Evaluate with the formula variable names from the IR.
            var_map: Dict[str, int] = {}
            if len(input_names) >= 2:
                var_map[input_names[0]] = hi
                var_map[input_names[1]] = lo
            elif len(input_names) == 1:
                var_map[input_names[0]] = (hi << 8) | lo
            target = _eval_mechanical_expression(expr, var_map)
            if target is None:
                continue

            # Build response frame: start byte + echoed command + data + checksum
            # The command byte at write_bytes[2] is echoed in the response.
            cmd_echo = 0x86
            if len(write_bytes) >= 3:
                cmd_echo = write_bytes[2]
            resp = [0xFF, cmd_echo, hi, lo, 0, 0, 0, 0, 0]
            while len(resp) < resp_len:
                resp.insert(-1, 0)
            resp = resp[:resp_len]
            if len(resp) >= 4:
                resp[-1] = (-sum(b & 0xFF for b in resp[1:-1])) & 0xFF

            # Map the full request frame to the queued response.
            req_hex = "".join(f"{b:02X}" for b in write_bytes)
            mock_preload: Dict[str, Any] = {
                f"req_{req_hex}": resp,
                "payload": [hi, lo],
            }
            stimuli.append({
                "name": f"mechanical_uart_{label}_{idx + 1}",
                "mock_preload": mock_preload,
                "expected_read_raw": target,
                "expected_channels": {ch_id: target},
                "raw_tolerance": 5,
                "derivation": (
                    f"{ch_id}: {expr} with "
                    f"{', '.join(input_names)}="
                    f"{', '.join(str(var_map.get(n, '?')) for n in input_names)}"
                ),
            })

        if stimuli:
            return tuple(stimuli)

    return ()


def _generate_mechanical_single_channel_stimuli(
    device_ir: Mapping[str, Any],
    api_contract: Mapping[str, Any],
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    if str(api_contract.get("eval_class") or "") != EVAL_CLASS_SINGLE_CHANNEL:
        return ()
    row = next(
        (
            item for item in (output_semantics_map or {}).get("channels", [])
            if isinstance(item, Mapping)
        ),
        None,
    ) if isinstance(output_semantics_map, Mapping) else None
    kind = str((row or {}).get("semantic_kind") or primary_semantic_kind(output_semantics_map)).strip().lower()
    if not semantic_kind_is_physical(kind):
        return ()
    public_unit = str(
        (row or {}).get("public_unit")
        or api_contract.get("primary_raw_unit")
        or api_contract.get("physical_unit")
        or api_contract.get("unit")
        or ""
    ).strip()
    channel = _first_read_channel(device_ir)
    if not isinstance(channel, Mapping):
        return ()
    formulas = _formula_by_name(device_ir)
    formula = formulas.get(str(channel.get("formula_id") or ""))
    if not isinstance(formula, Mapping):
        formula = _single_channel_formula_from_output_semantics(row)
    if not isinstance(formula, Mapping):
        return ()

    nbytes = _single_channel_read_nbytes(device_ir, channel)
    raw_encoding = _effective_single_channel_raw_encoding(
        device_ir.get("raw_encoding") if isinstance(device_ir.get("raw_encoding"), Mapping) else {},
        device_ir,
        channel,
        nbytes=nbytes,
    )
    scale = _single_channel_formula_scale(
        formula, public_unit, raw_encoding=raw_encoding,
    )
    if scale is None:
        return ()
    effective_bits = _parse_int_literal(raw_encoding.get("effective_bits"))
    if effective_bits is None:
        effective_bits = _parse_int_literal(raw_encoding.get("bit_width"))
    if effective_bits is None or effective_bits <= 0:
        effective_bits = nbytes * 8
    signed = _raw_type_is_signed(channel.get("raw_type"), raw_encoding)
    max_raw = (1 << (effective_bits - 1)) - 1 if signed else (1 << effective_bits) - 1
    candidates = (400, 160, 100, 80, 64, 16, 1)
    chosen: Optional[Tuple[int, int]] = None
    for raw_value in candidates:
        if raw_value <= 0 or raw_value > max_raw:
            continue
        expected_fraction = scale * raw_value
        if expected_fraction.denominator != 1:
            continue
        chosen = (raw_value, int(expected_fraction))
        break

    # Zero stimulus — always valid (0 * scale = 0).
    zero_chosen: Optional[Tuple[int, int]] = (0, 0)

    # Negative stimulus for signed sensors — use the negation of the
    # positive candidate when it stays within the signed range.
    neg_chosen: Optional[Tuple[int, int]] = None
    if signed and chosen is not None:
        neg_raw = -chosen[0]
        if neg_raw >= -(max_raw + 1) and neg_raw <= max_raw:
            neg_expected = int(scale * neg_raw)
            if scale.denominator * neg_raw % 1 == 0:
                neg_chosen = (neg_raw, neg_expected)

    if chosen is None:
        return ()

    def _make_stimulus(
        raw_value: int,
        expected: int,
        name: str,
        label: str,
    ):
        raw_bytes = _single_channel_encoded_bytes(
            raw_value=raw_value,
            raw_encoding=raw_encoding,
            nbytes=nbytes,
        )
        raw_expr = _raw_integer_derivation_from_bytes(
            raw_bytes,
            channel=channel,
            raw_encoding=raw_encoding,
        )
        if raw_expr is None:
            return None
        expr_text, decoded_raw = raw_expr
        if decoded_raw != raw_value:
            return None

        if scale.denominator == 1:
            scale_expr = f"{raw_value} * {scale.numerator}"
        else:
            scale_expr = f"({raw_value} * {scale.numerator}) // {scale.denominator}"
        preload_key = _single_channel_preload_key(device_ir)
        preload = _merge_identity_probe_preloads(
            device_ir,
            {preload_key: [_hex_byte(byte) for byte in raw_bytes]},
        )
        return {
            "name": f"mechanical_single_channel_{name}",
            "mock_preload": preload,
            "expected_read_raw": expected,
            "raw_tolerance": 0,
            "derivation": (
                f"SECTION B3 semantic_kind={kind}; {label} bytes "
                f"{' '.join(_hex_byte(byte) for byte in raw_bytes)} decode to "
                f"raw={expr_text} = {raw_value}; public value in {public_unit} is "
                f"{scale_expr} = {expected}."
            ),
        }

    pos_raw_value, pos_expected = chosen
    stimuli: list = []
    pos = _make_stimulus(pos_raw_value, pos_expected, "positive", "")
    if pos is not None:
        stimuli.append(pos)

    zero = _make_stimulus(zero_chosen[0], zero_chosen[1], "zero", "zero-value ")
    if zero is not None:
        stimuli.append(zero)

    if neg_chosen is not None:
        neg = _make_stimulus(
            neg_chosen[0], neg_chosen[1], "negative", "negative two's-complement ",
        )
        if neg is not None:
            stimuli.append(neg)

    return tuple(stimuli) if stimuli else ()


def _generate_mechanical_multi_channel_stimuli(
    device_ir: Mapping[str, Any],
    api_contract: Mapping[str, Any],
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    """Generate reliable self-test stimuli from executable high/low-byte IR."""
    if str(api_contract.get("eval_class") or "") != EVAL_CLASS_MULTI_CHANNEL:
        return ()
    api_channels = api_contract.get("channels") or []
    if not isinstance(api_channels, Sequence) or isinstance(api_channels, (str, bytes)):
        return ()
    channel_ids = [
        str(ch.get("id") or "")
        for ch in api_channels
        if isinstance(ch, Mapping) and ch.get("id")
    ]
    if not channel_ids:
        return ()

    if _device_uses_spi_stream(device_ir):
        return _generate_spi_stream_multi_channel_stimuli(
            device_ir,
            channel_ids,
            output_semantics_map=output_semantics_map,
        )

    formula_ids = _channel_formula_ids(device_ir)
    formulas = _formula_by_name(device_ir)

    if _device_uses_gpio_byte_frame(device_ir):
        generated = _generate_gpio_byte_frame_multi_channel_stimuli(
            device_ir,
            channel_ids,
            formula_ids,
            formulas,
            output_semantics_map=output_semantics_map,
        )
        if generated:
            return generated

    generated = _generate_register_mapped_multi_channel_stimuli(
        device_ir,
        api_contract,
        channel_ids,
        output_semantics_map=output_semantics_map,
    )
    if generated:
        return generated

    regs_by_channel = _channel_high_low_registers(device_ir)
        # Build an alias map for channel names when exact matching fails.
    _ir_ids = list(regs_by_channel.keys())
    _alias: dict[str, str] = {}
    for ch_id in channel_ids:
        if ch_id in regs_by_channel:
            continue
        norm = _normalise_identifier(ch_id)
        for ir_id in _ir_ids:
            if norm == _normalise_identifier(ir_id) or ir_id in ch_id or ch_id in ir_id:
                _alias[ch_id] = ir_id
                break
    usable: List[Tuple[str, int, int, str]] = []
    for ch_id in channel_ids:
        resolved = _alias.get(ch_id, ch_id)
        regs = regs_by_channel.get(resolved)
        formula = formulas.get(formula_ids.get(resolved, ""))
        if regs is None or not isinstance(formula, Mapping):
            continue
        integer_expr = formula.get("integer_approximation_expression")
        if not isinstance(integer_expr, Mapping):
            continue
        expr = str(integer_expr.get("expression") or "")
        if "high_byte" not in expr or "low_byte" not in expr:
            continue
        usable.append((ch_id, regs[0], regs[1], expr))
    if len(usable) != len(channel_ids):
        return _generate_mechanical_packed_multi_channel_stimuli(
            device_ir,
            channel_ids,
            formula_ids,
            formulas,
            output_semantics_map=output_semantics_map,
        )

    stimuli: List[Mapping[str, Any]] = []
    for stim_index in range(2):
        preload: Dict[str, List[str]] = {}
        expected: Dict[str, int] = {}
        derivation_parts: List[str] = []
        for ch_index, (ch_id, high_reg, low_reg, expr) in enumerate(usable):
            high_byte, low_byte = _MECHANICAL_BYTE_PATTERNS[
                (stim_index + ch_index) % len(_MECHANICAL_BYTE_PATTERNS)
            ]
            high_signed = _signed8(high_byte)
            value = _eval_mechanical_expression(
                expr,
                {"high_byte": high_signed, "low_byte": int(low_byte) & 0xFF},
            )
            if value is None:
                return ()
            preload[_hex_byte(high_reg)] = [_hex_byte(high_byte)]
            preload[_hex_byte(low_reg)] = [_hex_byte(low_byte)]
            expected[ch_id] = value
            expr_with_values = re.sub(
                r"\bhigh_byte\b", f"({high_signed})", expr,
            )
            expr_with_values = re.sub(
                r"\blow_byte\b", f"({_hex_byte(low_byte)})", expr_with_values,
            )
            derivation_parts.append(
                f"{ch_id}: high_byte={_hex_byte(high_byte)} signed={high_signed}; "
                f"low_byte={_hex_byte(low_byte)}; {expr_with_values} = {value}"
            )
        preload = _merge_identity_probe_preloads(device_ir, preload)
        stimuli.append({
            "name": f"mechanical_split_register_{stim_index + 1}",
            "mock_preload": preload,
            "expected_channels": expected,
            "raw_tolerance": 0,
            "derivation": "; ".join(derivation_parts),
        })
    return tuple(stimuli)


def _generate_register_mapped_multi_channel_stimuli(
    device_ir: Mapping[str, Any],
    api_contract: Mapping[str, Any],
    channel_ids: Sequence[str],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    """Generate multi-channel stimuli from channel source registers."""

    if str(api_contract.get("eval_class") or "") != EVAL_CLASS_MULTI_CHANNEL:
        return ()
    if not channel_ids:
        return ()

    channels_by_id = _read_channel_by_id(device_ir)
    register_addrs = _register_addresses_by_name(device_ir)
    formulas = _formula_by_name(device_ir)
    semantics_by_public = _output_semantics_rows_by_public_id(output_semantics_map)
    semantics_by_source = _output_semantics_rows_by_source_id(output_semantics_map)
    raw_encoding = (
        device_ir.get("raw_encoding")
        if isinstance(device_ir.get("raw_encoding"), Mapping)
        else {}
    )

    specs: List[_RegisterMappedChannelSpec] = []
    for ch_id in channel_ids:
        public_key = _normalise_identifier(ch_id)
        sem = semantics_by_public.get(public_key) or semantics_by_source.get(public_key)
        source_id = str((sem or {}).get("source_channel") or ch_id).strip()
        channel = channels_by_id.get(_normalise_identifier(source_id))
        if channel is None:
            channel = channels_by_id.get(public_key)
            source_id = ch_id
        if channel is None:
            return ()
        sources = channel.get("source_bytes")
        if not isinstance(sources, Sequence) or isinstance(sources, (str, bytes)):
            return ()
        source_names = [str(src or "").strip() for src in sources if str(src or "").strip()]
        if not source_names:
            return ()
        source_regs: List[Tuple[str, int]] = []
        for source_name in source_names:
            reg = _register_address_for_source(source_name, register_addrs)
            if reg is None:
                return ()
            source_regs.append((source_name, reg))

        formula = formulas.get(str(channel.get("formula_id") or ""))
        expr = ""
        input_names: Tuple[str, ...] = ()
        if isinstance(formula, Mapping):
            expr_obj = formula.get("integer_approximation_expression")
            if isinstance(expr_obj, Mapping):
                expr = str(expr_obj.get("expression") or "").strip()
                inputs = expr_obj.get("inputs") or []
                if isinstance(inputs, Sequence) and not isinstance(inputs, (str, bytes)):
                    input_names = tuple(
                        str(item.get("name") or "").strip()
                        for item in inputs
                        if isinstance(item, Mapping) and str(item.get("name") or "").strip()
                    )
        specs.append(_RegisterMappedChannelSpec(
            channel_id=ch_id,
            source_channel_id=source_id,
            channel=channel,
            source_regs=tuple(source_regs),
            expression=expr,
            input_names=input_names,
            semantic_kind=str((sem or {}).get("semantic_kind") or "").strip().lower(),
            raw_encoding=raw_encoding,
        ))

    stimuli: List[Mapping[str, Any]] = []
    for stim_index in range(2):
        preload: Dict[str, List[str]] = {}
        all_register_bytes: List[Tuple[str, int, int]] = []
        channel_preload_bytes: Dict[str, List[str]] = {}
        expected: Dict[str, int] = {}
        derivation_parts: List[str] = []
        for ch_index, spec in enumerate(specs):
            result = _register_mapped_channel_sample(spec, stim_index, ch_index)
            if result is None:
                return ()
            for source_name, reg, byte in result.register_bytes:
                all_register_bytes.append((source_name, reg, byte))
                preload[
                    _preload_key_for_source_register(
                        device_ir,
                        source_name=source_name,
                        channel_id=spec.channel_id,
                        channel=spec.channel,
                        reg=reg,
                    )
                ] = [_hex_byte(byte)]
            channel_preload_bytes[spec.channel_id] = [
                _hex_byte(byte)
                for _source_name, _reg, byte in sorted(
                    result.register_bytes,
                    key=lambda item: int(item[1]) & 0xFFFF,
                )
            ]
            expected[spec.channel_id] = result.expected
            derivation_parts.append(result.derivation)
        preload = _pack_spi_register_burst_preload(
            device_ir,
            preload,
            all_register_bytes,
        )
        preload = _merge_identity_probe_preloads(device_ir, preload)
        stimuli.append({
            "name": f"mechanical_register_mapped_{stim_index + 1}",
            "mock_preload": preload,
            "channel_preload_bytes": channel_preload_bytes,
            "expected_channels": expected,
            "raw_tolerance": 0,
            "derivation": "; ".join(derivation_parts),
        })
    return tuple(stimuli)


_MECHANICAL_WORD_PATTERNS: Tuple[int, ...] = (
    0x6666,
    0x8000,
    0x0000,
    0xFFFF,
    0x2570,
    0xBEEF,
)


def _generate_mechanical_packed_multi_channel_stimuli(
    device_ir: Mapping[str, Any],
    channel_ids: Sequence[str],
    formula_ids: Mapping[str, str],
    formulas: Mapping[str, Mapping[str, Any]],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> Tuple[Mapping[str, Any], ...]:
    """Generate stimuli for packed direct-read multi-channel sensors."""

    if not channel_ids:
        return ()
    read_len = _primary_direct_read_length(device_ir)
    if read_len is None:
        return ()

    semantics_by_public = _output_semantics_rows_by_public_id(output_semantics_map)
    semantics_by_source = _output_semantics_rows_by_source_id(output_semantics_map)
    usable: List[Tuple[str, str, str, str]] = []
    for ch_id in channel_ids:
        sem = (
            semantics_by_public.get(_normalise_identifier(ch_id))
            or semantics_by_source.get(_normalise_identifier(ch_id))
        )
        source_id = str((sem or {}).get("source_channel") or ch_id).strip()
        kind = str((sem or {}).get("semantic_kind") or "").strip().lower()
        formula = formulas.get(formula_ids.get(source_id, "")) or formulas.get(formula_ids.get(ch_id, ""))
        expr = ""
        input_name = "raw"
        if kind not in {"raw_count", "status_or_code"}:
            if not isinstance(formula, Mapping):
                return ()
            expr_obj = formula.get("integer_approximation_expression")
            if not isinstance(expr_obj, Mapping):
                return ()
            expr = str(expr_obj.get("expression") or "").strip()
            input_name = _single_formula_input_name(expr_obj)
            if not expr or not input_name:
                return ()
            try:
                ast.parse(expr, mode="eval")
            except SyntaxError:
                return ()
        usable.append((ch_id, input_name, expr, kind))

    payload_data_len = 2 * len(usable)
    crc_cfg = _crc8_config_from_ir(device_ir)
    insert_crc = False
    if read_len == payload_data_len:
        insert_crc = False
    elif read_len == payload_data_len + len(usable) and crc_cfg is not None:
        insert_crc = True
    else:
        return ()

    stimuli: List[Mapping[str, Any]] = []
    for stim_index in range(2):
        packed: List[int] = []
        expected: Dict[str, int] = {}
        derivation_parts: List[str] = []
        for ch_index, (ch_id, input_name, expr, kind) in enumerate(usable):
            raw = _MECHANICAL_WORD_PATTERNS[
                (stim_index + ch_index) % len(_MECHANICAL_WORD_PATTERNS)
            ] & 0xFFFF
            high = (raw >> 8) & 0xFF
            low = raw & 0xFF
            if kind in {"raw_count", "status_or_code"}:
                value = int(raw)
            else:
                value = _eval_mechanical_expression(expr, {input_name: raw})
                if value is None:
                    return ()
            packed.extend((high, low))
            if insert_crc and crc_cfg is not None:
                packed.append(_crc8((high, low), poly=crc_cfg[0], init=crc_cfg[1]))
            expected[ch_id] = value
            raw_expr = f"((0x{high:02X} << 8) | 0x{low:02X})"
            if kind in {"raw_count", "status_or_code"}:
                derivation_parts.append(
                    f"{ch_id}: SECTION B3 semantic_kind={kind}; "
                    f"public output is unconverted raw/code value; "
                    f"{raw_expr} = {value}"
                )
            else:
                expr_with_value = re.sub(
                    rf"\b{re.escape(input_name)}\b",
                    raw_expr,
                    expr,
                )
                derivation_parts.append(
                    f"{ch_id}: {input_name}={raw_expr}; "
                    f"{expr_with_value} = {value}"
                )
        if len(packed) != read_len:
            return ()
        stimuli.append({
            "name": f"mechanical_packed_direct_read_{stim_index + 1}",
            "mock_preload": {"read_bytes": [_hex_byte(byte) for byte in packed]},
            "expected_channels": expected,
            "raw_tolerance": 0,
            "derivation": "; ".join(derivation_parts),
        })
    return tuple(stimuli)


def _primary_direct_read_length(device_ir: Mapping[str, Any]) -> Optional[int]:
    flows = device_ir.get("operation_flows") or []
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return None
    target_flow_ids = _read_channel_flow_ids(device_ir)
    for flow in flows:
        if not isinstance(flow, Mapping):
            continue
        flow_id = str(flow.get("flow_id") or "")
        if target_flow_ids and flow_id not in target_flow_ids:
            continue
        steps = flow.get("steps")
        if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
            continue
        for step in steps:
            if not isinstance(step, Mapping):
                continue
            tx = step.get("transaction")
            if not isinstance(tx, Mapping):
                continue
            if str(tx.get("kind") or "") != "read":
                continue
            length = _parse_int_literal(tx.get("length"))
            if length is not None and length > 0:
                return length
    return None


def _read_channel_flow_ids(device_ir: Mapping[str, Any]) -> set[str]:
    channels = device_ir.get("read_channels") or []
    out: set[str] = set()
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return out
    for channel in channels:
        if isinstance(channel, Mapping):
            flow_id = str(channel.get("flow_id") or "")
            if flow_id:
                out.add(flow_id)
    return out


def _aggregate_read_flow_channel_groups(
    device_ir: Optional[Mapping[str, Any]],
) -> Dict[str, List[str]]:
    if not isinstance(device_ir, Mapping):
        return {}
    channels = device_ir.get("read_channels") or []
    flows = device_ir.get("operation_flows") or []
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return {}
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return {}

    by_flow: Dict[str, List[str]] = {}
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        flow_id = str(channel.get("flow_id") or "").strip()
        if channel_id and flow_id:
            by_flow.setdefault(flow_id, []).append(channel_id)

    flow_by_id = {
        str(flow.get("flow_id") or "").strip(): flow
        for flow in flows
        if isinstance(flow, Mapping) and str(flow.get("flow_id") or "").strip()
    }
    aggregate: Dict[str, List[str]] = {}
    for flow_id, channel_ids in by_flow.items():
        unique_channels = list(dict.fromkeys(channel_ids))
        if len(unique_channels) < 2:
            continue
        flow = flow_by_id.get(flow_id)
        if not isinstance(flow, Mapping):
            continue
        flow_channels = {
            str(channel or "").strip()
            for channel in (flow.get("channels") or [])
            if str(channel or "").strip()
        } if isinstance(flow.get("channels"), Sequence) and not isinstance(flow.get("channels"), (str, bytes)) else set()
        output_channels = {
            str(output.get("channel") or "").strip()
            for output in (flow.get("outputs") or [])
            if isinstance(output, Mapping) and str(output.get("channel") or "").strip()
        } if isinstance(flow.get("outputs"), Sequence) and not isinstance(flow.get("outputs"), (str, bytes)) else set()
        covered = set(unique_channels) & (flow_channels | output_channels)
        if len(covered) >= 2:
            aggregate[flow_id] = unique_channels
    return aggregate


def _single_formula_input_name(expr_obj: Mapping[str, Any]) -> str:
    inputs = expr_obj.get("inputs")
    if not isinstance(inputs, Sequence) or isinstance(inputs, (str, bytes)):
        return ""
    names = [
        str(item.get("name") or "").strip()
        for item in inputs
        if isinstance(item, Mapping) and str(item.get("name") or "").strip()
    ]
    return names[0] if len(names) == 1 else ""


def _crc8_config_from_ir(device_ir: Mapping[str, Any]) -> Optional[Tuple[int, int]]:
    text = json.dumps(device_ir, ensure_ascii=False).lower()
    if "crc" not in text:
        return None
    poly_match = re.search(r"(?:polynomial|poly)[^0-9a-fx]{0,24}(0x[0-9a-f]+)", text)
    init_match = re.search(r"(?:initialization|initial|init)[^0-9a-fx]{0,24}(0x[0-9a-f]+)", text)
    if poly_match is None or init_match is None:
        return None
    try:
        poly = int(poly_match.group(1), 16) & 0xFF
        init = int(init_match.group(1), 16) & 0xFF
    except ValueError:
        return None
    return poly, init


def _crc8(data: Sequence[int], *, poly: int, init: int) -> int:
    crc = init & 0xFF
    for byte in data:
        crc ^= int(byte) & 0xFF
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ poly) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc & 0xFF


_NARROW_PUBLIC_INT_TYPES = {
    "int8_t",
    "uint8_t",
    "char",
    "signed char",
    "unsigned char",
    "int16_t",
    "uint16_t",
    "short",
    "short int",
    "unsigned short",
    "unsigned short int",
}


def _unit_is_scaled_public(unit: Any) -> bool:
    lowered = str(unit or "").lower()
    return "milli" in lowered or "micro" in lowered


def _normalise_scaled_public_output_types(
    plan_bundle: PlanBundle,
    device_ir: Optional[Mapping[str, Any]] = None,
) -> PlanBundle:
    """Use conservative 32-bit public output types for scaled units."""
    api = dict(plan_bundle.api_contract)
    changed = False
    ir_units = _channel_units(device_ir or {})

    channels = api.get("channels")
    if isinstance(channels, Sequence) and not isinstance(channels, (str, bytes)):
        new_channels: List[Any] = []
        for channel in channels:
            if not isinstance(channel, Mapping):
                new_channels.append(channel)
                continue
            ch = dict(channel)
            ch_id = str(ch.get("id") or "")
            unit = ch.get("physical_unit", ch.get("unit")) or ir_units.get(ch_id)
            out_type = str(ch.get("out_type") or "").strip()
            if _unit_is_scaled_public(unit) and out_type.lower() in _NARROW_PUBLIC_INT_TYPES:
                ch["out_type"] = "int32_t"
                if unit and not ch.get("physical_unit"):
                    ch["physical_unit"] = str(unit)
                changed = True
            new_channels.append(ch)
        if changed:
            api["channels"] = new_channels

    unit = (
        api.get("physical_unit")
        or api.get("primary_raw_unit")
        or api.get("unit")
        or api.get("read_raw_unit")
    )
    for key in ("read_raw_out_type", "out_type"):
        out_type = str(api.get(key) or "").strip()
        if _unit_is_scaled_public(unit) and out_type.lower() in _NARROW_PUBLIC_INT_TYPES:
            api[key] = "int32_t"
            changed = True

    if not changed:
        return plan_bundle
    api.setdefault(
        "type_normalization_note",
        "scaled public units use int32_t to avoid int16_t overflow",
    )
    return dataclasses.replace(plan_bundle, api_contract=api)


_RAW_LIKE_UNIT_TOKENS = (
    "raw",
    "count",
    "counts",
    "code",
    "codes",
    "lsb",
    "adc",
)


def _unit_looks_raw_like(unit: Any) -> bool:
    text = str(unit or "").strip().lower()
    if not text:
        return False
    compact = re.sub(r"[^a-z0-9]+", "_", text)
    return any(token in compact for token in _RAW_LIKE_UNIT_TOKENS)


def _first_read_channel(device_ir: Mapping[str, Any]) -> Optional[Mapping[str, Any]]:
    channels = device_ir.get("read_channels") or []
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return None
    for channel in channels:
        if isinstance(channel, Mapping):
            return channel
    return None


def _single_channel_should_use_raw_primary(
    api: Mapping[str, Any],
    device_ir: Mapping[str, Any],
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> bool:
    """True when single_channel eval ABI should expose raw/count as primary."""
    if str(api.get("eval_class") or "") != EVAL_CLASS_SINGLE_CHANNEL:
        return False
    # B3 output semantics take priority: if a channel declares
    # conversion_required=true, the adapter should expose the physical value.
    _osm_rows: Sequence[Any] = []
    if isinstance(output_semantics_map, Mapping):
        rows = output_semantics_map.get("channels")
        if isinstance(rows, Sequence) and not isinstance(rows, (str, bytes)):
            _osm_rows = rows
    for row in _osm_rows:
        if not isinstance(row, Mapping):
            continue
        conv = row.get("conversion_required")
        if conv is True:
            return False
    unit = (
        api.get("primary_raw_unit")
        or api.get("physical_unit")
        or api.get("unit")
        or api.get("read_raw_unit")
    )
    if _unit_looks_raw_like(unit):
        return False
    channel = _first_read_channel(device_ir)
    if not isinstance(channel, Mapping):
        return False
    raw_type = str(channel.get("raw_type") or "").strip()
    source_bytes = channel.get("source_bytes")
    has_source = bool(raw_type) or (
        isinstance(source_bytes, Sequence)
        and not isinstance(source_bytes, (str, bytes))
        and len(source_bytes) > 0
    )
    if not has_source:
        return False
    if channel.get("formula_id"):
        return True
    formulae = device_ir.get("conversion_formulae")
    return isinstance(formulae, Sequence) and not isinstance(formulae, (str, bytes)) and bool(formulae)


def _coerce_byte_list(raw: Any) -> Optional[Tuple[int, ...]]:
    if not isinstance(raw, Sequence) or isinstance(raw, (str, bytes)):
        return None
    values: List[int] = []
    for item in raw:
        value = _parse_int_literal(item)
        if value is None:
            return None
        values.append(value & 0xFF)
    return tuple(values) if values else None


def _first_mock_preload_bytes(preload: Any) -> Optional[Tuple[int, ...]]:
    if not isinstance(preload, Mapping):
        return None
    preferred_keys = ("read_bytes", "0", "0x00", "reg_0x00", "reg_0")
    for key in preferred_keys:
        if key in preload:
            bs = _coerce_byte_list(preload.get(key))
            if bs is not None:
                return bs
    for value in preload.values():
        bs = _coerce_byte_list(value)
        if bs is not None:
            return bs
    return None


def _raw_type_is_signed(raw_type: Any, raw_encoding: Mapping[str, Any]) -> bool:
    explicit = raw_encoding.get("signed")
    if isinstance(explicit, bool):
        return explicit
    text = str(raw_type or "").strip().lower()
    return text.startswith("int") and not text.startswith("uint")


def _raw_integer_derivation_from_bytes(
    bs: Sequence[int],
    *,
    channel: Mapping[str, Any],
    raw_encoding: Mapping[str, Any],
) -> Optional[Tuple[str, int]]:
    """Return an AST-checkable expression for the same raw decoding path."""
    if not bs:
        return None
    values = [int(b) & 0xFF for b in bs]
    order = str(raw_encoding.get("byte_order") or "").lower()
    if not order:
        source = " ".join(str(x or "") for x in (channel.get("source_bytes") or [])).lower()
        order = "little" if "low" in source and source.find("low") < source.find("high") else "big"
    little = "little" in order or "lsb" in order
    if little:
        terms = [
            f"0x{value:02X}" if idx == 0 else f"(0x{value:02X} << {8 * idx})"
            for idx, value in enumerate(values)
        ]
        raw = sum(value << (8 * idx) for idx, value in enumerate(values))
    else:
        last = len(values) - 1
        terms = [
            f"0x{value:02X}" if last == idx else f"(0x{value:02X} << {8 * (last - idx)})"
            for idx, value in enumerate(values)
        ]
        raw = 0
        for value in values:
            raw = (raw << 8) | value
    expr = "(" + " | ".join(terms) + ")"

    right_shift = _parse_int_literal(raw_encoding.get("right_shift"))
    if right_shift is not None and right_shift > 0:
        raw >>= right_shift
        expr = f"({expr} >> {right_shift})"

    width = _parse_int_literal(raw_encoding.get("effective_bits"))
    if width is None:
        width = _parse_int_literal(raw_encoding.get("bit_width"))
    if width is None:
        raw_type = str(channel.get("raw_type") or "")
        m = re.search(r"(\d+)", raw_type)
        if m:
            width = int(m.group(1))
    if width is None or width <= 0:
        width = len(values) * 8
    if width < len(values) * 8:
        mask = (1 << width) - 1
        raw &= mask
        expr = f"({expr} & 0x{mask:X})"
    if _raw_type_is_signed(channel.get("raw_type"), raw_encoding):
        sign_bit = 1 << (width - 1)
        if raw & sign_bit:
            raw -= 1 << width
            expr = f"({expr} - {1 << width})"
    return expr, int(raw)


def _normalise_single_channel_raw_primary(
    plan_bundle: PlanBundle,
    device_ir: Optional[Mapping[str, Any]] = None,
    *,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> PlanBundle:
    """Align single-channel contracts with the eval adapter raw/count ABI."""
    ir = device_ir or {}
    api = dict(plan_bundle.api_contract)
    if not _single_channel_should_use_raw_primary(
        api, ir, output_semantics_map=output_semantics_map,
    ):
        return plan_bundle

    channel = _first_read_channel(ir) or {}
    raw_encoding = ir.get("raw_encoding") if isinstance(ir.get("raw_encoding"), Mapping) else {}
    api["primary_raw_unit"] = "raw_count"
    api.setdefault(
        "unit_normalization_note",
        "single_channel eval ABI read_raw_i32 returns raw/count; physical conversion is helper-only",
    )

    test_plan = dict(plan_bundle.test_plan)
    stimuli = test_plan.get("test_stimuli")
    changed_stimuli = False
    if isinstance(stimuli, Sequence) and not isinstance(stimuli, (str, bytes)):
        new_stimuli: List[Any] = []
        for stim in stimuli:
            if not isinstance(stim, Mapping):
                new_stimuli.append(stim)
                continue
            new_stim = dict(stim)
            bs = _first_mock_preload_bytes(new_stim.get("mock_preload"))
            raw_derivation = (
                _raw_integer_derivation_from_bytes(
                    bs,
                    channel=channel,
                    raw_encoding=raw_encoding,
                )
                if bs is not None else None
            )
            if raw_derivation is not None:
                raw_expr, raw_value = raw_derivation
                new_stim["expected_read_raw"] = int(raw_value)
                new_stim["raw_tolerance"] = 0
                byte_text = " ".join(_hex_byte(b) for b in bs)
                new_stim["derivation"] = (
                    f"single_channel raw-primary normalization: bytes "
                    f"[{byte_text}], raw_count = {raw_expr} = {int(raw_value)}. "
                    "Physical conversion formula is not applied to "
                    "drivergen_eval_read_raw_i32."
                )
                changed_stimuli = True
            new_stimuli.append(new_stim)
        if changed_stimuli:
            test_plan["test_stimuli"] = new_stimuli
            test_plan["raw_primary_normalization_source"] = (
                "device_ir.read_channels+conversion_formulae"
            )

    return dataclasses.replace(
        plan_bundle,
        api_contract=api,
        test_plan=test_plan if changed_stimuli else plan_bundle.test_plan,
    )


def _normalise_output_semantics_units(
    plan_bundle: PlanBundle,
    output_semantics_map: Optional[Mapping[str, Any]],
) -> PlanBundle:
    """Propagate B3 public units into the frozen API contract shape."""
    if not isinstance(output_semantics_map, Mapping):
        return plan_bundle
    rows = output_semantics_map.get("channels")
    if not isinstance(rows, Sequence) or isinstance(rows, (str, bytes)) or not rows:
        return plan_bundle

    api = dict(plan_bundle.api_contract)
    changed = False
    if str(plan_bundle.eval_class) == EVAL_CLASS_SINGLE_CHANNEL:
        first = next((row for row in rows if isinstance(row, Mapping)), None)
        if isinstance(first, Mapping):
            unit = str(first.get("public_unit") or "").strip()
            if unit and not api.get("primary_raw_unit"):
                api["primary_raw_unit"] = unit
                changed = True
        return dataclasses.replace(plan_bundle, api_contract=api) if changed else plan_bundle

    if str(plan_bundle.eval_class) != EVAL_CLASS_MULTI_CHANNEL:
        return plan_bundle
    api_channels = api.get("channels")
    if not isinstance(api_channels, Sequence) or isinstance(api_channels, (str, bytes)):
        return plan_bundle

    sem_by_public = _output_semantics_rows_by_public_id(output_semantics_map)
    sem_by_source = _output_semantics_rows_by_source_id(output_semantics_map)
    new_channels: List[Any] = []
    for channel in api_channels:
        if not isinstance(channel, Mapping):
            new_channels.append(channel)
            continue
        row = dict(channel)
        key = _normalise_identifier(str(row.get("id") or ""))
        sem = sem_by_public.get(key) or sem_by_source.get(key)
        unit = str((sem or {}).get("public_unit") or "").strip()
        if unit and not (row.get("physical_unit") or row.get("unit")):
            row["physical_unit"] = unit
            changed = True
        new_channels.append(row)
    if changed:
        api["channels"] = new_channels
        return dataclasses.replace(plan_bundle, api_contract=api)
    return plan_bundle


def _with_mechanical_plan_stimuli(
    plan_bundle: PlanBundle,
    device_ir: Mapping[str, Any],
    output_semantics_map: Optional[Mapping[str, Any]] = None,
) -> PlanBundle:
    generated = _generate_mechanical_memory_stimuli(plan_bundle.api_contract)
    source = "memory_harness_probe+api_contract.memory_size_bytes"
    if not generated:
        generated = _generate_gpio_pulse_single_channel_stimuli(
            device_ir,
            plan_bundle.api_contract,
        )
        source = "device_ir.gpio_measure_pulse+conversion_formulae"
    # UART: try packet-frame mechanical stimuli before generic single-channel
    bus = str(device_ir.get("bus_type") or "").strip().lower()
    if not generated and bus == "uart":
        generated = _generate_mechanical_uart_stimuli(
            device_ir,
            plan_bundle.api_contract,
            output_semantics_map=output_semantics_map,
        )
        source = "device_ir.operation_flows+conversion_formulae+uart_checksum"
    if not generated:
        generated = _generate_mechanical_single_channel_stimuli(
            device_ir,
            plan_bundle.api_contract,
            output_semantics_map=output_semantics_map,
        )
        source = "device_ir.read_sequence+raw_encoding+conversion_formulae+output_semantics"
    if not generated:
        generated = _generate_mechanical_multi_channel_stimuli(
            device_ir,
            plan_bundle.api_contract,
            output_semantics_map=output_semantics_map,
        )
        if generated and any(
            isinstance(stim.get("mock_preload"), Mapping)
            and "stream" in stim.get("mock_preload", {})
            for stim in generated
            if isinstance(stim, Mapping)
        ):
            source = (
                "device_ir.spi_stream_bitfields+read_channels+"
                "conversion_formulae+output_semantics"
            )
        elif generated and any(
            isinstance(stim.get("mock_preload"), Mapping)
            and "payload" in stim.get("mock_preload", {})
            for stim in generated
            if isinstance(stim, Mapping)
        ):
            source = (
                "device_ir.gpio_byte_frame+read_channels+"
                "conversion_formulae+output_semantics"
            )
        else:
            source = "device_ir.operation_flows+read_channels+conversion_formulae+output_semantics"
    if not generated:
        return plan_bundle
    test_plan = dict(plan_bundle.test_plan)
    test_plan["test_stimuli"] = [dict(stim) for stim in generated]
    test_plan["mechanical_test_stimuli_source"] = source
    return dataclasses.replace(plan_bundle, test_plan=test_plan)


def _output_semantics_static_errors(
    plan_bundle: PlanBundle,
    output_semantics_map: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    """Block plans that contradict the upstream semantic decision."""
    if not isinstance(output_semantics_map, Mapping):
        return ()
    rows = output_semantics_map.get("channels")
    if not isinstance(rows, Sequence) or isinstance(rows, (str, bytes)) or not rows:
        return ()

    errors: List[str] = []
    api = plan_bundle.api_contract
    if str(plan_bundle.eval_class) == EVAL_CLASS_SINGLE_CHANNEL:
        first = next((row for row in rows if isinstance(row, Mapping)), None)
        if isinstance(first, Mapping):
            unit = (
                api.get("primary_raw_unit")
                or api.get("physical_unit")
                or api.get("unit")
                or api.get("read_raw_unit")
            )
            note = str(api.get("unit_normalization_note") or "").lower()
            if _unit_looks_raw_like(unit) and "raw/count" in note:
                return ()
            errors.extend(_unit_semantic_mismatch_errors(first, unit, "primary output"))
        return tuple(errors)

    if str(plan_bundle.eval_class) != EVAL_CLASS_MULTI_CHANNEL:
        return ()

    api_channels = api.get("channels")
    if not isinstance(api_channels, Sequence) or isinstance(api_channels, (str, bytes)):
        return ()
    by_id = {
        str(row.get("id") or ""): row
        for row in api_channels
        if isinstance(row, Mapping)
    }
    for sem in rows:
        if not isinstance(sem, Mapping):
            continue
        public_id = str(sem.get("public_id") or "").strip()
        if not public_id:
            continue
        api_row = by_id.get(public_id)
        if not isinstance(api_row, Mapping):
            errors.append(
                "api_contract.channels is missing output_semantics public_id "
                f"{public_id!r}; planner must use SECTION B3 public_id values"
            )
            continue
        unit = api_row.get("physical_unit") or api_row.get("unit")
        errors.extend(_unit_semantic_mismatch_errors(sem, unit, f"channel {public_id!r}"))
    return tuple(errors)


def _unit_semantic_mismatch_errors(
    sem: Mapping[str, Any],
    plan_unit: Any,
    label: str,
) -> List[str]:
    kind = str(sem.get("semantic_kind") or "").strip().lower()
    expected_unit = str(sem.get("public_unit") or "").strip()
    unit_text = str(plan_unit or "").strip()
    errors: List[str] = []
    if kind == "raw_count":
        if unit_text and not _unit_looks_raw_like(unit_text):
            errors.append(
                f"{label} contradicts SECTION B3: semantic_kind=raw_count "
                f"but api_contract unit is {unit_text!r}"
            )
        return errors
    if semantic_kind_is_physical(kind):
        if not unit_text or _unit_looks_raw_like(unit_text):
            errors.append(
                f"{label} contradicts SECTION B3: semantic_kind={kind} "
                f"requires public_unit {expected_unit!r}, but api_contract "
                f"unit is {unit_text!r}"
            )
            return errors
        if expected_unit and _unit_key(expected_unit) != _unit_key(unit_text):
            errors.append(
                f"{label} contradicts SECTION B3: expected public_unit "
                f"{expected_unit!r}, but api_contract unit is {unit_text!r}"
            )
    return errors


def _unit_key(unit: Any) -> str:
    text = str(unit or "").strip().lower()
    text = text.replace("%rh", "percent_rh").replace("%", "percent")
    text = text.replace("mdegc", "milli_degc").replace("millidegc", "milli_degc")
    return re.sub(r"[^a-z0-9]+", "_", text).strip("_")


def _source_names_from_source_spec(value: Any) -> Tuple[str, ...]:
    if isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        out: List[str] = []
        for item in value:
            out.extend(_source_names_from_source_spec(item))
        return tuple(out)
    text = str(value or "").strip()
    if not text:
        return ()
    out: List[str] = []
    for part in re.split(r"\|\||[,;]", text):
        left = part.split(":", 1)[0].strip()
        match = re.search(r"0[xX][0-9A-Fa-f]+|[A-Za-z_][A-Za-z0-9_]*", left)
        if match:
            out.append(match.group(0))
    return tuple(out)


def _source_spec_has_non_byte_width(value: Any) -> bool:
    if value is None:
        return False
    if isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        return any(_source_spec_has_non_byte_width(item) for item in value)
    text = str(value or "")
    for part in re.split(r"\|\||[,;]", text):
        match = re.search(
            r"(?:0[xX][0-9A-Fa-f]+|[A-Za-z_][A-Za-z0-9_]*)\s*:\s*(\d+)",
            part,
        )
        if match and int(match.group(1)) != 8:
            return True
    return False


def _formula_has_bit_level_source(formula: Optional[Mapping[str, Any]]) -> bool:
    if not isinstance(formula, Mapping):
        return False
    expr_obj = formula.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return False
    inputs = expr_obj.get("inputs") or []
    if not isinstance(inputs, Sequence) or isinstance(inputs, (str, bytes)):
        return False
    for item in inputs:
        if not isinstance(item, Mapping):
            continue
        for key in ("bit_source", "source_bits"):
            if item.get(key):
                return True
        for key in ("byte_source", "source_bytes"):
            if _source_spec_has_non_byte_width(item.get(key)):
                return True
    return False


def _source_names_from_formula(formula: Optional[Mapping[str, Any]]) -> Tuple[str, ...]:
    if not isinstance(formula, Mapping):
        return ()
    expr_obj = formula.get("integer_approximation_expression")
    if not isinstance(expr_obj, Mapping):
        return ()
    inputs = expr_obj.get("inputs") or []
    if not isinstance(inputs, Sequence) or isinstance(inputs, (str, bytes)):
        return ()
    for item in inputs:
        if not isinstance(item, Mapping):
            continue
        for key in ("byte_source", "source_bytes"):
            names = _source_names_from_source_spec(item.get(key))
            if names:
                return names
    return ()


def _formula_for_raw_count_channel(
    source_id: str,
    channel: Mapping[str, Any],
    formulas: Mapping[str, Mapping[str, Any]],
) -> Optional[Mapping[str, Any]]:
    formula_id = str(channel.get("formula_id") or "").strip()
    if formula_id and formula_id in formulas:
        return formulas[formula_id]

    aliases = _channel_id_alias_tokens(source_id or str(channel.get("id") or ""))
    if not aliases:
        return None
    scored: List[Tuple[int, str, Mapping[str, Any]]] = []
    for name, formula in formulas.items():
        expr_obj = formula.get("integer_approximation_expression")
        output_obj = (
            expr_obj.get("output")
            if isinstance(expr_obj, Mapping)
            and isinstance(expr_obj.get("output"), Mapping)
            else {}
        )
        text = _normalise_identifier(" ".join((
            str(name),
            str(formula.get("name") or ""),
            " ".join(_source_names_from_formula(formula)),
            str(output_obj.get("name") or ""),
            str(output_obj.get("unit") or ""),
        )))
        score = sum(1 for alias in aliases if alias and alias in text)
        if score > 0:
            scored.append((score, str(name), formula))
    if not scored:
        return None
    scored.sort(key=lambda item: (item[0], item[1]), reverse=True)
    if len(scored) > 1 and scored[0][0] == scored[1][0]:
        return None
    return scored[0][2]


def _preload_register_from_key(key: Any) -> Optional[int]:
    text = str(key or "").strip()
    if not text:
        return None
    tail = text.split(":")[-1].strip()
    parsed = _parse_int_literal(tail)
    if parsed is not None:
        return int(parsed) & 0xFFFF
    matches = re.findall(r"0[xX][0-9A-Fa-f]+|\b\d+\b", tail)
    if not matches:
        return None
    parsed = _parse_int_literal(matches[-1])
    if parsed is None:
        return None
    return int(parsed) & 0xFFFF


def _preload_bytes_by_register(preload: Any) -> Dict[int, int]:
    if not isinstance(preload, Mapping):
        return {}
    out: Dict[int, int] = {}
    for key, value in preload.items():
        start = _preload_register_from_key(key)
        if start is None:
            continue
        bs = _extract_bytes(value)
        if bs is None:
            continue
        for offset, byte in enumerate(bs):
            out.setdefault((int(start) + offset) & 0xFFFF, int(byte) & 0xFF)
    return out


def _encoded_raw_bytes_from_source_values(
    source_regs: Sequence[Tuple[str, int]],
    source_values: Sequence[int],
    raw_encoding: Mapping[str, Any],
) -> Tuple[int, ...]:
    total = len(source_regs)
    if total != len(source_values):
        return ()
    little = _raw_encoding_little_endian(raw_encoding)
    if little is None:
        return tuple(int(byte) & 0xFF for byte in source_values)
    source_keys = tuple(
        _normalise_identifier(source_name)
        for source_name, _reg in source_regs
    )

    def index_for_role(role: str, position: int) -> int:
        if total == 1:
            return 0
        if role == "low":
            return 0 if little else total - 1
        if role == "high":
            return total - 1 if little else 0
        if role == "middle" and total == 3:
            return 1
        return position

    def role_for_source(source_name: str, position: int) -> str:
        key = _normalise_identifier(source_name)
        if total == 3 and any("xlsb" in source_key for source_key in source_keys):
            if "xlsb" in key:
                return "low"
            if "lsb" in key:
                return "middle"
            if "msb" in key:
                return "high"
        return _source_byte_role(source_name, position, total)

    encoded: List[Optional[int]] = [None] * total
    for idx, ((source_name, _reg), byte) in enumerate(zip(source_regs, source_values)):
        role = role_for_source(source_name, idx)
        byte_idx = index_for_role(role, idx)
        if byte_idx < 0 or byte_idx >= total:
            return ()
        value = int(byte) & 0xFF
        if encoded[byte_idx] is not None and encoded[byte_idx] != value:
            return ()
        encoded[byte_idx] = value
    if any(byte is None for byte in encoded):
        return ()
    return tuple(int(byte) for byte in encoded if byte is not None)


def _raw_count_from_preload_bytes(
    raw_bytes: Sequence[int],
    *,
    channel: Mapping[str, Any],
    raw_encoding: Mapping[str, Any],
) -> Tuple[int, str]:
    values = tuple(int(byte) & 0xFF for byte in raw_bytes)
    little = _raw_encoding_little_endian(raw_encoding) is True
    if little:
        unsigned = sum(byte << (8 * idx) for idx, byte in enumerate(values))
        expr_terms = [
            f"0x{byte:02X}" if idx == 0 else f"(0x{byte:02X} << {8 * idx})"
            for idx, byte in enumerate(values)
        ]
    else:
        unsigned = 0
        expr_terms = []
        last = len(values) - 1
        for idx, byte in enumerate(values):
            unsigned = (unsigned << 8) | byte
            expr_terms.append(
                f"0x{byte:02X}"
                if idx == last
                else f"(0x{byte:02X} << {8 * (last - idx)})"
            )
    expr = "(" + " | ".join(expr_terms) + ")"

    stored_width = len(values) * 8
    effective_bits = _parse_int_literal(raw_encoding.get("effective_bits"))
    channel_width = _channel_bit_width(channel, len(values))
    if effective_bits is None or effective_bits <= 0 or effective_bits > stored_width:
        effective_bits = min(channel_width, stored_width)

    right_shift = _parse_int_literal(raw_encoding.get("right_shift"))
    if (
        right_shift is not None
        and right_shift > 0
        and effective_bits is not None
        and effective_bits < stored_width
    ):
        unsigned >>= int(right_shift)
        expr = f"({expr} >> {int(right_shift)})"

    if effective_bits is not None and 0 < effective_bits < stored_width:
        mask = (1 << int(effective_bits)) - 1
        unsigned &= mask
        expr = f"({expr} & 0x{mask:X})"

    if _raw_type_is_signed(channel.get("raw_type"), raw_encoding) and effective_bits:
        sign_bit = 1 << (int(effective_bits) - 1)
        if unsigned & sign_bit:
            unsigned -= 1 << int(effective_bits)
            expr = f"({expr} - {1 << int(effective_bits)})"
    return int(unsigned), expr


def _preload_raw_count_static_errors(
    plan_bundle: PlanBundle,
    output_semantics_map: Optional[Mapping[str, Any]],
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    """Validate raw/code expected_channels against address-keyed preloads."""
    if str(plan_bundle.eval_class) != EVAL_CLASS_MULTI_CHANNEL:
        return ()
    if not isinstance(output_semantics_map, Mapping) or not isinstance(device_ir, Mapping):
        return ()
    stimuli = plan_bundle.test_plan.get("test_stimuli") or []
    if not isinstance(stimuli, Sequence) or isinstance(stimuli, (str, bytes)):
        return ()

    channels_by_id = _read_channel_by_id(device_ir)
    register_addrs = _register_addresses_by_name(device_ir)
    formulas = _formula_by_name(device_ir)
    raw_encoding = (
        device_ir.get("raw_encoding")
        if isinstance(device_ir.get("raw_encoding"), Mapping)
        else {}
    )
    if not channels_by_id or not register_addrs:
        return ()

    errors: List[str] = []
    for stim_index, stim in enumerate(stimuli):
        if not isinstance(stim, Mapping):
            continue
        expected_channels = stim.get("expected_channels") or {}
        if not isinstance(expected_channels, Mapping):
            continue
        register_bytes = _preload_bytes_by_register(stim.get("mock_preload"))
        if not register_bytes:
            continue
        name = str(stim.get("name") or f"#{stim_index}")
        try:
            tolerance = float(stim.get("raw_tolerance") or 0)
        except (TypeError, ValueError):
            tolerance = 0.0
        for public_id, expected in expected_channels.items():
            try:
                expected_value = float(expected)
            except (TypeError, ValueError):
                continue
            source_id, channel, sem = _resolve_public_channel_source(
                str(public_id),
                channels_by_id,
                output_semantics_map,
            )
            if channel is None:
                continue
            semantic_kind = str((sem or {}).get("semantic_kind") or "").strip().lower()
            if semantic_kind not in {"raw_count", "status_or_code"}:
                continue
            formula = _formula_for_raw_count_channel(source_id, channel, formulas)
            if (
                _source_spec_has_non_byte_width(channel.get("source_bytes"))
                or _formula_has_bit_level_source(formula)
            ):
                continue
            source_names = _source_names_from_source_spec(channel.get("source_bytes"))
            if not source_names:
                source_names = _source_names_from_formula(formula)
            if not source_names:
                continue
            source_regs: List[Tuple[str, int]] = []
            for source_name in source_names:
                reg = _register_address_for_source(source_name, register_addrs)
                if reg is None:
                    source_regs = []
                    break
                source_regs.append((source_name, int(reg) & 0xFFFF))
            if not source_regs:
                continue
            raw_bytes: List[int] = []
            for _source_name, reg in source_regs:
                if reg not in register_bytes:
                    raw_bytes = []
                    break
                raw_bytes.append(register_bytes[reg])
            if not raw_bytes:
                continue
            encoded_bytes = _encoded_raw_bytes_from_source_values(
                source_regs,
                raw_bytes,
                raw_encoding,
            )
            if not encoded_bytes:
                continue
            actual, expr = _raw_count_from_preload_bytes(
                encoded_bytes,
                channel=channel,
                raw_encoding=raw_encoding,
            )
            if abs(float(actual) - expected_value) <= max(tolerance, 0.0):
                continue
            regs_text = ", ".join(_hex_byte(reg) for _source_name, reg in source_regs)
            bytes_text = " ".join(_hex_byte(byte) for byte in raw_bytes)
            errors.append(
                f"test_plan.test_stimuli[{stim_index}] {name!r} "
                f"expected_channels[{str(public_id)!r}]={expected!r}, but "
                f"mock_preload register(s) {regs_text} bytes [{bytes_text}] "
                f"encode raw_count {actual} via {expr}. Update either "
                "mock_preload or expected_channels; derivation text is not "
                "authoritative."
            )
    return tuple(errors)


def _plan_static_validation_errors(
    plan_bundle: PlanBundle,
    consistency_report: ConsistencyReport,
    coverage_report: CoverageReport,
    output_semantics_map: Optional[Mapping[str, Any]] = None,
    device_ir: Optional[Mapping[str, Any]] = None,
) -> Tuple[str, ...]:
    """Return blocking errors that prevent freezing a generated plan."""
    errors: List[str] = []
    errors.extend(_plan_derivation_static_errors(plan_bundle))
    errors.extend(_display_contract_static_errors(plan_bundle))
    errors.extend(_rtc_contract_static_errors(plan_bundle))
    errors.extend(_output_semantics_static_errors(plan_bundle, output_semantics_map))
    errors.extend(_multi_channel_aggregate_call_static_errors(plan_bundle, device_ir))
    errors.extend(_uart_packet_checksum_static_errors(plan_bundle))
    errors.extend(_preload_raw_count_static_errors(
        plan_bundle,
        output_semantics_map,
        device_ir,
    ))

    if not consistency_report.ok:
        for stim in consistency_report.inconsistent():
            errors.append(
                "test_plan consistency failed for "
                f"{stim.name!r}: {stim.evidence}"
            )

    if (
        consistency_report.stimuli
        and consistency_report.llm_only_count == len(consistency_report.stimuli)
        and consistency_report.consistent_count == 0
        and str(plan_bundle.eval_class) != "display"
    ):
        errors.append(
            "test_plan has no mechanically checkable stimulus: every "
            "test_stimuli entry is llm_only. Add at least one stimulus whose "
            "mock_preload and expected_* value can be checked mechanically, or "
            "make the derivation executable enough for the static checker."
        )

    if str(plan_bundle.eval_class) == EVAL_CLASS_MULTI_CHANNEL:
        # Mechanical coverage makes partially-checkable stimuli non-blocking.
        _mech_ok = (
            consistency_report.stimuli
            and consistency_report.llm_only_count == 0
            and consistency_report.consistent_count > 0
        )
        for stim in consistency_report.stimuli:
            if stim.warnings and not _mech_ok:
                errors.append(
                    "test_plan multi_channel stimulus "
                    f"{stim.name!r} is only partially checkable: "
                    f"{'; '.join(stim.warnings)}. Every expected_channels "
                    "entry must have matching mock_preload data and an "
                    "executable derivation for this same stimulus; do not "
                    "declare placeholder channels as 0 or 'not read'."
                )

    # Mechanically checked stimuli can make transaction gaps non-blocking.
    _mechanical_stimuli_ok = (
        consistency_report.stimuli
        and consistency_report.llm_only_count == 0
        and consistency_report.consistent_count > 0
    )
    if not coverage_report.ok and not _mechanical_stimuli_ok:
        for match in coverage_report.missing:
            mech = match.mechanical
            errors.append(
                "test_plan.expected_transactions omits mechanical "
                f"transaction phase={mech.phase!r} "
                f"addr_or_pin={mech.addr_or_pin!r} "
                f"prefixes={mech.write_prefix_any_of!r}: {match.reason}"
            )
    elif not coverage_report.ok and _mechanical_stimuli_ok:
        # Mechanical stimuli already cover the protocol.
        pass

    stimuli = plan_bundle.test_plan.get("test_stimuli") or []
    if not isinstance(stimuli, Sequence) or isinstance(stimuli, (str, bytes)):
        errors.append("test_plan.test_stimuli must be a sequence")
        return tuple(errors)

    for index, stim in enumerate(stimuli):
        if not isinstance(stim, Mapping):
            errors.append(f"test_plan.test_stimuli[{index}] must be an object")
            continue
        if not _stimulus_has_expected_value(stim):
            name = str(stim.get("name") or f"#{index}")
            errors.append(
                f"test_plan.test_stimuli[{index}] {name!r} has no "
                "expected_* assertion; generation-side probe would only "
                "check that the test completed, not that outputs are correct"
            )
        if str(plan_bundle.eval_class) == EVAL_CLASS_MEMORY:
            preload = stim.get("mock_preload") or {}
            if isinstance(preload, Mapping):
                bad_keys = []
                for key in preload:
                    try:
                        int(str(key), 0)
                    except ValueError:
                        bad_keys.append(str(key))
                if bad_keys:
                    name = str(stim.get("name") or f"#{index}")
                    errors.append(
                        f"test_plan.test_stimuli[{index}] {name!r} memory "
                        "mock_preload uses non-address key(s) "
                        f"{bad_keys!r}. For eval_class=memory, preload keys "
                        "must be numeric device-internal byte addresses such "
                        "as '0x0000'; do not use direct-read sentinel keys "
                        "like 'read_bytes'."
                    )

    return tuple(errors)


def _multi_channel_aggregate_call_static_errors(
    plan_bundle: PlanBundle,
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    if str(plan_bundle.eval_class) != EVAL_CLASS_MULTI_CHANNEL:
        return ()
    aggregate_groups = _aggregate_read_flow_channel_groups(device_ir)
    if not aggregate_groups:
        return ()
    channels = plan_bundle.api_contract.get("channels")
    if not isinstance(channels, Sequence) or isinstance(channels, (str, bytes)):
        return ()

    call_by_channel: Dict[str, str] = {}
    for channel in channels:
        if not isinstance(channel, Mapping):
            continue
        channel_id = str(channel.get("id") or "").strip()
        call = str(channel.get("call") or "").strip()
        if channel_id and call:
            call_by_channel[channel_id] = call

    errors: List[str] = []
    for flow_id, channel_ids in aggregate_groups.items():
        group_calls = {
            call_by_channel[channel_id]
            for channel_id in channel_ids
            if channel_id in call_by_channel
        }
        if len(group_calls) <= 1:
            continue
        errors.append(
            "multi_channel api_contract splits channels from one Device IR "
            f"operation flow {flow_id!r} into different calls: "
            f"{sorted(group_calls)!r}. These channels are produced by one "
            "aggregate read flow, so expose one aggregate driver function that "
            "fills all of the channel output locals and repeat the exact same "
            "channels[*].call string for every channel in that flow. The "
            "adapter groups identical calls and invokes the aggregate read once."
        )
    return tuple(errors)


def _iter_contract_bindings(rtos_contract: Mapping[str, Any]) -> Tuple[Mapping[str, Any], ...]:
    bindings = rtos_contract.get("api_bindings")
    if isinstance(bindings, Mapping):
        values = bindings.values()
    else:
        values = rtos_contract.values()
    return tuple(v for v in values if isinstance(v, Mapping))


def _header_basename(raw: Any) -> str:
    text = str(raw or "").strip().strip("<>").strip('"')
    if not text:
        return ""
    return re.split(r"[/\\]", text)[-1]


def _header_include_name(raw: Any) -> str:
    text = str(raw or "").strip().strip("<>").strip('"').replace("\\", "/")
    if not text:
        return ""
    include_match = re.search(r"(?:^|/)include/(.+)$", text)
    if include_match:
        return include_match.group(1).strip("/")
    for prefix in ("cpukit/include/", "include/"):
        if text.startswith(prefix):
            return text[len(prefix):].strip("/")
    return text


def _driver_include_line(header: str) -> str:
    name = _header_include_name(header)
    if "/" in name:
        return f"#include <{name}>"
    return f'#include "{name}"'


def _add_header_candidate(out: set[str], raw: Any) -> None:
    text = str(raw or "").strip()
    if not text:
        return
    include = re.search(r'#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', text)
    if include:
        text = include.group(1).strip()
    base = _header_basename(text)
    if not base:
        return
    out.add(base)
    out.add(text.replace("\\", "/"))
    suffix = re.sub(r"\.(?:c|cc|cpp)$", ".h", base, flags=re.IGNORECASE)
    if suffix != base:
        out.add(suffix)


def _collect_header_candidates_from_value(out: set[str], value: Any) -> None:
    if isinstance(value, Mapping):
        for key, nested in value.items():
            key_text = str(key).lower()
            if key_text in {"required_headers", "include_headers", "headers", "evidence"}:
                _collect_header_candidates_from_value(out, nested)
                continue
            if key_text in {"declared_in", "implemented_in", "path"}:
                for match in _RTOS_HEADER_PATH_RE.finditer(str(nested or "")):
                    _add_header_candidate(out, match.group(0))
            elif isinstance(nested, (Mapping, list, tuple)):
                _collect_header_candidates_from_value(out, nested)
        return
    if isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        for item in value:
            _collect_header_candidates_from_value(out, item)
        return
    if isinstance(value, str):
        for match in _RTOS_HEADER_PATH_RE.finditer(value):
            _add_header_candidate(out, match.group(0))


def _rtos_header_allowlist(
    rtos_contract: Mapping[str, Any],
    *,
    task_package: Optional[Mapping[str, Any]] = None,
) -> set[str]:
    allowed: set[str] = set(_C_STANDARD_INCLUDE_HEADERS)
    _collect_header_candidates_from_value(allowed, rtos_contract)
    if isinstance(task_package, Mapping):
        _collect_header_candidates_from_value(allowed, task_package)
    return {item for item in allowed if item}


def _include_header_allowed(
    framing: str,
    header: str,
    allowed_headers: set[str],
    *,
    own_headers: Sequence[str] = (),
) -> bool:
    base = _header_basename(header)
    if not base:
        return True
    if base in _C_STANDARD_INCLUDE_HEADERS:
        return True
    own = {_header_basename(h) for h in own_headers if h}
    if base in own:
        return True
    normalised = str(header or "").replace("\\", "/")
    if base in allowed_headers or normalised in allowed_headers:
        return True
    if framing == "<" and base in _C_STANDARD_INCLUDE_HEADERS:
        return True
    return False


def _strip_unsupported_preamble_includes(
    preamble_c: str,
    allowed_headers: set[str],
    *,
    own_headers: Sequence[str] = (),
) -> Tuple[str, Tuple[str, ...]]:
    if not preamble_c:
        return "", ()
    removed: List[str] = []
    kept: List[str] = []
    include_re = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"]+)[>"]')
    for line in str(preamble_c).splitlines():
        match = include_re.match(line)
        if match and not _include_header_allowed(
            match.group(1),
            match.group(2).strip(),
            allowed_headers,
            own_headers=own_headers,
        ):
            removed.append(_header_basename(match.group(2).strip()))
            continue
        kept.append(line)
    return "\n".join(kept).strip(), tuple(removed)


def _normalise_plan_preamble_headers(
    plan_bundle: PlanBundle,
    rtos_contract: Mapping[str, Any],
    *,
    task_package: Optional[Mapping[str, Any]] = None,
) -> PlanBundle:
    api = plan_bundle.api_contract
    if not isinstance(api, Mapping):
        return plan_bundle
    preamble = api.get("preamble_c")
    if not isinstance(preamble, str) or "#include" not in preamble:
        return plan_bundle
    allowed_headers = _rtos_header_allowlist(
        rtos_contract,
        task_package=task_package,
    )
    filtered, removed = _strip_unsupported_preamble_includes(
        preamble,
        allowed_headers,
    )
    if not removed:
        return plan_bundle
    new_api = dict(api)
    new_api["preamble_c"] = filtered
    logger.info(
        "removed unsupported frozen-plan preamble include(s): %s",
        ", ".join(removed),
    )
    return dataclasses.replace(plan_bundle, api_contract=new_api)


def _normalise_bundle_preamble_headers(bundle: SynthesisBundle) -> SynthesisBundle:
    api = bundle.api_contract
    if not isinstance(api, Mapping):
        return bundle
    preamble = api.get("preamble_c")
    if not isinstance(preamble, str) or "#include" not in preamble:
        return bundle
    allowed_headers = set(_C_STANDARD_INCLUDE_HEADERS)
    allowed_headers.update(available_stub_headers(bundle.rtos_id))
    filtered, removed = _strip_unsupported_preamble_includes(
        preamble,
        allowed_headers,
        own_headers=(f"{bundle.device_id}.h",),
    )
    if not removed:
        return bundle
    new_api = dict(api)
    new_api["preamble_c"] = filtered
    logger.info(
        "removed unsupported adapter preamble include(s): %s",
        ", ".join(removed),
    )
    return dataclasses.replace(bundle, api_contract=new_api)


def _normalise_driver_include_headers(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
    *,
    task_package: Optional[Mapping[str, Any]] = None,
) -> SynthesisBundle:
    """Keep generated driver includes within the rendered contract surface."""
    source = str(bundle.driver_source or "")
    header = str(bundle.driver_header or "")
    if not source and not header:
        return bundle
    contract_view: Mapping[str, Any]
    if isinstance(rtos_contract, Mapping) and not rtos_contract.get("rtos") and bundle.rtos_id:
        contract_view = {**rtos_contract, "rtos": bundle.rtos_id}
    else:
        contract_view = rtos_contract

    contract_allowed_headers = _rtos_header_allowlist(
        contract_view,
        task_package=task_package,
    )
    resolvable_headers: set[str] = set()
    for stub_header in available_stub_headers(bundle.rtos_id):
        include_name = _header_include_name(stub_header)
        base = _header_basename(stub_header)
        if include_name:
            resolvable_headers.add(include_name)
        if base and "/" not in include_name:
            resolvable_headers.add(base)
    allowed_headers = set(_C_STANDARD_INCLUDE_HEADERS)
    for raw_header in contract_allowed_headers:
        base = _header_basename(raw_header)
        if base and base in resolvable_headers:
            allowed_headers.add(base)
            allowed_headers.add(str(raw_header).replace("\\", "/"))
    own_header = f"{bundle.device_id}.h"
    new_source, removed_source = _strip_unsupported_preamble_includes(
        source,
        allowed_headers,
        own_headers=(own_header,),
    )
    new_header, removed_header = _strip_unsupported_preamble_includes(
        header,
        allowed_headers,
        own_headers=(own_header,),
    )

    needed_headers = _driver_contract_headers_for_used_tokens(
        f"{new_header}\n{new_source}",
        contract_view,
        resolvable_headers=resolvable_headers,
    )
    header_needed_headers = _driver_contract_headers_for_header_types(
        new_header,
        contract_view,
        resolvable_headers=resolvable_headers,
    )
    if needed_headers:
        new_source = _insert_driver_source_includes(
            new_source,
            needed_headers,
            own_header=own_header,
        )
    if header_needed_headers:
        new_header = _insert_driver_header_includes(
            new_header,
            header_needed_headers,
            own_header=own_header,
        )

    if new_source == source and new_header == header:
        return bundle
    removed = tuple(dict.fromkeys(removed_header + removed_source))
    if removed:
        logger.info(
            "removed unsupported driver include(s): %s",
            ", ".join(removed),
        )
    added = [
        h
        for h in needed_headers
        if h not in _included_header_basenames(source)
    ]
    header_added = [
        h
        for h in header_needed_headers
        if h not in _included_header_basenames(header)
    ]
    if added:
        logger.info(
            "added driver include(s) for used contract symbols: %s",
            ", ".join(added),
        )
    if header_added:
        logger.info(
            "added driver-header include(s) for public contract types: %s",
            ", ".join(header_added),
        )
    return dataclasses.replace(
        bundle,
        driver_header=new_header,
        driver_source=new_source,
    )


def _driver_contract_headers_for_used_tokens(
    code_text: str,
    rtos_contract: Mapping[str, Any],
    *,
    resolvable_headers: Optional[set[str]] = None,
) -> Tuple[str, ...]:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", str(code_text or ""))
    identifiers = set(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", stripped))
    called = {name for name, _line in _iter_c_function_like_symbols(stripped)}
    headers: List[str] = []
    seen: set[str] = set()

    def add(raw_header: Any) -> None:
        include_name = _header_include_name(raw_header)
        base = _header_basename(include_name)
        if (
            not base
            or base in _C_STANDARD_INCLUDE_HEADERS
            or base in seen
        ):
            return
        if (
            resolvable_headers is not None
            and base not in resolvable_headers
            and include_name not in resolvable_headers
        ):
            return
        seen.add(base)
        headers.append(include_name)

    sanitized = sanitize_rtos_contract_for_codegen(rtos_contract)
    api_bindings = sanitized.get("api_bindings")
    if isinstance(api_bindings, Mapping):
        for binding in api_bindings.values():
            if not isinstance(binding, Mapping):
                continue
            symbol = str(binding.get("symbol") or "").strip()
            if not symbol or symbol not in called:
                continue
            for raw_header in binding.get("required_headers") or []:
                add(raw_header)

    support_headers: set[str] = set()
    integration = sanitized.get("integration_contract")
    if isinstance(integration, Mapping):
        for raw_header in integration.get("include_headers") or []:
            include_name = _header_include_name(raw_header)
            base = _header_basename(include_name)
            if include_name:
                support_headers.add(include_name)
            if base:
                support_headers.add(base)
    surface = sanitized.get("codegen_surface")
    if isinstance(surface, Mapping):
        for raw_header in surface.get("support_headers") or []:
            include_name = _header_include_name(raw_header)
            base = _header_basename(include_name)
            if include_name:
                support_headers.add(include_name)
            if base:
                support_headers.add(base)
    rtos_id = str(sanitized.get("rtos") or "").strip()
    if support_headers and rtos_id:
        for token in sorted(called | identifiers):
            for raw_header in stub_headers_declaring_symbol(rtos_id, token):
                include_name = _header_include_name(raw_header)
                base = _header_basename(include_name)
                if include_name in support_headers or base in support_headers:
                    add(include_name)

    for pattern, pattern_headers in _iter_helper_usage_patterns_with_headers(sanitized):
        if not _helper_usage_pattern_used(pattern, identifiers, called, stripped):
            continue
        for raw_header in pattern_headers:
            add(raw_header)

    return tuple(headers)


def _driver_contract_headers_for_header_types(
    header_text: str,
    rtos_contract: Mapping[str, Any],
    *,
    resolvable_headers: Optional[set[str]] = None,
) -> Tuple[str, ...]:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", str(header_text or ""))
    identifiers = set(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", stripped))
    if not identifiers:
        return ()

    headers: List[str] = []
    seen: set[str] = set()

    def add(raw_header: Any) -> None:
        include_name = _header_include_name(raw_header)
        base = _header_basename(include_name)
        if (
            not base
            or base in _C_STANDARD_INCLUDE_HEADERS
            or base in seen
        ):
            return
        if (
            resolvable_headers is not None
            and base not in resolvable_headers
            and include_name not in resolvable_headers
        ):
            return
        seen.add(base)
        headers.append(include_name)

    sanitized = sanitize_rtos_contract_for_codegen(rtos_contract)
    api_bindings = sanitized.get("api_bindings")
    if isinstance(api_bindings, Mapping):
        for binding in api_bindings.values():
            if not isinstance(binding, Mapping):
                continue
            tokens = _signature_type_tokens(str(binding.get("signature") or ""))
            if not tokens or not (tokens & identifiers):
                continue
            for raw_header in binding.get("required_headers") or []:
                add(raw_header)

    for pattern, pattern_headers in _iter_helper_usage_patterns_with_headers(sanitized):
        raw_types = pattern.get("required_types") or []
        if not isinstance(raw_types, Sequence) or isinstance(raw_types, (str, bytes)):
            continue
        matched = False
        for raw_type in raw_types:
            type_text = str(raw_type or "").strip()
            struct_match = re.fullmatch(r"struct\s+([A-Za-z_][A-Za-z0-9_]*)", type_text)
            if struct_match and struct_match.group(1) in identifiers:
                matched = True
                break
            if type_text and type_text in identifiers:
                matched = True
                break
        if matched:
            for raw_header in pattern_headers:
                add(raw_header)

    return tuple(headers)


_C_TYPE_QUALIFIER_OR_BUILTIN = {
    "const",
    "volatile",
    "restrict",
    "struct",
    "union",
    "enum",
    "void",
    "char",
    "short",
    "int",
    "long",
    "signed",
    "unsigned",
    "float",
    "double",
    "bool",
    "size_t",
    "ssize_t",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "uintptr_t",
    "intptr_t",
    "FAR",
    "NEAR",
    "VOID",
}


def _signature_type_tokens(signature: str) -> set[str]:
    text = str(signature or "").strip().rstrip(";")
    if not text or "(" not in text:
        return set()
    open_paren = text.find("(")
    head = text[:open_paren]
    params = text[open_paren + 1:text.rfind(")")] if ")" in text else ""
    tokens: set[str] = set()

    head_ids = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", head)
    if len(head_ids) > 1:
        for token in head_ids[:-1]:
            if token not in _C_TYPE_QUALIFIER_OR_BUILTIN:
                tokens.add(token)

    for raw_param in _split_c_parameters(params):
        param = re.sub(r"\[[^\]]*\]", " ", raw_param)
        ids = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", param)
        ids = [token for token in ids if token not in _C_TYPE_QUALIFIER_OR_BUILTIN]
        if len(ids) > 1:
            ids = ids[:-1]
        for token in ids:
            tokens.add(token)
    return tokens


def _split_c_parameters(params: str) -> Tuple[str, ...]:
    text = str(params or "").strip()
    if not text or text == "void":
        return ()
    parts: List[str] = []
    depth = 0
    start = 0
    for index, ch in enumerate(text):
        if ch == "(":
            depth += 1
        elif ch == ")" and depth:
            depth -= 1
        elif ch == "," and depth == 0:
            parts.append(text[start:index].strip())
            start = index + 1
    tail = text[start:].strip()
    if tail:
        parts.append(tail)
    return tuple(parts)


def _iter_helper_usage_patterns_with_headers(
    value: Any,
    inherited_headers: Sequence[Any] = (),
) -> Tuple[Tuple[Mapping[str, Any], Tuple[Any, ...]], ...]:
    out: List[Tuple[Mapping[str, Any], Tuple[Any, ...]]] = []
    if isinstance(value, Mapping):
        local_headers = list(inherited_headers)
        include_headers = value.get("include_headers")
        if isinstance(include_headers, Sequence) and not isinstance(include_headers, (str, bytes)):
            local_headers.extend(include_headers)
        raw_patterns = value.get("helper_usage_patterns")
        if isinstance(raw_patterns, Sequence) and not isinstance(raw_patterns, (str, bytes)):
            for raw_pattern in raw_patterns:
                if isinstance(raw_pattern, Mapping):
                    out.append((raw_pattern, tuple(local_headers)))
        for nested in value.values():
            if isinstance(nested, (Mapping, list, tuple)):
                out.extend(_iter_helper_usage_patterns_with_headers(nested, local_headers))
    elif isinstance(value, Sequence) and not isinstance(value, (str, bytes)):
        for item in value:
            if isinstance(item, (Mapping, list, tuple)):
                out.extend(_iter_helper_usage_patterns_with_headers(item, inherited_headers))
    return tuple(out)


def _helper_usage_pattern_used(
    pattern: Mapping[str, Any],
    identifiers: set[str],
    called: set[str],
    stripped_code: str,
) -> bool:
    raw_symbols = pattern.get("required_symbols") or []
    if isinstance(raw_symbols, Sequence) and not isinstance(raw_symbols, (str, bytes)):
        for raw in raw_symbols:
            symbol = str(raw or "").strip()
            if symbol and (symbol in called or symbol in identifiers):
                return True

    raw_constants = pattern.get("required_constants") or []
    if isinstance(raw_constants, Sequence) and not isinstance(raw_constants, (str, bytes)):
        for raw in raw_constants:
            constant = str(raw or "").strip()
            if constant and constant in identifiers:
                return True

    raw_types = pattern.get("required_types") or []
    if isinstance(raw_types, Sequence) and not isinstance(raw_types, (str, bytes)):
        for raw in raw_types:
            type_text = str(raw or "").strip()
            if not type_text:
                continue
            struct_match = re.fullmatch(r"struct\s+([A-Za-z_][A-Za-z0-9_]*)", type_text)
            if struct_match:
                if re.search(r"\bstruct\s+" + re.escape(struct_match.group(1)) + r"\b", stripped_code):
                    return True
            elif type_text in identifiers:
                return True
    return False


def _insert_driver_source_includes(
    source: str,
    headers: Sequence[str],
    *,
    own_header: str,
) -> str:
    existing = _included_header_basenames(source)
    to_add: List[str] = []
    for header in headers:
        include_name = _header_include_name(header)
        base = _header_basename(include_name)
        if (
            include_name
            and base not in existing
            and base != _header_basename(own_header)
        ):
            to_add.append(include_name)
    if not to_add:
        return source

    lines = str(source or "").splitlines()
    insert_at = 0
    include_re = re.compile(r'^\s*#\s*include\b')
    for index, line in enumerate(lines):
        stripped = line.strip()
        if include_re.match(line) or not stripped:
            insert_at = index + 1
            continue
        break
    include_lines = [_driver_include_line(header) for header in to_add]
    new_lines = lines[:insert_at] + include_lines + lines[insert_at:]
    return "\n".join(new_lines).rstrip() + "\n"


def _insert_driver_header_includes(
    header: str,
    headers: Sequence[str],
    *,
    own_header: str,
) -> str:
    existing = _included_header_basenames(header)
    to_add: List[str] = []
    for item in headers:
        include_name = _header_include_name(item)
        base = _header_basename(include_name)
        if (
            include_name
            and base not in existing
            and base != _header_basename(own_header)
        ):
            to_add.append(include_name)
    if not to_add:
        return header
    lines = str(header or "").splitlines()
    insert_at = _header_forward_decl_insert_index(lines)
    include_lines = [_driver_include_line(item) for item in to_add]
    new_lines = lines[:insert_at] + include_lines + lines[insert_at:]
    return "\n".join(new_lines).rstrip() + "\n"


def _normalise_header_opaque_struct_forwards(bundle: SynthesisBundle) -> SynthesisBundle:
    """Insert missing forward declarations for opaque ``struct *`` header types."""
    header = str(bundle.driver_header or "")
    needed = _opaque_struct_pointer_names_needing_forward(header)
    if not needed:
        return bundle
    lines = header.splitlines()
    insert_at = _header_forward_decl_insert_index(lines)
    decls = [f"struct {name};" for name in needed]
    new_lines = lines[:insert_at] + decls + [""] + lines[insert_at:]
    logger.info(
        "inserted opaque struct forward declaration(s) in driver header: %s",
        ", ".join(needed),
    )
    return dataclasses.replace(bundle, driver_header="\n".join(new_lines) + "\n")


_PRIMITIVE_DEV_STRUCT_TYPES = {
    "char",
    "short",
    "int",
    "long",
    "float",
    "double",
    "void",
    "bool",
    "uint8_t",
    "uint16_t",
    "uint32_t",
    "uint64_t",
    "int8_t",
    "int16_t",
    "int32_t",
    "int64_t",
    "size_t",
}


def _normalise_api_contract_dev_struct_type(bundle: SynthesisBundle) -> SynthesisBundle:
    api = bundle.api_contract
    if not isinstance(api, Mapping):
        return bundle
    current = str(api.get("dev_struct_type") or "").strip()
    if current and current not in _PRIMITIVE_DEV_STRUCT_TYPES:
        return bundle
    init_call = str(api.get("init_call") or "").strip()
    match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*\(", init_call)
    if not match:
        return bundle
    inferred = _first_pointer_param_type_for_function(bundle.driver_header, match.group(1))
    if not inferred or inferred == current:
        return bundle
    new_api = dict(api)
    new_api["dev_struct_type"] = inferred
    logger.info(
        "normalised api_contract.dev_struct_type from %r to %r based on driver header",
        current,
        inferred,
    )
    return dataclasses.replace(bundle, api_contract=new_api)


def _first_pointer_param_type_for_function(header: str, function_name: str) -> str:
    pattern = re.compile(
        r"\b[A-Za-z_][A-Za-z0-9_\s\*]*?\b"
        + re.escape(function_name)
        + r"\s*\((?P<params>[^;{}]*)\)\s*;",
        re.DOTALL,
    )
    match = pattern.search(str(header or ""))
    if not match:
        return ""
    first_param = match.group("params").split(",", 1)[0].strip()
    param_match = re.match(
        r"(?:const\s+|volatile\s+)*"
        r"(?P<type>struct\s+[A-Za-z_][A-Za-z0-9_]*|[A-Za-z_][A-Za-z0-9_]*)"
        r"\s*(?:const\s+|volatile\s+)*\*",
        first_param,
    )
    return param_match.group("type").strip() if param_match else ""


def _opaque_struct_pointer_names_needing_forward(header: str) -> Tuple[str, ...]:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", str(header or ""))
    pointer_names = {
        match.group(1)
        for match in re.finditer(
            r"\b(?:const\s+)?struct\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:const\s+)?\*",
            stripped,
        )
    }
    if not pointer_names:
        return ()
    defined_or_declared = {
        match.group(1)
        for match in re.finditer(
            r"\bstruct\s+([A-Za-z_][A-Za-z0-9_]*)\s*(?:\{|;)",
            stripped,
        )
    }
    return tuple(sorted(pointer_names - defined_or_declared))


def _header_forward_decl_insert_index(lines: Sequence[str]) -> int:
    insert_at = 0
    for i, line in enumerate(lines):
        stripped = str(line).strip()
        if not stripped:
            insert_at = i + 1
            continue
        if (
            stripped.startswith("#ifndef")
            or stripped.startswith("#define")
            or stripped.startswith("#pragma once")
            or stripped.startswith("#include")
        ):
            insert_at = i + 1
            continue
        break
    return insert_at


def _normalise_adapter_preamble_symbol_headers(
    bundle: SynthesisBundle,
) -> SynthesisBundle:
    """Add adapter preamble includes for RTOS calls used in init setup code."""
    api = bundle.api_contract
    if not isinstance(api, Mapping):
        return bundle
    init_extra = api.get("init_extra_setup_c")
    if not isinstance(init_extra, str) or "(" not in init_extra:
        return bundle

    preamble = str(api.get("preamble_c") or "")
    existing = _included_header_basenames(preamble)
    needed: List[str] = []
    for symbol in _called_function_names(init_extra):
        for header in stub_headers_declaring_symbol(bundle.rtos_id, symbol):
            include_name = _header_include_name(header)
            base = _header_basename(include_name)
            if not base or base in existing or base in _C_STANDARD_INCLUDE_HEADERS:
                continue
            existing.add(base)
            needed.append(include_name)
    if not needed:
        return bundle

    include_block = "\n".join(_driver_include_line(header) for header in needed)
    new_api = dict(api)
    new_api["preamble_c"] = (
        f"{include_block}\n{preamble}".strip()
        if preamble.strip() else include_block
    )
    logger.info(
        "added adapter preamble include(s) for init setup symbols: %s",
        ", ".join(needed),
    )
    return dataclasses.replace(bundle, api_contract=new_api)


def _called_function_names(source: str) -> Tuple[str, ...]:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", str(source or ""))
    out: List[str] = []
    seen: set[str] = set()
    for match in _C_FUNCTION_LIKE_RE.finditer(stripped):
        name = match.group(1)
        if name in _C_CONTROL_OR_BUILTIN_NAMES or name in seen:
            continue
        seen.add(name)
        out.append(name)
    return tuple(out)


def _included_header_basenames(source: str) -> set[str]:
    out: set[str] = set()
    include_re = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.MULTILINE)
    for match in include_re.finditer(str(source or "")):
        base = _header_basename(match.group(1))
        if base:
            out.add(base)
    return out


def _contract_uses_i2c_message_addr(rtos_contract: Mapping[str, Any]) -> bool:
    for binding in _iter_contract_bindings(rtos_contract):
        blob = " ".join(
            str(binding.get(k) or "")
            for k in ("symbol", "signature", "semantic_role")
        ).lower()
        if (
            "i2c_msg" in blob
            or "message" in blob
        ):
            return True
    return False


_C_INT_LITERAL_RE = re.compile(
    r"(?<![A-Za-z0-9_])(?:0[xX][0-9A-Fa-f]+|\d+)"
    r"(?:(?:[uU](?:ll|LL|[lL])?)|(?:(?:ll|LL|[lL])[uU]?))?"
    r"(?![A-Za-z0-9_])"
)
_C_IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_C_FUNCTION_LIKE_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\(")
_C_FUNCTION_DEF_RE = re.compile(
    r"\b([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{",
    re.DOTALL,
)

_C_STANDARD_INCLUDE_HEADERS = {
    "assert.h",
    "ctype.h",
    "errno.h",
    "float.h",
    "inttypes.h",
    "limits.h",
    "math.h",
    "stdbool.h",
    "stddef.h",
    "stdint.h",
    "stdio.h",
    "stdlib.h",
    "string.h",
    "time.h",
}

_RTOS_HEADER_PATH_RE = re.compile(r"[A-Za-z0-9_./\\:-]+\.(?:h|hpp|hh|c|cc|cpp)\b")
_C_CONTROL_OR_BUILTIN_NAMES = {
    "if",
    "for",
    "while",
    "switch",
    "return",
    "sizeof",
    "_Generic",
}
_C_COMMENT_OR_STRING_RE = re.compile(
    r"""
    /\*.*?\*/              |  # block comment
    //[^\r\n]*             |  # line comment
    "(?:\\.|[^"\\])*"      |  # string literal
    '(?:\\.|[^'\\])*'         # char literal
    """,
    re.DOTALL | re.VERBOSE,
)


def _c_integer_literals(source: str) -> Tuple[int, ...]:
    """Return integer literals that appear in real C code, not comments."""
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    values: List[int] = []
    for match in _C_INT_LITERAL_RE.finditer(stripped):
        try:
            values.append(_parse_c_integer_literal(match.group(0)))
        except ValueError:
            continue
    return tuple(values)


def _parse_c_integer_literal(token: str) -> int:
    value = re.sub(r"(?i)(?:u(?:ll|l)?|(?:ll|l)u?)$", "", str(token or ""))
    return int(value, 0)


def _iter_c_function_like_symbols(source: str) -> Tuple[Tuple[str, int], ...]:
    """Return function-like identifiers from comment/string-stripped C text."""
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    out: List[Tuple[str, int]] = []
    for match in _C_FUNCTION_LIKE_RE.finditer(stripped):
        name = match.group(1)
        if name in _C_CONTROL_OR_BUILTIN_NAMES:
            continue
        line_no = stripped.count("\n", 0, match.start()) + 1
        out.append((name, line_no))
    return tuple(out)


def _c_defined_function_names(source: str) -> set[str]:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    names: set[str] = set()
    for match in _C_FUNCTION_DEF_RE.finditer(stripped):
        name = match.group(1)
        if name in _C_CONTROL_OR_BUILTIN_NAMES:
            continue
        names.add(name)
    return names


def _contract_allowed_rtos_function_signatures(
    rtos_contract: Mapping[str, Any],
) -> Dict[str, str]:
    """Return function symbols the platform contract explicitly exposes."""
    out: Dict[str, str] = {}

    def add_symbol(raw_symbol: Any, raw_signature: Any = "") -> None:
        symbol = str(raw_symbol or "").strip()
        if not _C_IDENTIFIER_RE.match(symbol):
            return
        signature = str(raw_signature or "").strip()
        out.setdefault(symbol, signature)
        if signature and not out[symbol]:
            out[symbol] = signature

    for binding in _iter_contract_bindings(rtos_contract):
        if binding.get("allowed_for_codegen") is False:
            continue
        kind = str(binding.get("kind") or "function").strip().lower()
        if kind not in {"", "function", "func"}:
            continue
        add_symbol(binding.get("symbol"), binding.get("signature"))

    def visit_contract_section(section: Any) -> None:
        if not isinstance(section, Mapping):
            return
        for key, value in section.items():
            if isinstance(value, Mapping):
                if str(key).endswith("_api_signatures"):
                    for symbol, signature in value.items():
                        add_symbol(symbol, signature)
                elif str(key).endswith("_signatures"):
                    for symbol, signature in value.items():
                        add_symbol(symbol, signature)
                else:
                    visit_contract_section(value)
            elif (
                isinstance(value, Sequence)
                and not isinstance(value, (str, bytes))
                and str(key).endswith("_symbols")
            ):
                for symbol in value:
                    add_symbol(symbol)

    for section_key in (
        "bus_contract",
        "runtime_contract",
        "integration_contract",
        "device_contract",
    ):
        visit_contract_section(rtos_contract.get(section_key))

    return out


_BUS_NAMESPACE_PREFIXES_BY_KIND: Mapping[str, Tuple[str, ...]] = {
    "i2c": ("i2c_", "smbus_"),
    "smbus": ("i2c_", "smbus_"),
    "spi": ("spi_",),
    "gpio": ("gpio_",),
    "uart": ("uart_", "serial_"),
}
_RTOS_NAMESPACE_ROOTS = {
    "rt",
    "k",
    "device",
    "i2c",
    "smbus",
    "spi",
    "gpio",
    "uart",
    "serial",
    "tx",
    "ch",
    "tos",
    "LOS",
}


def _rtos_contract_namespace_prefixes(
    rtos_contract: Mapping[str, Any],
    bus_kind: str,
    allowed_symbols: Sequence[str],
) -> Tuple[str, ...]:
    prefixes: set[str] = set()
    for symbol in allowed_symbols:
        parts = str(symbol).split("_")
        if len(parts) >= 2 and parts[0] in _RTOS_NAMESPACE_ROOTS:
            prefixes.add(f"{parts[0]}_")
        if len(parts) >= 3:
            prefixes.add("_".join(parts[:2]) + "_")

    prefixes.update(_BUS_NAMESPACE_PREFIXES_BY_KIND.get(str(bus_kind), ()))

    rtos_id = str(rtos_contract.get("rtos") or "").lower()
    if "rtthread" in rtos_id or "rt-thread" in rtos_id:
        prefixes.add("rt_")

    return tuple(sorted(prefixes, key=lambda item: (-len(item), item)))


def _format_allowed_rtos_symbols(
    offending: str,
    prefixes: Sequence[str],
    signatures: Mapping[str, str],
) -> str:
    matched_prefix = next(
        (prefix for prefix in prefixes if offending.startswith(prefix)),
        "",
    )
    if matched_prefix:
        related = [
            symbol for symbol in sorted(signatures)
            if symbol.startswith(matched_prefix)
        ]
    else:
        related = []
    if not related:
        related = sorted(signatures)[:8]

    rendered: List[str] = []
    for symbol in related[:8]:
        signature = str(signatures.get(symbol) or "").strip()
        if signature:
            rendered.append(f"`{signature}`")
        else:
            rendered.append(f"`{symbol}`")
    if len(related) > 8:
        rendered.append("...")
    return ", ".join(rendered) if rendered else "(no allowed function symbols)"


def _contract_unlisted_rtos_call_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    signatures = _contract_allowed_rtos_function_signatures(rtos_contract)
    if not signatures:
        return ()
    prefixes = _rtos_contract_namespace_prefixes(
        rtos_contract,
        bundle.bus_kind,
        tuple(signatures),
    )
    if not prefixes:
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    local_defs = _c_defined_function_names(code_text)
    errors: List[str] = []
    seen: set[str] = set()
    for name, line_no in _iter_c_function_like_symbols(code_text):
        if name in signatures or name in local_defs:
            continue
        if not any(name.startswith(prefix) for prefix in prefixes):
            continue
        key = f"{name}:{line_no}"
        if key in seen:
            continue
        seen.add(key)
        allowed = _format_allowed_rtos_symbols(name, prefixes, signatures)
        errors.append(
            "driver_source/header line "
            f"{line_no}: function-like symbol `{name}` looks like an "
            "external platform or bus API, but SECTION C does not list it for "
            "this task. Treat the platform contract as a call allow-list and use "
            f"only the bound symbol/signature(s): {allowed}. If this is meant "
            "to be a private helper, define it with a device-specific helper "
            "name outside the platform/bus namespace."
        )
    return tuple(errors)


def _config_pointer_call_static_validation_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    """Catch scalar/address arguments passed to config-struct pointer APIs."""
    signatures = _contract_allowed_rtos_function_signatures(rtos_contract)
    if not signatures:
        return ()
    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    errors: List[str] = []
    seen: set[Tuple[str, int, int]] = set()
    for symbol, signature in sorted(signatures.items()):
        config_args = signature_config_pointer_args(signature)
        if not config_args:
            continue
        for line_no, call_args in _iter_c_function_calls(code_text, symbol):
            for index, struct_name, param_name, _raw_arg in config_args:
                if len(call_args) < index:
                    continue
                actual = call_args[index - 1].strip()
                if not _looks_like_scalar_config_pointer_argument(actual, struct_name):
                    continue
                key = (symbol, line_no, index)
                if key in seen:
                    continue
                seen.add(key)
                param = f" `{param_name}`" if param_name else ""
                errors.append(
                    "driver_source/header line "
                    f"{line_no}: `{symbol}` argument #{index}{param} "
                    f"expects a pointer to `struct {struct_name}` according "
                    "to SECTION C, but the generated call passes "
                    f"`{actual}`. Do not pass the device address, NULL, or "
                    "another scalar in a config-pointer slot. Declare and "
                    f"initialize `struct {struct_name} cfg` using the platform "
                    "struct field allow-list and Device IR/task context "
                    "address/frequency/address-length values, then pass "
                    "`&cfg` or an existing config pointer."
                )
    return tuple(errors)


def _looks_like_scalar_config_pointer_argument(actual: str, struct_name: str) -> bool:
    text = str(actual or "").strip()
    if not text:
        return False
    lower = text.lower()
    if lower.startswith("&"):
        return False
    if re.search(r"\b(?:cfg|config)[A-Za-z0-9_]*\b", lower):
        return False
    if re.search(r"\bstruct\s+" + re.escape(struct_name.lower()) + r"\s*\*", lower):
        return False
    stripped = re.sub(r"^\([^)]+\)\s*", "", text).strip()
    lowered = stripped.lower()
    if lowered in {"null", "0", "nullptr"}:
        return True
    if re.fullmatch(r"(?:0x[0-9a-fA-F]+|\d+)[uUlL]*", stripped):
        return True
    if re.search(r"(?:^|[.\->_\s])(?:i2c_)?(?:slave_)?addr(?:ess)?\b", lowered):
        return True
    return False


def _split_c_call_args(arg_text: str) -> List[str]:
    """Split a simple C call argument list on top-level commas."""
    args: List[str] = []
    start = 0
    depth = 0
    for i, ch in enumerate(arg_text):
        if ch in "([{":
            depth += 1
        elif ch in ")]}":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            args.append(arg_text[start:i].strip())
            start = i + 1
    tail = arg_text[start:].strip()
    if tail:
        args.append(tail)
    return args


def _iter_c_function_calls(source: str, func_name: str) -> Tuple[Tuple[int, List[str]], ...]:
    """Return `(line, args)` for calls to `func_name` in comment-stripped C."""
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    calls: List[Tuple[int, List[str]]] = []
    pattern = re.compile(rf"\b{re.escape(func_name)}\s*\(")
    for match in pattern.finditer(stripped):
        open_idx = stripped.find("(", match.start())
        if open_idx < 0:
            continue
        depth = 0
        close_idx: Optional[int] = None
        for i in range(open_idx, len(stripped)):
            ch = stripped[i]
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    close_idx = i
                    break
        if close_idx is None:
            continue
        line_no = stripped.count("\n", 0, match.start()) + 1
        calls.append((line_no, _split_c_call_args(stripped[open_idx + 1:close_idx])))
    return tuple(calls)


def _literal_is_zero(expr: str) -> bool:
    text = str(expr or "").strip()
    if not text:
        return False
    text = re.sub(r"^[({\s]+|[)}\s]+$", "", text)
    return bool(re.fullmatch(r"0(?:[uUlL]*)", text))


def _literal_is_true(expr: str) -> bool:
    text = str(expr or "").strip()
    if not text:
        return False
    text = re.sub(r"^[({\s]+|[)}\s]+$", "", text)
    return text in {"true", "TRUE", "1"}


def _expr_is_null_pointer(expr: str) -> bool:
    text = str(expr or "").strip()
    if not text:
        return False
    text = re.sub(r"^[({\s]+|[)}\s]+$", "", text)
    return text in {"NULL", "RT_NULL", "0", "nullptr"}


def _spi_stream_transfer_static_validation_errors(
    bundle: SynthesisBundle,
) -> Tuple[str, ...]:
    """Catch stream-mode SPI code that clocks out and discards the frame."""
    if bundle.bus_kind != "spi" or bundle.spi_sub_mode != SPI_SUB_STREAM:
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    errors: List[str] = []
    for line_no, args in _iter_c_function_calls(code_text, "rt_spi_send_then_recv"):
        if len(args) < 5:
            continue
        send_length = args[2]
        if _literal_is_zero(send_length):
            continue
        errors.append(
            "driver_source line "
            f"{line_no}: stream-mode SPI uses a send-then-receive helper with "
            f"non-zero/unknown send_length `{send_length}`. For read-only "
            "SPI stream devices every clocked byte is response data; sending "
            "a dummy frame before receiving discards the preloaded frame and "
            "the probe observes zeros. Use a receive-only or same-length "
            "full-duplex transfer for stream reads. Command/register SPI devices are routed as "
            "command/register mode, not stream mode."
        )
    return tuple(errors)


def _spi_register_split_transfer_static_validation_errors(
    bundle: SynthesisBundle,
) -> Tuple[str, ...]:
    """Catch register-mode SPI reads split into send-only then receive-only calls."""
    if bundle.bus_kind != "spi" or bundle.spi_sub_mode != "register":
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    calls = _iter_c_function_calls(code_text, "spi_transfer_bytes")
    errors: List[str] = []
    for (line_no, args), (next_line_no, next_args) in zip(calls, calls[1:]):
        if len(args) < 6 or len(next_args) < 6:
            continue
        first_is_send_only = (
            not _expr_is_null_pointer(args[3])
            and _expr_is_null_pointer(args[4])
        )
        second_is_receive_only = (
            _expr_is_null_pointer(next_args[3])
            and not _expr_is_null_pointer(next_args[4])
        )
        if not (first_is_send_only and second_is_receive_only):
            continue
        if _literal_is_true(args[2]):
            continue
        errors.append(
            "driver_source line "
            f"{line_no}: register-mode SPI appears to split one read "
            "transaction into a send-only `spi_transfer_bytes` call followed "
            f"by a receive-only call on line {next_line_no}, but the first "
            "`cont` argument is not true. This releases chip select after the "
            "register command, so register-mode slaves observe two unrelated "
            "transactions and probe reads return zeros. Use one full-duplex "
            "transfer containing command byte + dummy bytes and read from the "
            "same RX buffer, use a register-transfer helper if the platform contract "
            "exposes it, or keep CS asserted with `cont=true` until the final "
            "receive phase."
        )
    return tuple(errors)


_SPI_FULL_DUPLEX_TEXT_RE = re.compile(
    r"\bfull[-\s]?duplex\b|"
    r"\bsimultaneous(?:ly)?\b.{0,80}\b(?:send|transmit|tx|receive|read|rx)\b|"
    r"\b(?:same|single)\s+(?:spi\s+)?(?:transaction|transfer|exchange)\b.{0,120}\b(?:send|transmit|tx|receive|read|rx)\b|"
    r"\b(?:send|transmit|tx)\b.{0,80}\b(?:and|while|during)\b.{0,80}\b(?:receive|read|rx)\b",
    re.IGNORECASE,
)


def _spi_command_requires_full_duplex(
    device_ir: Optional[Mapping[str, Any]],
    rtos_contract: Mapping[str, Any],
) -> bool:
    """Infer whether command-mode SPI must use one tx/rx exchange."""
    snippets: List[Any] = []
    if isinstance(device_ir, Mapping):
        for key in (
            "access_model",
            "operation_flows",
            "read_sequence",
            "init_sequence",
            "registers_or_commands",
        ):
            value = device_ir.get(key)
            if value is not None:
                snippets.append(value)
    if isinstance(rtos_contract, Mapping):
        for key in ("device_contract", "bus_contract", "integration_contract"):
            value = rtos_contract.get(key)
            if value is not None:
                snippets.append(value)
    if not snippets:
        return False
    text = json.dumps(snippets, ensure_ascii=False, default=str)
    return bool(_SPI_FULL_DUPLEX_TEXT_RE.search(text))


def _spi_command_full_duplex_static_validation_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    if bundle.bus_kind != "spi" or bundle.spi_sub_mode != SPI_SUB_COMMAND:
        return ()
    if not _spi_command_requires_full_duplex(device_ir, rtos_contract):
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    errors: List[str] = []
    for line_no, args in _iter_c_function_calls(code_text, "rt_spi_send_then_recv"):
        if len(args) < 5:
            continue
        recv_length = args[4]
        if _literal_is_zero(recv_length):
            continue
        errors.append(
            "driver_source line "
            f"{line_no}: full-duplex command SPI uses a send-then-receive helper, "
            "which performs a send phase followed by a receive phase and can "
            "clock extra bytes after the command. The Device IR/contract says "
            "the command and response share the same CS-low SPI exchange. Use a "
            "same-length full-duplex transfer API; then extract result "
            "bits from the RX bytes captured during that same transaction."
        )
    return tuple(errors)


_EXPECTED_TX_LITERAL_RE = re.compile(
    r"(?<![A-Za-z0-9_])(?:0[xX][0-9A-Fa-f]+|\d+)"
    r"(?:(?:[uU](?:ll|LL|[lL])?)|(?:(?:ll|LL|[lL])[uU]?))?"
    r"(?![A-Za-z0-9_])"
)


def _literal_ints_from_expected_value(raw: Any) -> Tuple[int, ...]:
    """Best-effort parser for one expected_transactions byte literal value."""
    if isinstance(raw, bool) or raw is None:
        return ()
    if isinstance(raw, int):
        return (raw,)
    if isinstance(raw, float):
        return (int(raw),) if raw.is_integer() else ()
    values: List[int] = []
    for match in _EXPECTED_TX_LITERAL_RE.finditer(str(raw)):
        try:
            values.append(_parse_c_integer_literal(match.group(0)))
        except ValueError:
            continue
    return tuple(values)


def _expected_tx_prefix_options(entry: Mapping[str, Any]) -> Tuple[Tuple[int, ...], ...]:
    """Normalise ``write_prefix_any_of`` to integer byte alternatives."""
    raw = entry.get("write_prefix_any_of")
    if raw is None:
        return ()
    raw_options: Sequence[Any]
    if isinstance(raw, (list, tuple)):
        raw_options = raw
    else:
        raw_options = (raw,)

    options: List[Tuple[int, ...]] = []
    for option in raw_options:
        raw_values: Sequence[Any]
        if isinstance(option, (list, tuple)):
            raw_values = option
        else:
            raw_values = (option,)
        values: List[int] = []
        for raw_value in raw_values:
            values.extend(_literal_ints_from_expected_value(raw_value))
        if values:
            options.append(tuple(values))
    return tuple(options)


def _format_prefix_options(options: Sequence[Sequence[int]]) -> str:
    rendered = []
    for option in options:
        rendered.append("[" + ", ".join(f"0x{value:02X}" for value in option) + "]")
    return "[" + ", ".join(rendered) + "]"


def _prefix_option_present_in_code(
    option: Sequence[int],
    code_literals: set[int],
    *,
    spi_register_hints=None,
) -> bool:
    """Return true when code literals plausibly implement one byte prefix."""
    values = tuple((int(value) & 0xFF) for value in option)
    if not values:
        return False
    if all(value in code_literals for value in values):
        return True
    if len(values) == 2:
        packed_be = (values[0] << 8) | values[1]
        if packed_be in code_literals:
            return True
    if len(values) == 1 and spi_register_hints is not None:
        hints = spi_register_hints
        if getattr(hints, "is_register", False) and getattr(hints, "explicit", False):
            command = values[0]
            register_addr = command & int(getattr(hints, "addr_mask", 0xFF))
            required_masks = []
            rw_mask = int(getattr(hints, "rw_mask", 0)) & 0xFF
            mb_mask = int(getattr(hints, "mb_mask", 0)) & 0xFF
            if rw_mask and (command & rw_mask):
                required_masks.append(rw_mask)
            if mb_mask and (command & mb_mask):
                required_masks.append(mb_mask)
            combined_mask = 0
            for mask in required_masks:
                combined_mask |= mask
            masks_present = all(mask in code_literals for mask in required_masks)
            combined_mask_present = (
                bool(combined_mask) and combined_mask in code_literals
            )
            if register_addr in code_literals and (masks_present or combined_mask_present):
                return True
    return False


def _spi_register_prefix_missing_detail(
    option: Sequence[int],
    code_literals: set[int],
    spi_register_hints=None,
) -> str:
    if len(tuple(option)) != 1 or spi_register_hints is None:
        return ""
    hints = spi_register_hints
    if not (getattr(hints, "is_register", False) and getattr(hints, "explicit", False)):
        return ""

    command = int(tuple(option)[0]) & 0xFF
    addr_mask = int(getattr(hints, "addr_mask", 0xFF)) & 0xFF
    register_addr = command & addr_mask
    parts: List[Tuple[str, int]] = [("register address", register_addr)]
    rw_mask = int(getattr(hints, "rw_mask", 0)) & 0xFF
    mb_mask = int(getattr(hints, "mb_mask", 0)) & 0xFF
    if rw_mask and (command & rw_mask):
        parts.append(("read mask", rw_mask))
    if mb_mask and (command & mb_mask):
        parts.append(("multi-byte/burst mask", mb_mask))

    required_mask_values = [
        value
        for label, value in parts
        if label != "register address"
    ]
    combined_mask = 0
    for value in required_mask_values:
        combined_mask |= value
    combined_mask_present = bool(combined_mask) and combined_mask in code_literals

    def _component_present(label: str, value: int) -> bool:
        if value in code_literals:
            return True
        if label == "register address":
            return False
        return combined_mask_present and ((combined_mask & value) == value)

    missing = [
        f"{label} {_hex_byte(value)}"
        for label, value in parts
        if not _component_present(label, value)
    ]
    if not missing:
        return ""
    present = [
        (
            f"{label} {_hex_byte(value)} via combined mask "
            f"{_hex_byte(combined_mask)}"
            if label != "register address"
            and value not in code_literals
            and combined_mask_present
            else f"{label} {_hex_byte(value)}"
        )
        for label, value in parts
        if _component_present(label, value)
    ]
    detail = (
        f"SPI register-protocol command {_hex_byte(command)} decomposes to "
        + ", ".join(f"{label} {_hex_byte(value)}" for label, value in parts)
        + ". "
    )
    if present:
        detail += "The code contains " + ", ".join(present) + ", "
    else:
        detail += "The code contains none of those command components, "
    detail += "but is missing " + ", ".join(missing) + ". "
    if any(label.startswith("multi-byte") for label, _ in parts):
        detail += (
            "For burst reads, build the command as register address OR read "
            "mask OR multi-byte/burst mask, not just register address OR read "
            "mask. "
        )
    return detail


def _spi_register_expected_read_command_mix(
    bundle: SynthesisBundle,
    spi_register_hints,
) -> Tuple[bool, bool]:
    if spi_register_hints is None:
        return (False, False)
    hints = spi_register_hints
    if not (
        getattr(hints, "is_register", False)
        and getattr(hints, "explicit", False)
        and bool(getattr(hints, "read_when_set", True))
    ):
        return (False, False)
    rw_mask = int(getattr(hints, "rw_mask", 0)) & 0xFF
    mb_mask = int(getattr(hints, "mb_mask", 0)) & 0xFF
    if not rw_mask or not mb_mask:
        return (False, False)
    expected = (bundle.test_plan or {}).get("expected_transactions")
    if not isinstance(expected, Sequence) or isinstance(expected, (str, bytes)):
        return (False, False)

    has_single_read = False
    has_burst_read = False
    for entry in expected:
        if not isinstance(entry, Mapping):
            continue
        for option in _expected_tx_prefix_options(entry):
            if len(option) != 1:
                continue
            command = int(option[0]) & 0xFF
            if not (command & rw_mask):
                continue
            if command & mb_mask:
                has_burst_read = True
            else:
                has_single_read = True
    return (has_single_read, has_burst_read)


def _line_references_token(line: str, token: str) -> bool:
    if _C_IDENTIFIER_RE.match(token):
        return bool(re.search(rf"\b{re.escape(token)}\b", line))
    return token.lower() in line.lower()


def _integer_reference_tokens_for_value(source: str, value: int) -> Tuple[str, ...]:
    value = int(value) & 0xFF
    tokens = {
        f"0x{value:02X}",
        f"0x{value:02x}",
        str(value),
    }
    for name, define_value in _c_integer_defines(source).items():
        if (int(define_value) & 0xFF) == value:
            tokens.add(name)
    return tuple(sorted(tokens, key=lambda item: (-len(item), item)))


def _mb_mask_line_is_length_gated(lines: Sequence[str], index: int) -> bool:
    window_start = max(0, index - 2)
    window = " ".join(lines[window_start:index + 1]).lower()
    if re.search(
        r"\bif\s*\([^)]*\b(?:len|length|count|size|nbytes|n_bytes|read_len)\b"
        r"[^)]*(?:>\s*1|>=\s*2|!=\s*1)",
        window,
    ):
        return True
    line = lines[index].lower()
    if "?" in line and re.search(
        r"\b(?:len|length|count|size|nbytes|n_bytes|read_len)\b",
        line,
    ):
        return True
    return False


def _spi_register_unconditional_mb_mask_static_validation_errors(
    bundle: SynthesisBundle,
    spi_register_hints,
) -> Tuple[str, ...]:
    has_single_read, has_burst_read = _spi_register_expected_read_command_mix(
        bundle,
        spi_register_hints,
    )
    if not (has_single_read and has_burst_read):
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", code_text)
    mb_mask = int(getattr(spi_register_hints, "mb_mask", 0)) & 0xFF
    if not mb_mask:
        return ()
    mb_tokens = _integer_reference_tokens_for_value(stripped, mb_mask)
    if not mb_tokens:
        return ()

    errors: List[str] = []
    lines = stripped.splitlines()
    for index, line in enumerate(lines):
        text = line.strip()
        if not text or text.startswith("#"):
            continue
        if "|" not in text:
            continue
        if not any(_line_references_token(text, token) for token in mb_tokens):
            continue
        if not re.search(r"\b(?:reg|addr|address|register)\b", text.lower()):
            continue
        if _mb_mask_line_is_length_gated(lines, index):
            continue
        errors.append(
            "driver_source line "
            f"{index + 1}: SPI register protocol has both single-byte read "
            "commands and burst-read commands, but this line applies the "
            f"multi-byte/burst mask {_hex_byte(mb_mask)} unconditionally: "
            f"{text!r}. Build the command as register address OR read mask, "
            "and add the multi-byte/burst mask only when the requested read "
            "length is greater than 1; single-byte identity/status reads must "
            "not set the burst bit."
        )
    return tuple(errors)


def _flow_required_delay_prefixes(
    device_ir: Optional[Mapping[str, Any]],
    *,
    eval_class: str = "",
) -> Tuple[Tuple[Tuple[int, ...], int, str], ...]:
    if not isinstance(device_ir, Mapping):
        return ()
    flows = device_ir.get("operation_flows")
    if not isinstance(flows, Sequence) or isinstance(flows, (str, bytes)):
        return ()
    selected_indexes: Optional[set[int]] = None
    if eval_class and isinstance(flows, list):
        selected_indexes = {
            idx
            for idx, _flow in _select_codegen_operation_flows(
                device_ir,
                flows,
                eval_class=eval_class,
            )
        }
    out: List[Tuple[Tuple[int, ...], int, str]] = []
    for flow_index, flow in enumerate(flows):
        if not isinstance(flow, Mapping):
            continue
        if selected_indexes is not None and flow_index not in selected_indexes:
            continue
        steps = flow.get("steps")
        if not isinstance(steps, Sequence) or isinstance(steps, (str, bytes)):
            continue
        prefixes: List[Tuple[int, ...]] = []
        max_delay = 0
        for step in steps:
            if not isinstance(step, Mapping):
                continue
            tx = step.get("transaction")
            if isinstance(tx, Mapping) and str(tx.get("kind") or "") in {"write", "write_then_read"}:
                prefix = tuple(
                    value & 0xFF
                    for value in (
                        _parse_int_literal(item)
                        for item in (tx.get("bytes") or [])
                    )
                    if value is not None
                )
                if prefix:
                    prefixes.append(prefix)
            if str(step.get("op") or "") == "delay":
                delay = _parse_int_literal(step.get("delay_ms"))
                if delay is not None:
                    max_delay = max(max_delay, int(delay))
        if max_delay <= 0:
            continue
        flow_id = str(flow.get("flow_id") or "operation_flow")
        for prefix in prefixes:
            out.append((prefix, max_delay, flow_id))
    return tuple(out)


_DELAY_LINE_RE = re.compile(
    r"(?:delay|mdelay|msleep|sleep|k_sleep|k_msleep|tos_task_delay)",
    re.IGNORECASE,
)
_C_DEFINE_INT_RE = re.compile(
    r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s+"
    r"((?:0[xX][0-9A-Fa-f]+)|(?:\d+))\b"
)
_C_WIDE_INTERMEDIATE_RE = re.compile(
    r"\b(?:u?int64_t|long\s+long|intmax_t|uintmax_t)\b"
)


def _c_integer_defines(source: str) -> Dict[str, int]:
    defines: Dict[str, int] = {}
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    for line in stripped.splitlines():
        m = _C_DEFINE_INT_RE.match(line)
        if not m:
            continue
        try:
            defines[m.group(1)] = int(m.group(2), 0)
        except ValueError:
            continue
    return defines


def _max_delay_literal_ms(source: str) -> int:
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", source)
    defines = _c_integer_defines(source)
    max_delay = 0
    for line in stripped.splitlines():
        if not _DELAY_LINE_RE.search(line):
            continue
        for value in _c_integer_literals(line):
            max_delay = max(max_delay, int(value))
        for name, value in defines.items():
            if re.search(rf"\b{re.escape(name)}\b", line):
                max_delay = max(max_delay, int(value))
    return max_delay


def _scaled_conversion_large_coefficients(
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[int, ...]:
    if not isinstance(device_ir, Mapping):
        return ()
    formulae = device_ir.get("conversion_formulae")
    if not isinstance(formulae, Sequence) or isinstance(formulae, (str, bytes)):
        return ()
    coeffs: set[int] = set()
    for formula in formulae:
        if not isinstance(formula, Mapping):
            continue
        text = json.dumps(formula, ensure_ascii=False).lower()
        if "milli" not in text and "micro" not in text:
            continue
        for match in _EXPECTED_TX_LITERAL_RE.finditer(text):
            try:
                value = int(match.group(0), 0)
            except ValueError:
                continue
            if value >= 100000:
                coeffs.add(value)
    return tuple(sorted(coeffs))


def _scaled_conversion_static_validation_errors(
    bundle: SynthesisBundle,
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    coeffs = _scaled_conversion_large_coefficients(device_ir)
    if not coeffs:
        return ()
    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", code_text)
    if _C_WIDE_INTERMEDIATE_RE.search(stripped):
        return ()
    code_literals = set(_c_integer_literals(code_text))
    used = sorted(value for value in coeffs if value in code_literals)
    if not used:
        return ()
    rendered = ", ".join(str(value) for value in used)
    return (
        "driver_source uses scaled conversion coefficient(s) "
        f"{rendered} from Device IR milli/micro-unit formulae without any "
        "`int64_t`, `uint64_t`, or `long long` intermediate. Promote the "
        "raw-code multiplication/division to a 64-bit intermediate before "
        "assigning back to the public output type; 16-bit or wider raw "
        "fields can overflow `int32_t` during `raw * scale` even when the "
        "final result fits.",
    )


def _delay_static_validation_errors(
    bundle: SynthesisBundle,
    device_ir: Optional[Mapping[str, Any]],
) -> Tuple[str, ...]:
    required = _flow_required_delay_prefixes(
        device_ir,
        eval_class=bundle.eval_class,
    )
    if not required:
        return ()
    expected = (bundle.test_plan or {}).get("expected_transactions")
    if not isinstance(expected, Sequence) or isinstance(expected, (str, bytes)):
        return ()
    frozen_options: set[Tuple[int, ...]] = set()
    for entry in expected:
        if isinstance(entry, Mapping):
            frozen_options.update(_expected_tx_prefix_options(entry))
    if not frozen_options:
        return ()

    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    code_literals = set(_c_integer_literals(code_text))
    max_delay = _max_delay_literal_ms(code_text)
    errors: List[str] = []
    for prefix, required_delay, flow_id in required:
        if prefix not in frozen_options:
            continue
        if not _prefix_option_present_in_code(prefix, code_literals):
            continue
        if max_delay >= required_delay:
            continue
        errors.append(
            "driver_source/header implement operation_flow "
            f"{flow_id!r} prefix={_format_prefix_options((prefix,))}, "
            f"but no delay/sleep call with at least {required_delay} ms "
            f"was found (largest delay-like literal/macro is {max_delay} ms). "
            "Use Device IR delay_ms/max measurement time rather than a "
            "typical datasheet value."
        )
    return tuple(errors)


def _rtc_year_static_validation_errors(bundle: SynthesisBundle) -> Tuple[str, ...]:
    if str(bundle.eval_class) != EVAL_CLASS_RTC:
        return ()
    api = bundle.api_contract or {}
    time_fields = api.get("time_fields")
    year_expr = ""
    if isinstance(time_fields, Mapping):
        year_expr = str(time_fields.get("year") or "")
    from_in = str(api.get("time_struct_from_in") or "")
    if not year_expr and not from_in:
        return ()

    code_text = _C_COMMENT_OR_STRING_RE.sub(
        " ",
        f"{bundle.driver_header}\n{bundle.driver_source}",
    )
    errors: List[str] = []
    adapter_adds_epoch = bool(re.search(r"\+\s*2000\b", year_expr))
    adapter_subtracts_epoch = bool(re.search(r"\b(?:in|input)\s*->\s*year\s*-\s*2000\b", from_in))
    driver_get_adds_epoch = bool(re.search(
        r"(?:->|\.)\s*year\s*=\s*(?:\([^;=]*\)\s*)?2000\s*\+",
        code_text,
    ))
    driver_set_subtracts_epoch = bool(re.search(
        r"\byear\b\s*-\s*2000\b",
        code_text,
    ))
    if adapter_adds_epoch and driver_get_adds_epoch:
        errors.append(
            "rtc year conversion is applied twice: api_contract.time_fields.year "
            f"{year_expr!r} adds 2000, while driver_source also assigns a full "
            "Gregorian year with `2000 + ...` into the driver time struct. "
            "Choose one convention. With this frozen adapter contract, the "
            "driver-native year field must stay as the device/register offset "
            "0..99, and only the adapter should add 2000."
        )
    if adapter_subtracts_epoch and driver_set_subtracts_epoch:
        errors.append(
            "rtc set_time year conversion is applied twice: "
            "api_contract.time_struct_from_in already converts adapter input "
            f"with {from_in!r}, while driver_source also subtracts 2000 from "
            "the driver-native year before writing BCD. With this frozen "
            "adapter contract, write the driver-native 0..99 year directly; "
            "do not subtract 2000 again inside the driver."
        )
    return tuple(errors)


def _iter_helper_usage_patterns(
    rtos_contract: Mapping[str, Any],
) -> Tuple[Mapping[str, Any], ...]:
    patterns: List[Mapping[str, Any]] = []
    for section_key in ("connection", "integration_contract"):
        section = rtos_contract.get(section_key)
        if not isinstance(section, Mapping):
            continue
        raw_patterns = section.get("helper_usage_patterns") or []
        if not isinstance(raw_patterns, Sequence) or isinstance(raw_patterns, (str, bytes)):
            continue
        for raw_pattern in raw_patterns:
            if isinstance(raw_pattern, Mapping):
                patterns.append(raw_pattern)
    return tuple(patterns)


def _helper_usage_redeclaration_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    constants, types = _helper_usage_declared_names(rtos_contract)
    if not constants and not types:
        return ()
    code_text = f"{bundle.driver_header}\n{bundle.driver_source}"
    stripped = _C_COMMENT_OR_STRING_RE.sub(" ", code_text)
    errors: List[str] = []
    seen: set[str] = set()

    for constant, pattern_id in constants:
        key = f"constant:{constant}"
        if key in seen:
            continue
        if re.search(r"(?m)^\s*#\s*define\s+" + re.escape(constant) + r"\b", stripped):
            seen.add(key)
            errors.append(
                "driver_source/header redefine helper usage constant "
                f"`{constant}` from `{pattern_id}`. It is provided by "
                "the platform/task header in SECTION C; remove the local "
                "`#define` and use the existing constant so the runtime "
                "helper sees the documented value."
            )

    for type_text, key_name, is_struct, pattern_id in types:
        key = f"type:{type_text}"
        if key in seen:
            continue
        if is_struct:
            pattern_re = (
                r"\b(?:typedef\s+)?struct\s+"
                + re.escape(key_name)
                + r"\s*\{"
            )
        else:
            pattern_re = (
                r"\btypedef\b[^;{}]*\b"
                + re.escape(key_name)
                + r"\s*;"
            )
        if re.search(pattern_re, stripped, re.DOTALL):
            seen.add(key)
            errors.append(
                "driver_source/header redeclare helper usage type "
                f"`{type_text}` from `{pattern_id}`. It is provided by "
                "the platform/task header in SECTION C; remove the local "
                "type definition and include/use the existing declaration."
            )
    return tuple(errors)


def _helper_usage_declared_names(
    rtos_contract: Mapping[str, Any],
) -> Tuple[Tuple[Tuple[str, str], ...], Tuple[Tuple[str, str, bool, str], ...]]:
    constants: List[Tuple[str, str]] = []
    types: List[Tuple[str, str, bool, str]] = []
    for pattern in _iter_helper_usage_patterns(rtos_contract):
        pattern_id = str(pattern.get("pattern_id") or "helper_usage_pattern")
        raw_constants = pattern.get("required_constants") or []
        if isinstance(raw_constants, Sequence) and not isinstance(raw_constants, (str, bytes)):
            for raw_constant in raw_constants:
                constant = str(raw_constant or "").strip()
                if _C_IDENTIFIER_RE.match(constant):
                    constants.append((constant, pattern_id))

        raw_types = pattern.get("required_types") or []
        if isinstance(raw_types, Sequence) and not isinstance(raw_types, (str, bytes)):
            for raw_type in raw_types:
                type_text = str(raw_type or "").strip()
                struct_match = re.fullmatch(r"struct\s+([A-Za-z_][A-Za-z0-9_]*)", type_text)
                if struct_match:
                    types.append((type_text, struct_match.group(1), True, pattern_id))
                elif _C_IDENTIFIER_RE.match(type_text):
                    types.append((type_text, type_text, False, pattern_id))
    return tuple(constants), tuple(types)


def _strip_helper_usage_redeclarations_from_text(
    text: str,
    constants: Sequence[Tuple[str, str]],
    types: Sequence[Tuple[str, str, bool, str]],
) -> str:
    out = str(text or "")
    for constant, _pattern_id in constants:
        out = re.sub(
            r"(?m)^[ \t]*#[ \t]*define[ \t]+"
            + re.escape(constant)
            + r"\b[^\r\n]*(?:\r?\n)?",
            "",
            out,
        )
    for _type_text, key_name, is_struct, _pattern_id in types:
        if is_struct:
            out = re.sub(
                r"(?ms)^[ \t]*(?:typedef[ \t]+)?struct[ \t]+"
                + re.escape(key_name)
                + r"[ \t]*\{.*?\}[ \t]*(?:[A-Za-z_][A-Za-z0-9_]*[ \t]*)?;[ \t]*(?:\r?\n)?",
                "",
                out,
            )
        else:
            out = re.sub(
                r"(?ms)^[ \t]*typedef\b[^;{}]*\b"
                + re.escape(key_name)
                + r"[ \t]*;[ \t]*(?:\r?\n)?",
                "",
                out,
            )
    return out


def _normalise_helper_usage_redeclarations(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
) -> SynthesisBundle:
    constants, types = _helper_usage_declared_names(rtos_contract)
    if not constants and not types:
        return bundle
    new_header = _strip_helper_usage_redeclarations_from_text(
        bundle.driver_header,
        constants,
        types,
    )
    new_source = _strip_helper_usage_redeclarations_from_text(
        bundle.driver_source,
        constants,
        types,
    )
    if new_header == bundle.driver_header and new_source == bundle.driver_source:
        return bundle
    logger.info("removed local redeclarations for helper usage pattern symbols")
    return dataclasses.replace(
        bundle,
        driver_header=new_header,
        driver_source=new_source,
    )


def _driver_code_static_validation_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
    device_ir: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
) -> Tuple[str, ...]:
    """Return deterministic code-only errors before expensive runtime layers."""
    errors: List[str] = []
    sanitized_contract = sanitize_rtos_contract_for_codegen(rtos_contract)
    code_literals = set(_c_integer_literals(
        f"{bundle.driver_header}\n{bundle.driver_source}"
    ))
    errors.extend(_contract_unlisted_rtos_call_errors(bundle, sanitized_contract))
    errors.extend(forbidden_surface_usage_errors(
        bundle.driver_header,
        bundle.driver_source,
        rtos_contract,
    ))
    errors.extend(struct_field_validation_errors(
        bundle.driver_header,
        bundle.driver_source,
        rtos_contract,
    ))
    errors.extend(_config_pointer_call_static_validation_errors(
        bundle,
        sanitized_contract,
    ))
    errors.extend(_helper_usage_redeclaration_errors(bundle, sanitized_contract))
    spi_register_hints = None
    if bundle.bus_kind == "spi" and str(bundle.spi_sub_mode or "") == "register":
        spi_register_hints = spi_protocol_hints(
            device_ir=device_ir if isinstance(device_ir, Mapping) else None,
            task_package=task_package,
            default_proto="register",
    )
    if bundle.bus_kind in ("i2c", "smbus", "spi"):
        expected = (bundle.test_plan or {}).get("expected_transactions")
        if isinstance(expected, Sequence) and not isinstance(expected, (str, bytes)):
            for index, raw_entry in enumerate(expected):
                if not isinstance(raw_entry, Mapping):
                    continue
                options = _expected_tx_prefix_options(raw_entry)
                if not options:
                    continue
                if any(
                    _prefix_option_present_in_code(
                        option,
                        code_literals,
                        spi_register_hints=spi_register_hints,
                    )
                    for option in options
                ):
                    continue
                # Memory address bytes are computed dynamically from parameters.
                if str(bundle.eval_class) == "memory":
                    continue
                # Dynamic channel command bytes can cover related prefixes.
                if (
                    str(bundle.eval_class) == "multi_channel"
                    and str(bundle.bus_kind) == "spi"
                ):
                    _any_prefix_ok = any(
                        _prefix_option_present_in_code(
                            option, code_literals,
                            spi_register_hints=spi_register_hints,
                        )
                        for _e in expected
                        if isinstance(_e, Mapping)
                        for option in _expected_tx_prefix_options(_e)
                    )
                    _has_ch_shift = bool(
                        re.search(
                            r'\bchannel\b.*<<|<<.*\bchannel\b|\bch\b.*<<',
                            bundle.driver_source,
                        )
                    )
                    if _any_prefix_ok and _has_ch_shift:
                        continue
                spi_detail = " ".join(
                    detail
                    for detail in (
                        _spi_register_prefix_missing_detail(
                            option,
                            code_literals,
                            spi_register_hints=spi_register_hints,
                        )
                        for option in options
                    )
                    if detail
                )
                errors.append(
                    "driver_source/header do not contain any literal byte "
                    "sequence required by frozen "
                    "test_plan.expected_transactions entry "
                    f"#{index} phase={raw_entry.get('phase', '?')!r} "
                    f"addr_or_pin={raw_entry.get('addr_or_pin', '?')!r} "
                    f"write_prefix_any_of={_format_prefix_options(options)}. "
                    f"{spi_detail}"
                    "Implement the corresponding bus write/write_then_read "
                    "transaction; do not collapse separate register pointers "
                    "into a burst read unless Device IR explicitly states the "
                    "target registers are contiguous and burst-readable."
                )
    errors.extend(_spi_register_unconditional_mb_mask_static_validation_errors(
        bundle,
        spi_register_hints,
    ))
    if (
        bundle.bus_kind in ("i2c", "smbus")
        and _contract_uses_i2c_message_addr(sanitized_contract)
    ):
        for lineno, raw_line in enumerate(bundle.driver_source.splitlines(), 1):
            line = raw_line.strip()
            if "<<" not in line:
                continue
            lowered = line.lower()
            if "hal_i2c" in lowered or "devaddress" in lowered:
                continue
            if re.search(r"\b(?:i2c_addr|slave_addr|slave_address|addr)\b", lowered):
                errors.append(
                    "driver_source line "
                    f"{lineno}: message-struct I2C APIs use 7-bit logical "
                    "addresses in `.addr`, but this line left-shifts an "
                    f"address: {line!r}. Assign the Device IR / Section E "
                    "address directly and keep read/write direction in the "
                    "message flags."
                )
    errors.extend(_delay_static_validation_errors(bundle, device_ir))
    errors.extend(_scaled_conversion_static_validation_errors(bundle, device_ir))
    errors.extend(_rtc_year_static_validation_errors(bundle))
    errors.extend(_spi_stream_transfer_static_validation_errors(bundle))
    errors.extend(_spi_register_split_transfer_static_validation_errors(bundle))
    errors.extend(_spi_command_full_duplex_static_validation_errors(
        bundle, sanitized_contract, device_ir,
    ))
    return tuple(errors)


def _driver_code_pre_syntax_validation_errors(
    bundle: SynthesisBundle,
    rtos_contract: Mapping[str, Any],
) -> Tuple[str, ...]:
    """Return surface checks safe to run before C syntax compilation."""
    sanitized_contract = sanitize_rtos_contract_for_codegen(rtos_contract)
    return _config_pointer_call_static_validation_errors(
        bundle,
        sanitized_contract,
    )


def _generate_adapter(
    bundle: SynthesisBundle,
    classify_result: ClassifyResult,
    api_contract_overrides: Optional[Mapping[str, Any]] = None,
) -> Tuple[Optional[GeneratedAdapter], Optional[str]]:
    """Generate ``<device>_eval_adapter.c`` from the validated api_contract."""
    api_contract: Dict[str, Any] = dict(bundle.api_contract)
    if api_contract_overrides:
        for k, v in api_contract_overrides.items():
            api_contract.setdefault(k, v)
    try:
        # Pass the header filename, not the header source text.
        adapter = generate_adapter(
            classify_result,
            api_contract,
            device_id=bundle.device_id,
        )
        return adapter, None
    except AdapterContractError as e:
        logger.warning(
            "adapter_generator rejected bundle.api_contract (attempt %d): %s",
            bundle.attempt, e,
        )
        return None, str(e)
    except Exception as e:  # pragma: no cover - defensive
        logger.error(
            "adapter_generator raised unexpected %s: %s",
            type(e).__name__, e,
        )
        return None, f"{type(e).__name__}: {e}"


def _write_driver_and_adapter(
    bundle: SynthesisBundle,
    adapter: GeneratedAdapter,
    stage_dir: Path,
) -> Tuple[Path, Path]:
    """Materialise driver + adapter to disk for link_mode_compile."""
    stage_dir.mkdir(parents=True, exist_ok=True)
    driver_dir = stage_dir / "driver"
    driver_dir.mkdir(exist_ok=True)
    header_path = driver_dir / f"{bundle.device_id}.h"
    source_path = driver_dir / f"{bundle.device_id}.c"
    header_path.write_text(bundle.driver_header, encoding="utf-8")
    source_path.write_text(bundle.driver_source, encoding="utf-8")
    adapter_path = stage_dir / f"{bundle.device_id}_eval_adapter.c"
    adapter_path.write_text(adapter.source_c, encoding="utf-8")
    return driver_dir, adapter_path


def _run_link_compile(
    bundle: SynthesisBundle,
    adapter: GeneratedAdapter,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    stage_dir: Path,
    *,
    bus_instance: Optional[str] = None,
    timeout: int = 60,
) -> CompileResult:
    """Link driver, adapter, harness, and stubs into an ELF."""
    driver_dir, adapter_path = _write_driver_and_adapter(
        bundle, adapter, stage_dir,
    )
    out_dir = stage_dir / "out"
    return link_mode_compile(
        driver_dir=driver_dir,
        adapter_path=adapter_path,
        eval_class=classify_result.eval_class,
        bus_kind=routing.bus_kind or classify_result.bus_type,
        rtos_id=bundle.rtos_id,
        out_dir=out_dir,
        device_id=bundle.device_id,
        bus_instance=bus_instance,
        timeout=timeout,
    )


def _run_probe(
    elf_path: Path,
    bundle: SynthesisBundle,
    device_ir: Mapping[str, Any],
    classify_result: ClassifyResult,
    routing: RoutingResult,
    work_dir: Path,
    *,
    api_contract_overrides: Optional[Mapping[str, Any]] = None,
    probe_meta_overrides: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    timeout_per_stim: int,
    sleep_s: int,
    stop_on_error: bool,
) -> Tuple[ProbeMeta, Tuple[ProbeStimulus, ...], Tuple[ProbeOutcome, ...]]:
    """Run generated test stimuli through the runtime probe."""
    merged_api: Dict[str, Any] = dict(bundle.api_contract)
    if api_contract_overrides:
        for k, v in api_contract_overrides.items():
            merged_api.setdefault(k, v)

    meta = build_probe_meta(
        device_ir=device_ir,
        classify_result=classify_result,
        routing_result=routing,
        api_contract=merged_api,
        task_package=task_package,
        overrides=probe_meta_overrides,
        expected_transactions=(bundle.test_plan or {}).get(
            "expected_transactions"
        ),
    )
    stimuli = tuple(build_probe_stimuli(bundle.test_plan))
    if not stimuli:
        logger.warning(
            "test_plan.test_stimuli was empty for device=%s; "
            "Runtime probe cannot run.",
            bundle.device_id,
        )
        return meta, (), ()

    outcomes = tuple(probe_all_stimuli(
        elf_path=elf_path,
        meta=meta,
        stimuli=stimuli,
        work_dir=work_dir,
        routing=routing,
        timeout_per_stim=timeout_per_stim,
        sleep_s=sleep_s,
        stop_on_error=stop_on_error,
    ))
    return meta, stimuli, outcomes


def _probe_outcome_passed(
    outcome: ProbeOutcome,
    stim: Optional[ProbeStimulus] = None,
) -> bool:
    """One probe passes only if runtime status and expected values pass."""
    if outcome.error:
        return False
    if not outcome.boot_detected:
        return False
    if not outcome.test_done:
        return False
    expects_error = (
        stim is not None
        and stim.expected_err is not None
        and int(stim.expected_err) != 0
    )
    if not expects_error and not outcome.result_pass:
        return False
    return check_probe_expectations(outcome, stim).ok


def _probe_all_passed(
    outcomes: Sequence[ProbeOutcome],
    stimuli: Optional[Sequence[ProbeStimulus]] = None,
) -> bool:
    """A probe set counts as passing iff every outcome is a clean pass."""
    if not outcomes:
        return False
    stim_map = {s.name: s for s in (stimuli or ())}
    for o in outcomes:
        if not _probe_outcome_passed(o, stim_map.get(o.stimulus_name)):
            return False
    return True


def _score_attempt(rec: AttemptRecord) -> int:
    """Order attempts for "best-of" selection when the loop exhausts."""
    base = _LAYER_SCORE.get(rec.layer_failed, 0) * 1000
    if rec.probe_outcomes:
        stim_map = {s.name: s for s in rec.probe_stimuli}
        passing = sum(
            1 for o in rec.probe_outcomes
            if _probe_outcome_passed(o, stim_map.get(o.stimulus_name))
        )
        base += passing
    return base


# Feedback composition

def _compile_errors_as_strings(result: Optional[CompileResult | StubCompileResult]) -> Tuple[str, ...]:
    """Flatten ``CompileResult`` or ``StubCompileResult`` errors to plain strings."""
    if result is None:
        return ()
    errs = result.errors
    if not errs:
        return ()
    out: List[str] = []
    for e in errs:
        if isinstance(e, str):
            out.append(e)
        elif hasattr(e, "format") and callable(e.format):
            try:
                out.append(e.format())
            except Exception:
                out.append(str(e))
        else:
            out.append(str(e))
    return tuple(out)


def _build_attempt_feedback(
    attempt: int,
    tracker: InvariantsTracker,
    *,
    synthesis_error: Optional[SynthesisError],
    bundle: Optional[SynthesisBundle],
    syntax_result: Optional[StubCompileResult],
    consistency_report: Optional[ConsistencyReport],
    coverage_report: Optional[CoverageReport],
    adapter_error: Optional[str],
    link_result: Optional[CompileResult],
    probe_outcomes: Sequence[ProbeOutcome],
    probe_stimuli: Sequence[ProbeStimulus],
) -> str:
    """Compose the ``prior_feedback`` string for the NEXT attempt."""
    invariant_lines = tracker.to_feedback()

    # Gather compile errors from either the syntax or the link layer,
    # whichever failed. Feed both to renderer if both have content.
    compile_errs: List[str] = list(
        _compile_errors_as_strings(syntax_result)
    )
    compile_errs.extend(_compile_errors_as_strings(link_result))

    # Adapter-generator failures are surfaced with compile diagnostics.
    if adapter_error:
        compile_errs.append(f"adapter generator: {adapter_error}")

    # Keep compiler output direct; API choices come from the platform contract.
    compile_errs_with_hints = list(compile_errs)

    report = compose_diagnosis(
        attempt,
        invariant_lines=invariant_lines,
        synthesis_error=synthesis_error,
        consistency=consistency_report,
        coverage=coverage_report,
        probe_outcomes=probe_outcomes or None,
        probe_stimuli=probe_stimuli or None,
        compile_errors=tuple(compile_errs_with_hints) if compile_errs_with_hints else None,
        critic_failures=None,
    )
    return report.render()


def _build_plan_feedback(
    attempt: int,
    *,
    synthesis_error: Optional[SynthesisError],
    consistency_report: Optional[ConsistencyReport],
    coverage_report: Optional[CoverageReport],
    validation_errors: Sequence[str],
) -> str:
    """Compose deterministic feedback for the next planner attempt."""
    lines: List[str] = [
        "# Plan-stage feedback",
        "",
        "Regenerate only `api_contract` and `test_plan`. Do not output "
        "`driver_header` or `driver_source`.",
        "",
        f"Planner attempt {attempt} was not accepted.",
    ]

    if synthesis_error is not None:
        lines.extend((
            "",
            "## Provider/schema failure",
            f"- source: `{synthesis_error.source}`",
            f"- message: {str(synthesis_error)}",
        ))
        for err in synthesis_error.errors[:20]:
            lines.append(f"- schema: {err}")

    if validation_errors:
        lines.extend(("", "## Blocking plan validation errors"))
        for err in validation_errors[:40]:
            lines.append(f"- {err}")

    if consistency_report is not None:
        lines.extend(("", "## Consistency report"))
        lines.append(f"- {consistency_report.summary}")

    if coverage_report is not None:
        lines.extend(("", "## Coverage report"))
        lines.append(f"- {coverage_report.summary}")

    lines.extend((
        "",
        "The next response must keep the same split contract: only "
        "`api_contract` and `test_plan` at the top level.",
    ))
    return "\n".join(lines).strip() + "\n"


# Artifact dumping

def _safe_write(path: Path, content: str) -> None:
    """Write UTF-8 text to disk; best-effort (log-and-swallow)."""
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
    except OSError as e:  # pragma: no cover - defensive
        logger.warning("Could not write %s: %s", path, e)


def _safe_write_json(path: Path, obj: Any) -> None:
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(
            json.dumps(obj, indent=2, ensure_ascii=False, default=str),
            encoding="utf-8",
        )
    except OSError as e:  # pragma: no cover - defensive
        logger.warning("Could not write JSON %s: %s", path, e)


def _dump_plan_attempt_artifacts(
    run_dir: Optional[Path],
    record: PlanAttemptRecord,
) -> None:
    """Write planner artefacts to ``run_dir/plan_attempts/NNN/``."""
    if run_dir is None:
        return
    attempt_dir = run_dir / "plan_attempts" / f"{record.attempt:03d}"
    attempt_dir.mkdir(parents=True, exist_ok=True)

    plan = record.plan_bundle
    if plan is not None:
        _safe_write_json(attempt_dir / "plan_bundle.json", plan.to_dict())
        if plan.raw_response:
            _safe_write(attempt_dir / "raw_response.txt", plan.raw_response)
    elif record.synthesis_error is not None and record.synthesis_error.raw_response:
        _safe_write(
            attempt_dir / "raw_response.txt",
            record.synthesis_error.raw_response,
        )

    if record.consistency_report is not None:
        _safe_write_json(
            attempt_dir / "consistency.json",
            record.consistency_report.to_dict(),
        )
    if record.coverage_report is not None:
        _safe_write_json(
            attempt_dir / "coverage.json",
            record.coverage_report.to_dict(),
        )
    if record.validation_errors:
        _safe_write_json(
            attempt_dir / "plan_validation.json",
            {"errors": list(record.validation_errors)},
        )
    if record.feedback_for_next:
        _safe_write(attempt_dir / "plan_feedback.md", record.feedback_for_next)

    _safe_write_json(attempt_dir / "record.json", record.to_dict())


def _dump_frozen_plan(
    run_dir: Optional[Path],
    plan_bundle: PlanBundle,
) -> None:
    if run_dir is None:
        return
    _safe_write_json(run_dir / "frozen_plan.json", plan_bundle.to_dict())


def _dump_attempt_artifacts(
    run_dir: Optional[Path],
    record: AttemptRecord,
) -> None:
    """Write every per-attempt artefact to ``run_dir/attempts/NNN/``.

    No-op when ``run_dir`` is None. Failures are logged, not raised.
    """
    if run_dir is None:
        return
    attempt_dir = run_dir / "attempts" / f"{record.attempt:03d}"
    attempt_dir.mkdir(parents=True, exist_ok=True)

    bundle = record.bundle
    if bundle is not None:
        if bundle.driver_header:
            _safe_write(attempt_dir / f"{bundle.device_id}.h", bundle.driver_header)
        if bundle.driver_source:
            _safe_write(attempt_dir / f"{bundle.device_id}.c", bundle.driver_source)
        if bundle.raw_response:
            _safe_write(attempt_dir / "raw_response.txt", bundle.raw_response)
        _safe_write_json(attempt_dir / "bundle.json", bundle.to_dict())
    elif record.synthesis_error is not None and record.synthesis_error.raw_response:
        # Preserve the raw provider response for debugging.
        _safe_write(
            attempt_dir / "raw_response.txt",
            record.synthesis_error.raw_response,
        )

    if record.adapter is not None:
        _safe_write(
            attempt_dir / f"{record.adapter.device_id}_eval_adapter.c",
            record.adapter.source_c,
        )

    if record.consistency_report is not None:
        _safe_write_json(
            attempt_dir / "consistency.json",
            record.consistency_report.to_dict(),
        )
    if record.coverage_report is not None:
        _safe_write_json(
            attempt_dir / "coverage.json",
            record.coverage_report.to_dict(),
        )

    if record.syntax_result is not None:
        _safe_write(
            attempt_dir / "syntax_compile.log",
            record.syntax_result.raw_output or "",
        )
    if record.link_result is not None:
        _safe_write(
            attempt_dir / "link_compile.log",
            record.link_result.raw_output or "",
        )

    if record.probe_outcomes:
        _safe_write_json(
            attempt_dir / "probe_outcomes.json",
            [o.to_dict() for o in record.probe_outcomes],
        )

    if record.feedback_for_next:
        _safe_write(attempt_dir / "feedback.md", record.feedback_for_next)
    if record.repair_context_for_next:
        _safe_write_json(
            attempt_dir / "repair_context_for_next.json",
            dict(record.repair_context_for_next),
        )

    _safe_write_json(attempt_dir / "record.json", record.to_dict())


def _dump_final_result(
    run_dir: Optional[Path],
    result: RepairLoopResult,
    tracker: InvariantsTracker,
) -> None:
    """Write top-level summary + invariants snapshot."""
    if run_dir is None:
        return
    run_dir.mkdir(parents=True, exist_ok=True)
    _safe_write_json(run_dir / "loop_v2_result.json", result.to_dict())
    _safe_write_json(
        run_dir / "invariants.json",
        {"active": [inv.to_dict() for inv in tracker.active()]},
    )


def _run_plan_stage(
    *,
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction],
    artifact: Optional[Mapping[str, Any]],
    task_package: Optional[Mapping[str, Any]],
    channel_alias_map: Optional[Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
    max_attempts: int,
    run_dir: Optional[Path],
    prefer_json_mode: bool,
    extra_metadata: Optional[Mapping[str, Any]],
) -> Tuple[Optional[PlanBundle], List[PlanAttemptRecord]]:
    """Generate, validate, and freeze ``api_contract + test_plan``."""
    plan_attempts: List[PlanAttemptRecord] = []
    last_feedback: Optional[str] = None
    plan_budget = max(1, max_attempts)

    for attempt_num in range(1, plan_budget + 1):
        t_attempt = time.time()
        logger.info(
            "=== repair_loop_v2 plan attempt %d/%d (device=%s rtos=%s) ===",
            attempt_num,
            plan_budget,
            str(device_ir.get("device_id") or "unknown"),
            str(rtos_contract.get("rtos") or "unknown"),
        )

        plan, synth_err = _run_contract_test_plan(
            provider,
            device_ir,
            rtos_contract,
            classify_result=classify_result,
            routing=routing,
            expected_transactions=expected_transactions,
            artifact=artifact,
            channel_alias_map=channel_alias_map,
            output_semantics_map=output_semantics_map,
            prior_feedback=last_feedback,
            attempt=attempt_num,
            prefer_json_mode=prefer_json_mode,
            extra_metadata=extra_metadata,
        )

        cons_report: Optional[ConsistencyReport] = None
        cov_report: Optional[CoverageReport] = None
        validation_errors: Tuple[str, ...] = ()
        if plan is not None:
            plan = _normalise_scaled_public_output_types(plan, device_ir)
            plan = _normalise_output_semantics_units(plan, output_semantics_map)
            plan = _with_mechanical_plan_stimuli(
                plan,
                device_ir,
                output_semantics_map=output_semantics_map,
            )
            plan = _normalise_single_channel_raw_primary(
                plan,
                device_ir,
                output_semantics_map=output_semantics_map,
            )
            plan = _normalise_plan_preamble_headers(
                plan,
                rtos_contract,
                task_package=task_package,
            )
            cons_report = check_consistency(plan.test_plan, plan.eval_class)
            cov_report = check_bundle_coverage(
                plan.test_plan, expected_transactions,
            )
            validation_errors = _plan_static_validation_errors(
                plan,
                cons_report,
                cov_report,
                output_semantics_map,
                device_ir=device_ir,
            )

        # Let the final planner attempt carry validation warnings forward.
        final_plan_attempt = attempt_num >= plan_budget
        has_validation_errors = bool(validation_errors)
        success = (
            plan is not None
            and (not has_validation_errors or final_plan_attempt)
        )
        feedback = ""
        if not success:
            feedback = _build_plan_feedback(
                attempt_num,
                synthesis_error=synth_err,
                consistency_report=cons_report,
                coverage_report=cov_report,
                validation_errors=validation_errors,
            )
        elif plan is not None and has_validation_errors:
            logger.warning(
                "repair_loop_v2 freezing plan with %d validation warning(s) "
                "after exhausting %d planner attempt(s)",
                len(validation_errors),
                plan_budget,
            )

        record = PlanAttemptRecord(
            attempt=attempt_num,
            success=success,
            total_time_s=round(time.time() - t_attempt, 3),
            plan_bundle=plan,
            synthesis_error=synth_err,
            consistency_report=cons_report,
            coverage_report=cov_report,
            validation_errors=validation_errors,
            feedback_for_next=feedback,
        )
        plan_attempts.append(record)
        _dump_plan_attempt_artifacts(run_dir, record)

        if success and plan is not None:
            _dump_frozen_plan(run_dir, plan)
            logger.info(
                "repair_loop_v2 froze plan at plan_attempt=%d hash=%s",
                attempt_num, plan.plan_hash,
            )
            return plan, plan_attempts

        last_feedback = feedback

    logger.warning(
        "repair_loop_v2 could not freeze a valid plan after %d attempt(s)",
        len(plan_attempts),
    )
    return None, plan_attempts


# Main entry

def run_repair_loop(
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    *,
    classify_result: ClassifyResult,
    routing: RoutingResult,
    expected_transactions: Sequence[ExpectedTransaction] = (),
    artifact: Optional[Mapping[str, Any]] = None,
    task_package: Optional[Mapping[str, Any]] = None,
    api_contract_overrides: Optional[Mapping[str, Any]] = None,
    probe_meta_overrides: Optional[Mapping[str, Any]] = None,
    max_attempts: int = 3,
    run_dir: Optional[Path] = None,
    skip_runtime: bool = False,
    skip_syntax: bool = False,
    skip_probe: bool = False,
    compile_timeout: int = 120,
    probe_timeout: int = 60,
    probe_sleep_s: int = 20,
    stop_on_probe_error: bool = True,
    prefer_json_mode: bool = True,
    extra_metadata: Optional[Mapping[str, Any]] = None,
) -> RepairLoopResult:
    """Orchestrate a fail-fast driver repair loop."""
    t_start = time.time()
    device_id = str(device_ir.get("device_id") or "").strip() or "unknown"
    rtos_id = str(rtos_contract.get("rtos") or "").strip() or "unknown"
    eval_class = classify_result.eval_class
    bus_kind = routing.bus_kind or classify_result.bus_type

    if max_attempts < 1:
        raise ValueError(f"max_attempts must be >= 1; got {max_attempts!r}")

    if run_dir is not None:
        run_dir = Path(run_dir)
        run_dir.mkdir(parents=True, exist_ok=True)
        _safe_write_json(run_dir / "meta.json", {
            "device_id": device_id,
            "rtos_id": rtos_id,
            "eval_class": eval_class,
            "bus_kind": bus_kind,
            "classify": dataclasses.asdict(classify_result),
            "routing": dataclasses.asdict(routing),
            "expected_transactions": [
                dict(
                    phase=t.phase,
                    addr_or_pin=t.addr_or_pin,
                    write_prefix_any_of=[list(p) for p in t.write_prefix_any_of],
                    read_any=t.read_any,
                    forbid_write_prefix=getattr(t, "forbid_write_prefix", False),
                    note=t.note,
                )
                for t in expected_transactions
            ],
            "max_attempts": max_attempts,
            "split_plan_enabled": True,
            "plan_max_attempts": max(1, max_attempts),
            "skip_runtime": skip_runtime,
            "skip_probe": bool(skip_runtime or skip_probe),
            "skip_syntax": bool(skip_syntax),
        })

    channel_alias_map = build_or_load_channel_alias_map(
        provider,
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        task_package=task_package,
        run_dir=run_dir,
        prefer_json_mode=prefer_json_mode,
        extra_metadata=extra_metadata,
    )
    output_semantics_map = build_or_load_output_semantics_map(
        provider,
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        channel_alias_map=channel_alias_map,
        task_package=task_package,
        run_dir=run_dir,
        prefer_json_mode=prefer_json_mode,
        extra_metadata=extra_metadata,
    )

    tracker = InvariantsTracker()
    plan_bundle, plan_attempts = _run_plan_stage(
        provider=provider,
        device_ir=device_ir,
        rtos_contract=rtos_contract,
        classify_result=classify_result,
        routing=routing,
        expected_transactions=expected_transactions,
        artifact=artifact,
        task_package=task_package,
        channel_alias_map=channel_alias_map,
        output_semantics_map=output_semantics_map,
        max_attempts=max_attempts,
        run_dir=run_dir,
        prefer_json_mode=prefer_json_mode,
        extra_metadata=extra_metadata,
    )
    if plan_bundle is None:
        result = RepairLoopResult(
            success=False,
            final_attempt=0,
            total_time_s=round(time.time() - t_start, 3),
            device_id=device_id,
            rtos_id=rtos_id,
            eval_class=eval_class,
            bus_kind=bus_kind,
            plan_bundle=None,
            final_bundle=None,
            final_adapter=None,
            final_elf_path=None,
            plan_attempts=plan_attempts,
            attempts=[],
            invariants=tracker.active(),
            layer_failed=LAYER_SYNTHESIS,
        )
        _dump_final_result(run_dir, result, tracker)
        return result

    attempts: List[AttemptRecord] = []
    last_feedback: Optional[str] = None
    last_repair_context: Optional[Mapping[str, Any]] = None
    best_record: Optional[AttemptRecord] = None
    best_score = -1

    for attempt_num in range(1, max_attempts + 1):
        t_attempt = time.time()
        logger.info(
            "=== repair_loop_v2 attempt %d/%d (device=%s rtos=%s) ===",
            attempt_num, max_attempts, device_id, rtos_id,
        )
        record = _run_single_attempt(
            attempt_num=attempt_num,
            provider=provider,
            device_ir=device_ir,
            rtos_contract=rtos_contract,
            classify_result=classify_result,
            routing=routing,
            plan_bundle=plan_bundle,
            expected_transactions=expected_transactions,
            artifact=artifact,
            channel_alias_map=channel_alias_map,
            output_semantics_map=output_semantics_map,
            task_package=task_package,
            api_contract_overrides=api_contract_overrides,
            probe_meta_overrides=probe_meta_overrides,
            prior_feedback=last_feedback,
            repair_context=last_repair_context,
            tracker=tracker,
            run_dir=run_dir,
            skip_runtime=skip_runtime,
            skip_syntax=skip_syntax,
            skip_probe=bool(skip_runtime or skip_probe),
            compile_timeout=compile_timeout,
            probe_timeout=probe_timeout,
            probe_sleep_s=probe_sleep_s,
            stop_on_probe_error=stop_on_probe_error,
            prefer_json_mode=prefer_json_mode,
            extra_metadata=extra_metadata,
            t_attempt_start=t_attempt,
        )
        attempts.append(record)
        _dump_attempt_artifacts(run_dir, record)

        score = _score_attempt(record)
        if score > best_score:
            best_score = score
            best_record = record

        if record.success:
            logger.info(
                "repair_loop_v2 SUCCESS at attempt %d (device=%s)",
                attempt_num, device_id,
            )
            break

        # Feed this attempt's rendered feedback into the next one.
        last_feedback = record.feedback_for_next
        last_repair_context = record.repair_context_for_next
        logger.info(
            "repair_loop_v2 attempt %d failed at layer=%s; "
            "carrying %d invariant(s) into next round",
            attempt_num, record.layer_failed, len(tracker.active()),
        )

    # Finalise
    final_success = bool(attempts and attempts[-1].success)
    # "final" bundle: last attempt if success, else best.
    if final_success:
        final_rec = attempts[-1]
    elif best_record is not None:
        final_rec = best_record
    else:  # pragma: no cover - defensive; attempts always non-empty above
        final_rec = None

    result = RepairLoopResult(
        success=final_success,
        final_attempt=len(attempts),
        total_time_s=round(time.time() - t_start, 3),
        device_id=device_id,
        rtos_id=rtos_id,
        eval_class=eval_class,
        bus_kind=bus_kind,
        plan_bundle=plan_bundle,
        final_bundle=final_rec.bundle if final_rec else None,
        final_adapter=final_rec.adapter if final_rec else None,
        final_elf_path=(
            final_rec.link_result.elf_path
            if final_rec and final_rec.link_result
               and final_rec.link_result.success
            else None
        ),
        plan_attempts=plan_attempts,
        attempts=attempts,
        invariants=tracker.active(),
        layer_failed=attempts[-1].layer_failed if attempts else LAYER_SYNTHESIS,
    )
    _dump_final_result(run_dir, result, tracker)
    return result


def _run_single_attempt(
    *,
    attempt_num: int,
    provider: BaseProvider,
    device_ir: Mapping[str, Any],
    rtos_contract: Mapping[str, Any],
    classify_result: ClassifyResult,
    routing: RoutingResult,
    plan_bundle: PlanBundle,
    expected_transactions: Sequence[ExpectedTransaction],
    artifact: Optional[Mapping[str, Any]],
    channel_alias_map: Optional[Mapping[str, Any]],
    output_semantics_map: Optional[Mapping[str, Any]],
    task_package: Optional[Mapping[str, Any]],
    api_contract_overrides: Optional[Mapping[str, Any]],
    probe_meta_overrides: Optional[Mapping[str, Any]],
    prior_feedback: Optional[str],
    repair_context: Optional[Mapping[str, Any]],
    tracker: InvariantsTracker,
    run_dir: Optional[Path],
    skip_runtime: bool,
    skip_syntax: bool,
    skip_probe: bool,
    compile_timeout: int,
    probe_timeout: int,
    probe_sleep_s: int,
    stop_on_probe_error: bool,
    prefer_json_mode: bool,
    extra_metadata: Optional[Mapping[str, Any]],
    t_attempt_start: float,
) -> AttemptRecord:
    """Execute one pass and stop at the first failing layer."""

    def _finalise(
        *,
        layer_failed: str,
        success: bool,
        bundle: Optional[SynthesisBundle] = None,
        synthesis_error: Optional[SynthesisError] = None,
        syntax_result: Optional[StubCompileResult] = None,
        consistency_report: Optional[ConsistencyReport] = None,
        coverage_report: Optional[CoverageReport] = None,
        adapter: Optional[GeneratedAdapter] = None,
        adapter_error: Optional[str] = None,
        link_result: Optional[CompileResult] = None,
        probe_outcomes: Sequence[ProbeOutcome] = (),
        probe_stimuli: Sequence[ProbeStimulus] = (),
    ) -> AttemptRecord:
        # Ingest positive evidence before rendering feedback so invariants
        # tracker reflects everything this attempt learned.
        if consistency_report is not None:
            tracker.ingest_consistency(attempt_num, consistency_report)
        if coverage_report is not None:
            tracker.ingest_coverage(attempt_num, coverage_report)
        if probe_outcomes:
            tracker.ingest_probe(
                attempt_num, probe_outcomes, probe_stimuli or None,
            )
        # Preserve include lines that already compiled cleanly.
        if (
            bundle is not None
            and syntax_result is not None
            and syntax_result.success
        ):
            tracker.ingest_compile_clean_includes(
                attempt_num,
                header_text=bundle.driver_header,
                source_text=bundle.driver_source,
            )

        if not success:
            compile_errs: List[str] = list(_compile_errors_as_strings(syntax_result))
            compile_errs.extend(_compile_errors_as_strings(link_result))
            frozen_plan = {
                "api_contract": dict(plan_bundle.api_contract),
                "test_plan": dict(plan_bundle.test_plan),
                "plan_hash": plan_bundle.plan_hash,
            }
            repair_context_for_next = build_repair_context(
                layer_failed=layer_failed,
                device_ir=device_ir,
                rtos_contract=rtos_contract,
                expected_transactions=expected_transactions,
                frozen_plan=frozen_plan,
                synthesis_error=synthesis_error,
                compile_errors=tuple(compile_errs),
                consistency_report=consistency_report,
                coverage_report=coverage_report,
                adapter_error=adapter_error,
                probe_outcomes=probe_outcomes,
                probe_stimuli=probe_stimuli,
            )
            feedback = _build_attempt_feedback(
                attempt_num, tracker,
                synthesis_error=synthesis_error,
                bundle=bundle,
                syntax_result=syntax_result,
                consistency_report=consistency_report,
                coverage_report=coverage_report,
                adapter_error=adapter_error,
                link_result=link_result,
                probe_outcomes=probe_outcomes,
                probe_stimuli=probe_stimuli,
            )
        else:
            feedback = ""
            repair_context_for_next = None

        return AttemptRecord(
            attempt=attempt_num,
            layer_failed=layer_failed,
            success=success,
            total_time_s=round(time.time() - t_attempt_start, 3),
            synthesis_error=synthesis_error,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=consistency_report,
            coverage_report=coverage_report,
            adapter=adapter,
            adapter_error=adapter_error,
            link_result=link_result,
            probe_outcomes=tuple(probe_outcomes),
            probe_stimuli=tuple(probe_stimuli),
            feedback_for_next=feedback,
            repair_context_for_next=repair_context_for_next,
        )

    # Code generation against the frozen plan.
    code_bundle, synth_err = _run_driver_code(
        provider,
        device_ir,
        rtos_contract,
        classify_result=classify_result,
        routing=routing,
        plan_bundle=plan_bundle,
        expected_transactions=expected_transactions,
        artifact=artifact,
        channel_alias_map=channel_alias_map,
        output_semantics_map=output_semantics_map,
        prior_feedback=prior_feedback,
        repair_context=repair_context,
        attempt=attempt_num,
        prefer_json_mode=prefer_json_mode,
        extra_metadata=extra_metadata,
    )
    if code_bundle is None:
        return _finalise(
            layer_failed=LAYER_SYNTHESIS,
            success=False,
            synthesis_error=synth_err,
        )
    bundle = _assemble_synthesis_bundle(plan_bundle, code_bundle, routing)
    bundle = _normalise_helper_usage_redeclarations(bundle, rtos_contract)
    bundle = _normalise_header_opaque_struct_forwards(bundle)
    bundle = _normalise_adapter_preamble_symbol_headers(bundle)
    bundle = _normalise_driver_include_headers(
        bundle,
        rtos_contract,
        task_package=task_package,
    )

    # Pre-syntax surface checks (advisory).
    pre_syntax_errors = _driver_code_pre_syntax_validation_errors(
        bundle,
        rtos_contract,
    )
    if pre_syntax_errors:
        logger.warning(
            "Pre-syntax surface check: %d issue(s), advisory only",
            len(pre_syntax_errors),
        )

    # Syntax check.
    syntax_result: Optional[StubCompileResult] = None
    if not skip_syntax:
        try:
            syntax_result = _run_syntax_check(bundle, timeout=compile_timeout)
        except Exception as e:   # pragma: no cover - defensive
            logger.error(
                "Syntax compile raised unexpected %s: %s",
                type(e).__name__, e,
            )
            syntax_result = StubCompileResult(
                success=False,
                errors=[],
                warnings=[],
                raw_output=f"{type(e).__name__}: {e}",
                return_code=-999,
                compile_level="syntax",
            )
        if syntax_result is not None and not syntax_result.success:
            return _finalise(
                layer_failed=LAYER_SYNTAX,
                success=False,
                bundle=bundle,
                syntax_result=syntax_result,
            )

    # Static code invariants (advisory).
    # Static heuristics are reported as feedback but do not block probing.
    static_errors = _driver_code_static_validation_errors(
        bundle,
        rtos_contract,
        device_ir,
        task_package=task_package,
    )
    if static_errors:
        logger.warning(
            "Static code invariants: %d issue(s), advisory only",
            len(static_errors),
        )

    # Advisory self-check.
    cons_report, cov_report = _run_self_check(bundle, expected_transactions)

    # Stop here if runtime validation is disabled.
    if skip_runtime:
        return _finalise(
            layer_failed=LAYER_NONE,
            success=True,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
        )

    # Link compile.
    bundle = _normalise_api_contract_dev_struct_type(bundle)
    bundle = _normalise_bundle_preamble_headers(bundle)
    adapter, adapter_err = _generate_adapter(
        bundle, classify_result, api_contract_overrides,
    )
    if adapter is None:
        return _finalise(
            layer_failed=LAYER_LINK,
            success=False,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
            adapter_error=adapter_err,
        )

    stage_dir = _attempt_stage_dir(run_dir, attempt_num)
    try:
        link_result = _run_link_compile(
            bundle,
            adapter,
            classify_result,
            routing,
            stage_dir,
            timeout=compile_timeout,
        )
    except Exception as e:   # pragma: no cover - defensive
        logger.error(
            "Link compile raised unexpected %s: %s",
            type(e).__name__, e,
        )
        link_result = CompileResult(
            success=False,
            elf_path=None,
            errors=[f"{type(e).__name__}: {e}"],
            warnings=[],
            raw_output="",
        )

    if not link_result.success or link_result.elf_path is None:
        return _finalise(
            layer_failed=LAYER_LINK,
            success=False,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
            adapter=adapter,
            link_result=link_result,
        )

    # If probing is suppressed, linked output is sufficient.
    if skip_probe:
        return _finalise(
            layer_failed=LAYER_NONE,
            success=True,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
            adapter=adapter,
            link_result=link_result,
        )

    # Runtime probe.
    try:
        _meta, stimuli, outcomes = _run_probe(
            link_result.elf_path,
            bundle,
            device_ir,
            classify_result,
            routing,
            stage_dir / "probe",
            api_contract_overrides=api_contract_overrides,
            probe_meta_overrides=probe_meta_overrides,
            task_package=task_package,
            timeout_per_stim=probe_timeout,
            sleep_s=probe_sleep_s,
            stop_on_error=stop_on_probe_error,
        )
    except ProbeError as e:
        logger.warning(
            "Runtime probe raised ProbeError: %s", e,
        )
        return _finalise(
            layer_failed=LAYER_PROBE,
            success=False,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
            adapter=adapter,
            adapter_error=None,
            link_result=link_result,
            probe_outcomes=(),
            probe_stimuli=(),
        )
    except Exception as e:   # pragma: no cover - defensive
        logger.error(
            "Runtime probe raised unexpected %s: %s",
            type(e).__name__, e,
        )
        return _finalise(
            layer_failed=LAYER_PROBE,
            success=False,
            bundle=bundle,
            syntax_result=syntax_result,
            consistency_report=cons_report,
            coverage_report=cov_report,
            adapter=adapter,
            link_result=link_result,
            probe_outcomes=(),
            probe_stimuli=(),
        )

    all_probe_pass = _probe_all_passed(outcomes, stimuli)
    return _finalise(
        layer_failed=LAYER_NONE if all_probe_pass else LAYER_PROBE,
        success=all_probe_pass,
        bundle=bundle,
        syntax_result=syntax_result,
        consistency_report=cons_report,
        coverage_report=cov_report,
        adapter=adapter,
        link_result=link_result,
        probe_outcomes=outcomes,
        probe_stimuli=stimuli,
    )


def _attempt_stage_dir(run_dir: Optional[Path], attempt_num: int) -> Path:
    """Per-attempt scratch directory for compile + probe outputs."""
    if run_dir is not None:
        stage = run_dir / "attempts" / f"{attempt_num:03d}" / "stage"
        stage.mkdir(parents=True, exist_ok=True)
        return stage
    # Use a real temp path when no run directory was provided.
    import tempfile
    base = Path(tempfile.mkdtemp(prefix=f"drivergen_v2_loop_a{attempt_num:03d}_"))
    return base


__all__ = [
    "LAYER_SYNTHESIS",
    "LAYER_SYNTAX",
    "LAYER_SELF_CHECK",
    "LAYER_LINK",
    "LAYER_PROBE",
    "LAYER_NONE",
    "ALL_LAYERS",
    "AttemptRecord",
    "PlanAttemptRecord",
    "RepairLoopResult",
    "run_repair_loop",
]
