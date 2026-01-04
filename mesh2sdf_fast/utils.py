"""
utils.py - 工具函数

提供SDF数据转换、可视化等辅助功能。
"""

from typing import Optional, Tuple
import numpy as np

from . import _core


def sdf_to_numpy(sdf: _core.SDFData) -> np.ndarray:
    """
    将SDFData转换为NumPy数组
    
    Parameters
    ----------
    sdf : SDFData
        SDF数据对象
    
    Returns
    -------
    np.ndarray
        Shape为 (z, y, x) 的3D数组，数据类型为 float32
    
    Examples
    --------
    >>> from mesh2sdf import mesh_to_sdf, sdf_to_numpy
    >>> sdf = mesh_to_sdf("model.stl")
    >>> arr = sdf_to_numpy(sdf)
    >>> print(arr.shape)  # (64, 64, 64)
    """
    return sdf.to_numpy()


def numpy_to_sdf(
    arr: np.ndarray,
    origin: Tuple[float, float, float] = (0.0, 0.0, 0.0),
    cell_size: float = 1.0
) -> _core.SDFData:
    """
    从NumPy数组创建SDFData
    
    Parameters
    ----------
    arr : np.ndarray
        Shape为 (z, y, x) 的3D数组
    origin : tuple, optional
        原点坐标 (x, y, z)，默认 (0, 0, 0)
    cell_size : float, optional
        Cell size，默认 1.0
    
    Returns
    -------
    SDFData
        创建的SDF数据对象
    """
    if arr.ndim != 3:
        raise ValueError(f"期望3D数组，got {arr.ndim}D")
    
    sdf = _core.SDFData()
    sdf.size_z, sdf.size_y, sdf.size_x = arr.shape
    sdf.origin_x, sdf.origin_y, sdf.origin_z = origin
    # Set anisotropic cell size (assuming isotropic input if single float)
    sdf.cell_size_x = cell_size
    sdf.cell_size_y = cell_size
    sdf.cell_size_z = cell_size
    
    # 复制数据
    sdf.values = []
    # Use flat list construction for efficiency if possible, or simple loop
    # Accessing numpy array element by element is slow, but bindings might expect list.
    # Alternatively, if bindings support buffer protocol assignment, that would be better.
    # But for now, faithfully keeping the loop structure but removing int cast.
    # Flatten array to list might be faster: arr.flatten().tolist()
    # But let's stick to the existing nested loop structure to minimize risk, just changing the cast.
    for z in range(sdf.size_z):
        for y in range(sdf.size_y):
            for x in range(sdf.size_x):
                sdf.values.append(float(arr[z, y, x]))
    
    return sdf


def visualize_sdf_slice(
    sdf: _core.SDFData,
    axis: str = "z",
    index: Optional[int] = None,
    ax=None,
    cmap: str = "RdBu",
    show_contour: bool = True
):
    """
    可视化SDF的一个切片
    
    Parameters
    ----------
    sdf : SDFData
        SDF数据对象
    axis : str, optional
        切片轴，Options: 'x', 'y', 'z'（默认 'z'）
    index : int, optional
        切片索引，默认为中间切片
    ax : matplotlib.axes.Axes, optional
        matplotlib轴对象，如果为None则创建新图
    cmap : str, optional
        颜色映射（默认 'RdBu'）
    show_contour : bool, optional
        是否显示零等值线（默认 True）
    
    Returns
    -------
    matplotlib.axes.Axes
        绑定的轴对象
    
    Examples
    --------
    >>> import matplotlib.pyplot as plt
    >>> from mesh2sdf import mesh_to_sdf, visualize_sdf_slice
    >>> sdf = mesh_to_sdf("model.stl", resolution=32)
    >>> visualize_sdf_slice(sdf, axis='z', index=16)
    >>> plt.show()
    """
    import matplotlib.pyplot as plt
    
    arr = sdf_to_numpy(sdf)
    
    # 确定切片
    axis = axis.lower()
    if axis == "z":
        max_idx = arr.shape[0]
        if index is None:
            index = max_idx // 2
        slice_data = arr[index, :, :]
        xlabel, ylabel = "X", "Y"
        title = f"Z = {index}"
    elif axis == "y":
        max_idx = arr.shape[1]
        if index is None:
            index = max_idx // 2
        slice_data = arr[:, index, :]
        xlabel, ylabel = "X", "Z"
        title = f"Y = {index}"
    elif axis == "x":
        max_idx = arr.shape[2]
        if index is None:
            index = max_idx // 2
        slice_data = arr[:, :, index]
        xlabel, ylabel = "Y", "Z"
        title = f"X = {index}"
    else:
        raise ValueError(f"无效的轴: {axis}，Options:: x, y, z")
    
    if index < 0 or index >= max_idx:
        raise ValueError(f"索引 {index} 超出range [0, {max_idx})")
    
    # 创建图形
    if ax is None:
        fig, ax = plt.subplots(figsize=(8, 6))
    
    # 显示距离场
    vmax = max(abs(slice_data.min()), abs(slice_data.max()))
    im = ax.imshow(
        slice_data,
        cmap=cmap,
        vmin=-vmax,
        vmax=vmax,
        origin="lower",
        aspect="equal"
    )
    
    # 添加颜色条
    plt.colorbar(im, ax=ax, label="SDF值 (mesh距离)")
    
    # 显示零等值线（表面位置）
    if show_contour:
        ax.contour(slice_data, levels=[0], colors="black", linewidths=2)
    
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(f"SDF切片 ({title})")
    
    return ax


def get_surface_points(sdf: _core.SDFData, threshold: float = 0.5) -> np.ndarray:
    """
    提取接近表面的点（SDF值接近0的点）
    
    Parameters
    ----------
    sdf : SDFData
        SDF数据对象
    threshold : float, optional
        阈值（默认0.5个mesh单位）
    
    Returns
    -------
    np.ndarray
        Shape为 (N, 3) 的点坐标数组（世界坐标）
    """
    arr = sdf_to_numpy(sdf)
    
    # 找到接近0的点
    mask = np.abs(arr) <= threshold
    indices = np.argwhere(mask)  # (z, y, x)
    
    # 转换为世界坐标
    points = np.zeros((len(indices), 3))
    for i, (z, y, x) in enumerate(indices):
        world_pos = sdf.grid_to_world(x, y, z)
        points[i] = [world_pos.x, world_pos.y, world_pos.z]
    
    return points
