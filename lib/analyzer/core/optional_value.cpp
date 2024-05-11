#include "optional_value.h"

clsa::optional_value::optional_value() = default;

clsa::optional_value::optional_value(z3::expr value) : optional_value(std::move(value), {}) {}

clsa::optional_value::optional_value(std::optional<z3::expr> value) : optional_value(std::move(value), {}) {}

clsa::optional_value::optional_value(std::optional<z3::expr> value, std::unordered_map<std::string, z3::expr> meta)
    : _value(std::move(value)), _metadata(std::move(meta)) {}

clsa::optional_value::operator std::optional<z3::expr>() const {
    return _value;
}

bool clsa::optional_value::has_value() const {
    return _value.has_value();
}

const z3::expr& clsa::optional_value::value() const {
    return _value.value();
}

clsa::optional_value clsa::optional_value::copy_value() const {
    if (_value_copy.has_value())
    {
        return {_value_copy};
    }
    if (_value.has_value())
    {
        return {_value};
    }
    return{};
}

void clsa::optional_value::set_value(std::optional<z3::expr> value) {
    _value = std::move(value);
}

clsa::optional_value clsa::optional_value::map_value(const std::function<std::optional<z3::expr>(z3::expr)>& mapper) const {
    if (has_value()) {
        return {mapper(value()), _metadata};
    }
    return *this;
}

const std::unordered_map<std::string, z3::expr>& clsa::optional_value::metadata() const {
    return _metadata;
}

std::optional<z3::expr> clsa::optional_value::metadata(const std::string& key) const {
    if (const auto it = _metadata.find(key); it != _metadata.end()) {
        return it->second;
    }
    return std::nullopt;
}

void clsa::optional_value::set_metadata(const std::string& key, std::optional<z3::expr> data) {
    if (data.has_value()) {
        _metadata.insert_or_assign(key, std::move(data.value()));
    } else {
        _metadata.erase(key);
    }
}

void clsa::optional_value::set_value_copy(std::optional<z3::expr> value_copy)
{
    _value_copy = std::move(value_copy);
}

