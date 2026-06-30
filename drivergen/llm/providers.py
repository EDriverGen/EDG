"""Provider adapters for live LLM calls.

This module keeps the transport and provider selection logic in one place.
"""

from __future__ import annotations

import json
import os
import re
import socket
import time
import urllib.error
import urllib.request
from pathlib import Path

from .prompts import json_repair as _json_repair_prompts

_RETRIABLE_HTTP_CODES: set[int] = {429, 500, 502, 503, 504}
_BACKOFF_SECONDS: tuple[float, ...] = (2.0, 5.0, 10.0)


def _int_env_default(names: tuple[str, ...], default: int) -> int:
    for name in names:
        raw = os.getenv(name)
        if raw is None or not raw.strip():
            continue
        try:
            value = int(raw)
        except ValueError:
            continue
        if value > 0:
            return value
    return default


class ProviderError(RuntimeError):
    """Raised when a provider cannot produce valid output."""


class BaseProvider:
    """Common interface for structured and free-form model providers."""

    name = "base"

    def generate_json(
        self,
        task_name: str,
        schema: dict,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> dict:
        """Return a JSON object that matches the requested task schema."""
        raise NotImplementedError

    def generate_text(
        self,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str:
        """Return plain text for lightweight probing or ad-hoc questions."""
        raise NotImplementedError


class CompatibleResponsesProvider(BaseProvider):
    """OpenAI-compatible Responses API provider used by multiple backends."""

    def __init__(
        self,
        provider_name: str,
        model: str,
        api_key: str | None,
        api_key_env_names: tuple[str, ...],
        base_url: str,
        timeout_seconds: int = 120,
    ):
        self.name = provider_name
        self.model = model
        self.api_key = api_key
        self.api_key_env_names = api_key_env_names
        self.base_url = base_url.rstrip("/")
        self.timeout_seconds = timeout_seconds
        if not self.api_key:
            env_display = " or ".join(api_key_env_names)
            raise ProviderError(f"{env_display} is not set.")

    def generate_json(
        self,
        task_name: str,
        schema: dict,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> dict:
        """Call the Responses API and parse its strict JSON-schema output."""
        payload = {
            "model": self.model,
            "input": [
                {"role": "system", "content": [{"type": "input_text", "text": system_prompt}]},
                {"role": "user", "content": [{"type": "input_text", "text": user_prompt}]},
            ],
            "text": {
                "format": {
                    "type": "json_schema",
                    "name": task_name,
                    "strict": True,
                    "schema": schema,
                }
            },
        }
        body = self._post_responses(payload)
        output_text = self._normalize_output_text(self._extract_output_text(body))
        if metadata.get("dump_raw_output"):
            self._debug_dump_text(metadata, task_name, "raw_output", output_text)
        parsed = self._load_structured_output(output_text, schema)
        parsed = self._postprocess_json_output(task_name, parsed, metadata)
        if _satisfies_schema_shape(parsed, schema):
            return parsed

        self._debug_dump_json(metadata, task_name, "response_body", body)
        self._debug_dump_text(metadata, task_name, "initial_output", output_text)

        repaired = self._repair_json_output(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if repaired is not None:
            self._debug_dump_text(metadata, task_name, "repair_output", repaired)
            repaired_text = self._normalize_output_text(repaired)
            parsed = self._load_structured_output(repaired_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed

        fallback = self._fallback_json_output(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if fallback is not None:
            self._debug_dump_text(metadata, task_name, "fallback_output", fallback)
            fallback_text = self._normalize_output_text(fallback)
            parsed = self._load_structured_output(fallback_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed

        self._debug_dump_text(
            metadata,
            task_name,
            "final_error",
            f"Provider returned invalid JSON for task '{task_name}'.",
        )
        raise ProviderError(f"Provider returned invalid JSON for task '{task_name}'.")

    def generate_text(
        self,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str:
        """Call the Responses API without schema enforcement."""
        messages = []
        if system_prompt.strip():
            messages.append({"role": "system", "content": [{"type": "input_text", "text": system_prompt}]})
        messages.append({"role": "user", "content": [{"type": "input_text", "text": user_prompt}]})
        payload = {
            "model": self.model,
            "input": messages,
        }
        body = self._post_responses(payload)
        return self._normalize_output_text(self._extract_output_text(body))

    def _post_responses(self, payload: dict) -> dict:
        """Send a single OpenAI-compatible Responses request."""
        request = urllib.request.Request(
            f"{self.base_url}/responses",
            data=json.dumps(payload).encode("utf-8"),
            headers={
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
            },
            method="POST",
        )
        body: dict | None = None
        last_http_error: urllib.error.HTTPError | None = None
        last_http_error_text: str = ""
        for attempt in range(3):
            try:
                with urllib.request.urlopen(request, timeout=self.timeout_seconds) as response:
                    body = json.loads(response.read().decode("utf-8"))
                break
            except urllib.error.HTTPError as exc:
                last_http_error = exc
                last_http_error_text = exc.read().decode("utf-8", errors="replace")
                if exc.code in _RETRIABLE_HTTP_CODES and attempt < 2:
                    time.sleep(_BACKOFF_SECONDS[attempt])
                    continue
                raise ProviderError(
                    f"{self.name} Responses API failed: {exc.code} {last_http_error_text}"
                ) from exc
            except urllib.error.URLError as exc:
                raise ProviderError(f"{self.name} Responses API connection failed: {exc}") from exc

        if body is None:
            code = last_http_error.code if last_http_error else "unknown"
            raise ProviderError(
                f"{self.name} Responses API failed: {code} {last_http_error_text}"
            )

        # Check for logical failure returned inside a 200 response body
        if body.get("status") == "failed" or body.get("error"):
            error_info = body.get("error", {})
            code = error_info.get("code", "unknown") if isinstance(error_info, dict) else str(error_info)
            message = error_info.get("message", "") if isinstance(error_info, dict) else ""
            raise ProviderError(f"{self.name} API returned error ({code}): {message}")

        return body

    @staticmethod
    def _extract_output_text(body: dict) -> str:
        """Extract text across the common OpenAI-compatible response shapes."""
        # Responses API: top-level output_text
        if isinstance(body.get("output_text"), str) and body["output_text"].strip():
            return body["output_text"]

        # Responses API output[].content[].text
        texts = []
        for item in body.get("output", []):
            if isinstance(item, dict) and isinstance(item.get("content"), list):
                for content in item["content"]:
                    if isinstance(content, dict) and isinstance(content.get("text"), str):
                        texts.append(content["text"])
        if texts:
            return "\n".join(texts).strip()

        # Chat Completions fallback: choices[].message.content
        for choice in body.get("choices", []):
            msg = choice.get("message", {}) if isinstance(choice, dict) else {}
            content = msg.get("content")
            if isinstance(content, str) and content.strip():
                return content.strip()

        import logging
        logging.getLogger(__name__).error("Unrecognized response body keys: %s", list(body.keys()))
        raise ProviderError("Could not extract text from the Responses API payload.")

    def _normalize_output_text(self, text: str) -> str:
        """Allow subclasses to clean provider-specific response artifacts."""
        return text.strip()

    def _repair_json_output(
        self,
        task_name: str,
        schema: dict,
        invalid_output: str,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str | None:
        """Allow subclasses to repair non-JSON output with provider-specific logic."""
        return None

    def _fallback_json_output(
        self,
        task_name: str,
        schema: dict,
        invalid_output: str,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str | None:
        """Fallback recovery path using a plain text call and local JSON parsing."""
        fallback_system, fallback_user = _json_repair_prompts.build_fallback_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        )
        try:
            return self.generate_text(fallback_system, fallback_user, metadata)
        except ProviderError:
            return None

    def _postprocess_json_output(self, task_name: str, payload: object, metadata: dict) -> object:
        """Normalize provider output into the naming conventions expected downstream."""
        if task_name != "extract_device_ir" or not isinstance(payload, dict):
            return payload

        normalized = dict(payload)
        canonical_device_id = metadata.get("device_id")
        if canonical_device_id:
            normalized["device_id"] = canonical_device_id

        bus_type = normalized.get("bus_type")
        if isinstance(bus_type, str):
            normalized["bus_type"] = bus_type.lower()

        evidence_spans = normalized.get("evidence_spans")
        if canonical_device_id and isinstance(evidence_spans, list):
            normalized["evidence_spans"] = [
                {
                    **span,
                    "source_id": canonical_device_id if isinstance(span, dict) else span,
                }
                if isinstance(span, dict)
                else span
                for span in evidence_spans
            ]

        return normalized

    def _debug_root(self, metadata: dict) -> Path | None:
        path_text = metadata.get("debug_dir")
        if not path_text:
            return None
        try:
            path = Path(path_text)
            path.mkdir(parents=True, exist_ok=True)
            return path
        except OSError:
            return None

    def _debug_path(self, metadata: dict, task_name: str, label: str, suffix: str) -> Path | None:
        root = self._debug_root(metadata)
        if root is None:
            return None
        safe_task = re.sub(r"[^A-Za-z0-9._-]+", "_", task_name).strip("_") or "task"
        safe_label = re.sub(r"[^A-Za-z0-9._-]+", "_", label).strip("_") or "artifact"
        return root / f"{safe_task}__{safe_label}{suffix}"

    def _debug_dump_text(self, metadata: dict, task_name: str, label: str, text: str) -> None:
        path = self._debug_path(metadata, task_name, label, ".txt")
        if path is None:
            return
        try:
            path.write_text(text, encoding="utf-8")
        except OSError:
            return

    def _debug_dump_json(self, metadata: dict, task_name: str, label: str, payload: object) -> None:
        path = self._debug_path(metadata, task_name, label, ".json")
        if path is None:
            return
        try:
            path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
        except OSError:
            return

    @staticmethod
    def _load_structured_output(text: str, schema: dict) -> dict | list | None:
        """Parse direct JSON or recover it from fenced/code-mixed model output."""
        expected_type = schema.get("type")

        direct = _try_load_json(text)
        if _matches_expected_json_type(direct, expected_type):
            return direct

        for candidate in _iter_json_candidates(text):
            parsed = _try_load_json(candidate)
            if _matches_expected_json_type(parsed, expected_type):
                return parsed
        return None


class OpenAIResponsesProvider(CompatibleResponsesProvider):
    """Live provider for the official OpenAI Responses API."""

    def __init__(self, model: str = "gpt-5-mini", api_key: str | None = None, timeout_seconds: int = 120):
        super().__init__(
            provider_name="openai",
            model=model,
            api_key=api_key or os.getenv("OPENAI_API_KEY"),
            api_key_env_names=("OPENAI_API_KEY",),
            base_url=os.getenv("OPENAI_BASE_URL", "https://api.openai.com/v1"),
            timeout_seconds=timeout_seconds,
        )


class AliyunResponsesProvider(CompatibleResponsesProvider):
    """OpenAI-compatible provider for the Aliyun Bailian endpoint."""

    def __init__(self, model: str = "qwen3.5-35b-a3b", api_key: str | None = None, timeout_seconds: int = 600):
        self.chat_base_url = os.getenv(
            "ALIYUN_BAILIAN_CHAT_BASE_URL",
            os.getenv("DASHSCOPE_CHAT_BASE_URL", "https://dashscope.aliyuncs.com/compatible-mode/v1"),
        ).rstrip("/")
        super().__init__(
            provider_name="aliyun",
            model=model,
            api_key=api_key or os.getenv("DASHSCOPE_API_KEY") or os.getenv("ALIYUN_BAILIAN_API_KEY"),
            api_key_env_names=("DASHSCOPE_API_KEY", "ALIYUN_BAILIAN_API_KEY"),
            base_url=os.getenv(
                "ALIYUN_BAILIAN_BASE_URL",
                os.getenv(
                    "DASHSCOPE_BASE_URL",
                    "https://dashscope.aliyuncs.com/api/v2/apps/protocols/compatible-mode/v1",
                ),
            ),
            timeout_seconds=timeout_seconds,
        )

    def _uses_chat_completions_backend(self) -> bool:
        model_name = self.model.lower()
        return (
            model_name.startswith("glm-")
            or model_name.startswith("zhipu/glm-")
            or model_name.startswith("qwen3-coder-")
            or model_name.startswith("qwen/qwen3-coder-")
            or model_name.startswith("qwen3.5-")
            or model_name.startswith("minimax-")
            or model_name.startswith("kimi-")
        )

    def _post_chat_completions(self, payload: dict) -> dict:
        """Retries 429/503 with exponential backoff (same policy as Responses)."""
        request = urllib.request.Request(
            f"{self.chat_base_url}/chat/completions",
            data=json.dumps(payload).encode("utf-8"),
            headers={
                "Authorization": f"Bearer {self.api_key}",
                "Content-Type": "application/json",
            },
            method="POST",
        )
        last_error: Exception | None = None
        last_http_error_text: str = ""
        body: dict | None = None
        for attempt in range(3):
            try:
                with urllib.request.urlopen(request, timeout=self.timeout_seconds) as response:
                    body = json.loads(response.read().decode("utf-8"))
                break
            except urllib.error.HTTPError as exc:
                last_http_error_text = exc.read().decode("utf-8", errors="replace")
                last_error = exc
                if exc.code in _RETRIABLE_HTTP_CODES and attempt < 2:
                    time.sleep(_BACKOFF_SECONDS[attempt])
                    continue
                raise ProviderError(
                    f"{self.name} Chat Completions API failed: {exc.code} {last_http_error_text}"
                ) from exc
            except (urllib.error.URLError, TimeoutError, socket.timeout) as exc:
                last_error = exc
                if attempt == 2:
                    raise ProviderError(f"{self.name} Chat Completions API connection failed: {exc}") from exc
                time.sleep(_BACKOFF_SECONDS[attempt])
        if body is None:
            raise ProviderError(f"{self.name} Chat Completions API connection failed: {last_error}")

        if body.get("error"):
            error_info = body.get("error", {})
            code = error_info.get("code", "unknown") if isinstance(error_info, dict) else str(error_info)
            message = error_info.get("message", "") if isinstance(error_info, dict) else ""
            raise ProviderError(f"{self.name} API returned error ({code}): {message}")
        return body

    @staticmethod
    def _extract_chat_completion_text(body: dict) -> str:
        for choice in body.get("choices", []):
            message = choice.get("message", {}) if isinstance(choice, dict) else {}
            content = message.get("content")
            if isinstance(content, str) and content.strip():
                return content.strip()
        raise ProviderError("Could not extract text from the Chat Completions payload.")

    def _generate_json_via_chat_completions(
        self,
        task_name: str,
        schema: dict,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> dict:
        # Some models (MiniMax-M2.5) require enable_thinking=True
        model_lower = self.model.lower()
        enable_thinking = model_lower.startswith("minimax-")
        payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_prompt},
            ],
            "response_format": {
                "type": "json_schema",
                "json_schema": {
                    "name": task_name,
                    "strict": True,
                    "schema": schema,
                },
            },
        }
        if enable_thinking:
            payload["enable_thinking"] = True
        body = self._post_chat_completions(payload)
        output_text = self._normalize_output_text(self._extract_chat_completion_text(body))
        if metadata.get("dump_raw_output"):
            self._debug_dump_text(metadata, task_name, "raw_output", output_text)
        parsed = self._load_structured_output(output_text, schema)
        parsed = self._postprocess_json_output(task_name, parsed, metadata)
        if _satisfies_schema_shape(parsed, schema):
            return parsed
        self._debug_dump_json(metadata, task_name, "chat_completion_body", body)
        self._debug_dump_text(metadata, task_name, "initial_output", output_text)
        repaired = self._repair_json_via_chat_completions(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if repaired is not None:
            self._debug_dump_text(metadata, task_name, "repair_output", repaired)
            repaired_text = self._normalize_output_text(repaired)
            parsed = self._load_structured_output(repaired_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed
        fallback = self._fallback_json_output(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if fallback is not None:
            self._debug_dump_text(metadata, task_name, "fallback_output", fallback)
            fallback_text = self._normalize_output_text(fallback)
            parsed = self._load_structured_output(fallback_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed
        self._debug_dump_text(
            metadata,
            task_name,
            "final_error",
            f"{self.name} chat-completions backend returned invalid JSON for task '{task_name}'.",
        )
        raise ProviderError(f"{self.name} chat-completions backend returned invalid JSON for task '{task_name}'.")

    def _repair_json_via_chat_completions(
        self,
        task_name: str,
        schema: dict,
        invalid_output: str,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str | None:
        repair_system, repair_user = _json_repair_prompts.build_repair_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        )
        repair_payload = {
            "model": self.model,
            "messages": [
                {"role": "system", "content": repair_system},
                {"role": "user", "content": repair_user},
            ],
            "enable_thinking": False,
            "response_format": {
                "type": "json_schema",
                "json_schema": {
                    "name": f"{task_name}_repair",
                    "strict": True,
                    "schema": schema,
                },
            },
        }
        try:
            body = self._post_chat_completions(repair_payload)
        except ProviderError:
            return None
        return self._extract_chat_completion_text(body)

    def _generate_text_via_chat_completions(
        self,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str:
        messages = []
        if system_prompt.strip():
            messages.append({"role": "system", "content": system_prompt})
        messages.append({"role": "user", "content": user_prompt})
        payload = {
            "model": self.model,
            "messages": messages,
            "temperature": 0,
        }
        model_lower = self.model.lower()
        # MiniMax models require enable_thinking=True
        if model_lower.startswith("minimax-"):
            payload["enable_thinking"] = True
        # Qwen 3.5 thinking models: disable thinking for code-gen speed
        elif model_lower.startswith("qwen3.5-"):
            payload["enable_thinking"] = False
        body = self._post_chat_completions(payload)
        return self._normalize_output_text(self._extract_chat_completion_text(body))

    def generate_json(
        self,
        task_name: str,
        schema: dict,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> dict:
        if self._uses_chat_completions_backend():
            return self._generate_json_via_chat_completions(task_name, schema, system_prompt, user_prompt, metadata)
        return super().generate_json(task_name, schema, system_prompt, user_prompt, metadata)

    def generate_text(
        self,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str:
        if self._uses_chat_completions_backend():
            return self._generate_text_via_chat_completions(system_prompt, user_prompt, metadata)
        return super().generate_text(system_prompt, user_prompt, metadata)

    def _repair_json_output(
        self,
        task_name: str,
        schema: dict,
        invalid_output: str,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str | None:
        """_repair_json_output helper."""
        repair_system, repair_user = _json_repair_prompts.build_repair_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        )
        repair_payload = {
            "model": self.model,
            "input": [
                {
                    "role": "system",
                    "content": [
                        {"type": "input_text", "text": repair_system},
                    ],
                },
                {
                    "role": "user",
                    "content": [
                        {"type": "input_text", "text": repair_user},
                    ],
                },
            ],
            "text": {
                "format": {
                    "type": "json_schema",
                    "name": f"{task_name}_repair",
                    "strict": True,
                    "schema": schema,
                }
            },
        }
        try:
            body = self._post_responses(repair_payload)
        except ProviderError:
            return None
        return self._extract_output_text(body)


_DEEPSEEK_KEY_FILENAMES: tuple[str, ...] = ("", "deepseek_key")
_DEEPSEEK_KEY_DIRS: tuple[str, ...] = (".secrets", "secrets")


def _load_deepseek_key_from_file() -> str | None:
    """Walk a few well-known locations and return the first key found."""
    explicit = os.environ.get("DRIVERGEN_DEEPSEEK_KEY_FILE")
    if explicit:
        try:
            text = Path(explicit).read_text(encoding="utf-8").strip()
            if text:
                return text
        except OSError:
            pass

    candidates: list[Path] = []
    repo_root = Path(__file__).resolve().parents[2]
    for sub in _DEEPSEEK_KEY_DIRS:
        for name in _DEEPSEEK_KEY_FILENAMES:
            candidates.append(repo_root / sub / name)
    home = Path.home()
    for name in _DEEPSEEK_KEY_FILENAMES:
        candidates.append(home / ".drivergen" / name)

    for path in candidates:
        try:
            if path.is_file():
                text = path.read_text(encoding="utf-8").strip()
                if text:
                    return text
        except OSError:
            continue
    return None


class DeepSeekProvider(AliyunResponsesProvider):
    """Chat-completions provider for DeepSeek with thinking-mode support."""

    def __init__(
        self,
        model: str = "deepseek-v4-flash",
        api_key: str | None = None,
        timeout_seconds: int = 600,
    ):
        timeout_seconds = _int_env_default(
            ("DEEPSEEK_TIMEOUT_SECONDS", "DRIVERGEN_LLM_TIMEOUT_SECONDS"),
            timeout_seconds,
        )
        resolved_key = (
            api_key or os.getenv("DEEPSEEK_API_KEY") or _load_deepseek_key_from_file()
        )
        base_url = os.getenv("DEEPSEEK_BASE_URL", "https://api.deepseek.com")
        self.chat_base_url = base_url.rstrip("/")
        # ``high`` matches the v4-flash docs default; ``max`` tier is
        # reserved for agentic workflows. We expose this through an
        # env var so experiments can sweep effort without touching code.
        self.reasoning_effort = os.getenv("DEEPSEEK_REASONING_EFFORT", "high")
        # Optional thinking mode toggle for provider comparisons.
        thinking_raw = os.getenv("DEEPSEEK_THINKING", "disabled").strip().lower()
        if thinking_raw in ("disabled", "off", "false", "0", "no"):
            self.thinking_enabled = False
        else:
            self.thinking_enabled = True
        # Thinking mode shares the output budget between ``reasoning_content``
        # and ``content``. Use an explicit, generous default so JSON content is
        # not truncated by reasoning tokens; expose an env knob for sweeps.
        try:
            self.max_tokens = int(os.getenv("DEEPSEEK_MAX_TOKENS", "16384"))
        except ValueError:
            self.max_tokens = 16384
        CompatibleResponsesProvider.__init__(
            self,
            provider_name="deepseek",
            model=model,
            api_key=resolved_key,
            api_key_env_names=("DEEPSEEK_API_KEY",),
            base_url=base_url,
            timeout_seconds=timeout_seconds,
        )

    def _uses_chat_completions_backend(self) -> bool:
        return True

    def _with_thinking(self, base: dict) -> dict:
        """Attach thinking-mode flags onto a chat-completions payload."""
        payload = dict(base)
        if self.thinking_enabled:
            payload["thinking"] = {"type": "enabled"}
            if self.reasoning_effort:
                payload["reasoning_effort"] = self.reasoning_effort
        else:
            payload["thinking"] = {"type": "disabled"}
            # In non-thinking mode the API DOES accept ``temperature`` and
            # we want deterministic output for repeatable smoke tests.
            payload.setdefault("temperature", 0)
        if self.max_tokens and "max_tokens" not in payload:
            payload["max_tokens"] = self.max_tokens
        return payload

    def _generate_json_via_chat_completions(
        self,
        task_name: str,
        schema: dict,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> dict:
        """Use ``response_format=json_object`` plus an inline schema hint."""
        schema_hint = (
            "\n\nYou MUST respond with a single valid JSON object that matches this schema:\n"
            f"{json.dumps(schema, ensure_ascii=False, indent=2)}"
        )
        user_reminder = (
            "\n\nRespond with a single valid JSON object that matches the schema above."
        )
        payload = self._with_thinking({
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt + schema_hint},
                {"role": "user", "content": user_prompt + user_reminder},
            ],
            "response_format": {"type": "json_object"},
        })
        body = self._post_chat_completions(payload)
        output_text = self._normalize_output_text(self._extract_chat_completion_text(body))
        if metadata.get("dump_raw_output"):
            self._debug_dump_text(metadata, task_name, "raw_output", output_text)
        parsed = self._load_structured_output(output_text, schema)
        parsed = self._postprocess_json_output(task_name, parsed, metadata)
        if _satisfies_schema_shape(parsed, schema):
            return parsed
        self._debug_dump_json(metadata, task_name, "chat_completion_body", body)
        self._debug_dump_text(metadata, task_name, "initial_output", output_text)

        repaired = self._repair_json_via_chat_completions(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if repaired is not None:
            self._debug_dump_text(metadata, task_name, "repair_output", repaired)
            repaired_text = self._normalize_output_text(repaired)
            parsed = self._load_structured_output(repaired_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed

        fallback = self._fallback_json_output(
            task_name=task_name,
            schema=schema,
            invalid_output=output_text,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
            metadata=metadata,
        )
        if fallback is not None:
            self._debug_dump_text(metadata, task_name, "fallback_output", fallback)
            fallback_text = self._normalize_output_text(fallback)
            parsed = self._load_structured_output(fallback_text, schema)
            parsed = self._postprocess_json_output(task_name, parsed, metadata)
            if _satisfies_schema_shape(parsed, schema):
                return parsed

        raise ProviderError(
            f"{self.name} chat-completions returned invalid JSON for task '{task_name}'."
        )

    def _repair_json_via_chat_completions(
        self,
        task_name: str,
        schema: dict,
        invalid_output: str,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str | None:
        """Repair using ``json_object`` mode — thinking still on by default."""
        repair_system, repair_user = _json_repair_prompts.build_repair_prompt(
            task_name=task_name,
            schema=schema,
            invalid_output=invalid_output,
            system_prompt=system_prompt,
            user_prompt=user_prompt,
        )
        repair_payload = self._with_thinking({
            "model": self.model,
            "messages": [
                {"role": "system", "content": repair_system},
                {"role": "user", "content": repair_user},
            ],
            "response_format": {"type": "json_object"},
        })
        try:
            body = self._post_chat_completions(repair_payload)
        except ProviderError:
            return None
        return self._extract_chat_completion_text(body)

    def _generate_text_via_chat_completions(
        self,
        system_prompt: str,
        user_prompt: str,
        metadata: dict,
    ) -> str:
        """Plain-text path with thinking on, NO ``temperature`` field."""
        messages = []
        if system_prompt.strip():
            messages.append({"role": "system", "content": system_prompt})
        messages.append({"role": "user", "content": user_prompt})
        payload = self._with_thinking({
            "model": self.model,
            "messages": messages,
        })
        body = self._post_chat_completions(payload)
        return self._normalize_output_text(self._extract_chat_completion_text(body))

def default_model_for_provider(provider_name: str) -> str:
    """Return the default model name used when a caller omits one."""
    if provider_name == "aliyun":
        return "qwen3.5-35b-a3b"
    if provider_name == "deepseek":
        return "deepseek-v4-flash"
    return "gpt-5-mini"


def create_provider(provider_name: str, model: str) -> BaseProvider:
    """Instantiate a provider by name with project-level defaults."""
    resolved_model = model
    if provider_name == "aliyun":
        return AliyunResponsesProvider(model=resolved_model)
    if provider_name == "deepseek":
        return DeepSeekProvider(model=resolved_model)
    if provider_name == "openai":
        return OpenAIResponsesProvider(model=resolved_model)
    raise ProviderError(f"Unsupported provider '{provider_name}'.")


def _strip_reasoning_content(text: str) -> str:
    """Remove provider-emitted thinking blocks before the caller consumes text."""
    cleaned = text.replace("\r\n", "\n")
    cleaned = re.sub(r"<think>.*?</think>", "", cleaned, flags=re.DOTALL | re.IGNORECASE)
    if "</think>" in cleaned:
        cleaned = cleaned.split("</think>", 1)[1]
    if cleaned.lstrip().startswith("Thinking Process:") and "\n\n" in cleaned:
        cleaned = cleaned.split("\n\n", 1)[1]
    return cleaned.strip()


def _matches_expected_json_type(payload: object, expected_type: str | None) -> bool:
    """Check whether parsed JSON matches the top-level schema type."""
    if payload is None:
        return False
    if expected_type == "object":
        return isinstance(payload, dict)
    if expected_type == "array":
        return isinstance(payload, list)
    return isinstance(payload, (dict, list))


def _satisfies_schema_shape(payload: object, schema: dict) -> bool:
    """Check whether parsed JSON satisfies the schema's top-level shape hints."""
    if not _matches_expected_json_type(payload, schema.get("type")):
        return False
    if isinstance(payload, dict):
        required = schema.get("required", [])
        return all(field in payload for field in required)
    return True


# Lenient JSON repair for recoverable structured-output quirks.
_LLM_HEX_LITERAL_RE = re.compile(
    r"(?<=[:\[,])(\s*)(-?0[xX][0-9a-fA-F]+)"
)
_LLM_TRAILING_COMMA_RE = re.compile(r",(\s*[\]}])")


def _llm_string_segments(text: str) -> list[tuple[int, int]]:
    """Index ranges of JSON string literals - see ``_strip_string_segments`` in ``codegen.code_generator`` for the original implementation."""
    spans: list[tuple[int, int]] = []
    in_string = False
    escape = False
    start = -1
    for i, ch in enumerate(text):
        if in_string:
            if escape:
                escape = False
                continue
            if ch == "\\":
                escape = True
                continue
            if ch == '"':
                in_string = False
                spans.append((start, i + 1))
                start = -1
            continue
        if ch == '"':
            in_string = True
            start = i
    if in_string and start >= 0:
        spans.append((start, len(text)))
    return spans


def _llm_relax_json(text: str) -> str:
    """Apply the lenient-repair passes (hex literals, trailing commas) used by the structured-output path."""
    spans = _llm_string_segments(text)

    def _inside_string(idx: int) -> bool:
        for s, e in spans:
            if s <= idx < e:
                return True
            if s > idx:
                return False
        return False

    def _rewrite_hex(match: "re.Match[str]") -> str:
        # Preserve hex byte/address tokens as schema strings.
        leading_ws = match.group(1)
        token = match.group(2)
        if _inside_string(match.start(2)):
            return match.group(0)
        return f'{leading_ws}"{token}"'

    rewritten = _LLM_HEX_LITERAL_RE.sub(_rewrite_hex, text)
    rewritten = _LLM_TRAILING_COMMA_RE.sub(lambda m: m.group(1), rewritten)
    return rewritten


def _try_load_json(text: str) -> dict | list | None:
    """Try to parse a string as JSON without raising parsing errors."""
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        pass
    relaxed = _llm_relax_json(text)
    if relaxed == text:
        return None
    try:
        return json.loads(relaxed)
    except json.JSONDecodeError:
        return None


def _iter_json_candidates(text: str) -> list[str]:
    """Yield plausible JSON substrings from markdown-heavy model output."""
    candidates: list[str] = []
    seen: set[str] = set()

    for match in re.finditer(r"```(?:json)?\s*(.*?)```", text, flags=re.DOTALL | re.IGNORECASE):
        candidate = match.group(1).strip()
        if candidate and candidate not in seen:
            candidates.append(candidate)
            seen.add(candidate)

    for opener in ("{", "["):
        start = text.find(opener)
        while start != -1:
            candidate = _extract_balanced_json(text, start)
            if candidate and candidate not in seen:
                candidates.append(candidate)
                seen.add(candidate)
            start = text.find(opener, start + 1)

    return candidates


def _extract_balanced_json(text: str, start_index: int) -> str | None:
    """Extract a balanced JSON object/array substring from mixed text."""
    opening = text[start_index]
    closing = "}" if opening == "{" else "]"
    depth = 0
    in_string = False
    escape = False

    for index in range(start_index, len(text)):
        char = text[index]
        if in_string:
            if escape:
                escape = False
                continue
            if char == "\\":
                escape = True
                continue
            if char == '"':
                in_string = False
            continue

        if char == '"':
            in_string = True
            continue
        if char == opening:
            depth += 1
            continue
        if char == closing:
            depth -= 1
            if depth == 0:
                return text[start_index : index + 1].strip()

    return None
