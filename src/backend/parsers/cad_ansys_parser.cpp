#include "cad_ansys_parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstdint>

namespace workbench::parsers {

static inline std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

static inline bool is_finite_vec(const Vec3& v) {
    return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

static double triangle_area(const Vec3& a, const Vec3& b, const Vec3& c) {
    // Cross product (b - a) x (c - a)
    double abx = b.x - a.x, aby = b.y - a.y, abz = b.z - a.z;
    double acx = c.x - a.x, acy = c.y - a.y, acz = c.z - a.z;
    double cx = aby * acz - abz * acy;
    double cy = abz * acx - abx * acz;
    double cz = abx * acy - aby * acx;
    return 0.5 * std::sqrt(cx * cx + cy * cy + cz * cz);
}

ParseResult StlAsciiParser::parse_string(const std::string& content, StlModel& out_model) {
    out_model = StlModel{};
    out_model.file_size_bytes = content.size();

    if (content.empty()) {
        return {false, "File is empty (0 bytes).", "Ensure the chosen STL file contains valid ASCII text data."};
    }

    if (content.size() > MAX_RECOMMENDED_SIZE_BYTES) {
        return {false, "File exceeds maximum tested limit (50 MB).", 
                "Decimate the mesh or supply an ASCII STL under 50 MB to prevent browser memory exhaustion."};
    }

    // Binary STL heuristic: check first 512 bytes for null bytes or control characters
    size_t check_len = std::min(content.size(), size_t(512));
    for (size_t i = 0; i < check_len; ++i) {
        unsigned char ch = static_cast<unsigned char>(content[i]);
        if (ch == 0 || (ch < 9 && ch != 0)) {
            return {false, "Binary STL format detected.", 
                    "Task B requires ASCII STL. Re-export your CAD model selecting ASCII format instead of Binary."};
        }
    }

    std::istringstream stream(content);
    std::string line;
    bool in_solid = false;
    bool in_facet = false;
    bool in_loop = false;
    StlFacet current_facet;
    int vertex_count = 0;
    int line_num = 0;

    while (std::getline(stream, line)) {
        line_num++;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;

        std::istringstream ls(trimmed);
        std::string token;
        ls >> token;
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);

        if (token == "solid") {
            in_solid = true;
            std::string name;
            std::getline(ls, name);
            out_model.solid_name = trim(name);
            if (out_model.solid_name.empty()) out_model.solid_name = "UnnamedSolid";
        } else if (token == "endsolid") {
            in_solid = false;
            break;
        } else if (token == "facet") {
            std::string normal_tok;
            ls >> normal_tok;
            Vec3 norm;
            if (!(ls >> norm.x >> norm.y >> norm.z) || !is_finite_vec(norm)) {
                return {false, "Malformed normal vector at line " + std::to_string(line_num), 
                        "Verify that all normal components are valid finite numbers."};
            }
            current_facet.normal = norm;
            in_facet = true;
            vertex_count = 0;
        } else if (token == "endfacet") {
            if (vertex_count != 3) {
                return {false, "Facet at line " + std::to_string(line_num) + " has " + std::to_string(vertex_count) + " vertices (expected 3).",
                        "Ensure outer loop has exactly 3 vertex entries."};
            }
            current_facet.area = triangle_area(current_facet.vertices[0], current_facet.vertices[1], current_facet.vertices[2]);
            out_model.total_surface_area += current_facet.area;
            for (const auto& v : current_facet.vertices) {
                out_model.bbox.update(v);
            }
            out_model.facets.push_back(current_facet);
            in_facet = false;
        } else if (token == "outer") {
            in_loop = true;
        } else if (token == "endloop") {
            in_loop = false;
        } else if (token == "vertex") {
            if (vertex_count >= 3) {
                return {false, "More than 3 vertices in loop at line " + std::to_string(line_num),
                        "Check STL topology: each triangular facet must contain exactly 3 vertices."};
            }
            Vec3 v;
            if (!(ls >> v.x >> v.y >> v.z) || !is_finite_vec(v)) {
                return {false, "Non-finite vertex coordinate at line " + std::to_string(line_num),
                        "Clean the CAD geometry to remove NaN or Infinite coordinates."};
            }
            current_facet.vertices[vertex_count++] = v;
        }
    }

    if (out_model.facets.empty()) {
        return {false, "No valid triangular facets found in ASCII STL file.", 
                "Verify file is a populated ASCII STL starting with 'solid <name>' and containing 'facet normal' blocks."};
    }

    return {true, "", ""};
}

ParseResult StlAsciiParser::parse_file(const std::string& filepath, StlModel& out_model) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return {false, "Cannot open file: " + filepath, "Verify the file path exists and has read permissions."};
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_string(content, out_model);
}

std::string StlAsciiParser::to_json(const StlModel& model) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"success\":true,\"solid_name\":\"" << model.solid_name << "\",";
    ss << "\"facet_count\":" << model.facets.size() << ",";
    ss << "\"total_surface_area\":" << model.total_surface_area << ",";
    ss << "\"file_size_bytes\":" << model.file_size_bytes << ",";
    ss << "\"bbox\":{\"min\":[" << model.bbox.min_pt.x << "," << model.bbox.min_pt.y << "," << model.bbox.min_pt.z << "],";
    ss << "\"max\":[" << model.bbox.max_pt.x << "," << model.bbox.max_pt.y << "," << model.bbox.max_pt.z << "],";
    ss << "\"center\":[" << model.bbox.center().x << "," << model.bbox.center().y << "," << model.bbox.center().z << "],";
    ss << "\"size\":[" << model.bbox.size().x << "," << model.bbox.size().y << "," << model.bbox.size().z << "]},";
    
    // Output flattened triangle vertex array for WebGL buffer: [x1,y1,z1, x2,y2,z2, ...]
    ss << "\"vertices\":[";
    bool first = true;
    for (const auto& f : model.facets) {
        for (int i = 0; i < 3; ++i) {
            if (!first) ss << ",";
            ss << f.vertices[i].x << "," << f.vertices[i].y << "," << f.vertices[i].z;
            first = false;
        }
    }
    ss << "],";

    // Output flattened normal array for lighting:
    ss << "\"normals\":[";
    first = true;
    for (const auto& f : model.facets) {
        for (int i = 0; i < 3; ++i) {
            if (!first) ss << ",";
            ss << f.normal.x << "," << f.normal.y << "," << f.normal.z;
            first = false;
        }
    }
    ss << "]}";

    return ss.str();
}

ParseResult AnsysCsvParser::parse_string(const std::string& content, AnsysResults& out_results) {
    out_results = AnsysResults{};
    out_results.file_size_bytes = content.size();

    if (content.empty()) {
        return {false, "File is empty (0 bytes).", "Ensure the chosen ANSYS CSV file contains nodal data."};
    }

    if (content.size() > MAX_RECOMMENDED_SIZE_BYTES) {
        return {false, "File exceeds maximum tested limit (100 MB).", 
                "Downsample the nodal mesh to under 100 MB for browser rendering."};
    }

    std::istringstream stream(content);
    std::string line;
    int line_num = 0;
    bool header_found = false;
    int col_id = -1, col_x = -1, col_y = -1, col_z = -1, col_t = -1;

    double temp_sum = 0.0;

    while (std::getline(stream, line)) {
        line_num++;
        std::string trimmed = trim(line);
        if (trimmed.empty()) continue;
        if (trimmed[0] == '#' || trimmed.substr(0, 2) == "//") {
            // Comment / metadata line
            if (trimmed.find("SYNTHETIC") != std::string::npos || trimmed.find("synthetic") != std::string::npos) {
                out_results.is_synthetic = true;
            }
            continue;
        }

        if (!header_found) {
            // Parse header columns
            std::istringstream hs(trimmed);
            std::string col;
            int idx = 0;
            while (std::getline(hs, col, ',')) {
                std::string c = trim(col);
                std::transform(c.begin(), c.end(), c.begin(), ::tolower);
                if (c == "node_id" || c == "node" || c == "id") col_id = idx;
                else if (c == "x_m" || c == "x") col_x = idx;
                else if (c == "y_m" || c == "y") col_y = idx;
                else if (c == "z_m" || c == "z") col_z = idx;
                else if (c == "temperature_c" || c == "temperature" || c == "temp_c" || c == "temp") col_t = idx;
                idx++;
            }

            if (col_id == -1 || col_x == -1 || col_y == -1 || col_z == -1 || col_t == -1) {
                return {false, "Header missing required columns at line " + std::to_string(line_num) + 
                               " (expected: node_id, x_m, y_m, z_m, temperature_C).",
                        "Verify CSV export format and ensure required column names are present."};
            }
            header_found = true;
            continue;
        }

        // Data rows
        std::istringstream ds(trimmed);
        std::string item;
        std::vector<std::string> fields;
        while (std::getline(ds, item, ',')) {
            fields.push_back(trim(item));
        }

        int max_req_idx = std::max({col_id, col_x, col_y, col_z, col_t});
        if (static_cast<int>(fields.size()) <= max_req_idx) {
            return {false, "Incomplete row data at line " + std::to_string(line_num),
                    "Ensure every data row has all 5 required fields."};
        }

        try {
            int64_t nid = std::stoll(fields[col_id]);
            double x = std::stod(fields[col_x]);
            double y = std::stod(fields[col_y]);
            double z = std::stod(fields[col_z]);
            double temp = std::stod(fields[col_t]);

            if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(temp)) {
                return {false, "Non-finite value (NaN/Inf) at line " + std::to_string(line_num),
                        "Clean the ANSYS solution data to remove undefined or non-finite thermal values."};
            }

            AnsysNode node{nid, x, y, z, temp};
            out_results.nodes.push_back(node);
            out_results.bbox.update(Vec3{x, y, z});

            if (temp < out_results.min_temp) out_results.min_temp = temp;
            if (temp > out_results.max_temp) out_results.max_temp = temp;
            temp_sum += temp;
        } catch (const std::exception&) {
            return {false, "Number parsing failure at line " + std::to_string(line_num) + ": " + trimmed,
                    "Check for corrupted non-numeric characters in coordinates or temperatures."};
        }
    }

    if (out_results.nodes.empty()) {
        return {false, "No data rows found in ANSYS CSV file.",
                "Ensure file contains data rows beneath the column headers."};
    }

    out_results.mean_temp = temp_sum / static_cast<double>(out_results.nodes.size());
    return {true, "", ""};
}

ParseResult AnsysCsvParser::parse_file(const std::string& filepath, AnsysResults& out_results) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return {false, "Cannot open file: " + filepath, "Verify the file path exists and has read permissions."};
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return parse_string(content, out_results);
}

std::string AnsysCsvParser::to_json(const AnsysResults& res) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "{\"success\":true,\"node_count\":" << res.nodes.size() << ",";
    ss << "\"is_synthetic\":" << (res.is_synthetic ? "true" : "false") << ",";
    ss << "\"file_size_bytes\":" << res.file_size_bytes << ",";
    ss << "\"min_temp\":" << res.min_temp << ",";
    ss << "\"max_temp\":" << res.max_temp << ",";
    ss << "\"mean_temp\":" << res.mean_temp << ",";
    ss << "\"bbox\":{\"min\":[" << res.bbox.min_pt.x << "," << res.bbox.min_pt.y << "," << res.bbox.min_pt.z << "],";
    ss << "\"max\":[" << res.bbox.max_pt.x << "," << res.bbox.max_pt.y << "," << res.bbox.max_pt.z << "],";
    ss << "\"center\":[" << res.bbox.center().x << "," << res.bbox.center().y << "," << res.bbox.center().z << "],";
    ss << "\"size\":[" << res.bbox.size().x << "," << res.bbox.size().y << "," << res.bbox.size().z << "]},";
    
    // Output nodes array: [id, x, y, z, temp, ...]
    ss << "\"nodes\":[";
    bool first = true;
    for (const auto& n : res.nodes) {
        if (!first) ss << ",";
        ss << "[" << n.node_id << "," << n.x << "," << n.y << "," << n.z << "," << n.temperature << "]";
        first = false;
    }
    ss << "]}";

    return ss.str();
}

} // namespace workbench::parsers
