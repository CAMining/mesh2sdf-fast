/**
 * @file common.hpp
 * @brief Common constants and utilities
 */

#ifndef MESH2SDF_COMMON_HPP
#define MESH2SDF_COMMON_HPP

namespace mesh2sdf {

/**
 * @brief Default tolerance for floating point comparisons
 */
static constexpr double EPSILON = 1e-8;

/**
 * @brief Large value representing "far away" in distance fields
 */
static constexpr float INF_DIST = 1e10f;

} // namespace mesh2sdf

#endif // MESH2SDF_COMMON_HPP
