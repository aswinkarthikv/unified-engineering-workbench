import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as spla
import time
import os
import json

class ThermalSolversBenchmark:
    def __init__(self, output_dir="round2/results"):
        self.output_dir = output_dir
        os.makedirs(output_dir, exist_ok=True)
        self.k_al = 205.0       # Thermal conductivity of Aluminum 6061 [W/(m*K)]
        self.h_conv = 25.0      # Convection coefficient [W/(m^2*K)]
        self.t_inf = 20.0       # Ambient temperature [C]
        self.t_base = 100.0     # Base temperature [C]
        self.q_vol = 5.0e5      # Volumetric heat generation [W/m^3]

    def build_cube_mesh(self, nx, ny, nz, L=0.1, W=0.1, H=0.1):
        """
        Builds a regular 3D structured mesh for the Simple Cube.
        Preserves strictly Symmetric Positive Definite (SPD) structure by eliminating
        Dirichlet boundary nodes at z = 0 into the effective RHS.
        """
        dx = L / nx
        dy = W / ny
        dz = H / nz

        # Node index helper
        def node_idx(i, j, k):
            return i + j * (nx + 1) + k * (nx + 1) * (ny + 1)

        # Active unknown nodes: k from 1 to nz (k=0 is Dirichlet base T=100C)
        unknown_nodes = {}
        idx_count = 0
        for k in range(1, nz + 1):
            for j in range(ny + 1):
                for i in range(nx + 1):
                    unknown_nodes[(i, j, k)] = idx_count
                    idx_count += 1

        n_unknowns = idx_count
        rows = []
        cols = []
        vals = []
        b = np.zeros(n_unknowns)

        inv_dx2 = 1.0 / (dx * dx)
        inv_dy2 = 1.0 / (dy * dy)
        inv_dz2 = 1.0 / (dz * dz)

        for (i, j, k), u_idx in unknown_nodes.items():
            diag = 0.0
            rhs = self.q_vol / self.k_al

            # X-direction neighbors
            # Left (i - 1)
            if i > 0:
                nbr_idx = unknown_nodes[(i - 1, j, k)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dx2)
                diag += inv_dx2
            else:
                # Convection at x = 0
                diag += (self.h_conv / self.k_al) / dx
                rhs += (self.h_conv * self.t_inf / self.k_al) / dx

            # Right (i + 1)
            if i < nx:
                nbr_idx = unknown_nodes[(i + 1, j, k)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dx2)
                diag += inv_dx2
            else:
                # Convection at x = L
                diag += (self.h_conv / self.k_al) / dx
                rhs += (self.h_conv * self.t_inf / self.k_al) / dx

            # Y-direction neighbors
            # Front (j - 1)
            if j > 0:
                nbr_idx = unknown_nodes[(i, j - 1, k)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dy2)
                diag += inv_dy2
            else:
                # Convection at y = 0
                diag += (self.h_conv / self.k_al) / dy
                rhs += (self.h_conv * self.t_inf / self.k_al) / dy

            # Back (j + 1)
            if j < ny:
                nbr_idx = unknown_nodes[(i, j + 1, k)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dy2)
                diag += inv_dy2
            else:
                # Convection at y = W
                diag += (self.h_conv / self.k_al) / dy
                rhs += (self.h_conv * self.t_inf / self.k_al) / dy

            # Z-direction neighbors
            # Bottom (k - 1)
            if k > 1:
                nbr_idx = unknown_nodes[(i, j, k - 1)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dz2)
                diag += inv_dz2
            else:
                # k = 1: neighbor at k = 0 is Dirichlet boundary (T = t_base)
                # Move term to RHS to preserve symmetry:
                diag += inv_dz2
                rhs += inv_dz2 * self.t_base

            # Top (k + 1)
            if k < nz:
                nbr_idx = unknown_nodes[(i, j, k + 1)]
                rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_dz2)
                diag += inv_dz2
            else:
                # Convection at z = H
                diag += (self.h_conv / self.k_al) / dz
                rhs += (self.h_conv * self.t_inf / self.k_al) / dz

            rows.append(u_idx)
            cols.append(u_idx)
            vals.append(diag)
            b[u_idx] = rhs

        A = sp.csr_matrix((vals, (rows, cols)), shape=(n_unknowns, n_unknowns))
        return A, b, unknown_nodes, (nx, ny, nz), (dx, dy, dz)

    def build_complex_heat_sink_mesh(self, nx, ny, nz, L=0.1, W=0.1, H=0.06):
        """
        Builds a 3D Finned Aerospace Heat Sink geometry with cutouts.
        Strictly Symmetric Positive Definite (SPD) formulation.
        """
        dx = L / nx
        dy = W / ny
        dz = H / nz

        # 1. Determine solid domain nodes
        active_mask = np.zeros((nx + 1, ny + 1, nz + 1), dtype=bool)
        k_base = max(1, int(round(0.25 * nz)))

        r_cutout = 0.15 * L
        center_x = 0.5 * L
        center_y = 0.5 * W

        num_fins = 5
        fin_pitch = ny / (2 * num_fins - 1)

        for k in range(nz + 1):
            for j in range(ny + 1):
                y_coord = j * dy
                for i in range(nx + 1):
                    x_coord = i * dx
                    dist_to_center = np.sqrt((x_coord - center_x)**2 + (y_coord - center_y)**2)

                    # Cutout hole through base
                    if k <= k_base and dist_to_center < r_cutout:
                        active_mask[i, j, k] = False
                        continue

                    if k <= k_base:
                        active_mask[i, j, k] = True
                    else:
                        band_idx = int(j / fin_pitch)
                        if band_idx % 2 == 0 and band_idx < 2 * num_fins:
                            active_mask[i, j, k] = True
                        else:
                            active_mask[i, j, k] = False

        # Active unknown nodes: k from 1 to nz (k=0 is Dirichlet base)
        unknown_nodes = {}
        idx_count = 0
        for k in range(1, nz + 1):
            for j in range(ny + 1):
                for i in range(nx + 1):
                    if active_mask[i, j, k]:
                        unknown_nodes[(i, j, k)] = idx_count
                        idx_count += 1

        n_unknowns = idx_count
        rows = []
        cols = []
        vals = []
        b = np.zeros(n_unknowns)

        inv_dx2 = 1.0 / (dx * dx)
        inv_dy2 = 1.0 / (dy * dy)
        inv_dz2 = 1.0 / (dz * dz)
        h_fin = 45.0

        for (i, j, k), u_idx in unknown_nodes.items():
            diag = 0.0
            rhs = self.q_vol / self.k_al if k <= k_base else 0.0

            # 6 neighbor directions
            neighbors = [
                (i - 1, j, k, inv_dx2, dx),
                (i + 1, j, k, inv_dx2, dx),
                (i, j - 1, k, inv_dy2, dy),
                (i, j + 1, k, inv_dy2, dy),
                (i, j, k - 1, inv_dz2, dz),
                (i, j, k + 1, inv_dz2, dz)
            ]

            for ni, nj, nk, inv_d2, delta in neighbors:
                # Check if bottom neighbor is Dirichlet base (nk = 0)
                if nk == 0 and (0 <= ni <= nx) and (0 <= nj <= ny) and active_mask[ni, nj, 0]:
                    diag += inv_d2
                    rhs += inv_d2 * self.t_base
                    continue

                # Check if neighbor is an active unknown
                if (ni, nj, nk) in unknown_nodes:
                    nbr_idx = unknown_nodes[(ni, nj, nk)]
                    rows.append(u_idx); cols.append(nbr_idx); vals.append(-inv_d2)
                    diag += inv_d2
                else:
                    # Convective boundary face
                    h_curr = h_fin if k > k_base else self.h_conv
                    diag += (h_curr / self.k_al) / delta
                    rhs += (h_curr * self.t_inf / self.k_al) / delta

            rows.append(u_idx)
            cols.append(u_idx)
            vals.append(diag)
            b[u_idx] = rhs

        A = sp.csr_matrix((vals, (rows, cols)), shape=(n_unknowns, n_unknowns))
        return A, b, unknown_nodes, active_mask, (nx, ny, nz), (dx, dy, dz)

    # -------------------------------------------------------------
    # SOLVER 1: Direct Sparse Factorization (Cholesky/LU)
    # -------------------------------------------------------------
    def solve_direct_cholesky(self, A, b):
        t0 = time.perf_counter()
        solve_fn = spla.factorized(A.tocsc())
        t_factor = time.perf_counter() - t0

        t1 = time.perf_counter()
        x = solve_fn(b)
        t_solve = time.perf_counter() - t1
        t_total = t_factor + t_solve

        r = b - A.dot(x)
        res_norm = np.linalg.norm(r) / (np.linalg.norm(b) + 1e-12)

        # 3D fill-in non-zero estimate
        est_factor_nnz = int(A.nnz * (A.shape[0]**(1.0/3.0) * 0.45))
        mem_mb = (A.nnz + est_factor_nnz) * 16 / (1024 * 1024)

        return {
            'solution': x,
            'time_ms': t_total * 1000.0,
            'factor_time_ms': t_factor * 1000.0,
            'solve_time_ms': t_solve * 1000.0,
            'iterations': 1,
            'residual': float(res_norm),
            'memory_mb': float(mem_mb),
            'residual_history': [1.0, float(res_norm)]
        }

    # -------------------------------------------------------------
    # SOLVER 2: Preconditioned Conjugate Gradient (PCG)
    # -------------------------------------------------------------
    def solve_pcg(self, A, b, tol=1e-6, maxiter=3000):
        diag_A = A.diagonal()
        inv_diag = 1.0 / np.where(diag_A != 0, diag_A, 1.0)
        M = sp.diags(inv_diag, 0, shape=A.shape, format='csr')

        res_history = []
        b_norm = np.linalg.norm(b)
        if b_norm == 0: b_norm = 1.0

        def callback(xk):
            rk = b - A.dot(xk)
            res_history.append(float(np.linalg.norm(rk) / b_norm))

        t0 = time.perf_counter()
        x, info = spla.cg(A, b, M=M, rtol=tol, maxiter=maxiter, callback=callback)
        t_total = time.perf_counter() - t0

        r = b - A.dot(x)
        final_res = np.linalg.norm(r) / b_norm
        mem_mb = (A.nnz * 16 + 4 * A.shape[0] * 8) / (1024 * 1024)

        if not res_history:
            res_history = [1.0, float(final_res)]

        return {
            'solution': x,
            'time_ms': t_total * 1000.0,
            'iterations': len(res_history),
            'residual': float(final_res),
            'memory_mb': float(mem_mb),
            'residual_history': res_history
        }

    # -------------------------------------------------------------
    # SOLVER 3: Bi-Conjugate Gradient Stabilized (BiCGSTAB / Multilevel Smoother)
    # -------------------------------------------------------------
    def solve_bicgstab(self, A, b, tol=1e-6, maxiter=3000):
        diag_A = A.diagonal()
        inv_diag = 1.0 / np.where(diag_A != 0, diag_A, 1.0)
        M = sp.diags(inv_diag, 0, shape=A.shape, format='csr')

        res_history = []
        b_norm = np.linalg.norm(b)
        if b_norm == 0: b_norm = 1.0

        def callback(xk):
            rk = b - A.dot(xk)
            res_history.append(float(np.linalg.norm(rk) / b_norm))

        t0 = time.perf_counter()
        x, info = spla.bicgstab(A, b, M=M, rtol=tol, maxiter=maxiter, callback=callback)
        t_total = time.perf_counter() - t0

        r = b - A.dot(x)
        final_res = np.linalg.norm(r) / b_norm
        mem_mb = (A.nnz * 16 + 6 * A.shape[0] * 8) / (1024 * 1024)

        if not res_history:
            res_history = [1.0, float(final_res)]

        return {
            'solution': x,
            'time_ms': t_total * 1000.0,
            'iterations': len(res_history),
            'residual': float(final_res),
            'memory_mb': float(mem_mb),
            'residual_history': res_history
        }

    # -------------------------------------------------------------
    # SOLVER 4: Successive Over-Relaxation (SOR) / Geometric Multigrid Relaxation
    # -------------------------------------------------------------
    def solve_sor(self, A, b, tol=1e-6, maxiter=3000, omega=1.25):
        """
        Successive Over-Relaxation (SOR) Classical Stationary Solver.
        Splits A = D - L - U, solving (D - omega*L) x_{k+1} = [(1 - omega)*D + omega*U] x_k + omega*b.
        """
        N = A.shape[0]
        x = np.zeros(N)
        b_norm = np.linalg.norm(b)
        if b_norm == 0: b_norm = 1.0

        # Extract lower triangular and diagonal
        A_csr = A.tocsr()
        diag = A_csr.diagonal()
        D = sp.diags(diag, 0, format='csr')
        L = -sp.tril(A_csr, k=-1, format='csr')
        U = -sp.triu(A_csr, k=1, format='csr')

        # Iteration matrix: M_sor = (D - omega*L)
        M_sor = (D - omega * L).tocsc()
        N_sor = ((1.0 - omega) * D + omega * U).tocsr()
        rhs_fixed = omega * b

        res_history = []
        t0 = time.perf_counter()

        # Pre-factor lower triangular solver for fast forward substitution
        solve_L = spla.factorized(M_sor)

        for it in range(maxiter):
            # Compute residual
            r = b - A_csr.dot(x)
            res_val = np.linalg.norm(r) / b_norm
            res_history.append(float(res_val))

            if res_val < tol:
                break

            # Forward substitution sweep
            rhs_k = N_sor.dot(x) + rhs_fixed
            x = solve_L(rhs_k)

        t_total = time.perf_counter() - t0
        r_final = b - A_csr.dot(x)
        final_res = np.linalg.norm(r_final) / b_norm
        mem_mb = (A.nnz * 16 + 3 * N * 8) / (1024 * 1024)

        return {
            'solution': x,
            'time_ms': t_total * 1000.0,
            'iterations': len(res_history),
            'residual': float(final_res),
            'memory_mb': float(mem_mb),
            'residual_history': res_history
        }

    def run_full_benchmark(self):
        print("="*75)
        print("PASS SCIENCE WORKS ROUND 2: 3D THERMAL SOLVER BENCHMARK ENGINE")
        print("  Solvers: 1. Direct Cholesky | 2. PCG (Krylov SPD) | 3. BiCGSTAB | 4. SOR")
        print("="*75)

        cube_grids = [(8, 8, 8), (14, 14, 14), (20, 20, 20), (28, 28, 28)]
        complex_grids = [(12, 12, 10), (18, 18, 14), (26, 26, 20), (36, 36, 28)]

        all_results = {
            'cube': [],
            'complex': []
        }

        # 1. Cube Benchmark
        print("\n--- 1. BENCHMARKING SIMPLE 3D CUBE GEOMETRY ---")
        for gx, gy, gz in cube_grids:
            print(f"\nDiscretizing Cube {gx}x{gy}x{gz}...")
            A, b, u_nodes, dims, spacing = self.build_cube_mesh(gx, gy, gz)
            dofs = A.shape[0]
            print(f"Degrees of Freedom (DOFs): {dofs:,} | Matrix NNZ: {A.nnz:,}")

            print("  [1/4] Solving with Direct Sparse Cholesky...")
            res_direct = self.solve_direct_cholesky(A, b)
            print(f"        Time: {res_direct['time_ms']:.2f} ms | RAM: {res_direct['memory_mb']:.2f} MB | Res: {res_direct['residual']:.2e}")

            print("  [2/4] Solving with Preconditioned Conjugate Gradient (PCG)...")
            res_pcg = self.solve_pcg(A, b)
            print(f"        Time: {res_pcg['time_ms']:.2f} ms | Iters: {res_pcg['iterations']} | RAM: {res_pcg['memory_mb']:.2f} MB | Res: {res_pcg['residual']:.2e}")

            print("  [3/4] Solving with BiCGSTAB...")
            res_bicg = self.solve_bicgstab(A, b)
            print(f"        Time: {res_bicg['time_ms']:.2f} ms | Iters: {res_bicg['iterations']} | RAM: {res_bicg['memory_mb']:.2f} MB | Res: {res_bicg['residual']:.2e}")

            print("  [4/4] Solving with Successive Over-Relaxation (SOR)...")
            res_sor = self.solve_sor(A, b)
            print(f"        Time: {res_sor['time_ms']:.2f} ms | Iters: {res_sor['iterations']} | RAM: {res_sor['memory_mb']:.2f} MB | Res: {res_sor['residual']:.2e}")

            l2_err_pcg = np.linalg.norm(res_direct['solution'] - res_pcg['solution']) / np.linalg.norm(res_direct['solution'])
            l2_err_bicg = np.linalg.norm(res_direct['solution'] - res_bicg['solution']) / np.linalg.norm(res_direct['solution'])
            print(f"        Verification: L2 Error PCG vs Direct: {l2_err_pcg:.2e} | BiCG vs Direct: {l2_err_bicg:.2e}")

            all_results['cube'].append({
                'grid': f"{gx}x{gy}x{gz}",
                'dofs': dofs,
                'nnz': A.nnz,
                'direct': {k: v for k, v in res_direct.items() if k != 'solution'},
                'pcg': {k: v for k, v in res_pcg.items() if k != 'solution'},
                'bicgstab': {k: v for k, v in res_bicg.items() if k != 'solution'},
                'sor': {k: v for k, v in res_sor.items() if k != 'solution'},
                'consistency': {
                    'l2_err_pcg_vs_direct': float(l2_err_pcg),
                    'l2_err_bicg_vs_direct': float(l2_err_bicg),
                    't_min': float(np.min(res_direct['solution'])),
                    't_max': float(np.max(res_direct['solution'])),
                    't_avg': float(np.mean(res_direct['solution']))
                }
            })

            if (gx, gy, gz) == cube_grids[-1]:
                full_field = np.full((gx + 1, gy + 1, gz + 1), self.t_base)
                for (ci, cj, ck), u_idx in u_nodes.items():
                    full_field[ci, cj, ck] = res_direct['solution'][u_idx]
                np.save(os.path.join(self.output_dir, "cube_full_field.npy"), full_field)

        # 2. Complex Heat Sink Benchmark
        print("\n--- 2. BENCHMARKING COMPLEX 3D AEROSPACE FINNED HEAT SINK ---")
        for gx, gy, gz in complex_grids:
            print(f"\nDiscretizing Complex Heat Sink {gx}x{gy}x{gz}...")
            A, b, u_nodes, active_mask, dims, spacing = self.build_complex_heat_sink_mesh(gx, gy, gz)
            dofs = A.shape[0]
            print(f"Active Solid DOFs: {dofs:,} | Matrix NNZ: {A.nnz:,}")

            print("  [1/4] Solving with Direct Sparse Cholesky...")
            res_direct = self.solve_direct_cholesky(A, b)
            print(f"        Time: {res_direct['time_ms']:.2f} ms | RAM: {res_direct['memory_mb']:.2f} MB | Res: {res_direct['residual']:.2e}")

            print("  [2/4] Solving with Preconditioned Conjugate Gradient (PCG)...")
            res_pcg = self.solve_pcg(A, b)
            print(f"        Time: {res_pcg['time_ms']:.2f} ms | Iters: {res_pcg['iterations']} | RAM: {res_pcg['memory_mb']:.2f} MB | Res: {res_pcg['residual']:.2e}")

            print("  [3/4] Solving with BiCGSTAB...")
            res_bicg = self.solve_bicgstab(A, b)
            print(f"        Time: {res_bicg['time_ms']:.2f} ms | Iters: {res_bicg['iterations']} | RAM: {res_bicg['memory_mb']:.2f} MB | Res: {res_bicg['residual']:.2e}")

            print("  [4/4] Solving with Successive Over-Relaxation (SOR)...")
            res_sor = self.solve_sor(A, b)
            print(f"        Time: {res_sor['time_ms']:.2f} ms | Iters: {res_sor['iterations']} | RAM: {res_sor['memory_mb']:.2f} MB | Res: {res_sor['residual']:.2e}")

            l2_err_pcg = np.linalg.norm(res_direct['solution'] - res_pcg['solution']) / np.linalg.norm(res_direct['solution'])
            l2_err_bicg = np.linalg.norm(res_direct['solution'] - res_bicg['solution']) / np.linalg.norm(res_direct['solution'])
            print(f"        Verification: L2 Error PCG vs Direct: {l2_err_pcg:.2e} | BiCG vs Direct: {l2_err_bicg:.2e}")

            all_results['complex'].append({
                'grid': f"{gx}x{gy}x{gz}",
                'dofs': dofs,
                'nnz': A.nnz,
                'direct': {k: v for k, v in res_direct.items() if k != 'solution'},
                'pcg': {k: v for k, v in res_pcg.items() if k != 'solution'},
                'bicgstab': {k: v for k, v in res_bicg.items() if k != 'solution'},
                'sor': {k: v for k, v in res_sor.items() if k != 'solution'},
                'consistency': {
                    'l2_err_pcg_vs_direct': float(l2_err_pcg),
                    'l2_err_bicg_vs_direct': float(l2_err_bicg),
                    't_min': float(np.min(res_direct['solution'])),
                    't_max': float(np.max(res_direct['solution'])),
                    't_avg': float(np.mean(res_direct['solution']))
                }
            })

            if (gx, gy, gz) == complex_grids[-1]:
                full_field = np.full((gx + 1, gy + 1, gz + 1), np.nan)
                for ci in range(gx + 1):
                    for cj in range(gy + 1):
                        if active_mask[ci, cj, 0]:
                            full_field[ci, cj, 0] = self.t_base
                for (ci, cj, ck), u_idx in u_nodes.items():
                    full_field[ci, cj, ck] = res_direct['solution'][u_idx]
                np.save(os.path.join(self.output_dir, "complex_full_field.npy"), full_field)
                np.save(os.path.join(self.output_dir, "complex_mask.npy"), active_mask)

        json_path = os.path.join(self.output_dir, "benchmark_metrics.json")
        with open(json_path, 'w') as f:
            json.dump(all_results, f, indent=2)
        print(f"\n[SUCCESS] Benchmark completed and saved to {json_path}")
        return all_results

if __name__ == "__main__":
    bench = ThermalSolversBenchmark()
    bench.run_full_benchmark()
