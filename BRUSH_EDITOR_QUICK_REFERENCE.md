# Brush Editor - Quick Reference Guide

## 📋 Summary

**Component**: `SurfaceManipulationWidget`  
**Location**: `source/MRViewer/MRSurfaceManipulationWidget.{h,cpp}`  
**Status**: ✅ Production-ready  
**Purpose**: Interactive 3D mesh brush tool for surface sculpting

---

## 🎨 5 Editing Modes

| Mode | Description | Typical Use |
|------|-------------|-------------|
| **Add** | Push surface outward | Creating bumps, raising areas |
| **Remove** | Push surface inward | Carving, creating depressions |
| **Relax** | Smooth surface | Removing artifacts, blending |
| **Laplacian** | Detail-preserving smooth | Fine adjustments |
| **Patch** | Fill/reconstruct | Hole filling, surface repair |

---

## ⚙️ Key Parameters

```cpp
struct Settings {
    WorkMode workMode;           // Add/Remove/Relax/Laplacian/Patch
    float radius;                // Brush size (editing area)
    float editForce;             // Strength of change
    float sharpness;             // Force falloff (0-100)
    float relaxForce;            // Smoothing speed (0-0.5)
    float relaxForceAfterEdit;   // Auto-smooth after edit (0-0.5)
};
```

### Quick Settings Presets

**Gentle Smoothing**:
```cpp
workMode = Relax, radius = 10.0, relaxForce = 0.15, sharpness = 30
```

**Precise Carving**:
```cpp
workMode = Remove, radius = 2.0, editForce = 0.5, sharpness = 80
```

**Organic Sculpting**:
```cpp
workMode = Add, radius = 5.0, editForce = 1.5, sharpness = 40
```

---

## 🚀 Quick Start

```cpp
#include "MRSurfaceManipulationWidget.h"

// 1. Create widget
auto widget = std::make_shared<MR::SurfaceManipulationWidget>();

// 2. Load mesh
auto objectMesh = std::make_shared<MR::ObjectMesh>();
objectMesh->setMesh(yourMesh);

// 3. Initialize
widget->init(objectMesh);

// 4. Configure
MR::SurfaceManipulationWidget::Settings settings;
settings.workMode = MR::SurfaceManipulationWidget::WorkMode::Add;
settings.radius = 5.0f;
settings.editForce = 1.0f;
widget->setSettings(settings);

// 5. Register with viewer (handles mouse events automatically)
viewer.addListener(widget);
```

---

## 🔧 Advanced Features

### Lock Regions
```cpp
// Prevent modification of specific areas
FaceBitSet fixedFaces = /* create face set */;
widget->setFixedRegion(fixedFaces);
```

### Deviation Visualization
```cpp
// Show color-coded changes
widget->enableDeviationVisualization(true);
widget->setDeviationCalculationMethod(
    SurfaceManipulationWidget::DeviationCalculationMethod::ExactDistance
);
```

### Occlusion Control
```cpp
// Only edit visible surfaces
widget->setIgnoreOcclusion(false);

// Edit surfaces facing camera
widget->setEditOnlyCodirectedSurface(true);
```

---

## 📊 Performance Guidelines

| Mesh Size | Expected FPS | Notes |
|-----------|--------------|-------|
| < 10K verts | 60+ FPS | Smooth real-time |
| 10K-50K verts | 30-60 FPS | Good interactive |
| 50K-100K verts | 15-30 FPS | Acceptable |
| > 100K verts | < 15 FPS | May need optimization |

**Laplacian mode**: Slower due to solver complexity

---

## 🏗️ Architecture

### Key Data Structures
```cpp
VertBitSet activePickedVertices_;     // Current affected vertices
VertBitSet singleEditingRegion_;      // Current brush area
VertBitSet generalEditingRegion_;     // Accumulated changes
VertScalars editingDistanceMap_;      // Distance from brush center
```

### Mouse Event Flow
1. **Mouse Down**: Pick vertices, create history action
2. **Mouse Move**: Update region, apply changes in real-time
3. **Mouse Up**: Compress history, finalize edit

---

## 📦 Dependencies

### Required Libraries
```cpp
#include "MRMesh/MRMesh.h"              // Core mesh
#include "MRMesh/MRObjectMesh.h"        // Mesh object wrapper
#include "MRMesh/MRMeshRelax.h"         // Smoothing
#include "MRMesh/MRLaplacian.h"         // Laplacian deformation
#include "MRMesh/MRSurfaceDistance.h"   // Distance calculations
```

### Build Requirements
- C++17 or later
- CMake 3.16+
- MeshLib SDK
- OpenGL
- Boost (for signals)

---

## ⚠️ Common Gotchas

1. **Must call `init()` before use** - Widget needs mesh reference
2. **Radius in world units** - Scale appropriately for your mesh
3. **Viewer integration required** - Needs mouse picking system
4. **History compression** - Happens automatically on mouse up
5. **Topology preserved** - Doesn't add/remove vertices (by design)

---

## ✅ Validation Checklist

Before using in production:

- [ ] Validate radius > 0
- [ ] Check sharpness in [0, 100]
- [ ] Verify relaxForce in (0, 0.5]
- [ ] Ensure mesh has valid topology
- [ ] Test undo/redo functionality
- [ ] Profile performance with target mesh sizes
- [ ] Handle edge cases (empty meshes, degenerate geometry)

---

## 🔍 Debugging Tips

### Enable Visual Feedback
```cpp
// See brush area
widget->enableDeviationVisualization(true);

// Check affected region
const auto& region = widget->getSettings();
std::cout << "Radius: " << region.radius << std::endl;
```

### Check Performance
```cpp
// Time your edits
auto start = std::chrono::high_resolution_clock::now();
widget->changeSurface_();
auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Edit took: " << duration.count() << "ms" << std::endl;
```

---

## 📚 Further Reading

**Full Evaluation**: See `BRUSH_EDITOR_CODE_EVALUATION.md`  
**API Documentation**: MeshLib documentation at https://meshlib.io  
**Examples**: `source/EditableProject/HelloWorldPlugin.cpp`  
**Source Code**: `source/MRViewer/MRSurfaceManipulationWidget.{h,cpp}`

---

## 💡 Pro Tips

1. **Start with Relax mode** - Good for testing, safe, fast
2. **Adjust sharpness first** - Has biggest impact on feel
3. **Use relaxForceAfterEdit** - Prevents jaggy edges automatically
4. **Lock boundaries** - Use `setFixedRegion()` to protect edges
5. **Test incrementally** - Start with small radius, increase gradually

---

## 🎯 Recommended Workflow

1. Load mesh and initialize widget
2. Start with conservative settings (small radius, low force)
3. Test each mode to understand behavior
4. Adjust sharpness to get desired brush feel
5. Fine-tune relaxForceAfterEdit for smooth results
6. Add UI controls for real-time parameter adjustment
7. Implement undo/redo UI buttons
8. Add deviation visualization toggle
9. Profile and optimize for your target mesh sizes

---

**Last Updated**: 2026-01-29  
**Version**: 1.0  
**Repository**: AlleyKatPr0/MeshLib
