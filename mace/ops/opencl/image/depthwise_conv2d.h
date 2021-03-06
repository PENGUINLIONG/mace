// Copyright 2018 The MACE Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef MACE_OPS_OPENCL_IMAGE_DEPTHWISE_CONV2D_H_
#define MACE_OPS_OPENCL_IMAGE_DEPTHWISE_CONV2D_H_

#include "mace/ops/opencl/depthwise_conv2d.h"

#include <memory>
#include <vector>

#include "mace/core/ops/op_context.h"
#include "mace/core/tensor.h"
#include "mace/runtimes/opencl/core/opencl_helper.h"

namespace mace {
namespace ops {
namespace opencl {
namespace image {
namespace depthwise {

MaceStatus DepthwiseConv2d(OpContext *context,
                           cl::Kernel *kernel,
                           const Tensor *input,   // NHWC
                           const Tensor *filter,  // HWIM
                           const Tensor *bias,
                           const int stride,
                           const int *paddings,
                           const int *dilations,
                           const ActivationType activation,
                           const float relux_max_limit,
                           const float activation_coefficient,
                           std::vector<index_t> *prev_input_shape,
                           Tensor *output,
                           uint32_t *kwg_size);
}  // namespace depthwise

class DepthwiseConv2dKernel : public OpenCLDepthwiseConv2dKernel {
 public:
  MaceStatus Compute(
      OpContext *context,
      const Tensor *input,
      const Tensor *filter,
      const Tensor *bias,
      const int *strides,
      const Padding &padding_type,
      const std::vector<int> &padding_data,
      const int *dilations,
      const ActivationType activation,
      const float relux_max_limit,
      const float activation_coefficient,
      Tensor *output) override;

 private:
  cl::Kernel kernel_;
  uint32_t kwg_size_;
  std::vector<index_t> input_shape_;
};

}  // namespace image
}  // namespace opencl
}  // namespace ops
}  // namespace mace

#endif  // MACE_OPS_OPENCL_IMAGE_DEPTHWISE_CONV2D_H_
