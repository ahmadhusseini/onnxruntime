#include "core/providers/cpu/ml/imputer.h"
#include <cmath>
/**
https://github.com/onnx/onnx/blob/master/onnx/defs/traditionalml/defs.cc
ONNX_OPERATOR_SCHEMA(Imputer)
.SetDomain("ai.onnx.ml")
.SetDoc(R"DOC(
Replace imputs that equal replaceValue/s  with  imputeValue/s.
All other inputs are copied to the output unchanged.
This op is used to replace missing values where we know what a missing value looks like.
Only one of imputed_value_floats or imputed_value_int64s should be used.
The size can be 1 element, which will be reused, or the size of the feature set F in input N,F
)DOC")
.Input(0, "X", "Data to be imputed", "T")
.Output(0, "Y", "Imputed output data", "T")
.TypeConstraint(
"T",
{"tensor(float)", "tensor(double)", "tensor(int64)", "tensor(int32)"},
" allowed types.")
.Attr(
"imputed_value_floats",
"value to change to",
AttributeProto::FLOATS,
OPTIONAL)
.Attr(
"replaced_value_float",
"value that needs replacing",
AttributeProto::FLOAT,
0.f)
.Attr(
"imputed_value_int64s",
"value to change to",
AttributeProto::INTS,
OPTIONAL)
.Attr(
"replaced_value_int64",
"value that needs replacing",
AttributeProto::INT,
static_cast<int64_t>(0));
*/
using namespace Lotus::Common;

namespace Lotus {
namespace ML {

ONNX_CPU_OPERATOR_ML_KERNEL(
    Imputer,
    1,
    KernelDefBuilder().TypeConstraint("T", {DataTypeImpl::GetTensorType<float>(),
                                            DataTypeImpl::GetTensorType<int64_t>()}),
    ImputerOp);

ImputerOp::ImputerOp(const OpKernelInfo& info) : OpKernel(info) {
  info.GetAttrs<float>("imputed_value_floats", imputed_values_float_);
  info.GetAttr<float>("replaced_value_float", &replaced_value_float_);
  info.GetAttrs<int64_t>("imputed_value_int64s", imputed_values_int64_);
  info.GetAttr<int64_t>("replaced_value_int64", &replaced_value_int64_);
  LOTUS_ENFORCE(imputed_values_float_.empty() ^ imputed_values_int64_.empty(),
                "Must provide imputed_values_float_ or imputed_values_int64_ but not both.");
}

template <typename T>
Common::Status ComputeByType(OpKernelContext* context,
                             T replaced_value,
                             const std::vector<T>& imputed_values) {
  if (imputed_values.empty()) {
    return Status(LOTUS, FAIL, "Empty value of imputed values.");
  }

  const Tensor& X = *context->Input<Tensor>(0);
  const TensorShape& x_shape = X.Shape();
  auto& dims = x_shape.GetDims();
  if (dims.empty()) {
    return Status(LOTUS, FAIL, "Empty input dimensions.");
  }

  const T* x_data = X.Data<T>();
  size_t x_size = x_shape.Size();
  int64_t stride = dims.size() == 1 ? dims[0] : dims[1];

  Tensor* Y = context->Output(0, x_shape);
  T* y_data = Y->MutableData<T>();
  if (imputed_values.size() == static_cast<size_t>(stride)) {
    for (size_t i = 0; i < x_size; i++) {
      if (std::isnan(static_cast<float>(x_data[i])) && std::isnan(static_cast<float>(replaced_value))) {
        y_data[i] = imputed_values[i % stride];
      } else if (x_data[i] == replaced_value) {
        y_data[i] = imputed_values[i % stride];
      } else {
        y_data[i] = x_data[i];
      }
    }
  } else {
    for (size_t i = 0; i < x_size; i++) {
      if (std::isnan(static_cast<float>(x_data[i])) && std::isnan(static_cast<float>(replaced_value))) {
        y_data[i] = imputed_values[0];
      } else if (x_data[i] == replaced_value) {
        y_data[i] = imputed_values[0];
      } else {
        y_data[i] = x_data[i];
      }
    }
  }

  return Status::OK();
}

Common::Status ImputerOp::Compute(OpKernelContext* context) const {
  auto input_type = context->Input<Tensor>(0)->DataType();
  if (input_type == DataTypeImpl::GetType<float>()) {
    return ComputeByType<float>(context, replaced_value_float_, imputed_values_float_);
  } else if (input_type == DataTypeImpl::GetType<int64_t>()) {
    return ComputeByType<int64_t>(context, replaced_value_int64_, imputed_values_int64_);
  } else {
    return Status(LOTUS, INVALID_ARGUMENT, "Invalid type");
  }
}
}  // namespace ML
}  // namespace Lotus
