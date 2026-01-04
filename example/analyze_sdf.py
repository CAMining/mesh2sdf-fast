import numpy as np
import os

# Load the saved SDF
sdf_path = os.path.join(os.path.dirname(__file__), 'data', 'plane_sdf.npy')
sdf = np.load(sdf_path)

print(f"SDF Shape: {sdf.shape}")
print(f"SDF Range: [{sdf.min():.6f}, {sdf.max():.6f}]")
print(f"SDF Mean: {sdf.mean():.6f}")
print(f"SDF Std: {sdf.std():.6f}")

# Check for discontinuities
# Calculate gradients along each axis
grad_x = np.abs(np.diff(sdf, axis=2))
grad_y = np.abs(np.diff(sdf, axis=1))
grad_z = np.abs(np.diff(sdf, axis=0))

print(f"\n--- Gradient Analysis ---")
print(f"Gradient X - Max: {grad_x.max():.6f}, Mean: {grad_x.mean():.6f}")
print(f"Gradient Y - Max: {grad_y.max():.6f}, Mean: {grad_y.mean():.6f}")
print(f"Gradient Z - Max: {grad_z.max():.6f}, Mean: {grad_z.mean():.6f}")

# Identify large jumps (potential discontinuities)
threshold = 1.0  # Adjust based on expected smoothness
discontinuities_x = np.sum(grad_x > threshold)
discontinuities_y = np.sum(grad_y > threshold)
discontinuities_z = np.sum(grad_z > threshold)

print(f"\n--- Discontinuities (gradient > {threshold}) ---")
print(f"X-axis: {discontinuities_x} locations ({100*discontinuities_x/grad_x.size:.2f}%)")
print(f"Y-axis: {discontinuities_y} locations ({100*discontinuities_y/grad_y.size:.2f}%)")
print(f"Z-axis: {discontinuities_z} locations ({100*discontinuities_z/grad_z.size:.2f}%)")

# Check zero-crossing smoothness (near surface)
near_surface = np.abs(sdf) < 0.5  # Values close to surface
print(f"\n--- Near-Surface Analysis (|SDF| < 0.5) ---")
print(f"Near-surface voxels: {near_surface.sum()} ({100*near_surface.sum()/sdf.size:.2f}%)")

if near_surface.sum() > 0:
    surface_values = sdf[near_surface]
    print(f"Surface values range: [{surface_values.min():.6f}, {surface_values.max():.6f}]")
    print(f"Surface values std: {surface_values.std():.6f}")
