"""Safe evaluator for ``integer_approximation_expression`` strings emitted by the device IR."""
from __future__ import annotations

import ast
import re
from typing import Any, Mapping

__all__ = [
    "ExpressionError",
    "EXPRESSION_PATTERN",
    "ALLOWED_CALLS",
    "safe_eval_integer_approximation",
    "validate_expression_object",
    "check_expression_safety",
]


class ExpressionError(ValueError):
    """Raised when an ``integer_approximation_expression`` cannot be parsed
    or violates the safety whitelist."""


# Whitelist of characters accepted in a compiled expression string. Expressions
# are single-line arithmetic/bitwise formulas over caller-provided names.
EXPRESSION_PATTERN = re.compile(
    r"^[A-Za-z_][A-Za-z0-9_]*"   # first identifier
    r"|^[\-+0-9()]"              # or starts with number/paren/unary sign
)


ALLOWED_CALLS: frozenset[str] = frozenset({"abs", "min", "max", "round"})

_ALLOWED_BIN_OPS: tuple[type, ...] = (
    ast.Add, ast.Sub, ast.Mult, ast.Div, ast.FloorDiv, ast.Mod,
    ast.LShift, ast.RShift, ast.BitAnd, ast.BitOr, ast.BitXor,
)
_ALLOWED_UNARY_OPS: tuple[type, ...] = (ast.UAdd, ast.USub, ast.Invert)


def _reject(node: ast.AST, reason: str) -> "ExpressionError":
    return ExpressionError(f"{reason} at node {type(node).__name__}")


def _eval_node(node: ast.AST, env: Mapping[str, Any]) -> Any:
    if isinstance(node, ast.Expression):
        return _eval_node(node.body, env)

    if isinstance(node, ast.Constant):
        if isinstance(node.value, (int, float, bool)):
            return node.value
        raise _reject(node, "only int/float/bool literals are allowed")

    if isinstance(node, ast.Name):
        name = node.id
        if name.startswith("_"):
            raise _reject(node, f"name '{name}' is not allowed (dunder/private)")
        if name not in env:
            raise ExpressionError(f"unknown name '{name}' (pass it via env)")
        return env[name]

    if isinstance(node, ast.UnaryOp):
        if not isinstance(node.op, _ALLOWED_UNARY_OPS):
            raise _reject(node.op, "unary operator not allowed")
        return _apply_unary(node.op, _eval_node(node.operand, env))

    if isinstance(node, ast.BinOp):
        if not isinstance(node.op, _ALLOWED_BIN_OPS):
            raise _reject(node.op, "binary operator not allowed")
        left = _eval_node(node.left, env)
        right = _eval_node(node.right, env)
        return _apply_binary(node.op, left, right)

    if isinstance(node, ast.Call):
        # Only bare-name calls to the whitelist.  No ``obj.method(...)``.
        if not isinstance(node.func, ast.Name):
            raise _reject(node.func, "only bare-name calls allowed")
        name = node.func.id
        if name not in ALLOWED_CALLS:
            raise ExpressionError(f"call to '{name}' is not in ALLOWED_CALLS")
        if node.keywords:
            raise _reject(node, "keyword arguments not allowed")
        args = [_eval_node(a, env) for a in node.args]
        return _call_allowed(name, args)

    raise _reject(node, "node type not allowed")


def _apply_unary(op: ast.AST, value: Any) -> Any:
    if isinstance(op, ast.UAdd):
        return +value
    if isinstance(op, ast.USub):
        return -value
    if isinstance(op, ast.Invert):
        return ~int(value)
    raise ExpressionError(f"unary op {type(op).__name__} not handled")


def _apply_binary(op: ast.AST, left: Any, right: Any) -> Any:
    if isinstance(op, ast.Add):
        return left + right
    if isinstance(op, ast.Sub):
        return left - right
    if isinstance(op, ast.Mult):
        return left * right
    if isinstance(op, ast.Div):
        if right == 0:
            raise ExpressionError("division by zero")
        return left / right
    if isinstance(op, ast.FloorDiv):
        if right == 0:
            raise ExpressionError("floor-division by zero")
        return left // right
    if isinstance(op, ast.Mod):
        if right == 0:
            raise ExpressionError("modulo by zero")
        return left % right
    if isinstance(op, ast.LShift):
        return int(left) << int(right)
    if isinstance(op, ast.RShift):
        return int(left) >> int(right)
    if isinstance(op, ast.BitAnd):
        return int(left) & int(right)
    if isinstance(op, ast.BitOr):
        return int(left) | int(right)
    if isinstance(op, ast.BitXor):
        return int(left) ^ int(right)
    raise ExpressionError(f"binary op {type(op).__name__} not handled")


def _call_allowed(name: str, args: list[Any]) -> Any:
    if name == "abs":
        if len(args) != 1:
            raise ExpressionError("abs() takes exactly one argument")
        return abs(args[0])
    if name == "round":
        if len(args) not in (1, 2):
            raise ExpressionError("round() takes 1 or 2 arguments")
        return round(*args)
    if name == "min":
        if not args:
            raise ExpressionError("min() needs at least one argument")
        return min(*args) if len(args) > 1 else min(args[0])
    if name == "max":
        if not args:
            raise ExpressionError("max() needs at least one argument")
        return max(*args) if len(args) > 1 else max(args[0])
    raise ExpressionError(f"call '{name}' not implemented (should be in ALLOWED_CALLS)")


def safe_eval_integer_approximation(
    expr_obj: Mapping[str, Any] | str | None,
    env: Mapping[str, Any],
) -> Any:
    """Evaluate an ``integer_approximation_expression`` safely."""
    if expr_obj is None:
        raise ExpressionError("integer_approximation_expression is null")
    if isinstance(expr_obj, Mapping):
        expr_str = expr_obj.get("expression")
        if not isinstance(expr_str, str) or not expr_str.strip():
            raise ExpressionError(
                "integer_approximation_expression.expression must be a non-empty string"
            )
    elif isinstance(expr_obj, str):
        expr_str = expr_obj
    else:
        raise ExpressionError(
            f"integer_approximation_expression must be dict|str|None, got {type(expr_obj).__name__}"
        )

    expr_stripped = expr_str.strip()
    if "\n" in expr_stripped or ";" in expr_stripped:
        raise ExpressionError("expression must be a single line without ';'")

    try:
        tree = ast.parse(expr_stripped, mode="eval")
    except SyntaxError as exc:
        raise ExpressionError(f"syntax error: {exc}") from exc

    return _eval_node(tree, env)


def check_expression_safety(expr_str: str) -> None:
    """Lint an expression against the safe_eval whitelist **without evaluating it."""
    if not isinstance(expr_str, str) or not expr_str.strip():
        raise ExpressionError("expression must be a non-empty string")
    stripped = expr_str.strip()
    if "\n" in stripped or ";" in stripped:
        raise ExpressionError("expression must be a single line without ';'")
    try:
        tree = ast.parse(stripped, mode="eval")
    except SyntaxError as exc:
        raise ExpressionError(f"syntax error: {exc}") from exc
    _walk_check(tree)


def _walk_check(node: ast.AST) -> None:
    if isinstance(node, ast.Expression):
        _walk_check(node.body)
        return
    if isinstance(node, ast.Constant):
        if not isinstance(node.value, (int, float, bool)):
            raise _reject(node, "only int/float/bool literals are allowed")
        return
    if isinstance(node, ast.Name):
        if node.id.startswith("_"):
            raise _reject(node, f"name '{node.id}' is not allowed (dunder/private)")
        return
    if isinstance(node, ast.UnaryOp):
        if not isinstance(node.op, _ALLOWED_UNARY_OPS):
            raise _reject(node.op, "unary operator not allowed")
        _walk_check(node.operand)
        return
    if isinstance(node, ast.BinOp):
        if not isinstance(node.op, _ALLOWED_BIN_OPS):
            raise _reject(node.op, "binary operator not allowed")
        _walk_check(node.left)
        _walk_check(node.right)
        return
    if isinstance(node, ast.Call):
        if not isinstance(node.func, ast.Name):
            raise _reject(node.func, "only bare-name calls allowed")
        if node.func.id not in ALLOWED_CALLS:
            raise ExpressionError(f"call to '{node.func.id}' is not in ALLOWED_CALLS")
        if node.keywords:
            raise _reject(node, "keyword arguments not allowed")
        for arg in node.args:
            _walk_check(arg)
        return
    raise _reject(node, "node type not allowed")


def validate_expression_object(expr_obj: Mapping[str, Any]) -> list[str]:
    """Shallow shape check for ``integer_approximation_expression`` without evaluating."""
    issues: list[str] = []
    if not isinstance(expr_obj, Mapping):
        return [f"must be an object, got {type(expr_obj).__name__}"]
    expr = expr_obj.get("expression")
    if not isinstance(expr, str) or not expr.strip():
        issues.append("expression must be a non-empty string")
    inputs = expr_obj.get("inputs")
    declared_names: set[str] = set()
    if not isinstance(inputs, list):
        issues.append("inputs must be a list")
    else:
        for idx, item in enumerate(inputs):
            if not isinstance(item, Mapping) or not isinstance(item.get("name"), str):
                issues.append(f"inputs[{idx}] must have a string 'name'")
            else:
                declared_names.add(item["name"])
    output = expr_obj.get("output")
    if not isinstance(output, Mapping) or not isinstance(output.get("name"), str):
        issues.append("output must have a string 'name'")

    if isinstance(expr, str) and expr.strip():
        try:
            check_expression_safety(expr)
        except ExpressionError as exc:
            issues.append(f"expression whitelist violation: {exc}")
        try:
            tree = ast.parse(expr.strip(), mode="eval")
        except SyntaxError:
            pass
        else:
            free_names: set[str] = set()
            for node in ast.walk(tree):
                if isinstance(node, ast.Name) and not node.id.startswith("_"):
                    free_names.add(node.id)
            free_names -= ALLOWED_CALLS
            for name in sorted(free_names - declared_names):
                issues.append(f"expression uses undeclared input '{name}'")
    return issues
