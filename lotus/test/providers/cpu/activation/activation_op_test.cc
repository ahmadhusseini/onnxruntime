#include "core/providers/cpu/activation/activations.h"
#include "gtest/gtest.h"
#include "test/providers/provider_test_utils.h"

namespace Lotus {
namespace Test {

void TestUnaryElementwiseOp(const char* szOp, std::vector<float>& input_vals,
                            std::function<float(float)> expected_func,
                            const std::unordered_map<std::string, float> attribs = {}) {
  OpTester test(szOp);

  for (auto attr : attribs)
    test.AddAttribute(attr.first, attr.second);

  std::vector<int64_t> dims{(int64_t)input_vals.size()};

  std::vector<float> expected_vals;
  for (const auto& iv : input_vals)
    expected_vals.push_back(expected_func(iv));

  test.AddInput<float>("X", dims, input_vals);
  test.AddOutput<float>("Y", dims, expected_vals);
  test.Run();
}

std::vector<float> input_vals = {
    -1.0f, 0, 1.0f,                                              // normal input values for activation
    FLT_MIN, FLT_MIN / 10, -FLT_MIN / 10,                        // min, denorm, -denorm
    FLT_MAX, -FLT_MAX, std::numeric_limits<float>::infinity()};  // max, -max, inf

std::vector<float> no_inf_input_vals = {
    -1.0f, 0, 1.0f,                        // normal input values for activation
    FLT_MIN, FLT_MIN / 10, -FLT_MIN / 10,  // min, denorm, -denorm
    FLT_MAX, -FLT_MAX};                    // max, -max

std::vector<float> exp_safe_input_vals = {
    -1.0f, 0, 1.0f,                                   // normal input values for activation
    FLT_MIN, FLT_MIN / 10, -FLT_MIN / 10, -FLT_MAX};  // min, denorm, -denorm, -max

TEST(ActivationOpTest, Sigmoid) {
  TestUnaryElementwiseOp("Sigmoid",
                         input_vals,
                         [](float x) {
                           auto y = 1.f / (1.f + std::exp(-std::abs(x)));  // safe sigmoid
                           y = x > 0 ? y : 1 - y;
                           return y;
                         });
}

TEST(ActivationOpTest, HardSigmoid) {
  float alpha = 0.2f;
  float beta = 0.5f;
  TestUnaryElementwiseOp("HardSigmoid",
                         input_vals,
                         [alpha, beta](float x) {
                           return std::max(std::min((alpha * x + beta), 1.0f), 0.0f);
                         },
                         {{"alpha", alpha}, {"beta", beta}});
}

TEST(ActivationOpTest, Tanh) {
  TestUnaryElementwiseOp("Tanh",
                         input_vals,
                         [](float x) { return std::tanh(x); });
}

TEST(ActivationOpTest, Relu) {
  TestUnaryElementwiseOp("Relu",
                         input_vals,
                         [](float x) { return std::max(x, 0.0f); });
}

TEST(ActivationOpTest, Elu) {
  float alpha = 0.1f;
  TestUnaryElementwiseOp("Elu",
                         input_vals,
                         [alpha](float x) { return (x >= 0) ? x : alpha * (exp(x) - 1); },
                         {{"alpha", alpha}});
}

TEST(ActivationOpTest, LeakyRelu) {
  float alpha = 0.1f;
  TestUnaryElementwiseOp("LeakyRelu",
                         input_vals,
                         [alpha](float x) { return (x >= 0) ? x : alpha * x; },
                         {{"alpha", alpha}});
}

TEST(ActivationOpTest, ThresholdedRelu) {
  float alpha = 0.1f;
  TestUnaryElementwiseOp("ThresholdedRelu",
                         input_vals,
                         [alpha](float x) { return (x >= alpha) ? x : 0; },
                         {{"alpha", alpha}});
}

TEST(ActivationOpTest, ScaledTanh) {
  static constexpr float alpha = 2.0f;
  static constexpr float beta = 1.5f;

  TestUnaryElementwiseOp("ScaledTanh",
                         input_vals,
                         [](float x) { return alpha * tanh(beta * x); },
                         {{"alpha", alpha}, {"beta", beta}});
}

TEST(ActivationOpTest, Selu) {
  static constexpr float alpha = 1.6732f;
  static constexpr float gamma = 1.0507f;

  TestUnaryElementwiseOp("Selu",
                         input_vals,
                         [](float x) { return x <= 0 ? gamma * (alpha * exp(x) - alpha) : gamma * x; },
                         {{"alpha", alpha}, {"gamma", gamma}});
}

TEST(ActivationOpTest, Selu_Attributes) {
  static constexpr float alpha = 1.8f;
  static constexpr float gamma = 0.5f;

  TestUnaryElementwiseOp("Selu",
                         input_vals,
                         [](float x) { return x <= 0 ? gamma * (alpha * exp(x) - alpha) : gamma * x; },
                         {{"alpha", alpha}, {"gamma", gamma}});
}

TEST(ActivationOpTest, PRelu) {
  OpTester test("PRelu");

  auto formula = [](float x, float slope) { return x < 0 ? slope * x : x; };

  std::vector<float> inputs{1.0f, -4.0f, 0.0f, -9.0f};
  std::vector<float> slopes{1.0f, -2.0f, 3.0f, -4.0f};
  std::vector<float> outputs;
  for (unsigned i = 0; i < inputs.size(); i++)
    outputs.push_back(formula(inputs[i], slopes[i]));

  std::vector<int64_t> dims{2, 2};
  test.AddInput<float>("X", dims, inputs);
  test.AddInput<float>("slope", dims, slopes);
  test.AddOutput<float>("Y", dims, outputs);
  test.Run();
}

TEST(ActivationOpTest, PRelu_SingleSlope) {
  OpTester test("PRelu");

  auto formula = [](float x, float slope) { return x < 0 ? slope * x : x; };

  auto inputs = {1.0f, -4.0f, 0.0f, -9.0f};
  auto slope = 1.5f;
  std::vector<float> outputs;
  for (auto& input : inputs)
    outputs.push_back(formula(input, slope));

  std::vector<int64_t> dims{2, 2};
  test.AddInput<float>("X", dims, inputs);
  test.AddInput<float>("slope", {}, {slope});
  test.AddOutput<float>("Y", dims, outputs);
  test.Run();
}

TEST(ActivationOpTest, ParametricSoftplus) {
  static constexpr float alpha = 2.0f;
  static constexpr float beta = 1.5f;

  TestUnaryElementwiseOp("ParametricSoftplus",
                         exp_safe_input_vals,
                         [](float x) { return alpha * log(exp(beta * x) + 1); },
                         {{"alpha", alpha}, {"beta", beta}});
}

TEST(ActivationOpTest, Softplus) {
  TestUnaryElementwiseOp("Softplus",
                         exp_safe_input_vals,
                         [](float x) { return log(exp(x) + 1); });
}

TEST(ActivationOpTest, Softsign) {
  TestUnaryElementwiseOp("Softsign",
                         no_inf_input_vals,
                         [](float x) { return x / (1 + std::abs(x)); });
}

}  // namespace Test
}  // namespace Lotus
