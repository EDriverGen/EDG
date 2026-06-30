"""evaluation.cli - `python -m evaluation.cli` entry for end-to-end evaluation."""
from __future__ import annotations

import argparse
import logging
import sys
from pathlib import Path

from evaluation.orchestrator import (
    OrchestratorOptions,
    evaluate,
    summarise,
)


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="evaluation.cli", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command", required=True)

    ev = sub.add_parser("evaluate", help="Run the L1-L5 ladder on one driver bundle.")
    ev.add_argument("--device", required=True, help="device id (e.g. bh1750)")
    ev.add_argument("--rtos",   required=True, help="rtos id (e.g. rtthread)")
    ev.add_argument("--driver-dir", required=True, type=Path,
                    help="directory containing <device>.c/<device>.h sources "
                         "(and ideally <device>_eval_adapter.c)")
    ev.add_argument("--adapter", type=Path,
                    help="adapter .c path (default: <driver-dir>/<device>_eval_adapter.c)")
    ev.add_argument("--bus-kind", default="i2c",
                    choices=("i2c", "spi", "uart", "gpio"))
    ev.add_argument("--eval-class", default=None,
                    help="override oracle.meta.eval_class")
    ev.add_argument("--bus-instance", default=None,
                    help='e.g. "i2c1"; default depends on --bus-kind')
    ev.add_argument("--l3-policy", default="auto",
                    choices=("auto", "strict", "semantic", "relaxed"))
    ev.add_argument("--skip-l3", action="store_true")
    ev.add_argument("--skip-l4", action="store_true")
    ev.add_argument("--skip-l5", action="store_true")
    ev.add_argument("--compile-timeout", type=int, default=180,
                    help="seconds allowed for compile step")
    ev.add_argument("--timeout-per-vector", type=int, default=60)
    ev.add_argument("--sleep", type=int, default=20,
                    help="Renode simulation sleep per vector (seconds)")
    ev.add_argument("--l5-timeout", type=int, default=60)
    ev.add_argument("--work-dir", type=Path, default=None,
                    help="staging directory (default: tempfile)")
    ev.add_argument("--out", type=Path, default=None,
                    help="JSON report path (default: stdout only)")
    ev.add_argument("--oracle-root", type=Path, default=None)
    ev.add_argument("--task-package", type=Path, default=None,
                    help="optional fixed task package used to align GPIO runtime pins")
    ev.add_argument("-v", "--verbose", action="count", default=0)
    return parser


def _configure_logging(verbose: int) -> None:
    level = logging.WARNING
    if verbose == 1:
        level = logging.INFO
    elif verbose >= 2:
        level = logging.DEBUG
    logging.basicConfig(
        format="%(asctime)s %(name)s %(levelname)s: %(message)s",
        datefmt="%H:%M:%S", level=level,
    )


def _print_summary(summary: dict) -> None:
    device = summary.get("device")
    combo = summary.get("combo")
    print(f"device:  {device}")
    print(f"combo:   {combo}")
    print(f"claim:   {summary.get('overall_claim')}")
    print()
    print(f"  {'Level':<4}  {'Status':<8}  Claim                        Detail")
    for row in summary["levels"]:
        st = "PASS" if row["passed"] else ("SKIP"
             if row["detail"].startswith("skipped") else "FAIL")
        detail = row["detail"]
        if len(detail) > 80:
            detail = detail[:77] + "..."
        print(f"  {row['level']:<4}  {st:<8}  {row['claim']:<28}  {detail}")


def main(argv=None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)
    _configure_logging(args.verbose)

    if args.command != "evaluate":
        parser.print_help(); return 2

    opts = OrchestratorOptions(
        device_id        = args.device,
        rtos_id          = args.rtos,
        driver_dir       = args.driver_dir,
        adapter_path     = args.adapter,
        bus_kind         = args.bus_kind,
        eval_class       = args.eval_class,
        bus_instance     = args.bus_instance,
        compile_timeout  = args.compile_timeout,
        timeout_per_vector = args.timeout_per_vector,
        sleep_s          = args.sleep,
        l5_timeout_per_scenario = args.l5_timeout,
        l3_policy        = args.l3_policy,
        work_dir         = args.work_dir,
        report_out       = args.out,
        skip_l3          = args.skip_l3,
        skip_l4          = args.skip_l4,
        skip_l5          = args.skip_l5,
        oracle_root      = args.oracle_root,
        task_package_path = args.task_package,
    )

    try:
        run = evaluate(opts)
    except FileNotFoundError as e:
        print(f"error: {e}", file=sys.stderr)
        return 2

    summary = summarise(run)
    _print_summary(summary)

    # Exit 0 iff every non-skipped verdict passed.
    nonskipped_failed = any(
        not v.passed and not (v.detail or "").startswith("skipped")
        for v in run.report.verdicts
    )
    return 1 if nonskipped_failed else 0


if __name__ == "__main__":
    sys.exit(main())
