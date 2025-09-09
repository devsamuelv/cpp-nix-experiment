#include <image/util.h>
#include <torch/torch.h>
#include <ATen/Tensor.h>
#include <opencv4/opencv2/opencv.hpp>
#include <opencv4/opencv2/core/mat.hpp>

void Image::from_tensor(at::Tensor &tensor)
{
  // Do I need this?
  int64_t shape = *tensor.sizes().data();
  int64_t dim = tensor.dim();

  /*
    Mode used by the python runtime of the model uses image mode L.
    It has the typekey: ((1,1), '|u1')
    Size is (2048, 1024)

    Finished loading model!
    (16384, 8)
    <i8
    mode: L
    typekey: ((1, 1), '|u1')
    size: (2048, 1024)
  */

  // Because of mode L
  int ndmax = 2;

  // Width
  int64_t shape_1 = tensor.size(1);

  // Height
  int64_t shape_0 = tensor.size(0);
};
