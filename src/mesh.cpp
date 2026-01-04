/**
 * @file mesh.cpp
 * @brief Mesh data structure implementation
 */

#include "mesh2sdf/mesh.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <map>

namespace mesh2sdf {

Mesh load_stl(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw MeshLoadError("Cannot open file: " + filepath);
    }
    
    Mesh mesh;
    
    // Read header (80 bytes)
    char header[80];
    file.read(header, 80);
    if (!file) {
        throw MeshLoadError("Cannot read STL header: " + filepath);
    }
    
    // Check if ASCII format
    std::string header_str(header, 5);
    if (header_str == "solid") {
        // Possibly ASCII format, fallback after binary read fails
        file.seekg(0);
        std::string line;
        std::getline(file, line);
        
        // True ASCII format
        if (line.find("solid") != std::string::npos) {
            file.seekg(0);
            return load_stl(filepath);  // TODO: Implement ASCII STL loading
        }
    }
    
    // Read triangle count
    uint32_t num_triangles;
    file.read(reinterpret_cast<char*>(&num_triangles), 4);
    if (!file) {
        throw MeshLoadError("Cannot read triangle count: " + filepath);
    }
    
    // Read each triangle
    for (uint32_t i = 0; i < num_triangles; ++i) {
        // Normal vector (12 bytes)
        float normal[3];
        file.read(reinterpret_cast<char*>(normal), 12);
        
        // Three vertices (36 bytes)
        float v[3][3];
        file.read(reinterpret_cast<char*>(v), 36);
        
        // Attribute byte count (2 bytes)
        uint16_t attr;
        file.read(reinterpret_cast<char*>(&attr), 2);
        
        if (!file) {
            throw MeshLoadError("Failed to read triangle data: " + filepath);
        }
        
        // Add vertices and triangles
        size_t base_idx = mesh.vertices.size();
        mesh.vertices.emplace_back(v[0][0], v[0][1], v[0][2]);
        mesh.vertices.emplace_back(v[1][0], v[1][1], v[1][2]);
        mesh.vertices.emplace_back(v[2][0], v[2][1], v[2][2]);
        
        Triangle tri(base_idx, base_idx + 1, base_idx + 2);
        tri.normal = Vec3(normal[0], normal[1], normal[2]);
        mesh.triangles.push_back(tri);
    }
    
    return mesh;
}

Mesh load_obj(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw MeshLoadError("Cannot open file: " + filepath);
    }
    
    Mesh mesh;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;
        
        if (prefix == "v") {
            // vertex
            double x, y, z;
            iss >> x >> y >> z;
            mesh.vertices.emplace_back(x, y, z);
        } else if (prefix == "f") {
            // Face (only triangles supported)
            std::string v0_str, v1_str, v2_str;
            iss >> v0_str >> v1_str >> v2_str;
            
            // Parse vertex index（format: v or v/vt or v/vt/vn or v//vn）
            auto parse_vertex = [](const std::string& s) -> size_t {
                size_t pos = s.find('/');
                std::string idx_str = (pos != std::string::npos) ? s.substr(0, pos) : s;
                return std::stoul(idx_str) - 1;  // OBJ indices start from 1
            };
            
            try {
                size_t i0 = parse_vertex(v0_str);
                size_t i1 = parse_vertex(v1_str);
                size_t i2 = parse_vertex(v2_str);
                mesh.triangles.emplace_back(i0, i1, i2);
            } catch (...) {
                // Skip unparseable face
            }
        }
    }
    
    if (mesh.vertices.empty() || mesh.triangles.empty()) {
        throw MeshLoadError("OBJ file empty or format error: " + filepath);
    }
    
    // Compute normals
    mesh.compute_normals();
    
    return mesh;
}

Mesh create_sphere(int subdivisions) {
    Mesh mesh;
    
    // Create icosahedron as base
    const double t = (1.0 + std::sqrt(5.0)) / 2.0;
    
    // 12 vertices
    std::vector<Vec3> base_vertices = {
        Vec3(-1, t, 0), Vec3(1, t, 0), Vec3(-1, -t, 0), Vec3(1, -t, 0),
        Vec3(0, -1, t), Vec3(0, 1, t), Vec3(0, -1, -t), Vec3(0, 1, -t),
        Vec3(t, 0, -1), Vec3(t, 0, 1), Vec3(-t, 0, -1), Vec3(-t, 0, 1)
    };
    
    // Normalize to unit sphere
    for (auto& v : base_vertices) {
        v = v.normalized();
        mesh.vertices.push_back(v);
    }
    
    // 20 triangle faces
    std::vector<std::array<size_t, 3>> base_faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };
    
    for (const auto& f : base_faces) {
        mesh.triangles.emplace_back(f[0], f[1], f[2]);
    }
    
    // Subdivide
    for (int s = 0; s < subdivisions; ++s) {
        std::vector<Triangle> new_triangles;
        std::map<std::pair<size_t, size_t>, size_t> edge_midpoints;
        
        auto get_midpoint = [&](size_t i0, size_t i1) -> size_t {
            auto key = std::make_pair(std::min(i0, i1), std::max(i0, i1));
            auto it = edge_midpoints.find(key);
            if (it != edge_midpoints.end()) {
                return it->second;
            }
            Vec3 mid = (mesh.vertices[i0] + mesh.vertices[i1]).normalized();
            size_t idx = mesh.vertices.size();
            mesh.vertices.push_back(mid);
            edge_midpoints[key] = idx;
            return idx;
        };
        
        for (const auto& tri : mesh.triangles) {
            size_t a = get_midpoint(tri.v0, tri.v1);
            size_t b = get_midpoint(tri.v1, tri.v2);
            size_t c = get_midpoint(tri.v2, tri.v0);
            
            new_triangles.emplace_back(tri.v0, a, c);
            new_triangles.emplace_back(tri.v1, b, a);
            new_triangles.emplace_back(tri.v2, c, b);
            new_triangles.emplace_back(a, b, c);
        }
        
        mesh.triangles = std::move(new_triangles);
    }
    
    mesh.compute_normals();
    return mesh;
}

Mesh create_cube() {
    Mesh mesh;
    
    // 8 vertices
    mesh.vertices = {
        Vec3(-1, -1, -1), Vec3(1, -1, -1), Vec3(1, 1, -1), Vec3(-1, 1, -1),
        Vec3(-1, -1, 1), Vec3(1, -1, 1), Vec3(1, 1, 1), Vec3(-1, 1, 1)
    };
    
    // 12 triangles (6 faces, 2 triangles per face)
    // Bottom face (z=-1)
    mesh.triangles.emplace_back(0, 2, 1);
    mesh.triangles.emplace_back(0, 3, 2);
    // Top face (z=1)
    mesh.triangles.emplace_back(4, 5, 6);
    mesh.triangles.emplace_back(4, 6, 7);
    // Front face (y=-1)
    mesh.triangles.emplace_back(0, 1, 5);
    mesh.triangles.emplace_back(0, 5, 4);
    // Back face (y=1)
    mesh.triangles.emplace_back(2, 3, 7);
    mesh.triangles.emplace_back(2, 7, 6);
    // Left face (x=-1)
    mesh.triangles.emplace_back(0, 4, 7);
    mesh.triangles.emplace_back(0, 7, 3);
    // Right face (x=1)
    mesh.triangles.emplace_back(1, 2, 6);
    mesh.triangles.emplace_back(1, 6, 5);
    
    mesh.compute_normals();
    return mesh;
}

}  // namespace mesh2sdf
