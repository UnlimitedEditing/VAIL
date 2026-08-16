"""
VAIL Verification Script
Tests VAIL MCP tools against an active Unreal Engine editor instance.
"""

import sys
import os
import json
import logging

# Add parent directory to path
sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

from unreal_mcp_server import UnrealConnection

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("VAILTest")

def run_vail_tests():
    conn = UnrealConnection()
    if not conn.connect():
        logger.error("Could not connect to Unreal Engine on port 55557")
        return False

    logger.info("Connected to Unreal Engine. Starting VAIL validation tests...")

    # Test 1: Find commands
    logger.info("\n--- Test 1: vail_find (Search for 'Save' and 'Compile') ---")
    res_find = conn.send_command("vail_find", {"query": "Save"})
    logger.info(f"Find result: {json.dumps(res_find, indent=2)}")

    # Test 2: Set Scope to DetailsPanel
    logger.info("\n--- Test 2: vail_set_scope ---")
    res_scope = conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": ""})
    logger.info(f"Scope result: {json.dumps(res_scope, indent=2)}")

    # Test 3: Get Property Tree
    logger.info("\n--- Test 3: vail_get_tree ---")
    res_tree = conn.send_command("vail_get_tree", {"max_depth": 2, "category_filter": "Transform"})
    logger.info(f"Get tree result: {json.dumps(res_tree, indent=2)}")

    # Test 4: Wait For Settle
    logger.info("\n--- Test 4: vail_wait_for ---")
    res_wait = conn.send_command("vail_wait_for", {"timeout_seconds": 3.0})
    logger.info(f"Wait result: {json.dumps(res_wait, indent=2)}")

    # Test 5: Batch Transaction Lifecycle
    logger.info("\n--- Test 5: vail_begin_batch & vail_end_batch ---")
    res_begin = conn.send_command("vail_begin_batch", {"title": "Test Multi-Property Batch"})
    logger.info(f"Begin batch: {json.dumps(res_begin, indent=2)}")
    res_end = conn.send_command("vail_end_batch", {})
    logger.info(f"End batch: {json.dumps(res_end, indent=2)}")

    conn.disconnect()
    logger.info("\nVAIL validation test sequence complete.")
    return True

if __name__ == "__main__":
    run_vail_tests()
