#pragma once

#include <ATen/Tensor.h>

class Image
{
public:
  void from_tensor(at::Tensor &t);

private:
};