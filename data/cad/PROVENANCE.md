# CAD Geometry Provenance & Schema Documentation

## 1. File Details
- **File Name**: `bracket_ascii.stl`
- **Format**: ASCII StereoLithography (STL) format
- **Solid Identifier**: `L_Bracket_Mount`
- **Units**: Meters (m)
- **Facets**: 20 triangular facets
- **Bounding Box**:
  - $X$: $[0.000000, 0.060000]\text{ m}$ ($60\text{ mm}$)
  - $Y$: $[0.000000, 0.040000]\text{ m}$ ($40\text{ mm}$)
  - $Z$: $[0.000000, 0.050000]\text{ m}$ ($50\text{ mm}$)
- **Total Surface Area**: $0.0108\text{ m}^2$ ($10{,}800\text{ mm}^2$)

## 2. Origin & Provenance
- **Origin**: Created specifically for the Pass Science Works R&D Internship Assessment 2026-2027 by the candidate.
- **License**: Creative Commons Zero (CC0 1.0 Universal) / Public Domain.
- **Intended Purpose**: Demonstrates ASCII STL parsing, surface normal validation, 3D interactive orbit visualization, fit-to-view calculation, and wireframe rendering within the Unified Engineering Workbench.

## 3. Parsing & Validation Rules (C++ Core)
1. **ASCII Compliance**: Verifies the initial token is `solid` and confirms absence of binary control characters (null bytes). Rejects binary STL files with actionable guidance.
2. **Topology Checks**: Ensures each facet contains exactly one `outer loop` with three valid `vertex` lines and matching `endfacet`.
3. **Finite Numbers**: Checks that all coordinate and normal values satisfy `std::isfinite()`. Rejects NaN or Infinity with specific line references.
4. **Limits**: Recommended and tested up to 50 MB ASCII STL files (~200,000 facets).
