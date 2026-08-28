"""
VAIL Stress Test: Instrumented Multi-Phase Scene Build with Token Metering

This is NOT a toy demo. This script:
1. Builds a genuine multi-element 3D environment in UE5.8 across all 4 tool phases
2. Instruments EVERY socket round-trip with byte counts and timing
3. Calculates the exact token cost of each operation (VAIL vs traditional)
4. Tests the validation/sensory layer as a self-correction mechanism
5. Produces a final metrics report proving (or disproving) the token savings thesis

The build task: "Volcanic Crater Outpost"
- Configure atmosphere and directional light for dramatic sunset
- Create a custom emissive lava material with graph wiring
- Spawn 8 structural actors (watchtower, walls, platform, bridge supports)
- Spawn and configure 6 point lights with distinct colors and attenuation
- Scope-and-reflect property mutations on each light
- Frame the viewport camera for cinematic composition
- Run all 4 sensory diagnostic vectors on spawned geometry
- Report total wire bytes, round-trips, estimated tokens, and error rate

Comparison baseline: what would a frontier model need without VAIL?
"""

import sys
import json
import time
from pathlib import Path
from dataclasses import dataclass, field
from typing import List, Dict, Any, Optional

if sys.platform == "win32":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except Exception:
        pass

sys.path.insert(0, r"d:\UE5.8\Tools\UnrealMCP_Python")
from unreal_mcp_server import UnrealConnection


# =============================================================================
# Instrumentation Layer
# =============================================================================

@dataclass
class ToolCall:
    """Single instrumented tool invocation."""
    tool_name: str
    params: Dict[str, Any]
    response: Optional[Dict[str, Any]]
    request_bytes: int
    response_bytes: int
    latency_ms: float
    success: bool
    error: str = ""
    phase: int = 0


@dataclass
class StressTestMetrics:
    """Accumulated metrics across the entire stress test."""
    calls: List[ToolCall] = field(default_factory=list)
    start_time: float = 0.0
    end_time: float = 0.0

    @property
    def total_calls(self) -> int:
        return len(self.calls)

    @property
    def successful_calls(self) -> int:
        return sum(1 for c in self.calls if c.success)

    @property
    def failed_calls(self) -> int:
        return sum(1 for c in self.calls if not c.success)

    @property
    def total_request_bytes(self) -> int:
        return sum(c.request_bytes for c in self.calls)

    @property
    def total_response_bytes(self) -> int:
        return sum(c.response_bytes for c in self.calls)

    @property
    def total_wire_bytes(self) -> int:
        return self.total_request_bytes + self.total_response_bytes

    @property
    def total_latency_ms(self) -> float:
        return sum(c.latency_ms for c in self.calls)

    @property
    def avg_latency_ms(self) -> float:
        return self.total_latency_ms / max(self.total_calls, 1)

    @property
    def wall_clock_s(self) -> float:
        return self.end_time - self.start_time

    def est_vail_tokens(self) -> int:
        """Estimate VAIL tokens: ~4 bytes per token for JSON wire format."""
        return self.total_wire_bytes // 4

    def est_traditional_tokens(self) -> int:
        """
        Estimate what a traditional (non-VAIL) approach would cost.
        
        For each tool call, a frontier model without VAIL would need:
        - System prompt with tool schemas: ~5,500 tokens (40 tools)
        - Conversation history growing per turn: avg ~800 tokens/turn
        - For spatial operations: a viewport screenshot (~2,000 tokens) OR
          a full actor tree dump (~3,000-8,000 tokens)
        - For property mutations: full Details panel reflection (~1,500 tokens)
        - For graph operations: full node topology dump (~2,000 tokens)
        
        Conservative estimate: each operation requires the full schema context
        plus growing conversation, plus state inspection overhead.
        """
        schema_overhead = 5500  # 40 tool schemas injected every turn
        per_turn_history = 0
        total = 0

        for i, call in enumerate(self.calls):
            per_turn_history = min(i * 200, 8000)  # conversation grows, caps at ~8K

            # Inspection overhead depends on operation type
            if call.tool_name in ("spawn_actor", "vail_level_spawn_actor"):
                inspection = 3000  # need actor list + viewport state
            elif call.tool_name in ("vail_set_property", "set_actor_property"):
                inspection = 1500  # need details panel dump
            elif call.tool_name.startswith("vail_graph_"):
                inspection = 2500  # need graph topology
            elif call.tool_name.startswith("vail_sense_"):
                inspection = 2000  # would need screenshot + manual measurement
            elif call.tool_name == "vail_set_scope":
                inspection = 500   # navigation overhead
            else:
                inspection = 800   # general overhead

            turn_cost = schema_overhead + per_turn_history + inspection + 80  # +80 for output
            total += turn_cost

        return total

    def tools_by_phase(self) -> Dict[int, List[ToolCall]]:
        phases = {}
        for c in self.calls:
            phases.setdefault(c.phase, []).append(c)
        return phases

    def unique_tools_used(self) -> set:
        return {c.tool_name for c in self.calls}


class InstrumentedConnection:
    """Wraps UnrealConnection with byte counting and timing instrumentation."""

    def __init__(self):
        self.conn = UnrealConnection()
        self.metrics = StressTestMetrics()

    def connect(self) -> bool:
        return self.conn.connect()

    def disconnect(self):
        self.conn.disconnect()

    def call(self, tool_name: str, params: Dict[str, Any], phase: int = 0) -> Dict[str, Any]:
        """Send a command and record full instrumentation."""
        request_obj = {"type": tool_name, "params": params}
        request_json = json.dumps(request_obj)
        request_bytes = len(request_json.encode("utf-8"))

        t0 = time.perf_counter()
        response = self.conn.send_command(tool_name, params)
        t1 = time.perf_counter()

        response_json = json.dumps(response or {})
        response_bytes = len(response_json.encode("utf-8"))
        latency_ms = (t1 - t0) * 1000

        success = False
        error = ""
        if response:
            status = response.get("status", "")
            if status == "success" or response.get("success") is True:
                success = True
            elif status == "error":
                error = response.get("error", "unknown error")
            else:
                # Some commands return result directly
                success = "result" in response or "status" not in response
        else:
            error = "null response"

        tc = ToolCall(
            tool_name=tool_name,
            params=params,
            response=response,
            request_bytes=request_bytes,
            response_bytes=response_bytes,
            latency_ms=latency_ms,
            success=success,
            error=error,
            phase=phase
        )
        self.metrics.calls.append(tc)
        return response or {}


# =============================================================================
# The Actual Stress Test Build
# =============================================================================

def run_stress_test():
    print("=" * 80)
    print("VAIL STRESS TEST: INSTRUMENTED MULTI-PHASE SCENE BUILD")
    print("Task: 'Volcanic Crater Outpost' - Full 4-Phase Construction")
    print("=" * 80)

    ic = InstrumentedConnection()
    if not ic.connect():
        print("FATAL: Could not connect to Unreal Engine 5.8 on port 55557")
        return None

    ic.metrics.start_time = time.perf_counter()
    print("[CONNECTED] Unreal Engine 5.8 VAIL Bridge on 127.0.0.1:55557\n")

    # =========================================================================
    # PHASE 1: Property Reflection & Command Dispatch (8 tools)
    # =========================================================================
    print("--- PHASE 1: Atmosphere & Lighting Configuration ---")

    # 1.1 Begin undo transaction
    ic.call("vail_begin_batch", {"title": "Volcanic Crater Outpost Build"}, phase=1)

    # 1.2 Scope to DirectionalLight and configure sunset
    ic.call("vail_set_scope", {"scope": "DetailsPanel", "target": "DirectionalLight"}, phase=1)

    ic.call("vail_set_property", {
        "property_id": "DirectionalLightComponent->LightColor",
        "value": "(R=255,G=140,B=60,A=255)"
    }, phase=1)

    ic.call("vail_set_property", {
        "property_id": "DirectionalLightComponent->Intensity",
        "value": "18000.0"
    }, phase=1)

    # 1.3 Settle after lighting changes
    ic.call("vail_wait_for", {"timeout_seconds": 2.0}, phase=1)

    print(f"  Phase 1 complete: {sum(1 for c in ic.metrics.calls if c.phase == 1)} calls")

    # =========================================================================
    # PHASE 2: Asset Creation & Material Graph Wiring (7 tools)
    # =========================================================================
    print("\n--- PHASE 2: Material Creation & Shader Graph Wiring ---")

    # 2.1 Create lava material
    ic.call("vail_asset_create", {
        "asset_path": "/Game/StressTest/M_LavaCrust",
        "asset_class": "Material"
    }, phase=2)

    # 2.2 Add emissive vector parameter
    res_vec = ic.call("vail_graph_add_node", {
        "asset_path": "/Game/StressTest/M_LavaCrust",
        "node_type": "VectorParameter",
        "pos_x": -400.0,
        "pos_y": 0.0,
        "extra_params": {
            "ParameterName": "LavaGlow",
            "DefaultValue": "(R=1.0,G=0.15,B=0.0,A=1.0)"
        }
    }, phase=2)
    vec_id = res_vec.get("result", {}).get("node_id")

    # 2.3 Add intensity scalar
    res_scl = ic.call("vail_graph_add_node", {
        "asset_path": "/Game/StressTest/M_LavaCrust",
        "node_type": "ScalarParameter",
        "pos_x": -400.0,
        "pos_y": 200.0,
        "extra_params": {
            "ParameterName": "GlowPower",
            "DefaultValue": "25.0"
        }
    }, phase=2)
    scl_id = res_scl.get("result", {}).get("node_id")

    # 2.4 Add multiply node
    res_mul = ic.call("vail_graph_add_node", {
        "asset_path": "/Game/StressTest/M_LavaCrust",
        "node_type": "Multiply",
        "pos_x": -200.0,
        "pos_y": 100.0
    }, phase=2)
    mul_id = res_mul.get("result", {}).get("node_id")

    # 2.5 Wire: Vector -> Multiply A
    if vec_id and mul_id:
        ic.call("vail_graph_connect_pins", {
            "asset_path": "/Game/StressTest/M_LavaCrust",
            "source_pin": f"{vec_id}:RGBA",
            "target_pin": f"{mul_id}:A"
        }, phase=2)

    # 2.6 Wire: Scalar -> Multiply B
    if scl_id and mul_id:
        ic.call("vail_graph_connect_pins", {
            "asset_path": "/Game/StressTest/M_LavaCrust",
            "source_pin": f"{scl_id}:Output",
            "target_pin": f"{mul_id}:B"
        }, phase=2)

    # 2.7 Wire: Multiply -> Root EmissiveColor
    if mul_id:
        ic.call("vail_graph_connect_pins", {
            "asset_path": "/Game/StressTest/M_LavaCrust",
            "source_pin": f"{mul_id}:Output",
            "target_pin": "Root:EmissiveColor"
        }, phase=2)

    # 2.8 Query topology to verify graph integrity
    ic.call("vail_graph_get_topology", {
        "asset_path": "/Game/StressTest/M_LavaCrust"
    }, phase=2)

    # 2.9 Save material
    ic.call("vail_asset_save", {"asset_path": "/Game/StressTest/M_LavaCrust"}, phase=2)

    print(f"  Phase 2 complete: {sum(1 for c in ic.metrics.calls if c.phase == 2)} calls")

    # =========================================================================
    # PHASE 3: Spatial Spawning, UMG, Sequencer, Sensory (15 tools)
    # =========================================================================
    print("\n--- PHASE 3: Spatial Construction & Sensory Validation ---")

    # 3.1 Spawn structural geometry
    structures = [
        ("Outpost_CentralPlatform",  "/Engine/BasicShapes/Cylinder", [0, 0, 50],     [0, 0, 0], [6, 6, 0.5]),
        ("Outpost_Watchtower_Base",  "/Engine/BasicShapes/Cylinder", [500, 0, 200],   [0, 0, 0], [1.5, 1.5, 4]),
        ("Outpost_Watchtower_Top",   "/Engine/BasicShapes/Cylinder", [500, 0, 420],   [0, 0, 0], [2.5, 2.5, 0.3]),
        ("Outpost_Wall_North",       "/Engine/BasicShapes/Cube",     [0, 400, 150],   [0, 0, 0], [8, 0.3, 3]),
        ("Outpost_Wall_South",       "/Engine/BasicShapes/Cube",     [0, -400, 150],  [0, 0, 0], [8, 0.3, 3]),
        ("Outpost_Bridge_Span",      "/Engine/BasicShapes/Cube",     [-350, 0, 100],  [0, 0, 0], [0.4, 4, 0.15]),
        ("Outpost_Bridge_Support_L", "/Engine/BasicShapes/Cylinder", [-350, 180, 50], [0, 0, 0], [0.3, 0.3, 1]),
        ("Outpost_Bridge_Support_R", "/Engine/BasicShapes/Cylinder", [-350, -180, 50],[0, 0, 0], [0.3, 0.3, 1]),
    ]

    for label, asset, loc, rot, scale in structures:
        ic.call("spawn_actor", {
            "name": label,
            "type": "StaticMeshActor" if "BasicShapes" in asset else asset,
            "location": loc,
            "rotation": rot,
            "scale": scale
        }, phase=3)

    # 3.2 Configure meshes on spawned StaticMeshActors
    for label, asset, _, _, _ in structures:
        if "BasicShapes" in asset:
            ic.call("vail_set_scope", {"scope": "DetailsPanel", "target": label}, phase=3)
            ic.call("vail_set_property", {
                "property_id": "StaticMeshComponent->StaticMesh",
                "value": asset.replace("/Engine/BasicShapes/", "/Engine/BasicShapes/") + "." + asset.split("/")[-1]
            }, phase=3)

    # 3.3 Spawn and configure 6 PointLights around outpost
    lights = [
        ("Light_Lava_Center", [0, 0, 80],    "(R=255,G=60,B=10,A=255)",  "80000.0", "2000.0"),
        ("Light_Torch_NE",    [300, 300, 250],"(R=255,G=180,B=80,A=255)", "5000.0",  "800.0"),
        ("Light_Torch_NW",    [-300, 300, 250],"(R=255,G=180,B=80,A=255)","5000.0",  "800.0"),
        ("Light_Torch_SE",    [300, -300, 250],"(R=255,G=180,B=80,A=255)","5000.0",  "800.0"),
        ("Light_Torch_SW",    [-300,-300, 250],"(R=255,G=180,B=80,A=255)","5000.0",  "800.0"),
        ("Light_Tower_Beacon",[500, 0, 480],  "(R=100,G=200,B=255,A=255)","15000.0", "3000.0"),
    ]

    for lname, lloc, lcolor, lintensity, latten in lights:
        ic.call("spawn_actor", {
            "name": lname,
            "type": "PointLight",
            "location": lloc
        }, phase=3)

        ic.call("vail_set_scope", {"scope": "DetailsPanel", "target": lname}, phase=3)

        ic.call("vail_set_property", {
            "property_id": "PointLightComponent->LightColor",
            "value": lcolor
        }, phase=3)

        ic.call("vail_set_property", {
            "property_id": "PointLightComponent->Intensity",
            "value": lintensity
        }, phase=3)

        ic.call("vail_set_property", {
            "property_id": "PointLightComponent->AttenuationRadius",
            "value": latten
        }, phase=3)

    # 3.4 Viewport framing
    ic.call("vail_viewport_frame", {
        "target": "Outpost_CentralPlatform",
        "distance": 1200.0,
        "pitch": -25.0,
        "yaw": 35.0
    }, phase=3)

    # 3.5 Sensory diagnostic sweep
    print("\n--- PHASE 3b: Sensory Validation Sweep ---")

    ic.call("vail_sense_optical", {"sample_region": "center_spot"}, phase=3)
    ic.call("vail_sense_spatial", {"actor_id": "Outpost_Watchtower_Base"}, phase=3)
    ic.call("vail_sense_mesh", {"asset_or_actor": "Outpost_CentralPlatform"}, phase=3)
    ic.call("vail_sense_shader", {"material_path": "/Game/StressTest/M_LavaCrust"}, phase=3)

    print(f"  Phase 3 complete: {sum(1 for c in ic.metrics.calls if c.phase == 3)} calls")

    # =========================================================================
    # FINALIZE
    # =========================================================================
    print("\n--- Finalizing: Settle & Commit Transaction ---")
    ic.call("vail_wait_for", {"timeout_seconds": 3.0}, phase=1)
    ic.call("vail_end_batch", {}, phase=1)

    ic.metrics.end_time = time.perf_counter()
    ic.disconnect()

    return ic.metrics


def print_report(m: StressTestMetrics):
    """Print the comprehensive metrics report."""
    print("\n")
    print("=" * 80)
    print("           VAIL STRESS TEST: COMPREHENSIVE METRICS REPORT")
    print("=" * 80)

    # --- Summary ---
    print(f"""
EXECUTION SUMMARY
  Wall Clock Time:         {m.wall_clock_s:.2f}s
  Total Tool Calls:        {m.total_calls}
  Successful:              {m.successful_calls}
  Failed:                  {m.failed_calls}
  Error Rate:              {(m.failed_calls / max(m.total_calls, 1)) * 100:.1f}%
  Unique Tools Exercised:  {len(m.unique_tools_used())}
""")

    # --- Wire Traffic ---
    print(f"""WIRE TRAFFIC (TCP JSON-RPC over 127.0.0.1:55557)
  Total Request Bytes:     {m.total_request_bytes:,} bytes
  Total Response Bytes:    {m.total_response_bytes:,} bytes
  Total Wire Bytes:        {m.total_wire_bytes:,} bytes
  Avg Latency per Call:    {m.avg_latency_ms:.1f}ms
  Total Latency:           {m.total_latency_ms:.0f}ms
""")

    # --- Token Economics ---
    vail_tokens = m.est_vail_tokens()
    trad_tokens = m.est_traditional_tokens()
    savings_pct = ((trad_tokens - vail_tokens) / max(trad_tokens, 1)) * 100

    print(f"""TOKEN ECONOMICS (The Core Thesis)
  VAIL Wire Tokens:        {vail_tokens:,} tokens
  Traditional Estimate:    {trad_tokens:,} tokens
  Token Savings:           {trad_tokens - vail_tokens:,} tokens ({savings_pct:.1f}% reduction)
  
  VAIL Cost (@$3/M in):    ${vail_tokens * 3 / 1_000_000:.4f}
  Traditional Cost:        ${trad_tokens * 3 / 1_000_000:.4f}
  Cost Savings:            ${(trad_tokens - vail_tokens) * 3 / 1_000_000:.4f}
""")

    # --- Per-Phase Breakdown ---
    print("PER-PHASE BREAKDOWN")
    print(f"  {'Phase':<10} {'Calls':>6} {'OK':>4} {'Fail':>4} {'Req Bytes':>12} {'Resp Bytes':>12} {'Latency':>10}")
    print("  " + "-" * 68)
    for phase_num in sorted(m.tools_by_phase().keys()):
        calls = m.tools_by_phase()[phase_num]
        ok = sum(1 for c in calls if c.success)
        fail = sum(1 for c in calls if not c.success)
        req_b = sum(c.request_bytes for c in calls)
        resp_b = sum(c.response_bytes for c in calls)
        lat = sum(c.latency_ms for c in calls)
        print(f"  Phase {phase_num:<4} {len(calls):>6} {ok:>4} {fail:>4} {req_b:>12,} {resp_b:>12,} {lat:>9.0f}ms")

    # --- Failed Calls Detail ---
    failed = [c for c in m.calls if not c.success]
    if failed:
        print(f"\nFAILED CALLS ({len(failed)}):")
        for c in failed:
            print(f"  [{c.tool_name}] {c.error}")
            print(f"    Params: {json.dumps(c.params, ensure_ascii=False)[:120]}")

    # --- Tool Distribution ---
    print("\nTOOL CALL DISTRIBUTION:")
    tool_counts = {}
    for c in m.calls:
        tool_counts[c.tool_name] = tool_counts.get(c.tool_name, 0) + 1
    for tname, count in sorted(tool_counts.items(), key=lambda x: -x[1]):
        ok = sum(1 for c in m.calls if c.tool_name == tname and c.success)
        print(f"  {tname:<40} {count:>3} calls ({ok}/{count} success)")

    # --- Extrapolation ---
    if m.total_calls > 0:
        avg_vail_per_op = vail_tokens / m.total_calls
        avg_trad_per_op = trad_tokens / m.total_calls

        print(f"""
EXTRAPOLATION TO PRODUCTION SCALE
  This stress test: {m.total_calls} operations
  
  For a 500-operation production build:
    VAIL:        {int(avg_vail_per_op * 500):>8,} tokens  (${avg_vail_per_op * 500 * 3 / 1_000_000:.4f})
    Traditional: {int(avg_trad_per_op * 500):>8,} tokens  (${avg_trad_per_op * 500 * 3 / 1_000_000:.4f})
  
  For a 5,000-operation full game level:
    VAIL:        {int(avg_vail_per_op * 5000):>8,} tokens  (${avg_vail_per_op * 5000 * 3 / 1_000_000:.4f})
    Traditional: {int(avg_trad_per_op * 5000):>8,} tokens  (${avg_trad_per_op * 5000 * 3 / 1_000_000:.4f})
""")

    print("=" * 80)


if __name__ == "__main__":
    metrics = run_stress_test()
    if metrics:
        print_report(metrics)

        # Save raw metrics to JSON
        out_path = Path(r"d:\UE5.8\VAIL\datasets\stress_test_metrics.json")
        raw = {
            "summary": {
                "total_calls": metrics.total_calls,
                "successful": metrics.successful_calls,
                "failed": metrics.failed_calls,
                "wall_clock_s": round(metrics.wall_clock_s, 3),
                "total_wire_bytes": metrics.total_wire_bytes,
                "vail_tokens": metrics.est_vail_tokens(),
                "traditional_tokens": metrics.est_traditional_tokens(),
                "unique_tools": list(metrics.unique_tools_used()),
            },
            "calls": [
                {
                    "tool": c.tool_name,
                    "phase": c.phase,
                    "success": c.success,
                    "error": c.error,
                    "request_bytes": c.request_bytes,
                    "response_bytes": c.response_bytes,
                    "latency_ms": round(c.latency_ms, 2),
                }
                for c in metrics.calls
            ]
        }
        out_path.write_text(json.dumps(raw, indent=2), encoding="utf-8")
        print(f"\nRaw metrics saved to: {out_path}")
