"""
Mesh2SDF-Fast: Fast Signed Distance Field generation from mesh models

This library implements an efficient mesh-to-SDF conversion algorithm:
1. Slice mesh with voxel layer planes to generate contour lines
2. Apply Manhattan distance transform variant for 2D distance fields
3. Propagate distances in Z-direction to obtain complete 3D SDF
"""

__version__ = "0.1.0"

# importC++核心模块
from . import _core

# export核心类
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

# export函数
from ._core import (
    load_stl,
    load_obj,
    create_sphere,
    create_cube,
    save_sdf_csv,
    load_sdf_csv,
)

# export高层API
from .core import mesh_to_sdf, generate_sample_data
from .mesh_loader import load_mesh, get_mesh_info
from .utils import sdf_to_numpy, numpy_to_sdf, visualize_sdf_slice

__all__ = [
    # 版本
    "__version__",
    # 类
    "Vec3",
    "Vec2", 
    "Triangle",
    "BoundingBox",
    "Mesh",
    "SDFConfig",
    "SDFData",
    "SDFGenerator",
    "MeshLoadError",
    # 核心函数
    "load_stl",
    "load_obj",
    "create_sphere",
    "create_cube",
    "save_sdf_csv",
    "load_sdf_csv",
    # 高层API
    "mesh_to_sdf",
    "generate_sample_data",
    "load_mesh",
    "get_mesh_info",
    "sdf_to_numpy",
    "numpy_to_sdf",
    "visualize_sdf_slice",
]
