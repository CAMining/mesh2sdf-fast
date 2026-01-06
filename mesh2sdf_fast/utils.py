"""
utils.py - Utility functions

Provides auxiliary functions for SDF data conversion and visualization.
"""

from typing import Optional, Tuple, Union, List
import numpy as np

from . import _core


def sdf_to_numpy(sdf: _core.SDFData) -> np.ndarray:
    """
    Convert SDFData to NumPy array
    
    Parameters
    ----------
    sdf : SDFData
        SDF data object
    
    Returns
    -------
    np.ndarray
        3D array with shape (z, y, x), data type is float32
    
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
    Create SDFData from NumPy array
    
    Parameters
    ----------
    arr : np.ndarray
        3D array with shape (z, y, x)
    origin : tuple, optional
        Origin coordinates (x, y, z), default (0, 0, 0)
    cell_size : float, optional
        Cell size, default 1.0
    
    Returns
    -------
    SDFData
        Created SDF data object
    """
    if arr.ndim != 3:
        raise ValueError(f"Expected 3D array, got {arr.ndim}D")
    
    sdf = _core.SDFData()
    sdf.size_z, sdf.size_y, sdf.size_x = arr.shape
    sdf.origin_x, sdf.origin_y, sdf.origin_z = origin
    # Set anisotropic cell size (assuming isotropic input if single float)
    sdf.cell_size_x = cell_size
    sdf.cell_size_y = cell_size
    sdf.cell_size_z = cell_size
    
    # Copy data
    sdf.values = []
    # Use flat list construction for efficiency if possible, or simple loop
    for z in range(sdf.size_z):
        for y in range(sdf.size_y):
            for x in range(sdf.size_x):
                sdf.values.append(float(arr[z, y, x]))
    
    return sdf


def visualize_sdf_slice(
    sdf: _core.SDFData,
    axis: str = "z",
    index: Optional[Union[int, List[int]]] = None,
    ax=None,
    cmap: str = "RdBu",
    show_contour: bool = True
):
    """
    Visualize one or more slices of the SDF
    
    Parameters
    ----------
    sdf : SDFData
        SDF data object
    axis : str, optional
        Slicing axis, Options: 'x', 'y', 'z' (default 'z')
    index : int or List[int], optional
        Slicing indices. If it's a list, multiple subplots are drawn. Default is the middle slice.
    ax : matplotlib.axes.Axes or np.ndarray, optional
        Matplotlib axes object. If index is a list, this should be an array of axes or None.
    cmap : str, optional
        Color map (default 'RdBu')
    show_contour : bool, optional
        Whether to show the zero isosurface (default True)
    
    Returns
    -------
    matplotlib.axes.Axes or np.ndarray
        Bound axes object or axes array
    """
    import matplotlib.pyplot as plt
    
    arr = sdf_to_numpy(sdf)
    
    # Determine dimension info
    axis = axis.lower()
    if axis == "z":
        max_idx = arr.shape[0]
        xlabel, ylabel = "X", "Y"
        get_slice = lambda i: arr[i, :, :]
    elif axis == "y":
        max_idx = arr.shape[1]
        xlabel, ylabel = "X", "Z"
        get_slice = lambda i: arr[:, i, :]
    elif axis == "x":
        max_idx = arr.shape[2]
        xlabel, ylabel = "Y", "Z"
        get_slice = lambda i: arr[:, :, i]
    else:
        raise ValueError(f"Invalid axis: {axis}, Options: x, y, z")

    # Handle indices
    if index is None:
        indices = [max_idx // 2]
    elif isinstance(index, int):
        indices = [index]
    else:
        indices = index
        
    # Validate indices
    for idx in indices:
        if idx < 0 or idx >= max_idx:
            raise ValueError(f"Index {idx} out of range [0, {max_idx})")

    # Create figure (if needed)
    if ax is None:
        n_slices = len(indices)
        if n_slices == 1:
            fig, ax = plt.subplots(figsize=(7, 6))
            axes_list = [ax]
        else:
            # Max slices per page (e.g., 3x4=12)
            max_per_page = 12
            cols = 4
            rows_per_page = 3
            
            if n_slices > max_per_page:
                # Enable paging mode
                fig = plt.figure(figsize=(15, 12))
                from matplotlib.widgets import Button
                
                # State variables
                state = {'page': 0}
                n_pages = (n_slices + max_per_page - 1) // max_per_page
                
                # Create subplot placeholders
                axes_list = []
                for r in range(rows_per_page):
                    for c in range(cols):
                        # Get subplot position in figure [left, bottom, width, height]
                        # Leave space at the bottom for buttons
                        rect = [0.05 + c*0.23, 0.7 - r*0.28, 0.18, 0.22]
                        axes_list.append(fig.add_axes(rect))
                
                def update_display():
                    start_idx = state['page'] * max_per_page
                    end_idx = min(start_idx + max_per_page, n_slices)
                    
                    fig.suptitle(f"SDF Slices - Page {state['page'] + 1}/{n_pages} (Total: {n_slices})", fontsize=16)
                    
                    for i in range(max_per_page):
                        current_ax = axes_list[i]
                        current_ax.clear()
                        
                        slice_idx_in_indices = start_idx + i
                        if slice_idx_in_indices < n_slices:
                            idx = indices[slice_idx_in_indices]
                            slice_data = get_slice(idx)
                            vmax = max(abs(slice_data.min()), abs(slice_data.max()))
                            
                            im = current_ax.imshow(slice_data, cmap=cmap, vmin=-vmax, vmax=vmax, origin="lower", aspect="equal")
                            if show_contour:
                                current_ax.contour(slice_data, levels=[0], colors="black", linewidths=1.5)
                            
                            current_ax.set_title(f"{axis.upper()} = {idx}", fontsize=10)
                            current_ax.set_visible(True)
                            # Note: individually adding colorbars here, could be optimized to share.
                        else:
                            current_ax.set_visible(False)
                    
                    fig.canvas.draw_idle()

                # Add buttons
                ax_prev = fig.add_axes([0.4, 0.02, 0.08, 0.04])
                ax_next = fig.add_axes([0.52, 0.02, 0.08, 0.04])
                btn_prev = Button(ax_prev, 'Previous')
                btn_next = Button(ax_next, 'Next')

                def go_prev(event):
                    if state['page'] > 0:
                        state['page'] -= 1
                        update_display()

                def go_next(event):
                    if state['page'] < n_pages - 1:
                        state['page'] += 1
                        update_display()

                btn_prev.on_clicked(go_prev)
                btn_next.on_clicked(go_next)
                
                # Initial display
                update_display()
                
                # Keep references
                fig._btns = [btn_prev, btn_next]
                return None # Return None or a specific control object in this mode
            else:
                # Normal display mode (no paging)
                cols = min(n_slices, 4)
                rows = (n_slices + cols - 1) // cols
                fig, axes_raw = plt.subplots(rows, cols, figsize=(3.5 * cols + 1, 3.5 * rows))
                axes_list = axes_raw.flatten() if n_slices > 1 else [axes_raw]
                for i in range(n_slices, len(axes_list)):
                    axes_list[i].axis('off')
    else:
        if hasattr(ax, 'flatten'):
             axes_list = ax.flatten()
        else:
             axes_list = [ax]
             
    # Draw content (non-paging mode only)
    for i, idx in enumerate(indices):
        if i >= len(axes_list): break
        current_ax = axes_list[i]
        slice_data = get_slice(idx)
        vmax = max(abs(slice_data.min()), abs(slice_data.max()))
        im = current_ax.imshow(slice_data, cmap=cmap, vmin=-vmax, vmax=vmax, origin="lower", aspect="equal")
        if show_contour:
            current_ax.contour(slice_data, levels=[0], colors="black", linewidths=2)
        current_ax.set_xlabel(xlabel)
        current_ax.set_ylabel(ylabel)
        current_ax.set_title(f"{axis.upper()} = {idx}", fontsize=9)
        plt.colorbar(im, ax=current_ax, fraction=0.046, pad=0.04)

    if ax is None:
        if not hasattr(fig, '_btns'):
            plt.tight_layout()
        if len(indices) == 1:
            return axes_list[0]
    
    return ax


def get_surface_points(sdf: _core.SDFData, threshold: float = 0.5) -> np.ndarray:
    """
    Extract points near the surface (points with SDF values close to 0)
    
    Parameters
    ----------
    sdf : SDFData
        SDF data object
    threshold : float, optional
        Threshold (default 0.5 mesh units)
    
    Returns
    -------
    np.ndarray
        N x 3 point coordinate array (world coordinates)
    """
    arr = sdf_to_numpy(sdf)
    
    # Find points near zero
    mask = np.abs(arr) <= threshold
    indices = np.argwhere(mask)  # (z, y, x)
    
    # Convert to world coordinates
    points = np.zeros((len(indices), 3))
    for i, (z, y, x) in enumerate(indices):
        world_pos = sdf.grid_to_world(x, y, z)
        points[i] = [world_pos.x, world_pos.y, world_pos.z]
    
    return points
