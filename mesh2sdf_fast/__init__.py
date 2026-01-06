"""
Mesh2SDF-Fast: Fast Signed Distance Field generation from mesh models

This library implements an efficient mesh-to-SDF conversion algorithm:
1. Slice mesh with voxel layer planes to generate contour lines
2. Apply Manhattan distance transform variant for 2D distance fields
3. Propagate distances in Z-direction to obtain complete 3D SDF
"""

__version__ = "0.1.0"

# Import C++ core module
from . import _core

# Export core classes
from ._core import (
    Vec3,
    Vec2,
    Triangle,
    BoundingBox,
    Mesh,
    SDFConfig,
    SDFData,
    SDFGenerator,
    MeshLoadError,
)

# Export functions
from ._core import (
    load_stl,
    load_obj,
    create_sphere,
    create_cube,
    save_sdf_csv,
    load_sdf_csv,
)

# Export high-level APIs
from .core import mesh_to_sdf, generate_sample_data
from .mesh_loader import load_mesh, get_mesh_info
from .utils import sdf_to_numpy, numpy_to_sdf, visualize_sdf_slice

__all__ = [
    # Version
    "__version__",
    # Classes
    "Vec3",
    "Vec2", 
    "Triangle",
    "BoundingBox",
    "Mesh",
    "SDFConfig",
    "SDFData",
    "SDFGenerator",
    "MeshLoadError",
    # Core functions
    "load_stl",
    "load_obj",
    "create_sphere",
    "create_cube",
    "save_sdf_csv",
    "load_sdf_csv",
    # High-level APIs
    "mesh_to_sdf",
    "generate_sample_data",
    "load_mesh",
    "get_mesh_info",
    "sdf_to_numpy",
    "numpy_to_sdf",
    "visualize_sdf_slice",
]
