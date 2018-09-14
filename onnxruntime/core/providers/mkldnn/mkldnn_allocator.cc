// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "mkldnn_allocator.h"
#include "core/framework/allocatormgr.h"

namespace onnxruntime {

const AllocatorInfo& MKLDNNAllocator::Info() const {
  static constexpr AllocatorInfo mkl_allocator_info(MKLDNN, AllocatorType::kDeviceAllocator);
  return mkl_allocator_info;
}

const AllocatorInfo& MKLDNNCPUAllocator::Info() const {
  static constexpr AllocatorInfo mkl_cpu_allocator_info(MKLDNN_CPU, AllocatorType::kDeviceAllocator);
  return mkl_cpu_allocator_info;
}
}  // namespace onnxruntime
