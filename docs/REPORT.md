# Unified Engineering Workbench: Technical Report
**Pass Science Works | R&D Internship 2026-2027 Assessment**  
**Candidate Name**: Aswin Karthik V (Team D)  
**Date**: October 31, 2026  
**Target Operating System**: Windows 11 x64  

---

## 1. Executive Summary & Architecture Choices

The **Unified Engineering Workbench** is a high-performance, responsive engineering environment integrating CAD geometry visualization, finite element thermal modeling, multicore performance benchmarking, embedded hardware telemetry, and regression backpropagation.

```
+-----------------------------------------------------------------------------------------+
|                               UNIFIED ENGINEERING WORKBENCH                             |
+-----------------------------------------------------------------------------------------+
|                               PRESENTATION & TRANSPORT                                  |
|   - Offline HTML5 / CSS3 Dark CAD Console                                               |
|   - Three.js WebGL 3D Mesh & Point Cloud Rendering                                     |
|   - Chart.js Responsive 2D Scientific Plotting                                          |
+-----------------------------------------------------------------------------------------+
                                          | WinSock2 REST API (Port 48123)
+-----------------------------------------------------------------------------------------+
|                               C++17 NUMERICAL & PARSING CORE                            |
|   +-----------------------+   +-----------------------+   +-------------------------+   |
|   | 1D Thermal FEA Rod    |   | CAD / ANSYS Parsers   |   | Multicore Batch Engine  |   |
|   | - 2-Node Linear FEA   |   | - ASCII STL Parser    |   | - Bounded Thread Pool   |   |
|   | - Thomas Algorithm    |   | - ANSYS CSV Parser    |   | - Deterministic Order   |   |
|   | - O(h^2) Convergence  |   | - NaN/Inf Rejection   |   | - Agreement <= 1e-8 C   |   |
|   +-----------------------+   +-----------------------+   +-------------------------+   |
|   +-----------------------+   +-----------------------+                                 |
|   | Embedded Mocks        |   | Backprop Engine       |                                 |
|   | - MCU / FPGA / JTAG   |   | - Analytical Derivs   |                                 |
|   | - Fault Injection     |   | - Central Diff Checks |                                 |
|   | - Auto Recovery       |   | - 100-Step Training   |                                 |
|   +-----------------------+   +-----------------------+                                 |
+-----------------------------------------------------------------------------------------+
```

### Architectural Principles:
1. **Separation of Concerns**: All numerical computation, tridiagonal matrix inversion, file validation, thread synchronization, and mock hardware telemetry are written in strict standard C++17. The frontend is purely presentational and interacts with the backend over local HTTP sockets.
2. **True One-Click Deployment**: `Launch.exe` is a standalone, statically linked binary (`-static -static-libgcc -static-libstdc++ -lws2_32`). Double-clicking it in Windows 11 immediately spins up the embedded HTTP daemon and opens the user's default browser via `ShellExecuteA()`. Zero terminal commands, zero package managers, and zero external runtimes (no Node, no Python) are required on the host system.

---

## 2. Task B: Existing CAD / ANSYS Data Visualization

### 2.1 ASCII STL Parser
- **Geometry Supplied**: `data/cad/bracket_ascii.stl` representing an L-bracket mounting bracket ($0.06\text{ m} \times 0.04\text{ m} \times 0.05\text{ m}$, 20 facets, surface area $0.0108\text{ m}^2$, CC0 Public Domain).
- **Validation Rules**:
  - Rejects binary STL files by checking initial byte streams for null characters and verifying the `solid` ASCII token.
  - Rejects non-finite floating-point tokens (`NaN` or `Inf`) with actionable messages referencing exact line coordinates.
  - Rejects incomplete loops (facets lacking exactly 3 vertices).
- **3D Interaction**: Rendered using Three.js with full orbit controls (rotate, pan, zoom, fit-to-view, wireframe toggle, and real-time bounding box calculation).
- **Tested Limit**: Tested with meshes up to 50 MB (~250,000 triangular facets).

### 2.2 ANSYS CSV Point Cloud
- **Dataset Supplied**: `data/ansys/synthetic_rod_ansys.csv` (525 nodes, explicitly labeled **SYNTHETIC** in header per Task B).
- **Schema & Validation**: Requires columns `node_id, x_m, y_m, z_m, temperature_C`. Checks for missing headers, corrupted non-numeric fields, and negative volume offsets.
- **Rendering & Inspection**: Renders nodes in 3D using Turbo colormap mapping normalized temperatures from $14.95^\circ\text{C}$ to $187.90^\circ\text{C}$. Raycasting allows users to hover or click any node to inspect its exact spatial position and nodal temperature.
- **Tested Limit**: Tested up to 100 MB (~500,000 nodal entries).

---

## 3. Task C: Mechanical Thermal FEA & Interdisciplinary Adoption

### 3.1 Mathematical Formulation
Governing steady-state 1D heat conduction equation with uniform heat generation:
$$-\frac{d}{dx}\left(k A \frac{dT}{dx}\right) = q A$$
Given: $L = 1.0\text{ m}$, $A = 10^{-4}\text{ m}^2$, $k = 10.0\text{ W/(m}\cdot\text{K)}$, $q = 10{,}000\text{ W/m}^3$, with Dirichlet boundary conditions $T(0) = 100^\circ\text{C}$ and $T(L) = 20^\circ\text{C}$.

Integrating twice yields the exact analytical solution:
$$T_{\text{exact}}(x) = 100 - 80\frac{x}{L} + \frac{q}{2k}x(L - x) = 100 - 80x + 500x(1 - x)$$

### 3.2 Finite Element Formulation
Using two-node linear 1D elements of uniform length $h_e = L / N$:
- Element stiffness matrix:
  $$K^e = \frac{k A}{h_e} \begin{bmatrix} 1 & -1 \\ -1 & 1 \end{bmatrix}$$
- Element heat generation load vector:
  $$F^e = \frac{q A h_e}{2} \begin{bmatrix} 1 \\ 1 \end{bmatrix}$$
- Global assembly yields a symmetric tridiagonal linear system $K T = F$. Dirichlet boundary conditions $T_0 = 100$ and $T_N = 20$ are imposed on the algebraic system. The system is solved in $O(N)$ time using the Thomas Algorithm.

### 3.3 Convergence Analysis & Debugging Insight
In 1D linear FEA, nodal temperatures coincide with the exact solution at the nodes due to Green's function representation properties. Therefore, convergence **must be evaluated at element midpoints**:
$$x_{m, e} = \frac{x_i + x_{i+1}}{2}, \quad T_{\text{FE}}(x_{m, e}) = \frac{T_i + T_{i+1}}{2}$$
The analytical truncation error at midpoints is:
$$T_{\text{exact}}(x_{m, e}) - T_{\text{FE}}(x_{m, e}) = \frac{q h_e^2}{8k} = \frac{10000}{80} h_e^2 = 125 h_e^2$$

| Mesh ($N$) | Element Size $h$ [m] | Max Midpoint Error [$^\circ$C] | Theoretical Error [$^\circ$C] | Observed Order ($\alpha$) |
|---|---|---|---|---|
| 10 | 0.10000 | 1.250000 | 1.250000 | Baseline |
| 20 | 0.05000 | 0.312500 | 0.312500 | **2.00** |
| 40 | 0.02500 | 0.078125 | 0.078125 | **2.00** |
| 80 | 0.01250 | 0.019531 | 0.019531 | **2.00** |

*Debugging Insight*: During initial formulation, applying natural (Neumann) boundary conditions on a pure Dirichlet problem led to singular global stiffness matrices. Modifying RHS load terms to account for eliminated boundary equations ensured unconditional diagonal dominance and zero floating-point drift.

---

## 4. Task D: Multicore / Multithreaded Execution

### 4.1 Benchmark Design & Hardware Specs
- **CPU**: 13th Gen Intel(R) Core(TM) i5-13420H (4 Physical Performance Cores, 8 Logical Threads)
- **RAM**: 16,024 MB
- **Compiler**: GCC 16.2.0 MinGW-w64 (`-O3 -march=native -DNDEBUG`)
- **Launch-to-Ready Latency**: 35.0 ms
- **Workload**: Batch of 5,000 independent thermal solves ($N = 50$ elements per case), generated with fixed seed 42 and randomized positive $k \in [5, 50]$ and $q \in [1000, 50000]$.

### 4.2 Measured Timing & Deterministic Agreement
Five measured repetitions were recorded following a separate warm-up pass:

| Workers ($p$) | Repetitions [ms] | Median [ms] | Mean $\pm$ StdDev [ms] | Speedup ($T_1/T_p$) | Efficiency | Max Temp Diff [$^\circ$C] |
|---|---|---|---|---|---|---|
| **1 (Serial)** | 9.5, 12.5, 9.5, 10.0, 11.5 | **9.96** | 10.60 $\pm$ 1.19 | 1.00x | 100.0% | Baseline |
| **2 Workers** | 5.9, 7.4, 8.0, 6.7, 7.9 | **7.36** | 7.18 $\pm$ 0.79 | **1.35x** | 67.7% | **0.00e+00** |
| **4 Workers** | 4.3, 6.2, 6.4, 5.7, 6.9 | **6.21** | 5.90 $\pm$ 0.89 | **1.60x** | 40.1% | **0.00e+00** |

- **Agreement Verification**: Maximum absolute temperature discrepancy between serial and parallel runs was exactly **$0.00 \times 10^{00\ \circ}\text{C}$**, strictly meeting the $\le 10^{-8\ \circ}\text{C}$ criterion.
- **Concurrency Analysis**: Bounded worker pool chunking eliminates thread creation latency. Sub-linear scaling at 4 threads occurs due to shared L3 cache contention and memory bus bandwidth saturation on the lightweight 1D tridiagonal solver.

---

## 5. Task E: Embedded-System Integration Design

### 5.1 Architecture & Interconnect Topology
The workbench integrates three heterogeneous target devices connected to a central host workstation:

```
[Host PC: Windows 11 Workstation]
       |
       +--- USB CDC-ACM (921,600 baud) -------> [STM32H753ZI Microcontroller (3.3V)]
       |
       +--- FTDI FT2232H High-Speed FIFO ------> [Xilinx Artix-7 FPGA (1.0V/1.8V/3.3V)]
       |
       +--- USB 2.0 Bulk Endpoints (ST-Link) -> [ST-Link V3 Debug Adapter]
                                                      |
                                                      +--- (TCK, TMS, TDI, TDO, VREF) ---> [Target TAP]
```

### 5.2 Interface & Domain Table
| Device | Power Domain | Ground Domain | Host Protocol | Level Shifting | JTAG Signal Ownership |
|---|---|---|---|---|---|
| **MCU (STM32H7)** | 5V VBUS $\rightarrow$ 3.3V LDO | Common Chassis GND | USB CDC-ACM (Virtual COM) | Internal 5V-tolerant LVCMOS | **Slave TAP**: Halts core, reads memory |
| **FPGA (Artix-7)** | 1.0V VCCINT, 1.8V VCCAUX | Isolated Digital GND | FTDI FT2232H FIFO | TXS0108E Bidirectional Translator | **Slave TAP**: Boundary-scan & bitstream |
| **JTAG Adapter** | 5V USB (Target VREF Sensed) | Target Ground Ref | USB 2.0 Bulk (480 Mbps) | Passive $V_{\text{REF}}$ sensing (1.65–3.6V) | **BUS MASTER**: Controls TCK clock and TMS state machine |

### 5.3 Why a JTAG Adapter is a Debug Interface
A JTAG adapter is **not** a general-purpose compute board. It does not run user application code, manage external networking, or execute mathematical workloads. Instead, it implements a dedicated hardware state machine (IEEE 1149.1 / ARM CoreSight SWD) designed to drive the Test Clock (TCK) and Test Mode Select (TMS) lines to access internal CPU shift registers, program non-volatile flash, and monitor hardware watchpoints.

### 5.4 Mock Adapters & Recoverable Failure Simulation
C++ mock adapters implement `connect()`, `disconnect()`, `pollTelemetry()`, and `injectFault()`. When a 3000 ms timeout is injected into the STM32 adapter, the UI transitions to an `ERROR` state with a red badge. Triggering the automated recovery resets the virtual communication buffer and restores normal telemetry streaming.

---

## 6. Task F: Backpropagation & Gradient Check

### 6.1 Problem Setup & Forward Pass
Single sample regression: $x = 1.0$, target $y = 2.0$.  
Model: $z = w x + b$, $h = \tanh(z)$, $\hat{y} = v h + c$, Loss $J = 0.5(\hat{y} - y)^2$.  
Initial parameters: $w = 0.5$, $b = 0$, $v = 1$, $c = 0$, $\eta = 0.1$.

- $z = 0.5(1) + 0 = 0.5$
- $h = \tanh(0.5) = 0.46211716$
- $\hat{y} = (1)(0.46211716) + 0 = 0.46211716$
- $e = \hat{y} - y = -1.53788284$
- Loss $J = 0.5(-1.53788284)^2 = 1.18254182$

### 6.2 Analytical Chain-Rule Derivatives
$$\frac{\partial J}{\partial c} = \hat{y} - y = -1.53788284$$
$$\frac{\partial J}{\partial v} = (\hat{y} - y) h = (-1.53788284)(0.46211716) = -0.71068205$$
$$\frac{\partial h}{\partial z} = 1 - h^2 = 1 - (0.46211716)^2 = 0.78644773$$
$$\frac{\partial J}{\partial z} = (\hat{y} - y) v (1 - h^2) = (-1.53788284)(1)(0.78644773) = -1.20946435$$
$$\frac{\partial J}{\partial w} = \frac{\partial J}{\partial z} x = -1.20946435, \quad \frac{\partial J}{\partial b} = \frac{\partial J}{\partial z} = -1.20946435$$

### 6.3 Central Finite Difference Verification ($\epsilon = 10^{-6}$)
$$g_{\text{num}} = \frac{J(\theta + \epsilon) - J(\theta - \epsilon)}{2\epsilon}$$

| Parameter | Analytical Derivative | Central Difference | Absolute Error | Status ($\le 10^{-5}$) |
|---|---|---|---|---|
| $w$ | -1.20946435 | -1.20946435 | **2.21e-11** | **PASSED** |
| $b$ | -1.20946435 | -1.20946435 | **2.21e-11** | **PASSED** |
| $v$ | -0.71068205 | -0.71068205 | **1.89e-11** | **PASSED** |
| $c$ | -1.53788284 | -1.53788284 | **3.55e-11** | **PASSED** |

### 6.4 Parameter Updates & Training Convergence
Simultaneous update step ($\theta_1 = \theta_0 - \eta \frac{\partial J}{\partial \theta}$):
- $w_1 = 0.5 - 0.1(-1.20946435) = 0.62094644$
- $b_1 = 0.0 - 0.1(-1.20946435) = 0.12094644$
- $v_1 = 1.0 - 0.1(-0.71068205) = 1.07106821$
- $c_1 = 0.0 - 0.1(-1.53788284) = 0.15378828$

*100-Step Trajectory*: Loss decreases monotonically from $1.1825$ at step 0 to $0.00057$ at step 100, with prediction $\hat{y}$ converging smoothly to $1.966$ towards target $2.0$.

---

## 7. Test Outcomes, Error Bounds & Known Limitations

1. **Unit Test Coverage**: All 12 unit tests in `tests/test_all.cpp` pass with 100% success rate across FEA numerical convergence, parser rejection, multicore agreement, and mock recovery.
2. **Known Limitations**:
   - The FEA solver is limited to 1D conduction; multidimensional Navier-Stokes fluid-thermal coupling is outside assessment scope.
   - ASCII STL files consume more storage and parse slower than binary STL; large CAD files (>50 MB) should be decimated before loading.
   - Hardware telemetry is simulated using software state machines rather than physical USB-attached hardware boards.
