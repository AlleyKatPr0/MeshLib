# MeshLib APIs for External Brush Editor Projects

This document evaluates MeshLib components suitable for integration into external brush editor applications. It focuses on public APIs and algorithms that can be leveraged when building mesh sculpting/editing tools.

## Overview

If you're building a brush editor application and want to leverage MeshLib's powerful mesh processing capabilities, this guide identifies the key APIs and patterns you'll need.

**Target Project**: UE5 Brush Editor (https://github.com/AlleyKatPr0/ue5-brush-editor)  
**For UE5-specific integration**: See [UE5_INTEGRATION_GUIDE.md](./UE5_INTEGRATION_GUIDE.md)

## Core Components

### 1. Mesh Data Structures

#### `Mesh` Class (MRMesh/MRMesh.h)
The central mesh representation combining topology and geometry.

```cpp
#include "MRMesh/MRMesh.h"

// Create mesh from triangles
MR::Mesh mesh = MR::Mesh::fromTriangles(vertices, triangles);

// Access topology
const MR::MeshTopology& topology = mesh.topology;

// Access vertex positions
MR::VertCoords& points = mesh.points;

// Get normal at vertex
Vector3f normal = mesh.normal(vertId);
```

**Key Features:**
- Half-edge topology for efficient neighbor queries
- Automatic normal computation
- Support for regions/subsets via `MeshPart`
- Memory-efficient BitSet representations

#### `MeshTopology` (MRMesh/MRMeshTopology.h)
Low-level topology operations.

```cpp
// Iterate over vertex neighbors
for (EdgeId e : topology.orgRing(vertexId)) {
    VertId neighbor = topology.dest(e);
    // Process neighbor...
}

// Get faces around vertex
auto faces = topology.getVertFaces(vertexId);
```

### 2. Mesh Modification Algorithms

#### Relaxation/Smoothing (MRMesh/MRMeshRelax.h)

**Laplacian Smoothing:**
```cpp
#include "MRMesh/MRMeshRelax.h"

// Simple uniform smoothing
MR::relax(mesh);

// Region-based smoothing with iterations
MR::relax(mesh, &vertexRegion, /* iterations */ 5);

// Volume-preserving relaxation
MR::relaxKeepVolume(mesh, &vertexRegion);
```

**Use Case:** Perfect for softening brush strokes or cleaning up artifacts.

#### Laplacian Deformation (MRMesh/MRLaplacian.h)

**Detail-Preserving Shape Deformation:**
```cpp
#include "MRMesh/MRLaplacian.h"

// Setup Laplacian system
MR::Laplacian laplacian(mesh);

// Fix boundary vertices
for (VertId v : boundaryVerts) {
    laplacian.fixVertex(v);
}

// Move a vertex and compute smooth deformation
laplacian.fixVertex(brushCenterVert, newPosition);
mesh.points = laplacian.apply();
```

**Use Case:** High-quality sculpting that preserves surface details while allowing large deformations.

#### Free-Form Deformation (MRMesh/MRFreeFormDeformer.h)

```cpp
#include "MRMesh/MRFreeFormDeformer.h"

// Create deformer with control grid
MR::FreeFormDeformer deformer(mesh, gridResolution);

// Move control points
deformer.updateControlPoint(controlPointId, offset);

// Apply deformation
deformer.apply();
```

**Use Case:** Advanced sculpting with lattice-based deformation.

#### Vertex Offsetting (MRMesh/MROffsetVerts.h)

```cpp
#include "MRMesh/MROffsetVerts.h"

// Offset vertices along normals/pseudonormals
MR::VertScalars offsets(mesh.topology.lastValidVert() + 1, 0.0f);
// Set offsets for affected vertices
for (VertId v : affectedVerts) {
    offsets[v] = brushStrength * falloff;
}

MR::offsetVerts(mesh.points, offsets, mesh);
```

**Use Case:** Push/pull brush operations - moving surface outward or inward.

### 3. Spatial Queries & Distance Calculations

#### AABB Tree (MRMesh/MRAABBTree.h)

**Spatial Acceleration for Fast Queries:**
```cpp
#include "MRMesh/MRAABBTree.h"

// Build AABB tree once
mesh.getAABBTree(); // Auto-cached

// After modifying vertices, refit for fast update
mesh.getAABBTreeNotCreate()->refit();

// For custom queries
MR::AABBTree tree(mesh);
```

**Performance:** Refit is much faster than rebuilding (~10-100x).

#### Point-to-Mesh Projection (MRMesh/MRMeshProject.h)

**Find Closest Point on Surface:**
```cpp
#include "MRMesh/MRMeshProject.h"

// Project 3D point to mesh surface
Vector3f brushPosition = /* from cursor/input */;
auto projection = MR::findProjection(brushPosition, mesh);

if (projection.has_value()) {
    FaceId hitFace = projection->proj.face;
    Vector3f surfacePoint = mesh.triPoint(hitFace, projection->proj.point);
    float distance = projection->distSq;
    
    // Get barycentric coordinates for interpolation
    MR::PointOnFace& pof = projection->proj;
    // pof.point contains barycentric coordinates
}
```

**Use Case:** Converting 2D screen coordinates to 3D mesh surface positions for brush interaction.

#### Surface Distance (MRMesh/MRSurfaceDistance.h)

**Geodesic Distance Along Surface:**
```cpp
#include "MRMesh/MRSurfaceDistance.h"

// Compute distances from brush center
MR::VertBitSet startVerts;
startVerts.set(brushCenterVert);

MR::VertScalars distances = MR::computeSurfaceDistances(
    mesh, 
    startVerts,
    brushRadius  // max distance
);

// Use distances for falloff calculation
for (VertId v : affectedRegion) {
    float dist = distances[v];
    float falloff = 1.0f - (dist / brushRadius);
    // Apply with falloff...
}
```

**Use Case:** Calculate brush influence falloff based on surface distance (geodesic), not Euclidean distance.

#### Signed Distance (MRMesh/MRMeshDistance.h)

```cpp
#include "MRMesh/MRMeshDistance.h"

// Get signed distance (inside/outside mesh)
float signedDist = MR::signedDistanceToMesh(mesh, point);
```

**Use Case:** Determining if points are inside or outside the mesh for certain operations.

### 4. Region Selection

#### BitSet Operations

**Efficient Vertex/Face Selection:**
```cpp
#include "MRMesh/MRBitSet.h"

// Create vertex region
MR::VertBitSet affectedVerts;

// Set vertices within brush radius
for (VertId v : mesh.topology.getValidVerts()) {
    float dist = (mesh.points[v] - brushCenter).length();
    if (dist < brushRadius) {
        affectedVerts.set(v);
    }
}

// Expand/shrink regions
#include "MRMesh/MRExpandShrink.h"
MR::expand(mesh.topology, affectedVerts, /* layers */ 1);
MR::shrink(mesh.topology, affectedVerts, /* layers */ 1);
```

**Use Case:** Defining and manipulating brush-affected regions efficiently.

### 5. Undo/Redo System

#### History Actions (MRMesh/MRChangeMeshAction.h)

**Automatic Undo for Mesh Changes:**
```cpp
#include "MRMesh/MRChangeMeshAction.h"
#include "MRMesh/MRHistoryStore.h"

// At brush stroke start
auto undoAction = std::make_shared<MR::ChangeMeshPointsAction>(
    "Brush Stroke", 
    meshObject
);

// Modify mesh...
// ... apply brush operations ...

// At brush stroke end
MR::AppendHistory(undoAction);
```

**For Partial Changes (Memory-Efficient):**
```cpp
#include "MRMesh/MRPartialChangeMeshAction.h"

// Only store changed vertices
auto undoAction = std::make_shared<MR::PartialChangeMeshPointsAction>(
    "Brush Stroke",
    meshObject,
    affectedVerts,  // Only these vertices
    oldPositions
);
```

**Use Case:** Implement undo/redo for all brush operations.

### 6. Normal Calculations

#### Mesh Normals (MRMesh/MRMeshNormals.h)

```cpp
#include "MRMesh/MRMeshNormals.h"

// Update normals after modification
MR::computeMeshNormals(mesh);

// Get vertex normals
MR::VertNormals vertNormals = mesh.computePerVertNormals();

// Get face normals
MR::FaceNormals faceNormals = mesh.computePerFaceNormals();
```

**Use Case:** Recalculate normals after mesh deformation for correct shading.

### 7. Common Brush Editor Patterns

#### Pattern 1: Basic Push/Pull Brush

```cpp
void applyPushBrush(
    MR::Mesh& mesh,
    const Vector3f& brushCenter,
    float brushRadius,
    float brushStrength
) {
    // 1. Find affected vertices
    MR::VertBitSet affected;
    MR::VertScalars distances;
    
    // Project brush to surface
    auto proj = MR::findProjection(brushCenter, mesh);
    if (!proj) return;
    
    // Get surface distances
    MR::VertBitSet startVerts;
    startVerts.set(proj->proj.face /* convert to nearby vertices */);
    distances = MR::computeSurfaceDistances(mesh, startVerts, brushRadius);
    
    // 2. Calculate offsets with falloff
    MR::VertScalars offsets(mesh.topology.lastValidVert() + 1, 0.0f);
    for (VertId v : mesh.topology.getValidVerts()) {
        if (distances[v] < brushRadius) {
            float falloff = 1.0f - (distances[v] / brushRadius);
            falloff = smoothstep(falloff); // Optional: smooth falloff
            offsets[v] = brushStrength * falloff;
        }
    }
    
    // 3. Apply offset
    MR::offsetVerts(mesh.points, offsets, mesh);
    
    // 4. Update acceleration structure
    mesh.getAABBTreeNotCreate()->refit();
    
    // 5. Update normals
    MR::computeMeshNormals(mesh);
}
```

#### Pattern 2: Smooth Brush

```cpp
void applySmoothBrush(
    MR::Mesh& mesh,
    const Vector3f& brushCenter,
    float brushRadius,
    int iterations = 3
) {
    // Find affected region
    MR::VertBitSet region = findBrushRegion(mesh, brushCenter, brushRadius);
    
    // Apply relaxation
    MR::relax(mesh, &region, iterations);
    
    // Update
    mesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(mesh);
}
```

#### Pattern 3: Sculpt Brush with Laplacian

```cpp
void applySculptBrush(
    MR::Mesh& mesh,
    VertId dragVertex,
    const Vector3f& targetPosition
) {
    // Setup Laplacian
    MR::Laplacian laplacian(mesh);
    
    // Fix boundary vertices
    MR::VertBitSet boundary = findBoundary(mesh);
    for (VertId v : boundary) {
        laplacian.fixVertex(v);
    }
    
    // Move dragged vertex
    laplacian.fixVertex(dragVertex, targetPosition);
    
    // Solve and apply
    mesh.points = laplacian.apply();
    
    // Update
    mesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(mesh);
}
```

#### Pattern 4: Brush with Undo Support

```cpp
class BrushStroke {
    std::shared_ptr<MR::ObjectMesh> meshObject_;
    std::shared_ptr<MR::ChangeMeshPointsAction> undoAction_;
    bool active_ = false;
    
public:
    void begin() {
        // Create undo action capturing current state
        undoAction_ = std::make_shared<MR::ChangeMeshPointsAction>(
            "Brush Stroke",
            meshObject_
        );
        active_ = true;
    }
    
    void update(const Vector3f& brushPos, float strength) {
        if (!active_) return;
        
        // Apply brush modification
        applyBrushOperation(meshObject_->mesh(), brushPos, strength);
    }
    
    void end() {
        if (!active_) return;
        
        // Append to history
        MR::AppendHistory(undoAction_);
        active_ = false;
    }
};
```

## Integration Checklist

When integrating MeshLib into your brush editor:

- [ ] **Include MeshLib headers** - Add MRMesh include paths
- [ ] **Link MRMesh library** - Link against MRMesh.lib/.so
- [ ] **Initialize mesh** - Load or create `MR::Mesh` object
- [ ] **Setup AABB tree** - Build spatial acceleration once
- [ ] **Implement brush projection** - Map input to surface
- [ ] **Choose modification algorithm** - relax, Laplacian, or offset
- [ ] **Calculate affected region** - Use BitSets and distance queries
- [ ] **Apply falloff** - Smooth brush influence
- [ ] **Update structures** - Refit AABB, recompute normals
- [ ] **Add undo/redo** - Wrap in HistoryActions
- [ ] **Optimize updates** - Refit not rebuild, partial updates

## Performance Tips

1. **Refit AABB tree** instead of rebuilding: `tree->refit()` is ~100x faster
2. **Use VertBitSet** for region tracking - memory efficient
3. **Limit brush radius** - Smaller radius = fewer vertices = faster
4. **Use surface distance** for better brush feel vs Euclidean distance
5. **Batch updates** - Accumulate changes during drag, apply once per frame
6. **Partial undo** - `PartialChangeMeshAction` only stores changed vertices
7. **Consider threading** - Many MeshLib operations support parallel execution

## Common Pitfalls

⚠️ **Don't rebuild AABB tree every frame** - Use refit() instead
⚠️ **Remember to update normals** - After vertex modification
⚠️ **Check for null projections** - findProjection() can return empty
⚠️ **Validate vertex IDs** - Use `topology.hasVert(v)` before access
⚠️ **Handle empty BitSets** - Check `bitset.any()` before operations

## API Documentation

For complete API documentation, see:
- **Online**: https://meshlib.io/documentation/
- **Headers**: `source/MRMesh/*.h` files have detailed comments
- **Examples**: `examples/` directory for usage patterns

## Example: Minimal Brush Editor

```cpp
#include "MRMesh/MRMesh.h"
#include "MRMesh/MRMeshRelax.h"
#include "MRMesh/MRMeshProject.h"
#include "MRMesh/MRSurfaceDistance.h"
#include "MRMesh/MROffsetVerts.h"
#include "MRMesh/MRChangeMeshAction.h"

class SimpleBrushEditor {
    MR::Mesh mesh_;
    
public:
    enum class BrushMode { Push, Pull, Smooth };
    
    void loadMesh(const std::string& filename) {
        // Load mesh from file
        mesh_ = /* ... */;
        mesh_.getAABBTree(); // Build spatial index
    }
    
    void applyBrush(
        const Vector3f& cursorPos,
        BrushMode mode,
        float radius,
        float strength
    ) {
        // Project cursor to surface
        auto proj = MR::findProjection(cursorPos, mesh_);
        if (!proj) return;
        
        // Find affected region
        MR::VertBitSet affected = findRegion(proj->proj.face, radius);
        
        // Apply operation based on mode
        switch (mode) {
            case BrushMode::Push:
                applyOffset(affected, radius, strength);
                break;
            case BrushMode::Pull:
                applyOffset(affected, radius, -strength);
                break;
            case BrushMode::Smooth:
                MR::relax(mesh_, &affected, 3);
                break;
        }
        
        // Update structures
        mesh_.getAABBTreeNotCreate()->refit();
        MR::computeMeshNormals(mesh_);
    }
    
private:
    MR::VertBitSet findRegion(FaceId startFace, float radius) {
        // Get vertices of start face
        MR::VertBitSet startVerts;
        auto verts = mesh_.topology.getVertIds(startFace);
        for (VertId v : verts) {
            startVerts.set(v);
        }
        
        // Compute surface distances
        auto distances = MR::computeSurfaceDistances(
            mesh_, startVerts, radius
        );
        
        // Build region BitSet
        MR::VertBitSet region;
        for (VertId v : mesh_.topology.getValidVerts()) {
            if (distances[v] < radius) {
                region.set(v);
            }
        }
        return region;
    }
    
    void applyOffset(
        const MR::VertBitSet& region,
        float radius,
        float strength
    ) {
        // Calculate offsets with falloff
        MR::VertScalars offsets(mesh_.topology.lastValidVert() + 1, 0.0f);
        
        for (VertId v : region) {
            // Simple distance-based falloff
            // (In real implementation, use surface distance)
            offsets[v] = strength;
        }
        
        // Apply
        MR::offsetVerts(mesh_.points, offsets, mesh_);
    }
};
```

## Conclusion

MeshLib provides a comprehensive suite of APIs for building brush-based mesh editors:

✅ **Robust mesh data structures** with half-edge topology
✅ **High-quality deformation algorithms** (Laplacian, relaxation, FFD)
✅ **Fast spatial queries** via AABB trees with efficient refitting
✅ **Flexible region operations** using efficient BitSets
✅ **Built-in undo/redo** infrastructure
✅ **Production-tested** in MeshInspector application

The APIs are well-designed for external integration and provide all the building blocks needed for a professional brush editor application.

---

**Document Version**: 1.0  
**Date**: 2026-01-29  
**For**: External brush editor projects integrating MeshLib
