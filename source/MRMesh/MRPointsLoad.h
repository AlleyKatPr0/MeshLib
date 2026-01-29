#pragma once

#include "MRMeshFwd.h"
#include "MRExpected.h"
#include "MRIOFilters.h"
#include "MRPointsLoadSettings.h"

#include <filesystem>
#include <string>

namespace MR::PointsLoad
{

/// \defgroup PointsLoadGroup Points Load
/// \addtogroup IOGroup
/// \{

/// structure that holds the analysis results of a text-based point cloud file
struct TextFileAnalysisResult
{
    std::string header;              ///< header line if detected (first unparseable non-comment line)
    std::string firstDataLine;       ///< first valid data line as a sample
    size_t totalLines = 0;           ///< total number of lines in the file
    size_t commentLines = 0;         ///< number of comment lines
    size_t emptyLines = 0;           ///< number of empty lines
    size_t dataLines = 0;            ///< number of lines with valid coordinate data
    bool hasNormals = false;         ///< whether the file contains normal data
    bool hasColors = false;          ///< whether the file contains color data
};

/// analyzes text-based point cloud file structure without loading the full data
/// useful for understanding CSV/ASC/XYZ/TXT file format before loading
MRMESH_API Expected<TextFileAnalysisResult> analyzeText( const std::filesystem::path& file );
MRMESH_API Expected<TextFileAnalysisResult> analyzeText( std::istream& in );

/// loads from .csv, .asc, .xyz, .txt file
MRMESH_API Expected<PointCloud> fromText( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromText( std::istream& in, const PointsLoadSettings& settings = {} );

/// loads from Laser scan plain data format (.pts) file
MRMESH_API Expected<PointCloud> fromPts( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromPts( std::istream& in, const PointsLoadSettings& settings = {} );

/// loads from .ply file
MRMESH_API Expected<PointCloud> fromPly( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromPly( std::istream& in, const PointsLoadSettings& settings = {} );

/// loads from .obj file
MRMESH_API Expected<PointCloud> fromObj( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromObj( std::istream& in, const PointsLoadSettings& settings = {} );

MRMESH_API Expected<PointCloud> fromDxf( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
MRMESH_API Expected<PointCloud> fromDxf( std::istream& in, const PointsLoadSettings& settings = {} );

/// detects the format from file extension and loads points from it
MRMESH_API Expected<PointCloud> fromAnySupportedFormat( const std::filesystem::path& file, const PointsLoadSettings& settings = {} );
/// extension in `*.ext` format
MRMESH_API Expected<PointCloud> fromAnySupportedFormat( std::istream& in, const std::string& extension, const PointsLoadSettings& settings = {} );

/// \}

} // namespace MR::PointsLoad
