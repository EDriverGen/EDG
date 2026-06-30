from __future__ import annotations

import argparse
import json
from pathlib import Path

from ..context.fixed import TASK_PACKAGES_ROOT
from ..llm.providers import ProviderError, create_provider
from ..rtos import list_registered_rtos
from .orchestrator import extract_device_ir_structured, run_task_package


DEFAULT_PROVIDER = "deepseek"
DEFAULT_MODEL = "deepseek-v4-flash"
PROVIDER_CHOICES = ["openai", "aliyun", "deepseek"]


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="DriverGen formal pipeline")
    subparsers = parser.add_subparsers(dest="command", required=True)

    list_rtos_parser = subparsers.add_parser("list-rtos", help="List registered RTOS profiles")
    list_rtos_parser.set_defaults(handler=handle_list_rtos)

    list_packages_parser = subparsers.add_parser("list-task-packages", help="List fixed task package ids")
    list_packages_parser.set_defaults(handler=handle_list_task_packages)

    run_package_parser = subparsers.add_parser(
        "run",
        aliases=["run-package"],
        help="Run the pipeline from a fixed task package",
    )
    run_package_parser.add_argument(
        "--task-package",
        "--combo",
        dest="task_package",
        required=True,
        help="Task package id or path from list-task-packages",
    )
    run_package_parser.add_argument("--provider", choices=PROVIDER_CHOICES, default=DEFAULT_PROVIDER)
    run_package_parser.add_argument("--model", default=DEFAULT_MODEL)
    run_package_parser.add_argument("--output-dir", type=Path, default=None, help="Optional fixed output directory")
    run_package_parser.add_argument("--codegen", action="store_true", default=False)
    run_package_parser.add_argument("--max-repairs", type=int, default=2)
    run_package_parser.add_argument("--artifact", type=Path, default=None)
    run_package_parser.add_argument("--skip-compile", action="store_true", default=False)
    run_package_parser.add_argument("--run-renode", dest="run_renode", action="store_true", default=True)
    run_package_parser.add_argument("--no-renode", dest="run_renode", action="store_false")
    run_package_parser.add_argument("--no-llm-cache", action="store_true", default=False)
    run_package_parser.set_defaults(handler=handle_run_package)

    ask_parser = subparsers.add_parser("ask-model", help="Ask a live model directly")
    ask_parser.add_argument(
        "--provider",
        choices=PROVIDER_CHOICES,
        default="aliyun",
    )
    ask_parser.add_argument("--model", default=None, help="Model name when using the selected provider")
    ask_parser.add_argument("--system", default="", help="Optional system prompt")
    prompt_source_group = ask_parser.add_mutually_exclusive_group(required=True)
    prompt_source_group.add_argument("--prompt", help="Inline user prompt")
    prompt_source_group.add_argument("--prompt-file", type=Path, help="Read the user prompt from a UTF-8 text file")
    ask_parser.add_argument("--output-file", type=Path, default=None, help="Optional file to write the model response into")
    ask_parser.set_defaults(handler=handle_ask_model)

    struct_parser = subparsers.add_parser(
        "extract-structured",
        help="Extract device IR using the Docling pipeline",
    )
    struct_parser.add_argument("--pdf", type=Path, required=True)
    struct_parser.add_argument("--device-id", default=None, help="Device identifier (defaults to PDF stem)")
    struct_parser.add_argument(
        "--provider",
        choices=PROVIDER_CHOICES,
        default="aliyun",
    )
    struct_parser.add_argument("--model", default=None, help="Model name for the selected provider")
    struct_parser.add_argument("--rtos", default=None, help="Optional target RTOS id to validate")
    struct_parser.add_argument(
        "--bus",
        default="i2c",
        help="Target bus type (i2c/spi/uart/1-wire/...), used for relevance assessment",
    )
    struct_parser.add_argument("--use-vlm", action="store_true", help="Enable Docling VLM mode")
    struct_parser.add_argument("--output-dir", type=Path, default=None, help="Optional fixed output directory")
    struct_parser.set_defaults(handler=handle_extract_structured)

    return parser


def handle_list_rtos(args: argparse.Namespace) -> int:
    print(json.dumps(list_registered_rtos(), indent=2))
    return 0


def handle_list_task_packages(args: argparse.Namespace) -> int:
    index_path = TASK_PACKAGES_ROOT / "index.json"
    payload = json.loads(index_path.read_text(encoding="utf-8"))
    package_ids = [
        str(entry["package_id"])
        for entry in payload.get("packages", [])
        if isinstance(entry, dict) and entry.get("package_id")
    ]
    print(json.dumps(sorted(package_ids), indent=2))
    return 0


def _load_prompt_text(args: argparse.Namespace) -> str:
    if args.prompt is not None:
        return args.prompt
    return args.prompt_file.read_text(encoding="utf-8")


def handle_run_package(args: argparse.Namespace) -> int:
    try:
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
            disable_llm_cache=args.no_llm_cache,
        )
    except (ProviderError, ValueError, FileNotFoundError, KeyError) as exc:
        print(json.dumps({"ok": False, "error": str(exc)}, indent=2))
        return 1

    print(json.dumps(report, indent=2))
    return 0 if report["all_checks_passed"] else 2


def handle_ask_model(args: argparse.Namespace) -> int:
    try:
        provider = create_provider(args.provider, args.model)
        prompt_text = _load_prompt_text(args).strip()
        if not prompt_text:
            raise ProviderError("The prompt is empty.")

        response_text = provider.generate_text(
            system_prompt=args.system,
            user_prompt=prompt_text,
            metadata={"command": "ask-model"},
        )
    except ProviderError as exc:
        print(json.dumps({"ok": False, "error": str(exc)}, indent=2))
        return 1

    if args.output_file is not None:
        args.output_file.parent.mkdir(parents=True, exist_ok=True)
        args.output_file.write_text(response_text, encoding="utf-8")
        print(
            json.dumps(
                {
                    "ok": True,
                    "provider": provider.name,
                    "model": provider.model,
                    "output_file": str(args.output_file),
                },
                indent=2,
            )
        )
        return 0

    print(response_text)
    return 0


def handle_extract_structured(args: argparse.Namespace) -> int:
    """Handle the extract-structured CLI command."""
    from ..core.catalog import RUNS_ROOT
    from ..datasheet.docling_backend import DoclingConfig
    from ..rtos import get_rtos_profile

    try:
        pdf_path = Path(args.pdf)
        if not pdf_path.exists():
            raise FileNotFoundError(f"PDF not found: {pdf_path}")
        device_id = args.device_id or pdf_path.stem

        if args.rtos:
            get_rtos_profile(args.rtos)
        provider = create_provider(args.provider, args.model)

        docling_config = DoclingConfig()
        docling_config.use_vlm = args.use_vlm

        timestamp = __import__("datetime").datetime.utcnow().strftime("%y%m%dT%H%M%SZ")
        run_dir = args.output_dir or (RUNS_ROOT / f"structured_{device_id}_{provider.name}_{timestamp}")

        device_ir = extract_device_ir_structured(
            device_id=device_id,
            pdf_path=pdf_path,
            provider=provider,
            run_dir=run_dir,
            server_config=docling_config,
            bus_type=args.bus,
        )
        run_dir.mkdir(parents=True, exist_ok=True)
        (run_dir / "device_ir.json").write_text(
            json.dumps(device_ir, indent=2), encoding="utf-8"
        )

        report = {
            "ok": True,
            "device_id": device_id,
            "provider": provider.name,
            "run_dir": str(run_dir),
            "extraction_backend": device_ir.get("_extraction_backend", "docling_structured"),
        }
        print(json.dumps(report, indent=2))
        return 0

    except (ProviderError, ValueError, FileNotFoundError, RuntimeError) as exc:
        print(json.dumps({"ok": False, "error": str(exc)}, indent=2))
        return 1


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    return args.handler(args)


if __name__ == "__main__":
    raise SystemExit(main())
