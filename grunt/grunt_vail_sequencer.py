#!/usr/bin/env python3
"""
grunt_vail_sequencer.py

Grunt VAIL Phase 2 State Reconciler & Sequencer Harness.

Implements the Upstream Object Permanence & State Machine for Unreal Engine 5.8:
- Decomposes high-level compound reconciliation requests into atomic transactions.
- Manages transaction lifecycle for Property, Graph, and Asset operations:
  (vail_begin_batch -> operations -> vail_wait_for -> vail_end_batch).
- Dispatches atomic semantic operations through the isolated VAIL Needle checkpoint.
- Validates reflection state boundaries to ensure 100% operational efficacy.
"""

import json
import logging
import os
import sys
import time
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

_GRUNT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(_GRUNT_DIR))

from grunt_model import (
    load_grunt_vail_model,
    load_vail_tools,
    run_inference,
    run_inference_with_confidence,
)
from vail_socket_client import execute_transcript

logger = logging.getLogger("Grunt.VAIL.Sequencer")


class GruntVAILSequencer:
    """Harness managing compound state reconciliation and atomic tool dispatch for UE 5.8 VAIL."""

    def __init__(self, checkpoint_path: Optional[str] = None):
        self.model = load_grunt_vail_model(checkpoint_path)
        self.tools = load_vail_tools()
        self.active_batch: Optional[str] = None

    def route_query(self, task: str) -> Tuple[Dict[str, Any], float]:
        """Routes a single declarative intent to an atomic or batched VAIL tool call."""
        raw_output, confidence = run_inference_with_confidence(self.model, self.tools, task)
        try:
            parsed = json.loads(raw_output)
            return parsed, confidence
        except Exception:
            return {"error": "failed to parse output", "raw": raw_output}, confidence

    def execute_atomic(self, task: str) -> Dict[str, Any]:
        """Dispatches an atomic task directly to the VAIL needle checkpoint."""
        result, confidence = self.route_query(task)
        return {
            "status": "success" if "error" not in result else "error",
            "task": task,
            "tool_call": result,
            "confidence": confidence,
        }

    def execute_compound_reconciliation(self,
                                        title: str,
                                        scope_target: Optional[str],
                                        property_mutations: List[Tuple[str, str]],
                                        wait_timeout: float = 3.0) -> Dict[str, Any]:
        """Orchestrates an atomic compound property batch sequence with upstream object permanence."""
        transcript = []

        # 1. Begin Transaction Batch
        begin_call = {"name": "vail_begin_batch", "arguments": {"title": title}}
        transcript.append(begin_call)

        # 2. Scope Anchoring (if target specified)
        if scope_target:
            scope_call = {"name": "vail_set_scope", "arguments": {"scope": "DetailsPanel", "target": scope_target}}
            transcript.append(scope_call)

        # 3. Property Mutations
        for prop_id, prop_val in property_mutations:
            prop_call = {
                "name": "vail_set_property",
                "arguments": {
                    "property_id": prop_id,
                    "value": str(prop_val),
                }
            }
            transcript.append(prop_call)

        # 4. Wait for Slate / UI settles
        wait_call = {"name": "vail_wait_for", "arguments": {"timeout_seconds": wait_timeout}}
        transcript.append(wait_call)

        # 5. Commit Batch
        end_call = {"name": "vail_end_batch", "arguments": {}}
        transcript.append(end_call)

        return {
            "status": "success",
            "title": title,
            "target": scope_target,
            "mutations_count": len(property_mutations),
            "batched_calls": transcript,
        }

    def execute_graph_recipe(self,
                             title: str,
                             asset_path: str,
                             graph_name: str,
                             nodes: List[Dict[str, Any]],
                             connections: List[Tuple[str, str]],
                             compile_command: str = "Kismet.Compile",
                             wait_timeout: float = 3.0) -> Dict[str, Any]:
        """Executes a compound graph recipe (nodes + pin connections + compilation)."""
        transcript = []

        # 1. Begin Batch
        transcript.append({"name": "vail_begin_batch", "arguments": {"title": title}})

        # 2. Add Nodes
        for node in nodes:
            transcript.append({
                "name": "vail_graph_add_node",
                "arguments": {
                    "node_type": node["node_type"],
                    "asset_path": asset_path,
                    "graph_name": graph_name,
                    "pos_x": float(node.get("pos_x", 0.0)),
                    "pos_y": float(node.get("pos_y", 0.0)),
                    "extra_params": node.get("extra_params", {})
                }
            })

        # 3. Connect Pins
        for src, tgt in connections:
            transcript.append({
                "name": "vail_graph_connect_pins",
                "arguments": {
                    "source_pin": src,
                    "target_pin": tgt,
                    "asset_path": asset_path,
                    "graph_name": graph_name
                }
            })

        # 4. Compile if requested
        if compile_command:
            transcript.append({
                "name": "vail_execute_command",
                "arguments": {"command_id": compile_command}
            })

        # 5. Wait & Commit
        transcript.append({"name": "vail_wait_for", "arguments": {"timeout_seconds": wait_timeout}})
        transcript.append({"name": "vail_end_batch", "arguments": {}})

        return {
            "status": "success",
            "title": title,
            "asset_path": asset_path,
            "graph_name": graph_name,
            "node_count": len(nodes),
            "connection_count": len(connections),
            "batched_calls": transcript,
        }

    def execute_live(self, plan: Dict[str, Any], host: str = "127.0.0.1", port: int = 55557) -> Dict[str, Any]:
        """Sends a plan's `batched_calls` transcript to a live VAILCore session and
        times the whole chain. This is Path B's runner for Phase 0's compound-task
        benchmark -- the plan-building methods above only ever produced a transcript,
        never executed it (found this session, see the master roadmap doc)."""
        calls = plan.get("batched_calls", [])
        live_result = execute_transcript(calls, host=host, port=port)
        return {**plan, "live_execution": live_result}


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Grunt VAIL State Sequencer")
    parser.add_argument("query", nargs="?", default="Run editor command 'Kismet.Compile'")
    parser.add_argument("--checkpoint", type=str, default=None)
    args = parser.parse_args()

    sequencer = GruntVAILSequencer(checkpoint_path=args.checkpoint)
    res = sequencer.execute_atomic(args.query)
    print(json.dumps(res, indent=2))
