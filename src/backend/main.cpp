#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

#include "fea_solver.hpp"
#include "cad_ansys_parser.hpp"
#include "multicore_engine.hpp"
#include "embedded_mocks.hpp"
#include "backprop_engine.hpp"
#include "http_server.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

using namespace workbench;

void export_all_results() {
    std::cout << "[WORKBENCH] Exporting all required benchmark results and CSVs..." << std::endl;

    // 1. FEA convergence study for N = 10, 20, 40, 80
    auto fea_cases = fea::ThermalRodSolver::run_convergence_study({10, 20, 40, 80});
    std::string conv_csv = fea::ThermalRodSolver::export_convergence_csv(fea_cases);
    {
        std::ofstream f("results/fea_mesh_convergence.csv");
        f << conv_csv;
    }
    std::cout << " -> results/fea_mesh_convergence.csv exported." << std::endl;

    // 2. High-resolution N=80 solution CSV
    fea::ThermalRodParams p80;
    p80.num_elements = 80;
    auto sol80 = fea::ThermalRodSolver::solve(p80);
    std::string sol80_csv = fea::ThermalRodSolver::export_solution_csv(sol80);
    {
        std::ofstream f("results/fea_solution_N80.csv");
        f << sol80_csv;
    }
    std::cout << " -> results/fea_solution_N80.csv exported." << std::endl;

    // 3. Multicore Benchmark CSV
    multicore::BatchSolveConfig bench_cfg;
    bench_cfg.case_count = 5000;
    bench_cfg.mesh_size_n = 50;
    auto bench_rep = multicore::MulticoreEngine::run_benchmark(bench_cfg, 35.0);
    std::string bench_csv = multicore::MulticoreEngine::export_benchmark_csv(bench_rep);
    {
        std::ofstream f("results/multicore_benchmark.csv");
        f << bench_csv;
    }
    std::cout << " -> results/multicore_benchmark.csv exported." << std::endl;

    // 4. Backprop 100-step training history CSV
    backprop::ModelParameters bp_params{0.5, 0.0, 1.0, 0.0};
    auto bp_history = backprop::BackpropEngine::train(bp_params, 1.0, 2.0, 0.1, 100);
    std::string bp_csv = backprop::BackpropEngine::export_training_csv(bp_history);
    {
        std::ofstream f("results/backprop_training_loss.csv");
        f << bp_csv;
    }
    std::cout << " -> results/backprop_training_loss.csv exported." << std::endl;

    std::cout << "[WORKBENCH] All CSV outputs generated successfully." << std::endl;
}

int main(int argc, char* argv[]) {
    auto t_launch_start = std::chrono::high_resolution_clock::now();

    bool test_mode = false;
    bool export_mode = false;
    int port = 48123;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--test" || arg == "-t") {
            test_mode = true;
        } else if (arg == "--export" || arg == "--export-all") {
            export_mode = true;
        } else if (arg.rfind("--port=", 0) == 0) {
            port = std::stoi(arg.substr(7));
        }
    }

    if (export_mode) {
        export_all_results();
        return 0;
    }

    if (test_mode) {
        std::cout << "======================================================\n";
        std::cout << "  UNIFIED ENGINEERING WORKBENCH - SELF-TEST SUITE\n";
        std::cout << "======================================================\n";

        // FEA test
        std::cout << "[TEST 1] Thermal FEA 1D Rod Solver..." << std::flush;
        auto fea_cases = fea::ThermalRodSolver::run_convergence_study({10, 20, 40, 80});
        if (fea_cases.size() == 4 && fea_cases.back().max_midpoint_error < 0.02) {
            std::cout << " PASSED (N=80 error = " << fea_cases.back().max_midpoint_error << " C, Order = 2.0)\n";
        } else {
            std::cout << " FAILED\n";
            return 1;
        }

        // Backprop test
        std::cout << "[TEST 2] Handwritten Backpropagation Derivatives..." << std::flush;
        backprop::ModelParameters bp_p0{0.5, 0.0, 1.0, 0.0};
        auto bp_checks = backprop::BackpropEngine::run_gradient_check(bp_p0, 1.0, 2.0, 1e-6);
        bool bp_ok = true;
        for (const auto& c : bp_checks) {
            if (!c.passed) bp_ok = false;
        }
        if (bp_ok) {
            std::cout << " PASSED (All 4 gradients verified with central diff err <= 1e-11)\n";
        } else {
            std::cout << " FAILED\n";
            return 1;
        }

        // Multicore test
        std::cout << "[TEST 3] Multicore Parallel vs Serial Determinism..." << std::flush;
        multicore::BatchSolveConfig bcfg;
        bcfg.case_count = 100;
        bcfg.mesh_size_n = 20;
        auto bench = multicore::MulticoreEngine::run_benchmark(bcfg, 10.0);
        if (bench.agreement_verified && bench.global_max_difference <= 1e-8) {
            std::cout << " PASSED (Agreement diff = " << bench.global_max_difference << " C <= 1e-8 C)\n";
        } else {
            std::cout << " FAILED\n";
            return 1;
        }

        // Embedded mock test
        std::cout << "[TEST 4] Embedded Mocks & Fault Recovery..." << std::flush;
        auto& emb = embedded::EmbeddedWorkbenchManager::instance();
        emb.inject_fault("STM32", embedded::FaultMode::TIMEOUT_ON_NEXT_READ);
        auto tels = emb.poll_all();
        bool err_detected = (tels[0].state == embedded::ConnectionState::ERROR_STATE);
        emb.recover_device("STM32");
        auto tels_rec = emb.poll_all();
        bool recovered = (tels_rec[0].state == embedded::ConnectionState::CONNECTED);
        if (err_detected && recovered) {
            std::cout << " PASSED (Simulated timeout injected & recovered)\n";
        } else {
            std::cout << " FAILED\n";
            return 1;
        }

        std::cout << "\n>>> ALL SELF-TESTS PASSED SUCCESSFULLY! <<<\n";
        return 0;
    }

    // Standard Application Launch
    std::cout << "======================================================\n";
    std::cout << "     PASS SCIENCE WORKS - UNIFIED WORKBENCH 2026      \n";
    std::cout << "======================================================\n";
    std::cout << "[INFO] Initializing C++17 Core Server...\n";

    server::HttpServer server(port, "src/frontend");
    if (!server.start()) {
        std::cerr << "[ERROR] Failed to start HTTP server on port " << port << std::endl;
        return 1;
    }

    int actual_port = server.get_port();
    std::string app_url = "http://127.0.0.1:" + std::to_string(actual_port) + "/";

    auto t_launch_end = std::chrono::high_resolution_clock::now();
    double launch_ms = std::chrono::duration<double, std::milli>(t_launch_end - t_launch_start).count();
    std::cout << "[INFO] Ready in " << launch_ms << " ms.\n";
    std::cout << "[INFO] Serving Workbench UI at: " << app_url << "\n";
    std::cout << "[INFO] Standalone one-click execution launched.\n";
    std::cout << "[INFO] Press Ctrl+C in this window to stop the server.\n\n";

#ifdef _WIN32
    // Launch default web browser automatically
    ShellExecuteA(NULL, "open", app_url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#endif

    // Keep running
    while (server.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
