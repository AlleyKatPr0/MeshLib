import meshlib.mrmeshpy as mrmeshpy

# Analyze a text-based point cloud file before loading it
analysis_result = mrmeshpy.PointsLoad.analyzeText("pointcloud.csv")

# Print analysis results
print("File Analysis Results:")
print(f"  Total lines: {analysis_result.totalLines}")
print(f"  Data lines: {analysis_result.dataLines}")
print(f"  Comment lines: {analysis_result.commentLines}")
print(f"  Empty lines: {analysis_result.emptyLines}")
print(f"  Has normals: {'yes' if analysis_result.hasNormals else 'no'}")
print(f"  Has colors: {'yes' if analysis_result.hasColors else 'no'}")

if analysis_result.header:
    print(f"  Header: {analysis_result.header}")

if analysis_result.firstDataLine:
    print(f"  First data line: {analysis_result.firstDataLine}")

# Now load the full point cloud
point_cloud = mrmeshpy.loadPoints("pointcloud.csv")

print(f"Loaded {point_cloud.validPoints.count()} points")
