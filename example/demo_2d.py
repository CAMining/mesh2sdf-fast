import numpy as np
import mesh2sdf_fast as mesh2sdf
import os
import matplotlib.pyplot as plt
import time

def main():
    print("--- Mesh2SDF 2D Slice Visualization Demo ---")
    
    # 1. Load Mesh
    mesh_path = os.path.join(os.path.dirname(__file__), 'data', 'ore.obj')
    print(f"Loading mesh from: {mesh_path}")
    
    try:
        mesh = mesh2sdf.load_mesh(mesh_path)
    except Exception as e:
        print(f"Error loading mesh: {e}")
        return

    # 2. Generate SDF
    resolution = 128
    print(f"Generating SDF with resolution {resolution}...")
    start_time = time.time()
    sdf_data = mesh2sdf.mesh_to_sdf(mesh, resolution=resolution)
    print(f"SDF generation took {time.time() - start_time:.4f} seconds")

    # 3. Visualize Multiple Slices
    print("Visualizing multiple SDF slices along Z-axis...")
    
    # Generate N evenly spaced indices
    num_slices = 64
    get_indices = lambda size, n: [int((i + 1) * size / (n + 1)) for i in range(n)]
    indices = get_indices(sdf_data.size_z, num_slices)
    
    try:
        # Using the enhanced visualize_sdf_slice with multiple indices
        mesh2sdf.visualize_sdf_slice(sdf_data, axis='z', index=indices)
        plt.suptitle(f"SDF Cross-Sections (Z-Axis, Res={resolution})")
        
        print("Opening Matplotlib window...")
        plt.show()
    except Exception as e:
        print(f"Visualization error: {e}")

    print("Demo finished.")

if __name__ == "__main__":
    main()
