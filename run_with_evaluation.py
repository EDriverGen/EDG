"""Top-level entry point: run drivergen pipeline and evaluate the output."""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

from drivergen.pipeline.orchestrator import run_task_package  # noqa: E402
from evaluation.pipeline_hook import make_pipeline_hook  # noqa: E402


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument(
        "--task-package",
        "--combo",
        dest="task_package",
        required=True,
        help="Task package id or task_package.json path.",
    )
    ap.add_argument("--provider", default="deepseek")
    ap.add_argument("--model", default="deepseek-v4-flash")
    ap.add_argument("--output-dir", type=Path, default=None)
    ap.add_argument("--codegen", action="store_true", default=False)
    ap.add_argument("--max-repairs", type=int, default=2)
    ap.add_argument("--artifact", type=Path, default=None)
    ap.add_argument("--skip-compile", action="store_true", default=False)
    ap.add_argument("--run-renode", action="store_true", default=False)
    ap.add_argument("--skip-evaluation", action="store_true", default=False,
                    help="Run codegen but skip the L1..L6 evaluation hook")
    ap.add_argument("--eval-timeout", type=int, default=30,
                    help="Renode timeout per vector (seconds)")
    args = ap.parse_args()

    hook = None if args.skip_evaluation else make_pipeline_hook(
        timeout_per_vector=args.eval_timeout,
    )

    report = run_task_package(
        args.task_package,
        provider=args.provider,
        model=args.model,
        output_root=args.output_dir,
        skip_codegen=not args.codegen,
        max_repairs=args.max_repairs,
        artifact_path=args.artifact,
        skip_compile=args.skip_compile,
        run_renode=args.run_renode,
        evaluation_hook=hook,
    )

    print(json.dumps(report, indent=2))
    return 0 if report.get("all_checks_passed") else 2


if __name__ == "__main__":
    sys.exit(main())
