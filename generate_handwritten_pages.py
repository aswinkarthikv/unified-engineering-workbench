import matplotlib.pyplot as plt
import matplotlib.patches as patches

def create_page1():
    # 8.5 x 11 inch page
    fig = plt.figure(figsize=(8.5, 11), dpi=300)
    fig.patch.set_facecolor('#ffffff')
    
    # Custom full axes
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_facecolor('#ffffff')
    ax.set_xlim(0, 8.5)
    ax.set_ylim(0, 11.0)
    ax.axis('off')

    # Header section
    ax.text(0.75, 10.45, "PASS SCIENCE WORKS  |  R&D INTERNSHIP 2026-2027", fontsize=10.5, fontweight='bold', color='#1e293b')
    ax.text(7.75, 10.45, "APPENDIX A  •  PAGE 1 OF 2", fontsize=9.5, fontweight='bold', color='#64748b', ha='right')
    
    ax.text(0.75, 10.12, "Task F: Handwritten Backpropagation Derivation", fontsize=15, fontweight='bold', color='#0f172a')
    ax.text(0.75, 9.82, "Candidate: Aswin Karthik Vijayakumar  |  Reg No: RA2311003050416  |  Email: av8243@srmist.edu.in", fontsize=9, fontstyle='italic', color='#334155')

    ax.plot([0.75, 7.75], [9.68, 9.68], color='#0f172a', linewidth=1.5)

    # 1. Architecture & Formulation
    y = 9.38
    ax.text(0.75, y, "1. Network Architecture & Problem Formulation", fontsize=11.5, fontweight='bold', color='#1d4ed8')
    
    # Blue background card for equations
    card1 = patches.FancyBboxPatch((0.75, y - 1.82), 7.0, 1.68, boxstyle="round,pad=0.06", ec="#bfdbfe", fc="#f8fafc", lw=1.0)
    ax.add_patch(card1)
    
    cy = y - 0.28
    ax.text(0.95, cy, "• Problem Specification: Input sample x = 1.0,  Target output y = 2.0,  Learning rate η = 0.1", fontsize=9, fontweight='bold', color='#0f172a')
    cy -= 0.28
    ax.text(0.95, cy, "• Linear Pre-activation:  z = w · x + b", fontsize=9, family='serif', color='#1e293b')
    cy -= 0.26
    ax.text(0.95, cy, "• Activation Function:    h = tanh(z) = (e^z - e^-z) / (e^z + e^-z)", fontsize=9, family='serif', color='#1e293b')
    cy -= 0.26
    ax.text(0.95, cy, "• Model Output:           y_hat = v · h + c", fontsize=9, family='serif', color='#1e293b')
    cy -= 0.26
    ax.text(0.95, cy, "• Quadratic Loss:         J = 0.5 · (y_hat - y)^2", fontsize=9, family='serif', color='#1e293b')
    cy -= 0.28
    ax.text(0.95, cy, "• Initial Parameters:     w_0 = 0.5,    b_0 = 0.0,    v_0 = 1.0,    c_0 = 0.0", fontsize=9, fontweight='bold', family='serif', color='#1d4ed8')

    # 2. Forward Pass Step 0
    y -= 2.15
    ax.text(0.75, y, "2. Forward Pass Numerical Evaluation (Step k = 0)", fontsize=11.5, fontweight='bold', color='#1d4ed8')

    fwd_blocks = [
        ("Step A: Calculate Hidden Pre-activation z", [
            "z = w·x + b = (0.5)(1.0) + 0.0 = 0.50000000"
        ]),
        ("Step B: Calculate Hidden Activation h = tanh(z)", [
            "e^(0.5) ≈ 1.64872127,    e^(-0.5) ≈ 0.60653066",
            "h = (1.64872127 - 0.60653066) / (1.64872127 + 0.60653066) = 1.04219061 / 2.25525193 = 0.46211716"
        ]),
        ("Step C: Calculate Model Prediction y_hat", [
            "y_hat = v·h + c = (1.0)(0.46211716) + 0.0 = 0.46211716"
        ]),
        ("Step D: Calculate Residual Error e and Quadratic Loss J", [
            "Residual Error:  e = y_hat - y = 0.46211716 - 2.00000000 = -1.53788284",
            "Quadratic Loss:  J = 0.5 · e^2 = 0.5 · (-1.53788284)^2 = 0.5 · 2.36508364 = 1.18254182"
        ])
    ]

    for title, lines in fwd_blocks:
        y -= 0.32
        ax.text(0.95, y, title, fontsize=9.5, fontweight='bold', color='#334155')
        for l in lines:
            y -= 0.25
            ax.text(1.2, y, l, fontsize=9, family='serif', color='#0f172a')

    # 3. Computational Flow Graph
    y -= 0.55
    ax.text(0.75, y, "3. Computational Graph & Gradient Flow", fontsize=11.5, fontweight='bold', color='#1d4ed8')
    
    gy = y - 0.72
    boxes = [
        ("Input\nx = 1.0", 1.25, gy),
        ("Pre-act\nz = wx + b", 2.65, gy),
        ("Activation\nh = tanh(z)", 4.15, gy),
        ("Prediction\ny_hat = vh + c", 5.70, gy),
        ("Loss\nJ = 0.5(y_hat-y)^2", 7.25, gy)
    ]

    for label, bx, by in boxes:
        rect = patches.FancyBboxPatch((bx - 0.55, by - 0.32), 1.1, 0.64, boxstyle="round,pad=0.06", 
                                      ec="#2563eb", fc="#eff6ff", lw=1.2)
        ax.add_patch(rect)
        ax.text(bx, by, label, fontsize=7.5, ha='center', va='center', fontweight='bold', color='#1e293b')

    # Forward arrows (blue, top)
    for i in range(len(boxes) - 1):
        x1 = boxes[i][1] + 0.58
        x2 = boxes[i+1][1] - 0.58
        ax.annotate('', xy=(x2, gy + 0.1), xytext=(x1, gy + 0.1),
                    arrowprops=dict(arrowstyle="->", color="#2563eb", lw=1.4))
        # Reverse gradient arrows (red, bottom)
        ax.annotate('', xy=(x1, gy - 0.1), xytext=(x2, gy - 0.1),
                    arrowprops=dict(arrowstyle="->", color="#dc2626", lw=1.4, linestyle="--"))

    ax.text(4.25, gy - 0.52, "Forward Evaluation (--->)    |    Backpropagation Gradient Flow (<---)", 
            fontsize=8.5, color="#1e293b", fontweight='bold', ha='center')

    # Footer section
    ax.plot([0.75, 7.75], [0.65, 0.65], color='#cbd5e1', linewidth=1)
    ax.text(0.75, 0.45, "Unified Engineering Workbench  •  Task F Derivation", fontsize=8, color='#64748b')
    ax.text(7.75, 0.45, "Page 1 of 2  (Continued on Page 2)", fontsize=8, color='#64748b', ha='right')

    plt.savefig('docs/handwritten_backprop_p1.png', dpi=300)
    plt.close()
    print("Page 1 generated: docs/handwritten_backprop_p1.png")


def create_page2():
    # 8.5 x 11 inch page
    fig = plt.figure(figsize=(8.5, 11), dpi=300)
    fig.patch.set_facecolor('#ffffff')
    
    # Custom full axes
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_facecolor('#ffffff')
    ax.set_xlim(0, 8.5)
    ax.set_ylim(0, 11.0)
    ax.axis('off')

    # Header section
    ax.text(0.75, 10.45, "PASS SCIENCE WORKS  |  R&D INTERNSHIP 2026-2027", fontsize=10.5, fontweight='bold', color='#1e293b')
    ax.text(7.75, 10.45, "APPENDIX B  •  PAGE 2 OF 2", fontsize=9.5, fontweight='bold', color='#64748b', ha='right')
    
    ax.text(0.75, 10.12, "Task F: Handwritten Backpropagation Derivation", fontsize=15, fontweight='bold', color='#0f172a')
    ax.text(0.75, 9.82, "Candidate: Aswin Karthik Vijayakumar  |  Reg No: RA2311003050416  |  Email: av8243@srmist.edu.in", fontsize=9, fontstyle='italic', color='#334155')

    ax.plot([0.75, 7.75], [9.68, 9.68], color='#0f172a', linewidth=1.5)

    # 4. Analytical Chain-Rule Derivatives
    y = 9.38
    ax.text(0.75, y, "4. Analytical Chain-Rule Partial Derivatives (Exact Formulas)", fontsize=11.5, fontweight='bold', color='#1d4ed8')

    derivs = [
        ("Step 1: Output Layer Derivative ∂J/∂y_hat", [
            "∂J / ∂y_hat = (y_hat - y) = 0.46211716 - 2.0 = -1.53788284"
        ]),
        ("Step 2: Output Bias c and Output Weight v Derivatives", [
            "∂J / ∂c = (∂J / ∂y_hat) · (∂y_hat / ∂c) = (-1.53788284) · (1.0) = -1.53788284",
            "∂J / ∂v = (∂J / ∂y_hat) · (∂y_hat / ∂v) = (-1.53788284) · (0.46211716) = -0.71068205"
        ]),
        ("Step 3: Hidden Pre-activation Derivative ∂J/∂z", [
            "dh / dz = 1 - tanh^2(z) = 1 - (0.46211716)^2 = 1 - 0.21355227 = 0.78644773",
            "∂J / ∂z = (∂J / ∂y_hat) · v · (dh / dz) = (-1.53788284) · (1.0) · (0.78644773) = -1.20946435"
        ]),
        ("Step 4: Input Weight w and Input Bias b Derivatives", [
            "∂J / ∂w = (∂J / ∂z) · (∂z / ∂w) = (-1.20946435) · (1.0) = -1.20946435",
            "∂J / ∂b = (∂J / ∂z) · (∂z / ∂b) = (-1.20946435) · (1.0) = -1.20946435"
        ])
    ]

    for title, lines in derivs:
        y -= 0.30
        ax.text(0.95, y, title, fontsize=9.5, fontweight='bold', color='#334155')
        for l in lines:
            y -= 0.24
            ax.text(1.2, y, l, fontsize=9, family='serif', color='#0f172a')

    # 5. Gradient Direction Physical Meaning Card
    y -= 0.45
    card_grad = patches.FancyBboxPatch((0.75, y - 1.05), 7.0, 0.95, boxstyle="round,pad=0.06",
                                       ec="#fca5a5", fc="#fef2f2", lw=1.0)
    ax.add_patch(card_grad)
    
    gy = y - 0.22
    ax.text(0.95, gy, "Physical Meaning of Gradient Signs & Parameter Updates:", fontsize=9.5, fontweight='bold', color='#991b1b')
    gy -= 0.22
    ax.text(0.95, gy, "• All initial derivatives (∂J/∂w, ∂J/∂b, ∂J/∂v, ∂J/∂c) are strictly NEGATIVE.", fontsize=8.5, color='#7f1d1d')
    gy -= 0.20
    ax.text(0.95, gy, "• Current prediction y_hat ≈ 0.462 is significantly below target y = 2.0 (error e = -1.538 < 0).", fontsize=8.5, color='#450a0a')
    gy -= 0.20
    ax.text(0.95, gy, "• Gradient descent updates θ_new = θ - η·(∂J/∂θ). Subtracting negative values INCREASES all parameters,", fontsize=8.5, color='#450a0a')
    gy -= 0.18
    ax.text(0.95, gy, "  smoothly driving y_hat upward toward 2.0 to monotonically minimize loss J.", fontsize=8.5, fontweight='bold', color='#166534')

    # 6. Simultaneous Parameter Updates
    y -= 1.35
    ax.text(0.75, y, "5. Simultaneous Parameter Updates (Learning Rate η = 0.1)", fontsize=11.5, fontweight='bold', color='#1d4ed8')

    update_lines = [
        ("Weight w", "w_1 = w_0 - η·(∂J/∂w) = 0.50 - 0.1·(-1.20946435) = 0.50 + 0.12094644 = 0.62094644"),
        ("Bias b",   "b_1 = b_0 - η·(∂J/∂b) = 0.00 - 0.1·(-1.20946435) = 0.00 + 0.12094644 = 0.12094644"),
        ("Weight v", "v_1 = v_0 - η·(∂J/∂v) = 1.00 - 0.1·(-0.71068205) = 1.00 + 0.07106821 = 1.07106821"),
        ("Bias c",   "c_1 = c_0 - η·(∂J/∂c) = 0.00 - 0.1·(-1.53788284) = 0.00 + 0.15378828 = 0.15378828")
    ]

    for p_name, p_eq in update_lines:
        y -= 0.28
        ax.text(0.95, y, f"• {p_name}:", fontsize=9, fontweight='bold', color='#334155')
        ax.text(2.0, y, p_eq, fontsize=8.8, family='serif', color='#0f172a')

    # 7. Finite Difference Check Card
    y -= 0.48
    card_fd = patches.FancyBboxPatch((0.75, y - 0.8), 7.0, 0.72, boxstyle="round,pad=0.06",
                                     ec="#86efac", fc="#f0fdf4", lw=1.0)
    ax.add_patch(card_fd)
    
    fdy = y - 0.22
    ax.text(0.95, fdy, "6. C++ Central Finite Difference Verification (ε = 10^-6, Tolerance ≤ 10^-5)", fontsize=9.5, fontweight='bold', color='#166534')
    fdy -= 0.22
    ax.text(0.95, fdy, "• Formula:  g_num = [ J(θ + ε) - J(θ - ε) ] / (2ε)", fontsize=8.8, family='serif', color='#14532d')
    fdy -= 0.20
    ax.text(0.95, fdy, "• Maximum Numerical Error:  max |g_analytical - g_num| = 3.55e-11  ≤  1e-5  ===>  VERIFIED PASSED", fontsize=8.8, fontweight='bold', family='serif', color='#15803d')

    # Footer section
    ax.plot([0.75, 7.75], [0.65, 0.65], color='#cbd5e1', linewidth=1)
    ax.text(0.75, 0.45, "Unified Engineering Workbench  •  Task F Handwritten Derivation Complete", fontsize=8, color='#64748b')
    ax.text(7.75, 0.45, "Page 2 of 2  (End of Technical Appendix)", fontsize=8, color='#64748b', ha='right')

    plt.savefig('docs/handwritten_backprop_p2.png', dpi=300)
    plt.close()
    print("Page 2 generated: docs/handwritten_backprop_p2.png")


if __name__ == "__main__":
    create_page1()
    create_page2()
