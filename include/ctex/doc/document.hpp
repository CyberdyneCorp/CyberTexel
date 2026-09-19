#ifndef CTEX_DOC_DOCUMENT_HPP
#define CTEX_DOC_DOCUMENT_HPP

#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::mesh {
class MeshView;
}

namespace ctex::doc {

enum class PartitionSourceKind { material, object, submesh, explicit_faces };

struct TextureSetDescriptor {
    std::string display_name;
    PartitionSourceKind partition_kind;
    std::string partition_key;
    std::string uv_set;
    std::uint32_t width;
    std::uint32_t height;
    std::uint8_t default_bit_depth;
};

class TextureSet {
public:
    explicit TextureSet(TextureSetDescriptor descriptor);

    [[nodiscard]] const std::string& id() const noexcept { return id_; }
    [[nodiscard]] const TextureSetDescriptor& descriptor() const noexcept { return descriptor_; }
    [[nodiscard]] TextureChannels& channels() noexcept { return channels_; }
    [[nodiscard]] const TextureChannels& channels() const noexcept { return channels_; }

private:
    TextureSetDescriptor descriptor_;
    std::string id_;
    TextureChannels channels_;
};

class TextureDocument {
public:
    TextureSet& create_texture_set(TextureSetDescriptor descriptor);
    std::vector<std::string> create_texture_sets_from_mesh(const mesh::MeshView& mesh,
                                                           std::string_view uv_set,
                                                           std::uint32_t width,
                                                           std::uint32_t height,
                                                           std::uint8_t default_bit_depth);
    [[nodiscard]] bool contains_texture_set(std::string_view stable_id) const noexcept;
    [[nodiscard]] TextureSet& texture_set(std::string_view stable_id);
    [[nodiscard]] const TextureSet& texture_set(std::string_view stable_id) const;
    [[nodiscard]] std::vector<std::string> texture_set_ids() const;
    [[nodiscard]] std::size_t texture_set_count() const noexcept { return texture_sets_.size(); }

private:
    std::map<std::string, TextureSet, std::less<>> texture_sets_;
};

[[nodiscard]] std::string texture_set_stable_id(const TextureSetDescriptor& descriptor);

}  // namespace ctex::doc

#endif
