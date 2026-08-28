#!/usr/bin/env python3
"""
VAIL CLI: direct one-shot tool dispatch, no MCP registration required.

VAIL's tools are meant to be reachable two ways: through an MCP server
(vail_server.py) for hosts that speak MCP, and directly from the command
line for a frontier coding agent working in a plain shell (Claude Code, a
CI script, a one-off debug session) that doesn't want to stand up an MCP
connection just to flip one property. This is the second path -- it does
nothing vail_server.py's tools don't already do, it just skips the MCP
handshake and talks straight over the same TCP socket the plugin already
listens on (127.0.0.1:55557 by default).

Usage:
    python vail_cli.py <tool_name> '<json_arguments>'
    python vail_cli.py vail_set_property '{"property_id": "Mobility", "value": "Movable"}'
    python vail_cli.py vail_get_tree '{}'
    echo '{"max_depth": 1}' | python vail_cli.py vail_get_tree -

Exit code is 0 on {"status": "success", ...}, 1 otherwise (including
connection failure) -- so it composes normally in shell scripts.
"""

import argparse
import json
import sys
from pathlib import Path

_GRUNT_DIR = Path(__file__).resolve().parent.parent / "grunt"
sys.path.insert(0, str(_GRUNT_DIR))

from vail_socket_client import send_vail_cmd  # noqa: E402


def main():
    parser = argparse.ArgumentParser(description="Direct VAIL tool dispatch over TCP, no MCP required.")
    parser.add_argument("tool_name", help="VAIL tool name, e.g. vail_set_property")
    parser.add_argument("arguments", nargs="?", default="{}", help="JSON object of arguments, or '-' to read JSON from stdin")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=55557)
    parser.add_argument("--timeout", type=float, default=15.0)
    parser.add_argument("--pretty", action="store_true", help="Pretty-print the JSON result")
    args = parser.parse_args()

    raw_args = sys.stdin.read() if args.arguments == "-" else args.arguments
    try:
        arguments = json.loads(raw_args) if raw_args.strip() else {}
    except json.JSONDecodeError as exc:
        sys.exit(f"Invalid JSON arguments: {exc}")

    result = send_vail_cmd(args.tool_name, arguments, host=args.host, port=args.port, timeout=args.timeout)

    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result.get("status") == "success" else 1)


if __name__ == "__main__":
    main()
