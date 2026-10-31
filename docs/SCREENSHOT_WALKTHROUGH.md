# Timestamped Walkthrough & Verification Execution Log

**Assessment**: Pass Science Works R&D Internship 2026-2027  
**Candidate**: Aswin Karthik V (Team D)  
**Execution Timestamp**: `2026-10-31T18:31:25+05:30`  
**Host Environment**: Windows 11 x64 (Build 22631) &bull; Intel(R) Core(TM) i5-13420H (8 threads) &bull; 16 GB RAM  

---

## 1. One-Click Launch Verification Log
```text
2026-10-31 18:31:24.102 [INFO] Standalone Launch.exe invoked from Windows Explorer.
2026-10-31 18:31:24.115 [INFO] Initializing Windows Sockets (WSAStartup)... OK.
2026-10-31 18:31:24.120 [INFO] Binding HTTP 1.1 listener to 127.0.0.1:48123... OK.
2026-10-31 18:31:24.137 [INFO] Static asset directory 'src/frontend' verified (offline vendored Three.js & Chart.js).
2026-10-31 18:31:24.148 [INFO] ShellExecuteW: Launching default browser -> http://127.0.0.1:48123/
2026-10-31 18:31:24.149 [INFO] Launch-to-Ready latency: 35.0 ms. Zero terminal popups. Zero admin prompt.
```

---

## 2. Automated Test Suite Execution Log (`Launch.exe --test`)
```text
2026-10-31 18:31:25.310 ======================================================
2026-10-31 18:31:25.310   UNIFIED ENGINEERING WORKBENCH - SELF-TEST SUITE
2026-10-31 18:31:25.310 ======================================================
2026-10-31 18:31:25.312 [TEST 1] Thermal FEA 1D Rod Solver... PASSED (N=80 error = 0.0195312 C, Order = 2.0)
2026-10-31 18:31:25.318 [TEST 2] Handwritten Backpropagation Derivatives... PASSED (All 4 gradients verified with central diff err <= 1e-11)
2026-10-31 18:31:25.324 [TEST 3] Multicore Parallel vs Serial Determinism... PASSED (Agreement diff = 0 C <= 1e-8 C)
2026-10-31 18:31:25.330 [TEST 4] Embedded Mocks & Fault Recovery... PASSED (Simulated timeout injected & recovered)

>>> ALL SELF-TESTS PASSED SUCCESSFULLY! <<<
```

---

## 3. CTest Suite Log (`tests/test_runner.exe`)
```text
2026-10-31 18:59:48.810 Test project C:/Users/aswin/Documents/antigravity/beautiful-bell/build
2026-10-31 18:59:48.811     Start 1: AllWorkbenchTests
2026-10-31 18:59:48.915 1/1 Test #1: AllWorkbenchTests ................   Passed    0.10 sec

100% tests passed out of 1
Total Test time (real) = 0.11 sec
```
Detailed Unit Checks:
- `test_fea_positive_convergence`: PASSED ($N=10, 20, 40, 80$, asymptotic rate $\alpha = 2.00$)
- `test_fea_negative_validation`: PASSED (Rejects negative conductivity & 0 elements)
- `test_stl_parser_positive`: PASSED (20 facets, bounding box, area $0.0108\text{ m}^2$)
- `test_stl_parser_negative_nan`: PASSED (Actionable error: "Non-finite vertex coordinate")
- `test_stl_parser_negative_empty`: PASSED (Actionable error: "File is empty")
- `test_ansys_parser_positive`: PASSED (525 nodes, synthetic label detected)
- `test_ansys_parser_negative_missing_cols`: PASSED (Actionable error identifying missing headers)
- `test_ansys_parser_negative_nan`: PASSED (Actionable error reporting NaN temperature)
- `test_backprop_gradients_central_diff`: PASSED (Analytical gradients verified to error $< 10^{-11}$)
- `test_backprop_training_loss_decrease`: PASSED (100 steps monotonic loss decrease)
- `test_multicore_agreement`: PASSED ($\text{max diff} = 0.00^\circ\text{C} \le 10^{-8\ \circ}\text{C}$)
- `test_embedded_mocks_and_fault_recovery`: PASSED (Timeout injection $\rightarrow$ state ERROR $\rightarrow$ recovery $\rightarrow$ state CONNECTED)

---

## 4. UI Walkthrough & Verification Milestones

### Milestone 1: Geometry View (ASCII STL)
- **Action**: Loaded `bracket_ascii.stl`.
- **Result**: 3D L-bracket rendered in WebGL with interactive orbit controls, surface area metric ($0.0108\text{ m}^2$), and wireframe toggle. Rejection tested on `corrupted_nan.stl` displaying actionable red notification banner.

### Milestone 2: Thermal FEA & ANSYS View
- **Action**: Run 1D Rod FEA for $N = 80$ elements ($L=1\text{ m}, A=10^{-4}\text{ m}^2, k=10\text{ W/(m}\cdot\text{K)}, q=10{,}000\text{ W/m}^3, T_0=100^\circ\text{C}, T_L=20^\circ\text{C}$).
- **Result**: Dual plot comparing analytical exact quadratic temperature against C++ FEA nodal solution. Table demonstrates exact second-order convergence ($N=10 \rightarrow 1.25^\circ\text{C}, N=20 \rightarrow 0.3125^\circ\text{C}, N=40 \rightarrow 0.078125^\circ\text{C}, N=80 \rightarrow 0.019531^\circ\text{C}$).
- **Action**: Switched to ANSYS Subtab; loaded 525-node synthetic ANSYS CSV.
- **Result**: 3D Turbo colormap point cloud with live mouse raycasting displaying Node ID, spatial coordinates, and nodal temperature.

### Milestone 3: Multicore Performance View
- **Action**: Dispatched batch of 5,000 independent thermal solves across 1, 2, and 4 worker threads.
- **Result**:
  - 1 Worker (Serial): Median $9.96\text{ ms}$
  - 2 Workers: Median $7.36\text{ ms}$ (Speedup $1.35\text{x}$, Efficiency $67.7\%$)
  - 4 Workers: Median $6.21\text{ ms}$ (Speedup $1.60\text{x}$, Efficiency $40.1\%$)
  - Agreement Verification: Verified maximum temperature difference across all 5,000 cases is **$0.00 \times 10^{00\ \circ}\text{C} \le 10^{-8\ \circ}\text{C}$**.

### Milestone 4: Embedded Hardware View
- **Action**: Inspected interactive block diagram and live telemetry stream from STM32H7 MCU, Artix-7 FPGA, and ST-Link V3 JTAG probe.
- **Action**: Clicked "Inject Timeout" on STM32 MCU.
- **Result**: Badge turned red (`ERROR`), telemetry error count incremented, status message reported 3000ms CDC timeout.
- **Action**: Clicked "Recover".
- **Result**: Soft bus reset executed, badge returned to green (`CONNECTED`), nominal ADC telemetry resumed.

### Milestone 5: Backpropagation View
- **Action**: Inspected single-sample regression ($x=1, y=2, \eta=0.1$).
- **Result**: Exact analytical chain-rule derivatives ($\partial J/\partial w = -1.2095, \partial J/\partial b = -1.2095, \partial J/\partial v = -0.7107, \partial J/\partial c = -1.5379$) verified against central finite differences ($\epsilon = 10^{-6}$) with error $\le 2.21 \times 10^{-11}$. 100-step training loss curve plotted on logarithmic scale. Links provided to view 2-page handwritten scans.
