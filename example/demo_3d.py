import numpy as np
import mesh2sdf_fast as mesh2sdf
import os
import pyvista as pv
import time

def main():
    print("--- Mesh2SDF 3D Visualization Demo ---")
    print(f"Mesh2SDF version: {mesh2sdf.__version__}")
    
    # 1. Load Mesh
    mesh_path = os.path.join(os.path.dirname(__file__), 'data', 'plane.obj')
    print(f"Loading mesh from: {mesh_path}")
    
    try:
        mesh = mesh2sdf.load_mesh(mesh_path)
        info = mesh2sdf.get_mesh_info(mesh)
        print(f"Mesh loaded: {info['vertex_count']} vertices, {info['triangle_count']} triangles")
    except Exception as e:
        print(f"Error loading mesh: {e}")
        return

    # 2. Generate SDF
    resolution = 256
    sx, sy, sz = True, False, False
    active_axes = [a for a, v in zip(['X', 'Y', 'Z'], [sx, sy, sz]) if v]
    axes_str = f"({', '.join(active_axes)}-Axis slicing)" if active_axes else ""
    print(f"Generating SDF with resolution {resolution} {axes_str}...")
    
    start_time = time.time()
    sdf_data = mesh2sdf.mesh_to_sdf(
        mesh, 
        resolution=resolution, 
        padding=0.2, 
        normalize=True,
        slice_x=sx,
        slice_y=sy,
        slice_z=sz,
        export_contours=False,
    )
    print(f"SDF generation took {time.time() - start_time:.4f} seconds")

    # 3. Marching Cubes with PyVista
    print("Running Marching Cubes (PyVista Contour)...")
    try:      
        grid = pv.ImageData()
        grid.dimensions = (sdf_data.size_x, sdf_data.size_y, sdf_data.size_z)
        grid.origin = (sdf_data.origin_x, sdf_data.origin_y, sdf_data.origin_z)
        grid.spacing = (sdf_data.cell_size_x, sdf_data.cell_size_y, sdf_data.cell_size_z)
        
        sdf_np = mesh2sdf.sdf_to_numpy(sdf_data)
        grid.point_data["values"] = sdf_np.flatten()
        
        # Extract Isosurface at level 0
        recon_mesh = grid.contour(isosurfaces=[0.0], scalars="values")
        tri_mesh = recon_mesh.triangulate()
        print(f"Extracted {tri_mesh.n_points} vertices and {tri_mesh.n_cells} faces")

        # 4. Visualization
        print("Opening PyVista window...")
        pl = pv.Plotter()
        
        # Original Mesh (Wireframe)
        try:
            original_mesh = pv.read(mesh_path)
            pl.add_mesh(original_mesh, color='orange', style='wireframe', line_width=1, label='Original Mesh', opacity=0.5)
        except:
            pass
            
        # Reconstructed Mesh
        pl.add_mesh(tri_mesh, color='lightblue', pbr=True, metallic=0.5, label='Reconstructed Surface')
        
        # 5. Add Contours (if available)
        if hasattr(sdf_data, 'debug_contours') and len(sdf_data.debug_contours) > 0:
            print(f"Processing {len(sdf_data.debug_contours)//6} contour segments...")
            raw_contours = np.array(sdf_data.debug_contours).reshape(-1, 6)
            points = raw_contours.reshape(-1, 3)
            n_segments = len(raw_contours)
            lines = np.column_stack((
                np.full(n_segments, 2), 
                np.arange(0, 2*n_segments, 2), 
                np.arange(1, 2*n_segments, 2)
            )).flatten()
            contour_poly = pv.PolyData(points, lines=lines)
            pl.add_mesh(contour_poly, color='lime', line_width=3, label='Cutting Contours')
        
        pl.show_grid()
        pl.add_axes()
        pl.add_legend()
        pl.add_title("SDF 3D Reconstruction Demo")
        pl.show()
        
    except Exception as e:
        print(f"Visualization error: {e}")

if __name__ == "__main__":
    main()
