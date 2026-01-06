/**
 * @file distance_transform.hpp
 * @brief 2D distance transform (Manhattan distance variant)
 * 
 * Implements scanline-based distance transform algorithm using a variant of
 * Manhattan distance (L1 Norm) with integer-step distance propagation on grid.
 */

#ifndef MESH2SDF_DISTANCE_TRANSFORM_HPP
#define MESH2SDF_DISTANCE_TRANSFORM_HPP

#include "plane_cutter.hpp"
#include <vector>

namespace mesh2sdf {

/**
 * @brief 2D distance transform class
 * 
 * Uses two-pass scan algorithm to compute Manhattan distance variant:
 * - First pass: scan from top-left to bottom-right
 * - Second pass: scan from bottom-right to top-left
 */
class DistanceTransform2D {
public:
    /**
     * @brief Constructor
     * @param width Grid width
     * @param height Grid height
     */
    DistanceTransform2D(int width, int height);
    
    /**
     * @brief Compute signed distance field
     * @param contours List of contour line segments
     * @param origin_x Grid origin X coordinate
     * @param origin_y Grid origin Y coordinate
     * @param cell_size_x Cell size in X direction
     * @param cell_size_y Cell size in Y direction
     * @param output Output buffer (must be pre-allocated)
     */
    void compute_signed(const std::vector<ContourSegment>& contours,
                        double origin_x, double origin_y,
                        double cell_size_x, double cell_size_y,
                        std::vector<float>& output);
    
    int width() const { return width_; }
    int height() const { return height_; }
    
private:
    int width_;   ///< Grid width
    int height_;  ///< Grid height
};

}  // namespace mesh2sdf

#endif  // MESH2SDF_DISTANCE_TRANSFORM_HPP
