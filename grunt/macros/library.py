"""
Deterministic macro library for the Grunt VAIL router.

The 26M Needle model is reliable at simple lookups but not at free-form
generation of multi-step tool call sequences -- it has no way to guarantee
valid JSON, valid tool names, or the right argument shape for anything
beyond a single atomic call. Rather than asking it to "reason" its way to a
tool call, each macro below is a fixed Python function: a regex trigger
extracts slots from the intent text, and a builder turns those slots into
an exact, pre-validated sequence of VAIL tool calls. No model inference
happens for anything matched here -- match + build is microseconds, not a
26M-parameter forward pass, so it's also strictly faster and cheaper than
the neural path for the intents it covers.

`match_macro(task)` returns None if nothing matches, so the caller (see
grunt/dispatcher.py) can fall back to GruntVAIL.route() for genuinely novel
intents -- the fallback path this library was built to shrink, not replace
entirely.
"""

import re
from typing import Any, Callable, Dict, List, Optional, Pattern


class Macro:
    def __init__(self, macro_id: str, pattern: Pattern, builder: Callable[[re.Match], List[Dict[str, Any]]]):
        self.macro_id = macro_id
        self.pattern = pattern
        self.builder = builder

    def try_match(self, task: str) -> Optional[List[Dict[str, Any]]]:
        m = self.pattern.match(task.strip())
        if not m:
            return None
        return self.builder(m)


def _set_scope(scope: str, target: str) -> Dict[str, Any]:
    return {"name": "vail_set_scope", "arguments": {"scope": scope, "target": target}}


def _set_property(property_id: str, value: str) -> Dict[str, Any]:
    return {"name": "vail_set_property", "arguments": {"property_id": property_id, "value": value}}


def _wait_for(timeout_seconds: float = 3.0) -> Dict[str, Any]:
    return {"name": "vail_wait_for", "arguments": {"timeout_seconds": timeout_seconds}}


def _batch(title: str, calls: List[Dict[str, Any]]) -> List[Dict[str, Any]]:
    return [{"name": "vail_begin_batch", "arguments": {"title": title}}, *calls, _wait_for(), {"name": "vail_end_batch", "arguments": {}}]


# -- Macro definitions -------------------------------------------------------
# Each pattern is deliberately narrow. A macro should only claim an intent it
# can build with 100% certainty; anything even slightly ambiguous should fall
# through to the neural router (or, if that also fails, to the frontier
# model) rather than have a macro guess.

MACROS: List[Macro] = [
    Macro(
        "set_scope",
        re.compile(r"^set (?:active )?scope to target '(?P<target>[^']+)'(?: in (?P<scope>\w+))?$", re.IGNORECASE),
        lambda m: [_set_scope(m.group("scope") or "DetailsPanel", m.group("target"))],
    ),
    Macro(
        "set_single_property",
        re.compile(r"^set property '(?P<prop>[^']+)' to '(?P<value>[^']+)'$", re.IGNORECASE),
        lambda m: [_set_property(m.group("prop"), m.group("value"))],
    ),
    Macro(
        "wait_for_settle",
        re.compile(r"^wait for editor to settle(?: with timeout (?P<timeout>[\d.]+) seconds)?$", re.IGNORECASE),
        lambda m: [_wait_for(float(m.group("timeout")) if m.group("timeout") else 3.0)],
    ),
    Macro(
        "move_actor",
        re.compile(
            r"^move '(?P<actor>[^']+)' to (?:x=)?(?P<x>-?[\d.]+),?\s*(?:y=)?(?P<y>-?[\d.]+),?\s*(?:z=)?(?P<z>-?[\d.]+)$",
            re.IGNORECASE,
        ),
        lambda m: _batch(
            f"Move {m.group('actor')}",
            [
                _set_scope("DetailsPanel", m.group("actor")),
                _set_property("RelativeLocation", f"X={m.group('x')},Y={m.group('y')},Z={m.group('z')}"),
            ],
        ),
    ),
    Macro(
        "set_actor_color",
        re.compile(
            r"^set '(?P<actor>[^']+)' color to (?:r=)?(?P<r>[\d.]+),?\s*(?:g=)?(?P<g>[\d.]+),?\s*(?:b=)?(?P<b>[\d.]+)$",
            re.IGNORECASE,
        ),
        lambda m: _batch(
            f"Recolor {m.group('actor')}",
            [
                _set_scope("DetailsPanel", m.group("actor")),
                _set_property(
                    "LightColor",
                    f"(R={m.group('r')},G={m.group('g')},B={m.group('b')},A=1.000000)",
                ),
            ],
        ),
    ),
    Macro(
        "rename_actor",
        re.compile(r"^rename '(?P<actor>[^']+)' to '(?P<new_name>[^']+)'$", re.IGNORECASE),
        lambda m: _batch(
            f"Rename {m.group('actor')}",
            [
                _set_scope("DetailsPanel", m.group("actor")),
                {"name": "vail_execute_command", "arguments": {"command_id": f"LevelEditor.RenameActor:{m.group('new_name')}"}},
            ],
        ),
    ),
    Macro(
        "save_and_compile",
        re.compile(r"^(save and compile|compile and save)$", re.IGNORECASE),
        lambda m: [
            {"name": "vail_execute_command", "arguments": {"command_id": "Kismet.Compile"}},
            _wait_for(5.0),
            {"name": "vail_execute_command", "arguments": {"command_id": "MainFrame.SaveAll"}},
        ],
    ),
]


def match_macro(task: str) -> Optional[Dict[str, Any]]:
    """Try every macro in order; return the first match as a routed result, or None."""
    for macro in MACROS:
        calls = macro.try_match(task)
        if calls is not None:
            return {"macro_id": macro.macro_id, "calls": calls}
    return None
