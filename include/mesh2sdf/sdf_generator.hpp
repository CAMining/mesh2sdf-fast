/**
 * @file sdf_generator.hpp
 * @brief 3D Signed Distance Field generator
 * 
 * Main SDF generator class that creates complete 3D signed distance fields
 * by slicing meshes layer-by-layer and computing 2D distance transforms,
 * then merging results from multiple axes.
 */

#ifndef MESH2SDF_SDF_GENERATOR_HPP
#define MESH2SDF_SDF_GENERATOR_HPP

#include "mesh.hpp"
#include "plane_cutter.hpp"
#include "distance_transform.hpp"
#include <vector>
#include <functional>

namespace mesh2sdf {

/**
 * @brief SDF generation configuration
 */
struct SDFConfig {
    int resolution_x = 64;      ///< X-axis resolution
    int resolution_y = 64;      ///< Y-axis resolution
    int resolution_z = 64;      ///< Z-axis resolution
    double padding = 0.1;       ///< Boundary padding (relative to bbox size)
    bool normalize_mesh = true; ///< Whether to normalize mesh first
    bool export_contours = false; ///< Export contour lines for debugging (default: off)
    
    // Multi-axis slicing configuration
    bool slice_x = false;       ///< Whether to slice along X-axis
    bool slice_y = false;       ///< Whether to slice along Y-axis
    bool slice_z = true;        ///< Whether to slice along Z-axis (default: on, for backward compatibility)
    
    /// Set uniform resolution for all axes
    void set_resolution(int res) {
        resolution_x = resolution_y = resolution_z = res;
    }
};

/**
 * @brief SDF data structure
 */
struct SDFData {
    std::vector<float> values;   ///< SDF values (signed distances)
    int size_x, size_y, size_z; ///< Dimensions
    double origin_x, origin_y, origin_z;  ///< Origin coordinates
    double cell_size_x, cell_size_y, cell_size_z; ///< Cell sizes per dimension
    
    // Debug data: Flattened contour lines [x0, y0, z0, x1, y1, z1, ...]
    std::vector<float> debug_contours;
    
    /// Get SDF value at specified position
    float at(int x, int y, int z) const {
        if (x < 0 || x >= size_x || y < 0 || y >= size_y || z < 0 || z >= size_z) {
            return 1000000000;  // Return large value for out-of-bounds
        }
        return values[z * size_x * size_y + y * size_x + x];
    }
    
    /// Set SDF value at specified position
    void set(int x, int y, int z, float value) {
        if (x >= 0 && x < size_x && y >= 0 && y < size_y && z >= 0 && z < size_z) {
            values[z * size_x * size_y + y * size_x + x] = value;
        }
    }
    
    /// Convert grid coordinates to world coordinates
    Vec3 grid_to_world(int x, int y, int z) const {
        return Vec3(
            origin_x + x * cell_size_x,
            origin_y + y * cell_size_y,
            origin_z + z * cell_size_z
        );
    }
    
    /// Convert distance to real-world distance
    /// Note: With anisotropic grids, this returns an approximation. Kept for compatibility.
    double to_real_distance(int grid_dist) const {
        // Use average cell size as approximation
        return grid_dist * (cell_size_x + cell_size_y + cell_size_z) / 3.0;
    }
};

/**
 * @brief SDF Generator class
 */
class SDFGenerator {
public:
    /**
     * @brief Constructor
     * @param config SDF generation configuration
     */
    explicit SDFGenerator(const SDFConfig& config);
    
    /**
     * @brief Generate SDF from mesh
     * @param mesh Input mesh
     * @param progress Optional progress callback function(current, total)
     * @return Generated SDF data
     */
    SDFData generate(const Mesh& mesh,
                     std::function<void(int, int)> progress = nullptr);
    
private:
    SDFConfig config_;
};

/**
 * @brief Save SDF to CSV file (for debugging/inspection)
 * @param sdf SDF data
 * @param filepath Output file path
 */
void save_sdf_csv(const SDFData& sdf, const std::string& filepath);

/**
 * @brief Load SDF from CSV file
 * @param filepath Input file path
 * @return Loaded SDF data
 */
SDFData load_sdf_csv(const std::string& filepath);

}  // namespace mesh2sdf

#endif  // MESH2SDF_SDF_GENERATOR_HPP
