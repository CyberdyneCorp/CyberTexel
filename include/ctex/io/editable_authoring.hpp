#ifndef CTEX_IO_EDITABLE_AUTHORING_HPP
#define CTEX_IO_EDITABLE_AUTHORING_HPP

#include <cstddef>
#include <ctex/doc/editable_authoring.hpp>
#include <ctex/io/project_container.hpp>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace ctex::io {

inline constexpr std::uint32_t current_editable_authoring_schema = 1;
inline constexpr std::string_view editable_authoring_asset_kind = "editable-authoring";

struct EditableAuthoringReadLimits {
    std::size_t maximum_input_bytes{256ULL << 20};
    std::size_t maximum_string_bytes{1ULL << 20};
    std::size_t maximum_entries{1'000'000};
    std::size_t maximum_parameters{16'000'000};
    std::size_t maximum_surface_points{16'000'000};
    std::size_t maximum_tile_dependencies{16'000'000};
};

class EditableAuthoringIoError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

[[nodiscard]] std::vector<std::byte> serialize_editable_authoring(
    const doc::EditableAuthoringStore& store);
[[nodiscard]] doc::EditableAuthoringStore deserialize_editable_authoring(
    std::span<const std::byte> serialized, EditableAuthoringReadLimits limits = {});
[[nodiscard]] StandaloneAsset package_editable_authoring(std::string identifier,
                                                         const doc::EditableAuthoringStore& store);
[[nodiscard]] doc::EditableAuthoringStore unpack_editable_authoring(
    const StandaloneAsset& asset, EditableAuthoringReadLimits limits = {});
void upsert_editable_authoring(ProjectContainer& project, std::string identifier,
                               const doc::EditableAuthoringStore& store);

}  // namespace ctex::io

#endif
