"""
mesh_loader.py - Mesh file loader

Supports loading multiple mesh file formats.
"""

import os
from pathlib import Path
from typing import Union

from . import _core


class MeshFileNotFoundError(FileNotFoundError):
    """Mesh file not found exception"""
    pass


class MeshFormatError(ValueError):
    """Mesh file format error exception"""
    pass


def load_mesh(filepath: Union[str, Path]) -> _core.Mesh:
    """
    Automatically load mesh file based on file extension
    
    Supported formats:
    - STL (Binary and ASCII)
    - OBJ (Wavefront)
    
    Parameters
    ----------
    filepath : str or Path
        Mesh file path
    
    Returns
    -------
    Mesh
        Loaded mesh object
    
    Raises
    ------
    MeshFileNotFoundError
        File does not exist
    MeshFormatError
        Unsupported file format
    MeshLoadError
        Failed to parse file content
    
    Examples
    --------
    >>> from mesh2sdf import load_mesh
    >>> mesh = load_mesh("model.stl")
    >>> print(f"Vertex count: {mesh.vertex_count()}")
    >>> print(f"Triangle count: {mesh.triangle_count()}")
    """
    filepath = Path(filepath)
    
    # Check if file exists
    if not filepath.exists():
        raise MeshFileNotFoundError(f"Mesh file not found: {filepath}")
    
    # Get file extension
    ext = filepath.suffix.lower()
    
    try:
        if ext == ".stl":
            mesh = _core.load_stl(str(filepath))
        elif ext == ".obj":
            mesh = _core.load_obj(str(filepath))
        else:
            raise MeshFormatError(
                f"Unsupported mesh format: {ext}\n"
                f"Supported formats: .stl, .obj"
            )
    except _core.MeshLoadError as e:
        # Re-throw with more user-friendly error message
        raise _core.MeshLoadError(f"Failed to load mesh file: {filepath}\nReason: {str(e)}")
    
    # Validate mesh
    if not mesh.is_valid():
        raise MeshFormatError(f"Mesh file content is invalid or empty: {filepath}")
    
    return mesh


def get_mesh_info(mesh: _core.Mesh) -> dict:
    """
    Get detailed information about the mesh
    
    Parameters
    ----------
    mesh : Mesh
        Mesh object
    
    Returns
    -------
    dict
        Dictionary containing vertex count, triangle count, bounding box, etc.
    """
    bbox = mesh.compute_bounding_box()
    
    return {
        "vertex_count": mesh.vertex_count(),
        "triangle_count": mesh.triangle_count(),
        "bounding_box": {
            "min": (bbox.min_corner.x, bbox.min_corner.y, bbox.min_corner.z),
            "max": (bbox.max_corner.x, bbox.max_corner.y, bbox.max_corner.z),
            "center": (bbox.center().x, bbox.center().y, bbox.center().z),
            "size": (bbox.size().x, bbox.size().y, bbox.size().z),
            "max_extent": bbox.max_extent(),
        },
        "is_valid": mesh.is_valid(),
    }
