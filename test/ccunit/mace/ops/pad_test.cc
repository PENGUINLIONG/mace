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
#include <string>
#include <vector>

#include "mace/ops/common/pad_type.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

class PadTest : public OpsTestBase {};

namespace {
template <RuntimeType D>
void SimpleConstant() {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRepeatedInput<D, float>("Input", {1, 2, 3, 1}, 2);
  if (D == RuntimeType::RT_OPENCL) {
    OpDefBuilder("Pad", "PadTest")
        .Input("Input")
        .Output("Output")
        .AddIntsArg("paddings", {0, 0, 1, 2, 1, 2, 0, 0})
        .AddFloatArg("constant_value", 1.0)
        .AddIntArg("has_data_format", 1)
        .Finalize(net.NewOperatorDef());

    // Run
    net.RunOp(D);
  } else {
    net.TransformDataFormat<RuntimeType::RT_CPU, float>(
        "Input", DataFormat::NHWC, "TInput", DataFormat::NCHW);
    OpDefBuilder("Pad", "PadTest")
        .Input("TInput")
        .Output("TOutput")
        .AddIntsArg("paddings", {0, 0, 1, 2, 1, 2, 0, 0})
        .AddFloatArg("constant_value", 1.0)
        .AddIntArg("has_data_format", 1)
        .Finalize(net.NewOperatorDef());

    // Run
    net.RunOp();

    net.TransformDataFormat<RuntimeType::RT_CPU, float>(
        "TOutput", DataFormat::NCHW, "Output", DataFormat::NHWC);
  }

  auto output = net.GetTensor("Output");

  auto expected = net.CreateTensor<float>(
      {1, 5, 6, 1}, {
                        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 2,   2,   2,
                        1.0, 1.0, 1.0, 2,   2,   2,   1.0, 1.0, 1.0, 1.0,
                        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
                    });
  ExpectTensorNear<float>(*expected, *output, 1e-5);
}

template <RuntimeType D, typename T>
void Result(const std::vector<index_t> &input_shape,
            const std::vector<float> &input_data,
            const std::vector<index_t> &expected_shape,
            const std::vector<float> &expected_data,
            const std::vector<int> &paddings,
            const PadType pad_type) {
  // Construct graph
  OpsTestNet net;
  std::string input("Input");
  std::string t_input(input);
  std::string output("Output");
  std::string t_output(output);

  // Add input data
  net.AddInputFromArray<D, float>(input, input_shape, input_data);

  if (D == RuntimeType::RT_CPU) {
    t_input = "TInput";
    t_output = "TOutput";
    net.TransformDataFormat<RuntimeType::RT_CPU, T>(
        input, DataFormat::NHWC, t_input, DataFormat::NCHW);
  }

  OpDefBuilder("Pad", "PadTest")
  .Input(t_input)
  .Output(t_output)
  .AddIntsArg("paddings", paddings)
  .AddIntArg("pad_type", static_cast<int>(pad_type))
  .AddIntArg("has_data_format", 1)
  .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(D);

  if (D == RuntimeType::RT_CPU) {
    net.TransformDataFormat<RuntimeType::RT_CPU, T>(
        t_output, DataFormat::NCHW, output, DataFormat::NHWC);
  }

  auto actual = net.GetTensor(output.c_str());
  auto expected = net.CreateTensor<float>(expected_shape, expected_data);

  ExpectTensorNear<float>(*expected, *actual, 1e-5);
}

void SimpleMirror(const std::vector<float> &expected_data,
                  const PadType pad_type) {
  std::vector<index_t> input_shape{1, 3, 4, 1};
  int size = std::accumulate(input_shape.begin(), input_shape.end(),
                             1, std::multiplies<index_t>());
  std::vector<float> input_data;
  std::vector<index_t> expected_shape{1, 6, 7, 1};
  const std::vector<int> paddings{0, 0, 1, 2, 3, 0, 0, 0};

  input_data.reserve(size);
  for (int i = 1; i <= size; i++) {
    input_data.push_back(i);
  }

  Result<RuntimeType::RT_CPU, float>(input_shape, input_data, expected_shape,
      expected_data, paddings, pad_type);
  Result<RuntimeType::RT_OPENCL, float>(input_shape, input_data, expected_shape,
      expected_data, paddings, pad_type);
  Result<RuntimeType::RT_OPENCL, half>(input_shape, input_data, expected_shape,
      expected_data, paddings, pad_type);
}
}  // namespace

TEST_F(PadTest, SimpleConstantCPU) { SimpleConstant<RuntimeType::RT_CPU>(); }

TEST_F(PadTest, SimpleConstantGPU) { SimpleConstant<RuntimeType::RT_OPENCL>(); }

TEST_F(PadTest, SimpleReflect) {
  SimpleMirror({
     8,  7,  6, 5,  6,  7,  8,
     4,  3,  2, 1,  2,  3,  4,
     8,  7,  6, 5,  6,  7,  8,
    12, 11, 10, 9, 10, 11, 12,
     8,  7,  6, 5,  6,  7,  8,
     4,  3,  2, 1,  2,  3,  4,
  }, PadType::REFLECT);
}

TEST_F(PadTest, SimpleSymmetric) {
  SimpleMirror({
     3,  2, 1, 1,  2,  3,  4,
     3,  2, 1, 1,  2,  3,  4,
     7,  6, 5, 5,  6,  7,  8,
    11, 10, 9, 9, 10, 11, 12,
    11, 10, 9, 9, 10, 11, 12,
     7,  6, 5, 5,  6,  7,  8,
  }, PadType::SYMMETRIC);
}

TEST_F(PadTest, ComplexCPU) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRepeatedInput<RuntimeType::RT_CPU, float>("Input", {1, 1, 1, 2}, 2);
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "Input", DataFormat::NHWC, "TInput", DataFormat::NCHW);
  OpDefBuilder("Pad", "PadTest")
      .Input("TInput")
      .Output("TOutput")
      .AddIntsArg("paddings", {0, 0, 1, 1, 1, 1, 1, 1})
      .AddFloatArg("constant_value", 1.0)
      .AddIntArg("has_data_format", 1)
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "TOutput", DataFormat::NCHW, "Output", DataFormat::NHWC);

  auto output = net.GetTensor("Output");

  auto expected = net.CreateTensor<float>(
      {1, 3, 3, 4},
      {
          1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
          1.0, 1.0, 1.0, 1.0, 1.0, 2.0, 2.0, 1.0, 1.0, 1.0, 1.0, 1.0,
          1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,
      });
  ExpectTensorNear<float>(*expected, *output, 1e-5);
}

namespace {
template <typename T>
void Complex(const std::vector<index_t> &input_shape,
             const std::vector<int> &paddings, const int pad_type) {
  // Construct graph
  OpsTestNet net;

  // Add input data
  net.AddRandomInput<RuntimeType::RT_OPENCL, float>("Input", input_shape);

  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "Input", DataFormat::NHWC, "TInput", DataFormat::NCHW);
  OpDefBuilder("Pad", "PadTest")
      .Input("TInput")
      .Output("TOutput")
      .AddIntsArg("paddings", paddings)
      .AddIntArg("pad_type", pad_type)
      .AddFloatArg("constant_value", 1.0)
      .AddIntArg("has_data_format", 1)
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "TOutput", DataFormat::NCHW, "Output", DataFormat::NHWC);

  auto expected = net.CreateTensor<float>();
  expected->Copy(*net.GetOutput("Output"));

  OpDefBuilder("Pad", "PadTest")
      .Input("Input")
      .Output("Output")
      .AddIntsArg("paddings", paddings)
      .AddIntArg("pad_type", pad_type)
      .AddFloatArg("constant_value", 1.0)
      .AddIntArg("has_data_format", 1)
      .Finalize(net.NewOperatorDef());

  // Run
  net.RunOp(RuntimeType::RT_OPENCL);

  auto output = net.GetTensor("Output");

  if (DataTypeToEnum<T>::value == DT_HALF) {
    ExpectTensorNear<float>(*expected, *output, 1e-2, 1e-2);
  } else {
    ExpectTensorNear<float>(*expected, *output, 1e-5);
  }
}
}  // namespace

TEST_F(PadTest, ComplexFloat) {
  for (int i = PadType::CONSTANT; i <= PadType::SYMMETRIC; i++) {
    Complex<float>({1, 32, 32, 4}, {0, 0, 2, 2, 1, 1, 0, 0}, i);
    Complex<float>({1, 31, 37, 16}, {0, 0, 2, 0, 1, 0, 0, 0}, i);
    Complex<float>({1, 128, 128, 32}, {0, 0, 0, 1, 0, 2, 0, 0}, i);
  }
}

TEST_F(PadTest, ComplexHalf) {
  for (int i = PadType::CONSTANT; i <= PadType::SYMMETRIC; i++) {
    Complex<half>({1, 32, 32, 4}, {0, 0, 2, 2, 1, 1, 0, 0}, i);
    Complex<half>({1, 31, 37, 16}, {0, 0, 2, 0, 1, 0, 0, 0}, i);
    Complex<half>({1, 128, 128, 32}, {0, 0, 0, 1, 0, 2, 0, 0}, i);
  }
}

TEST_F(PadTest, ReflectCPU) {
  std::vector<index_t> input_shape{2, 2, 2, 2};
  int size = std::accumulate(input_shape.begin(), input_shape.end(),
                             1, std::multiplies<index_t>());
  std::vector<float> input_data;
  std::vector<index_t> expected_shape{4, 4, 4, 4};
  std::vector<float> expected_data{
          16, 15, 16, 15,
          14, 13, 14, 13,
          16, 15, 16, 15,
          14, 13, 14, 13,

          12, 11, 12, 11,
          10,  9, 10,  9,
          12, 11, 12, 11,
          10,  9, 10,  9,

          16, 15, 16, 15,
          14, 13, 14, 13,
          16, 15, 16, 15,
          14, 13, 14, 13,

          12, 11, 12, 11,
          10,  9, 10,  9,
          12, 11, 12, 11,
          10,  9, 10,  9,


           8,  7,  8,  7,
           6,  5,  6,  5,
           8,  7,  8,  7,
           6,  5,  6,  5,

           4,  3,  4,  3,
           2,  1,  2,  1,
           4,  3,  4,  3,
           2,  1,  2,  1,

           8,  7,  8,  7,
           6,  5,  6,  5,
           8,  7,  8,  7,
           6,  5,  6,  5,

           4,  3,  4,  3,
           2,  1,  2,  1,
           4,  3,  4,  3,
           2,  1,  2,  1,


          16, 15, 16, 15,
          14, 13, 14, 13,
          16, 15, 16, 15,
          14, 13, 14, 13,

          12, 11, 12, 11,
          10,  9, 10,  9,
          12, 11, 12, 11,
          10,  9, 10,  9,

          16, 15, 16, 15,
          14, 13, 14, 13,
          16, 15, 16, 15,
          14, 13, 14, 13,

          12, 11, 12, 11,
          10,  9, 10,  9,
          12, 11, 12, 11,
          10,  9, 10,  9,


           8,  7,  8,  7,
           6,  5,  6,  5,
           8,  7,  8,  7,
           6,  5,  6,  5,

           4,  3,  4,  3,
           2,  1,  2,  1,
           4,  3,  4,  3,
           2,  1,  2,  1,

           8,  7,  8,  7,
           6,  5,  6,  5,
           8,  7,  8,  7,
           6,  5,  6,  5,

           4,  3,  4,  3,
           2,  1,  2,  1,
           4,  3,  4,  3,
           2,  1,  2,  1,
  };
  const std::vector<int> paddings{1, 1, 1, 1, 1, 1, 1, 1};

  input_data.reserve(size);
  for (int i = 1; i <= size; i++) {
    input_data.push_back(i);
  }

  Result<RuntimeType::RT_CPU, float>(input_shape, input_data, expected_shape,
      expected_data, paddings, PadType::REFLECT);
}

TEST_F(PadTest, SymmetricCPU) {
  std::vector<index_t> input_shape{2, 2, 2, 2};
  int size = std::accumulate(input_shape.begin(), input_shape.end(),
                             1, std::multiplies<index_t>());
  std::vector<float> input_data;
  std::vector<index_t> expected_shape{4, 4, 4, 4};
  std::vector<float> expected_data{
           1,  1,  2,  2,
           1,  1,  2,  2,
           3,  3,  4,  4,
           3,  3,  4,  4,

           1,  1,  2,  2,
           1,  1,  2,  2,
           3,  3,  4,  4,
           3,  3,  4,  4,

           5,  5,  6,  6,
           5,  5,  6,  6,
           7,  7,  8,  8,
           7,  7,  8,  8,

           5,  5,  6,  6,
           5,  5,  6,  6,
           7,  7,  8,  8,
           7,  7,  8,  8,


           1,  1,  2,  2,
           1,  1,  2,  2,
           3,  3,  4,  4,
           3,  3,  4,  4,

           1,  1,  2,  2,
           1,  1,  2,  2,
           3,  3,  4,  4,
           3,  3,  4,  4,

           5,  5,  6,  6,
           5,  5,  6,  6,
           7,  7,  8,  8,
           7,  7,  8,  8,

           5,  5,  6,  6,
           5,  5,  6,  6,
           7,  7,  8,  8,
           7,  7,  8,  8,


           9,  9, 10, 10,
           9,  9, 10, 10,
          11, 11, 12, 12,
          11, 11, 12, 12,

           9,  9, 10, 10,
           9,  9, 10, 10,
          11, 11, 12, 12,
          11, 11, 12, 12,

          13, 13, 14, 14,
          13, 13, 14, 14,
          15, 15, 16, 16,
          15, 15, 16, 16,

          13, 13, 14, 14,
          13, 13, 14, 14,
          15, 15, 16, 16,
          15, 15, 16, 16,


           9,  9, 10, 10,
           9,  9, 10, 10,
          11, 11, 12, 12,
          11, 11, 12, 12,

           9,  9, 10, 10,
           9,  9, 10, 10,
          11, 11, 12, 12,
          11, 11, 12, 12,

          13, 13, 14, 14,
          13, 13, 14, 14,
          15, 15, 16, 16,
          15, 15, 16, 16,

          13, 13, 14, 14,
          13, 13, 14, 14,
          15, 15, 16, 16,
          15, 15, 16, 16,
  };
  const std::vector<int> paddings{1, 1, 1, 1, 1, 1, 1, 1};

  input_data.reserve(size);
  for (int i = 1; i <= size; i++) {
    input_data.push_back(i);
  }

  Result<RuntimeType::RT_CPU, float>(input_shape, input_data, expected_shape,
      expected_data, paddings, PadType::SYMMETRIC);
}

namespace {
void TestQuantized(const std::vector<int> &paddings,
                   const int pad_type) {
  static unsigned int seed = time(NULL);
  index_t batch = 1 + rand_r(&seed) % 10;
  index_t channels = 3 + rand_r(&seed) % 50;
  index_t height = 64 + rand_r(&seed) % 50;
  index_t width = 64 + rand_r(&seed) % 50;

  OpsTestNet net;
  std::vector<index_t> input_shape{batch, height, width, channels};
  net.AddRandomInput<RT_CPU, float>(
      "Input", input_shape, false, false);
  net.TransformDataFormat<RT_CPU, float>(
      "Input", DataFormat::NHWC, "TInput", DataFormat::NCHW);
  OpDefBuilder("Pad", "PadTest")
      .Input("TInput")
      .Output("TOutput")
      .AddIntsArg("paddings", paddings)
      .AddIntArg("pad_type", pad_type)
      .AddFloatArg("constant_value", 1.0)
      .AddIntArg("has_data_format", 1)
      .AddIntArg("T", DT_FLOAT)
      .Finalize(net.NewOperatorDef());

  net.RunOp();
  net.TransformDataFormat<RuntimeType::RT_CPU, float>(
      "TOutput", DataFormat::NCHW, "Output", DataFormat::NHWC);

  OpDefBuilder("Quantize", "QuantizeInput")
      .Input("Input")
      .Output("QuantizedInput")
      .OutputType({DT_UINT8})
      .AddIntArg("T", DT_UINT8)
      .AddIntArg("non_zero", true)
      .Finalize(net.NewOperatorDef());
  net.RunOp();

  net.AddRandomInput<RuntimeType::RT_CPU, uint8_t>("QuantizedOutput",
                                                   input_shape);
  OpDefBuilder("Pad", "QuantizedPadTest")
      .Input("QuantizedInput")
      .Output("QuantizedOutput")
      .AddIntsArg("paddings", paddings)
      .AddIntArg("pad_type", pad_type)
      .AddFloatArg("constant_value", 1.0)
      .AddIntArg("has_data_format", 1)
      .AddIntArg("T", DT_UINT8)
      .Finalize(net.NewOperatorDef());
  net.RunOp();

  OpDefBuilder("Dequantize", "DeQuantizeTest")
      .Input("QuantizedOutput")
      .Output("DequantizedOutput")
      .OutputType({DT_FLOAT})
      .AddIntArg("T", DT_UINT8)
      .Finalize(net.NewOperatorDef());
  net.RunOp();

  // Check
  ExpectTensorSimilar<float>(*net.GetOutput("Output"),
                             *net.GetTensor("DequantizedOutput"), 0.01);
}
}  // namespace

TEST_F(PadTest, Quantized) {
  for (int i = PadType::CONSTANT; i <= PadType::SYMMETRIC; i++) {
    TestQuantized({0, 0, 2, 2, 1, 1, 0, 0}, i);
    TestQuantized({0, 0, 2, 0, 1, 0, 0, 0}, i);
    TestQuantized({0, 0, 0, 1, 0, 2, 0, 0}, i);
  }
}

}  // namespace test
}  // namespace ops
}  // namespace mace
