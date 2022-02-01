#ifndef OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H
#define OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H


#include <unordered_map>

#include <z3++.h>

namespace clsma {

    class optional_value {
    public:
        optional_value();
        explicit(false) optional_value(z3::expr value);
        explicit(false) optional_value(std::optional<z3::expr> value);
        optional_value(std::optional<z3::expr> value, std::unordered_map<std::string, z3::expr> metadata);

        void set_value(std::optional<z3::expr> value);
        [[nodiscard]] const z3::expr& value() const;
        [[nodiscard]] bool has_value() const;

        [[nodiscard]] const std::unordered_map<std::string, z3::expr>& metadata() const;

        void set_meta(const std::string& key, const std::optional<z3::expr>& data);
        [[nodiscard]] std::optional<z3::expr> get_meta(const std::string& key) const;
    private:
        std::optional<z3::expr> _value;
        std::unordered_map<std::string, z3::expr> _metadata;
    };

}


#endif //OPENCL_RESTRICT_ANALYZER_OPTIONAL_VALUE_H
