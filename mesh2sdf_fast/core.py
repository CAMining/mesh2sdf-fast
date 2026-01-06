"""
core.py - High-level API wrapper

Provides simplified mesh-to-SDF conversion interface.
"""

import os
from typing import Optional, Callable, Union
from pathlib import Path

from . import _core
from .mesh_loader import load_mesh


def mesh_to_sdf(
    mesh_or_path: Union[str, Path, _core.Mesh],
    resolution: int = 64,
    padding: float = 0.1,
    normalize: bool = True,
    export_contours: bool = False,
    slice_x: bool = False,
    slice_y: bool = False,
    slice_z: bool = True,
    progress_callback: Optional[Callable[[int, int], None]] = None
) -> _core.SDFData:
    """
    Generate Signed Distance Field (SDF) from mesh
    
    This is the main high-level API, which converts a mesh file or Mesh object to SDF voxel data.
    
    Parameters
    ----------
    mesh_or_path : str, Path, or Mesh
        Mesh file path (STL/OBJ) or loaded Mesh object
    resolution : int, optional
        Voxel resolution (default 64, i.e., 64x64x64)
    padding : float, optional
        Boundary padding ratio (default 0.1, i.e., 10%)
    normalize : bool, optional
        Whether to normalize mesh to [-1, 1] range (default True)
    export_contours : bool, optional
        Whether to export contour lines for debugging (default False)
    slice_x : bool, optional
        Whether to slice along X-axis and merge distance fields (default False)
    slice_y : bool, optional
        Whether to slice along Y-axis and merge distance fields (default False)
    slice_z : bool, optional
        Whether to slice along Z-axis and merge distance fields (default True)
    progress_callback : callable, optional
        Progress callback function, Signature: callback(current, total)
    
    Returns
    -------
    SDFData
        Signed distance field data, contains voxel values and metadata
    
    Raises
    ------
    FileNotFoundError
        Mesh file not found
    MeshLoadError
        Mesh file format error or load failed
    ValueError
        Invalid parameter
    
    Examples
    --------
    >>> from mesh2sdf import mesh_to_sdf
    >>> sdf = mesh_to_sdf("model.stl", resolution=32)
    >>> print(f"SDF shape: {sdf.size_x}x{sdf.size_y}x{sdf.size_z}")
    
    >>> # Use progress callback
    >>> def on_progress(cur, total):
    ...     print(f"Processing: {cur}/{total}")
    >>> sdf = mesh_to_sdf("model.obj", progress_callback=on_progress)
    """
    # Parameter validation
    if resolution <= 0:
        raise ValueError(f"Resolution must be a positive integer, got: {resolution}")
    if padding < 0:
        raise ValueError(f"Padding ratio cannot be negative, got: {padding}")
    
    # Load mesh (if it's a path)
    if isinstance(mesh_or_path, (str, Path)):
        mesh = load_mesh(mesh_or_path)
    else:
        mesh = mesh_or_path
    
    # Check if it's a valid triangle mesh
    if mesh.triangle_count() == 0:
        raise ValueError("Input mesh contains no faces (triangles or polygons).")
    
    # Create configuration
    config = _core.SDFConfig()
    config.set_resolution(resolution)
    config.padding = padding
    config.normalize_mesh = normalize
    config.export_contours = export_contours
    config.slice_x = slice_x
    config.slice_y = slice_y
    config.slice_z = slice_z
    
    # Create generator and generate SDF
    generator = _core.SDFGenerator(config)
    sdf = generator.generate(mesh, progress_callback)
    
    return sdf


def generate_sample_data(
    output_path: Union[str, Path] = "sample_data.csv",
    shape: str = "sphere",
    resolution: int = 32
) -> _core.SDFData:
    """
    Generate sample SDF data for testing
    
    Parameters
    ----------
    output_path : str or Path, optional
        Output CSV file path (default "sample_data.csv")
    shape : str, optional
        Shape type, options: "sphere" or "cube" (default "sphere")
    resolution : int, optional
        Voxel resolution (default 32)
    
    Returns
    -------
    SDFData
        Generated SDF data
    
    Examples
    --------
    >>> from mesh2sdf import generate_sample_data
    >>> sdf = generate_sample_data("test_sphere.csv", shape="sphere", resolution=16)
    """
    # Create basic shape
    if shape.lower() == "sphere":
        mesh = _core.create_sphere(subdivisions=2)
    elif shape.lower() == "cube":
        mesh = _core.create_cube()
    else:
        raise ValueError(f"Unsupported shape: {shape}, Options: sphere, cube")
    
    # Generate SDF
    sdf = mesh_to_sdf(mesh, resolution=resolution)
    
    # Save to CSV
    output_path = Path(output_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    _core.save_sdf_csv(sdf, str(output_path))
    
    print(f"Sample data saved to: {output_path}")
    print(f"  Shape: {shape}")
    print(f"  Resolution: {sdf.size_x}x{sdf.size_y}x{sdf.size_z}")
    print(f"  Cell size: {sdf.cell_size:.6f}")
    
    return sdf
