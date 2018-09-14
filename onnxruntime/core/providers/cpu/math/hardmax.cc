// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cpu/math/hardmax.h"
#include "core/util/math_cpuonly.h"
#include "core/util/math.h"

namespace onnxruntime {

template <>
Status Hardmax<float>::Compute(OpKernelContext* ctx) const {
  const Tensor* X = ctx->Input<Tensor>(0);
  const TensorShape& input_shape = X->Shape();
  const float* Xdata = X->Data<float>();

  size_t tmpN = input_shape.SizeToDimension(axis_);
  size_t tmpD = input_shape.SizeFromDimension(axis_);

  // Math::RowwiseMax expects int N and D.
  if (tmpN * tmpD > INT32_MAX || tmpN > INT32_MAX || tmpD > INT32_MAX) {
    std::ostringstream ss;
    ss << "Hardmax inputs N, D and N * D must be < " << INT32_MAX << ". N=" << tmpN << ", D=" << tmpD;
    std::string msg = ss.str();

    return Status(common::LOTUS, common::INVALID_ARGUMENT, msg);
  }

  const int N = gsl::narrow_cast<int>(tmpN);
  const int D = gsl::narrow_cast<int>(tmpD);

  std::vector<float> rowmax_(N);
  float* rowmax_data = rowmax_.data();
  Math::RowwiseMax<float, CPUMathUtil>(N, D, Xdata, rowmax_data, nullptr);

  Tensor* Y = ctx->Output(0, input_shape);
  float* Ydata = Y->MutableData<float>();
  Math::Set<float, CPUMathUtil>(input_shape.Size(), 0.f, Ydata, &CPUMathUtil::Instance());

  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < D; ++j) {
      if (Xdata[i * D + j] == rowmax_data[i]) {
        Ydata[i * D + j] = 1;
        break;
      }
    }
  }

  return Status::OK();
}

ONNX_CPU_OPERATOR_KERNEL(
    Hardmax,
    1,
    KernelDefBuilder().TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
    Hardmax<float>);

}  // namespace onnxruntime
