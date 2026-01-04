/**
 * @file plane_cutter.cpp
 * @brief Plane cutter implementation
 * 
 * Implementation of algorithm for cutting triangle mesh with planes。
 */

#include "mesh2sdf/plane_cutter.hpp"
#include <cmath>
#include <algorithm>

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

std::vector<ContourSegment> PlaneCutter::cut(const Mesh& mesh, double z_level) {
    // Compatible with old API: set state first, then cut
    // Note: for multiple calls, recommend external manual management of set_mesh and compute_scalars
    if (mesh_ != &mesh) {
        set_mesh(mesh);
        compute_scalars(Vec3(0,0,0), Vec3(0,0,1)); // Default to Z-slicing
    } else {
        // If mesh unchanged, assume scalars (Z values) already computed（or need recomputation？）
        // For safety, recompute by default, or assume cut always for Z-axis
        // Actually old cut interface always Z_LEVEL cutting, so normal is always (0,0,1)
        // As long as scalars contain Z coordinates。
        // But for safety（because p0 may change），recomputing scalars is safer unless optimizing
        // Here, z_level in cut(mesh, z_level) is iso_value，
        // compute_scalars needs p0.z=0 to make scalar = z
        // So: scalar = (v - (0,0,0)) . (0,0,1) = v.z
        // This is universal for all z_levels！
        // As long as mesh unchanged, scalars do not need recomputation。
        if (scalars_.empty()) {
             compute_scalars(Vec3(0,0,0), Vec3(0,0,1));
        }
    }
    
    // Get 3D result
    auto segments_3d = intersect(z_level);
    
    // Project to 2D (take XY plane)
    std::vector<ContourSegment> result;
    result.reserve(segments_3d.size());
    
    for (const auto& seg : segments_3d) {
        result.emplace_back(
            Vec2(seg.p0.x, seg.p0.y),
            Vec2(seg.p1.x, seg.p1.y)
        );
    }
    
    return result;
}

std::vector<ContourSegment3D> PlaneCutter::cut_3d(const Mesh& mesh, double z_level) {
    if (mesh_ != &mesh) {
        set_mesh(mesh);
        compute_scalars(Vec3(0,0,0), Vec3(0,0,1));
    } else if (scalars_.empty()) {
        compute_scalars(Vec3(0,0,0), Vec3(0,0,1));
    }
    return intersect(z_level);
}



}  // namespace mesh2sdf
