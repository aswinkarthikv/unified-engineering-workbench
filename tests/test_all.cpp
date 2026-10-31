#include <iostream>
#include <cassert>
#include <cmath>
#include <vector>
#include <string>

#include "fea_solver.hpp"
#include "cad_ansys_parser.hpp"
#include "multicore_engine.hpp"
#include "embedded_mocks.hpp"
#include "backprop_engine.hpp"
#include <filesystem>

namespace fs = std::filesystem;
using namespace workbench;

static std::string resolve_test_path(const std::string& path) {
    if (fs::exists(path)) return path;
    if (fs::exists("../" + path)) return "../" + path;
    if (fs::exists("../../" + path)) return "../../" + path;
    return path;
}

int test_failures = 0;
int test_passes = 0;

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "  [FAIL] " << msg << " (" << __FILE__ << ":" << __LINE__ << ")\n"; \
            test_failures++; \
            return; \
        } \
    } while(0)

#define ASSERT_NEAR(a, b, tol, msg) \
    do { \
        if (std::abs((a) - (b)) > (tol)) { \
            std::cerr << "  [FAIL] " << msg << ": got " << (a) << ", expected " << (b) \
                      << " (diff=" << std::abs((a) - (b)) << " > tol=" << (tol) << ")\n"; \
            test_failures++; \
            return; \
        } \
    } while(0)

void test_fea_positive_convergence() {
    std::cout << "[RUN] test_fea_positive_convergence..." << std::flush;
    auto cases = fea::ThermalRodSolver::run_convergence_study({10, 20, 40, 80});
    ASSERT_TRUE(cases.size() == 4, "Must return 4 convergence cases");

    // Theoretical error is q * h^2 / (8 * k) = 125 * h^2
    // N=10: h=0.1 => 1.25 C
    // N=20: h=0.05 => 0.3125 C
    // N=40: h=0.025 => 0.078125 C
    // N=80: h=0.0125 => 0.01953125 C
    ASSERT_NEAR(cases[0].max_midpoint_error, 1.25, 1e-6, "N=10 midpoint error");
    ASSERT_NEAR(cases[1].max_midpoint_error, 0.3125, 1e-6, "N=20 midpoint error");
    ASSERT_NEAR(cases[2].max_midpoint_error, 0.078125, 1e-6, "N=40 midpoint error");
    ASSERT_NEAR(cases[3].max_midpoint_error, 0.01953125, 1e-6, "N=80 midpoint error");

    // Convergence order must be 2.0 (second-order)
    ASSERT_NEAR(cases[1].observed_order, 2.0, 0.01, "N=20 convergence order");
    ASSERT_NEAR(cases[2].observed_order, 2.0, 0.01, "N=40 convergence order");
    ASSERT_NEAR(cases[3].observed_order, 2.0, 0.01, "N=80 convergence order");

    std::cout << " OK\n";
    test_passes++;
}

void test_fea_negative_validation() {
    std::cout << "[RUN] test_fea_negative_validation..." << std::flush;
    fea::ThermalRodParams p;
    p.thermal_conductivity = -5.0; // Invalid negative k
    try {
        fea::ThermalRodSolver::solve(p);
        ASSERT_TRUE(false, "Should have thrown for negative conductivity");
    } catch (const std::invalid_argument&) {
        // Expected
    }

    p.thermal_conductivity = 10.0;
    p.num_elements = 0; // Invalid elements count
    try {
        fea::ThermalRodSolver::solve(p);
        ASSERT_TRUE(false, "Should have thrown for 0 elements");
    } catch (const std::invalid_argument&) {
        // Expected
    }

    std::cout << " OK\n";
    test_passes++;
}

void test_stl_parser_positive() {
    std::cout << "[RUN] test_stl_parser_positive..." << std::flush;
    parsers::StlModel model;
    auto res = parsers::StlAsciiParser::parse_file(resolve_test_path("data/cad/bracket_ascii.stl"), model);
    ASSERT_TRUE(res.success, "Parsing valid ASCII STL should succeed");
    ASSERT_TRUE(model.facets.size() == 20, "Should have exactly 20 facets");
    ASSERT_NEAR(model.bbox.min_pt.x, 0.0, 1e-5, "bbox min x");
    ASSERT_NEAR(model.bbox.max_pt.x, 0.06, 1e-5, "bbox max x");
    ASSERT_NEAR(model.total_surface_area, 0.0108, 1e-4, "Surface area should be 0.0108 m2");

    std::cout << " OK\n";
    test_passes++;
}

void test_stl_parser_negative_nan() {
    std::cout << "[RUN] test_stl_parser_negative_nan..." << std::flush;
    parsers::StlModel model;
    auto res = parsers::StlAsciiParser::parse_file(resolve_test_path("tests/fixtures/corrupted_nan.stl"), model);
    ASSERT_TRUE(!res.success, "Should fail on NaN vertex coordinate");
    ASSERT_TRUE(res.error_message.find("Non-finite") != std::string::npos, "Error should indicate non-finite coordinate");
    ASSERT_TRUE(!res.actionable_fix.empty(), "Should provide actionable fix");

    std::cout << " OK\n";
    test_passes++;
}

void test_stl_parser_negative_empty() {
    std::cout << "[RUN] test_stl_parser_negative_empty..." << std::flush;
    parsers::StlModel model;
    auto res = parsers::StlAsciiParser::parse_file(resolve_test_path("tests/fixtures/empty.stl"), model);
    ASSERT_TRUE(!res.success, "Should fail on empty file");
    ASSERT_TRUE(res.error_message.find("empty") != std::string::npos, "Error should identify empty file");

    std::cout << " OK\n";
    test_passes++;
}

void test_ansys_parser_positive() {
    std::cout << "[RUN] test_ansys_parser_positive..." << std::flush;
    parsers::AnsysResults results;
    auto res = parsers::AnsysCsvParser::parse_file(resolve_test_path("data/ansys/synthetic_rod_ansys.csv"), results);
    ASSERT_TRUE(res.success, "Should parse synthetic ANSYS CSV successfully");
    ASSERT_TRUE(results.nodes.size() == 525, "Should have 525 nodes");
    ASSERT_TRUE(results.is_synthetic, "Should correctly identify synthetic label");
    ASSERT_NEAR(results.min_temp, 14.95, 0.5, "Min temp near 14.95 C");
    ASSERT_NEAR(results.max_temp, 187.9, 0.5, "Max temp near 187.9 C");

    std::cout << " OK\n";
    test_passes++;
}

void test_ansys_parser_negative_missing_cols() {
    std::cout << "[RUN] test_ansys_parser_negative_missing_cols..." << std::flush;
    parsers::AnsysResults results;
    auto res = parsers::AnsysCsvParser::parse_file(resolve_test_path("tests/fixtures/missing_columns.csv"), results);
    ASSERT_TRUE(!res.success, "Should reject missing columns");
    ASSERT_TRUE(res.error_message.find("missing") != std::string::npos, "Error message should mention missing columns");

    std::cout << " OK\n";
    test_passes++;
}

void test_ansys_parser_negative_nan() {
    std::cout << "[RUN] test_ansys_parser_negative_nan..." << std::flush;
    parsers::AnsysResults results;
    auto res = parsers::AnsysCsvParser::parse_file(resolve_test_path("tests/fixtures/non_finite.csv"), results);
    ASSERT_TRUE(!res.success, "Should reject NaN temperature");
    ASSERT_TRUE(res.error_message.find("Non-finite") != std::string::npos, "Error should report Non-finite");

    std::cout << " OK\n";
    test_passes++;
}

void test_backprop_gradients_central_diff() {
    std::cout << "[RUN] test_backprop_gradients_central_diff..." << std::flush;
    backprop::ModelParameters p0{0.5, 0.0, 1.0, 0.0};
    double x = 1.0, y = 2.0;
    auto fwd = backprop::BackpropEngine::forward(p0, x, y);
    ASSERT_NEAR(fwd.z, 0.5, 1e-9, "z = w*x + b");
    ASSERT_NEAR(fwd.h, 0.46211715726000976, 1e-9, "h = tanh(0.5)");
    ASSERT_NEAR(fwd.y_hat, 0.46211715726000976, 1e-9, "y_hat = v*h + c");
    ASSERT_NEAR(fwd.loss, 1.1825418196147457, 1e-9, "loss = 0.5*(y_hat - y)^2");

    auto checks = backprop::BackpropEngine::run_gradient_check(p0, x, y, 1e-6);
    ASSERT_TRUE(checks.size() == 4, "Must check all 4 parameters");
    for (const auto& c : checks) {
        ASSERT_TRUE(c.passed, "Gradient check must pass for " + c.param_name);
        ASSERT_TRUE(c.abs_error <= 1e-5, "Absolute error must be <= 1e-5 per brief");
    }

    std::cout << " OK\n";
    test_passes++;
}

void test_backprop_training_loss_decrease() {
    std::cout << "[RUN] test_backprop_training_loss_decrease..." << std::flush;
    backprop::ModelParameters p0{0.5, 0.0, 1.0, 0.0};
    auto history = backprop::BackpropEngine::train(p0, 1.0, 2.0, 0.1, 100);
    ASSERT_TRUE(history.size() == 101, "Should record 101 steps (0 to 100)");

    double initial_loss = history.front().fwd.loss;
    double final_loss = history.back().fwd.loss;

    ASSERT_TRUE(final_loss < initial_loss, "Final loss must be significantly lower than initial loss");
    ASSERT_TRUE(final_loss < 0.001, "Final loss after 100 steps should converge below 0.001");
    ASSERT_NEAR(history.back().fwd.y_hat, 2.0, 0.05, "y_hat should converge towards target 2.0");

    std::cout << " OK\n";
    test_passes++;
}

void test_multicore_agreement() {
    std::cout << "[RUN] test_multicore_agreement..." << std::flush;
    multicore::BatchSolveConfig cfg;
    cfg.case_count = 200;
    cfg.mesh_size_n = 40;
    cfg.seed = 42;

    auto batch = multicore::MulticoreEngine::generate_batch(cfg);
    auto serial_sol = multicore::MulticoreEngine::solve_batch_serial(batch);
    auto parallel_sol_2 = multicore::MulticoreEngine::solve_batch_parallel(batch, 2);
    auto parallel_sol_4 = multicore::MulticoreEngine::solve_batch_parallel(batch, 4);

    double max_diff = 0.0;
    for (size_t i = 0; i < batch.size(); ++i) {
        for (size_t j = 0; j < serial_sol[i].t_fe_nodes.size(); ++j) {
            double d2 = std::abs(serial_sol[i].t_fe_nodes[j] - parallel_sol_2[i].t_fe_nodes[j]);
            double d4 = std::abs(serial_sol[i].t_fe_nodes[j] - parallel_sol_4[i].t_fe_nodes[j]);
            if (d2 > max_diff) max_diff = d2;
            if (d4 > max_diff) max_diff = d4;
        }
    }

    ASSERT_TRUE(max_diff <= 1e-8, "Serial vs Parallel difference must be <= 1e-8 C per Task D");

    std::cout << " OK (max diff = " << max_diff << " C <= 1e-8 C)\n";
    test_passes++;
}

void test_embedded_mocks_and_fault_recovery() {
    std::cout << "[RUN] test_embedded_mocks_and_fault_recovery..." << std::flush;
    auto& mgr = embedded::EmbeddedWorkbenchManager::instance();
    auto caps = mgr.discover_all();
    ASSERT_TRUE(caps.size() == 3, "Should discover 3 devices (MCU, FPGA, JTAG)");

    // Test JTAG adapter telemetry and explanation
    auto tels = mgr.poll_all();
    ASSERT_TRUE(tels.size() == 3, "Should poll 3 devices");
    ASSERT_TRUE(tels[2].role_explanation.find("debug") != std::string::npos, 
                "JTAG adapter must explain that it is a debug interface, not generic compute board");

    // Inject fault into FPGA
    mgr.inject_fault("Artix", embedded::FaultMode::TIMEOUT_ON_NEXT_READ);
    auto tels_fault = mgr.poll_all();
    ASSERT_TRUE(tels_fault[1].state == embedded::ConnectionState::ERROR_STATE, "FPGA should transition to ERROR_STATE");
    ASSERT_TRUE(tels_fault[1].error_count > 0, "Error count should increment");

    // Recover FPGA
    bool rec_ok = mgr.recover_device("Artix");
    ASSERT_TRUE(rec_ok, "Recovery should succeed");
    auto tels_recovered = mgr.poll_all();
    ASSERT_TRUE(tels_recovered[1].state == embedded::ConnectionState::CONNECTED, "FPGA should restore to CONNECTED");

    std::cout << " OK\n";
    test_passes++;
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  UNIFIED ENGINEERING WORKBENCH UNIT TEST SUITE (C++17)  \n";
    std::cout << "=========================================================\n";

    test_fea_positive_convergence();
    test_fea_negative_validation();
    test_stl_parser_positive();
    test_stl_parser_negative_nan();
    test_stl_parser_negative_empty();
    test_ansys_parser_positive();
    test_ansys_parser_negative_missing_cols();
    test_ansys_parser_negative_nan();
    test_backprop_gradients_central_diff();
    test_backprop_training_loss_decrease();
    test_multicore_agreement();
    test_embedded_mocks_and_fault_recovery();

    std::cout << "\n---------------------------------------------------------\n";
    std::cout << "TEST SUMMARY: " << test_passes << " PASSED, " << test_failures << " FAILED.\n";
    std::cout << "---------------------------------------------------------\n";

    return (test_failures == 0) ? 0 : 1;
}
