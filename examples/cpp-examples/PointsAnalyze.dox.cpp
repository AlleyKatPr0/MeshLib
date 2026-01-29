#include <MRMesh/MRPointsLoad.h>
#include <MRMesh/MRPointCloud.h>

#include <iostream>

int main()
{
    // Analyze a text-based point cloud file before loading it
    auto analysisResult = MR::PointsLoad::analyzeText( "pointcloud.csv" );
    
    if ( !analysisResult )
    {
        std::cerr << "Failed to analyze file: " << analysisResult.error() << std::endl;
        return 1;
    }
    
    // Print analysis results
    std::cout << "File Analysis Results:" << std::endl;
    std::cout << "  Total lines: " << analysisResult->totalLines << std::endl;
    std::cout << "  Data lines: " << analysisResult->dataLines << std::endl;
    std::cout << "  Comment lines: " << analysisResult->commentLines << std::endl;
    std::cout << "  Empty lines: " << analysisResult->emptyLines << std::endl;
    std::cout << "  Has normals: " << ( analysisResult->hasNormals ? "yes" : "no" ) << std::endl;
    std::cout << "  Has colors: " << ( analysisResult->hasColors ? "yes" : "no" ) << std::endl;
    
    if ( !analysisResult->header.empty() )
        std::cout << "  Header: " << analysisResult->header << std::endl;
    
    if ( !analysisResult->firstDataLine.empty() )
        std::cout << "  First data line: " << analysisResult->firstDataLine << std::endl;
    
    // Now load the full point cloud
    auto pointCloud = MR::PointsLoad::fromText( "pointcloud.csv" );
    
    if ( !pointCloud )
    {
        std::cerr << "Failed to load point cloud: " << pointCloud.error() << std::endl;
        return 1;
    }
    
    std::cout << "Loaded " << pointCloud->validPoints.count() << " points" << std::endl;
    
    return 0;
}
