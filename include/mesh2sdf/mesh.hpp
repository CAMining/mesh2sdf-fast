/**
 * @file mesh.hpp
 * @brief Mesh data structure definitions
 * 
 * Defines vertex, triangle, and mesh container classes for storing and manipulating 3D mesh models.
 */

#ifndef MESH2SDF_MESH_HPP
#define MESH2SDF_MESH_HPP

#include <vector>
#include <array>
#include <string>
#include <stdexcept>
#include <cmath>
#include <limits>
#include <algorithm>

namespace mesh2sdf {

/**
 * @brief 3D vector/point structure
 */
struct Vec3 {
    double x, y, z;
    
    Vec3() : x(0), y(0), z(0) {}
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x + v.x, y + v.y, z + v.z); }
    Vec3 operator-(const Vec3& v) const { return Vec3(x - v.x, y - v.y, z - v.z); }
    Vec3 operator*(double s) const { return Vec3(x * s, y * s, z * s); }
    Vec3 operator/(double s) const { return Vec3(x / s, y / s, z / s); }
    
    double dot(const Vec3& v) const { return x * v.x + y * v.y + z * v.z; }
    Vec3 cross(const Vec3& v) const {
        return Vec3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    double norm() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 normalized() const {
        double n = norm();
        return n > 0 ? *this / n : Vec3();
    }
    
    double& operator[](int i) {
        switch(i) { case 0: return x; case 1: return y; default: return z; }
    }
    double operator[](int i) const {
        switch(i) { case 0: return x; case 1: return y; default: return z; }
    }
};

/**
 * @brief 2D vector/point structure
 */
struct Vec2 {
    double x, y;
    
    Vec2() : x(0), y(0) {}
    Vec2(double x_, double y_) : x(x_), y(y_) {}
    
    Vec2 operator+(const Vec2& v) const { return Vec2(x + v.x, y + v.y); }
    Vec2 operator-(const Vec2& v) const { return Vec2(x - v.x, y - v.y); }
    Vec2 operator*(double s) const { return Vec2(x * s, y * s); }
};

/**
 * @brief Triangle structure storing three vertex indices
 */
struct Triangle {
    size_t v0, v1, v2;  ///< Three vertex indices
    Vec3 normal;         ///< Face normal vector
    
    Triangle() : v0(0), v1(0), v2(0) {}
    Triangle(size_t i0, size_t i1, size_t i2) : v0(i0), v1(i1), v2(i2) {}
};

/**
 * @brief Axis-Aligned Bounding Box (AABB)
 */
struct BoundingBox {
    Vec3 min_corner;  ///< Minimum corner point
    Vec3 max_corner;  ///< Maximum corner point
    
    BoundingBox() {
        double inf = std::numeric_limits<double>::infinity();
        min_corner = Vec3(inf, inf, inf);
        max_corner = Vec3(-inf, -inf, -inf);
    }
    
    /// Expand bounding box to include given point
    void expand(const Vec3& p) {
        min_corner.x = std::min(min_corner.x, p.x);
        min_corner.y = std::min(min_corner.y, p.y);
        min_corner.z = std::min(min_corner.z, p.z);
        max_corner.x = std::max(max_corner.x, p.x);
        max_corner.y = std::max(max_corner.y, p.y);
        max_corner.z = std::max(max_corner.z, p.z);
    }
    
    /// Get bounding box center
    Vec3 center() const {
        return (min_corner + max_corner) * 0.5;
    }
    
    /// Get bounding box size
    Vec3 size() const {
        return max_corner - min_corner;
    }
    
    /// Get maximum extent length
    double max_extent() const {
        Vec3 s = size();
        return std::max({s.x, s.y, s.z});
    }
};

/**
 * @brief Mesh class storing vertex and triangle data
 */
class Mesh {
public:
    std::vector<Vec3> vertices;       ///< Vertex list
    std::vector<Triangle> triangles;  ///< Triangle list
    
    Mesh() = default;
    
    /**
     * @brief Compute mesh bounding box
     * @return Axis-aligned bounding box
     */
    BoundingBox compute_bounding_box() const {
        BoundingBox bbox;
        for (const auto& v : vertices) {
            bbox.expand(v);
        }
        return bbox;
    }
    
    /**
     * @brief Compute normals for all triangles
     */
    void compute_normals() {
        for (auto& tri : triangles) {
            const Vec3& p0 = vertices[tri.v0];
            const Vec3& p1 = vertices[tri.v1];
            const Vec3& p2 = vertices[tri.v2];
            Vec3 edge1 = p1 - p0;
            Vec3 edge2 = p2 - p0;
            tri.normal = edge1.cross(edge2).normalized();
        }
    }
    
    /**
     * @brief Normalize mesh to [-1, 1] range
     */
    void normalize() {
        BoundingBox bbox = compute_bounding_box();
        Vec3 center = bbox.center();
        double scale = 2.0 / bbox.max_extent();
        
        for (auto& v : vertices) {
            v = (v - center) * scale;
        }
    }
    
    /**
     * @brief Check if mesh is valid
     * @return true if valid, false otherwise
     */
    bool is_valid() const {
        if (vertices.empty() || triangles.empty()) {
            return false;
        }
        // Check if triangle indices are valid
        for (const auto& tri : triangles) {
            if (tri.v0 >= vertices.size() || 
                tri.v1 >= vertices.size() || 
                tri.v2 >= vertices.size()) {
                return false;
            }
        }
        return true;
    }
    
    /**
     * @brief Get vertex count
     */
    size_t vertex_count() const { return vertices.size(); }
    
    /**
     * @brief Get triangle count
     */
    size_t triangle_count() const { return triangles.size(); }
};

/**
 * @brief Mesh file loading exception
 */
class MeshLoadError : public std::runtime_error {
public:
    explicit MeshLoadError(const std::string& msg) : std::runtime_error(msg) {}
};

/**
 * @brief Load mesh from STL file (binary format)
 * @param filepath File path
 * @return Loaded mesh
 * @throws MeshLoadError if file not found or format error
 */
Mesh load_stl(const std::string& filepath);

/**
 * @brief Load mesh from OBJ file
 * @param filepath File path
 * @return Loaded mesh
 * @throws MeshLoadError if file not found or format error
 */
Mesh load_obj(const std::string& filepath);

/**
 * @brief Create unit sphere mesh
 * @param subdivisions Number of subdivisions
 * @return Sphere mesh
 */
Mesh create_sphere(int subdivisions = 2);

/**
 * @brief Create unit cube mesh
 * @return Cube mesh
 */
Mesh create_cube();

}  // namespace mesh2sdf

#endif  // MESH2SDF_MESH_HPP
