"""
GruntDispatcher: macro-first tool routing for VAIL.

Deterministic macros (grunt/macros/library.py) are tried first -- exact
regex match, exact tool call sequence, no model inference, no way to
hallucinate a bad tool name or argument shape. Only intents that don't match
any macro fall through to GruntVAIL.route(), which asks the 26M Needle model
to generate a single atomic tool call. This mirrors what actually turned out
to work in practice: scripts for the known-shape tasks, a small model only
for genuinely novel single-property lookups it's actually sized for.
"""

from typing import Any, Dict, List, Optional

from grunt_vail import GruntVAIL
from macros import match_macro


class GruntDispatcher:
    def __init__(self, checkpoint_path: Optional[str] = None, tools_dir: Optional[str] = None, lazy_model: bool = True):
        self._checkpoint_path = checkpoint_path
        self._tools_dir = tools_dir
        self._grunt: Optional[GruntVAIL] = None if lazy_model else GruntVAIL(checkpoint_path, tools_dir)

    def _neural_router(self) -> GruntVAIL:
        # Loaded on first actual fallback rather than at construction time --
        # a macro-only session (the common case once the library covers most
        # real usage) never pays the checkpoint-load cost at all.
        if self._grunt is None:
            self._grunt = GruntVAIL(self._checkpoint_path, self._tools_dir)
        return self._grunt

    def dispatch(self, task: str) -> Dict[str, Any]:
        """Route a natural-language intent to a tool call sequence.

        Returns:
            {
              "path": "macro" | "neural" | "unresolved",
              "macro_id": str | None,
              "calls": [ {"name": ..., "arguments": {...}}, ... ],
            }
        """
        macro_result = match_macro(task)
        if macro_result is not None:
            return {"path": "macro", "macro_id": macro_result["macro_id"], "calls": macro_result["calls"]}

        neural_result = self._neural_router().route(task)
        if "error" in neural_result:
            return {"path": "unresolved", "macro_id": None, "calls": [], "error": neural_result.get("error"), "raw": neural_result.get("raw")}

        return {"path": "neural", "macro_id": None, "calls": [neural_result]}
