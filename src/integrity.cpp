#include "integrity.hpp"
#include "koch.hpp"

#include <vector>

#include <opencv2/core/core.hpp>
#include <opencv2/img_hash.hpp>

std::vector<bool> getPHash(const cv::Mat& image) {
  std::vector<bool> result;

  std::array<uint8_t, 8> hash;
  cv::Ptr<cv::img_hash::ImgHashBase> phash_ptr = cv::img_hash::PHash::create();
  phash_ptr->compute(image, hash);
  for (uint8_t byte : hash) {
    for (unsigned int i = 0; i < BITS_IN_BYTE; ++i) {
      result.push_back((byte >> i) & true);
    }
  }

  return result;
}

/**
 * Calculates pHash of an image and embeds the result
 * into it with steganography.
 *
 * @param image_ptr pointer to an image
 * @param block_side side of a square block
 * which the image is split into (in pixels)
 * @param p noise margin factor, see [1]
 */
void signMat(cv::Mat* image_ptr, unsigned int block_side, unsigned int p) {
  if (image_ptr->channels() != 3)
    throw std::logic_error(std::string(__FUNCTION__) +
      ": Channel count " + std::to_string(image_ptr->channels()) + " is not supported.");

  size_t complete_segments_i = image_ptr->cols / block_side;
  size_t complete_segments_j = image_ptr->rows / block_side;

  for (unsigned int i = 0; i < complete_segments_i; ++i) {
    for (unsigned int j = 0; j < complete_segments_j; ++j) {
      if (i == complete_segments_i - 1 && j == complete_segments_j - 1) {
        break;
      }

      cv::Point2i hashed_point{
        static_cast<int>(block_side * i),
        static_cast<int>(block_side * j)
      };

      cv::Point2i modified_point{
        static_cast<int>(block_side * i),
        static_cast<int>(block_side * (j + 1))
      };

      if (j == complete_segments_j - 1) {
        modified_point = cv::Point2i{
          static_cast<int>(block_side * (i + 1)),
          static_cast<int>(0)
        };
      }

      cv::Rect hashed_roi(hashed_point.x, hashed_point.y, block_side, block_side);
      cv::Rect modified_roi(modified_point.x, modified_point.y, block_side, block_side);

      cv::Mat hashed_block = (*image_ptr)(hashed_roi);
      cv::Mat modified_block = (*image_ptr)(modified_roi);

      std::vector<bool> stego_payload = getPHash(hashed_block);

      std::vector<cv::Mat> channels(3);
      cv::split(modified_block, channels);
      channels[0].convertTo(channels[0], CV_64F);

      Koch::embed(&channels[0], stego_payload, p);

      channels[0].convertTo(channels[0], CV_8U);
      cv::merge(channels, modified_block);
    }
  }
}

cv::Mat checkMat(const cv::Mat& image, unsigned int block_side, unsigned int critical_distance) {
  cv::Mat result = image;

  if (result.channels() != 3)
    throw std::logic_error(std::string(__FUNCTION__) +
      ": Channel count " + std::to_string(result.channels()) + " is not supported.");

  size_t complete_segments_i = result.cols / block_side;
  size_t complete_segments_j = result.rows / block_side;

  for (unsigned int i = 0; i < complete_segments_i; ++i) {
    for (unsigned int j = 0; j < complete_segments_j; ++j) {
      if (i == complete_segments_i - 1 && j == complete_segments_j - 1) {
        break;
      }

      cv::Point2i hashed_point{
        static_cast<int>(block_side * i),
        static_cast<int>(block_side * j)
      };

      cv::Point2i modified_point{
        static_cast<int>(block_side * i),
        static_cast<int>(block_side * (j + 1))
      };

      if (j == complete_segments_j - 1) {
        modified_point = cv::Point2i{
          static_cast<int>(block_side * (i + 1)),
          static_cast<int>(0)
        };
      }

      cv::Rect hashed_roi(hashed_point.x, hashed_point.y, block_side, block_side);
      cv::Rect modified_roi(modified_point.x, modified_point.y, block_side, block_side);

      cv::Mat hashed_block = result(hashed_roi);
      cv::Mat modified_block = result(modified_roi);

      std::vector<bool> original_hash = getPHash(hashed_block);

      std::vector<cv::Mat> modified_channels(3);
      cv::split(modified_block, modified_channels);
      modified_channels[0].convertTo(modified_channels[0], CV_64F);

      std::vector<bool> actual_hash = Koch::extract(modified_channels[0], original_hash.size());
      unsigned int distance =
        getHammingDistance(original_hash.begin(), actual_hash.begin(), original_hash.size());

      if (distance > critical_distance) {
        std::vector<cv::Mat> hashed_channels(3);
        cv::split(hashed_block, hashed_channels);

        for (int k = 0; k < block_side; k++)
          for (int l = 0; l < block_side; l++)
            hashed_channels[0].at<uint8_t>(l, k) = 0;

        cv::merge(hashed_channels, hashed_block);
      }
    }
  }

  return result;
}
