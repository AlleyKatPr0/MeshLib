/**
 * @file brush_editor_example.cpp
 * @brief Example integration of SurfaceManipulationWidget for brush editor
 * 
 * This example demonstrates how to integrate MeshLib's SurfaceManipulationWidget
 * into a brush editor application. It shows basic setup, configuration, and
 * usage patterns.
 */

#include "MRSurfaceManipulationWidget.h"
#include "MRMesh/MRObjectMesh.h"
#include "MRMesh/MRMesh.h"
#include "MRViewer/MRViewer.h"
#include <memory>
#include <iostream>

namespace BrushEditor {

/**
 * @brief Main brush editor class integrating SurfaceManipulationWidget
 */
class BrushEditorApp {
public:
    BrushEditorApp() = default;
    ~BrushEditorApp() = default;

    /**
     * @brief Initialize the brush editor with a mesh
     * @param mesh The 3D mesh to edit
     * @return true if initialization successful
     */
    bool initialize(std::shared_ptr<MR::Mesh> mesh) {
        if (!mesh) {
            std::cerr << "Error: null mesh provided" << std::endl;
            return false;
        }

        // Create mesh object wrapper
        objectMesh_ = std::make_shared<MR::ObjectMesh>();
        objectMesh_->setMesh(mesh);

        // Create brush widget
        brushWidget_ = std::make_shared<MR::SurfaceManipulationWidget>();
        
        // Initialize widget with mesh
        brushWidget_->init(objectMesh_);

        // Setup default brush settings
        setupDefaultSettings();

        // Enable deviation visualization by default
        brushWidget_->enableDeviationVisualization(true);
        brushWidget_->setDeviationCalculationMethod(
            MR::SurfaceManipulationWidget::DeviationCalculationMethod::ExactDistance
        );

        std::cout << "Brush editor initialized successfully" << std::endl;
        return true;
    }

    /**
     * @brief Setup default brush settings
     */
    void setupDefaultSettings() {
        MR::SurfaceManipulationWidget::Settings settings;
        
        // Default to Add mode
        settings.workMode = MR::SurfaceManipulationWidget::WorkMode::Add;
        
        // Calculate appropriate radius based on mesh size
        float diagonal = objectMesh_->getBoundingBox().diagonal();
        settings.radius = diagonal * 0.02f;  // 2% of mesh diagonal
        settings.editForce = diagonal * 0.01f;  // 1% of diagonal
        
        // Good default values for interactive editing
        settings.sharpness = 50.0f;  // Medium falloff
        settings.relaxForce = 0.2f;
        settings.relaxForceAfterEdit = 0.25f;  // Smooth edges automatically
        
        brushWidget_->setSettings(settings);
        currentSettings_ = settings;
    }

    /**
     * @brief Switch brush mode
     */
    void setMode(MR::SurfaceManipulationWidget::WorkMode mode) {
        currentSettings_.workMode = mode;
        brushWidget_->setSettings(currentSettings_);
        
        const char* modeNames[] = {"Add", "Remove", "Relax", "Laplacian", "Patch"};
        std::cout << "Switched to " << modeNames[static_cast<int>(mode)] << " mode" << std::endl;
    }

    /**
     * @brief Set brush radius
     */
    void setRadius(float radius) {
        if (radius <= 0.0f) {
            std::cerr << "Warning: radius must be positive, clamping to 0.1" << std::endl;
            radius = 0.1f;
        }
        
        float minRadius = brushWidget_->getMinRadius();
        if (radius < minRadius) {
            std::cerr << "Warning: radius below minimum (" << minRadius << "), clamping" << std::endl;
            radius = minRadius;
        }
        
        currentSettings_.radius = radius;
        brushWidget_->setSettings(currentSettings_);
    }

    /**
     * @brief Set brush strength/force
     */
    void setStrength(float strength) {
        if (strength < 0.0f) {
            std::cerr << "Warning: strength must be non-negative, using 0" << std::endl;
            strength = 0.0f;
        }
        
        currentSettings_.editForce = strength;
        brushWidget_->setSettings(currentSettings_);
    }

    /**
     * @brief Set brush sharpness (0-100)
     */
    void setSharpness(float sharpness) {
        // Clamp to valid range
        if (sharpness < 0.0f) sharpness = 0.0f;
        if (sharpness > 100.0f) sharpness = 100.0f;
        
        currentSettings_.sharpness = sharpness;
        brushWidget_->setSettings(currentSettings_);
    }

    /**
     * @brief Set auto-smoothing strength after editing
     */
    void setAutoSmooth(float strength) {
        // Clamp to reasonable range
        if (strength < 0.0f) strength = 0.0f;
        if (strength > 0.5f) {
            std::cerr << "Warning: auto-smooth > 0.5 may be unstable, clamping" << std::endl;
            strength = 0.5f;
        }
        
        currentSettings_.relaxForceAfterEdit = strength;
        brushWidget_->setSettings(currentSettings_);
    }

    /**
     * @brief Lock a region of the mesh to prevent editing
     */
    void lockRegion(const MR::FaceBitSet& faces) {
        brushWidget_->setFixedRegion(faces);
        std::cout << "Locked " << faces.count() << " faces from editing" << std::endl;
    }

    /**
     * @brief Toggle occlusion checking
     * @param ignore If true, allows editing of occluded surfaces
     */
    void setIgnoreOcclusion(bool ignore) {
        brushWidget_->setIgnoreOcclusion(ignore);
        std::cout << "Occlusion " << (ignore ? "ignored" : "respected") << std::endl;
    }

    /**
     * @brief Only edit surfaces facing the camera
     */
    void setEditCodirectionalOnly(bool codirectional) {
        brushWidget_->setEditOnlyCodirectedSurface(codirectional);
        std::cout << "Co-directional editing " << (codirectional ? "enabled" : "disabled") << std::endl;
    }

    /**
     * @brief Toggle visualization of mesh changes
     */
    void toggleVisualization(bool enable) {
        brushWidget_->enableDeviationVisualization(enable);
        std::cout << "Deviation visualization " << (enable ? "enabled" : "disabled") << std::endl;
    }

    /**
     * @brief Get current brush settings
     */
    const MR::SurfaceManipulationWidget::Settings& getSettings() const {
        return currentSettings_;
    }

    /**
     * @brief Get the brush widget (for viewer integration)
     */
    std::shared_ptr<MR::SurfaceManipulationWidget> getWidget() {
        return brushWidget_;
    }

    /**
     * @brief Apply a preset configuration
     */
    void applyPreset(const std::string& presetName) {
        if (presetName == "smooth") {
            // Gentle smoothing brush
            currentSettings_.workMode = MR::SurfaceManipulationWidget::WorkMode::Relax;
            currentSettings_.relaxForce = 0.15f;
            currentSettings_.sharpness = 30.0f;
            std::cout << "Applied 'smooth' preset" << std::endl;
        }
        else if (presetName == "carve") {
            // Precise carving tool
            currentSettings_.workMode = MR::SurfaceManipulationWidget::WorkMode::Remove;
            float diagonal = objectMesh_->getBoundingBox().diagonal();
            currentSettings_.radius = diagonal * 0.01f;
            currentSettings_.editForce = diagonal * 0.005f;
            currentSettings_.sharpness = 80.0f;
            currentSettings_.relaxForceAfterEdit = 0.1f;
            std::cout << "Applied 'carve' preset" << std::endl;
        }
        else if (presetName == "sculpt") {
            // Organic sculpting
            currentSettings_.workMode = MR::SurfaceManipulationWidget::WorkMode::Add;
            float diagonal = objectMesh_->getBoundingBox().diagonal();
            currentSettings_.radius = diagonal * 0.025f;
            currentSettings_.editForce = diagonal * 0.015f;
            currentSettings_.sharpness = 40.0f;
            currentSettings_.relaxForceAfterEdit = 0.25f;
            std::cout << "Applied 'sculpt' preset" << std::endl;
        }
        else {
            std::cerr << "Unknown preset: " << presetName << std::endl;
            return;
        }
        
        brushWidget_->setSettings(currentSettings_);
    }

    /**
     * @brief Reset to original mesh state
     */
    void reset() {
        if (brushWidget_) {
            brushWidget_->reset();
        }
        std::cout << "Brush editor reset" << std::endl;
    }

    /**
     * @brief Print current settings (for debugging)
     */
    void printSettings() const {
        const char* modeNames[] = {"Add", "Remove", "Relax", "Laplacian", "Patch"};
        std::cout << "\n=== Current Brush Settings ===" << std::endl;
        std::cout << "Mode: " << modeNames[static_cast<int>(currentSettings_.workMode)] << std::endl;
        std::cout << "Radius: " << currentSettings_.radius << std::endl;
        std::cout << "Strength: " << currentSettings_.editForce << std::endl;
        std::cout << "Sharpness: " << currentSettings_.sharpness << std::endl;
        std::cout << "Relax Force: " << currentSettings_.relaxForce << std::endl;
        std::cout << "Auto-Smooth: " << currentSettings_.relaxForceAfterEdit << std::endl;
        std::cout << "==============================\n" << std::endl;
    }

private:
    std::shared_ptr<MR::ObjectMesh> objectMesh_;
    std::shared_ptr<MR::SurfaceManipulationWidget> brushWidget_;
    MR::SurfaceManipulationWidget::Settings currentSettings_;
};

} // namespace BrushEditor

// ============================================================================
// Example usage
// ============================================================================

#ifdef EXAMPLE_MAIN

int main(int argc, char** argv) {
    using namespace BrushEditor;
    
    // Create a sample mesh (in real application, load from file)
    auto mesh = std::make_shared<MR::Mesh>();
    // ... load mesh data ...
    
    // Create brush editor
    BrushEditorApp editor;
    if (!editor.initialize(mesh)) {
        std::cerr << "Failed to initialize brush editor" << std::endl;
        return 1;
    }
    
    // Example 1: Setup basic brush
    editor.setMode(MR::SurfaceManipulationWidget::WorkMode::Add);
    editor.setRadius(5.0f);
    editor.setStrength(1.0f);
    editor.setSharpness(50.0f);
    editor.printSettings();
    
    // Example 2: Apply a preset
    editor.applyPreset("smooth");
    editor.printSettings();
    
    // Example 3: Advanced configuration
    editor.setMode(MR::SurfaceManipulationWidget::WorkMode::Laplacian);
    editor.setIgnoreOcclusion(false);  // Only edit visible surfaces
    editor.setEditCodirectionalOnly(true);  // Only surfaces facing camera
    editor.toggleVisualization(true);  // Show color-coded changes
    
    // Example 4: Lock a region (mesh boundaries)
    // MR::FaceBitSet boundaryFaces = /* identify boundary faces */;
    // editor.lockRegion(boundaryFaces);
    
    // In a real application, integrate with viewer:
    // viewer.addListener(editor.getWidget());
    
    std::cout << "Brush editor examples completed successfully" << std::endl;
    return 0;
}

#endif // EXAMPLE_MAIN

/**
 * @section INTEGRATION Integration Notes
 * 
 * To integrate this into your application:
 * 
 * 1. Create a BrushEditorApp instance
 * 2. Initialize it with your mesh
 * 3. Register the widget with your viewer:
 *    viewer.addListener(editor.getWidget());
 * 4. The widget automatically handles mouse events:
 *    - Left click + drag: Apply brush
 *    - Release: Finalize edit and create undo action
 * 5. Create UI controls to call setter methods (setRadius, setMode, etc.)
 * 6. Optionally implement undo/redo buttons using MeshLib's history system
 * 
 * @section PERFORMANCE Performance Tips
 * 
 * - Start with small radius values and increase as needed
 * - Use Relax mode for fastest performance
 * - Laplacian mode is slower but gives better quality
 * - For large meshes (>100K vertices), consider:
 *   - Limiting brush radius
 *   - Using LOD (Level of Detail)
 *   - Implementing spatial optimization
 * 
 * @section VALIDATION Input Validation
 * 
 * The example includes basic validation:
 * - Radius must be positive
 * - Sharpness clamped to [0, 100]
 * - Auto-smooth clamped to [0, 0.5]
 * - Null checks on initialization
 * 
 * Extend validation based on your application's requirements.
 */
