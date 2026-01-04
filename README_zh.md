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
3. **Z方向传播**：合并各层距离场，完成3D SDF生成

## 安装

### 从源码安装

```bash
# 克隆仓库
git clone https://github.com/csubilin/mesh2sdf-fast.git
cd mesh2sdf-fast

# 安装依赖
pip install scikit-build-core pybind11 cmake

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

```python
from mesh2sdf_fast import mesh_to_sdf, visualize_sdf_slice
import matplotlib.pyplot as plt

# 从网格文件生成SDF
sdf = mesh_to_sdf("model.stl", resolution=64)

# 查看SDF信息
print(f"SDF尺寸: {sdf.size_x}x{sdf.size_y}x{sdf.size_z}")
print(f"单元格大小: {sdf.cell_size}")

# 可视化中间切片
visualize_sdf_slice(sdf, axis='z')
plt.show()
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

## 目录结构

```
Mesh2SDF-Fast/
├── include/mesh2sdf/     # C++ 头文件
├── src/                  # C++ 源文件
├── mesh2sdf_fast/        # Python 包
├── tests/                # 测试代码
├── notebooks/            # Jupyter 示例
├── CMakeLists.txt        # CMake 配置
└── pyproject.toml        # Python 项目配置
```

## 许可证

MIT License
