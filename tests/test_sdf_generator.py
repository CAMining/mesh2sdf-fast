"""
test_sdf_generator.py - SDF生成器测试
"""

import pytest
import numpy as np
import sys
import os

# 添加项目路径
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class TestSDFConfig:
    """SDFConfig 配置测试"""
    
    def test_default_config(self):
        """测试默认配置"""
        from mesh2sdf import SDFConfig
        
        config = SDFConfig()
        assert config.resolution_x == 64
        assert config.resolution_y == 64
        assert config.resolution_z == 64
        assert config.padding == 0.1
        assert config.normalize_mesh == True
    
    def test_set_resolution(self):
        """测试设置分辨率"""
        from mesh2sdf import SDFConfig
        
        config = SDFConfig()
        config.set_resolution(32)
        
        assert config.resolution_x == 32
        assert config.resolution_y == 32
        assert config.resolution_z == 32


class TestSDFGenerator:
    """SDFGenerator 生成器测试"""
    
    def test_generate_sphere(self):
        """测试生成球体SDF"""
        from mesh2sdf import create_sphere, SDFGenerator, SDFConfig
        
        mesh = create_sphere(subdivisions=2)
        
        config = SDFConfig()
        config.set_resolution(16)  # 小分辨率加快测试
        
        generator = SDFGenerator(config)
        sdf = generator.generate(mesh)
        
        # 检查SDF尺寸
        assert sdf.size_x == 16
        assert sdf.size_y == 16
        assert sdf.size_z == 16
        
        # 检查数据有效性
        arr = sdf.to_numpy()
        assert arr.shape == (16, 16, 16)
        
        # 球体中心附近应该是负值（内部）
        center = 8
        center_val = sdf.at(center, center, center)
        assert center_val < 0, f"球心应为负值（内部），得到: {center_val}"
        
        # 边角应该是正值（外部）
        corner_val = sdf.at(0, 0, 0)
        assert corner_val > 0, f"角点应为正值（外部），得到: {corner_val}"
    
    def test_generate_cube(self):
        """测试生成立方体SDF"""
        from mesh2sdf import create_cube, mesh_to_sdf
        
        mesh = create_cube()
        sdf = mesh_to_sdf(mesh, resolution=16)
        
        # 检查SDF尺寸
        assert sdf.size_x == 16
        assert sdf.size_y == 16
        assert sdf.size_z == 16
    
    def test_progress_callback(self):
        """测试进度回调"""
        from mesh2sdf import create_sphere, mesh_to_sdf
        
        mesh = create_sphere(subdivisions=1)
        progress_values = []
        
        def on_progress(current, total):
            progress_values.append((current, total))
        
        sdf = mesh_to_sdf(mesh, resolution=8, progress_callback=on_progress)
        
        # 应该有进度回调
        assert len(progress_values) > 0
        # 最后一次回调应该是完成
        assert progress_values[-1][0] == progress_values[-1][1]


class TestSDFData:
    """SDFData 数据测试"""
    
    def test_to_numpy(self):
        """测试转换为NumPy数组"""
        from mesh2sdf import create_sphere, mesh_to_sdf, sdf_to_numpy
        
        mesh = create_sphere(subdivisions=1)
        sdf = mesh_to_sdf(mesh, resolution=8)
        
        arr = sdf_to_numpy(sdf)
        
        assert isinstance(arr, np.ndarray)
        assert arr.shape == (8, 8, 8)
        assert arr.dtype == np.int32
    
    def test_grid_to_world(self):
        """测试网格坐标到世界坐标转换"""
        from mesh2sdf import create_cube, mesh_to_sdf
        
        mesh = create_cube()
        sdf = mesh_to_sdf(mesh, resolution=16)
        
        # 获取中心点的世界坐标
        world_pos = sdf.grid_to_world(8, 8, 8)
        
        # 归一化后中心应接近原点
        assert abs(world_pos.x) < 1.0
        assert abs(world_pos.y) < 1.0
        assert abs(world_pos.z) < 1.0


class TestCSVIO:
    """CSV 导入导出测试"""
    
    def test_save_and_load(self):
        """测试保存和加载CSV"""
        import tempfile
        from mesh2sdf import create_sphere, mesh_to_sdf, save_sdf_csv, load_sdf_csv
        
        mesh = create_sphere(subdivisions=1)
        sdf = mesh_to_sdf(mesh, resolution=8)
        
        # 保存到临时文件
        with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as f:
            temp_path = f.name
        
        try:
            save_sdf_csv(sdf, temp_path)
            
            # 加载回来
            loaded_sdf = load_sdf_csv(temp_path)
            
            # 检查尺寸
            assert loaded_sdf.size_x == sdf.size_x
            assert loaded_sdf.size_y == sdf.size_y
            assert loaded_sdf.size_z == sdf.size_z
            
            # 检查数据
            for z in range(min(2, sdf.size_z)):
                for y in range(min(2, sdf.size_y)):
                    for x in range(min(2, sdf.size_x)):
                        assert loaded_sdf.at(x, y, z) == sdf.at(x, y, z)
        finally:
            os.unlink(temp_path)


class TestHighLevelAPI:
    """高层API测试"""
    
    def test_mesh_to_sdf_with_path(self):
        """测试使用无效路径"""
        from mesh2sdf import mesh_to_sdf
        from mesh2sdf.mesh_loader import MeshFileNotFoundError
        
        with pytest.raises(MeshFileNotFoundError):
            mesh_to_sdf("nonexistent.stl")
    
    def test_invalid_resolution(self):
        """测试无效分辨率"""
        from mesh2sdf import mesh_to_sdf, create_sphere
        
        mesh = create_sphere()
        
        with pytest.raises(ValueError):
            mesh_to_sdf(mesh, resolution=0)
        
        with pytest.raises(ValueError):
            mesh_to_sdf(mesh, resolution=-1)
    
    def test_invalid_padding(self):
        """测试无效填充"""
        from mesh2sdf import mesh_to_sdf, create_sphere
        
        mesh = create_sphere()
        
        with pytest.raises(ValueError):
            mesh_to_sdf(mesh, padding=-0.1)
    
    def test_generate_sample_data(self):
        """测试生成样本数据"""
        import tempfile
        from mesh2sdf import generate_sample_data
        
        with tempfile.NamedTemporaryFile(suffix=".csv", delete=False) as f:
            temp_path = f.name
        
        try:
            sdf = generate_sample_data(temp_path, shape="sphere", resolution=8)
            
            assert sdf.size_x == 8
            assert os.path.exists(temp_path)
        finally:
            if os.path.exists(temp_path):
                os.unlink(temp_path)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
