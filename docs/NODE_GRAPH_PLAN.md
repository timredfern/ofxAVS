# AVS Node Graph Architecture Plan

## Overview

Transform AVS from a linear effect chain to a unified node graph system supporting multiple inputs/outputs and typed data connections, while maintaining full backward compatibility.

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Architecture | Unified model | Linear chains become simple node graphs internally. All rendering goes through node system. |
| Compatibility | New effect types | Create new node classes (MixerNode, etc.) rather than modifying existing effects. |
| Scope | Full node system | Support both image compositing AND data flow (scalars, vectors, geometry). |

## Core Data Types

### Port Data Types
```cpp
enum class DataType {
    SCALAR,      // double - single value
    VECTOR2,     // {x, y} position
    VECTOR3,     // {x, y, z} for 3D
    VECTOR_LIST, // array of vectors (line drawings)
    IMAGE,       // framebuffer (templated pixel type)
    AUDIO,       // AudioData reference
    BEAT         // beat signal (bool)
};
```

### Key Structures
- **PortDef**: Describes port name, type, direction, required flag, default value
- **PortData**: Type-safe union holding actual data for a port
- **Connection**: Links `{from_node, from_port}` to `{to_node, to_port}`
- **NodeGraph**: Contains nodes + connections, handles execution order

## Node Architecture

### NodeBase Interface
```cpp
class NodeBase {
    virtual const std::vector<PortDef>& input_ports() const = 0;
    virtual const std::vector<PortDef>& output_ports() const = 0;
    virtual void process(const std::vector<PortData>& inputs,
                        std::vector<PortData>& outputs,
                        int w, int h) = 0;
    ParameterGroup& parameters();  // Reuses existing parameter system
};
```

### LegacyEffectNode (Backward Compatibility)
Wraps existing `EffectBase` subclasses as nodes:
- **Inputs**: image (optional), audio, beat
- **Outputs**: image
- Calls existing `render(visdata, isBeat, framebuffer, fbout, w, h)` internally
- All 53 existing effects work unchanged

### New Node Types
| Node | Purpose | Inputs | Outputs |
|------|---------|--------|---------|
| MixerNode | Composite multiple images | image_a, image_b, ... | image |
| SplitterNode | Fan-out single image | image | out_a, out_b, ... |
| ScalarSourceNode | Constant/animated value | - | scalar |
| BeatScalarNode | ADSR envelope on beat | beat | scalar |
| AudioScalarNode | Audio-reactive value | audio | scalar |
| VectorListNode | Points from audio | audio | points |
| VectorTransformNode | Transform geometry | points, scalars | points |
| VectorRenderNode | Draw points to image | points, image? | image |

## Execution Model

1. **Topological sort** - Kahn's algorithm determines execution order (cached, rebuilt only on graph changes)
2. **Per-node execution** - Gather inputs from connected sources, call `process()`, store outputs
3. **Buffer pool** - Reusable framebuffer allocation, reference counting for lifetime

**Performance note:** Graph traversal overhead is negligible. For a 20-effect preset, graph operations are ~100 ops vs ~40M pixel operations at 1080p. No special "fast path" needed for linear chains.

## Serialization

### Extended JSON Format
```json
{
  "version": "2.0",
  "format": "avs-node-graph",
  "graph_mode": "simple_chain",
  "nodes": [
    {"id": "node_1", "type": "SuperScope", "pos": [100, 200], "params": {...}}
  ],
  "connections": [
    {"from": "node_1", "from_port": "image", "to": "node_2", "to_port": "image"}
  ],
  "effects": [...]  // Legacy format for backward compat
}
```

### Legacy Preset Loading
- Convert `EffectContainer` tree to node graph
- Each effect becomes `LegacyEffectNode`
- Sequential effects get `image` connections
- `EffectList` with blending becomes subgraph with blend nodes

## Implementation Phases

### Phase 1: Core Infrastructure
**Files to create in `libs/avs_lib/core/node/`:**
- `port_types.h` - DataType enum, PortDef, PortData
- `node_base.h` - NodeBase abstract class
- `node_graph.h/.cpp` - NodeGraph with connection management
- `buffer_pool.h/.cpp` - Framebuffer allocation pool
- `legacy_effect_node.h/.cpp` - EffectBase wrapper

**Tests:** Graph operations, topological sort, cycle detection

### Phase 2: Graph Execution
- `node_graph_executor.h/.cpp` - Execute node graph
- Integrate with `Renderer` - node graph becomes the execution path

### Phase 3: New Node Types
- Implement MixerNode, SplitterNode
- Implement scalar source nodes (constant, beat, audio)
- Implement vector/geometry nodes

### Phase 4: Serialization
- Extend `preset.cpp` with node graph JSON support
- Implement legacy preset → node graph conversion
- Bidirectional: node graph → legacy format for simple chains

### Phase 5: UI Integration (ofxAVS layer)
- Node canvas UI for node-based app variant
- Legacy presets load as linear chain of nodes
- Separate app variant can use legacy list UI with legacy renderer

## Files to Modify

| File | Changes |
|------|---------|
| `libs/avs_lib/core/renderer.h` | Add NodeGraph integration point |
| `libs/avs_lib/core/preset.cpp` | Extend serialization for node graphs |
| `libs/avs_lib/core/effect_registry.h` | Add node type registration |

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| Buffer allocation churn | Pool-based allocation, reuse between frames |
| Type mismatches | Validate connections at connect time |
| Complex preset conversion | Extensive testing with existing preset library |
| Nested EffectList conversion | Map to subgraphs with blend nodes at entry/exit |

## Out of Scope (Future)

- GPU-accelerated nodes
- Custom node scripting (user-defined node types)
- Subgraph/macro nodes (reusable node groups)
