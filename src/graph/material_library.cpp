#include <algorithm>
#include <ctex/graph/material_library.hpp>
#include <utility>

namespace ctex::graph {
namespace {

constexpr char hex_digits[] = "0123456789abcdef";

std::string encode_bytes(std::string_view value) {
    std::string result;
    result.reserve(value.size() * 2);
    for (const unsigned char byte : value) {
        result.push_back(hex_digits[byte >> 4U]);
        result.push_back(hex_digits[byte & 0x0fU]);
    }
    return result;
}

std::uint8_t hex_nibble(char value) {
    if (value >= '0' && value <= '9') {
        return static_cast<std::uint8_t>(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return static_cast<std::uint8_t>(value - 'a' + 10);
    }
    throw MaterialLibraryError("material library contains invalid hexadecimal data");
}

std::string decode_bytes(std::string_view encoded) {
    if (encoded.size() % 2 != 0) {
        throw MaterialLibraryError("material library contains an odd hexadecimal string");
    }
    std::string result;
    result.reserve(encoded.size() / 2);
    for (std::size_t index = 0; index < encoded.size(); index += 2) {
        result.push_back(
            static_cast<char>((hex_nibble(encoded[index]) << 4U) | hex_nibble(encoded[index + 1])));
    }
    return result;
}

std::vector<std::string_view> split(std::string_view value, char delimiter) {
    std::vector<std::string_view> result;
    std::size_t begin = 0;
    while (true) {
        const std::size_t end = value.find(delimiter, begin);
        if (end == std::string_view::npos) {
            result.push_back(value.substr(begin));
            return result;
        }
        result.push_back(value.substr(begin, end - begin));
        begin = end + 1;
    }
}

void validate_preset(const MaterialPreset& preset) {
    if (preset.stable_id.empty() || preset.name.empty()) {
        throw MaterialLibraryError("material preset requires a stable identity and name");
    }
}

}  // namespace

void MaterialLibrary::add(MaterialPreset preset) {
    validate_preset(preset);
    const auto position =
        std::lower_bound(presets_.begin(), presets_.end(), preset.stable_id,
                         [](const MaterialPreset& candidate, std::string_view stable_id) {
                             return candidate.stable_id < stable_id;
                         });
    if (position != presets_.end() && position->stable_id == preset.stable_id) {
        throw MaterialLibraryError("material library repeats stable identity '" + preset.stable_id +
                                   "'");
    }
    presets_.insert(position, std::move(preset));
}

const MaterialPreset* MaterialLibrary::find(std::string_view stable_id) const noexcept {
    const auto position =
        std::lower_bound(presets_.begin(), presets_.end(), stable_id,
                         [](const MaterialPreset& candidate, std::string_view sought) {
                             return candidate.stable_id < sought;
                         });
    return position == presets_.end() || position->stable_id != stable_id ? nullptr : &*position;
}

GraphDocument MaterialLibrary::instantiate(std::string_view stable_id) const {
    const MaterialPreset* preset = find(stable_id);
    if (preset == nullptr) {
        throw MaterialLibraryError("material preset does not exist: " + std::string(stable_id));
    }
    return preset->graph.clone();
}

std::string serialize_material_library(const MaterialLibrary& library) {
    std::string result = "CTEX_MATERIAL_LIBRARY\t1\n";
    for (const MaterialPreset& preset : library.presets()) {
        result.append("PRESET\t");
        result.append(encode_bytes(preset.stable_id));
        result.push_back('\t');
        result.append(encode_bytes(preset.name));
        result.push_back('\t');
        result.append(encode_bytes(preset.thumbnail_resource));
        result.push_back('\t');
        result.append(encode_bytes(serialize_graph(preset.graph)));
        result.push_back('\n');
    }
    result.append("END\n");
    return result;
}

MaterialLibrary deserialize_material_library(std::string_view serialized) {
    const std::vector<std::string_view> records = split(serialized, '\n');
    if (records.size() < 3 || records.front() != "CTEX_MATERIAL_LIBRARY\t1" ||
        records[records.size() - 2] != "END" || !records.back().empty()) {
        throw MaterialLibraryError("material library has an unsupported or malformed envelope");
    }
    MaterialLibrary result;
    for (std::size_t index = 1; index + 2 < records.size(); ++index) {
        const std::vector<std::string_view> fields = split(records[index], '\t');
        if (fields.size() != 5 || fields[0] != "PRESET") {
            throw MaterialLibraryError("material library contains a malformed preset record");
        }
        try {
            result.add({decode_bytes(fields[1]), decode_bytes(fields[2]), decode_bytes(fields[3]),
                        deserialize_graph(decode_bytes(fields[4]))});
        } catch (const MaterialLibraryError&) {
            throw;
        } catch (const std::exception& error) {
            throw MaterialLibraryError("material preset graph is invalid: " +
                                       std::string(error.what()));
        }
    }
    return result;
}

}  // namespace ctex::graph
