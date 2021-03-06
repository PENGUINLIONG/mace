// Copyright 2019 The MACE Authors. All Rights Reserved.
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


#include <gtest/gtest.h>

#include "mace/core/ops/op_context.h"
#include "mace/core/tensor.h"
#include "mace/ops/delegator/gemv.h"
#include "mace/ops/ops_test_util.h"
#include "mace/ops/testing/test_utils.h"

namespace mace {
namespace ops {
namespace test {

void TestGemvFloat32(const index_t batch,
                     const index_t height,
                     const index_t width,
                     const bool lhs_batched,
                     const bool rhs_batched) {
  auto *cpu_runtime = OpTestContext::Get()->GetRuntime(RuntimeType::RT_CPU);
  Tensor lhs(cpu_runtime, DataType::DT_FLOAT);
  Tensor rhs(cpu_runtime, DataType::DT_FLOAT);
  Tensor bias(cpu_runtime, DataType::DT_FLOAT);
  Tensor output(cpu_runtime, DataType::DT_FLOAT);
  lhs.Resize({lhs_batched ? batch : 1, height, width});
  rhs.Resize({rhs_batched ? batch : 1, width});
  bias.Resize({height});
  output.Resize({batch, height});
  {
    Tensor::MappingGuard lhs_guard(&lhs);
    Tensor::MappingGuard rhs_guard(&rhs);
    Tensor::MappingGuard bias_guard(&bias);
    float *lhs_data = lhs.mutable_data<float>();
    float *rhs_data = rhs.mutable_data<float>();
    float *bias_data = bias.mutable_data<float>();
    GenerateRandomRealTypeData<float>(lhs.shape(), lhs_data);
    GenerateRandomRealTypeData<float>(rhs.shape(), rhs_data);
    GenerateRandomRealTypeData<float>(bias.shape(), bias_data);
  }

  OpsTestNet net;
  OpContext context(net.ws(), cpu_runtime);
  std::unique_ptr<delegator::Gemv> gemv = delegator::Gemv::Create(
      context.workspace(),
      MACE_DELEGATOR_KEY(Gemv, RuntimeType::RT_CPU, float, ImplType::NEON),
      DelegatorParam());
  gemv->Compute(&context,
                &lhs,
                &rhs,
                &bias,
                batch,
                height,
                width,
                lhs_batched,
                rhs_batched,
                &output);

  Tensor expected_output(cpu_runtime, DataType::DT_FLOAT);
  expected_output.Resize({batch, height});
  std::unique_ptr<delegator::Gemv> gemv_ref = delegator::Gemv::Create(
      context.workspace(), MACE_DELEGATOR_KEY(
      Gemv, RuntimeType::RT_CPU, float, ImplType::REF), DelegatorParam());
  gemv_ref->Compute(&context,
                    &lhs,
                    &rhs,
                    &bias,
                    batch,
                    height,
                    width,
                    lhs_batched,
                    rhs_batched,
                    &expected_output);

  ExpectTensorNear<float>(expected_output, output);
}

TEST(ArmGemv, TestGemvFloat32) {
  TestGemvFloat32(1, 16, 4, true, true);
  TestGemvFloat32(1, 16, 256, true, true);
  TestGemvFloat32(2, 16, 256, true, true);
  TestGemvFloat32(3, 63, 257, true, true);

  TestGemvFloat32(2, 16, 256, false, true);
  TestGemvFloat32(3, 63, 257, false, true);
  TestGemvFloat32(2, 16, 256, true, false);
  TestGemvFloat32(3, 63, 257, true, false);
}

}  // namespace test
}  // namespace ops
}  // namespace mace
