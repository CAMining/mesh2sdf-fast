# Mesh2SDF-Fast

中文 | **[English](README.md)**

从网格模型快速生成符号距离场（Signed Distance Field）的高性能库。

## 特性

- **高性能**：C++ 核心实现，支持 OpenMP 多线程
- **简洁API**：Python 接口简单易用
- **多格式支持**：支持 STL、OBJ 网格文件
- **曼哈顿距离**：使用整数步进距离变换，高效准确

## 算法原理

1. **平面切割**：用体素每一层所在平面切割网格，生成轮廓线
2. **2D距离变换**：使用曼哈顿距离变体（两遍扫描算法）
3. **方向传播**：合并各层距离场，完成3D SDF生成
4. **自动闭合**：自动检测并闭合非封闭网格产生的开放轮廓线，确保奇偶校验（Jordan Curve Theorem）的有效性。

## 注意事项

- **网格质量**：得益于稳健的自动闭合算法，现在支持处理**非封闭**（Open/Non-watertight）网格以及包含重复面的网格。
- **网格类型**：支持任意多边形网格（加载时自动进行三角化）。为获得最佳效果，请确保网格是流形且封闭的，尽管该算法对许多常见的网格缺陷具有鲁棒性。
- **预处理**：如果遇到重构结果出现瑕疵，建议先使用工具（如 Blender 或 MeshLab）对模型进行“修复”或“封口”处理。
- **数值精度**：如果坐标数值太大（超过浮点数所能精确表达的范围），通过 Marching Cubes 等算法生成的模型可能会显得不够光滑（锯齿感）。建议在处理前将模型平移到原点附近或进行缩放。
- **平行边界**：由于本算法采用平面切割 2D 距离变换方式，在极限情况下，如果网格的边界正好与切割面平行，可能无法正确切割出轮廓线。对于这种情况，可以考虑从两个甚至三个方向进行切割（参考 `mesh_to_sdf` 的调用参数开启 X/Y/Z 多轴切片）以确保完整覆盖。

## 安装

### 从源码安装

```bash
# 克隆仓库
git clone https://github.com/csubilin/mesh2sdf-fast.git
cd mesh2sdf-fast

# 安装（选择一种）
pip install -e .              # 仅核心功能 (numpy)
pip install -e ".[viz]"       # + 可视化功能 (pyvista, matplotlib)
pip install -e ".[dev]"       # + 开发工具 (pytest, pytest-cov)
pip install -e ".[all]"       # 所有可选依赖
```

### 依赖

- Python >= 3.8
- NumPy >= 1.20
- CMake >= 3.15
- C++17 编译器

## 快速开始

最简单的入门方式是运行提供的示例脚本：

```bash
# 3D 重建与可视化 (PyVista)
python example/demo_3d.py

# 2D 切片可视化 (Matplotlib)
python example/demo_2d.py
```

### 基本用法

```python
import mesh2sdf_fast as mesh2sdf

# 从网格文件生成 SDF
sdf = mesh2sdf.mesh_to_sdf("model.obj", resolution=64)

# 获取指定的 SDF 值
val = sdf.at(10, 20, 30)

# 转换为 NumPy 数组
sdf_array = sdf.to_numpy() # 形状: (res+1, res+1, res+1)
```

### 生成测试数据

```python
from mesh2sdf_fast import generate_sample_data

# 生成球体SDF
sdf = generate_sample_data("sample_data.csv", shape="sphere", resolution=32)
```

### 使用内置形状

```python
from mesh2sdf_fast import create_sphere, create_cube, mesh_to_sdf

# 创建球体网格
sphere = create_sphere(subdivisions=3)
sdf = mesh_to_sdf(sphere, resolution=64)

# 创建立方体网格
cube = create_cube()
sdf = mesh_to_sdf(cube, resolution=64)
```

## API 参考

### 主要函数

| 函数 | 描述 |
|------|------|
| `mesh_to_sdf(mesh, resolution=64)` | 网格转SDF |
| `load_mesh(filepath)` | 加载网格文件 |
| `create_sphere(subdivisions=2)` | 创建球体网格 |
| `create_cube()` | 创建立方体网格 |
| `save_sdf_csv(sdf, path)` | 保存SDF到CSV |
| `load_sdf_csv(path)` | 从CSV加载SDF |
| `visualize_sdf_slice(sdf, axis='z')` | 可视化切片 |

### SDFData 属性

| 属性 | 描述 |
|------|------|
| `size_x, size_y, size_z` | 体素尺寸 |
| `origin_x, origin_y, origin_z` | 原点坐标 |
| `cell_size` | 单元格大小 |
| `at(x, y, z)` | 获取SDF值 |
| `to_numpy()` | 转换为NumPy数组 |

## 开发与调试

### 切换编译模式

为了获得最佳性能，请使用 **Release** 模式（默认）。如果需要调试 C++ 代码（例如在 VS Code 中使用 GDB），请使用 **Debug** 模式。

#### 方式 A：临时切换（推荐）
通过命令行安装时指定模式，无需修改 `pyproject.toml`：
```bash
# 切换到 Debug 模式
pip install -e . --config-settings=cmake.build-type=Debug

# 切换回 Release 模式
pip install -e . --config-settings=cmake.build-type=Release
```

#### 方式 B：持久化配置
直接修改 `pyproject.toml` 文件中的相关字段：
```toml
[tool.scikit-build]
cmake.build-type = "Debug"  # 生产环境请改为 "Release"

[tool.scikit-build.cmake.define]
CMAKE_BUILD_TYPE = "Debug"
```

### VS Code 调试
如果您配置了 `.vscode/launch.json` 用于 C++ 调试，请务必先使用上述方法之一以 **Debug** 模式重新安装包，以确保包含调试符号并禁用编译器优化。

## 目录结构

```
Mesh2SDF-Fast/
├── include/mesh2sdf/     # C++ 头文件
├── src/                  # C++ 源文件
├── mesh2sdf_fast/        # Python 包
├── example/              # 使用示例与演示
│   └── data/             # 示例网格数据
├── tests/                # 单元测试
├── CMakeLists.txt        # CMake 配置
└── pyproject.toml        # Python 项目配置
```

## 许可证

MIT License
