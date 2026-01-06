/**
 * @file plane_cutter.hpp
 * @brief Plane cutter for mesh slicing
 * 
 * Implements plane cutting of triangle meshes to generate contour line segments.
 */

#ifndef MESH2SDF_PLANE_CUTTER_HPP
#define MESH2SDF_PLANE_CUTTER_HPP

#include "mesh.hpp"
#include <vector>

namespace mesh2sdf {

/**
 * @brief Contour line segment structure (2D projection)
 */
struct ContourSegment {
    Vec2 p0;  ///< Start point (2D projection)
    Vec2 p1;  ///< End point (2D projection)
    
    ContourSegment() = default;
    ContourSegment(const Vec2& start, const Vec2& end) : p0(start), p1(end) {}
};

/**
 * @brief 3D contour line segment (cutting result)
 */
struct ContourSegment3D {
    Vec3 p0;  ///< Start point
    Vec3 p1;  ///< End point
    
    ContourSegment3D() = default;
    ContourSegment3D(const Vec3& start, const Vec3& end) : p0(start), p1(end) {}
};

/**
 * @brief Plane cutter class
 * 
 * Used to cut 3D meshes with arbitrary planes defined by point and normal,
 * generating 2D/3D contour lines.
 */
class PlaneCutter {
public:
    /**
     * @brief Set input mesh
     * @param mesh Input mesh reference
     */
    void set_mesh(const Mesh& mesh);

    /**
     * @brief Compute scalar field (signed distance from points to plane)
     * @param p0 Point on the plane
     * @param normal Plane normal vector
     */
    void compute_scalars(const Vec3& p0, const Vec3& normal);

    /**
     * @brief Get intersection lines for specified iso-value (3D)
     * @param iso_value Iso-surface value
     * @return List of 3D contour line segments
     */
    std::vector<ContourSegment3D> intersect(double iso_value) const;

    /**
     * @brief Force close open contour paths
     * @param contours Contour segments to process
     */
    static void close_contours(std::vector<ContourSegment>& contours);
    
private:
    const Mesh* mesh_ = nullptr;     ///< Current mesh being processed
    std::vector<double> scalars_;    ///< Pre-computed scalar field

    // Unused legacy methods removed
};

}  // namespace mesh2sdf

#endif  // MESH2SDF_PLANE_CUTTER_HPP
