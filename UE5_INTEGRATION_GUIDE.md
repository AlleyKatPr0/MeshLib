# MeshLib Integration Guide for UE5 Brush Editor

## Overview

This guide documents how to integrate MeshLib's mesh processing capabilities into an Unreal Engine 5 brush editor project. It covers data type conversions, plugin architecture, and UE5-specific considerations.

## Project Context

**Target**: UE5 Brush Editor (https://github.com/AlleyKatPr0/ue5-brush-editor)  
**Integration Goal**: Leverage MeshLib's robust mesh processing algorithms within UE5  
**Approach**: Create a UE5 plugin wrapper around MeshLib APIs

---

## Integration Architecture

### Option 1: Direct Library Integration (Recommended)

Link MeshLib as a third-party library within your UE5 plugin:

```
YourProject/
├── Plugins/
│   └── MeshLibBrushEditor/
│       ├── Source/
│       │   └── MeshLibBrushEditor/
│       │       ├── Private/
│       │       │   ├── MeshLibWrapper.cpp
│       │       │   └── BrushOperations.cpp
│       │       ├── Public/
│       │       │   ├── MeshLibWrapper.h
│       │       │   └── BrushOperations.h
│       │       └── MeshLibBrushEditor.Build.cs
│       ├── ThirdParty/
│       │   └── MeshLib/
│       │       ├── Include/    (MeshLib headers)
│       │       └── Lib/        (MRMesh.lib/so)
│       └── MeshLibBrushEditor.uplugin
```

### Option 2: Static/Dynamic Library

Build MeshLib as a separate DLL/SO and load at runtime.

---

## Build Configuration

### MeshLibBrushEditor.Build.cs

```csharp
using UnrealBuildTool;
using System.IO;

public class MeshLibBrushEditor : ModuleRules
{
    public MeshLibBrushEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "ProceduralMeshComponent",  // For dynamic mesh
            "GeometryCore",             // UE5 geometry framework
            "GeometryFramework",
            "DynamicMesh"
        });
        
        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Slate",
            "SlateCore",
            "UnrealEd"  // If editor-only plugin
        });
        
        // MeshLib integration
        string MeshLibPath = Path.Combine(ModuleDirectory, "../ThirdParty/MeshLib");
        string MeshLibInclude = Path.Combine(MeshLibPath, "Include");
        string MeshLibLib = Path.Combine(MeshLibPath, "Lib");
        
        PublicIncludePaths.Add(MeshLibInclude);
        
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add(Path.Combine(MeshLibLib, "Win64", "MRMesh.lib"));
            
            // Copy DLL to binaries
            string DllPath = Path.Combine(MeshLibLib, "Win64", "MRMesh.dll");
            RuntimeDependencies.Add(DllPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicAdditionalLibraries.Add(Path.Combine(MeshLibLib, "Linux", "libMRMesh.so"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add(Path.Combine(MeshLibLib, "Mac", "libMRMesh.dylib"));
        }
        
        // C++ standard
        CppStandard = CppStandardVersion.Cpp17;
    }
}
```

---

## Data Type Conversions

### UE5 ↔ MeshLib Coordinate System

**Critical**: UE5 uses left-handed Z-up, MeshLib may use different conventions.

```cpp
// UE5 to MeshLib
MR::Vector3f ToMeshLib(const FVector& UEVec)
{
    // Adjust for coordinate system if needed
    // UE5: X=Forward, Y=Right, Z=Up (left-handed)
    return MR::Vector3f(UEVec.X, UEVec.Y, UEVec.Z);
}

// MeshLib to UE5
FVector ToUnreal(const MR::Vector3f& MRVec)
{
    return FVector(MRVec.x, MRVec.y, MRVec.z);
}
```

### Mesh Conversion: UDynamicMesh ↔ MR::Mesh

#### UE5 → MeshLib

```cpp
#include "MRMesh/MRMesh.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"

class FMeshLibWrapper
{
public:
    static MR::Mesh ConvertToMeshLib(const UE::Geometry::FDynamicMesh3& DynMesh)
    {
        // Extract vertices
        std::vector<MR::Vector3f> vertices;
        vertices.reserve(DynMesh.VertexCount());
        
        for (int32 VID : DynMesh.VertexIndicesItr())
        {
            FVector3d Pos = DynMesh.GetVertex(VID);
            vertices.push_back(MR::Vector3f(
                static_cast<float>(Pos.X),
                static_cast<float>(Pos.Y),
                static_cast<float>(Pos.Z)
            ));
        }
        
        // Extract triangles
        std::vector<MR::MeshBuilder::Triangle> triangles;
        triangles.reserve(DynMesh.TriangleCount());
        
        for (int32 TID : DynMesh.TriangleIndicesItr())
        {
            FIndex3i Tri = DynMesh.GetTriangle(TID);
            triangles.push_back({
                MR::VertId(Tri.A),
                MR::VertId(Tri.B),
                MR::VertId(Tri.C)
            });
        }
        
        // Create MeshLib mesh
        return MR::Mesh::fromTriangles(vertices, triangles);
    }
};
```

#### MeshLib → UE5

```cpp
static void ConvertToUE5(
    const MR::Mesh& MRMesh,
    UE::Geometry::FDynamicMesh3& OutDynMesh
)
{
    OutDynMesh.Clear();
    
    // Copy vertices
    const MR::VertCoords& points = MRMesh.points;
    TMap<MR::VertId, int32> VertexMap;
    
    for (MR::VertId vid : MRMesh.topology.getValidVerts())
    {
        const MR::Vector3f& pos = points[vid];
        int32 NewVID = OutDynMesh.AppendVertex(FVector3d(pos.x, pos.y, pos.z));
        VertexMap.Add(vid, NewVID);
    }
    
    // Copy triangles
    for (MR::FaceId fid : MRMesh.topology.getValidFaces())
    {
        MR::VertId v0, v1, v2;
        MRMesh.topology.getTriVerts(fid, v0, v1, v2);
        
        int32 UE_V0 = VertexMap[v0];
        int32 UE_V1 = VertexMap[v1];
        int32 UE_V2 = VertexMap[v2];
        
        OutDynMesh.AppendTriangle(UE_V0, UE_V1, UE_V2);
    }
    
    // Optionally compute normals
    // OutDynMesh.EnableAttributes();
    // FMeshNormals::QuickComputeVertexNormals(OutDynMesh);
}
```

---

## Brush Operations for UE5

### Wrapper Class

```cpp
// MeshLibWrapper.h
#pragma once

#include "CoreMinimal.h"
#include "MRMesh/MRMesh.h"
#include "DynamicMesh/DynamicMesh3.h"

UENUM(BlueprintType)
enum class EBrushMode : uint8
{
    Push    UMETA(DisplayName = "Push"),
    Pull    UMETA(DisplayName = "Pull"),
    Smooth  UMETA(DisplayName = "Smooth"),
    Sculpt  UMETA(DisplayName = "Sculpt (Laplacian)")
};

class MESHLIBBRUSHEDITOR_API FBrushOperation
{
public:
    // Apply brush stroke
    static bool ApplyBrush(
        UE::Geometry::FDynamicMesh3& DynMesh,
        const FVector& BrushCenter,
        float BrushRadius,
        float BrushStrength,
        EBrushMode Mode
    );
    
private:
    static void ApplyPushPull(
        MR::Mesh& Mesh,
        const MR::Vector3f& Center,
        float Radius,
        float Strength,
        bool bPush
    );
    
    static void ApplySmooth(
        MR::Mesh& Mesh,
        const MR::Vector3f& Center,
        float Radius,
        int32 Iterations
    );
    
    static void ApplySculpt(
        MR::Mesh& Mesh,
        const MR::Vector3f& Center,
        float Radius,
        const MR::Vector3f& TargetPosition
    );
};
```

### Implementation

```cpp
// BrushOperations.cpp
#include "BrushOperations.h"
#include "MRMesh/MRMeshProject.h"
#include "MRMesh/MRSurfaceDistance.h"
#include "MRMesh/MROffsetVerts.h"
#include "MRMesh/MRMeshRelax.h"
#include "MRMesh/MRLaplacian.h"
#include "MRMesh/MRMeshNormals.h"

bool FBrushOperation::ApplyBrush(
    UE::Geometry::FDynamicMesh3& DynMesh,
    const FVector& BrushCenter,
    float BrushRadius,
    float BrushStrength,
    EBrushMode Mode
)
{
    // Convert UE5 mesh to MeshLib
    MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(DynMesh);
    
    // Build AABB tree for spatial queries
    MRMesh.getAABBTree();
    
    // Convert brush center
    MR::Vector3f MRCenter(BrushCenter.X, BrushCenter.Y, BrushCenter.Z);
    
    // Apply operation
    switch (Mode)
    {
        case EBrushMode::Push:
            ApplyPushPull(MRMesh, MRCenter, BrushRadius, BrushStrength, true);
            break;
        case EBrushMode::Pull:
            ApplyPushPull(MRMesh, MRCenter, BrushRadius, -BrushStrength, false);
            break;
        case EBrushMode::Smooth:
            ApplySmooth(MRMesh, MRCenter, BrushRadius, 3);
            break;
        case EBrushMode::Sculpt:
            ApplySculpt(MRMesh, MRCenter, BrushRadius, MRCenter);
            break;
    }
    
    // Convert back to UE5
    FMeshLibWrapper::ConvertToUE5(MRMesh, DynMesh);
    
    return true;
}

void FBrushOperation::ApplyPushPull(
    MR::Mesh& Mesh,
    const MR::Vector3f& Center,
    float Radius,
    float Strength,
    bool bPush
)
{
    // Find affected region
    auto projection = MR::findProjection(Center, Mesh);
    if (!projection) return;
    
    // Get start vertices for distance calculation
    MR::VertBitSet startVerts;
    MR::VertId v0, v1, v2;
    Mesh.topology.getTriVerts(projection->proj.face, v0, v1, v2);
    startVerts.set(v0);
    startVerts.set(v1);
    startVerts.set(v2);
    
    // Compute surface distances
    MR::VertScalars distances = MR::computeSurfaceDistances(
        Mesh, startVerts, Radius
    );
    
    // Calculate offsets with falloff
    MR::VertScalars offsets(Mesh.topology.lastValidVert() + 1, 0.0f);
    
    for (MR::VertId vid : Mesh.topology.getValidVerts())
    {
        if (distances[vid] < Radius)
        {
            // Smooth falloff
            float t = 1.0f - (distances[vid] / Radius);
            float falloff = t * t * (3.0f - 2.0f * t); // smoothstep
            
            offsets[vid] = Strength * falloff * (bPush ? 1.0f : -1.0f);
        }
    }
    
    // Apply offset
    MR::offsetVerts(Mesh.points, offsets, Mesh);
    
    // Update structures
    Mesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(Mesh);
}

void FBrushOperation::ApplySmooth(
    MR::Mesh& Mesh,
    const MR::Vector3f& Center,
    float Radius,
    int32 Iterations
)
{
    // Find affected region
    auto projection = MR::findProjection(Center, Mesh);
    if (!projection) return;
    
    MR::VertBitSet startVerts;
    MR::VertId v0, v1, v2;
    Mesh.topology.getTriVerts(projection->proj.face, v0, v1, v2);
    startVerts.set(v0);
    startVerts.set(v1);
    startVerts.set(v2);
    
    MR::VertScalars distances = MR::computeSurfaceDistances(
        Mesh, startVerts, Radius
    );
    
    // Build region
    MR::VertBitSet region;
    for (MR::VertId vid : Mesh.topology.getValidVerts())
    {
        if (distances[vid] < Radius)
        {
            region.set(vid);
        }
    }
    
    // Apply relaxation
    MR::relax(Mesh, &region, Iterations);
    
    // Update
    Mesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(Mesh);
}
```

---

## UE5 Integration Points

### 1. Tool Mode Integration

```cpp
// In your UInteractiveTool subclass
class MESHLIBBRUSHEDITOR_API UMeshLibBrushTool : public UInteractiveTool
{
    GENERATED_BODY()
    
public:
    virtual void OnTick(float DeltaTime) override;
    virtual void OnBeginDrag(const FRay& Ray) override;
    virtual void OnUpdateDrag(const FRay& Ray) override;
    virtual void OnEndDrag(const FRay& Ray) override;
    
private:
    UPROPERTY()
    UDynamicMesh* TargetMesh;
    
    FVector LastBrushPosition;
    bool bIsDragging;
};

void UMeshLibBrushTool::OnUpdateDrag(const FRay& Ray)
{
    if (!TargetMesh) return;
    
    // Ray-mesh intersection
    FVector HitPoint;
    if (RayIntersectMesh(Ray, TargetMesh, HitPoint))
    {
        // Apply brush operation
        FBrushOperation::ApplyBrush(
            TargetMesh->GetMeshRef(),
            HitPoint,
            BrushRadius,
            BrushStrength,
            CurrentBrushMode
        );
        
        // Notify mesh changed
        TargetMesh->GetMesh()->MarkMeshModified();
        OnMeshChanged.Broadcast();
    }
}
```

### 2. Undo/Redo with Transaction System

```cpp
class FMeshLibBrushTransaction : public FToolCommandChange
{
public:
    FMeshLibBrushTransaction(
        const UE::Geometry::FDynamicMesh3& BeforeMesh
    )
        : BeforeMeshState(BeforeMesh)
    {
    }
    
    virtual void Apply(UObject* Object) override
    {
        UDynamicMesh* DynMesh = Cast<UDynamicMesh>(Object);
        if (DynMesh)
        {
            // Save current for redo
            AfterMeshState = DynMesh->GetMeshRef();
            
            // Apply undo
            DynMesh->SetMesh(BeforeMeshState);
        }
    }
    
    virtual void Revert(UObject* Object) override
    {
        UDynamicMesh* DynMesh = Cast<UDynamicMesh>(Object);
        if (DynMesh)
        {
            DynMesh->SetMesh(AfterMeshState);
        }
    }
    
private:
    UE::Geometry::FDynamicMesh3 BeforeMeshState;
    UE::Geometry::FDynamicMesh3 AfterMeshState;
};

// Usage in tool
void UMeshLibBrushTool::OnBeginDrag(const FRay& Ray)
{
    // Capture mesh state for undo
    BeforeState = TargetMesh->GetMeshRef();
}

void UMeshLibBrushTool::OnEndDrag(const FRay& Ray)
{
    // Emit transaction
    GetToolManager()->EmitObjectChange(
        TargetMesh,
        MakeUnique<FMeshLibBrushTransaction>(BeforeState),
        LOCTEXT("BrushStroke", "Brush Stroke")
    );
}
```

---

## Performance Considerations for UE5

### 1. Threading

MeshLib operations can be expensive. Use UE5's task system:

```cpp
#include "Async/Async.h"

void ApplyBrushAsync(
    UDynamicMesh* Mesh,
    FVector BrushPos,
    float Radius,
    float Strength
)
{
    // Copy mesh data
    auto MeshCopy = Mesh->GetMeshRef();
    
    // Run on background thread
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [=]()
    {
        // Perform heavy MeshLib operations
        MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(MeshCopy);
        // ... apply brush ...
        
        // Return to game thread for mesh update
        AsyncTask(ENamedThreads::GameThread, [=]()
        {
            FMeshLibWrapper::ConvertToUE5(MRMesh, Mesh->GetMeshRef());
            Mesh->GetMesh()->MarkMeshModified();
        });
    });
}
```

### 2. Progressive Updates

For large meshes, update incrementally:

```cpp
// Process in chunks
const int32 VertsPerFrame = 1000;
int32 CurrentOffset = 0;

void TickBrushUpdate()
{
    // Process limited vertices per frame
    // to maintain framerate
}
```

### 3. AABB Tree Caching

```cpp
class FMeshLibCache
{
public:
    MR::Mesh* CachedMesh = nullptr;
    std::shared_ptr<MR::AABBTree> CachedTree;
    
    void UpdateCache(const UE::Geometry::FDynamicMesh3& DynMesh)
    {
        if (!CachedMesh)
        {
            CachedMesh = new MR::Mesh(
                FMeshLibWrapper::ConvertToMeshLib(DynMesh)
            );
            CachedTree = std::make_shared<MR::AABBTree>(*CachedMesh);
        }
        else
        {
            // Just refit, don't rebuild
            CachedTree->refit();
        }
    }
};
```

---

## Blueprint Exposure

For UE5 Blueprint support:

```cpp
UCLASS()
class MESHLIBBRUSHEDITOR_API UMeshLibBrushFunctionLibrary 
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    UFUNCTION(BlueprintCallable, Category = "MeshLib|Brush")
    static bool ApplyBrushStroke(
        UDynamicMesh* TargetMesh,
        FVector BrushCenter,
        float BrushRadius,
        float BrushStrength,
        EBrushMode Mode
    );
    
    UFUNCTION(BlueprintCallable, Category = "MeshLib|Brush")
    static void SmoothMeshRegion(
        UDynamicMesh* TargetMesh,
        FVector Center,
        float Radius,
        int32 Iterations = 3
    );
};
```

---

## Common Issues & Solutions

### Issue: Coordinate System Mismatch

**Symptom**: Brush appears in wrong location  
**Solution**: Verify coordinate system conversion, apply transform matrices

```cpp
// Apply component transform
FTransform ComponentTransform = MeshComponent->GetComponentTransform();
FVector WorldPos = ComponentTransform.TransformPosition(LocalPos);
```

### Issue: Performance Drops

**Symptom**: Lag during brush strokes  
**Solution**: 
- Use refit() not rebuild AABB
- Limit brush radius
- Process async on background thread
- Use progressive updates

### Issue: Mesh Corruption

**Symptom**: Holes or invalid topology  
**Solution**:
- Validate mesh before/after MeshLib operations
- Check for degenerate triangles
- Ensure proper ID mapping during conversion

---

## Example: Complete UE5 Brush Tool

See the companion file `UE5_BRUSH_TOOL_EXAMPLE.cpp` for a complete working example of a UE5 Interactive Tool using MeshLib.

---

## Testing Strategy

### 1. Unit Tests

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMeshLibConversionTest,
    "MeshLibBrushEditor.Conversion.BasicMesh",
    EAutomationTestFlags::ApplicationContextMask | 
    EAutomationTestFlags::ProductFilter
)

bool FMeshLibConversionTest::RunTest(const FString& Parameters)
{
    // Create simple UE5 mesh
    UE::Geometry::FDynamicMesh3 UEMesh;
    // ... build test mesh ...
    
    // Convert to MeshLib
    MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(UEMesh);
    
    // Verify vertex count
    TestEqual("Vertex Count", 
        MRMesh.topology.numValidVerts(), 
        UEMesh.VertexCount()
    );
    
    // Convert back
    UE::Geometry::FDynamicMesh3 ResultMesh;
    FMeshLibWrapper::ConvertToUE5(MRMesh, ResultMesh);
    
    // Verify roundtrip
    TestEqual("Roundtrip Vertex Count",
        ResultMesh.VertexCount(),
        UEMesh.VertexCount()
    );
    
    return true;
}
```

---

## Deployment

### Packaging MeshLib with Plugin

1. **Include binaries** in `ThirdParty/MeshLib/Lib/{Platform}/`
2. **Add to RuntimeDependencies** in Build.cs
3. **Test on target platform** (Windows, Linux, Mac)
4. **Verify DLL/SO loading** at runtime

### Distribution

For plugin marketplace or GitHub release:
- Include MeshLib license in your plugin
- Document MeshLib version compatibility
- Provide platform-specific binaries
- Include example content

---

## Resources

- **MeshLib Documentation**: https://meshlib.io/documentation/
- **MeshLib Source**: https://github.com/MeshInspector/MeshLib
- **UE5 Geometry Framework**: Unreal Engine Documentation
- **Full API Reference**: See `MESHLIB_APIS_FOR_BRUSH_EDITORS.md`

---

**Document Version**: 1.0  
**Date**: 2026-01-29  
**Target**: UE5 Brush Editor Integration (https://github.com/AlleyKatPr0/ue5-brush-editor)
