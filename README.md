# Mesh2SDF-Fast

**[中文文档](README_zh.md)** | English

Fast Signed Distance Field (SDF) generation from mesh models - A high-performance library.

## Features

- **High Performance**: C++ core implementation with OpenMP multithreading support
- **Simple API**: Easy-to-use Python interface
- **Multiple Formats**: Support for STL and OBJ mesh files
- **Manhattan Distance**: Efficient and accurate integer-step distance transform

## Algorithm

1. **Plane Slicing**: Slice mesh with planes at each voxel layer to generate contour lines
2. **2D Distance Transform**: Manhattan distance variant with two-pass scanning algorithm
3. **Multi-Axis Fusion**: Merge distance fields from X, Y, Z axes for robust 3D SDF generation

## Installation

### From Source

```bash
# Clone repository
git clone https://github.com/csubilin/mesh2sdf-fast.git
cd mesh2sdf-fast

# Install (choose one)
pip install -e .              # Core only (numpy)
pip install -e ".[viz]"       # + Visualization (pyvista, matplotlib)
pip install -e ".[dev]"       # + Development tools (pytest, pytest-cov)
pip install -e ".[all]"       # All optional dependencies
```

### Requirements

- Python >= 3.8
- NumPy >= 1.20
- CMake >= 3.15
- C++17 compiler

## Quick Start

The easiest way to get started is to run the provided demo script, which handles mesh loading, SDF generation, and visualization:

```bash
python example/demo.py
```

### Basic Usage

```python
import mesh2sdf_fast as mesh2sdf

# Generate SDF from mesh file
sdf = mesh2sdf.mesh_to_sdf("model.obj", resolution=64)

# Get SDF values
val = sdf.at(10, 20, 30)

# Convert to NumPy array
sdf_array = sdf.to_numpy() # Shape: (res+1, res+1, res+1)
```

### Generate Test Data

```python
from mesh2sdf_fast import generate_sample_data

# Generate sphere SDF
sdf = generate_sample_data("sample_data.csv", shape="sphere", resolution=32)
```

### Using Built-in Shapes

```python
from mesh2sdf_fast import create_sphere, create_cube, mesh_to_sdf

# Create sphere mesh
sphere = create_sphere(subdivisions=3)
sdf = mesh_to_sdf(sphere, resolution=64)

# Create cube mesh
cube = create_cube()
sdf = mesh_to_sdf(cube, resolution=64)
```

## API Reference

### Main Functions

| Function | Description |
|----------|-------------|
| `mesh_to_sdf(mesh, resolution=64)` | Convert mesh to SDF |
| `load_mesh(filepath)` | Load mesh file |
| `create_sphere(subdivisions=2)` | Create sphere mesh |
| `create_cube()` | Create cube mesh |
| `save_sdf_csv(sdf, path)` | Save SDF to CSV |
| `load_sdf_csv(path)` | Load SDF from CSV |
| `visualize_sdf_slice(sdf, axis='z')` | Visualize slice |

### SDFData Properties

| Property | Description |
|----------|-------------|
| `size_x, size_y, size_z` | Voxel dimensions |
| `origin_x, origin_y, origin_z` | Origin coordinates |
| `cell_size` | Cell size |
| `at(x, y, z)` | Get SDF value |
| `to_numpy()` | Convert to NumPy array |

## Development & Debugging

### Switching Build Types

For performance, use **Release** mode (default). For debugging C++ code (e.g., using GDB/LLDB in VS Code), use **Debug** mode.

#### Option A: Temporary (Recommended)
Reinstall with the specific build type without modifying `pyproject.toml`:
```bash
# For Debug
pip install -e . --config-settings=cmake.build-type=Debug

# For Release
pip install -e . --config-settings=cmake.build-type=Release
```

#### Option B: Persistent
Modify `pyproject.toml` directly:
```toml
[tool.scikit-build]
cmake.build-type = "Debug"  # Change to "Release" for production

[tool.scikit-build.cmake.define]
CMAKE_BUILD_TYPE = "Debug"
```

### VS Code Debugging
If you have configured `.vscode/launch.json` for C++ debugging, ensure you have reinstalled the package in **Debug** mode using one of the methods above to include debug symbols and disable optimizations.

## Directory Structure

```
Mesh2SDF-Fast/
├── include/mesh2sdf/     # C++ header files
├── src/                  # C++ source files
├── mesh2sdf_fast/        # Python package
├── tests/                # Test code
├── notebooks/            # Jupyter examples
├── CMakeLists.txt        # CMake configuration
└── pyproject.toml        # Python project configuration
```

## License

MIT License
