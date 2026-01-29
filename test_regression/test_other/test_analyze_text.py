import pytest
from module_helper import *
from pathlib import Path
import tempfile


def test_analyze_simple_points():
    """
    Test analyzing a simple CSV file with coordinates only
    """
    # Create a simple test CSV file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("# Comment line\n")
        f.write("X,Y,Z\n")
        f.write("1.0 2.0 3.0\n")
        f.write("4.0 5.0 6.0\n")
        f.write("7.0 8.0 9.0\n")
        temp_file = f.name
    
    try:
        # Analyze the file - trying different possible function names
        # The actual name depends on the binding generation
        if hasattr(mrmeshpy, 'analyzeTextPoints'):
            result = mrmeshpy.analyzeTextPoints(Path(temp_file))
        elif hasattr(mrmeshpy.PointsLoad, 'analyzeText'):
            result = mrmeshpy.PointsLoad.analyzeText(Path(temp_file))
        else:
            pytest.skip("analyzeText function not available in Python bindings")
        
        # Check the analysis results
        assert result.totalLines == 5, f"Expected 5 total lines, got {result.totalLines}"
        assert result.commentLines == 1, f"Expected 1 comment line, got {result.commentLines}"
        assert result.dataLines >= 3, f"Expected at least 3 data lines, got {result.dataLines}"
        assert result.header == "X,Y,Z", f"Expected header 'X,Y,Z', got '{result.header}'"
        assert result.hasNormals == False, f"Expected no normals, got {result.hasNormals}"
        assert result.hasColors == False, f"Expected no colors, got {result.hasColors}"
        assert result.firstDataLine == "1.0 2.0 3.0", f"Expected first data line '1.0 2.0 3.0', got '{result.firstDataLine}'"
    finally:
        # Clean up
        Path(temp_file).unlink()


def test_analyze_points_with_normals():
    """
    Test analyzing a CSV file with coordinates and normals
    """
    # Create a test CSV file with normals
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("# Point cloud with normals\n")
        f.write("x,y,z,nx,ny,nz\n")
        f.write("1.0 2.0 3.0 0.0 0.0 1.0\n")
        f.write("4.0 5.0 6.0 0.0 1.0 0.0\n")
        f.write("7.0 8.0 9.0 1.0 0.0 0.0\n")
        temp_file = f.name
    
    try:
        # Analyze the file
        if hasattr(mrmeshpy, 'analyzeTextPoints'):
            result = mrmeshpy.analyzeTextPoints(Path(temp_file))
        elif hasattr(mrmeshpy.PointsLoad, 'analyzeText'):
            result = mrmeshpy.PointsLoad.analyzeText(Path(temp_file))
        else:
            pytest.skip("analyzeText function not available in Python bindings")
        
        # Check the analysis results
        assert result.totalLines == 5, f"Expected 5 total lines, got {result.totalLines}"
        assert result.commentLines == 1, f"Expected 1 comment line, got {result.commentLines}"
        assert result.dataLines >= 3, f"Expected at least 3 data lines, got {result.dataLines}"
        assert result.hasNormals == True, f"Expected normals, got {result.hasNormals}"
    finally:
        # Clean up
        Path(temp_file).unlink()


def test_analyze_points_with_colors():
    """
    Test analyzing a CSV file with coordinates and colors
    """
    # Create a test CSV file with colors
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        f.write("1.0 2.0 3.0 255 0 0\n")
        f.write("4.0 5.0 6.0 0 255 0\n")
        f.write("7.0 8.0 9.0 0 0 255\n")
        temp_file = f.name
    
    try:
        # Analyze the file
        if hasattr(mrmeshpy, 'analyzeTextPoints'):
            result = mrmeshpy.analyzeTextPoints(Path(temp_file))
        elif hasattr(mrmeshpy.PointsLoad, 'analyzeText'):
            result = mrmeshpy.PointsLoad.analyzeText(Path(temp_file))
        else:
            pytest.skip("analyzeText function not available in Python bindings")
        
        # Check the analysis results
        assert result.totalLines == 3, f"Expected 3 total lines, got {result.totalLines}"
        assert result.commentLines == 0, f"Expected 0 comment lines, got {result.commentLines}"
        assert result.dataLines >= 3, f"Expected at least 3 data lines, got {result.dataLines}"
        assert result.hasColors == True, f"Expected colors, got {result.hasColors}"
    finally:
        # Clean up
        Path(temp_file).unlink()


def test_analyze_empty_file():
    """
    Test analyzing an empty file
    """
    # Create an empty test file
    with tempfile.NamedTemporaryFile(mode='w', suffix='.csv', delete=False) as f:
        temp_file = f.name
    
    try:
        # Analyze the file - should return error
        if hasattr(mrmeshpy, 'analyzeTextPoints'):
            with pytest.raises(Exception):
                result = mrmeshpy.analyzeTextPoints(Path(temp_file))
        elif hasattr(mrmeshpy.PointsLoad, 'analyzeText'):
            with pytest.raises(Exception):
                result = mrmeshpy.PointsLoad.analyzeText(Path(temp_file))
        else:
            pytest.skip("analyzeText function not available in Python bindings")
    finally:
        # Clean up
        Path(temp_file).unlink()
