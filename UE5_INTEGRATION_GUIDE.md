# MeshLib Integration Guide for UE5 Blockout Level Editor

## Overview

This guide documents how to integrate MeshLib's mesh processing capabilities into an Unreal Engine 5 blockout/greybox level editor. It covers data type conversions, plugin architecture, and UE5-specific considerations tailored for rapid level prototyping.

## Project Context

**Target**: UE5 Brush Editor (https://github.com/AlleyKatPr0/ue5-brush-editor)  
**Purpose**: Blockout/greybox/whitebox level design tool for rapid prototyping  
**Use Case**: Quickly create and modify level geometry for testing game flow and layout  
**Integration Goal**: Leverage MeshLib for fast geometric operations on primitive shapes  
**Approach**: Create a UE5 plugin wrapper around MeshLib APIs optimized for level design workflows

---

## Level Design Workflow Context

### What is Blockout/Greybox/Whitebox Level Design?

**Blockout** (also called greybox or whitebox) is the process of creating rough level geometry to:
- Test gameplay flow and pacing
- Establish scale and spatial relationships
- Prototype level layouts before art production
- Iterate quickly on level design ideas
- Test collision and navigation

**Key Characteristics:**
- Simple primitive shapes (boxes, planes, wedges)
- Fast creation and modification
- No textures or detailed art
- Focus on functionality over aesthetics
- Rapid iteration cycles

### MeshLib's Role in Level Design

For blockout workflows, MeshLib provides:
1. **Fast Boolean Operations** - Combine, subtract, intersect primitive shapes (CSG-like)
2. **Mesh Simplification** - Optimize blockout geometry for performance
3. **Snapping and Alignment** - Precise positioning of geometry
4. **Quick Modifications** - Push/pull faces, extrude, bevel
5. **Collision Generation** - Create simplified collision meshes
6. **Mesh Repair** - Fix issues in procedurally generated geometry

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

## Level Design Operations for UE5

### Wrapper Class for Blockout Tools

```cpp
// MeshLibWrapper.h
#pragma once

#include "CoreMinimal.h"
#include "MRMesh/MRMesh.h"
#include "DynamicMesh/DynamicMesh3.h"

UENUM(BlueprintType)
enum class EBlockoutOperation : uint8
{
    // Face operations (for box/primitive editing)
    ExtrudeFace     UMETA(DisplayName = "Extrude Face"),
    PushFace        UMETA(DisplayName = "Push Face"),
    PullFace        UMETA(DisplayName = "Pull Face"),
    
    // Edge operations
    BevelEdge       UMETA(DisplayName = "Bevel Edge"),
    
    // Mesh operations
    BooleanUnion    UMETA(DisplayName = "Boolean Union"),
    BooleanSubtract UMETA(DisplayName = "Boolean Subtract"),
    BooleanIntersect UMETA(DisplayName = "Boolean Intersect"),
    
    // Cleanup
    Simplify        UMETA(DisplayName = "Simplify"),
    Smooth          UMETA(DisplayName = "Smooth")
};

class MESHLIBBRUSHEDITOR_API FBlockoutOperation
{
public:
    // Face-level operations for box editing
    static bool ExtrudeFace(
        UE::Geometry::FDynamicMesh3& DynMesh,
        int32 FaceID,
        float ExtrudeDistance
    );
    
    static bool PushPullFace(
        UE::Geometry::FDynamicMesh3& DynMesh,
        int32 FaceID,
        float Distance
    );
    
    // Boolean operations for combining primitives
    static bool BooleanOperation(
        const UE::Geometry::FDynamicMesh3& MeshA,
        const UE::Geometry::FDynamicMesh3& MeshB,
        EBlockoutOperation Operation,
        UE::Geometry::FDynamicMesh3& Result
    );
    
    // Simplification for optimizing blockout geometry
    static bool SimplifyMesh(
        UE::Geometry::FDynamicMesh3& DynMesh,
        float TargetReduction
    );
    
private:
    static void ApplyFaceOffset(
        MR::Mesh& Mesh,
        MR::FaceId Face,
        float Distance
    );
};
```

### Implementation

```cpp
// BlockoutOperations.cpp
#include "BlockoutOperations.h"
#include "MRMesh/MRMeshProject.h"
#include "MRMesh/MRBoolean.h"           // Boolean operations
#include "MRMesh/MRMeshDecimate.h"       // Simplification
#include "MRMesh/MROffsetMesh.h"         // Face offsetting
#include "MRMesh/MRMeshNormals.h"

bool FBlockoutOperation::ExtrudeFace(
    UE::Geometry::FDynamicMesh3& DynMesh,
    int32 FaceID,
    float ExtrudeDistance
)
{
    // Convert to MeshLib
    MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(DynMesh);
    
    // Get face vertices
    MR::FaceId fid(FaceID);
    if (!MRMesh.topology.hasFace(fid))
        return false;
    
    // Get face normal
    MR::Vector3f normal = MRMesh.normal(fid);
    
    // Get vertices of the face
    MR::VertId v0, v1, v2;
    MRMesh.topology.getTriVerts(fid, v0, v1, v2);
    
    // Duplicate vertices for extrusion
    MR::VertId newV0 = MRMesh.topology.addVert(MRMesh.points[v0] + normal * ExtrudeDistance);
    MR::VertId newV1 = MRMesh.topology.addVert(MRMesh.points[v1] + normal * ExtrudeDistance);
    MR::VertId newV2 = MRMesh.topology.addVert(MRMesh.points[v2] + normal * ExtrudeDistance);
    
    // Create side faces connecting old and new vertices
    MRMesh.topology.addTriWithVerts(v0, v1, newV1);
    MRMesh.topology.addTriWithVerts(v0, newV1, newV0);
    MRMesh.topology.addTriWithVerts(v1, v2, newV2);
    MRMesh.topology.addTriWithVerts(v1, newV2, newV1);
    MRMesh.topology.addTriWithVerts(v2, v0, newV0);
    MRMesh.topology.addTriWithVerts(v2, newV0, newV2);
    
    // Replace original face with new face at extruded position
    MRMesh.topology.setTriVerts(fid, newV0, newV1, newV2);
    
    // Update normals
    MR::computeMeshNormals(MRMesh);
    
    // Convert back
    FMeshLibWrapper::ConvertToUE5(MRMesh, DynMesh);
    
    return true;
}

bool FBlockoutOperation::PushPullFace(
    UE::Geometry::FDynamicMesh3& DynMesh,
    int32 FaceID,
    float Distance
)
{
    MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(DynMesh);
    
    MR::FaceId fid(FaceID);
    if (!MRMesh.topology.hasFace(fid))
        return false;
    
    // Get face normal
    MR::Vector3f normal = MRMesh.normal(fid);
    MR::Vector3f offset = normal * Distance;
    
    // Move face vertices
    MR::VertId v0, v1, v2;
    MRMesh.topology.getTriVerts(fid, v0, v1, v2);
    
    MRMesh.points[v0] += offset;
    MRMesh.points[v1] += offset;
    MRMesh.points[v2] += offset;
    
    // Update structures
    MRMesh.getAABBTreeNotCreate()->refit();
    MR::computeMeshNormals(MRMesh);
    
    FMeshLibWrapper::ConvertToUE5(MRMesh, DynMesh);
    return true;
}

bool FBlockoutOperation::BooleanOperation(
    const UE::Geometry::FDynamicMesh3& MeshA,
    const UE::Geometry::FDynamicMesh3& MeshB,
    EBlockoutOperation Operation,
    UE::Geometry::FDynamicMesh3& Result
)
{
    // Convert meshes
    MR::Mesh MRA = FMeshLibWrapper::ConvertToMeshLib(MeshA);
    MR::Mesh MRB = FMeshLibWrapper::ConvertToMeshLib(MeshB);
    
    // Perform boolean operation
    MR::BooleanOperation boolOp;
    switch (Operation)
    {
        case EBlockoutOperation::BooleanUnion:
            boolOp = MR::BooleanOperation::Union;
            break;
        case EBlockoutOperation::BooleanSubtract:
            boolOp = MR::BooleanOperation::DifferenceAB;
            break;
        case EBlockoutOperation::BooleanIntersect:
            boolOp = MR::BooleanOperation::Intersection;
            break;
        default:
            return false;
    }
    
    // Execute boolean
    auto boolResult = MR::boolean(MRA, MRB, boolOp);
    if (!boolResult.has_value())
        return false;
    
    // Convert result back
    FMeshLibWrapper::ConvertToUE5(*boolResult, Result);
    return true;
}

bool FBlockoutOperation::SimplifyMesh(
    UE::Geometry::FDynamicMesh3& DynMesh,
    float TargetReduction
)
{
    MR::Mesh MRMesh = FMeshLibWrapper::ConvertToMeshLib(DynMesh);
    
    // Calculate target face count
    int32 CurrentFaces = MRMesh.topology.numValidFaces();
    int32 TargetFaces = FMath::Max(4, (int32)(CurrentFaces * (1.0f - TargetReduction)));
    
    // Simplify
    MR::DecimateSettings settings;
    settings.maxDeletedFaces = CurrentFaces - TargetFaces;
    
    MR::decimateMesh(MRMesh, settings);
    
    // Update and convert back
    MR::computeMeshNormals(MRMesh);
    FMeshLibWrapper::ConvertToUE5(MRMesh, DynMesh);
    
    return true;
}
```

---

## Level Design-Specific Operations

### 1. Box Primitive Creation

```cpp
// Helper for creating standard blockout primitives
class FBlockoutPrimitives
{
public:
    static UE::Geometry::FDynamicMesh3 CreateBox(
        FVector Dimensions = FVector(100, 100, 100)
    )
    {
        UE::Geometry::FDynamicMesh3 Mesh;
        
        // Create basic box with 8 vertices, 12 triangles
        // (Standard UE5 box creation)
        
        return Mesh;
    }
    
    static UE::Geometry::FDynamicMesh3 CreateWedge(
        FVector Dimensions = FVector(100, 100, 100)
    )
    {
        // Create triangular prism (common in level design)
        UE::Geometry::FDynamicMesh3 Mesh;
        // ... implementation
        return Mesh;
    }
    
    static UE::Geometry::FDynamicMesh3 CreateStairs(
        int32 Steps,
        FVector StepDimensions
    )
    {
        // Generate stairs procedurally
        UE::Geometry::FDynamicMesh3 Mesh;
        
        for (int32 i = 0; i < Steps; ++i)
        {
            // Create each step as a box
            auto Step = CreateBox(StepDimensions);
            // Position and combine
            // (Use boolean union)
        }
        
        return Mesh;
    }
};
```

### 2. CSG-Style Workflow

```cpp
// Level design workflow: Build complex geometry from primitives
class FLevelGeometryBuilder
{
public:
    UE::Geometry::FDynamicMesh3 CurrentMesh;
    
    // Additive workflow
    void AddPrimitive(const UE::Geometry::FDynamicMesh3& Primitive)
    {
        UE::Geometry::FDynamicMesh3 Result;
        FBlockoutOperation::BooleanOperation(
            CurrentMesh,
            Primitive,
            EBlockoutOperation::BooleanUnion,
            Result
        );
        CurrentMesh = MoveTemp(Result);
    }
    
    // Subtractive workflow (carve out spaces)
    void SubtractPrimitive(const UE::Geometry::FDynamicMesh3& Primitive)
    {
        UE::Geometry::FDynamicMesh3 Result;
        FBlockoutOperation::BooleanOperation(
            CurrentMesh,
            Primitive,
            EBlockoutOperation::BooleanSubtract,
            Result
        );
        CurrentMesh = MoveTemp(Result);
    }
    
    // Create a room with door opening
    void CreateRoomWithDoor()
    {
        // 1. Create outer box (room)
        auto Room = FBlockoutPrimitives::CreateBox(FVector(500, 500, 300));
        CurrentMesh = Room;
        
        // 2. Subtract inner box (hollow out)
        auto Interior = FBlockoutPrimitives::CreateBox(FVector(480, 480, 280));
        SubtractPrimitive(Interior);
        
        // 3. Subtract door opening
        auto Door = FBlockoutPrimitives::CreateBox(FVector(100, 50, 200));
        // Position door appropriately
        SubtractPrimitive(Door);
        
        // 4. Simplify if needed
        FBlockoutOperation::SimplifyMesh(CurrentMesh, 0.1f); // 10% reduction
    }
};
```

---

## UE5 Integration Points

### 1. Tool Mode Integration for Level Design

```cpp
// In your UInteractiveTool subclass
class MESHLIBBRUSHEDITOR_API UBlockoutLevelTool : public UInteractiveTool
{
    GENERATED_BODY()
    
public:
    virtual void OnTick(float DeltaTime) override;
    
    // Face selection and manipulation
    virtual void OnClickFace(const FRay& Ray) override;
    virtual void OnDragFace(const FRay& Ray, float DragDistance) override;
    
    // Primitive placement
    virtual void PlacePrimitive(const FVector& Location) override;
    
private:
    UPROPERTY()
    UDynamicMesh* WorkingMesh;
    
    // Current operation mode
    EBlockoutOperation CurrentOperation;
    
    // Selected face for push/pull/extrude
    int32 SelectedFaceID = -1;
    
    // Primitive library for quick placement
    TArray<UE::Geometry::FDynamicMesh3> PrimitiveTemplates;
};

void UBlockoutLevelTool::OnClickFace(const FRay& Ray)
{
    if (!WorkingMesh) return;
    
    // Ray-mesh intersection to select face
    FVector HitPoint;
    int32 HitFaceID;
    if (RayIntersectMesh(Ray, WorkingMesh, HitPoint, HitFaceID))
    {
        SelectedFaceID = HitFaceID;
        
        // Highlight selected face in viewport
        UpdateFaceHighlight(HitFaceID);
    }
}

void UBlockoutLevelTool::OnDragFace(const FRay& Ray, float DragDistance)
{
    if (SelectedFaceID < 0 || !WorkingMesh) return;
    
    // Apply operation based on mode
    switch (CurrentOperation)
    {
        case EBlockoutOperation::ExtrudeFace:
            FBlockoutOperation::ExtrudeFace(
                WorkingMesh->GetMeshRef(),
                SelectedFaceID,
                DragDistance
            );
            break;
            
        case EBlockoutOperation::PushFace:
            FBlockoutOperation::PushPullFace(
                WorkingMesh->GetMeshRef(),
                SelectedFaceID,
                DragDistance
            );
            break;
            
        // ... other operations
    }
    
    // Notify mesh changed
    WorkingMesh->GetMesh()->MarkMeshModified();
    OnMeshChanged.Broadcast();
}

void UBlockoutLevelTool::PlacePrimitive(const FVector& Location)
{
    // Quick primitive placement for level design
    auto Primitive = FBlockoutPrimitives::CreateBox(CurrentPrimitiveSize);
    
    // Transform to location
    TransformMesh(Primitive, Location, CurrentRotation);
    
    // Add to working mesh via boolean union
    UE::Geometry::FDynamicMesh3 Result;
    FBlockoutOperation::BooleanOperation(
        WorkingMesh->GetMeshRef(),
        Primitive,
        EBlockoutOperation::BooleanUnion,
        Result
    );
    
    WorkingMesh->SetMesh(MoveTemp(Result));
}
```

### 2. Quick Primitive Palette

```cpp
// Widget for rapid primitive selection
class UBlockoutPrimitiveWidget : public UUserWidget
{
    GENERATED_BODY()
    
protected:
    UPROPERTY(EditAnywhere, Category = "Primitives")
    TArray<FPrimitiveDefinition> AvailablePrimitives;
    
    UFUNCTION(BlueprintCallable)
    void SelectPrimitive(EPrimitiveType Type, FVector Dimensions);
    
private:
    // Common blockout shapes
    void InitializePrimitives()
    {
        AvailablePrimitives.Add({"Box", EPrimitiveType::Box, FVector(100, 100, 100)});
        AvailablePrimitives.Add({"Wall", EPrimitiveType::Box, FVector(400, 20, 300)});
        AvailablePrimitives.Add({"Floor", EPrimitiveType::Box, FVector(400, 400, 20)});
        AvailablePrimitives.Add({"Ramp", EPrimitiveType::Wedge, FVector(400, 200, 100)});
        AvailablePrimitives.Add({"Stairs", EPrimitiveType::Stairs, FVector(200, 100, 20)});
        AvailablePrimitives.Add({"Pillar", EPrimitiveType::Box, FVector(50, 50, 300)});
        AvailablePrimitives.Add({"Door Frame", EPrimitiveType::Box, FVector(150, 20, 250)});
    }
};
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

## Performance Considerations for Level Design

### 1. Optimizing Boolean Operations

Boolean operations (Union, Subtract, Intersect) are the most expensive operations in blockout workflows.

```cpp
// Cache primitives for reuse
class FPrimitiveCache
{
    TMap<FString, UE::Geometry::FDynamicMesh3> CachedPrimitives;
    
public:
    const UE::Geometry::FDynamicMesh3& GetPrimitive(
        const FString& Name,
        FVector Dimensions
    )
    {
        FString Key = FString::Printf(TEXT("%s_%s"), 
            *Name, *Dimensions.ToString());
            
        if (!CachedPrimitives.Contains(Key))
        {
            // Create and cache
            auto Primitive = FBlockoutPrimitives::CreateBox(Dimensions);
            CachedPrimitives.Add(Key, MoveTemp(Primitive));
        }
        
        return CachedPrimitives[Key];
    }
};
```

### 2. Progressive Boolean Operations

For complex level geometry, perform booleans incrementally:

```cpp
// Instead of one giant boolean with 100 primitives
// Break into smaller operations
void BuildComplexLevel()
{
    // Method 1: Sequential (slow but stable)
    for (auto& Primitive : Primitives)
    {
        AddPrimitive(Primitive);
    }
    
    // Method 2: Hierarchical (faster)
    // Combine primitives in groups, then combine groups
    TArray<UE::Geometry::FDynamicMesh3> Groups;
    
    // Create 10 groups of 10 primitives each
    for (int32 i = 0; i < 10; ++i)
    {
        UE::Geometry::FDynamicMesh3 Group;
        for (int32 j = 0; j < 10; ++j)
        {
            // Boolean within group
        }
        Groups.Add(MoveTemp(Group));
    }
    
    // Combine groups
    for (auto& Group : Groups)
    {
        AddPrimitive(Group);
    }
}
```

### 3. Level of Detail for Blockout

```cpp
// Create simplified collision meshes
void GenerateCollisionMesh()
{
    auto VisualMesh = GetCurrentLevelGeometry();
    
    // Create highly simplified version for collision
    auto CollisionMesh = VisualMesh;
    FBlockoutOperation::SimplifyMesh(CollisionMesh, 0.7f); // 70% reduction
    
    // Use simplified mesh for physics
    SetCollisionMesh(CollisionMesh);
}
```

### 4. Deferred Boolean Operations

```cpp
// Don't recalculate booleans on every change
class FDeferredBooleanSystem
{
    TArray<FPendingBooleanOp> PendingOps;
    bool bNeedsUpdate = false;
    
public:
    void QueueOperation(const FPendingBooleanOp& Op)
    {
        PendingOps.Add(Op);
        bNeedsUpdate = true;
    }
    
    void ProcessQueue()
    {
        if (!bNeedsUpdate) return;
        
        // Batch process all pending booleans
        for (auto& Op : PendingOps)
        {
            ExecuteBoolean(Op);
        }
        
        PendingOps.Empty();
        bNeedsUpdate = false;
    }
};
```

---

## Blueprint Exposure

For UE5 Blueprint support in level design tools:

```cpp
UCLASS()
class MESHLIBBRUSHEDITOR_API UBlockoutFunctionLibrary 
    : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
    
public:
    // Primitive creation
    UFUNCTION(BlueprintCallable, Category = "Blockout|Primitives")
    static UDynamicMesh* CreateBlockoutBox(
        FVector Dimensions = FVector(100, 100, 100)
    );
    
    UFUNCTION(BlueprintCallable, Category = "Blockout|Primitives")
    static UDynamicMesh* CreateBlockoutWall(
        float Length = 400.f,
        float Height = 300.f,
        float Thickness = 20.f
    );
    
    UFUNCTION(BlueprintCallable, Category = "Blockout|Primitives")
    static UDynamicMesh* CreateBlockoutStairs(
        int32 Steps = 10,
        FVector StepDimensions = FVector(100, 100, 20)
    );
    
    // Boolean operations
    UFUNCTION(BlueprintCallable, Category = "Blockout|Operations")
    static UDynamicMesh* CombineMeshes(
        UDynamicMesh* MeshA,
        UDynamicMesh* MeshB,
        EBlockoutOperation Operation
    );
    
    // Face operations
    UFUNCTION(BlueprintCallable, Category = "Blockout|Operations")
    static bool ExtrudeFace(
        UDynamicMesh* Mesh,
        int32 FaceID,
        float Distance
    );
    
    // Utility
    UFUNCTION(BlueprintCallable, Category = "Blockout|Utility")
    static bool SimplifyForPlaytest(
        UDynamicMesh* Mesh,
        float ReductionPercent = 50.f
    );
    
    UFUNCTION(BlueprintCallable, Category = "Blockout|Utility")
    static UDynamicMesh* CreateCollisionMesh(
        UDynamicMesh* SourceMesh,
        float SimplificationLevel = 0.7f
    );
};
```

### Blueprint Workflow Examples

```cpp
// Example: Room generator accessible from Blueprint
UFUNCTION(BlueprintCallable, Category = "Blockout|Generators")
static UDynamicMesh* GenerateRoom(
    FVector RoomSize,
    float WallThickness,
    bool bIncludeCeiling,
    TArray<FDoorwayDefinition> Doorways
)
{
    auto Mesh = CreateBlockoutBox(RoomSize);
    
    // Hollow out interior
    FVector InteriorSize = RoomSize - FVector(WallThickness * 2);
    auto Interior = CreateBlockoutBox(InteriorSize);
    Mesh = CombineMeshes(Mesh, Interior, EBlockoutOperation::BooleanSubtract);
    
    // Add doorways
    for (auto& Door : Doorways)
    {
        auto DoorCutout = CreateBlockoutBox(Door.Size);
        // Position appropriately
        Mesh = CombineMeshes(Mesh, DoorCutout, EBlockoutOperation::BooleanSubtract);
    }
    
    // Remove ceiling if requested
    if (!bIncludeCeiling)
    {
        auto CeilingCutout = CreateBlockoutBox(
            FVector(RoomSize.X, RoomSize.Y, WallThickness)
        );
        // Position at top
        Mesh = CombineMeshes(Mesh, CeilingCutout, EBlockoutOperation::BooleanSubtract);
    }
    
    return Mesh;
}
```

---

## Level Design Workflow Patterns

### Pattern 1: Additive Level Building

```cpp
// Start with primitives, build up complexity
void BuildLevelAdditively()
{
    // 1. Create floor
    auto Floor = FBlockoutPrimitives::CreateBox(FVector(2000, 2000, 50));
    LevelMesh = Floor;
    
    // 2. Add walls
    for (auto& WallSpec : WallDefinitions)
    {
        auto Wall = FBlockoutPrimitives::CreateBox(WallSpec.Dimensions);
        // Transform to position
        TransformMesh(Wall, WallSpec.Location, WallSpec.Rotation);
        
        // Union with level
        UE::Geometry::FDynamicMesh3 Result;
        FBlockoutOperation::BooleanOperation(
            LevelMesh, Wall,
            EBlockoutOperation::BooleanUnion,
            Result
        );
        LevelMesh = MoveTemp(Result);
    }
    
    // 3. Add details (stairs, platforms, etc.)
    // ...
}
```

### Pattern 2: Subtractive Level Carving

```cpp
// Start with solid block, carve out spaces
void CarveOutLevel()
{
    // 1. Start with large solid block
    auto Solid = FBlockoutPrimitives::CreateBox(FVector(3000, 3000, 500));
    LevelMesh = Solid;
    
    // 2. Carve out rooms
    for (auto& RoomSpec : RoomDefinitions)
    {
        auto RoomCutout = FBlockoutPrimitives::CreateBox(RoomSpec.Size);
        TransformMesh(RoomCutout, RoomSpec.Location, FRotator::ZeroRotator);
        
        // Subtract from level
        UE::Geometry::FDynamicMesh3 Result;
        FBlockoutOperation::BooleanOperation(
            LevelMesh, RoomCutout,
            EBlockoutOperation::BooleanSubtract,
            Result
        );
        LevelMesh = MoveTemp(Result);
    }
    
    // 3. Carve corridors connecting rooms
    // ...
    
    // 4. Add doorways
    // ...
}
```

### Pattern 3: Modular Level Assembly

```cpp
// Pre-built modules combined at runtime
class FModularLevelBuilder
{
    TMap<FString, UE::Geometry::FDynamicMesh3> ModuleLibrary;
    
public:
    void InitializeLibrary()
    {
        // Pre-create common modules
        ModuleLibrary.Add("Room_Small", GenerateRoomModule(FVector(500, 500, 300)));
        ModuleLibrary.Add("Room_Large", GenerateRoomModule(FVector(1000, 1000, 300)));
        ModuleLibrary.Add("Corridor_Straight", GenerateCorridorModule(FVector(400, 200, 300)));
        ModuleLibrary.Add("Corridor_Corner", GenerateCornerModule());
        ModuleLibrary.Add("Stairwell", GenerateStairwellModule());
    }
    
    void PlaceModule(const FString& ModuleName, FVector Location, FRotator Rotation)
    {
        if (!ModuleLibrary.Contains(ModuleName))
            return;
            
        auto Module = ModuleLibrary[ModuleName];
        TransformMesh(Module, Location, Rotation);
        
        // Add to level
        UE::Geometry::FDynamicMesh3 Result;
        FBlockoutOperation::BooleanOperation(
            LevelMesh, Module,
            EBlockoutOperation::BooleanUnion,
            Result
        );
        LevelMesh = MoveTemp(Result);
    }
    
private:
    UE::Geometry::FDynamicMesh3 GenerateRoomModule(FVector Size)
    {
        // Generate standard room with door sockets
        // ...
    }
};
```

---

## Common Issues & Solutions

### Issue: Boolean Operation Failures

**Symptom**: Boolean operations return null or invalid geometry  
**Solution**: 
- Ensure meshes are manifold (watertight)
- Check for degenerate triangles
- Simplify meshes before boolean operations
- Use MeshLib's mesh repair functions

```cpp
// Validate and repair before boolean
void PrepareForBoolean(MR::Mesh& Mesh)
{
    // Remove degenerate faces
    MR::removeIsolatedVerts(Mesh);
    
    // Ensure manifold
    if (!MR::isManifold(Mesh))
    {
        // Attempt repair
        MR::makeManifold(Mesh);
    }
}
```

### Issue: Performance Drops with Many Primitives

**Symptom**: Level editor becomes slow with complex geometry  
**Solution**: 
- Use hierarchical boolean operations
- Cache primitive meshes
- Defer boolean calculations
- Simplify intermediate results

### Issue: Z-Fighting in Blockout

**Symptom**: Flickering where primitives overlap  
**Solution**:
- Add small offset (0.1-1.0 units) between primitives
- Use proper boolean operations instead of overlapping
- Snap geometry to grid

```cpp
// Grid snapping for clean geometry
FVector SnapToGrid(FVector Position, float GridSize = 10.0f)
{
    return FVector(
        FMath::Round NearestMultiple(Position.X, GridSize),
        FMath::RoundToNearestMultiple(Position.Y, GridSize),
        FMath::RoundToNearestMultiple(Position.Z, GridSize)
    );
}
```

### Issue: Collision Issues in Blockout

**Symptom**: Character falls through blockout geometry  
**Solution**:
- Generate simplified collision meshes
- Ensure proper collision channel setup
- Use complex collision for detailed blockout

---

## Testing Strategy for Level Design Tools

### 1. Primitive Creation Tests

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPrimitiveCreationTest,
    "BlockoutEditor.Primitives.CreateBox",
    EAutomationTestFlags::ApplicationContextMask | 
    EAutomationTestFlags::ProductFilter
)

bool FPrimitiveCreationTest::RunTest(const FString& Parameters)
{
    // Create box
    auto Box = FBlockoutPrimitives::CreateBox(FVector(100, 100, 100));
    
    // Verify it's valid
    TestTrue("Box has vertices", Box.VertexCount() > 0);
    TestTrue("Box has triangles", Box.TriangleCount() > 0);
    TestEqual("Box has 12 triangles", Box.TriangleCount(), 12);
    
    // Verify manifold
    TestTrue("Box is manifold", Box.IsValid());
    
    return true;
}
```

### 2. Boolean Operation Tests

```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBooleanOperationTest,
    "BlockoutEditor.Boolean.Union",
    EAutomationTestFlags::ApplicationContextMask | 
    EAutomationTestFlags::ProductFilter
)

bool FBooleanOperationTest::RunTest(const FString& Parameters)
{
    // Create two boxes
    auto BoxA = FBlockoutPrimitives::CreateBox(FVector(100, 100, 100));
    auto BoxB = FBlockoutPrimitives::CreateBox(FVector(100, 100, 100));
    
    // Offset BoxB slightly
    // ... transform BoxB ...
    
    // Perform union
    UE::Geometry::FDynamicMesh3 Result;
    bool bSuccess = FBlockoutOperation::BooleanOperation(
        BoxA, BoxB,
        EBlockoutOperation::BooleanUnion,
        Result
    );
    
    TestTrue("Boolean succeeded", bSuccess);
    TestTrue("Result is valid", Result.IsValid());
    TestTrue("Result has geometry", Result.TriangleCount() > 0);
    
    return true;
}
```

---

## Example: Complete Blockout Tool

See the companion file `BLOCKOUT_TOOL_EXAMPLE.cpp` for a complete working example of a UE5 Interactive Tool for blockout level design using MeshLib.

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
- Include example blockout content

---

## Resources

- **MeshLib Documentation**: https://meshlib.io/documentation/
- **MeshLib Boolean Operations**: https://meshlib.io/documentation/MRBoolean.html
- **MeshLib Source**: https://github.com/MeshInspector/MeshLib
- **UE5 Geometry Framework**: Unreal Engine Documentation
- **Full API Reference**: See `MESHLIB_APIS_FOR_BRUSH_EDITORS.md`

---

## Quick Start Summary for Blockout Level Design

1. **Setup Plugin**
   - Add MeshLib to ThirdParty/
   - Configure Build.cs with libraries
   
2. **Implement Data Converters**
   - UDynamicMesh ↔ MR::Mesh
   - Test round-trip conversions

3. **Create Primitive Library**
   - Box, Wall, Floor, Ramp, Stairs
   - Cache for performance

4. **Implement Core Operations**
   - Face extrude/push/pull
   - Boolean operations (Union, Subtract, Intersect)
   - Simplification

5. **Build Tool Interface**
   - Face selection
   - Primitive placement
   - Operation mode switching

6. **Optimize Performance**
   - Hierarchical booleans
   - Deferred operations
   - Grid snapping

7. **Test Workflows**
   - Room creation
   - Corridor building
   - Stair placement

---

**Document Version**: 1.0  
**Date**: 2026-01-29  
**Target**: UE5 Blockout Level Editor (https://github.com/AlleyKatPr0/ue5-brush-editor)  
**Purpose**: Rapid level prototyping and greybox/whitebox design
