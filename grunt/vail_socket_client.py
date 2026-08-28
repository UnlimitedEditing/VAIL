#!/usr/bin/env python3
"""Robust TCP JSON-RPC client for a live VAILCore session (127.0.0.1:55557).

Single-recv() reads silently mis-parse larger responses (found live this
session on vail_get_tree against a SkeletalMeshActor) -- this loops until the
buffered bytes parse as JSON or the socket times out/closes.
"""
import json
import socket
import time
from typing import Any, Dict, Optional


def send_vail_cmd(
    tool_name: str,
    arguments: Optional[Dict[str, Any]] = None,
    host: str = "127.0.0.1",
    port: int = 55557,
    timeout: float = 5.0,
) -> Dict[str, Any]:
    """Sends one VAIL command and returns the parsed JSON response.

    On a socket timeout or connection error, returns
    {"status": "error", "error": "<reason>"} rather than raising -- callers
    executing a multi-step transcript need to keep the chain's error/timing
    accounting simple rather than wrapping every call in a try/except.
    """
    payload = {"type": tool_name, "params": arguments or {}}
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(timeout)
        s.connect((host, port))
        s.sendall(json.dumps(payload).encode("utf-8"))

        chunks = []
        while True:
            try:
                chunk = s.recv(65536)
            except socket.timeout:
                break
            if not chunk:
                break
            chunks.append(chunk)
            try:
                json.loads(b"".join(chunks).decode("utf-8"))
                break
            except (json.JSONDecodeError, UnicodeDecodeError):
                continue
        s.close()
        return json.loads(b"".join(chunks).decode("utf-8"))
    except (OSError, socket.timeout) as exc:
        return {"status": "error", "error": f"socket error: {exc}"}
    except json.JSONDecodeError as exc:
        return {"status": "error", "error": f"unparseable response: {exc}"}


def execute_transcript(calls: list, host: str = "127.0.0.1", port: int = 55557) -> Dict[str, Any]:
    """Executes an ordered list of {"name", "arguments"} calls over one session,
    timing the whole chain. Stops on the first error past vail_begin_batch so a
    failed batch doesn't silently continue mutating state.

    This is Path B's actual runner for Phase 0's compound-task benchmark
    (see docs/vail_grunt_master_handoff_and_roadmap.md §3 Phase 0 task 4) --
    grunt_vail_sequencer.py builds the transcript, this sends it.
    """
    start = time.perf_counter()
    results = []
    in_batch = False

    for call in calls:
        name = call["name"]
        args = call.get("arguments", {})
        call_start = time.perf_counter()
        result = send_vail_cmd(name, args, host=host, port=port)
        call_ms = (time.perf_counter() - call_start) * 1000

        results.append({"name": name, "arguments": args, "result": result, "ms": round(call_ms, 1)})

        if name == "vail_begin_batch":
            in_batch = True
        elif name == "vail_end_batch":
            in_batch = False

        if result.get("status") == "error" and in_batch:
            break

    total_ms = (time.perf_counter() - start) * 1000
    return {
        "status": "error" if any(r["result"].get("status") == "error" for r in results) else "success",
        "total_ms": round(total_ms, 1),
        "call_count": len(results),
        "calls": results,
    }


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        transcript = json.loads(sys.argv[1])
        print(json.dumps(execute_transcript(transcript), indent=2))
    else:
        print(json.dumps(send_vail_cmd("vail_get_tree", {"max_depth": 1, "category_filter": ""}), indent=2))
