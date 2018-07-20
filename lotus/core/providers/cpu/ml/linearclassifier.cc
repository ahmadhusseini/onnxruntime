#include "core/providers/cpu/ml/linearclassifier.h"

namespace Lotus {
namespace ML {

const std::vector<MLDataType> linearClassifierOutputConstraints{
    DataTypeImpl::GetTensorType<std::string>(),
    DataTypeImpl::GetTensorType<int64_t>()};

ONNX_CPU_OPERATOR_TYPED_ML_KERNEL(
    LinearClassifier,
    1,
	float,
    KernelDefBuilder().TypeConstraint("T1", DataTypeImpl::GetTensorType<float>())
                      .TypeConstraint("T2", linearClassifierOutputConstraints),
    LinearClassifier<float>);

ONNX_CPU_OPERATOR_TYPED_ML_KERNEL(
    LinearClassifier,
    1,
	double,
    KernelDefBuilder().TypeConstraint("T1", DataTypeImpl::GetTensorType<double>())
                      .TypeConstraint("T2", linearClassifierOutputConstraints),
    LinearClassifier<double>);

ONNX_CPU_OPERATOR_TYPED_ML_KERNEL(
    LinearClassifier,
    1,
    int64_t,
    KernelDefBuilder().TypeConstraint("T1", DataTypeImpl::GetTensorType<int64_t>())
	                  .TypeConstraint("T2", linearClassifierOutputConstraints),
    LinearClassifier<int64_t>);

ONNX_CPU_OPERATOR_TYPED_ML_KERNEL(
    LinearClassifier,
    1,
    int32_t,
    KernelDefBuilder().TypeConstraint("T1", DataTypeImpl::GetTensorType<int32_t>())
	                  .TypeConstraint("T2", linearClassifierOutputConstraints),
    LinearClassifier<int32_t>);

template <typename T>
LinearClassifier<T>::LinearClassifier(const OpKernelInfo& info) : OpKernel(info) {
  info.GetAttr<int64_t>("multi_class", &multi_class_);
  info.GetAttrs<float>("coefficients", coefficients_);
  info.GetAttrs<float>("intercepts", intercepts_);
  info.GetAttrs<std::string>("classlabels_strings", classlabels_strings_);
  info.GetAttrs<int64_t>("classlabels_ints", classlabels_ints_);
  LOTUS_ENFORCE(!coefficients_.empty());

  using_strings_ = !classlabels_strings_.empty();
  class_count_ = static_cast<int64_t>(intercepts_.size());

  std::string tmp = "NONE";
  info.GetAttr<std::string>("post_transform", &tmp);
  post_transform_ = MakeTransform(tmp);
}

template <typename T>
Status LinearClassifier<T>::Compute(OpKernelContext* ctx) const {
  const Tensor* X = ctx->Input<Tensor>(0);
  if (X->Shape().Size() == 0) {
    return Status(Common::LOTUS, Common::INVALID_ARGUMENT,
                  "Input shape needs to be at least a single dimension.");
  }

  int64_t stride = X->Shape().Size() == 1 ? X->Shape()[0] : X->Shape()[1];
  int64_t N = X->Shape().Size() == 1 ? 1 : X->Shape()[0];
  Tensor* Y = ctx->Output(0, TensorShape({N}));

  int64_t output_classes = class_count_;
  bool add_second_class = false;
  if (intercepts_.size() == 1 && ((using_strings_ && classlabels_strings_.size() == 2) || (!using_strings_ && classlabels_ints_.size() == 2))) {
    output_classes = 2;
    add_second_class = true;
  }
  std::vector<int64_t> dims = {N, output_classes};
  Tensor* Z = ctx->Output(1, TensorShape(dims));

  int64_t zindex = 0;
  const auto* x_data = X->Data<T>();

  size_t class_count = static_cast<size_t>(class_count_);
  std::vector<float> scores;
  scores.reserve(class_count);
  for (int64_t i = 0; i < N; i++)  //for each point
  {
    scores.clear();
    size_t current_weight_0 = i * stride;
    int maxclass = -1;
    float maxweight = 0.f;
    for (int j = 0; j < class_count; j++)  //for each class
    {
      size_t current_coeff_0 = j * stride;
      float weight = 0.f;
      for (int64_t k = 0; k < stride; k++)  //for each weight
      {
        weight += static_cast<float>(x_data[current_weight_0 + k] * coefficients_[current_coeff_0 + k]);
      }
      if (intercepts_.size() == class_count) {
        weight += intercepts_[j];
      }
      scores.push_back(weight);
      if (weight > maxweight || maxclass == -1) {
        maxweight = weight;
        maxclass = j;
      }
    }
    //write top class
    if (intercepts_.size() == 1)  //binary
    {
      if (using_strings_) {
        if (classlabels_strings_.size() == 2 && maxweight > 0) {
          Y->MutableData<std::string>()[i] = classlabels_strings_[1];  //positive label
        } else if (classlabels_strings_.size() == 2) {
          Y->MutableData<std::string>()[i] = classlabels_strings_[0];  //negative label
        } else if (maxweight > 0) {
          Y->MutableData<std::string>()[i] = "1";  //positive label
        } else {
          Y->MutableData<std::string>()[i] = "0";  //negative label
        }
      } else  //no strings
      {
        if (classlabels_ints_.size() == 2 && maxweight > 0) {
          Y->MutableData<int64_t>()[i] = classlabels_ints_[1];  //positive label
        } else if (classlabels_ints_.size() == 2) {
          Y->MutableData<int64_t>()[i] = classlabels_ints_[0];  //negative label
        } else if (maxweight > 0) {
          Y->MutableData<int64_t>()[i] = 1;  //positive label
        } else {
          Y->MutableData<int64_t>()[i] = 0;  //negative label
        }
      }
    } else  //multiclass
    {
      if (using_strings_) {
        Y->MutableData<std::string>()[i] = classlabels_strings_[maxclass];
      } else {
        Y->MutableData<int64_t>()[i] = classlabels_ints_[maxclass];
      }
    }
    //write float values
    if (add_second_class && maxweight > 0) {
      Lotus::ML::write_scores(scores, post_transform_, zindex, Z, 0);
    } else if (add_second_class) {
      Lotus::ML::write_scores(scores, post_transform_, zindex, Z, 1);
    } else {
      Lotus::ML::write_scores(scores, post_transform_, zindex, Z, -1);
    }
    zindex += scores.size();
  }  //for each point
  return Status::OK();
}

}  // namespace ML
}  // namespace Lotus
