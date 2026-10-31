#include "fea_solver.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>

namespace workbench::fea {

FeaSolution ThermalRodSolver::solve(const ThermalRodParams& params) {
    if (params.length <= 0.0) throw std::invalid_argument("Length must be > 0");
    if (params.area <= 0.0) throw std::invalid_argument("Area must be > 0");
    if (params.thermal_conductivity <= 0.0) throw std::invalid_argument("Thermal conductivity k must be > 0");
    if (params.num_elements < 1) throw std::invalid_argument("Number of elements must be >= 1");

    int N = params.num_elements;
    int num_nodes = N + 1;
    double h = params.length / static_cast<double>(N);
    double kA_h = (params.thermal_conductivity * params.area) / h;
    double qA_h_half = (params.heat_generation * params.area * h) * 0.5;

    FeaSolution solution;
    solution.params = params;
    solution.x_coords.resize(num_nodes);
    solution.t_fe_nodes.resize(num_nodes, 0.0);
    solution.t_exact_nodes.resize(num_nodes, 0.0);

    for (int i = 0; i < num_nodes; ++i) {
        solution.x_coords[i] = i * h;
        solution.t_exact_nodes[i] = exact_temperature(solution.x_coords[i], params);
    }
    // Snap the final node coordinate to exactly length to prevent floating-point accumulation
    solution.x_coords[num_nodes - 1] = params.length;
    solution.t_exact_nodes[num_nodes - 1] = params.t_end;

    // Boundary conditions
    solution.t_fe_nodes[0] = params.t_start;
    solution.t_fe_nodes[N] = params.t_end;

    if (N == 1) {
        // Special case: Single element, both nodes are Dirichlet boundary conditions
        // Nothing to solve for nodes!
    } else {
        // System for interior nodes i = 1 to N - 1 (size M = N - 1)
        int M = N - 1;
        std::vector<double> a(M, -kA_h);     // subdiagonal
        std::vector<double> b(M, 2.0 * kA_h); // main diagonal
        std::vector<double> c(M, -kA_h);     // superdiagonal
        std::vector<double> d(M, 2.0 * qA_h_half); // RHS load contribution

        // Adjust RHS for Dirichlet boundary conditions
        d[0] += kA_h * params.t_start;
        d[M - 1] += kA_h * params.t_end;

        // Thomas algorithm (Tridiagonal matrix algorithm)
        std::vector<double> c_prime(M, 0.0);
        std::vector<double> d_prime(M, 0.0);

        c_prime[0] = c[0] / b[0];
        d_prime[0] = d[0] / b[0];

        for (int i = 1; i < M; ++i) {
            double denom = b[i] - a[i] * c_prime[i - 1];
            c_prime[i] = (i < M - 1) ? (c[i] / denom) : 0.0;
            d_prime[i] = (d[i] - a[i] * d_prime[i - 1]) / denom;
        }

        // Back substitution
        std::vector<double> u(M, 0.0);
        u[M - 1] = d_prime[M - 1];
        for (int i = M - 2; i >= 0; --i) {
            u[i] = d_prime[i] - c_prime[i] * u[i + 1];
        }

        for (int i = 0; i < M; ++i) {
            solution.t_fe_nodes[i + 1] = u[i];
        }
    }

    // Midpoint evaluation & interpolation
    solution.midpoints.resize(N);
    double max_err = 0.0;
    double sum_sq_err = 0.0;

    for (int e = 0; e < N; ++e) {
        double x_mid = 0.5 * (solution.x_coords[e] + solution.x_coords[e + 1]);
        double t_fe_interp = 0.5 * (solution.t_fe_nodes[e] + solution.t_fe_nodes[e + 1]);
        double t_exact = exact_temperature(x_mid, params);
        double err = std::abs(t_fe_interp - t_exact);

        solution.midpoints[e] = ElementMidpointResult{
            e + 1,
            x_mid,
            t_fe_interp,
            t_exact,
            err
        };

        if (err > max_err) {
            max_err = err;
        }
        sum_sq_err += err * err;
    }

    solution.max_midpoint_error = max_err;
    solution.l2_error = std::sqrt(sum_sq_err / static_cast<double>(N));

    return solution;
}

std::vector<ConvergenceCase> ThermalRodSolver::run_convergence_study(
    const std::vector<int>& element_counts, 
    ThermalRodParams base_params) 
{
    std::vector<ConvergenceCase> cases;
    cases.reserve(element_counts.size());

    double prev_h = 0.0;
    double prev_err = 0.0;

    for (int N : element_counts) {
        base_params.num_elements = N;
        FeaSolution sol = solve(base_params);

        double h = base_params.length / static_cast<double>(N);
        // Analytical truncation error for uniform heat generation: q * h^2 / (8 * k)
        double th_err = (base_params.heat_generation * h * h) / (8.0 * base_params.thermal_conductivity);

        double order = 0.0;
        if (prev_h > 0.0 && prev_err > 0.0 && sol.max_midpoint_error > 0.0) {
            order = std::log(prev_err / sol.max_midpoint_error) / std::log(prev_h / h);
        }

        cases.push_back(ConvergenceCase{
            N,
            h,
            sol.max_midpoint_error,
            th_err,
            order
        });

        prev_h = h;
        prev_err = sol.max_midpoint_error;
    }

    return cases;
}

std::string ThermalRodSolver::export_solution_csv(const FeaSolution& sol) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# Unified Engineering Workbench - Thermal FEA 1D Solution\n";
    ss << "# Parameters: L=" << sol.params.length << "m, A=" << sol.params.area 
       << "m2, k=" << sol.params.thermal_conductivity << "W/(m*K), q=" << sol.params.heat_generation 
       << "W/m3, T0=" << sol.params.t_start << "C, TL=" << sol.params.t_end << "C\n";
    ss << "type,index,x_m,T_FE_degC,T_exact_degC,abs_error_degC\n";

    for (size_t i = 0; i < sol.x_coords.size(); ++i) {
        double err = std::abs(sol.t_fe_nodes[i] - sol.t_exact_nodes[i]);
        ss << "node," << i << "," << sol.x_coords[i] << "," << sol.t_fe_nodes[i] << "," 
           << sol.t_exact_nodes[i] << "," << err << "\n";
    }

    for (const auto& m : sol.midpoints) {
        ss << "midpoint," << m.element_index << "," << m.x_mid << "," << m.t_fe_interp << "," 
           << m.t_exact << "," << m.abs_error << "\n";
    }

    return ss.str();
}

std::string ThermalRodSolver::export_convergence_csv(const std::vector<ConvergenceCase>& cases) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# Unified Engineering Workbench - Thermal FEA Mesh Convergence Study\n";
    ss << "num_elements_N,element_size_h_m,max_midpoint_error_degC,theoretical_error_degC,convergence_order\n";

    for (const auto& c : cases) {
        ss << c.num_elements << "," << c.element_size_h << "," 
           << c.max_midpoint_error << "," << c.theoretical_error << "," << c.observed_order << "\n";
    }

    return ss.str();
}

} // namespace workbench::fea
