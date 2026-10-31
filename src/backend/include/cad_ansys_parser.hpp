#ifndef WORKBENCH_CAD_ANSYS_PARSER_HPP
#define WORKBENCH_CAD_ANSYS_PARSER_HPP

#include <vector>
#include <string>
#include <array>
#include <limits>
#include <cstdint>

namespace workbench::parsers {

struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct BoundingBox {
    Vec3 min_pt{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
    Vec3 max_pt{-std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()};

    void update(const Vec3& pt) {
        if (pt.x < min_pt.x) min_pt.x = pt.x;
        if (pt.y < min_pt.y) min_pt.y = pt.y;
        if (pt.z < min_pt.z) min_pt.z = pt.z;

        if (pt.x > max_pt.x) max_pt.x = pt.x;
        if (pt.y > max_pt.y) max_pt.y = pt.y;
        if (pt.z > max_pt.z) max_pt.z = pt.z;
    }

    Vec3 center() const {
        return Vec3{
            0.5 * (min_pt.x + max_pt.x),
            0.5 * (min_pt.y + max_pt.y),
            0.5 * (min_pt.z + max_pt.z)
        };
    }

    Vec3 size() const {
        return Vec3{
            max_pt.x - min_pt.x,
            max_pt.y - min_pt.y,
            max_pt.z - min_pt.z
        };
    }
};

struct StlFacet {
    Vec3 normal;
    std::array<Vec3, 3> vertices;
    double area = 0.0;
};

struct StlModel {
    std::string solid_name;
    std::vector<StlFacet> facets;
    BoundingBox bbox;
    double total_surface_area = 0.0;
    size_t file_size_bytes = 0;
    std::string provenance_info;
};

struct AnsysNode {
    int64_t node_id;
    double x; // meters
    double y; // meters
    double z; // meters
    double temperature; // Celsius
};

struct AnsysResults {
    std::vector<AnsysNode> nodes;
    BoundingBox bbox;
    double min_temp = std::numeric_limits<double>::infinity();
    double max_temp = -std::numeric_limits<double>::infinity();
    double mean_temp = 0.0;
    size_t file_size_bytes = 0;
    bool is_synthetic = true;
    std::string provenance_info;
};

struct ParseResult {
    bool success = false;
    std::string error_message;
    std::string actionable_fix;
};

class StlAsciiParser {
public:
    static constexpr size_t MAX_RECOMMENDED_SIZE_BYTES = 50 * 1024 * 1024; // 50 MB

    static ParseResult parse_string(const std::string& content, StlModel& out_model);
    static ParseResult parse_file(const std::string& filepath, StlModel& out_model);
    static std::string to_json(const StlModel& model);
};

class AnsysCsvParser {
public:
    static constexpr size_t MAX_RECOMMENDED_SIZE_BYTES = 100 * 1024 * 1024; // 100 MB

    static ParseResult parse_string(const std::string& content, AnsysResults& out_results);
    static ParseResult parse_file(const std::string& filepath, AnsysResults& out_results);
    static std::string to_json(const AnsysResults& results);
};

} // namespace workbench::parsers

#endif // WORKBENCH_CAD_ANSYS_PARSER_HPP
