// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "sample.h"
#include "onnx/defs/schema.h"

namespace onnxruntime {
namespace ml {
// These ops are internal-only, so register outside of onnx
ONNX_CPU_OPERATOR_TYPED_MS_KERNEL(
    SampleOp,
    1,
    float,
    KernelDefBuilder().TypeConstraint("T", DataTypeImpl::GetTensorType<float>()).MayInplace(0, 0),
    SampleOp<float>);
}  // namespace ml
}  // namespace onnxruntime
