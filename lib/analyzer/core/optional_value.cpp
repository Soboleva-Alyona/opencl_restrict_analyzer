#include "optional_value.h"

clsma::optional_value::optional_value() = default;

clsma::optional_value::optional_value(z3::expr value) : optional_value(std::move(value), {}) {}

clsma::optional_value::optional_value(std::optional<z3::expr> value) : optional_value(std::move(value), {}) {}

clsma::optional_value::optional_value(std::optional<z3::expr> value, std::unordered_map<std::string, z3::expr> meta) : _value(std::move(value)), _metadata(std::move(meta)) {}

bool clsma::optional_value::has_value() const {
    return _value.has_value();
}

const z3::expr& clsma::optional_value::value() const {
    return _value.value();
}

void clsma::optional_value::set_value(std::optional<z3::expr> value) {
    _value = std::move(value);
}

clsma::optional_value clsma::optional_value::map_value(const std::function<std::optional<z3::expr>(z3::expr)>& mapper) const {
    if (has_value()) {
        return {mapper(value()), _metadata};
    }
    return *this;
}

const std::unordered_map<std::string, z3::expr>& clsma::optional_value::metadata() const {
    return _metadata;
}

std::optional<z3::expr> clsma::optional_value::metadata(const std::string& key) const {
    if (const auto it = _metadata.find(key); it != _metadata.end()) {
        return it->second;
    }
    return std::nullopt;
}

void clsma::optional_value::set_metadata(const std::string& key, std::optional<z3::expr> data) {
    if (data.has_value()) {
        _metadata.insert_or_assign(key, std::move(data.value()));
    } else {
        _metadata.erase(key);
    }
}
