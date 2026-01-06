/**
 * @file bindings.cpp
 * @brief pybind11 Python bindings
 * 
 * Export C++ core interface as Python module。
 */

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/numpy.h>

#include "mesh2sdf/mesh.hpp"
#include "mesh2sdf/plane_cutter.hpp"
#include "mesh2sdf/distance_transform.hpp"
#include "mesh2sdf/sdf_generator.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_core, m) {
    m.doc() = "Mesh2SDF-Fast: Fast signed distance field generation from mesh models";
    
    // Vec3 class
    py::class_<mesh2sdf::Vec3>(m, "Vec3", "3D vector")
        .def(py::init<>())
        .def(py::init<double, double, double>(), 
             py::arg("x"), py::arg("y"), py::arg("z"))
        .def_readwrite("x", &mesh2sdf::Vec3::x)
        .def_readwrite("y", &mesh2sdf::Vec3::y)
        .def_readwrite("z", &mesh2sdf::Vec3::z)
        .def("__repr__", [](const mesh2sdf::Vec3& v) {
            return "Vec3(" + std::to_string(v.x) + ", " + 
                   std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
        });
    
    // Vec2 class
    py::class_<mesh2sdf::Vec2>(m, "Vec2", "2D vector")
        .def(py::init<>())
        .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
        .def_readwrite("x", &mesh2sdf::Vec2::x)
        .def_readwrite("y", &mesh2sdf::Vec2::y);
    
    // Triangle class
    py::class_<mesh2sdf::Triangle>(m, "Triangle", "triangle")
        .def(py::init<>())
        .def(py::init<size_t, size_t, size_t>(), 
             py::arg("v0"), py::arg("v1"), py::arg("v2"))
        .def_readwrite("v0", &mesh2sdf::Triangle::v0)
        .def_readwrite("v1", &mesh2sdf::Triangle::v1)
        .def_readwrite("v2", &mesh2sdf::Triangle::v2)
        .def_readwrite("normal", &mesh2sdf::Triangle::normal);
    
    // BoundingBox class
    py::class_<mesh2sdf::BoundingBox>(m, "BoundingBox", "Axis-aligned bounding box")
        .def(py::init<>())
        .def_readwrite("min_corner", &mesh2sdf::BoundingBox::min_corner)
        .def_readwrite("max_corner", &mesh2sdf::BoundingBox::max_corner)
        .def("center", &mesh2sdf::BoundingBox::center)
        .def("size", &mesh2sdf::BoundingBox::size)
        .def("max_extent", &mesh2sdf::BoundingBox::max_extent);
    
    // Mesh class
    py::class_<mesh2sdf::Mesh>(m, "Mesh", "Mesh data")
        .def(py::init<>())
        .def_readwrite("vertices", &mesh2sdf::Mesh::vertices)
        .def_readwrite("triangles", &mesh2sdf::Mesh::triangles)
        .def("compute_bounding_box", &mesh2sdf::Mesh::compute_bounding_box,
             "Compute mesh bounding box")
        .def("compute_normals", &mesh2sdf::Mesh::compute_normals,
             "Compute normals for all triangles")
        .def("normalize", &mesh2sdf::Mesh::normalize,
             "Normalize mesh to [-1, 1] range")
        .def("is_valid", &mesh2sdf::Mesh::is_valid,
             "Check if mesh is valid")
        .def("vertex_count", &mesh2sdf::Mesh::vertex_count,
             "Get vertex count")
        .def("triangle_count", &mesh2sdf::Mesh::triangle_count,
             "Get triangle count")
        .def("add_polygon", &mesh2sdf::Mesh::add_polygon, 
             py::arg("indices"), 
             "Add a polygon (list of vertex indices). Automatically triangulates n-gons.");
    
    // Mesh loading functions
    m.def("load_stl", &mesh2sdf::load_stl, py::arg("filepath"),
          "Load mesh from STL file");
    m.def("load_obj", &mesh2sdf::load_obj, py::arg("filepath"),
          "Load mesh from OBJ file");
    m.def("create_sphere", &mesh2sdf::create_sphere, 
          py::arg("subdivisions") = 2,
          "Create unit sphere mesh");
    m.def("create_cube", &mesh2sdf::create_cube,
          "Create unit cube mesh");
    
    // SDFConfig class
    py::class_<mesh2sdf::SDFConfig>(m, "SDFConfig", "SDF generation configuration")
        .def(py::init<>())
        .def_readwrite("resolution_x", &mesh2sdf::SDFConfig::resolution_x)
        .def_readwrite("resolution_y", &mesh2sdf::SDFConfig::resolution_y)
        .def_readwrite("resolution_z", &mesh2sdf::SDFConfig::resolution_z)
        .def_readwrite("padding", &mesh2sdf::SDFConfig::padding)
        .def_readwrite("normalize_mesh", &mesh2sdf::SDFConfig::normalize_mesh)
        .def_readwrite("export_contours", &mesh2sdf::SDFConfig::export_contours)
        .def_readwrite("slice_x", &mesh2sdf::SDFConfig::slice_x)
        .def_readwrite("slice_y", &mesh2sdf::SDFConfig::slice_y)
        .def_readwrite("slice_z", &mesh2sdf::SDFConfig::slice_z)
        .def("set_resolution", &mesh2sdf::SDFConfig::set_resolution,
             py::arg("res"), "setuniformresolution");
    
    // SDFData class
    py::class_<mesh2sdf::SDFData>(m, "SDFData", "SDF data")
        .def(py::init<>())
        .def_readwrite("size_x", &mesh2sdf::SDFData::size_x)
        .def_readwrite("size_y", &mesh2sdf::SDFData::size_y)
        .def_readwrite("size_z", &mesh2sdf::SDFData::size_z)
        .def_readwrite("origin_x", &mesh2sdf::SDFData::origin_x)
        .def_readwrite("origin_y", &mesh2sdf::SDFData::origin_y)
        .def_readwrite("origin_z", &mesh2sdf::SDFData::origin_z)
        .def_readwrite("cell_size_x", &mesh2sdf::SDFData::cell_size_x)
        .def_readwrite("cell_size_y", &mesh2sdf::SDFData::cell_size_y)
        .def_readwrite("cell_size_z", &mesh2sdf::SDFData::cell_size_z)
        .def_readwrite("values", &mesh2sdf::SDFData::values)
        .def_readwrite("debug_contours", &mesh2sdf::SDFData::debug_contours)
        .def("at", &mesh2sdf::SDFData::at,
             py::arg("x"), py::arg("y"), py::arg("z"),
             "Get SDF value at specified position")
        .def("grid_to_world", &mesh2sdf::SDFData::grid_to_world,
             py::arg("x"), py::arg("y"), py::arg("z"),
             "Convert mesh coordinates to world coordinates")
        .def("to_real_distance", &mesh2sdf::SDFData::to_real_distance,
             py::arg("grid_dist"), "Convert mesh distance to real distance")
        .def("to_numpy", [](const mesh2sdf::SDFData& sdf) {
            // Create NumPy array
            py::array_t<float> arr({sdf.size_z, sdf.size_y, sdf.size_x});
            auto buf = arr.mutable_unchecked<3>();
            for (int z = 0; z < sdf.size_z; ++z) {
                for (int y = 0; y < sdf.size_y; ++y) {
                    for (int x = 0; x < sdf.size_x; ++x) {
                        buf(z, y, x) = sdf.at(x, y, z);
                    }
                }
            }
            return arr;
        }, "Convert SDF data to NumPy array (shape: z, y, x)");
    
    // SDFGenerator class
    py::class_<mesh2sdf::SDFGenerator>(m, "SDFGenerator", "SDFgenerator")
        .def(py::init<const mesh2sdf::SDFConfig&>(), py::arg("config"))
        .def("generate", &mesh2sdf::SDFGenerator::generate,
             py::arg("mesh"), py::arg("progress") = nullptr,
             "Generate SDF from mesh");
    
    // CSVimportexport
    m.def("save_sdf_csv", &mesh2sdf::save_sdf_csv,
          py::arg("sdf"), py::arg("filepath"),
          "Save SDF data as CSV file");
    m.def("load_sdf_csv", &mesh2sdf::load_sdf_csv,
          py::arg("filepath"),
          "Load SDF data from CSV file");
    
    // Exceptions
    py::register_exception<mesh2sdf::MeshLoadError>(m, "MeshLoadError");
}
