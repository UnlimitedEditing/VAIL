"""
Grunt VAIL: High-Speed Local UI State Reconciler and Tool Router for Unreal Engine 5.8.
Redistributable on-device engine powered by 26M Needle 1 architecture.
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

_ROOT = Path(__file__).resolve().parent
_CKPT = _ROOT / "checkpoints" / "grunt_needle_vail_master.pkl"
_TOOLS_DIR = _ROOT / "tools"
_ENGINE_DIR = _ROOT / "needle_engine"

if str(_ENGINE_DIR) not in sys.path:
    sys.path.insert(0, str(_ENGINE_DIR))

from architecture import SimpleAttentionNetwork, TransformerConfig
from run import load_checkpoint, generate
from tokenizer import NeedleTokenizer


def flatten_tool(tool: dict) -> dict:
    params = tool.get("parameters", {})
    props = params.get("properties", {}) if isinstance(params, dict) else {}
    required = set(params.get("required", [])) if isinstance(params, dict) else set()
    flat_params = {}
    for key, schema in props.items():
        flat = dict(schema) if isinstance(schema, dict) else {}
        flat["required"] = key in required
        flat_params[key] = flat
    return {
        "name": tool["name"],
        "description": tool.get("description", ""),
        "parameters": flat_params,
    }


class GruntVAIL:
    """Zero-setup standalone Grunt VAIL State Reconciler and Tool Router."""

    def __init__(self, checkpoint_path: Optional[str] = None, tools_dir: Optional[str] = None):
        self.checkpoint_path = Path(checkpoint_path) if checkpoint_path else _CKPT
        if not self.checkpoint_path.exists():
            raise FileNotFoundError(f"Grunt VAIL checkpoint not found at {self.checkpoint_path}")

        self.tools_dir = Path(tools_dir) if tools_dir else _TOOLS_DIR
        self.tools = self._load_tools()
        self.flat_tools_json = json.dumps([flatten_tool(t) for t in self.tools])

        self.params, self.config = load_checkpoint(str(self.checkpoint_path))
        cfg = TransformerConfig(**self.config) if isinstance(self.config, dict) else self.config
        self.model = SimpleAttentionNetwork(cfg)
        
        tok_model_path = _ENGINE_DIR / "tokenizer" / "needle.model"
        self.tokenizer = NeedleTokenizer(model_path=str(tok_model_path))

    def _load_tools(self) -> List[Dict[str, Any]]:
        tools = []
        if self.tools_dir.exists():
            for p in sorted(self.tools_dir.glob("*.json")):
                try:
                    tools.append(json.loads(p.read_text(encoding="utf-8")))
                except Exception:
                    pass
        return tools

    def route(self, task: str) -> Dict[str, Any]:
        """Translates declarative natural language intent into an exact atomic VAIL tool call."""
        raw = generate(
            self.model,
            self.params,
            self.tokenizer,
            query=task,
            tools=self.flat_tools_json,
            stream=False,
            constrained=True,
            max_gen_len=512,
        )
        try:
            parsed = json.loads(raw)
            if isinstance(parsed, list):
                return parsed[0] if len(parsed) == 1 else {"error": "ambiguous", "candidates": parsed}
            return parsed
        except Exception:
            return {"error": "unparseable", "raw": raw}

    def reconcile_compound(self,
                           title: str,
                           scope_target: Optional[str],
                           property_mutations: List[Tuple[str, str]],
                           wait_timeout: float = 3.0) -> Dict[str, Any]:
        """Executes upstream state machine batch for complex mutations."""
        calls = [
            {"name": "vail_begin_batch", "arguments": {"title": title}}
        ]
        if scope_target:
            calls.append({"name": "vail_set_scope", "arguments": {"scope": "DetailsPanel", "target": scope_target}})
        for prop_id, prop_val in property_mutations:
            calls.append({"name": "vail_set_property", "arguments": {"property_id": prop_id, "value": str(prop_val)}})
        calls.append({"name": "vail_wait_for", "arguments": {"timeout_seconds": wait_timeout}})
        calls.append({"name": "vail_end_batch", "arguments": {}})

        return {
            "status": "success",
            "title": title,
            "target": scope_target,
            "batched_calls": calls,
        }


def main():
    parser = argparse.ArgumentParser(description="Grunt VAIL CLI Router")
    parser.add_argument("query", nargs="?", default=None, help="User intent or command description")
    parser.add_argument("--checkpoint", type=str, default=None)
    args = parser.parse_args()

    user_query = args.query
    if not user_query:
        user_query = sys.stdin.read().strip()

    if not user_query:
        sys.exit("Error: No query provided.")

    agent = GruntVAIL(checkpoint_path=args.checkpoint)
    res = agent.route(user_query)
    print(json.dumps(res))


if __name__ == "__main__":
    main()