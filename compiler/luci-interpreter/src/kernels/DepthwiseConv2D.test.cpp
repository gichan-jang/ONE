/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "kernels/DepthwiseConv2D.h"
#include "kernels/TestUtils.h"

namespace luci_interpreter
{
namespace kernels
{
namespace
{

using namespace testing;

TEST(DepthwiseConv2DTest, Float)
{
  Shape input_shape{1, 4, 2, 2};
  Shape filter_shape{1, 2, 2, 4};
  Shape bias_shape{4};
  std::vector<float> input_data{
      1,  2,  7,  8,  //
      3,  4,  9,  10, //
      5,  6,  11, 12, //
      13, 14, 15, 16, //
  };
  std::vector<float> filter_data{
      1,  2,   3,   4,   //
      -9, 10,  -11, 12,  //
      5,  6,   7,   8,   //
      13, -14, 15,  -16, //
  };
  std::vector<float> bias_data{1, 2, 3, 4};
  Tensor input_tensor = makeInputTensor<DataType::FLOAT32>(input_shape, input_data);
  Tensor filter_tensor = makeInputTensor<DataType::FLOAT32>(filter_shape, filter_data);
  Tensor bias_tensor = makeInputTensor<DataType::FLOAT32>(bias_shape, bias_data);
  Tensor output_tensor = makeOutputTensor(DataType::FLOAT32);

  DepthwiseConv2DParams params{};
  params.padding = Padding::VALID;
  params.depth_multiplier = 2;
  params.stride_height = 2;
  params.stride_width = 1;
  params.dilation_height_factor = 1;
  params.dilation_width_factor = 1;
  params.activation = Activation::RELU;

  DepthwiseConv2D kernel(&input_tensor, &filter_tensor, &bias_tensor, &output_tensor, params);
  kernel.configure();
  kernel.execute();

  std::vector<float> ref_output_data{
      71,  0, 99,  0,  //
      167, 0, 227, 28, //
  };
  EXPECT_THAT(extractTensorData<float>(output_tensor),
              ElementsAreArray(ArrayFloatNear(ref_output_data)));
  EXPECT_THAT(extractTensorShape(output_tensor), ::testing::ElementsAreArray({1, 2, 1, 4}));
}

TEST(DepthwiseConv2DTest, Uint8)
{
  std::pair<float, int32_t> input_quant_param = quantizationParams<uint8_t>(-63.5, 64);
  std::pair<float, int32_t> output_quant_param = quantizationParams<uint8_t>(-127, 128);

  Tensor input_tensor{
      DataType::U8, {1, 3, 2, 2}, {{input_quant_param.first}, {input_quant_param.second}}, ""};
  Tensor filter_tensor{
      DataType::U8, {1, 2, 2, 4}, {{input_quant_param.first}, {input_quant_param.second}}, ""};
  Tensor bias_tensor{
      DataType::S32, {4}, {{input_quant_param.first * input_quant_param.first}, {0}}, ""};
  Tensor output_tensor =
      makeOutputTensor(DataType::U8, output_quant_param.first, output_quant_param.second);

  std::vector<uint8_t> quant_input = quantize<uint8_t>(
      {
          1, 2, 7, 8,   // column 1
          3, 4, 9, 10,  // column 2
          5, 6, 11, 12, // column 3
      },
      input_quant_param.first, input_quant_param.second);
  std::vector<uint8_t> quant_filter = quantize<uint8_t>(
      {
          1, 2, 3, 4,       //
          -9, 10, -11, 12,  //
          5, 6, 7, 8,       //
          13, -14, 15, -16, //
      },
      input_quant_param.first, input_quant_param.second);
  std::vector<int32_t> quant_bias =
      quantize<int32_t>({1, 2, 3, 4}, input_quant_param.first * input_quant_param.first, 0);

  input_tensor.writeData(quant_input.data(), quant_input.size() * sizeof(uint8_t));
  filter_tensor.writeData(quant_filter.data(), quant_filter.size() * sizeof(uint8_t));
  bias_tensor.writeData(quant_bias.data(), quant_bias.size() * sizeof(int32_t));

  DepthwiseConv2DParams params{};
  params.padding = Padding::VALID;
  params.depth_multiplier = 2;
  params.stride_height = 1;
  params.stride_width = 1;
  params.dilation_height_factor = 1;
  params.dilation_width_factor = 1;
  params.activation = Activation::NONE;

  DepthwiseConv2D kernel(&input_tensor, &filter_tensor, &bias_tensor, &output_tensor, params);
  kernel.configure();
  kernel.execute();

  std::vector<float> ref_output_data{
      71, -34, 99,  -20, //
      91, -26, 127, -4,  //
  };
  EXPECT_THAT(dequantize(extractTensorData<uint8_t>(output_tensor), output_tensor.scale(),
                         output_tensor.zero_point()),
              ElementsAreArray(ArrayFloatNear(ref_output_data)));
  EXPECT_THAT(extractTensorShape(output_tensor), ::testing::ElementsAreArray({1, 2, 1, 4}));
}

} // namespace
} // namespace kernels
} // namespace luci_interpreter
