#include <cstring>
#include <numeric>
#include <db/Tuple.hpp>
#include <stdexcept>
#include <unordered_set>

using namespace db;

Tuple::Tuple(const std::vector<field_t> &fields) : fields(fields) {}

type_t Tuple::field_type(size_t i) const {
    const field_t &field = fields.at(i);
    if (std::holds_alternative<int>(field)) {
        return type_t::INT;
    }
    if (std::holds_alternative<double>(field)) {
        return type_t::DOUBLE;
    }
    if (std::holds_alternative<std::string>(field)) {
        return type_t::CHAR;
    }
    throw std::logic_error("Unknown field type");
}

size_t Tuple::size() const { return fields.size(); }

const field_t &Tuple::get_field(size_t i) const { return fields.at(i); }
TupleDesc::TupleDesc(const std::vector<type_t> &types, const std::vector<std::string> &names) {
    // Validate that types and names vectors match in size
    if (types.size() != names.size()) {
        throw std::invalid_argument("Mismatch between types and names count");
    }

    // Initialize structures
    this->types.reserve(types.size());
    this->offsets.reserve(types.size());
    size_t currentOffset = 0;

    std::unordered_set<std::string> uniqueNames;  // To check for duplicate names

    for (size_t i = 0; i < types.size(); ++i) {
        // Ensure unique attribute names
        if (!uniqueNames.insert(names[i]).second) {
            throw std::invalid_argument("Duplicate attribute name: " + names[i]);
        }

        // Store type and offset mapping
        this->types.push_back(types[i]);
        offsets.push_back(currentOffset);
        name_to_index[names[i]] = i;

        // Compute next offset based on type size
        currentOffset += getTypeSize(types[i]);
    }
}

// Helper function to get the size of a type
size_t TupleDesc::getTypeSize(type_t type) const {
    static const std::unordered_map<type_t, size_t> typeSizes = {
        {type_t::INT, INT_SIZE},
        {type_t::DOUBLE, DOUBLE_SIZE},
        {type_t::CHAR, CHAR_SIZE}
    };

    auto it = typeSizes.find(type);
    if (it == typeSizes.end()) {
        throw std::runtime_error("Unknown type in TupleDesc");
    }
    return it->second;
}

bool TupleDesc::compatible(const Tuple &tuple) const {
    if (tuple.size() != types.size()) {
        return false;
    }

    for (size_t i = 0; i < tuple.size(); i++) {
        if (tuple.field_type(i) != types[i]) {
            return false;
        }
    }

    return true;
}

size_t TupleDesc::offset_of(const size_t &index) const { return offsets.at(index); }

size_t TupleDesc::index_of(const std::string &name) const { return name_to_index.at(name); }

size_t TupleDesc::length() const {
    return std::accumulate(types.begin(), types.end(), static_cast<size_t>(0),
                           [](size_t total, type_t type) {
                               static const std::unordered_map<type_t, size_t> typeSizes = {
                                   {type_t::INT, INT_SIZE},
                                   {type_t::DOUBLE, DOUBLE_SIZE},
                                   {type_t::CHAR, CHAR_SIZE}
                               };
                               return total + typeSizes.at(type);
                           });
}

size_t TupleDesc::size() const { return types.size(); }

Tuple TupleDesc::deserialize(const uint8_t *data) const {
    std::vector<field_t> fields;
    fields.reserve(types.size());
    for (const type_t &type : types) {
        switch (type) {
            case type_t::INT:
                fields.emplace_back(*reinterpret_cast<const int *>(data));
            data += INT_SIZE;
            break;
            case type_t::DOUBLE:
                fields.emplace_back(*reinterpret_cast<const double *>(data));
            data += DOUBLE_SIZE;
            break;
            case type_t::CHAR:
                fields.emplace_back(std::string(reinterpret_cast<const char *>(data)));
            data += CHAR_SIZE;
            break;
        }
    }
    return {fields};
}

void TupleDesc::serialize(uint8_t *data, const Tuple &t) const {
    size_t offset = 0;
    for (size_t i = 0; i < types.size(); ++i) {
        switch (types[i]) {
            case type_t::INT:
                std::memcpy(data + offset, &std::get<int>(t.get_field(i)), INT_SIZE);
                offset += INT_SIZE;
                break;
            case type_t::DOUBLE:
                std::memcpy(data + offset, &std::get<double>(t.get_field(i)), DOUBLE_SIZE);
                offset += DOUBLE_SIZE;
                break;
            case type_t::CHAR: {
                const std::string &str = std::get<std::string>(t.get_field(i));
                std::fill(data + offset, data + offset + CHAR_SIZE, '\0');  // Ensure null termination
                std::memcpy(data + offset, str.c_str(), std::min(str.size(), static_cast<size_t>(CHAR_SIZE - 1)));
                offset += CHAR_SIZE;
                break;
            }
        }
    }
}

db::TupleDesc TupleDesc::merge(const TupleDesc &td1, const TupleDesc &td2) {
    size_t totalSize = td1.types.size() + td2.types.size();
    std::vector<type_t> types;
    std::vector<std::string> names;

    types.reserve(totalSize);
    names.reserve(totalSize);

    for (size_t i = 0; i < td1.types.size(); ++i) {
        types.push_back(td1.types[i]);
        for (const auto &[name, index] : td1.name_to_index) {
            if (index == i) {
                names.push_back(name);
                break;
            }
        }
    }

    for (size_t i = 0; i < td2.types.size(); ++i) {
        types.push_back(td2.types[i]);
        for (const auto &[name, index] : td2.name_to_index) {
            if (index == i) {
                names.push_back(name);
                break;
            }
        }
    }

    return db::TupleDesc(types, names);
}
