# ANSYS Nodal Temperature Results Provenance & Schema

## 1. Classification & Labeling
> [!IMPORTANT]
> **SYNTHETIC DATA DISCLOSURE**: In strict adherence to Task B instructions, this dataset is explicitly labeled **SYNTHETIC** and is NOT represented as an actual commercial ANSYS export.

- **File Name**: `synthetic_rod_ansys.csv`
- **Format**: Comma-Separated Values (CSV)
- **Node Count**: 525 nodes ($21 \times 5 \times 5$ spatial lattice)
- **Bounding Box**:
  - $X$: $[0.000000, 1.000000]\text{ m}$
  - $Y$: $[-0.010000, 0.010000]\text{ m}$
  - $Z$: $[-0.010000, 0.010000]\text{ m}$
- **Temperature Range**:
  - Minimum: $14.95^\circ\text{C}$
  - Maximum: $187.90^\circ\text{C}$
  - Mean: $120.84^\circ\text{C}$

## 2. Mathematical Generation Model
The synthetic nodal temperature values simulate steady-state 3D heat conduction through a uniform square prism rod with internal volumetric generation ($q = 10{,}000\text{ W/m}^3$), thermal conductivity ($k = 10\text{ W/(m}\cdot\text{K)}$), Dirichlet boundary conditions at endpoints ($T(0) = 100^\circ\text{C}$, $T(1) = 20^\circ\text{C}$), and mild transverse radiative/convective surface heat loss:

$$T(x, y, z) = 100 - 80 x + \left(\frac{q}{2k}\right) x (1 - x) - 2.5 \left(\frac{y^2 + z^2}{r_0^2}\right) + 0.1 \sin(12 x)$$

Where $r_0 = 0.01\text{ m}$.

## 3. CSV Schema
| Column Name | Type | Unit | Description |
|---|---|---|---|
| `node_id` | Integer (64-bit) | dimensionless | Unique positive nodal identifier |
| `x_m` | Float (Double) | meters ($\text{m}$) | Longitudinal rod position |
| `y_m` | Float (Double) | meters ($\text{m}$) | Transverse horizontal offset |
| `z_m` | Float (Double) | meters ($\text{m}$) | Transverse vertical offset |
| `temperature_C` | Float (Double) | Celsius ($^\circ\text{C}$) | Nodal steady-state temperature |

## 4. Parser Validation & Limits
- Tested with datasets up to 100 MB (~500,000 nodes).
- Robust handling of leading comment lines (`#` or `//`).
- Strict rejection of missing columns, non-finite values (NaN / Inf), and non-numeric strings with actionable repair hints.
