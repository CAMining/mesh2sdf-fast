import numpy as np
import mesh2sdf_fast as mesh2sdf
import os
import pyvista as pv
import time

def main():
    print(f"Mesh2SDF version: {mesh2sdf.__version__}")
    
    # 1. Load Mesh
    mesh_path = os.path.join(os.path.dirname(__file__), 'data', 'ore.obj')
    print(f"Loading mesh from: {mesh_path}")
    
    try:
        mesh = mesh2sdf.load_mesh(mesh_path)
        info = mesh2sdf.get_mesh_info(mesh)
        print(f"Mesh loaded: {info['vertex_count']} vertices, {info['triangle_count']} triangles")
    except Exception as e:
        print(f"Error loading mesh: {e}")
        return

    # 2. Generate SDF
    resolution = 128
    padding = 0.0
    print(f"Generating SDF with resolution {resolution}...")
    start_time = time.time()
    # Pass the loaded mesh object directly
    sdf_data = mesh2sdf.mesh_to_sdf(
        mesh, 
        resolution=resolution, 
        padding=padding, 
        normalize=False,
        export_contours=False,
        slice_x=True,
        slice_y=False,
        slice_z=False
    )
    end_time = time.time()
    print(f"SDF generation took {end_time - start_time:.4f} seconds")

    # 3. Convert to NumPy
    sdf_np = mesh2sdf.sdf_to_numpy(sdf_data)
    print(f"SDF Grid Shape: {sdf_np.shape}")

    # 4. Marching Cubes with PyVista
    print("Running Marching Cubes (PyVista Contour)...")
    try:      
        # Create PyVista Grid (ImageData)
        grid = pv.ImageData()
        # Dimensions are node counts (Nx, Ny, Nz)
        grid.dimensions = (sdf_data.size_x, sdf_data.size_y, sdf_data.size_z)
        grid.origin = (sdf_data.origin_x, sdf_data.origin_y, sdf_data.origin_z)
        grid.spacing = (sdf_data.cell_size_x, sdf_data.cell_size_y, sdf_data.cell_size_z)
        
        # Add Data
        grid.point_data["values"] = sdf_np.flatten()
        
        # Determine Level
        min_val = sdf_np.min()
        max_val = sdf_np.max()
        target_level = 0.0
                   
        # Extract Isosurface
        recon_mesh = grid.contour(isosurfaces=[target_level], scalars="values")
        
        print(f"Extracted {recon_mesh.n_points} vertices and {recon_mesh.n_cells} faces")
        
        # Save for verification
        # output_path = os.path.join(os.path.dirname(mesh_path), 'plane_reconstructed.obj')
        tri_mesh = recon_mesh.triangulate()
        # tri_mesh.save(output_path)
        # print(f"Reconstructed mesh saved to: {output_path}")

        # Save SDF Data (New)
        sdf_output_path = os.path.join(os.path.dirname(mesh_path), 'plane_sdf.npy')
        np.save(sdf_output_path, np.array(sdf_data.values).reshape(sdf_data.size_z, sdf_data.size_y, sdf_data.size_x))
        print(f"SDF data saved to: {sdf_output_path}")

        # 5. Process Contours (if available)
        contours_obj_path = os.path.join(os.path.dirname(mesh_path), 'plane_contours.obj')
        contour_lines = None
        if hasattr(sdf_data, 'debug_contours') and len(sdf_data.debug_contours) > 0:
            print(f"Processing {len(sdf_data.debug_contours)//6} contour segments...")
            raw_contours = np.array(sdf_data.debug_contours).reshape(-1, 6)

            # PyVista Visualization
            points = raw_contours.reshape(-1, 3)
            n_segments = len(raw_contours)
            lines = np.column_stack((np.full(n_segments, 2), np.arange(0, 2*n_segments, 2), np.arange(1, 2*n_segments, 2))).flatten()
            contour_lines = pv.PolyData(points, lines=lines)

        # 6. Show with PyVista
        print("Visualization with PyVista...")
        
        pl = pv.Plotter()
        
        # Original Mesh
        try:
            original_mesh = pv.read(mesh_path)
            pl.add_mesh(original_mesh, color='orange', style='wireframe', line_width=2, label='Original Mesh')
        except Exception as e:
            print(f"Failed to load original mesh for visualization: {e}")
            
        # Reconstructed Mesh
        pl.add_mesh(tri_mesh, show_edges=False, color='lightblue', pbr=True, metallic=0.5, opacity=1.0, label='Reconstructed Mesh')
        
        # Contours (Green Lines)
        if contour_lines:
            pl.add_mesh(contour_lines, color='lime', line_width=3, label='Scanline Contours')
        
        
        pl.show_grid()
        pl.add_axes()
        pl.add_legend()
        pl.add_title(f"Reconstructed vs Original ({tri_mesh.n_cells} faces)")
        
        print("Opening PyVista window...")
        pl.show()
        
    except ImportError:
        print("PyVista not found. Please install it: pip install pyvista")
    except Exception as e:
        print(f"PyVista error: {e}")
        import traceback
        traceback.print_exc()
    print("Window closed.")
        


if __name__ == "__main__":
    main()
