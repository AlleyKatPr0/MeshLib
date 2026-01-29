#include <MRMeshC/MRPointsLoad.h>
#include <MRMeshC/MRString.h>

#include <stdio.h>
#include <stdlib.h>

int main( void )
{
    // Analyze a text-based point cloud file before loading it
    MRString* error = NULL;
    MRTextFileAnalysisResult* analysisResult = mrPointsAnalyzeText( "pointcloud.csv", &error );
    
    if ( !analysisResult )
    {
        fprintf( stderr, "Failed to analyze file: %s\n", mrStringData( error ) );
        mrStringFree( error );
        return 1;
    }
    
    // Print analysis results
    printf( "File Analysis Results:\n" );
    printf( "  Total lines: %zu\n", analysisResult->totalLines );
    printf( "  Data lines: %zu\n", analysisResult->dataLines );
    printf( "  Comment lines: %zu\n", analysisResult->commentLines );
    printf( "  Empty lines: %zu\n", analysisResult->emptyLines );
    printf( "  Has normals: %s\n", analysisResult->hasNormals ? "yes" : "no" );
    printf( "  Has colors: %s\n", analysisResult->hasColors ? "yes" : "no" );
    
    if ( analysisResult->header )
        printf( "  Header: %s\n", analysisResult->header );
    
    if ( analysisResult->firstDataLine )
        printf( "  First data line: %s\n", analysisResult->firstDataLine );
    
    // Clean up
    mrTextFileAnalysisResultFree( analysisResult );
    
    // Now load the full point cloud
    MRPointCloud* pointCloud = mrPointsLoadFromAnySupportedFormat( "pointcloud.csv", NULL, &error );
    
    if ( !pointCloud )
    {
        fprintf( stderr, "Failed to load point cloud: %s\n", mrStringData( error ) );
        mrStringFree( error );
        return 1;
    }
    
    printf( "Loaded point cloud\n" );
    
    // Clean up
    mrPointCloudFree( pointCloud );
    
    return 0;
}
