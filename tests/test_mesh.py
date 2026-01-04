"""
test_mesh.py - 网格模块测试
"""

import pytest
import sys
import os

# 添加项目路径
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class TestVec3:
    """Vec3 向量测试"""
    
    def test_creation(self):
        """测试创建向量"""
        from mesh2sdf import Vec3
        
        v = Vec3(1.0, 2.0, 3.0)
        assert v.x == 1.0
        assert v.y == 2.0
        assert v.z == 3.0
    
    def test_default_creation(self):
        """测试默认构造"""
        from mesh2sdf import Vec3
        
        v = Vec3()
        assert v.x == 0.0
        assert v.y == 0.0
        assert v.z == 0.0


class TestMesh:
    """Mesh 网格测试"""
    
    def test_create_sphere(self):
        """测试创建球体"""
        from mesh2sdf import create_sphere
        
        mesh = create_sphere(subdivisions=1)
        assert mesh.is_valid()
        assert mesh.vertex_count() > 0
        assert mesh.triangle_count() > 0
    
    def test_create_cube(self):
        """测试创建立方体"""
        from mesh2sdf import create_cube
        
        mesh = create_cube()
        assert mesh.is_valid()
        assert mesh.vertex_count() == 8
        assert mesh.triangle_count() == 12
    
    def test_bounding_box(self):
        """测试包围盒计算"""
        from mesh2sdf import create_cube
        
        mesh = create_cube()
        bbox = mesh.compute_bounding_box()
        
        # 立方体的包围盒应该是 [-1, 1] x [-1, 1] x [-1, 1]
        assert abs(bbox.min_corner.x - (-1)) < 1e-6
        assert abs(bbox.max_corner.x - 1) < 1e-6
        assert abs(bbox.max_extent() - 2) < 1e-6
    
    def test_normalize(self):
        """测试网格归一化"""
        from mesh2sdf import create_cube
        
        mesh = create_cube()
        mesh.normalize()
        bbox = mesh.compute_bounding_box()
        
        # 归一化后应该在 [-1, 1] 范围内
        assert bbox.min_corner.x >= -1.1
        assert bbox.max_corner.x <= 1.1


class TestMeshLoader:
    """网格加载器测试"""
    
    def test_file_not_found(self):
        """测试文件不存在异常"""
        from mesh2sdf import load_mesh
        from mesh2sdf.mesh_loader import MeshFileNotFoundError
        
        with pytest.raises(MeshFileNotFoundError):
            load_mesh("nonexistent_file.stl")
    
    def test_unsupported_format(self):
        """测试不支持的格式"""
        from mesh2sdf import load_mesh
        from mesh2sdf.mesh_loader import MeshFormatError
        
        # 创建临时文件
        import tempfile
        with tempfile.NamedTemporaryFile(suffix=".xyz", delete=False) as f:
            f.write(b"test")
            temp_path = f.name
        
        try:
            with pytest.raises(MeshFormatError):
                load_mesh(temp_path)
        finally:
            os.unlink(temp_path)
