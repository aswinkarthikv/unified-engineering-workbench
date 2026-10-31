# Unified Engineering Workbench
**Pass Science Works R&D Internship 2026-2027 Assessment Submission**  
**Candidate**: Aswin Karthik V  
**Target Platform**: Windows 11 x64  

---

## 1. Quick Start: One-Click Execution (Zero Install Required)

1. Extract `CandidateID_Workbench.zip` to any local folder on Windows 11.
2. **Double-click `Launch.exe`** directly from Windows File Explorer.
3. The standalone workbench automatically:
   - Starts the embedded high-performance C++17 core HTTP server on `127.0.0.1:48123`.
   - Opens your default web browser to the interactive Workbench Console.
   - **Requires NO command prompt, NO PowerShell, NO administrator privileges, NO Python runtime, NO Node.js, and NO internet connection.**

---

## 2. System Architecture

The application cleanly separates numerical computational routines from presentation:

```
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

---

## 3. Supported Data Formats

1. **ASCII StereoLithography (`.stl`)**:
   - Parses `solid`, `facet normal`, `outer loop`, `vertex`, `endloop`, `endfacet`, `endsolid`.
   - Validates surface normal vectors and checks for non-finite (`NaN`/`Inf`) coordinates.
   - Sample supplied: `data/cad/bracket_ascii.stl` (L-bracket mounting, 20 facets).
2. **ANSYS Nodal Temperature Export (`.csv`)**:
   - Mandatory columns: `node_id,x_m,y_m,z_m,temperature_C`.
   - Sample supplied: `data/ansys/synthetic_rod_ansys.csv` (525 nodes, explicitly labeled **SYNTHETIC** per Task B).
   - Renders 3D coloured point clouds with interactive node inspection tooltips.

---

## 4. Source Build & Testing Instructions

If compiling from source code:

### Prerequisites
- Windows 11 x64
- CMake 3.16 or later
- GCC (MinGW-w64 10+) or Clang or MSVC with C++17 support

### CMake Build Steps
```powershell
# Configure build with MinGW Makefiles
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release

# Build workbench_core, Launch.exe, and test runner
cmake --build build --config Release

# Run automated CTest test suite
ctest --test-dir build --output-on-failure
```

### Direct CLI Flags on `Launch.exe`
```powershell
# Run the built-in C++ test suite directly
./Launch.exe --test

# Export all raw benchmark CSVs into results/
./Launch.exe --export-all

# Specify custom port
./Launch.exe --port=8080
```

---

## 5. VS Code & Antigravity Editor Configuration

Open the extracted folder in **VS Code** or **Antigravity IDE**:
- Select **Run and Debug** (`Ctrl+Shift+D`).
- Choose **"Launch Workbench (Packaged Executable)"** and press `F5` to start in one action.
- Choose **"Run Workbench Test Suite"** to debug or execute the automated verification suite.
- Recommended Extension: `ms-vscode.cpptools` (C/C++ extension pack).

---

## 6. Directory Structure

```
CandidateID_Workbench/
├── Launch.exe                  # Standalone Windows 11 executable (Zero-install launcher)
├── CMakeLists.txt              # Standard CMake source build file
├── README.md                   # This document
├── .vscode/                    # VS Code / Antigravity run and build configurations
│   ├── launch.json
│   └── tasks.json
├── data/                       # Datasets with full provenance and schemas
│   ├── cad/                    # bracket_ascii.stl & PROVENANCE.md
│   └── ansys/                  # synthetic_rod_ansys.csv & PROVENANCE.md
├── src/
│   ├── backend/                # Pure C++17 numerical & transport core
│   │   ├── include/            # Header interfaces (FEA, CAD, Multicore, Mocks, Backprop)
│   │   ├── fea/                # 1D thermal FEA rod solver & Thomas algorithm
│   │   ├── parsers/            # Robust ASCII STL & ANSYS CSV parsers
│   │   ├── multicore/          # Bounded thread pool & scaling benchmark engine
│   │   ├── embedded/           # MCU, FPGA, and JTAG mock adapters
│   │   ├── backprop/           # Manual chain-rule & central finite difference checker
│   │   ├── server/             # Embedded WinSock2 HTTP REST server
│   │   └── main.cpp            # Main entry point & CLI controller
│   └── frontend/               # Modern Dark-Mode R&D Console
│       ├── index.html
│       ├── css/style.css
│       ├── js/app.js
│       └── vendor/             # Offline vendored Three.js & Chart.js
├── tests/                      # Automated unit test suite & test runner
│   ├── test_all.cpp
│   └── fixtures/               # Malformed STL/CSV test files (NaN, missing cols, empty)
├── results/                    # Exported raw CSV results & publication plots
│   ├── fea_mesh_convergence.csv
│   ├── fea_solution_N80.csv
│   ├── multicore_benchmark.csv
│   ├── backprop_training_loss.csv
│   └── plots/                  # High-res 300 DPI PNG plots
└── docs/                       # Technical reports & handwritten derivations
    ├── REPORT.md               # 6-page technical report
    ├── AI_USE_LOG.md           # Disclosed AI assistance & critical review log
    ├── embedded_block_diagram.svg
    ├── handwritten_backprop_p1.png # Task F handwritten derivation (Page 1)
    └── handwritten_backprop_p2.png # Task F handwritten derivation (Page 2)
```

---

## 7. Known Limitations

1. **Dimensionality**: The FEA solver implements 1D longitudinal steady-state conduction; 2D/3D thermal Navier-Stokes conjugate heat transfer is outside the prototype scope.
2. **CAD Solids**: Native binary CAD solids (e.g., STEP, IGES, SLDPRT) and binary STL files are intentionally rejected per Task B specifications; models must be exported as ASCII STL.
3. **Hardware Emulation**: Microcontroller, FPGA, and JTAG adapters run through validated C++ software mocks rather than physical USB-attached silicon boards.
