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

#include <vector>

#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

class ResizeBicubicTest : public OpsTestBase {};

TEST_F(ResizeBicubicTest, CPUResizeBicubicWOAlignCorners) {
  testing::internal::LogToStderr();
  // Construct graph
  OpsTestNet net;

  // Add input data
  std::vector<float> input(24);
  std::iota(begin(input), end(input), 0);
  net.AddInputFromArray<RuntimeType::RT_CPU, float>("Input",
                                                    {1, 2, 4, 3}, input);
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "Input", DataFormat::NHWC, "InputNCHW", DataFormat::NCHW);

  OpDefBuilder("ResizeBicubic", "ResizeBicubicTest")
      .Input("InputNCHW")
      .Output("OutputNCHW")
      .AddIntsArg("size", {1, 2})
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "OutputNCHW", DataFormat::NCHW, "Output", DataFormat::NHWC);

  // Check
  auto expected = net.CreateTensor<float>({1, 1, 2, 3}, {0, 1, 2, 6, 7, 8});

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-2);
}

TEST_F(ResizeBicubicTest, CPUResizeBicubicWOAlignCornersFloat) {
  testing::internal::LogToStderr();
  // Construct graph
  OpsTestNet net;

  // Add input data
  std::vector<float> input(48);
  std::iota(begin(input), end(input), 0);
  net.AddInputFromArray<RuntimeType::RT_CPU, float>("Input",
                                                    {1, 4, 4, 3}, input);
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "Input", DataFormat::NHWC, "InputNCHW", DataFormat::NCHW);

  OpDefBuilder("ResizeBicubic", "ResizeBicubicTest")
      .Input("InputNCHW")
      .Output("OutputNCHW")
      .AddIntsArg("size", {2, 3})
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "OutputNCHW", DataFormat::NCHW, "Output", DataFormat::NHWC);

  // Check
  auto expected = net.CreateTensor<float>({1, 2, 3, 3},
      {0., 1., 2., 4.110297, 5.110297, 6.110297,
       8.223037, 9.223036, 10.223037, 24., 25., 26.,
       28.110298, 29.1103, 30.110298, 32.223038, 33.223038, 34.223038});

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-2);
}

TEST_F(ResizeBicubicTest, ResizeBicubicWAlignCorners) {
  testing::internal::LogToStderr();
  // Construct graph
  OpsTestNet net;

  // Add input data
  std::vector<float> input(24);
  std::iota(begin(input), end(input), 0);
  net.AddInputFromArray<RuntimeType::RT_CPU, float>("Input",
                                                    {1, 2, 4, 3}, input);
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "Input", DataFormat::NHWC, "InputNCHW", DataFormat::NCHW);

  OpDefBuilder("ResizeBicubic", "ResizeBicubicTest")
      .Input("InputNCHW")
      .Output("OutputNCHW")
      .AddIntArg("align_corners", 1)
      .AddIntsArg("size", {1, 2})
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "OutputNCHW", DataFormat::NCHW, "Output", DataFormat::NHWC);

  // Check
  auto expected = net.CreateTensor<float>({1, 1, 2, 3}, {0, 1, 2, 9, 10, 11});

  ExpectTensorNear<float>(*expected, *net.GetOutput("Output"), 1e-2);
}

namespace {
template <RuntimeType D>
void TestRandomResizeBicubic() {
  testing::internal::LogToStderr();
  static unsigned int seed = time(NULL);
  for (int round = 0; round < 10; ++round) {
    int batch = 1 + rand_r(&seed) % 5;
    int channels = 1 + rand_r(&seed) % 100;
    int height = 1 + rand_r(&seed) % 100;
    int width = 1 + rand_r(&seed) % 100;
    int in_height = 1 + rand_r(&seed) % 100;
    int in_width = 1 + rand_r(&seed) % 100;
    int align_corners = rand_r(&seed) % 2;
    int coordinate_transformation_mode = 0;
    if (align_corners == 0) {
      coordinate_transformation_mode = rand_r(&seed) % 3;
    }

    // Construct graph
    OpsTestNet net;
    // Add input data
    net.AddRandomInput<D, float>("Input",
                                 {batch, in_height, in_width, channels},
                                 false, true, true);
    net.TransformDataFormat<RuntimeType::RT_CPU, float>(
        "Input", DataFormat::NHWC, "InputNCHW", DataFormat::NCHW);

    OpDefBuilder("ResizeBicubic", "ResizeBicubicTest")
        .Input("InputNCHW")
        .Output("OutputNCHW")
        .AddIntArg("align_corners", align_corners)
        .AddIntArg("coordinate_transformation_mode",
                   coordinate_transformation_mode)
        .AddIntsArg("size", {height, width})
        .Finalize(net.NewOperatorDef());
    // Run on CPU
    net.RunOp(RuntimeType::RT_CPU);
    net.TransformDataFormat<RuntimeType::RT_CPU, float>(
        "OutputNCHW", DataFormat::NCHW, "Output", DataFormat::NHWC);

    auto cpu_runtime = OpTestContext::Get()->GetRuntime(RuntimeType::RT_CPU);
    Tensor expected(cpu_runtime, DT_FLOAT);
    expected.Copy(*net.GetOutput("Output"));

    if (D == RuntimeType::RT_OPENCL) {
      OpDefBuilder("ResizeBicubic", "ResizeBicubicTest")
          .Input("Input")
          .Output("Output")
          .AddIntArg("align_corners", align_corners)
          .AddIntArg("coordinate_transformation_mode",
                     coordinate_transformation_mode)
          .AddIntsArg("size", {height, width})
          .Finalize(net.NewOperatorDef());
      // Run
      net.RunOp(D);
    }
    // Check
    ExpectTensorNear<float>(expected, *net.GetOutput("Output"), 1e-2,
                            1e-2);
  }
}
}  // namespace

TEST_F(ResizeBicubicTest, OPENCLRandomResizeBicubic) {
  TestRandomResizeBicubic<RuntimeType::RT_OPENCL>();
}

}  // namespace test
}  // namespace ops
}  // namespace mace
