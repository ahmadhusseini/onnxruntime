#include "core/util/math.h"
#include "core/util/math_cpuonly.h"
#include "core/providers/cpu/math/matmul.h"

namespace Lotus {

template <typename T>
class MatMulComputeHelper {
 public:
  MatMulComputeHelper(const TensorShape& left_shape, const TensorShape& right_shape) {
    // Following numpy.matmul for shape inference:
    // https://docs.scipy.org/doc/numpy/reference/generated/numpy.matmul.html
    // The behavior depends on the arguments in the following way.
    // * If both arguments are 2 - D they are multiplied like conventional matrices.
    // * If either argument is N - D, N > 2, it is treated as a stack of matrices residing in the last two indexes and broadcast accordingly.
    // * If the first argument is 1 - D, it is promoted to a matrix by prepending a 1 to its dimensions.After matrix multiplication the prepended 1 is removed.
    // * If the second argument is 1 - D, it is promoted to a matrix by appending a 1 to its dimensions.After matrix multiplication the appended 1 is removed.

    size_t left_num_dims = left_shape.NumDimensions();
    size_t right_num_dims = right_shape.NumDimensions();
    LOTUS_ENFORCE(left_num_dims >= 1 && right_num_dims >= 1);
    bool has_1D_input = (left_num_dims == 1 || right_num_dims == 1);

    size_t num_input_dims = std::max(left_num_dims, right_num_dims);

    // use padded dims to compute matrix offsets, right 1D would be padded
    size_t num_dims_with_pad = num_input_dims + (right_num_dims == 1 ? 1 : 0);

    // output shape would squeeze the reduced 1D dimension
    size_t num_output_dims = num_input_dims - (has_1D_input ? 1 : 0);

    left_padded_dims_ = std::vector<int64_t>(num_dims_with_pad, 1);
    right_padded_dims_ = std::vector<int64_t>(num_dims_with_pad, 1);

    if (right_num_dims == 1) {
      // right padded to (1,...,K,1)
      right_padded_dims_[num_dims_with_pad - 2] = right_shape[0];

      if (num_input_dims >= 2) {
        // left padded to (...,1,K)
        left_shape.CopyDims(&left_padded_dims_[0], left_num_dims - 1);
        left_padded_dims_[num_dims_with_pad - 1] = left_shape[left_num_dims - 1];
      } else {
        // pad 1 in the front
        left_shape.CopyDims(&left_padded_dims_[num_dims_with_pad - left_num_dims], left_num_dims);
      }
    } else {
      // pad 1 in the front for left
      left_shape.CopyDims(&left_padded_dims_[num_dims_with_pad - left_num_dims], left_num_dims);
      // pad 1 in the front for right
      right_shape.CopyDims(&right_padded_dims_[num_dims_with_pad - right_num_dims], right_num_dims);
    }

    // validate input shape and generate output shape
    std::vector<int64_t> output_dims(num_output_dims);

    // broadcasting for all output dims except last two
    for (int idx_dim = 0; idx_dim < num_dims_with_pad - 2; ++idx_dim) {
      output_dims[idx_dim] = std::max(left_padded_dims_[idx_dim], right_padded_dims_[idx_dim]);
      if (left_padded_dims_[idx_dim] != output_dims[idx_dim])
        LOTUS_ENFORCE(left_padded_dims_[idx_dim] == 1, "left operand cannot broadcast on dim ", idx_dim);
      if (right_padded_dims_[idx_dim] != output_dims[idx_dim])
        LOTUS_ENFORCE(right_padded_dims_[idx_dim] == 1, "right operand cannot broadcast on dim ", idx_dim);
    }

    M_ = has_1D_input ? 1 : left_shape[left_num_dims - 2];
    K_ = left_shape[left_num_dims - 1];
    N_ = (right_num_dims == 1) ? 1 : right_shape[right_num_dims - 1];

    if (!has_1D_input) {
      LOTUS_ENFORCE(K_ == right_shape[right_num_dims - 2], "MatMul dimension mismatch");
      // left (...M x K), right (...K x N), output (...M x N)
      assert(num_dims_with_pad == num_output_dims);
      output_dims[num_output_dims - 2] = M_;
      output_dims[num_output_dims - 1] = N_;
    } else {
      if (num_output_dims == 0) {
        // for left and right being both vector, output is scalar thus no shape
        assert(M_ == 1 && N_ == 1);
      } else {
        if (left_num_dims == 1) {
          assert(num_dims_with_pad - 1 == num_output_dims);
          LOTUS_ENFORCE(K_ == right_shape[right_num_dims - 1], "MatMul dimension mismatch");
          // left (K), right (...K,N), output (...N)
          output_dims[num_output_dims - 1] = N_;
        } else {
          assert(num_dims_with_pad - 2 == num_output_dims);
          LOTUS_ENFORCE(K_ == right_shape[0], "MatMul dimension mismatch");
          // left(...K), right (K), output (...), already assigned
        }
      }
    }

    // assign shape
    output_shape_ = TensorShape(output_dims);

    // compute broadcast offsets
    ComputeBroadcastOffsets();
  }

 private:
  void ComputeBroadcastOffsets() {
    num_broadcasted_dims_ = left_padded_dims_.size() - 2;

    if (num_broadcasted_dims_ == 0) {
      left_offsets_ = {0};
      right_offsets_ = {0};
      output_offsets_ = {0};
      return;
    }

    left_mat_size_ = M_ * K_;
    right_mat_size_ = K_ * N_;
    output_mat_size_ = M_ * N_;

    // stride in mats and dims for broadcasting
    left_padded_strides_.resize(num_broadcasted_dims_);
    right_padded_strides_.resize(num_broadcasted_dims_);
    output_broadcast_strides_.resize(num_broadcasted_dims_);
    output_broadcast_dims_.resize(num_broadcasted_dims_);
    for (size_t i = num_broadcasted_dims_; i > 0; --i) {
      size_t idx = i - 1;
      output_broadcast_dims_[idx] = std::max(left_padded_dims_[idx], right_padded_dims_[idx]);
      output_broadcast_strides_[idx] = ((i == num_broadcasted_dims_) ? 1 : output_broadcast_strides_[idx + 1] * output_broadcast_dims_[idx + 1]);
      left_padded_strides_[idx] = ((i == num_broadcasted_dims_) ? 1 : left_padded_strides_[idx + 1] * left_padded_dims_[idx + 1]);
      right_padded_strides_[idx] = ((i == num_broadcasted_dims_) ? 1 : right_padded_strides_[idx + 1] * right_padded_dims_[idx + 1]);
    }

    size_t num_offsets = output_broadcast_dims_[0] * output_broadcast_strides_[0];
    left_offsets_.resize(num_offsets);
    right_offsets_.resize(num_offsets);
    output_offsets_.resize(num_offsets);

    RecursiveFill(0, 0, 0, 0);
  }

  void
  RecursiveFill(size_t idx_dim, size_t idx_left, size_t idx_right, size_t idx_out) {
    if (idx_dim == num_broadcasted_dims_) {
      left_offsets_[idx_out] = idx_left * left_mat_size_;
      right_offsets_[idx_out] = idx_right * right_mat_size_;
      output_offsets_[idx_out] = idx_out * output_mat_size_;
    } else {
      auto left_dim = left_padded_dims_[idx_dim];
      auto right_dim = right_padded_dims_[idx_dim];
      auto output_dim = output_broadcast_dims_[idx_dim];
      for (int64_t i = 0; i < output_dim; ++i) {
        RecursiveFill(idx_dim + 1,
                      idx_left + i * (left_dim == 1 ? 0 : left_padded_strides_[idx_dim]),
                      idx_right + i * (right_dim == 1 ? 0 : right_padded_strides_[idx_dim]),
                      idx_out + i * output_broadcast_strides_[idx_dim]);
      }
    }
  }

 private:
  size_t left_mat_size_;
  size_t right_mat_size_;
  size_t output_mat_size_;

  size_t num_broadcasted_dims_;

  std::vector<int64_t> left_padded_dims_;
  std::vector<int64_t> right_padded_dims_;
  std::vector<int64_t> output_broadcast_dims_;

  std::vector<size_t> left_padded_strides_;
  std::vector<size_t> right_padded_strides_;
  std::vector<size_t> output_broadcast_strides_;

  TensorShape output_shape_;

  int64_t M_;
  int64_t N_;
  int64_t K_;

  std::vector<size_t> left_offsets_;
  std::vector<size_t> right_offsets_;
  std::vector<size_t> output_offsets_;

 public:
  // output shape
  const TensorShape& OutputShape() const {
    return output_shape_;
  }

  // left and output matrices' first dim
  int64_t M() const {
    return M_;
  }

  // right and output matrices' second dim
  int64_t N() const {
    return N_;
  }

  // left matrices' second dim, and right matrices' first dim
  int64_t K() const {
    return K_;
  }

  // Batched Gemm offsets in left matrices
  const std::vector<size_t>& LeftOffsets() const {
    return left_offsets_;
  }

  // Batched Gemm offsets in right matrices
  const std::vector<size_t>& RightOffsets() const {
    return right_offsets_;
  }

  // Batched Gemm offsets in output matrices
  const std::vector<size_t>& OutputOffsets() const {
    return output_offsets_;
  }
};

template <>
Status MatMul<float>::compute(OpKernelContext* ctx) const {
  const Tensor* left_X = ctx->template input<Tensor>(0);
  const Tensor* right_X = ctx->template input<Tensor>(1);

  MatMulComputeHelper<float> helper(left_X->shape(), right_X->shape());

  Tensor* Y = ctx->output(0, helper.OutputShape());

  // TODO: replace it with GemmBatch for performance, it's OK for now as GemmBatch unrolls as well
  for (int i = 0; i < helper.OutputOffsets().size(); i++) {
    math::Gemm<float, CPUMathUtil>(
        CblasNoTrans,
        CblasNoTrans,
        static_cast<int>(helper.M()),
        static_cast<int>(helper.N()),
        static_cast<int>(helper.K()),
        /* alpha */ 1.0f,
        left_X->data<float>() + helper.LeftOffsets()[i],
        right_X->data<float>() + helper.RightOffsets()[i],
        /* beta */ 0.0f,
        Y->mutable_data<float>() + helper.OutputOffsets()[i],
        &CPUMathUtil::Instance());
  }
  return Status::OK();
}

REGISTER_KERNEL(KernelDefBuilder("MatMul")
                    .Domain(LotusIR::kOnnxDomain)
                    .SinceVersion(1, 2)
                    .Provider(LotusIR::kCpuExecutionProvider)
                    .TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
                MatMul<float>);
}  // namespace Lotus
