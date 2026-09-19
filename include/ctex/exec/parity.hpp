#ifndef CTEX_EXEC_PARITY_HPP
#define CTEX_EXEC_PARITY_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>

namespace ctex::exec {

enum class ParityValueClass : std::uint8_t { unorm8, unorm16, floating_point };

struct ParityTolerance {
    double absolute{};
    double relative{};
    friend bool operator==(ParityTolerance, ParityTolerance) noexcept = default;
};

struct ParityFailure {
    std::size_t value_index{};
    double reference{};
    double measured{};
    double absolute_deviation{};
    double allowed_deviation{};
    std::string message;
};

struct ParityComparison {
    bool matches{};
    std::size_t compared_values{};
    double maximum_absolute_deviation{};
    std::optional<ParityFailure> first_failure;
};

[[nodiscard]] ParityTolerance parity_tolerance(ParityValueClass value_class,
                                               bool filtered) noexcept;
[[nodiscard]] ParityComparison compare_parity(std::span<const double> reference,
                                              std::span<const double> measured,
                                              ParityValueClass value_class, bool filtered);

}  // namespace ctex::exec

#endif
