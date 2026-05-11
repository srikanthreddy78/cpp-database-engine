#include <algorithm> // For finding min and max values
#include <db/Query.hpp>
#include <numeric> // For calculations like sum

using namespace db;

// This function extracts a subset of fields from each tuple in the input DbFile
// and stores them in the output DbFile, based on a provided list of field names.
void db::projection(const DbFile &in, DbFile &out, const std::vector<std::string> &field_names) {
  const TupleDesc &desc = in.getTupleDesc();  // Cache descriptor for reuse
  std::vector<size_t> indices;

  // Precompute indices of fields to project for efficiency
  for (const auto &name : field_names) {
    indices.push_back(desc.index_of(name));
  }

  // Traverse all tuples in the input DbFile
  for (auto it = in.begin(); it != in.end(); ++it) {
    const Tuple &input_tuple = *it;
    std::vector<field_t> projected_fields;

    // Collect only the fields of interest based on precomputed indices
    for (size_t idx : indices) {
      projected_fields.emplace_back(input_tuple.get_field(idx));
    }

    // Form a new tuple with the selected fields and add it to the output DbFile
    out.insertTuple(Tuple(projected_fields));
  }
}

// This function filters tuples from the input DbFile based on a set of predicates.
// Only tuples that satisfy every predicate are copied to the output DbFile.
void db::filter(const DbFile &in, DbFile &out, const std::vector<FilterPredicate> &predicates) {
  const TupleDesc &desc = in.getTupleDesc();  // Tuple descriptor used for field lookups

  // Cache field indices for all predicates to avoid redundant lookups
  std::vector<size_t> indices;
  for (const auto &p : predicates) {
    indices.push_back(desc.index_of(p.field_name));
  }

  // Iterate over each tuple in the input
  for (const Tuple &tuple : in) {
    bool matches_all = std::equal(predicates.begin(), predicates.end(), indices.begin(),
      [&](const FilterPredicate &predicate, size_t idx) {
        const field_t &val = tuple.get_field(idx);

        switch (predicate.op) {
          case PredicateOp::EQ: return val == predicate.value;
          case PredicateOp::NE: return val != predicate.value;
          case PredicateOp::LT: return val < predicate.value;
          case PredicateOp::LE: return val <= predicate.value;
          case PredicateOp::GT: return val > predicate.value;
          case PredicateOp::GE: return val >= predicate.value;
        }
        return false; // default (should not be reached)
      });

    if (matches_all) {
      out.insertTuple(tuple); // Only insert if all predicates are satisfied
    }
  }
}
// Applies an aggregation function (e.g., SUM, AVG, MIN, etc.) over a selected field of the input tuples.
// If a grouping field is provided, the aggregation is computed separately for each group.
void db::aggregate(const DbFile &in, DbFile &out, const Aggregate &agg) {
  const TupleDesc &desc = in.getTupleDesc();
  size_t value_idx = desc.index_of(agg.field);
  std::unordered_map<field_t, std::vector<field_t>> group_values;

  // Collect target field values, organizing by group key if grouping is enabled
  for (const Tuple &tuple : in) {
    field_t group_key;
    if (agg.group.has_value()) {
      size_t group_idx = desc.index_of(*agg.group);
      group_key = tuple.get_field(group_idx);
    }

    group_values[group_key].emplace_back(tuple.get_field(value_idx));
  }

  // Iterate through grouped data and apply the aggregation operation
  for (auto &[key, values] : group_values) {
    field_t aggregate_result;

    auto is_int_type = std::holds_alternative<int>(values[0]);
    switch (agg.op) {
      case AggregateOp::SUM: {
        if (is_int_type) {
          int total = std::accumulate(values.begin(), values.end(), 0,
                        [](int acc, const field_t &f) { return acc + std::get<int>(f); });
          aggregate_result = total;
        } else {
          double total = std::accumulate(values.begin(), values.end(), 0.0,
                          [](double acc, const field_t &f) { return acc + std::get<double>(f); });
          aggregate_result = total;
        }
        break;
      }

      case AggregateOp::AVG: {
        if (is_int_type) {
          int total = std::accumulate(values.begin(), values.end(), 0,
                        [](int acc, const field_t &f) { return acc + std::get<int>(f); });
          aggregate_result = static_cast<double>(total) / values.size();
        } else {
          double total = std::accumulate(values.begin(), values.end(), 0.0,
                          [](double acc, const field_t &f) { return acc + std::get<double>(f); });
          aggregate_result = total / values.size();
        }
        break;
      }

      case AggregateOp::MIN:
        aggregate_result = *std::min_element(values.begin(), values.end());
        break;

      case AggregateOp::MAX:
        aggregate_result = *std::max_element(values.begin(), values.end());
        break;

      case AggregateOp::COUNT:
        aggregate_result = static_cast<int>(values.size());
        break;
    }

    // Construct and insert the resulting tuple with or without group key
    if (agg.group.has_value()) {
      out.insertTuple(Tuple({key, aggregate_result}));
    } else {
      out.insertTuple(Tuple({aggregate_result}));
    }
  }
}

// Executes a join between two input DbFiles based on a condition between fields.
// When tuples from both files satisfy the predicate, their fields are merged into a result tuple.
void db::join(const DbFile &left, const DbFile &right, DbFile &out, const JoinPredicate &pred) {
  const TupleDesc &left_desc = left.getTupleDesc();
  const TupleDesc &right_desc = right.getTupleDesc();
  size_t left_field_index = left_desc.index_of(pred.left);
  size_t right_field_index = right_desc.index_of(pred.right);

  // Loop over all tuples from the left DbFile
  for (const Tuple &left_tuple : left) {
    const field_t &left_value = left_tuple.get_field(left_field_index);

    // For each tuple in the left, scan all tuples from the right
    for (const Tuple &right_tuple : right) {
      const field_t &right_value = right_tuple.get_field(right_field_index);
      bool matched = false;

      // Evaluate the join condition
      switch (pred.op) {
        case PredicateOp::EQ: matched = (left_value == right_value); break;
        case PredicateOp::NE: matched = (left_value != right_value); break;
        case PredicateOp::LT: matched = (left_value < right_value); break;
        case PredicateOp::LE: matched = (left_value <= right_value); break;
        case PredicateOp::GT: matched = (left_value > right_value); break;
        case PredicateOp::GE: matched = (left_value >= right_value); break;
      }

      if (matched) {
        // Merge fields manually since Tuple doesn't support iteration
        std::vector<field_t> joined_fields;

        for (size_t i = 0; i < left_tuple.size(); ++i) {
          joined_fields.push_back(left_tuple.get_field(i));
        }

        for (size_t i = 0; i < right_tuple.size(); ++i) {
          // Avoid repeating join field for equality joins
          if (pred.op != PredicateOp::EQ || i != right_field_index) {
            joined_fields.push_back(right_tuple.get_field(i));
          }
        }

        out.insertTuple(Tuple(joined_fields));  // Add resulting tuple to output
      }
    }
  }
}