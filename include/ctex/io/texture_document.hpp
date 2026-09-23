#ifndef CTEX_IO_TEXTURE_DOCUMENT_HPP
#define CTEX_IO_TEXTURE_DOCUMENT_HPP

#include <cstddef>
#include <ctex/doc/document.hpp>
#include <ctex/io/project_container.hpp>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::io {

inline constexpr std::uint32_t current_texture_document_schema = 1;
inline constexpr std::string_view texture_document_asset_kind = "texture-document";

struct TextureDocumentReadLimits {
    std::size_t maximum_payload_bytes{256ULL << 20};
    std::size_t maximum_string_bytes{1ULL << 20};
    std::size_t maximum_texture_sets{1'000'000};
    std::size_t maximum_channels{16'000'000};
    std::size_t maximum_udim_tiles{16'000'000};
    std::size_t maximum_layer_entries{16'000'000};
    std::size_t maximum_channel_modulations{16'000'000};
    std::size_t maximum_applications{1'000'000};
    std::size_t maximum_parameter_values{16'000'000};
    std::size_t maximum_atlases{1'000'000};
    std::size_t maximum_atlas_regions{16'000'000};
    std::size_t maximum_embedded_blob_bytes{256ULL << 20};
};

class TextureDocumentIoError final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

struct TextureDocumentAssetInfo {
    std::string identifier;
    std::size_t texture_set_count{};
    std::size_t tiled_image_count{};
    std::size_t layer_entry_count{};
    std::size_t atlas_count{};
    std::size_t editable_entry_count{};
    std::size_t preset_application_count{};
    friend bool operator==(const TextureDocumentAssetInfo&,
                           const TextureDocumentAssetInfo&) = default;
};

[[nodiscard]] std::vector<TextureDocumentAssetInfo> list_texture_documents(
    const ProjectContainer& project, TextureDocumentReadLimits limits = {});
void upsert_texture_document(ProjectContainer& project, std::string identifier,
                             const doc::TextureDocument& document);
[[nodiscard]] doc::TextureDocument unpack_texture_document(
    const ProjectContainer& project, std::string_view identifier,
    TextureDocumentReadLimits limits = {},
    std::pmr::memory_resource* memory_resource = std::pmr::get_default_resource());

// Internal bridge used only by the canonical project-document decoder.
struct TextureDocumentArchiveAccess;

}  // namespace ctex::io

#endif
