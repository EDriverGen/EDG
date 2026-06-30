"""evaluation.batch_eval - parallel L1-L5 evaluation for an e2e batch."""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
from collections import Counter
from concurrent.futures import ProcessPoolExecutor, as_completed
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable, Sequence

# ── Repo / worker constants ──────────────────────────────────────────────

REPO_ROOT = Path(__file__).resolve().parent.parent

# ``python -m evaluation._worker_one`` is the canonical per-combo CLI.
# Kept as a constant so tests can swap in a stub worker if they ever
# need to.
_WORKER_MODULE = "evaluation._worker_one"

# Catalog of devices / RTOS the project currently knows about.  Used
# *only* to split ``"<device>_<rtos>"`` job names — it does not gate
# what jobs you can run.  Add new entries here when registering a new
# device or RTOS so the longest-prefix split keeps being correct.
DEVICES: tuple[str, ...] = (
    # I2C
    "bh1750", "lm75a", "vl53l0x", "tmp105", "bme280", "dps310",
    "sht30", "tmp421", "mpu6050", "lsm303dlhc", "emc1413", "at24c256",
    "ssd1306", "ds3231",
    # SPI
    "adxl345", "max31855", "mcp3008",
    # GPIO
    "dht22", "ds18b20", "hcsr04",
    # UART
    "mhz19b",
)

RTOS_LIST: tuple[str, ...] = (
    "rtthread", "freertos", "threadx", "tobudos",
    "chibios", "zephyr", "riot", "nuttx", "xiuos",
    "openharmony", "openharmony-liteosm",
)


# ── Job discovery ────────────────────────────────────────────────────────


def parse_job_name(job_name: str) -> tuple[str, str] | None:
    """Split ``"<device>_<rtos>"`` into ``(device, rtos)``."""
    for dev in sorted(DEVICES, key=len, reverse=True):
        if job_name.startswith(dev + "_"):
            rtos = job_name[len(dev) + 1 :]
            if rtos in RTOS_LIST:
                return dev, rtos
            return None
    return None


def discover_jobs(batch_src: Path) -> list[tuple[str, str, str]]:
    """Walk ``batch_src`` and return ``[(job_name, device, rtos), ...]``."""
    out: list[tuple[str, str, str]] = []
    if not batch_src.is_dir():
        return out
    for child in sorted(batch_src.iterdir()):
        if not child.is_dir():
            continue
        parsed = parse_job_name(child.name)
        if parsed is None:
            continue
        dev, rtos = parsed
        if not (child / f"{dev}.c").exists():
            continue
        out.append((child.name, dev, rtos))
    return out


# ── Per-job worker (must be top-level for pickle / spawn) ────────────────


def _stage_driver_dir(src_dir: Path, dst_dir: Path) -> None:
    """_stage_driver_dir helper."""
    dst_dir.mkdir(parents=True, exist_ok=True)
    for pattern in ("*.c", "*.h"):
        for f in src_dir.glob(pattern):
            shutil.copy2(f, dst_dir / f.name)
    artefact = src_dir / "rtos_artifact.json"
    if artefact.exists():
        shutil.copy2(artefact, dst_dir / artefact.name)


def _parse_evaluation_report(run_dir: Path) -> dict:
    """Read ``evaluation_report.json`` and pull the bits the TSV needs."""
    report_path = run_dir / "evaluation_report.json"
    if not report_path.exists():
        return {"_present": False}
    try:
        rep = json.loads(report_path.read_text(encoding="utf-8"))
    except Exception as exc:
        return {"_present": True, "_error": f"parse:{type(exc).__name__}"}
    levels: dict[str, str] = {}
    for v in rep.get("verdicts") or []:
        if not isinstance(v, dict):
            continue
        lvl = v.get("level")
        if not lvl:
            continue
        levels[lvl] = "OK" if v.get("passed") else "FAIL"
    return {
        "_present": True,
        "overall_claim": rep.get("overall_claim", "-"),
        "levels": levels,
        "verdict_details": [
            {"level": v.get("level"), "passed": v.get("passed"),
             "detail": (v.get("detail") or "")[:200]}
            for v in (rep.get("verdicts") or [])
            if isinstance(v, dict)
        ],
    }


def run_one_job(args: tuple) -> dict:
    """Worker entry point."""
    (
        job_name,
        device_id,
        rtos_id,
        src_dir_str,
        run_dir_str,
        timeout_s,
        timeout_per_vector,
    ) = args
    src_dir = Path(src_dir_str)
    run_dir = Path(run_dir_str)

    t0 = time.time()
    try:
        _stage_driver_dir(src_dir, run_dir)
    except Exception as exc:
        return {
            "job": job_name,
            "ok": False,
            "error": f"stage:{type(exc).__name__}:{exc}",
            "overall_claim": "-",
            "levels": {},
            "elapsed_s": int(time.time() - t0),
        }

    cmd = [
        sys.executable,
        "-m", _WORKER_MODULE,
        str(run_dir),
        device_id,
        rtos_id,
    ]
    log_path = run_dir / "_eval.log"
    rc = -1
    err_str = "-"
    try:
        with log_path.open("wb") as logf:
            r = subprocess.run(
                cmd,
                stdout=logf,
                stderr=subprocess.STDOUT,
                cwd=str(REPO_ROOT),
                timeout=timeout_s,
                env={
                    **os.environ,
                    "PYTHONPATH": str(REPO_ROOT) + os.pathsep
                                  + os.environ.get("PYTHONPATH", ""),
                    "DRIVERGEN_EVAL_TIMEOUT_PER_VECTOR": str(timeout_per_vector),
                },
            )
            rc = r.returncode
    except subprocess.TimeoutExpired:
        err_str = f"timeout:{timeout_s}s"
    except Exception as exc:  # pragma: no cover - defensive
        err_str = f"subprocess:{type(exc).__name__}:{exc}"

    parsed = _parse_evaluation_report(run_dir)
    elapsed = int(time.time() - t0)

    if not parsed.get("_present"):
        # Tail the worker log to make the failure easier to triage.
        log_tail = ""
        try:
            log_tail = log_path.read_text(encoding="utf-8", errors="replace")[-400:]
        except Exception:
            pass
        return {
            "job": job_name,
            "ok": False,
            "error": err_str if err_str != "-" else f"no-report rc={rc}",
            "overall_claim": "-",
            "levels": {},
            "elapsed_s": elapsed,
            "log_tail": log_tail,
        }
    if "_error" in parsed:
        return {
            "job": job_name,
            "ok": False,
            "error": parsed["_error"],
            "overall_claim": "-",
            "levels": {},
            "elapsed_s": elapsed,
        }

    return {
        "job": job_name,
        "ok": True,
        "error": err_str,
        "overall_claim": parsed["overall_claim"],
        "levels": parsed["levels"],
        "verdict_details": parsed["verdict_details"],
        "elapsed_s": elapsed,
    }


# ── Summary writers ──────────────────────────────────────────────────────


_TSV_HEADER: tuple[str, ...] = (
    "job", "ok", "overall_claim", "L1", "L2", "L3", "L4", "L5",
    "error", "elapsed_s",
)


def write_tsv(rows: Sequence[dict], path: Path) -> None:
    """Persist rows in the current ``_summary.tsv`` format."""
    with path.open("w", encoding="utf-8") as f:
        f.write("\t".join(_TSV_HEADER) + "\n")
        for r in rows:
            line = "\t".join(
                [
                    r["job"],
                    "true" if r["ok"] else "false",
                    str(r.get("overall_claim", "-")),
                    r["levels"].get("L1", "-"),
                    r["levels"].get("L2", "-"),
                    r["levels"].get("L3", "-"),
                    r["levels"].get("L4", "-"),
                    r["levels"].get("L5", "-"),
                    str(r.get("error", "-")),
                    str(r.get("elapsed_s", 0)),
                ]
            )
            f.write(line + "\n")


def write_json_report(rows: Sequence[dict], path: Path) -> None:
    """Write a machine-readable copy of the run, embedding metadata."""
    payload = {
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "results": list(rows),
    }
    path.write_text(json.dumps(payload, indent=2, default=str), encoding="utf-8")


def summarise_rows(rows: Sequence[dict]) -> dict:
    """Compute summary counters used by both TSV and stdout printing."""
    total = len(rows)

    def cnt(level: str) -> int:
        return sum(1 for r in rows if r["levels"].get(level) == "OK")

    robust = sum(1 for r in rows if r.get("overall_claim") == "robust-valid")
    semantic = sum(
        1 for r in rows
        if r.get("overall_claim") in ("robust-valid", "semantic-valid")
    )

    rtos_breakdown: dict[str, dict] = {}
    for r in rows:
        parsed = parse_job_name(r["job"])
        if not parsed:
            continue
        rtos = parsed[1]
        b = rtos_breakdown.setdefault(
            rtos, {"jobs": 0, "L1": 0, "L2": 0, "L4": 0, "robust": 0}
        )
        b["jobs"] += 1
        for lvl in ("L1", "L2", "L4"):
            if r["levels"].get(lvl) == "OK":
                b[lvl] += 1
        if r.get("overall_claim") == "robust-valid":
            b["robust"] += 1

    return {
        "total": total,
        "L1": cnt("L1"),
        "L2": cnt("L2"),
        "L3": cnt("L3"),
        "L4": cnt("L4"),
        "L5": cnt("L5"),
        "robust": robust,
        "semantic_or_better": semantic,
        "per_rtos": rtos_breakdown,
    }


def print_summary(rows: Sequence[dict], total_elapsed: int) -> None:
    """Pretty-print the current stdout summary block."""
    s = summarise_rows(rows)
    total = s["total"]
    if total == 0:
        print("(no jobs)")
        return

    print()
    print(f"=== Eval batch summary ({total} jobs in {total_elapsed}s) ===")
    print(f"  L1 build      : {s['L1']}/{total}")
    print(f"  L2 boot       : {s['L2']}/{total}")
    print(f"  L3 protocol   : {s['L3']}/{total}")
    print(f"  L4 semantic   : {s['L4']}/{total}")
    print(f"  L5 robust     : {s['L5']}/{total}")
    print(f"  >= semantic   : {s['semantic_or_better']}/{total}")
    print(f"  == robust     : {s['robust']}/{total}")

    if s["per_rtos"]:
        print()
        print("Per-RTOS breakdown:")
        for rtos in sorted(s["per_rtos"]):
            b = s["per_rtos"][rtos]
            print(
                f"  {rtos:10s} {b['jobs']} jobs : "
                f"L1={b['L1']} L2={b['L2']} L4={b['L4']} robust={b['robust']}"
            )

    fails = [r for r in rows if r.get("overall_claim") != "robust-valid"]
    if fails:
        print()
        print(f"Non-robust jobs ({len(fails)}):")
        for r in fails:
            l_compact = "/".join(
                f"{lvl}={r['levels'].get(lvl, '-')}"
                for lvl in ("L1", "L2", "L3", "L4", "L5")
            )
            print(
                f"  {r['job']:24s} claim={r.get('overall_claim', '-'):15s} "
                f"{l_compact} err={r.get('error', '-')}"
            )

    error_kinds = Counter(
        r.get("error", "-") for r in rows if r.get("error", "-") not in ("-",)
    )
    if error_kinds:
        print()
        print("Error breakdown:")
        for k, v in error_kinds.most_common():
            print(f"  {v:3d} x {k}")


# ── Driver ───────────────────────────────────────────────────────────────


def _build_work_args(
    discovered: Iterable[tuple[str, str, str]],
    batch_src: Path,
    eval_out: Path,
    timeout_per_job: int,
    timeout_per_vector: int,
) -> list[tuple]:
    """Materialise the per-job tuples consumed by :func:`run_one_job`."""
    work_args: list[tuple] = []
    for job_name, dev, rtos in discovered:
        src_dir = batch_src / job_name
        run_dir = eval_out / job_name
        work_args.append(
            (
                job_name,
                dev,
                rtos,
                str(src_dir),
                str(run_dir),
                timeout_per_job,
                timeout_per_vector,
            )
        )
    return work_args


def run_batch(
    *,
    batch_src: Path,
    eval_out: Path | None = None,
    workers: int = 4,
    jobs_filter: Iterable[str] | None = None,
    timeout_per_job: int = 600,
    timeout_per_vector: int = 30,
    print_progress: bool = True,
) -> tuple[list[dict], Path]:
    """Run the parallel evaluation and return ``(rows, eval_out)``."""
    batch_src = Path(batch_src).expanduser().resolve()
    if not batch_src.is_dir():
        raise FileNotFoundError(f"--batch-src not a directory: {batch_src}")

    if eval_out is None:
        ts = datetime.now(timezone.utc).strftime("%y%m%dT%H%M%SZ")
        eval_out = REPO_ROOT / "runs" / f"eval_{ts}"
    eval_out = Path(eval_out).expanduser().resolve()
    eval_out.mkdir(parents=True, exist_ok=True)

    discovered = discover_jobs(batch_src)
    if jobs_filter is not None:
        wanted = {j.strip() for j in jobs_filter if j and j.strip()}
        discovered = [t for t in discovered if t[0] in wanted]
        missing = wanted - {t[0] for t in discovered}
        if missing and print_progress:
            print(f"WARNING: --jobs filter dropped unknown combos: {sorted(missing)}")

    if not discovered:
        if print_progress:
            print(f"ERROR: no eligible jobs under {batch_src}", file=sys.stderr)
        return [], eval_out

    if print_progress:
        print(f"Batch src    : {batch_src}")
        print(f"Eval out     : {eval_out}")
        print(f"Workers      : {workers}")
        print(f"Per-job tout : {timeout_per_job}s")
        print(f"Discovered   : {len(discovered)} jobs")
        print()

    work_args = _build_work_args(
        discovered, batch_src, eval_out, timeout_per_job, timeout_per_vector
    )

    t_total = time.time()
    rows: list[dict] = []
    with ProcessPoolExecutor(max_workers=workers) as pool:
        futures = {pool.submit(run_one_job, w): w[0] for w in work_args}
        done = 0
        for fut in as_completed(futures):
            done += 1
            r = fut.result()
            rows.append(r)
            if print_progress:
                l_compact = "/".join(
                    f"{lvl}={r['levels'].get(lvl, '-')}"
                    for lvl in ("L1", "L2", "L3", "L4", "L5")
                )
                print(
                    f"  [{done:3d}/{len(work_args)}] {r['job']:24s} "
                    f"claim={r.get('overall_claim', '-'):15s} {l_compact} "
                    f"({r['elapsed_s']}s) err={r.get('error', '-')}"
                )

    rows.sort(key=lambda r: r["job"])
    total_elapsed = int(time.time() - t_total)

    write_tsv(rows, eval_out / "_summary.tsv")
    write_json_report(rows, eval_out / "_summary.json")

    if print_progress:
        print_summary(rows, total_elapsed)
        print()
        print(f"EVAL_DIR={eval_out}")
        print(f"TSV={eval_out / '_summary.tsv'}")
        print(f"JSON={eval_out / '_summary.json'}")

    return rows, eval_out


def _build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="evaluation.batch_eval",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "--batch-src",
        required=True,
        help="e2e output batch dir (e.g. runs/e2e_compile_260426T162058Z)",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=min(os.cpu_count() or 4, 6),
        help="parallel workers (default: min(cpu_count, 6))",
    )
    parser.add_argument(
        "--eval-out",
        default=None,
        help="output dir, default runs/eval_<ts>",
    )
    parser.add_argument(
        "--jobs",
        default=None,
        help="comma-separated job names to filter "
             "(e.g. lm75a_threadx,bh1750_threadx)",
    )
    parser.add_argument(
        "--timeout-per-job",
        type=int,
        default=600,
        help="per-combo wall-clock timeout in seconds (default: 600)",
    )
    parser.add_argument(
        "--timeout-per-vector",
        type=int,
        default=30,
        help="per-vector timeout passed through to evaluate_run_dir "
             "(default: 30)",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = _build_parser()
    args = parser.parse_args(argv)

    jobs_filter = None
    if args.jobs:
        jobs_filter = [j.strip() for j in args.jobs.split(",") if j.strip()]

    try:
        rows, _ = run_batch(
            batch_src=Path(args.batch_src),
            eval_out=Path(args.eval_out) if args.eval_out else None,
            workers=args.workers,
            jobs_filter=jobs_filter,
            timeout_per_job=args.timeout_per_job,
            timeout_per_vector=args.timeout_per_vector,
        )
    except FileNotFoundError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    if not rows:
        return 3
    return 0


if __name__ == "__main__":
    sys.exit(main())
