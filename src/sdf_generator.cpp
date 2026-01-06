/**
 * @file sdf_generator.cpp
 * @brief SDF generator implementation
 * 
 * Generate complete 3D signed distance field by layer-by-layer mesh cutting and 2D distance transform。
 */

#include "mesh2sdf/sdf_generator.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <iostream>

#ifdef _OPENMP
#include <omp.h>
#endif

namespace mesh2sdf {

SDFGenerator::SDFGenerator(const SDFConfig& config) : config_(config) {}

SDFData SDFGenerator::generate(const Mesh& mesh, std::function<void(int, int)> progress) {
    // Copy mesh for modification
    Mesh work_mesh = mesh;
    
    // Normalize mesh (if configuration requires)
    if (config_.normalize_mesh) {
        work_mesh.normalize();
    }
    
    // Compute bounding box
    BoundingBox bbox = work_mesh.compute_bounding_box();
    
    // Add padding
    Vec3 padding_vec = bbox.size() * config_.padding;
    bbox.min_corner = bbox.min_corner - padding_vec;
    bbox.max_corner = bbox.max_corner + padding_vec;
    
    // Compute cell size (anisotropic)
    Vec3 box_size = bbox.size();
    double cell_size_x = box_size.x / config_.resolution_x;
    double cell_size_y = box_size.y / config_.resolution_y;
    double cell_size_z = box_size.z / config_.resolution_z;
    
    // Initialize SDF data
    SDFData sdf;
    // Node sampling: need (resolution+1) nodes to define resolution voxels
    sdf.size_x = config_.resolution_x + 1;
    sdf.size_y = config_.resolution_y + 1;
    sdf.size_z = config_.resolution_z + 1;
    sdf.origin_x = bbox.min_corner.x;
    sdf.origin_y = bbox.min_corner.y;
    sdf.origin_z = bbox.min_corner.z;
    sdf.cell_size_x = cell_size_x;
    sdf.cell_size_y = cell_size_y;
    sdf.cell_size_z = cell_size_z;

    const float FAR_VALUE = 9.99e10f; // Standard convention: exterior is positive infinity
    sdf.values.resize(sdf.size_x * sdf.size_y * sdf.size_z, FAR_VALUE);
    
    PlaneCutter cutter;
    cutter.set_mesh(work_mesh);

    // Helper function: merge logic
    auto merge_sdf = [](float& current, float new_val, float far) {
        if (std::abs(new_val) >= far) return; // New value invalid, ignore
        // If current is invalid, overwrite directly
        if (std::abs(current) >= far) {
            current = new_val;
            return;
        }
        // Otherwise take minimum absolute value
        if (std::abs(new_val) < std::abs(current)) {
            current = new_val;
        }
    };

    // --- Z Axis Pass ---
    if (config_.slice_z) {
        cutter.compute_scalars(Vec3(0,0,0), Vec3(0,0,1));
        
        #pragma omp parallel for schedule(dynamic)
        for (int z = 0; z <= config_.resolution_z; ++z) {
            // Report progress
            // Note: z ranges from 0 to resolution_z (inclusive) for node sampling
            if (progress && omp_get_thread_num() == 0) progress(z, sdf.size_z);

            double z_level = sdf.origin_z + z * cell_size_z; // Node sampling
            auto segments_3d = cutter.intersect(z_level);
            
            if (config_.export_contours) {
                #pragma omp critical
                for (const auto& seg : segments_3d) {
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.x));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.y));
                    sdf.debug_contours.push_back(static_cast<float>(z_level));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.x));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.y));
                    sdf.debug_contours.push_back(static_cast<float>(z_level));
                }
            }
            
            if (segments_3d.empty()) continue;

            std::vector<ContourSegment> contours(segments_3d.size());
            for (size_t i = 0; i < segments_3d.size(); ++i) {
                contours[i] = ContourSegment(Vec2(segments_3d[i].p0.x, segments_3d[i].p0.y),
                                             Vec2(segments_3d[i].p1.x, segments_3d[i].p1.y));
            }
            
            // Auto-close open contours
            PlaneCutter::close_contours(contours);

            DistanceTransform2D dt(sdf.size_x, sdf.size_y);
            std::vector<float> slice_values(sdf.size_x * sdf.size_y);
            dt.compute_signed(contours, sdf.origin_x, sdf.origin_y, cell_size_x, cell_size_y, slice_values);

            // Merge Z-slice (Block memory)
            size_t offset = (size_t)z * sdf.size_x * sdf.size_y;
            for (size_t i = 0; i < slice_values.size(); ++i) {
                merge_sdf(sdf.values[offset + i], slice_values[i], FAR_VALUE - 1.0f);
            }
        }
    }

    // --- Y Axis Pass ---
    if (config_.slice_y) {
        cutter.compute_scalars(Vec3(0,0,0), Vec3(0,1,0)); // Normal Y

        #pragma omp parallel for schedule(dynamic)
        for (int y = 0; y <= config_.resolution_y; ++y) {
            double y_level = sdf.origin_y + y * cell_size_y; // Node sampling
            auto segments_3d = cutter.intersect(y_level);
            
             if (config_.export_contours) {
                #pragma omp critical
                for (const auto& seg : segments_3d) {
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.x));
                    sdf.debug_contours.push_back(static_cast<float>(y_level));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.z));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.x));
                    sdf.debug_contours.push_back(static_cast<float>(y_level));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.z));
                }
            }

            if (segments_3d.empty()) continue;

            // Project to XZ plane: u=x, v=z
            std::vector<ContourSegment> contours(segments_3d.size());
            for (size_t i = 0; i < segments_3d.size(); ++i) {
                contours[i] = ContourSegment(Vec2(segments_3d[i].p0.x, segments_3d[i].p0.z),
                                             Vec2(segments_3d[i].p1.x, segments_3d[i].p1.z));
            }
            
            // Auto-close open contours
            PlaneCutter::close_contours(contours);

            // DT dimensions: X * Z
            DistanceTransform2D dt(sdf.size_x, sdf.size_z);
            std::vector<float> slice_values(sdf.size_x * sdf.size_z);
            dt.compute_signed(contours, sdf.origin_x, sdf.origin_z, cell_size_x, cell_size_z, slice_values);

            // Merge Y-slice (Strided memory)
            // slice index k = z * size_x + x
            // grid index  = z * (sx*sy) + y * sx + x
            for (int z = 0; z < sdf.size_z; ++z) {
                for (int x = 0; x < sdf.size_x; ++x) {
                    float val = slice_values[z * sdf.size_x + x];
                    size_t grid_idx = (size_t)z * sdf.size_x * sdf.size_y + (size_t)y * sdf.size_x + x;
                    merge_sdf(sdf.values[grid_idx], val, FAR_VALUE - 1.0f);
                }
            }
        }
    }

    // --- X Axis Pass ---
    if (config_.slice_x) {
        cutter.compute_scalars(Vec3(0,0,0), Vec3(1,0,0)); // Normal X

        #pragma omp parallel for schedule(dynamic)
        for (int x = 0; x <= config_.resolution_x; ++x) {
            double x_level = sdf.origin_x + x * cell_size_x; // Node sampling
            auto segments_3d = cutter.intersect(x_level);

            if (config_.export_contours) {
                #pragma omp critical
                for (const auto& seg : segments_3d) {
                    sdf.debug_contours.push_back(static_cast<float>(x_level));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.y));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p0.z));
                    sdf.debug_contours.push_back(static_cast<float>(x_level));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.y));
                    sdf.debug_contours.push_back(static_cast<float>(seg.p1.z));
                }
            }
            
            if (segments_3d.empty()) continue;

            // Project to YZ plane: u=y, v=z
            std::vector<ContourSegment> contours(segments_3d.size());
            for (size_t i = 0; i < segments_3d.size(); ++i) {
                contours[i] = ContourSegment(Vec2(segments_3d[i].p0.y, segments_3d[i].p0.z),
                                             Vec2(segments_3d[i].p1.y, segments_3d[i].p1.z));
            }
            
            // Auto-close open contours
            PlaneCutter::close_contours(contours);

            // DT dimensions: Y * Z
            DistanceTransform2D dt(sdf.size_y, sdf.size_z);
            std::vector<float> slice_values(sdf.size_y * sdf.size_z);
            dt.compute_signed(contours, sdf.origin_y, sdf.origin_z, cell_size_y, cell_size_z, slice_values);

            // Merge X-slice (Strided memory)
            // slice index k = z * size_y + y
            // grid index  = z * (sx*sy) + y * sx + x
            for (int z = 0; z < sdf.size_z; ++z) {
                for (int y = 0; y < sdf.size_y; ++y) {
                    float val = slice_values[z * sdf.size_y + y];
                    size_t grid_idx = (size_t)z * sdf.size_x * sdf.size_y + (size_t)y * sdf.size_x + x;
                    merge_sdf(sdf.values[grid_idx], val, FAR_VALUE - 1.0f);
                }
            }
        }
    }
    
    // Final progress
    if (progress) {
        progress(sdf.size_z, sdf.size_z);
    }
    
    return sdf;
}


void save_sdf_csv(const SDFData& sdf, const std::string& filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot create file: " + filepath);
    }
    
    file << "# Mesh2SDF-Fast SDF Data\n";
    file << "# size_x,size_y,size_z,origin_x,origin_y,origin_z,cell_size_x,cell_size_y,cell_size_z\n";
    file << sdf.size_x << "," << sdf.size_y << "," << sdf.size_z << ","
         << sdf.origin_x << "," << sdf.origin_y << "," << sdf.origin_z << ","
         << sdf.cell_size_x << "," << sdf.cell_size_y << "," << sdf.cell_size_z << "\n";
    
    file << "x,y,z,sdf\n";
    
    for (int z = 0; z < sdf.size_z; ++z) {
        for (int y = 0; y < sdf.size_y; ++y) {
            for (int x = 0; x < sdf.size_x; ++x) {
                float val = sdf.at(x, y, z);
                file << x << "," << y << "," << z << "," << val << "\n";
            }
        }
    }
}

SDFData load_sdf_csv(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filepath);
    }
    
    SDFData sdf;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        break;
    }
    
    std::istringstream meta_ss(line);
    char comma;
    // Try reading new format (separate cell_size)
    // For compatibility, may need more robust parsing here。
    // Assume file is newly generated by us。
    // If old file, reading may fail or be garbled。
    // Simple implementation here for reading new format。
    meta_ss >> sdf.size_x >> comma >> sdf.size_y >> comma >> sdf.size_z >> comma
            >> sdf.origin_x >> comma >> sdf.origin_y >> comma >> sdf.origin_z >> comma
            >> sdf.cell_size_x >> comma >> sdf.cell_size_y >> comma >> sdf.cell_size_z;

    // Check if read successful (e.g., if old file has only one cell_size）
    if (meta_ss.fail()) {
        // Reset stream and try old format
        meta_ss.clear();
        meta_ss.seekg(0);
        double cell_size;
        meta_ss >> sdf.size_x >> comma >> sdf.size_y >> comma >> sdf.size_z >> comma
                >> sdf.origin_x >> comma >> sdf.origin_y >> comma >> sdf.origin_z >> comma
                >> cell_size;
        sdf.cell_size_x = sdf.cell_size_y = sdf.cell_size_z = cell_size;
    }
    
    std::getline(file, line);
    
    sdf.values.resize(sdf.size_x * sdf.size_y * sdf.size_z);
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream ss(line);
        int x, y, z;
        float val;
        ss >> x >> comma >> y >> comma >> z >> comma >> val;
        
        if (x >= 0 && x < sdf.size_x &&
            y >= 0 && y < sdf.size_y &&
            z >= 0 && z < sdf.size_z) {
            sdf.set(x, y, z, val);
        }
    }
    
    return sdf;
}

}  // namespace mesh2sdf
