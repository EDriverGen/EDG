#!/usr/bin/env python3
"""Single-combo evaluation entry point used by ``evaluation.batch_eval`` workers."""
from __future__ import annotations

import json
import sys
from pathlib import Path


def main(argv: list[str] | None = None) -> int:
    args = list(sys.argv[1:] if argv is None else argv)
    if len(args) < 3:
        print(__doc__, file=sys.stderr)
        return 2

    combo_dir = Path(args[0]).resolve()
    device_id = args[1]
    rtos_id = args[2]

    repo_root = Path(__file__).resolve().parent.parent
    if str(repo_root) not in sys.path:
        sys.path.insert(0, str(repo_root))

    from evaluation.pipeline_hook import evaluate_run_dir  # noqa: E402

    result = evaluate_run_dir(combo_dir, device_id, rtos_id)
    print(json.dumps(result, indent=2, default=str))
    return 0 if result.get("ok") else 1


if __name__ == "__main__":
    sys.exit(main())
