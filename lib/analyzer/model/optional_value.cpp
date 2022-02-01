#include "optional_value.h"

clsma::optional_value::optional_value() = default;

clsma::optional_value::optional_value(std::optional<z3::expr> value, std::unordered_map<std::string, z3::expr> meta) : _value(std::move(value)), _metadata(std::move(meta)) {}

const z3::expr& clsma::optional_value::value() const {
    return _value.value();
}

const std::unordered_map<std::string, z3::expr>& clsma::optional_value::metadata() const {
    return _metadata;
}

void clsma::optional_value::set_meta(const std::string& key, const std::optional<z3::expr>& data) {
    if (data) {
        _metadata.insert_or_assign(key, data.value());
    } else {
        _metadata.erase(key);
    }
}

std::optional<z3::expr> clsma::optional_value::get_meta(const std::string& key) const {
    if (const auto it = _metadata.find(key); it != _metadata.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool clsma::optional_value::has_value() const {
    return _value.has_value();
}

clsma::optional_value::optional_value(z3::expr value) : optional_value(std::move(value), {}) {}

clsma::optional_value::optional_value(std::optional<z3::expr> value) : optional_value(std::move(value), {}) {}

void clsma::optional_value::set_value(std::optional<z3::expr> value) {
    _value = std::move(value);
}
