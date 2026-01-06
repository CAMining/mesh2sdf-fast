/**
 * @file plane_cutter.cpp
 * @brief Plane cutter implementation
 * 
 * Implementation of algorithm for cutting triangle mesh with planes。
 */

#include "mesh2sdf/plane_cutter.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map> // For spatial hash
#include <functional>    // For std::hash
#include <utility>       // For std::pair

namespace mesh2sdf {

void PlaneCutter::set_mesh(const Mesh& mesh) {
    mesh_ = &mesh;
    scalars_.resize(mesh.vertices.size());
}

void PlaneCutter::compute_scalars(const Vec3& p0, const Vec3& normal) {
    if (!mesh_) return;
    
    Vec3 n = normal.normalized();
    const auto& vertices = mesh_->vertices;
    
    #pragma omp parallel for
    for (size_t i = 0; i < vertices.size(); ++i) {
        scalars_[i] = (vertices[i] - p0).dot(n);
    }
}

std::vector<ContourSegment3D> PlaneCutter::intersect(double iso_value) const {
    if (!mesh_) return {};

    std::vector<ContourSegment3D> segments;
    // Simple estimated allocation, assuming average small portion of each triangle is cut
    // For sparse mesh may be much smaller, for dense slicing may be close
    // No excessive reservation here to save memory
    
    const auto& triangles = mesh_->triangles;
    const auto& vertices = mesh_->vertices;

    // Thread-local storage, avoid frequent lock contention
    #pragma omp parallel
    {
        std::vector<ContourSegment3D> local_segments;
        
        #pragma omp for nowait
        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            
            // Get scalar values
            double s0 = scalars_[tri.v0];
            double s1 = scalars_[tri.v1];
            double s2 = scalars_[tri.v2];

            // Quick rejection: all on same side
            bool all_less = (s0 < iso_value && s1 < iso_value && s2 < iso_value);
            bool all_greater = (s0 > iso_value && s1 > iso_value && s2 > iso_value);
            
            if (all_less || all_greater) {
                continue;
            }

            // Find intersections
            Vec3 v[3] = { vertices[tri.v0], vertices[tri.v1], vertices[tri.v2] };
            double s[3] = { s0, s1, s2 };
            
            Vec3 intersections[2];
            int count = 0;

            for (int k = 0; k < 3; ++k) {
                int next = (k + 1) % 3;
                double val_k = s[k];
                double val_next = s[next];

                // Check if edge crosses isosurface (Left-closed right-open interval, avoid duplicate calculation when vertices coincide)
                // Note: carefully handle vertices exactly on plane
                // User example uses: (s1 <= isoValue && s2 > isoValue) || (s1 > isoValue && s2 <= isoValue)
                // This is a robust method, ensuring only one side contributes intersection
                
                bool case1 = (val_k <= iso_value && val_next > iso_value);
                bool case2 = (val_k > iso_value && val_next <= iso_value);
                
                if (case1 || case2) {
                    double t = (iso_value - val_k) / (val_next - val_k);
                    if (count < 2) {
                        intersections[count++] = v[k] + (v[next] - v[k]) * t;
                    }
                }
            }

            if (count == 2) {
                local_segments.emplace_back(intersections[0], intersections[1]);
            }
        }
        
        #pragma omp critical
        {
            segments.insert(segments.end(), local_segments.begin(), local_segments.end());
        }
    }

    return segments;
}

void PlaneCutter::close_contours(std::vector<ContourSegment>& contours) {
    if (contours.empty()) return;

    // Epsilon for point matching
    // Using global constant EPSILON from common.hpp
    const double EPSILON_SQ = EPSILON * EPSILON;
    // Quantization scale for spatial hashing (1/E)
    // Use slightly larger cell size to ensure matches
    const double SCALE = 1.0 / (EPSILON * 2.0);

    // 1. Build Spatial Index & Graph
    struct PointHash {
        std::size_t operator()(const std::pair<long, long>& k) const {
            return std::hash<long>()(k.first) ^ (std::hash<long>()(k.second) << 1);
        }
    };
    
    // Map grid key -> list of segment indices that have an endpoint in this cell
    // We store segment index + which end (0 or 1)
    struct EndpointRef {
        size_t seg_idx;
        int end_idx; // 0 for p0, 1 for p1
    };

    std::unordered_map<std::pair<long, long>, std::vector<EndpointRef>, PointHash> grid;
    
    auto get_key = [&](const Vec2& p) {
        return std::make_pair(
            static_cast<long>(std::floor(p.x * SCALE)),
            static_cast<long>(std::floor(p.y * SCALE))
        );
    };

    // Populate grid
    for (size_t i = 0; i < contours.size(); ++i) {
        Vec2 p[2] = {contours[i].p0, contours[i].p1};
        for (int j = 0; j < 2; ++j) {
            auto key = get_key(p[j]);
            // Insert into 3x3 neighborhood to handle boundary cases?
            // Simple grid usually sufficient if points are EXACTLY coincident.
            // For "close enough", checking 9 cells is safer but slower.
            // Given marching cubes/plane cut produces exact shared vertices usually:
            grid[key].push_back({i, j});
            
            // To be robust against float errors putting neighbors in adjacent cells,
            // we search neighbors during lookup, but here we just insert to primary cell.
        }
    }

    std::vector<bool> visited(contours.size(), false);
    std::vector<ContourSegment> closed_contours;
    closed_contours.reserve(contours.size() * 1.1); // Expect slight growth

    // 2. Assemble Polylines
    for (size_t i = 0; i < contours.size(); ++i) {
        if (visited[i]) continue;
        
        // Start a new polyline chain
        // We treat segments as edges. We grow from both ends of segment i.
        std::vector<size_t> chain; 
        chain.push_back(i);
        visited[i] = true;

        // Current open ends of the chain
        Vec2 chain_head = contours[i].p0;
        Vec2 chain_tail = contours[i].p1;
        
        // Helper to find neighbor
        auto find_neighbor = [&](Vec2 p, size_t exclude_seg_idx) -> std::pair<size_t, int> {
             long cx = static_cast<long>(std::floor(p.x * SCALE));
             long cy = static_cast<long>(std::floor(p.y * SCALE));
             
             // Search 3x3 neighborhood
             for (long dx = -1; dx <= 1; ++dx) {
                 for (long dy = -1; dy <= 1; ++dy) {
                     auto key = std::make_pair(cx + dx, cy + dy);
                     auto it = grid.find(key);
                     if (it != grid.end()) {
                         for (const auto& ref : it->second) {
                             if (ref.seg_idx == exclude_seg_idx) continue;
                             if (visited[ref.seg_idx]) continue;
                             
                             // Exact distance check
                             Vec2 target = (ref.end_idx == 0) ? contours[ref.seg_idx].p0 : contours[ref.seg_idx].p1;
                             double d2 = (p.x - target.x)*(p.x - target.x) + (p.y - target.y)*(p.y - target.y);
                             if (d2 < EPSILON_SQ) {
                                 return {ref.seg_idx, ref.end_idx};
                             }
                         }
                     }
                 }
             }
             return {static_cast<size_t>(-1), -1}; // Not found
        };

        // Grow Tail (forward from p1)
        while (true) {
            auto next = find_neighbor(chain_tail, chain.back());
            if (next.first != static_cast<size_t>(-1)) {
                size_t idx = next.first;
                int connect_at = next.second; // 0 or 1
                visited[idx] = true;
                
                // If we connected at p0, the new tail is p1. If connected at p1, new tail is p0.
                if (connect_at == 0) {
                    chain_tail = contours[idx].p1;
                } else {
                    chain_tail = contours[idx].p0;
                    // Properly orienting segment isn't strictly necessary for just creating a closed LOOP of segments,
                    // but for "polyline" ordered points it is. 
                    // To keep implementation simple and since we just output separate segments anyway,
                    // we just track the current exposed world coordinate.
                }
                chain.push_back(idx);
            } else {
                break;
            }
        }

        // Grow Head (backward from p0)
        // Note: chain[0] is the starting segment. original head was contours[i].p0
        // We need to find something that connects to `chain_head`.
        while (true) {
            auto prev = find_neighbor(chain_head, chain.front()); // Actually exclude "current" which is chain.front()
             if (prev.first != static_cast<size_t>(-1)) {
                 size_t idx = prev.first;
                 int connect_at = prev.second;
                 visited[idx] = true;
                 
                 // Update head
                 if (connect_at == 0) {
                     chain_head = contours[idx].p1;
                 } else {
                     chain_head = contours[idx].p0;
                 }
                 // Prepend (expensive for vector, but chains are short)
                 chain.insert(chain.begin(), idx);
             } else {
                 break;
             }
        }
        
        // 3. Process the assembled polyline
        // Add original segments
        for (size_t seg_idx : chain) {
            closed_contours.push_back(contours[seg_idx]);
        }
        
        // Check closure
        double d2 = (chain_head.x - chain_tail.x)*(chain_head.x - chain_tail.x) + 
                    (chain_head.y - chain_tail.y)*(chain_head.y - chain_tail.y);
                    
        if (d2 > EPSILON_SQ) {
            // It is open, close it!
            closed_contours.emplace_back(chain_tail, chain_head);
        }
    }
    
    // Replace original
    contours = std::move(closed_contours);
}

}  // namespace mesh2sdf
