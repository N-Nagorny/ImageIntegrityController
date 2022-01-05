#pragma once

#include <vector>

#include <opencv2/core/core.hpp>

// Main diagonal has index 0,
// right diagonals have positive numbers,
// left diagonals have negative numbers.
struct Diagonal {
  int index;
  unsigned int end;
  unsigned int matrix_side;
};

namespace Koch {
  void embed(cv::Mat* image_ptr, const std::vector<bool>& payload, unsigned int p);
  std::vector<bool> extract(const cv::Mat& image, unsigned int payload_length);
}