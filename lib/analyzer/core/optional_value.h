#ifndef OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H
#define OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H


#include <functional>
#include <optional>
#include <unordered_map>

#include <z3++.h>

namespace clsma {

    class optional_value {
    public:
        optional_value();
        explicit(false) optional_value(z3::expr value);
        explicit(false) optional_value(std::optional<z3::expr> value);
        optional_value(std::optional<z3::expr> value, std::unordered_map<std::string, z3::expr> metadata);

        [[nodiscard]] bool has_value() const;
        [[nodiscard]] const z3::expr& value() const;
        void set_value(std::optional<z3::expr> value);
        clsma::optional_value map_value(const std::function<std::optional<z3::expr>(z3::expr)>& mapper) const;

        [[nodiscard]] const std::unordered_map<std::string, z3::expr>& metadata() const;
        [[nodiscard]] std::optional<z3::expr> metadata(const std::string& key) const;
        void set_metadata(const std::string& key, std::optional<z3::expr> value);
    private:
        std::optional<z3::expr> _value;
        std::unordered_map<std::string, z3::expr> _metadata;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H
