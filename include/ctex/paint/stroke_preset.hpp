#ifndef CTEX_PAINT_STROKE_PRESET_HPP
#define CTEX_PAINT_STROKE_PRESET_HPP

#include <cstdint>
#include <ctex/paint/stroke.hpp>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ctex::paint {

inline constexpr std::uint32_t current_stroke_preset_schema_version = 2;

struct StrokePreset {
    std::uint32_t schema_version{current_stroke_preset_schema_version};
    std::string name;
    StrokeSettings settings;
    friend bool operator==(const StrokePreset&, const StrokePreset&) = default;
};

class StrokePresetError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

[[nodiscard]] std::string serialize_stroke_preset(const StrokePreset& preset);
[[nodiscard]] StrokePreset deserialize_stroke_preset(std::string_view serialized);

}  // namespace ctex::paint

#endif
