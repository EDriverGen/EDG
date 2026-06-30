"""evaluation.oracle - human-curated test oracle data."""
from pathlib import Path

ORACLE_ROOT = Path(__file__).resolve().parent
ORACLE_DATA_DIR = ORACLE_ROOT / "data"


def device_oracle_dir(device_id: str) -> Path:
    """Return the oracle directory for a given device id."""
    return ORACLE_DATA_DIR / device_id


__all__ = [
    "ORACLE_ROOT",
    "ORACLE_DATA_DIR",
    "device_oracle_dir",
]
