#ifndef CTEX_GRAPH_MATERIAL_LIBRARY_HPP
#define CTEX_GRAPH_MATERIAL_LIBRARY_HPP

#include <ctex/graph/document.hpp>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::graph {

struct MaterialPreset {
    std::string stable_id;
    std::string name;
    std::string thumbnail_resource;
    GraphDocument graph;
    friend bool operator==(const MaterialPreset&, const MaterialPreset&) = default;
};

class MaterialLibraryError final : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};

class MaterialLibrary {
public:
    void add(MaterialPreset preset);

    [[nodiscard]] const MaterialPreset* find(std::string_view stable_id) const noexcept;
    [[nodiscard]] GraphDocument instantiate(std::string_view stable_id) const;
    [[nodiscard]] std::span<const MaterialPreset> presets() const noexcept { return presets_; }

    friend bool operator==(const MaterialLibrary&, const MaterialLibrary&) = default;

private:
    std::vector<MaterialPreset> presets_;
};

// Canonical, versioned, device-independent library representation. Presets are
// ordered by stable identity so insertion order cannot affect serialized bytes.
[[nodiscard]] std::string serialize_material_library(const MaterialLibrary& library);
[[nodiscard]] MaterialLibrary deserialize_material_library(std::string_view serialized);

}  // namespace ctex::graph

#endif
