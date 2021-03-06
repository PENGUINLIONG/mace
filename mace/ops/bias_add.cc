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

#include <functional>
#include <memory>
#include <vector>

#include "mace/core/ops/operator.h"
#include "mace/core/registry/ops_registry.h"
#include "mace/ops/activation.h"
#include "mace/ops/delegator/bias_add.h"

#ifdef MACE_ENABLE_OPENCL
#include "mace/ops/opencl/image/bias_add.h"
#include "mace/runtimes/opencl/transform/buffer_transformer.h"
#endif  // MACE_ENABLE_OPENCL
#include "mace/utils/memory.h"

namespace mace {
namespace ops {

template<RuntimeType D, class T>
class BiasAddOp;

template<class T>
class BiasAddOp<RuntimeType::RT_CPU, T> : public Operation {
 public:
  explicit BiasAddOp(OpConstructContext *context)
      : Operation(context),
        has_data_format_(Operation::GetOptionalArg<int>("has_data_format", 0)),
        bias_add_delegator_(delegator::BiasAdd::Create(
            context->workspace(),
            MACE_DELEGATOR_KEY(BiasAdd, RuntimeType::RT_CPU, T, kCpuImplType),
            DelegatorParam())) {}

  MaceStatus Run(OpContext *context) override {
    MACE_UNUSED(context);
    const Tensor *input = this->Input(0);
    const Tensor *bias = this->Input(1);
    Tensor *output = this->Output(0);

    MACE_CHECK(bias->dim_size() == 1 || bias->dim_size() == 2,
                 "bias must be 1 or 2 dimensionals for caffe.",
                 bias->dim_size(), MakeString(bias->shape()));
    if (input->dim_size() == 4 &&
        ((has_data_format_ && DataTypeToEnum<T>::value != DT_UINT8) ||
         input->data_format() == DataFormat::NCHW)) {  // NCHW
      bias_add_delegator_->Compute(context, input, bias, output, true);
    } else {  // NHWC
      bias_add_delegator_->Compute(context, input, bias, output, false);
    }

    return MaceStatus::MACE_SUCCESS;
  }

 private:
  int has_data_format_;
  std::unique_ptr<delegator::BiasAdd> bias_add_delegator_;
};

#ifdef MACE_ENABLE_OPENCL
template<>
class BiasAddOp<RuntimeType::RT_OPENCL, float> : public Operation {
 public:
  explicit BiasAddOp(OpConstructContext *context)
      : Operation(context),
        has_data_format_(Operation::GetOptionalArg<int>("has_data_format", 1)) {
    MemoryType mem_type = MemoryType::CPU_BUFFER;
    if (context->GetOpMemoryType() == MemoryType::GPU_IMAGE) {
      mem_type = MemoryType::GPU_IMAGE;
      kernel_ = make_unique<opencl::image::BiasAddKernel>();
    } else {
      MACE_NOT_IMPLEMENTED;
    }

    // for const bias tensor
    if (context->workspace()->GetTensor(operator_def_->input(1)) != nullptr) {
      MACE_CHECK(TransformFilter(context, operator_def_.get(), 1,
                                 BufferContentType::ARGUMENT, mem_type)
                     == MaceStatus::MACE_SUCCESS, "TransformFilter failed");
    }
  }

  MaceStatus Run(OpContext *context) override {
    const Tensor *input = this->Input(0);
    const Tensor *bias = this->Input(1);
    Tensor *output = this->Output(0);
    MACE_RETURN_IF_ERROR(output->ResizeLike(input));
    MACE_CHECK(input->dim_size() == 4 && has_data_format_,
               "gpu only support biasadd for 4-dimensional NHWC format tensor");
    MACE_CHECK(bias->dim_size() == 1 || bias->dim_size() == 2,
               "bias must be 1-dimensional or 2-dimensional for caffee. ",
               MakeString(bias->shape()));
    return kernel_->Compute(context, input, bias, output);
  }

 private:
  int has_data_format_;
  std::unique_ptr<OpenCLBiasAddKernel> kernel_;
};
#endif  // MACE_ENABLE_OPENCL

void RegisterBiasAdd(OpRegistry *op_registry) {
  MACE_REGISTER_OP(op_registry, "BiasAdd", BiasAddOp,
                   RuntimeType::RT_CPU, float);
  MACE_REGISTER_BF16_OP(op_registry, "BiasAdd",
                        BiasAddOp, RuntimeType::RT_CPU);
#ifdef MACE_ENABLE_QUANTIZE
  MACE_REGISTER_OP(op_registry, "BiasAdd", BiasAddOp,
                   RuntimeType::RT_CPU, uint8_t);
#endif  // MACE_ENABLE_QUANTIZE
  MACE_REGISTER_GPU_OP(op_registry, "BiasAdd", BiasAddOp);
  MACE_REGISTER_OP_CONDITION(
      op_registry,
      OpConditionBuilder("BiasAdd")
          .SetDevicePlacerFunc(
              [](OpConditionContext *context) -> std::set<RuntimeType> {
                auto op = context->operator_def();
                if (op->output_shape_size() != op->output_size()) {
                  return {RuntimeType::RT_CPU, RuntimeType::RT_OPENCL};
                }
                int has_data_format =
                    ProtoArgHelper::GetOptionalArg<OperatorDef, int>(
                        *op, "has_data_format", 0);
                if (!has_data_format ||
                    op->output_shape(0).dims_size() != 4) {
                  LOG(INFO) << "BiasAdd only support cpu, has_data_format="
                            << has_data_format
                            << ", op->output_shape(0).dims_size()="
                            << op->output_shape(0).dims_size();
                  return {RuntimeType::RT_CPU};
                }
                return {RuntimeType::RT_CPU, RuntimeType::RT_OPENCL};
              }));
}

}  // namespace ops
}  // namespace mace
