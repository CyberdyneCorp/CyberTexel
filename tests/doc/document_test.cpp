#include <array>
#include <cstddef>
#include <ctex/doc/document.hpp>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using ctex::doc::PartitionSourceKind;
using ctex::doc::TextureDocument;
using ctex::doc::TextureSetDescriptor;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

TextureSetDescriptor descriptor(std::string name, std::string partition, std::string uv_set,
                                std::uint32_t resolution, std::uint8_t bit_depth) {
    return {
        .display_name = std::move(name),
        .partition_kind = PartitionSourceKind::material,
        .partition_key = std::move(partition),
        .uv_set = std::move(uv_set),
        .width = resolution,
        .height = resolution,
        .default_bit_depth = bit_depth,
    };
}

bool sets_have_independent_storage() {
    TextureDocument document;
    auto& body = document.create_texture_set(descriptor("Body", "body-material", "uv0", 4096, 8));
    const std::string body_id = body.id();
    auto& eyes = document.create_texture_set(descriptor("Eyes", "eye-material", "uv1", 1024, 16));
    const std::string eyes_id = eyes.id();

    document.texture_set(body_id).channels().enable("pbr.base_color");
    const std::array red{std::byte{255}, std::byte{0}, std::byte{0}};
    document.texture_set(body_id).channels().pixels("pbr.base_color").write_pixel(0, 0, red);

    return expect(document.texture_set_count() == 2, "document did not retain both texture sets") &&
           expect(document.texture_set(body_id).descriptor().width == 4096,
                  "body resolution changed") &&
           expect(document.texture_set(eyes_id).descriptor().width == 1024,
                  "eye resolution changed") &&
           expect(document.texture_set(eyes_id).descriptor().default_bit_depth == 16,
                  "eye precision changed") &&
           expect(!document.texture_set(eyes_id).channels().is_enabled("pbr.base_color"),
                  "painting body allocated eye storage");
}

bool identity_survives_reordering_and_rename() {
    TextureDocument first;
    const std::string body_id =
        first.create_texture_set(descriptor("Body", "body-material", "uv0", 2048, 8)).id();
    const std::string eyes_id =
        first.create_texture_set(descriptor("Eyes", "eye-material", "uv0", 2048, 8)).id();

    TextureDocument reordered;
    const std::string renamed_eyes_id =
        reordered.create_texture_set(descriptor("Renamed eyes", "eye-material", "uv0", 2048, 8))
            .id();
    const std::string renamed_body_id =
        reordered.create_texture_set(descriptor("Renamed body", "body-material", "uv0", 2048, 8))
            .id();

    return expect(body_id == renamed_body_id, "body identity depended on order or display name") &&
           expect(eyes_id == renamed_eyes_id, "eye identity depended on order or display name") &&
           expect(first.texture_set_ids() == reordered.texture_set_ids(),
                  "stable identity enumeration changed after reorder");
}

bool uv_binding_participates_in_identity() {
    const auto uv0 = descriptor("Body UV0", "body-material", "uv0", 1024, 8);
    const auto uv1 = descriptor("Body UV1", "body-material", "uv1", 1024, 8);
    return expect(ctex::doc::texture_set_stable_id(uv0) != ctex::doc::texture_set_stable_id(uv1),
                  "UV binding was absent from texture-set identity");
}

}  // namespace

int main() {
    return sets_have_independent_storage() && identity_survives_reordering_and_rename() &&
                   uv_binding_participates_in_identity()
               ? 0
               : 1;
}
