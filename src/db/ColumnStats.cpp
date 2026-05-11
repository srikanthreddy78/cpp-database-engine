
// This file implements a histogram-based class to track value distributions in a column.
// It provides methods to add values and estimate the cardinality of query results for
//  various predicates, enabling efficient query optimization.
#include <algorithm>
#include <cmath>
#include <db/ColumnStats.hpp>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace db {

  // Constructor: Initializes the histogram for tracking the distribution of integer values
  // across a fixed number of buckets, defined over an inclusive range [min, max].
  ColumnStats::ColumnStats(unsigned buckets, int min, int max)
      : buckets(buckets), min(min), max(max),
        histogram(std::vector<unsigned long>::size_type(buckets), 0),
        count(0) {

    // Validate input parameters to ensure a usable configuration
    if (buckets == 0 || min >= max) {
      throw std::invalid_argument("Bucket count must be > 0 and min must be less than max");
    }

    // Calculate width of each bucket so that the full range [min, max] is covered
    // Ceiling ensures all values from min to max can be captured across the buckets
    bucketWidth = static_cast<int>(std::ceil((max - min + 1.0) / buckets));
  }

  // Adds a value to the histogram, incrementing the appropriate bucket and total count.
  // Values outside the defined range [min, max] are ignored.
  void ColumnStats::addValue(int v) {
    // Discard values outside the histogram's range
    if (v < min || v > max) {
      return;
    }

    // Calculate which bucket the value belongs to
    size_t bucketIndex = static_cast<size_t>((v - min) / bucketWidth);

    // Clamp to last valid bucket index in case of rounding issues or edge cases
    bucketIndex = std::min(bucketIndex, histogram.size() - 1);

    // Increment the corresponding bucket and overall value count
    histogram[bucketIndex]++;
    count++;
  }

// Estimates the number of tuples that satisfy a given predicate for a specific column.
// The estimate is based on a histogram approximation of the column's value distribution.
size_t ColumnStats::estimateCardinality(PredicateOp op, int v) const {
  // If the column contains no values, any condition results in 0 matches
  if (count == 0) {
    return 0;
  }

  // Quickly resolve queries for values clearly out of the min-max range
  if (v < min) {
    // All values are greater than v if v is less than the minimum
    return (op == PredicateOp::GT || op == PredicateOp::GE) ? count : 0;
  }
  if (v > max) {
    // All values are less than v if v is greater than the maximum
    return (op == PredicateOp::LT || op == PredicateOp::LE) ? count : 0;
  }

  // Determine which bucket the value `v` falls into
  size_t bucketIndex = static_cast<size_t>((v - min) / bucketWidth);
  bucketIndex = std::min(bucketIndex, histogram.size() - 1); // Clamp to valid index range

  // Gather bucket-specific metadata
  double bucketCount = static_cast<double>(histogram[bucketIndex]); // Number of values in the bucket
  int bucketStart = min + static_cast<int>(bucketIndex * bucketWidth);
  int bucketEnd = bucketStart + static_cast<int>(bucketWidth) - 1;

  // Used to store partial bucket contribution
  double fraction = 0.0;

  switch (op) {
    case PredicateOp::EQ:
      // Estimate equal value count based on uniform distribution within the bucket
      return bucketCount == 0 ? 0 : static_cast<size_t>(bucketCount / bucketWidth);

    case PredicateOp::NE:
      // Estimate non-equal by subtracting EQ estimate from total count
      return bucketCount == 0 ? count : count - static_cast<size_t>(bucketCount / bucketWidth);

    case PredicateOp::LT:
      // If v falls inside the bucket, calculate fractional contribution
      if (v > bucketStart) {
        fraction = static_cast<double>(v - bucketStart) / bucketWidth;
      }
      return static_cast<size_t>(fraction * bucketCount) +
             std::accumulate(histogram.begin(), histogram.begin() + bucketIndex, 0UL);

    case PredicateOp::LE:
      // Include the value itself by expanding the fractional range
      if (v >= bucketStart) {
        fraction = static_cast<double>(v - bucketStart + 1) / bucketWidth;
      }
      return static_cast<size_t>(fraction * bucketCount) +
             std::accumulate(histogram.begin(), histogram.begin() + bucketIndex, 0UL);

    case PredicateOp::GT:
      // If v is within this bucket, calculate right-side fractional estimate
      if (v < bucketEnd) {
        fraction = static_cast<double>(bucketEnd - v) / bucketWidth;
      }
      return static_cast<size_t>(fraction * bucketCount) +
             std::accumulate(histogram.begin() + bucketIndex + 1, histogram.end(), 0UL);

    case PredicateOp::GE:
      // Estimate greater-than-or-equal by including current and all higher buckets
      if (bucketCount > 0 && v <= bucketEnd) {
        fraction = static_cast<double>(bucketEnd - v + 1) / bucketWidth;
        fraction = std::clamp(fraction, 0.0, 1.0);
      } else {
        fraction = 0.0;
      }
      return static_cast<size_t>(fraction * bucketCount) +
             std::accumulate(histogram.begin() + bucketIndex + 1, histogram.end(), 0UL);

    default:
      throw std::logic_error("Unsupported PredicateOp in estimateCardinality");
  }
}

}
