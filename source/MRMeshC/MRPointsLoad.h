#pragma once

#include "MRMeshFwd.h"
#include "MRPointsLoadSettings.h"

MR_EXTERN_C_BEGIN

/// structure that holds the analysis results of a text-based point cloud file
typedef struct MRTextFileAnalysisResult
{
    const char* header;              ///< header line if detected (first unparseable non-comment line)
    const char* firstDataLine;       ///< first valid data line as a sample
    size_t totalLines;               ///< total number of lines in the file
    size_t commentLines;             ///< number of comment lines
    size_t emptyLines;               ///< number of empty lines
    size_t dataLines;                ///< number of lines with valid coordinate data
    bool hasNormals;                 ///< whether the file contains normal data
    bool hasColors;                  ///< whether the file contains color data
} MRTextFileAnalysisResult;

/// analyzes text-based point cloud file structure without loading the full data
/// useful for understanding CSV/ASC/XYZ/TXT file format before loading
/// caller is responsible for freeing the result with mrTextFileAnalysisResultFree
MRMESHC_API MRTextFileAnalysisResult* mrPointsAnalyzeText( const char* filename, MRString** errorString );

/// frees the MRTextFileAnalysisResult structure
MRMESHC_API void mrTextFileAnalysisResultFree( MRTextFileAnalysisResult* result );

/// detects the format from file extension and loads points from it
MRMESHC_API MRPointCloud* mrPointsLoadFromAnySupportedFormat( const char* filename, const MRPointsLoadSettings* settings, MRString** errorString );

MR_EXTERN_C_END
