"""
VAIL Live Actor Mutation Test 2
Tests setting properties via exact ID and Label on live DirectionalLight actor.
"""

import sys
import os
import json
import logging

sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from unreal_mcp_server import UnrealConnection

logging.basicConfig(level=logging.INFO)

def test_actor():
    conn = UnrealConnection()
    if not conn.connect():
        print("ERROR: Could not connect to UE5")
        return False

    print("\n=== STEP 1: Scoping DirectionalLight ===")
    res = conn.send_command("vail_set_scope", {"scope": "DetailsPanel", "target": "DirectionalLight"})
    print(f"Scope Result: {json.dumps(res, indent=2)}")

    print("\n=== STEP 2: Setting Mobility to 'Stationary' ===")
    res_mob = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->Mobility", 
        "value": "Stationary"
    })
    print(f"Mobility Mutation Result:\n{json.dumps(res_mob, indent=2)}")

    print("\n=== STEP 3: Setting RelativeLocation to (X=150.0, Y=250.0, Z=999.0) ===")
    res_loc = conn.send_command("vail_set_property", {
        "property_id": "DirectionalLightComponent->RelativeLocation", 
        "value": "X=150.0 Y=250.0 Z=999.0"
    })
    print(f"Location Mutation Result:\n{json.dumps(res_loc, indent=2)}")

    print("\n=== STEP 4: Inspecting Updated Tree ===")
    res_tree = conn.send_command("vail_get_tree", {"max_depth": 2, "category_filter": "Transform"})
    print(f"Updated Tree:\n{json.dumps(res_tree, indent=2)}")

    conn.disconnect()
    return True

if __name__ == "__main__":
    test_actor()
