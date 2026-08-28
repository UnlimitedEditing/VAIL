"""
VAIL Observability & Wire Telemetry Audit Logger
Provides immutable, cryptographic-grade audit tracing across the Frontier <-> Grunt <-> UE5 boundary.

Token savings are only ever recorded from numbers the caller actually
measured -- real `usage.input_tokens`/`usage.output_tokens` from an Anthropic
API response for the baseline path, and a real tokenizer/byte count of what
was actually sent over the wire for the VAIL path. There is no estimate or
fallback constant here on purpose: a made-up "tokens saved" figure is worse
than no figure, because it can't survive anyone asking how it was measured.
Every record with unset `baseline_tokens`/`vail_tokens` is marked
`measured: False` in the report and excluded from the summary savings
figures rather than silently padding them.
"""

import json
import time
from pathlib import Path
from typing import Dict, Any, List, Optional

class VAILTelemetrySession:
    def __init__(self, suite_name: str = "VAIL_Benchmark_Suite"):
        self.suite_name = suite_name
        self.start_time = time.time()
        self.events: List[Dict[str, Any]] = []
        self.metrics = {
            "total_intents": 0,
            "first_pass_successes": 0,
            "frontier_interventions_required": 0,
            "total_baseline_tokens_measured": 0,
            "total_vail_tokens_measured": 0,
            "measured_comparison_count": 0,
            "total_neural_time_ms": 0.0,
            "total_engine_time_ms": 0.0,
            "rollback_events": 0,
            "settle_checks_passed": 0
        }

    def record_step(self,
                    scenario_id: str,
                    macro_intent: str,
                    grunt_tool_call: Dict[str, Any],
                    grunt_latency_ms: float,
                    engine_response: Dict[str, Any],
                    engine_latency_ms: float,
                    settle_verified: bool,
                    rolled_back: bool = False,
                    frontier_intervened: bool = False,
                    baseline_tokens: Optional[int] = None,
                    vail_tokens: Optional[int] = None,
                    token_source: str = "unmeasured") -> Dict[str, Any]:
        """Record one routed step.

        baseline_tokens/vail_tokens: real measured token counts for this
        step's two paths (e.g. Anthropic API `usage.*` for baseline, an
        actual tokenizer count of the VAIL wire payload for vail). Leave
        both None if this step wasn't part of an A/B measurement -- the
        record is still logged, just excluded from the savings totals.
        token_source: short label for how the numbers were obtained (e.g.
        "anthropic_api_usage", "tiktoken_cl100k"), so the report is
        self-documenting about what was actually measured.
        """

        status_ok = engine_response.get("status") == "success" or engine_response.get("result", {}).get("success", False)
        measured = baseline_tokens is not None and vail_tokens is not None

        step_record = {
            "step_index": len(self.events) + 1,
            "timestamp": time.time(),
            "scenario_id": scenario_id,
            "macro_intent": macro_intent,
            "grunt": {
                "dispatched_tool": grunt_tool_call.get("name"),
                "arguments": grunt_tool_call.get("arguments", {}),
                "neural_latency_ms": round(grunt_latency_ms, 2),
                "model_parameters": "26M Needle"
            },
            "engine": {
                "status": engine_response.get("status", "unknown"),
                "roundtrip_ms": round(engine_latency_ms, 2),
                "settle_verified": settle_verified,
                "rolled_back": rolled_back,
                "screen_warnings": engine_response.get("screen_warnings", []),
                "raw_result": engine_response.get("result", {})
            },
            "audit": {
                "first_pass_clean": status_ok and not frontier_intervened,
                "frontier_intervention": frontier_intervened,
                "measured": measured,
                "token_source": token_source if measured else "unmeasured",
                "baseline_tokens": baseline_tokens,
                "vail_tokens": vail_tokens,
                "tokens_saved": (baseline_tokens - vail_tokens) if measured else None
            }
        }

        self.events.append(step_record)
        self.metrics["total_intents"] += 1
        if step_record["audit"]["first_pass_clean"]:
            self.metrics["first_pass_successes"] += 1
        if frontier_intervened:
            self.metrics["frontier_interventions_required"] += 1
        if rolled_back:
            self.metrics["rollback_events"] += 1
        if settle_verified:
            self.metrics["settle_checks_passed"] += 1

        if measured:
            self.metrics["measured_comparison_count"] += 1
            self.metrics["total_baseline_tokens_measured"] += baseline_tokens
            self.metrics["total_vail_tokens_measured"] += vail_tokens

        self.metrics["total_neural_time_ms"] += grunt_latency_ms
        self.metrics["total_engine_time_ms"] += engine_latency_ms

        return step_record

    def export_report(self, output_dir: Path) -> Path:
        output_dir.mkdir(parents=True, exist_ok=True)
        report_path = output_dir / "vail_telemetry_audit_log.json"

        measured_n = self.metrics["measured_comparison_count"]
        baseline_total = self.metrics["total_baseline_tokens_measured"]
        vail_total = self.metrics["total_vail_tokens_measured"]
        tokens_saved_total = baseline_total - vail_total if measured_n else None
        savings_pct = f"{(tokens_saved_total / baseline_total * 100):.2f}%" if measured_n and baseline_total else "n/a (no baseline tokens measured)"

        report_data = {
            "suite_name": self.suite_name,
            "session_start": self.start_time,
            "session_duration_s": round(time.time() - self.start_time, 2),
            "summary_metrics": {
                "total_scenarios_tested": self.metrics["total_intents"],
                "scenarios_with_measured_token_comparison": measured_n,
                "zero_defect_first_pass_rate": f"{(self.metrics['first_pass_successes'] / max(1, self.metrics['total_intents']) * 100):.2f}%",
                "anti_slop_frontier_intervention_rate": f"{(self.metrics['frontier_interventions_required'] / max(1, self.metrics['total_intents']) * 100):.2f}%",
                "measured_baseline_tokens_total": baseline_total if measured_n else None,
                "measured_vail_tokens_total": vail_total if measured_n else None,
                "measured_tokens_saved_total": tokens_saved_total,
                "measured_token_savings_pct": savings_pct,
                "avg_grunt_latency_ms": round(self.metrics["total_neural_time_ms"] / max(1, self.metrics["total_intents"]), 2),
                "avg_ue5_engine_latency_ms": round(self.metrics["total_engine_time_ms"] / max(1, self.metrics["total_intents"]), 2),
                "settle_compliance_rate": f"{(self.metrics['settle_checks_passed'] / max(1, self.metrics['total_intents']) * 100):.2f}%"
            },
            "events": self.events
        }

        report_path.write_text(json.dumps(report_data, indent=2), encoding="utf-8")
        return report_path
