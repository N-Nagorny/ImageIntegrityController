#pragma once

#include <cstddef>

#include <opencv2/core/core.hpp>

#define BITS_IN_BYTE 8

template<typename It>
size_t getHammingDistance(It v1, It v2, size_t length) {
  size_t distance = 0;

  for (int i = 0; i < length; i++, ++v1, ++v2)
    if (*v1 != *v2)
      distance++;

  return distance;
}

void signMat(cv::Mat* image_ptr, unsigned int block_side, unsigned int p);
cv::Mat checkMat(const cv::Mat& image, unsigned int block_side, unsigned int critical_distance);
