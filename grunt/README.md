# Grunt for Unreal Engine 5.8 VAIL

Zero-setup, on-device State Reconciler & Tool Router powered by the fine-tuned 26M Needle model.

## Quickstart (Python API)

```python
from grunt import GruntVAIL

agent = GruntVAIL()

# 1. Atomic Intent Routing (<20ms)
call = agent.route("Set property 'RelativeLocation.X' to '500.0'.")
print(call)
# -> {"name": "vail_set_property", "arguments": {"property_id": "RelativeLocation.X", "value": "500.0"}}

# 2. Compound State Batch Reconciliation
batch = agent.reconcile_compound(
    title="Configure CameraActor_1",
    scope_target="CameraActor_1",
    property_mutations=[
        ("RelativeLocation.X", "27.0"),
        ("RelativeLocation.Z", "139.0"),
        ("Mobility", "Static"),
        ("bSimulatePhysics", "false")
    ]
)
print(batch)
```

## Quickstart (CLI)

```powershell
python grunt_vail.py "Run editor command 'Kismet.Compile' to recompile active blueprint"
# -> {"name": "vail_execute_command", "arguments": {"command_id": "Kismet.Compile"}}
```

## Included Components
- **`checkpoints/grunt_needle_vail_master.pkl`**: Pre-trained & validated 50.19MB master checkpoint.
- **`needle_engine/`**: Self-contained neural execution engine (JAX/Flax SimpleAttentionNetwork + constrained trie decoder + SentencePiece tokenizer).
- **`tools/`**: All 8 pre-flattened VAIL tool definitions.