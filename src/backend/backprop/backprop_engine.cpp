#include "backprop_engine.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace workbench::backprop {

double BackpropEngine::compute_loss(const ModelParameters& p, double x, double y_target) {
    double z = p.w * x + p.b;
    double h = std::tanh(z);
    double y_hat = p.v * h + p.c;
    double err = y_hat - y_target;
    return 0.5 * err * err;
}

ForwardPassResult BackpropEngine::forward(const ModelParameters& p, double x, double y_target) {
    ForwardPassResult res;
    res.x = x;
    res.y_target = y_target;
    res.z = p.w * x + p.b;
    res.h = std::tanh(res.z);
    res.y_hat = p.v * res.h + p.c;
    res.error = res.y_hat - y_target;
    res.loss = 0.5 * res.error * res.error;
    return res;
}

Gradients BackpropEngine::compute_gradients(const ModelParameters& p, const ForwardPassResult& fwd) {
    Gradients g;
    // dJ/dy_hat = y_hat - y
    double dJ_dyhat = fwd.error;

    // Output layer gradients: y_hat = v * h + c
    g.dJ_dc = dJ_dyhat * 1.0;
    g.dJ_dv = dJ_dyhat * fwd.h;

    // Backprop to hidden activation:
    // dJ/dh = dJ/dy_hat * dy_hat/dh = (y_hat - y) * v
    double dJ_dh = dJ_dyhat * p.v;

    // dh/dz = 1 - tanh^2(z) = 1 - h^2
    double dh_dz = 1.0 - (fwd.h * fwd.h);
    double dJ_dz = dJ_dh * dh_dz;

    // Input layer gradients: z = w * x + b
    g.dJ_dw = dJ_dz * fwd.x;
    g.dJ_db = dJ_dz * 1.0;

    return g;
}

Gradients BackpropEngine::compute_numerical_gradients(
    const ModelParameters& p, double x, double y_target, double epsilon) 
{
    Gradients num_g;

    // w
    ModelParameters pw_plus = p, pw_minus = p;
    pw_plus.w += epsilon; pw_minus.w -= epsilon;
    num_g.dJ_dw = (compute_loss(pw_plus, x, y_target) - compute_loss(pw_minus, x, y_target)) / (2.0 * epsilon);

    // b
    ModelParameters pb_plus = p, pb_minus = p;
    pb_plus.b += epsilon; pb_minus.b -= epsilon;
    num_g.dJ_db = (compute_loss(pb_plus, x, y_target) - compute_loss(pb_minus, x, y_target)) / (2.0 * epsilon);

    // v
    ModelParameters pv_plus = p, pv_minus = p;
    pv_plus.v += epsilon; pv_minus.v -= epsilon;
    num_g.dJ_dv = (compute_loss(pv_plus, x, y_target) - compute_loss(pv_minus, x, y_target)) / (2.0 * epsilon);

    // c
    ModelParameters pc_plus = p, pc_minus = p;
    pc_plus.c += epsilon; pc_minus.c -= epsilon;
    num_g.dJ_dc = (compute_loss(pc_plus, x, y_target) - compute_loss(pc_minus, x, y_target)) / (2.0 * epsilon);

    return num_g;
}

std::vector<GradientCheckResult> BackpropEngine::run_gradient_check(
    const ModelParameters& p, double x, double y_target, double epsilon) 
{
    auto fwd = forward(p, x, y_target);
    auto analytical = compute_gradients(p, fwd);
    auto numerical = compute_numerical_gradients(p, x, y_target, epsilon);

    std::vector<GradientCheckResult> results;

    auto check_param = [&](const std::string& name, double ana, double num) {
        double diff = std::abs(ana - num);
        results.push_back({name, ana, num, diff, diff <= GRADIENT_CHECK_TOLERANCE});
    };

    check_param("w", analytical.dJ_dw, numerical.dJ_dw);
    check_param("b", analytical.dJ_db, numerical.dJ_db);
    check_param("v", analytical.dJ_dv, numerical.dJ_dv);
    check_param("c", analytical.dJ_dc, numerical.dJ_dc);

    return results;
}

std::vector<TrainingStepRecord> BackpropEngine::train(
    ModelParameters params, 
    double x, 
    double y_target, 
    double learning_rate, 
    int steps) 
{
    std::vector<TrainingStepRecord> history;
    history.reserve(steps + 1);

    for (int step = 0; step <= steps; ++step) {
        auto fwd = forward(params, x, y_target);
        auto grads = compute_gradients(params, fwd);

        history.push_back({step, params, fwd, grads});

        if (step < steps) {
            // Simultaneous gradient descent update using step 0 gradients
            params.w -= learning_rate * grads.dJ_dw;
            params.b -= learning_rate * grads.dJ_db;
            params.v -= learning_rate * grads.dJ_dv;
            params.c -= learning_rate * grads.dJ_dc;
        }
    }

    return history;
}

std::string BackpropEngine::export_training_csv(const std::vector<TrainingStepRecord>& history) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "# Unified Engineering Workbench - Task F Backpropagation Training Trajectory\n";
    ss << "# Sample: x=1, target y=2, Model: y_hat = v*tanh(w*x + b) + c, loss = 0.5*(y_hat - y)^2\n";
    ss << "step,w,b,v,c,z,h,y_hat,error,loss_J,dJ_dw,dJ_db,dJ_dv,dJ_dc\n";

    for (const auto& r : history) {
        ss << r.step << "," << r.params.w << "," << r.params.b << "," << r.params.v << "," 
           << r.params.c << "," << r.fwd.z << "," << r.fwd.h << "," << r.fwd.y_hat << "," 
           << r.fwd.error << "," << r.fwd.loss << "," << r.grads.dJ_dw << "," << r.grads.dJ_db << "," 
           << r.grads.dJ_dv << "," << r.grads.dJ_dc << "\n";
    }

    return ss.str();
}

std::string BackpropEngine::to_json(
    const ForwardPassResult& fwd_step0,
    const Gradients& grads_step0,
    const ModelParameters& updated_step1,
    const std::vector<GradientCheckResult>& checks,
    const std::vector<TrainingStepRecord>& history) 
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(8);
    ss << "{\"step0\":{";
    ss << "\"z\":" << fwd_step0.z << ",\"h\":" << fwd_step0.h << ",\"y_hat\":" << fwd_step0.y_hat << ",";
    ss << "\"error\":" << fwd_step0.error << ",\"loss\":" << fwd_step0.loss << ",";
    ss << "\"dJ_dw\":" << grads_step0.dJ_dw << ",\"dJ_db\":" << grads_step0.dJ_db << ",";
    ss << "\"dJ_dv\":" << grads_step0.dJ_dv << ",\"dJ_dc\":" << grads_step0.dJ_dc << "},";

    ss << "\"step1_params\":{";
    ss << "\"w\":" << updated_step1.w << ",\"b\":" << updated_step1.b << ",";
    ss << "\"v\":" << updated_step1.v << ",\"c\":" << updated_step1.c << "},";

    ss << "\"checks\":[";
    for (size_t i = 0; i < checks.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& c = checks[i];
        ss << "{\"param\":\"" << c.param_name << "\",\"analytical\":" << c.analytical_grad 
           << ",\"numerical\":" << c.numerical_grad << ",\"abs_error\":" << std::scientific 
           << std::setprecision(3) << c.abs_error << std::fixed << std::setprecision(8) 
           << ",\"passed\":" << (c.passed ? "true" : "false") << "}";
    }
    ss << "],";

    ss << "\"history\":[";
    for (size_t i = 0; i < history.size(); ++i) {
        if (i > 0) ss << ",";
        const auto& h = history[i];
        ss << "{\"step\":" << h.step << ",\"loss\":" << h.fwd.loss << ",\"y_hat\":" << h.fwd.y_hat 
           << ",\"w\":" << h.params.w << ",\"b\":" << h.params.b << ",\"v\":" << h.params.v << ",\"c\":" << h.params.c << "}";
    }
    ss << "]}";

    return ss.str();
}

} // namespace workbench::backprop
