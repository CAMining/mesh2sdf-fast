"""
mesh_loader.py - mesh文件load器

支持多种mesh文件格式的load。
"""

import os
from pathlib import Path
from typing import Union

from . import _core


class MeshFileNotFoundError(FileNotFoundError):
    """Mesh file not found异常"""
    pass


class MeshFormatError(ValueError):
    """mesh文件格式错误异常"""
    pass


def load_mesh(filepath: Union[str, Path]) -> _core.Mesh:
    """
    根据文件扩展名自动Load mesh文件
    
    支持的格式：
    - STL (二进制和ASCII)
    - OBJ (Wavefront)
    
    Parameters
    ----------
    filepath : str or Path
        Mesh file path
    
    Returns
    -------
    Mesh
        load的mesh对象
    
    Raises
    ------
    MeshFileNotFoundError
        文件不存在
    MeshFormatError
        不支持的文件格式
    MeshLoadError
        文件内容解析失败
    
    Examples
    --------
    >>> from mesh2sdf import load_mesh
    >>> mesh = load_mesh("model.stl")
    >>> print(f"vertex数: {mesh.vertex_count()}")
    >>> print(f"triangle数: {mesh.triangle_count()}")
    """
    filepath = Path(filepath)
    
    # 检查文件是否存在
    if not filepath.exists():
        raise MeshFileNotFoundError(f"Mesh file not found: {filepath}")
    
    # 获取文件扩展名
    ext = filepath.suffix.lower()
    
    try:
        if ext == ".stl":
            mesh = _core.load_stl(str(filepath))
        elif ext == ".obj":
            mesh = _core.load_obj(str(filepath))
        else:
            raise MeshFormatError(
                f"不支持的mesh格式: {ext}\n"
                f"支持的格式: .stl, .obj"
            )
    except _core.MeshLoadError as e:
        # 重新抛出更友好的错误信息
        raise _core.MeshLoadError(f"Load mesh文件失败: {filepath}\n原因: {str(e)}")
    
    # 验证mesh
    if not mesh.is_valid():
        raise MeshFormatError(f"mesh文件内容无效或为空: {filepath}")
    
    return mesh


def get_mesh_info(mesh: _core.Mesh) -> dict:
    """
    获取mesh的详细信息
    
    Parameters
    ----------
    mesh : Mesh
        mesh对象
    
    Returns
    -------
    dict
        包含vertex数、triangle数、包围盒等信息的字典
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
