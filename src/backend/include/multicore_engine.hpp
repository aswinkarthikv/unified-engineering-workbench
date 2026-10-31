#ifndef WORKBENCH_MULTICORE_ENGINE_HPP
#define WORKBENCH_MULTICORE_ENGINE_HPP

#include <vector>
#include <string>
#include <chrono>
#include <functional>
#include "fea_solver.hpp"

namespace workbench::multicore {

struct HardwareInfo {
    std::string cpu_brand = "Unknown x86_64 CPU";
    uint32_t logical_cores = 1;
    uint32_t physical_cores = 1;
    uint64_t total_ram_mb = 0;
    uint64_t available_ram_mb = 0;
    std::string compiler_info = "GCC 16.2.0 MinGW-w64";
    std::string optimization_flags = "-O3 -march=native -DNDEBUG";
};

struct BatchSolveConfig {
    uint32_t seed = 42;
    size_t case_count = 5000;
    int mesh_size_n = 50;
    double rod_length = 1.0;
    double rod_area = 1.0e-4;
    double t_start = 100.0;
    double t_end = 20.0;
    double k_min = 5.0;
    double k_max = 50.0;
    double q_min = 1000.0;
    double q_max = 50000.0;
};

struct ThermalSolveCase {
    size_t case_id;
    fea::ThermalRodParams params;
};

struct BenchmarkThreadRun {
    int worker_count;
    std::vector<double> repetition_times_ms; // 5 repetitions
    double median_time_ms = 0.0;
    double mean_time_ms = 0.0;
    double std_dev_ms = 0.0;
    double speedup = 1.0;                   // T1 / Tp
    double efficiency = 1.0;                // (T1 / Tp) / p
    double max_abs_diff_vs_serial = 0.0;    // Agreement check <= 1e-8 C
};

struct MulticoreBenchmarkReport {
    HardwareInfo hw_info;
    BatchSolveConfig config;
    double launch_to_ready_ms = 0.0;
    std::vector<BenchmarkThreadRun> thread_runs;
    bool agreement_verified = false;
    double global_max_difference = 0.0;
    std::string analysis_notes;
};

class ThreadPool {
public:
    explicit ThreadPool(size_t threads);
    ~ThreadPool();
    void enqueue(std::function<void()> task);
    void wait_all();

private:
    struct Impl;
    Impl* pimpl;
};

class MulticoreEngine {
public:
    static HardwareInfo detect_hardware();
    static std::vector<ThermalSolveCase> generate_batch(const BatchSolveConfig& config);
    
    // Serial execution
    static std::vector<fea::FeaSolution> solve_batch_serial(const std::vector<ThermalSolveCase>& batch);

    // Parallel execution using bounded worker pool
    static std::vector<fea::FeaSolution> solve_batch_parallel(const std::vector<ThermalSolveCase>& batch, int num_workers);

    // Full benchmark suite: 1, 2, 4 workers, 5 repetitions + warmup, agreement check
    static MulticoreBenchmarkReport run_benchmark(
        const BatchSolveConfig& config = BatchSolveConfig{},
        double launch_to_ready_time = 0.0);

    static std::string export_benchmark_csv(const MulticoreBenchmarkReport& report);
    static std::string to_json(const MulticoreBenchmarkReport& report);
};

} // namespace workbench::multicore

#endif // WORKBENCH_MULTICORE_ENGINE_HPP
