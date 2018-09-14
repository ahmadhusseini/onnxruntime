// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/common/common.h"
#include "core/framework/tensor.h"
#include "gsl/gsl_util"

namespace onnxruntime {

// Verify and convert unknown dim during reshape
class ReshapeHelper {
 public:
  ReshapeHelper(const TensorShape& input_shape, std::vector<int64_t>& requested_shape) {
    auto nDims = requested_shape.size();
    int64_t unknown_dim = -1;
    int64_t size = 1;
    for (size_t i = 0; i < nDims; ++i) {
      LOTUS_ENFORCE(requested_shape[i] >= -1, "A dimension cannot be less than -1.");
      if (requested_shape[i] == -1) {
        LOTUS_ENFORCE(unknown_dim == -1, "At most one dimension can be -1.");
        unknown_dim = i;
      } else {
        if (requested_shape[i] == 0) {
          LOTUS_ENFORCE(i < input_shape.NumDimensions(),
                        "The dimension with value zero exceeds"
                        " the dimension size of the input tensor.");
          requested_shape[i] = input_shape[i];
        }
        size *= requested_shape[i];
      }
    }

    if (unknown_dim != -1) {
      // calculate unknown dimension
      LOTUS_ENFORCE((input_shape.Size() % size) == 0,
                    "The input tensor cannot be reshaped to the requested shape. Input shape:", input_shape);
      requested_shape[unknown_dim] = input_shape.Size() / size;
    } else {
      // check if the output shape is valid.
      LOTUS_ENFORCE(gsl::narrow_cast<int64_t>(input_shape.Size()) == size,
                    "The input tensor cannot be reshaped to the requested shape. Input shape:", input_shape);
    }
  }
};

}  //namespace onnxruntime
