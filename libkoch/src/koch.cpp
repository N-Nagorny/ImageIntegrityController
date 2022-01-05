#include "koch.hpp"

#include <utility>
#include <vector>

#include <opencv2/core/core.hpp>

// "private" functions
namespace {
/**
 * Returns the start of a diagonal of a matrix.
 *
 * @param diagonal diagonal of a square matrix
 * @returns start point
 */
cv::Point2i getStartOfDiagonal(const Diagonal& diagonal) {
  cv::Point2i start;

  if (diagonal.index >= 0) {
    start.x = diagonal.index;
    start.y = diagonal.matrix_side - 1;
  }
  else {
    start.x = 0;
    start.y = diagonal.matrix_side - 1 - abs(diagonal.index);
  }

  return start;
}

/**
 * Returns the point on a diagonal of a matrix.
 *
 * @param diagonal diagonal of a square matrix
 * @param idx index of a matrix item on the diagonal
 * (where 0 is the left bottom corner)
 * @returns point
 */
cv::Point2i getPointOnDiagonal(const Diagonal& diagonal, int idx) {
  cv::Point2i start = getStartOfDiagonal(diagonal);
  return cv::Point2i{
    start.x + idx,
    start.y - idx
  };
}

/**
 * Returns the main diagonal of a matrix.
 *
 * @param matrix_side side of a square matrix
 * @returns main diagonal
 */
Diagonal getMainDiagonal(unsigned int matrix_side) {
  return Diagonal{
    0,
    matrix_side,
    matrix_side
  };
}

/**
 * Returns the next diagonal going from central
 * diagonal to the right neighbour, then to
 * the left neighbour and so on.
 *
 * @param diagonal current diagonal
 * @returns next diagonal
 */
Diagonal getNextDiagonal(const Diagonal& diagonal) {
  int old_index = diagonal.index;
  int new_index = 0;

  if (old_index == 0) {
    new_index = 1;
  }
  else if (old_index > 0) {
    new_index = -old_index;
  }
  else {
    new_index = -old_index + 1;
  }

  return Diagonal{
    new_index,
    diagonal.matrix_side - abs(new_index),
    diagonal.matrix_side
  };
}

/**
 * Extracts a single bit from a pair of DCT matrix
 * coefficients.
 *
 * @param dct_mat DCT matrix
 * @param coeff_idx_1 index of the first coefficient for extracting
 * @param coeff_idx_2 index of the second coefficient for extracting
 * @returns extracted bit
 */
bool extractBit(const cv::Mat& dct_mat, const cv::Point2i& coeff_idx_1, const cv::Point2i& coeff_idx_2) {
  auto [x1, y1] = coeff_idx_1;
  auto [x2, y2] = coeff_idx_2;
  const double& coeff_1 = dct_mat.at<double>(y1, x1);
  const double& coeff_2 = dct_mat.at<double>(y2, x2);

  return abs(coeff_2) > abs(coeff_1);
}

/**
 * Embeds a single bit into a pair of DCT matrix
 * coefficients.
 *
 * @param bit bit for embedding
 * @param dct_mat_ptr pointer to a DCT matrix with floating-point elements
 * @param coeff_idx_1 index of the first coefficient for embedding
 * @param coeff_idx_2 index of the second coefficient for embedding
 * @param p noise margin factor, see [1]
 */
void embedBit(bool bit, cv::Mat* dct_mat_ptr, const cv::Point2i& coeff_idx_1, const cv::Point2i& coeff_idx_2, unsigned int p) {
  auto [x1, y1] = coeff_idx_1;
  auto [x2, y2] = coeff_idx_2;
  double& coeff_1 = dct_mat_ptr->at<double>(y1, x1);
  double& coeff_2 = dct_mat_ptr->at<double>(y2, x2);

  if (bit) {
    while (abs(coeff_2) - abs(coeff_1) <= p) {
      if ((coeff_1 >= 0 && coeff_2 >= 0) ||
          (coeff_1 <= 0 && coeff_2 <= 0)) {
            coeff_1 -= 1.0;
            coeff_2 += 1.0;
      }
      else if (coeff_1 < 0) {
        coeff_1 += 1.0;
        coeff_2 += 1.0;
      }
      else {
        coeff_1 -= 1.0;
        coeff_2 -= 1.0;
      }
    }
  }
  else {
    while (abs(coeff_1) - abs(coeff_2) <= p) {
      if ((coeff_1 >= 0 && coeff_2 >= 0) ||
          (coeff_1 <= 0 && coeff_2 <= 0)) {
            coeff_1 += 1.0;
            coeff_2 -= 1.0;
      }
      else if (coeff_1 < 0) {
        coeff_1 -= 1.0;
        coeff_2 -= 1.0;
      }
      else {
        coeff_1 += 1.0;
        coeff_2 += 1.0;
      }
    }
  }
}
} // end of the anonymous namespace

/**
 * Calculates DCT from an image carrier and embeds each bit of
 * a payload message into two DCT coefficients placed in middle
 * frequency range (diagonals of the DCT matrix).
 *
 * @param image_ptr pointer to an image which plays the role of a carrier
 * @param payload payload message
 * @param p noise margin factor, see [1]
 */
void Koch::embed(cv::Mat* image_ptr, const std::vector<bool>& payload, unsigned int p) {
  if (image_ptr->cols != image_ptr->rows)
    throw std::logic_error("Image is not a square.");
  // It's an assumption that the three central diagonals of DCT matrix
  // are optimal in terms of robustness and invisibility.
  size_t max_payload_size = (image_ptr->cols + 2 * (image_ptr->cols - 1) + 2 * (image_ptr->cols - 2)) / 2;
  if (payload.size() > max_payload_size)
    throw std::logic_error("The payload message is too long for image side " + std::to_string(image_ptr->cols));

  cv::Mat segment_dct;
  cv::dct(*image_ptr, segment_dct);

  Diagonal diagonal = getMainDiagonal(image_ptr->cols);
  unsigned int idx = 0;

  for (bool bit : payload) {
    cv::Point2i coeff_idx_1 = getPointOnDiagonal(diagonal, idx++);

    if (idx == diagonal.end) {
      diagonal = getNextDiagonal(diagonal);
      idx = 0;
    }

    cv::Point2i coeff_idx_2 = getPointOnDiagonal(diagonal, idx++);

    if (idx == diagonal.end) {
      diagonal = getNextDiagonal(diagonal);
      idx = 0;
    }

    embedBit(bit, &segment_dct, coeff_idx_1, coeff_idx_2, p);
  }

  cv::dct(segment_dct, *image_ptr, cv::DCT_INVERSE);
}

/**
 * Extracts a payload from an image package.
 *
 * @param image image which plays the role of a package
 * @param payload_length payload_length
 * @returns payload message
 */
std::vector<bool> Koch::extract(const cv::Mat& image, unsigned int payload_length) {
  if (image.cols != image.rows)
    throw std::logic_error("Image is not a square.");

  std::vector<bool> payload_result;

  cv::Mat segment_dct;
  cv::dct(image, segment_dct);

  Diagonal diagonal = getMainDiagonal(image.cols);
  unsigned int idx = 0;

  for (unsigned int i = 0; i < payload_length; ++i) {
    cv::Point2i coeff_idx_1 = getPointOnDiagonal(diagonal, idx++);

    if (idx == diagonal.end) {
      diagonal = getNextDiagonal(diagonal);
      idx = 0;
    }

    cv::Point2i coeff_idx_2 = getPointOnDiagonal(diagonal, idx++);

    if (idx == diagonal.end) {
      diagonal = getNextDiagonal(diagonal);
      idx = 0;
    }

    payload_result.push_back(extractBit(segment_dct, coeff_idx_1, coeff_idx_2));
  }

  return payload_result;
}
