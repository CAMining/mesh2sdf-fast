/**
 * @file distance_transform.cpp
 * @brief 2D distance transform implementation (scanline-based algorithm)
 * 
 */

#include "mesh2sdf/distance_transform.hpp"
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <limits>

namespace mesh2sdf {

DistanceTransform2D::DistanceTransform2D(int width, int height)
    : width_(width), height_(height) {}

// Compute voxel distance
// Uses voxel node sampling, not voxel center sampling
void DistanceTransform2D::compute_signed(
    const std::vector<ContourSegment>& contours,
    double origin_x, double origin_y,
    double cell_size_x, double cell_size_y,
    std::vector<float>& grid) {
    
    // Ensure output buffer size is correct
    if (grid.size() != static_cast<size_t>(width_ * height_)) {
        grid.resize(width_ * height_);
    }
    
    // 1. Initialize mesh
    std::fill(grid.begin(), grid.end(), INF_DIST);

    // Internal structure definition
    struct LineSegmentLocal {
        struct Point { double x, y; } p1, p2;
    };

    // 2. Preprocessing: convert contour lines to“Index Space”,
    // so we compute voxel distance later，
    // Voxel size determined by resolution spacing，
    // When calling marching_cubes, pass spacing parameter=spacing，to restore physical coordinates
    std::vector<LineSegmentLocal> idxContours;
    idxContours.reserve(contours.size());
    // cell_size i.e., resolution spacing
    double res_x = cell_size_x;
    double res_y = cell_size_y;

    for (const auto& seg : contours) {
        LineSegmentLocal l;
        l.p1.x = (seg.p0.x - origin_x) / res_x;
        l.p1.y = (seg.p0.y - origin_y) / res_y;
        l.p2.x = (seg.p1.x - origin_x) / res_x;
        l.p2.y = (seg.p1.y - origin_y) / res_y;
        idxContours.push_back(l);
    }

    // Helper Lambda: ray casting logic
    auto CastLines = [&](int type) {
        int outerLimit = (type == 0) ? width_ : height_;
        int innerLimit = (type == 0) ? height_ : width_;
        
        for (int i = 0; i < outerLimit; ++i) {
            double scanPos = static_cast<double>(i); // Node sampling
            std::vector<double> intersections;

            for (const auto& line : idxContours) {
                double u1 = (type == 0) ? line.p1.x : line.p1.y;
                double v1 = (type == 0) ? line.p1.y : line.p1.x;
                double u2 = (type == 0) ? line.p2.x : line.p2.y;
                double v2 = (type == 0) ? line.p2.y : line.p2.x;

                // Use half-open interval [min, max) to handle vertex on scanline issues
                // Simulation of Simplicity
                if ((u1 <= scanPos && u2 > scanPos) || (u2 <= scanPos && u1 > scanPos)) {
                    double t = (scanPos - u1) / (u2 - u1);
                    double intersectVal = v1 + t * (v2 - v1);
                    intersections.push_back(intersectVal);
                }
            }
            
            if (intersections.empty()) continue;

            std::sort(intersections.begin(), intersections.end());
            
            // Deduplicate intersections (Fix for coincident faces/double geometry)
            // If duplicate intersections exist (A, A, B, B), parity toggles twice resulting in no sign change.
            // Deduplication is critical!
            auto last = std::unique(intersections.begin(), intersections.end(), 
                                  [](double a, double b) { return std::abs(a - b) < EPSILON; });
            intersections.erase(last, intersections.end());

            int currentIntersectionIdx = 0;
            // Standard convention: initial exterior (+1.0)
            double sign = 1.0; 

            for (int j = 0; j < innerLimit; ++j) {
                double pixelPos = static_cast<double>(j); // Node sampling

                while (currentIntersectionIdx < (int)intersections.size() && 
                       intersections[currentIntersectionIdx] < pixelPos) {
                    currentIntersectionIdx++;
                    sign *= -1.0;
                }

                double dist = 0.0;
                if (currentIntersectionIdx == 0) {
                    dist = pixelPos - intersections[0]; 
                } else if (currentIntersectionIdx == (int)intersections.size()) {
                    dist = intersections.back() - pixelPos;
                } else {
                    double d1 = pixelPos - intersections[currentIntersectionIdx - 1];
                    double d2 = intersections[currentIntersectionIdx] - pixelPos;
                    dist = (d1 < d2) ? d1 : d2;
                }
                
                // Standard convention: exterior positive, interior negative
                float val = static_cast<float>(sign * std::abs(dist));

                int idx = (type == 0) ? (j * width_ + i) : (i * width_ + j);

                if (type == 0) {
                    grid[idx] = val;
                } else {
                    float current = grid[idx];
                    if (current == INF_DIST || std::abs(val) < std::abs(current)) {
                        grid[idx] = val;
                    }
                }
            }
        }
    };

    // Execute ray casting in two directions
    CastLines(0);
    CastLines(1);

    // 3. PushDistances: Distance propagation (Manhattan distance based)
    
    // 3.1 Propagate along X direction (within-row propagation)
    for (int y = 0; y < height_; ++y) {
        float* rowPtr = grid.data() + y * width_;

        // Forward: left -> right
        for (int x = 1; x < width_; ++x) {
            float curr = rowPtr[x];
            float prev = rowPtr[x - 1];
            
            // Exterior (+): if through left neighbor(+1)can get shorter path
            if (curr > 0 && prev + 1 < curr) {
                rowPtr[x] = prev + 1;
            }
            // Interior (-): if through left neighbor(-1)can get closer to boundary
            else if (curr < 0 && prev - 1 > curr) {
                rowPtr[x] = prev - 1;
            }
        }

        // Backward: right -> left
        for (int x = width_ - 2; x >= 0; --x) {
            float curr = rowPtr[x];
            float next = rowPtr[x + 1];

            if (curr > 0 && next + 1 < curr) {
                rowPtr[x] = next + 1;
            }
            else if (curr < 0 && next - 1 > curr) {
                rowPtr[x] = next - 1;
            }
        }
    }

    // 3.2 Propagate along Y direction (between-column propagation)
    int stride = width_;
    for (int x = 0; x < width_; ++x) {
        float* colPtr = grid.data() + x;

        // Forward: up -> down
        for (int y = 1; y < height_; ++y) {
            float* currPtr = colPtr + y * stride;
            float* prevPtr = colPtr + (y - 1) * stride;
            
            if (*currPtr > 0 && *prevPtr + 1 < *currPtr) {
                *currPtr = *prevPtr + 1;
            }
            else if (*currPtr < 0 && *prevPtr - 1 > *currPtr) {
                *currPtr = *prevPtr - 1;
            }
        }

        // Backward: down -> up
        for (int y = height_ - 2; y >= 0; --y) {
            float* currPtr = colPtr + y * stride;
            float* nextPtr = colPtr + (y + 1) * stride;

            if (*currPtr > 0 && *nextPtr + 1 < *currPtr) {
                *currPtr = *nextPtr + 1;
            }
            else if (*currPtr < 0 && *nextPtr - 1 > *currPtr) {
                *currPtr = *nextPtr - 1;
            }
        }
    }
}



}  // namespace mesh2sdf
