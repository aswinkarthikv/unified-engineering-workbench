#ifndef WORKBENCH_FEA_SOLVER_HPP
#define WORKBENCH_FEA_SOLVER_HPP

#include <vector>
#include <string>
#include <cmath>
#include <stdexcept>

namespace workbench::fea {

struct ThermalRodParams {
    double length = 1.0;            // m
    double area = 1.0e-4;           // m^2
    double thermal_conductivity = 10.0; // W/(m*K)
    double heat_generation = 10000.0;   // W/m^3
    double t_start = 100.0;         // C at x = 0
    double t_end = 20.0;            // C at x = L
    int num_elements = 10;          // N uniform elements
};

struct ElementMidpointResult {
    int element_index;
    double x_mid;
    double t_fe_interp;
    double t_exact;
    double abs_error;
};

struct FeaSolution {
    ThermalRodParams params;
    std::vector<double> x_coords;       // Size N + 1
    std::vector<double> t_fe_nodes;     // Size N + 1
    std::vector<double> t_exact_nodes;  // Size N + 1
    std::vector<ElementMidpointResult> midpoints; // Size N
    double max_midpoint_error = 0.0;
    double l2_error = 0.0;
};

struct ConvergenceCase {
    int num_elements;
    double element_size_h;
    double max_midpoint_error;
    double theoretical_error; // q * h^2 / (8 * k)
    double observed_order;     // log(e_prev / e_curr) / log(h_prev / h_curr)
};

class ThermalRodSolver {
public:
    static double exact_temperature(double x, const ThermalRodParams& p) {
        if (p.length <= 0.0) throw std::invalid_argument("Rod length must be positive.");
        return p.t_start - (p.t_start - p.t_end) * (x / p.length) 
               + (p.heat_generation / (2.0 * p.thermal_conductivity)) * x * (p.length - x);
    }

    static FeaSolution solve(const ThermalRodParams& params);

    static std::vector<ConvergenceCase> run_convergence_study(
        const std::vector<int>& element_counts, 
        ThermalRodParams base_params = ThermalRodParams{});

    static std::string export_solution_csv(const FeaSolution& sol);
    static std::string export_convergence_csv(const std::vector<ConvergenceCase>& cases);
};

} // namespace workbench::fea

#endif // WORKBENCH_FEA_SOLVER_HPP
