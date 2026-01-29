# MeshLib API Quick Reference for Brush Editors

Quick lookup guide for integrating MeshLib into external brush editor projects.

## Essential Headers

```cpp
#include "MRMesh/MRMesh.h"              // Core mesh structure
#include "MRMesh/MRMeshProject.h"       // Surface projection
#include "MRMesh/MRSurfaceDistance.h"   // Geodesic distances
#include "MRMesh/MRMeshRelax.h"         // Smoothing
#include "MRMesh/MRLaplacian.h"         // Detail-preserving deformation
#include "MRMesh/MROffsetVerts.h"       // Push/pull operations
#include "MRMesh/MRChangeMeshAction.h"  // Undo/redo
#include "MRMesh/MRAABBTree.h"          // Spatial acceleration
```

## Quick Operations

### Load/Create Mesh
```cpp
MR::Mesh mesh = MR::Mesh::fromTriangles(vertices, triangles);
mesh.getAABBTree(); // Build spatial index
```

### Project Cursor to Surface
```cpp
auto proj = MR::findProjection(cursorPosition, mesh);
if (proj) {
    FaceId face = proj->proj.face;
    Vector3f point = mesh.triPoint(face, proj->proj.point);
}
```

### Find Brush Region
```cpp
// From projection point, get nearby vertices
MR::VertBitSet startVerts = getVerticesFromFace(mesh, face);
MR::VertScalars distances = MR::computeSurfaceDistances(
    mesh, startVerts, brushRadius
);

MR::VertBitSet affectedRegion;
for (VertId v : mesh.topology.getValidVerts()) {
    if (distances[v] < brushRadius) {
        affectedRegion.set(v);
    }
}
```

### Push/Pull Vertices
```cpp
MR::VertScalars offsets(mesh.topology.lastValidVert() + 1, 0.0f);
for (VertId v : affectedRegion) {
    float falloff = 1.0f - (distances[v] / brushRadius);
    offsets[v] = brushStrength * falloff;
}
MR::offsetVerts(mesh.points, offsets, mesh);
```

### Smooth Region
```cpp
MR::relax(mesh, &affectedRegion, iterations);
```

### Laplacian Deformation
```cpp
MR::Laplacian laplacian(mesh);
for (VertId v : boundaryVerts) laplacian.fixVertex(v);
laplacian.fixVertex(dragVertex, newPosition);
mesh.points = laplacian.apply();
```

### Update After Changes
```cpp
mesh.getAABBTreeNotCreate()->refit();  // Update spatial index (fast!)
MR::computeMeshNormals(mesh);          // Recalculate normals
```

### Undo/Redo
```cpp
// Before modification
auto undo = std::make_shared<MR::ChangeMeshPointsAction>(
    "Brush Stroke", meshObject
);
// ... modify mesh ...
MR::AppendHistory(undo); // After modification
```

## Common Brush Modes

### Mode 1: Push (Add)
```cpp
// Offset vertices outward along normals
MR::offsetVerts(mesh.points, positiveOffsets, mesh);
```

### Mode 2: Pull (Remove)
```cpp
// Offset vertices inward (negative values)
MR::offsetVerts(mesh.points, negativeOffsets, mesh);
```

### Mode 3: Smooth
```cpp
MR::relax(mesh, &region, 3-5 /* iterations */);
```

### Mode 4: Sculpt (Laplacian)
```cpp
MR::Laplacian lap(mesh);
// Fix boundaries, move vertices, apply
```

## Performance Patterns

✅ **DO:**
- Refit AABB: `tree->refit()` after vertex changes
- Use `VertBitSet` for regions
- Batch updates per frame
- Use surface distances for falloff

❌ **DON'T:**
- Rebuild AABB tree every frame
- Forget to update normals
- Use Euclidean distance for falloff
- Process vertices outside affected region

## Typical Brush Update Loop

```cpp
void onBrushDrag(Vector3f cursorPos) {
    // 1. Project to surface
    auto proj = MR::findProjection(cursorPos, mesh);
    if (!proj) return;
    
    // 2. Find affected vertices
    VertBitSet region = computeBrushRegion(proj, brushRadius);
    
    // 3. Apply operation
    switch (brushMode) {
        case Push:  applyOffset(region, +strength); break;
        case Pull:  applyOffset(region, -strength); break;
        case Smooth: MR::relax(mesh, &region, 3); break;
    }
    
    // 4. Update structures
    mesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(mesh);
}
```

## Key Classes

| Class | Purpose | Header |
|-------|---------|--------|
| `Mesh` | Core mesh data | MRMesh.h |
| `MeshTopology` | Edge/face/vertex relationships | MRMeshTopology.h |
| `VertBitSet` | Vertex selection | MRBitSet.h |
| `AABBTree` | Spatial queries | MRAABBTree.h |
| `Laplacian` | Detail-preserving deform | MRLaplacian.h |
| `ChangeMeshPointsAction` | Undo/redo | MRChangeMeshAction.h |

## Distance Functions

| Function | Use Case | Header |
|----------|----------|--------|
| `findProjection()` | Cursor → surface | MRMeshProject.h |
| `computeSurfaceDistances()` | Geodesic distances | MRSurfaceDistance.h |
| `signedDistanceToMesh()` | Inside/outside test | MRMeshDistance.h |

## Modification Functions

| Function | Effect | Header |
|----------|--------|--------|
| `offsetVerts()` | Push/pull along normals | MROffsetVerts.h |
| `relax()` | Laplacian smoothing | MRMeshRelax.h |
| `Laplacian::apply()` | Detail-preserving deform | MRLaplacian.h |
| `FreeFormDeformer` | Lattice deformation | MRFreeFormDeformer.h |

## Helper Utilities

```cpp
// Get vertices of a face
auto [v0, v1, v2] = mesh.topology.getTriVerts(faceId);

// Iterate vertex neighbors
for (EdgeId e : mesh.topology.orgRing(vertId)) {
    VertId neighbor = mesh.topology.dest(e);
}

// Get vertex position
Vector3f pos = mesh.points[vertId];

// Get vertex normal
Vector3f normal = mesh.normal(vertId);

// Triangle center
Vector3f center = mesh.triCenter(faceId);

// Expand/shrink region
MR::expand(mesh.topology, region, layers);
MR::shrink(mesh.topology, region, layers);
```

## Falloff Curves

```cpp
// Linear falloff
float falloff = 1.0f - (distance / radius);

// Smooth falloff (smoothstep)
float t = 1.0f - (distance / radius);
float falloff = t * t * (3.0f - 2.0f * t);

// Quadratic falloff
float t = 1.0f - (distance / radius);
float falloff = t * t;

// Cosine falloff
float falloff = 0.5f * (1.0f + cos(PI * distance / radius));
```

## Build Configuration

**CMake:**
```cmake
find_package(MeshLib REQUIRED)
target_link_libraries(YourBrushEditor PRIVATE MeshLib::MRMesh)
```

**Include paths:**
```
/path/to/MeshLib/source
```

**Link libraries:**
```
MRMesh (core library)
```

## Common Issues & Solutions

**Issue**: Slow performance after vertex changes
**Solution**: Use `tree->refit()` not rebuild

**Issue**: Normals look wrong after editing
**Solution**: Call `MR::computeMeshNormals(mesh)`

**Issue**: Brush affects wrong area
**Solution**: Use surface distance not Euclidean

**Issue**: Memory usage grows with undo
**Solution**: Use `PartialChangeMeshAction` for large meshes

**Issue**: Projection returns empty
**Solution**: Check `if (proj.has_value())` before use

## Resources

- **Full Guide**: See `MESHLIB_APIS_FOR_BRUSH_EDITORS.md`
- **Documentation**: https://meshlib.io/documentation/
- **Examples**: MeshLib `examples/` directory
- **Headers**: `source/MRMesh/*.h` for detailed API

---

**Quick Ref Version**: 1.0  
**Date**: 2026-01-29
