#ifndef WORKBENCH_BACKPROP_ENGINE_HPP
#define WORKBENCH_BACKPROP_ENGINE_HPP

#include <vector>
#include <string>
#include <cmath>

namespace workbench::backprop {

struct ModelParameters {
    double w = 0.5;
    double b = 0.0;
    double v = 1.0;
    double c = 0.0;
};

struct Gradients {
    double dJ_dw = 0.0;
    double dJ_db = 0.0;
    double dJ_dv = 0.0;
    double dJ_dc = 0.0;
};

struct ForwardPassResult {
    double x;
    double y_target;
    double z;           // w * x + b
    double h;           // tanh(z)
    double y_hat;       // v * h + c
    double error;       // y_hat - y_target
    double loss;        // 0.5 * error^2
};

struct GradientCheckResult {
    std::string param_name;
    double analytical_grad;
    double numerical_grad;
    double abs_error;
    bool passed;
};

struct TrainingStepRecord {
    int step;
    ModelParameters params;
    ForwardPassResult fwd;
    Gradients grads;
};

class BackpropEngine {
public:
    static constexpr double DEFAULT_EPSILON = 1.0e-6;
    static constexpr double GRADIENT_CHECK_TOLERANCE = 1.0e-5;

    static double compute_loss(const ModelParameters& p, double x, double y_target);

    static ForwardPassResult forward(const ModelParameters& p, double x, double y_target);

    static Gradients compute_gradients(const ModelParameters& p, const ForwardPassResult& fwd);

    static Gradients compute_numerical_gradients(
        const ModelParameters& p, double x, double y_target, double epsilon = DEFAULT_EPSILON);

    static std::vector<GradientCheckResult> run_gradient_check(
        const ModelParameters& p, double x, double y_target, double epsilon = DEFAULT_EPSILON);

    static std::vector<TrainingStepRecord> train(
        ModelParameters initial_params, 
        double x = 1.0, 
        double y_target = 2.0, 
        double learning_rate = 0.1, 
        int steps = 100);

    static std::string export_training_csv(const std::vector<TrainingStepRecord>& history);
    static std::string to_json(
        const ForwardPassResult& fwd_step0,
        const Gradients& grads_step0,
        const ModelParameters& updated_step1,
        const std::vector<GradientCheckResult>& checks,
        const std::vector<TrainingStepRecord>& history);
};

} // namespace workbench::backprop

#endif // WORKBENCH_BACKPROP_ENGINE_HPP
