"""evaluation.infrastructure - static assets self-managed by evaluation."""
import re
from pathlib import Path

INFRASTRUCTURE_ROOT = Path(__file__).resolve().parent
PROJECT_ROOT = INFRASTRUCTURE_ROOT.parent.parent
PLATFORMS_DIR = INFRASTRUCTURE_ROOT / "platforms"
SLAVES_DIR = INFRASTRUCTURE_ROOT / "slaves"
HARNESS_DIR = INFRASTRUCTURE_ROOT / "harness"
STUBS_DIR = INFRASTRUCTURE_ROOT / "stubs"
HW_DIR = INFRASTRUCTURE_ROOT / "hw"
SHARED_DIR = INFRASTRUCTURE_ROOT / "shared"


# RTOS IDs come from two layers that use different spellings:
#   * task manifests / fixed_task_context use canonical long names
#     ("rt-thread", "freertos-kernel", "openharmony-liteosm").
#   * stub directories on disk use short names ("rtthread", "freertos",
#     "openharmony-liteosm") to keep the filesystem shallow.
# This alias table translates every known long-form RTOS id to the
# short-form stub directory name; unknown ids fall through unchanged
# so callers get the usual "stubs not found" diagnostic for typos.
_RTOS_ALIAS = {
    "rt-thread": "rtthread",
    "freertos-kernel": "freertos",
    "freertos_kernel": "freertos",
    "free-rtos": "freertos",
    "free_rtos": "freertos",
    "openharmony": "openharmony-liteosm",
    "liteos-m": "openharmony-liteosm",
    "liteos_m": "openharmony-liteosm",
    "liteosm": "openharmony-liteosm",
    "thread-x": "threadx",
    "thread_x": "threadx",
    "chibi-os": "chibios",
    "chibi_os": "chibios",
    "nutt-x": "nuttx",
    "nutt_x": "nuttx",
    "zephyr-rtos": "zephyr",
    "zephyr_rtos": "zephyr",
    "tobud": "tobudos",
    "tobud-os": "tobudos",
    "tobud_os": "tobudos",
    "xiu-os": "xiuos",
    "xiu_os": "xiuos",
    "cmsis_rtx": "cmsis-rtx",
    "cmsis-rtos2": "cmsis-rtx",
    "cmsis_rtos2": "cmsis-rtx",
    "rtx5": "cmsis-rtx",
    "apache_mynewt": "apache-mynewt",
    "mynewt": "apache-mynewt",
    "mynewt-core": "apache-mynewt",
    "mynewt_core": "apache-mynewt",
}


def stubs_for_rtos(rtos_id: str) -> Path:
    """Return the stubs directory for a given RTOS id."""
    key = str(rtos_id).strip().lower()
    canonical = _RTOS_ALIAS.get(key, key)
    return STUBS_DIR / canonical


# ---------- .repl filename path portability ----------

# Keeps older .repl snippets portable after the repository is moved.
_REPL_FILENAME_RE = re.compile(r'(filename:\s*")[^"]*(/DriverGen/)([^"]*")')


def rebase_repl_paths(repl_text: str) -> str:
    """Rebase generated .repl paths to project-relative paths."""
    project_prefix = ""

    def _replace(m: re.Match) -> str:
        return m.group(1) + project_prefix + m.group(3)

    return _REPL_FILENAME_RE.sub(_replace, repl_text)


def project_relative_path(path: Path) -> str:
    """Return a project-relative POSIX path when possible."""
    resolved = Path(path).resolve()
    try:
        return resolved.relative_to(PROJECT_ROOT.resolve()).as_posix()
    except ValueError:
        return str(resolved).replace("\\", "/")


__all__ = [
    "INFRASTRUCTURE_ROOT",
    "PROJECT_ROOT",
    "PLATFORMS_DIR",
    "SLAVES_DIR",
    "HARNESS_DIR",
    "STUBS_DIR",
    "HW_DIR",
    "SHARED_DIR",
    "stubs_for_rtos",
    "rebase_repl_paths",
    "project_relative_path",
]
