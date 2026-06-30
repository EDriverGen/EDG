"""Command-line entry for ad-hoc scope-triage runs / cache warm-up."""

from __future__ import annotations

import argparse
import json
import logging
import sys
from pathlib import Path
from typing import Any

from ..llm_infra import make_budget_tracker
from ...llm.providers import create_provider
from .synthesizer import (
    cache_path_for,
    load_or_synthesize_scope_map,
)
from .triage import (
    ROLES,
    derive_scope_fragment,
    role_for_bus,
    run_triage_for_role,
)

logger = logging.getLogger(__name__)


def _build_role(role_name: str):
    if role_name == "kernel":
        return ROLES["kernel"]
    if role_name.startswith("driver_framework_"):
        return role_for_bus(role_name[len("driver_framework_") :])
    raise ValueError(f"Unknown triage role: {role_name}")


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--rtos", required=True, help="rtos id (for logging only)")
    ap.add_argument("--rtos-root", required=True, type=Path)
    ap.add_argument(
        "--roles",
        nargs="+",
        default=["kernel", "driver_framework_i2c"],
        help=(
            "Triage role list. Use 'kernel' for the kernel pass and "
            "'driver_framework_<bus>' for a per-bus pass (e.g. "
            "driver_framework_spi, driver_framework_i2c, ...)."
        ),
    )
    ap.add_argument(
        "--mcu-family",
        default=None,
        help="Target MCU family for MCU-aware triage.",
    )
    ap.add_argument("--provider", choices=["openai", "aliyun", "deepseek"], default="deepseek")
    ap.add_argument("--model", default="deepseek-v4-flash")
    ap.add_argument("--max-rounds", type=int, default=5)
    ap.add_argument("--sample-size", type=int, default=5)
    ap.add_argument("--output", required=True, type=Path)
    ap.add_argument(
        "--write-cache",
        action="store_true",
        help=(
            "After standalone triage, also persist a synthesised "
            "ScopeMap into drivergen/rtos/config/scope_map/_llm_cache/. "
            "This requires a manifest-style invocation; for ad-hoc use "
            "from --rtos-root only the per-role artefacts are written."
        ),
    )
    args = ap.parse_args(argv)

    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(name)s [%(levelname)s] %(message)s",
    )
    args.output.mkdir(parents=True, exist_ok=True)

    if not args.rtos_root.is_dir():
        print(f"ERROR: --rtos-root {args.rtos_root} is not a directory", file=sys.stderr)
        return 2

    provider = create_provider(args.provider, model=args.model)
    budget = make_budget_tracker(mode="exhaustive")

    summary: dict[str, Any] = {
        "rtos_id": args.rtos,
        "rtos_root": str(args.rtos_root),
        "model": args.model,
        "max_rounds": args.max_rounds,
        "sample_size": args.sample_size,
        "mcu_family": args.mcu_family,
        "roles": {},
    }

    mcu_suffix = f"__{args.mcu_family}" if args.mcu_family else ""

    for role_name in args.roles:
        try:
            role = _build_role(role_name)
        except ValueError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 2

        logger.info(
            "=== begin role=%s mcu=%s ===", role_name, args.mcu_family or "(none)"
        )
        state = run_triage_for_role(
            rtos_root=args.rtos_root,
            rtos_id=args.rtos,
            role=role,
            provider=provider,
            budget=budget,
            max_rounds=args.max_rounds,
            sample_size=args.sample_size,
            mcu_family=args.mcu_family,
        )

        audit_path = args.output / f"{args.rtos}__{role_name}{mcu_suffix}.audit.json"
        audit_path.write_text(
            json.dumps(
                {
                    "rtos_id": state.rtos_id,
                    "role": state.role.name,
                    "rounds": state.audit,
                    "kept_terminal": sorted(state.kept_terminal),
                    "kept_self_files": sorted(state.kept_self_files - state.kept_terminal),
                    "dropped": sorted(state.dropped),
                },
                indent=2,
                ensure_ascii=False,
            ),
            encoding="utf-8",
        )

        scope_fragment = derive_scope_fragment(state)
        if args.mcu_family:
            scope_fragment["mcu_family"] = args.mcu_family
        scope_path = args.output / f"{args.rtos}__{role_name}{mcu_suffix}.scope.json"
        scope_path.write_text(
            json.dumps(scope_fragment, indent=2, ensure_ascii=False),
            encoding="utf-8",
        )

        summary["roles"][role_name] = {
            "audit_path": str(audit_path),
            "scope_path": str(scope_path),
            "kept_terminal_count": len(state.kept_terminal),
            "dropped_count": len(state.dropped),
        }

        logger.info("=== end role=%s wrote %s + %s", role_name, audit_path, scope_path)

    summary_path = args.output / f"{args.rtos}{mcu_suffix}.summary.json"
    summary_path.write_text(
        json.dumps(summary, indent=2, ensure_ascii=False),
        encoding="utf-8",
    )
    print(f"Done. Summary at {summary_path}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
