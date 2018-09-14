// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "core/framework/tensor.h"
#include "cuda_execution_provider.h"
namespace onnxruntime {

class CUDAFence : public IFence {
 public:
  CUDAFence(const CUDAExecutionProvider* provider);
  virtual ~CUDAFence();
  virtual void BeforeUsingAsInput(onnxruntime::ProviderType provider_type, int queue_id) override;
  virtual void BeforeUsingAsOutput(onnxruntime::ProviderType provider_type, int queue_id) override;
  virtual void AfterUsedAsInput(int queue_id) override;
  virtual void AfterUsedAsOutput(int queue_id) override;

 private:
  cudaEvent_t read_event_;
  cudaEvent_t write_event_;
  const CUDAExecutionProvider* provider_;
};

}  // namespace onnxruntime
