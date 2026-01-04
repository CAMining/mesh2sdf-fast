import numpy as np
import pyvista as pv
import os

# Load SDF
sdf_path = os.path.join(os.path.dirname(__file__), 'data', 'plane_sdf.npy')
sdf = np.load(sdf_path)

print(f"SDF Shape: {sdf.shape}")
print(f"SDF Range: [{sdf.min():.2f}, {sdf.max():.2f}]")

# Identify unprocessed regions (FAR_VALUE)
FAR_THRESHOLD = 1e8  # Threshold to detect FAR_VALUE
unprocessed_mask = np.abs(sdf) > FAR_THRESHOLD

num_unprocessed = unprocessed_mask.sum()
total_voxels = sdf.size
print(f"\nUnprocessed voxels: {num_unprocessed} / {total_voxels} ({100*num_unprocessed/total_voxels:.2f}%)")

# Get coordinates of unprocessed voxels
unprocessed_coords = np.argwhere(unprocessed_mask)
print(f"Unprocessed coordinates shape: {unprocessed_coords.shape}")

# Create PyVista visualization
pl = pv.Plotter()

# 1. Load original mesh
mesh_path = os.path.join(os.path.dirname(__file__), 'data', 'ore.obj')
try:
    original_mesh = pv.read(mesh_path)
    pl.add_mesh(original_mesh, color='lightblue', opacity=0.3, label='Original Mesh')
    print(f"Original mesh loaded: {original_mesh.n_points} points")
except Exception as e:
    print(f"Failed to load mesh: {e}")

# 2. Create point cloud for unprocessed voxels
if num_unprocessed > 0 and num_unprocessed < 100000:  # Limit for performance
    # Sample if too many points
    if num_unprocessed > 10000:
        print(f"Sampling {num_unprocessed} points to 10000 for visualization...")
        indices = np.random.choice(num_unprocessed, 10000, replace=False)
        unprocessed_coords = unprocessed_coords[indices]
    
    # Convert voxel indices to world coordinates (assuming unit spacing for now)
    # You may need to adjust origin and spacing based on actual SDF metadata
    points = unprocessed_coords.astype(float)
    
    # Create point cloud
    point_cloud = pv.PolyData(points)
    pl.add_mesh(point_cloud, color='red', point_size=5, render_points_as_spheres=True, 
                label=f'Unprocessed Regions ({len(points)} samples)')
    print(f"Visualizing {len(points)} unprocessed voxel samples")
else:
    print(f"Too many ({num_unprocessed}) or no unprocessed voxels to visualize")

# 3. Visualize processed SDF as volume (isosurface at level=0)
# Create ImageData from SDF
processed_sdf = sdf.copy()
processed_sdf[unprocessed_mask] = sdf.max()  # Set unprocessed to max for clarity

grid = pv.ImageData()
grid.dimensions = (sdf.shape[2], sdf.shape[1], sdf.shape[0])
grid.spacing = (1, 1, 1)  # Adjust if you have actual metadata
grid.origin = (0, 0, 0)
grid.point_data["values"] = processed_sdf.flatten(order='F')

try:
    isosurface = grid.contour(isosurfaces=[0.0], scalars="values")
    pl.add_mesh(isosurface, color='green', opacity=0.5, label='SDF Isosurface (level=0)')
    print(f"Isosurface extracted: {isosurface.n_points} points")
except Exception as e:
    print(f"Failed to extract isosurface: {e}")

# Setup camera and display
pl.add_legend()
pl.add_axes()
pl.show_grid()
pl.add_title("Unprocessed Regions Visualization\nRed = FAR_VALUE, Green = SDF Surface, Blue = Original")
print("\nOpening visualization window...")
pl.show()
