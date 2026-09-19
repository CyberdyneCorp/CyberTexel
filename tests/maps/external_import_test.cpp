#include <array>
#include <cmath>
#include <cstddef>
#include <ctex/maps/bake_provider.hpp>
#include <ctex/maps/external_import.hpp>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::maps;

constexpr mesh::MeshRevision fixture_mesh_revision = 31;

mesh::TangentFrameDescriptor tangent_frame() { return {.uv_set = "paint"}; }

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-6) {
    return std::abs(actual - expected) <= tolerance;
}

doc::TextureSet& texture_set(doc::TextureDocument& document) {
    return document.create_texture_set({.display_name = "Body",
                                        .partition_kind = doc::PartitionSourceKind::material,
                                        .partition_key = "body",
                                        .uv_set = "paint",
                                        .width = 4,
                                        .height = 4,
                                        .default_bit_depth = 8});
}

MeshMapPixelBufferView view(const std::vector<std::byte>& pixels, std::uint32_t width,
                            std::uint32_t height, std::uint8_t channels) {
    return {.width = width,
            .height = height,
            .format = {.channel_type = image::ChannelType::uint8_unorm, .channel_count = channels},
            .row_stride_bytes = static_cast<std::size_t>(width) * channels,
            .pixels = pixels.data(),
            .pixel_bytes = pixels.size()};
}

ExternalMeshMapImport ao_import(const doc::TextureSet& set, const std::vector<std::byte>& pixels) {
    return {.kind = MeshMapKind::ambient_occlusion,
            .channel_meaning = MeshMapChannelMeaning::scalar_data,
            .color_space = image::ColorSpace::srgb_rec709,
            .texture_set_id = set.id(),
            .uv_set = "paint",
            .mesh_revision = fixture_mesh_revision,
            .buffer = view(pixels, 2, 2, 1)};
}

struct ProviderState {
    std::vector<std::byte> pixels;
};

bool can_produce_ao(void*, MeshMapKind kind) noexcept {
    return kind == MeshMapKind::ambient_occlusion;
}

BakeProviderStatus produce_ao(void* user_data, const BakeRequest* request, const BakeControl*,
                              BakeProviderOutput* output) noexcept {
    auto& state = *static_cast<ProviderState*>(user_data);
    output->image = view(state.pixels, request->width, request->height, 1);
    return BakeProviderStatus::completed;
}

bool external_ao_matches_provider_output_and_owns_its_copy() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet imported(set, fixture_mesh_revision);
    MeshMapSet provided(set, fixture_mesh_revision);
    std::vector<std::byte> source{std::byte{0}, std::byte{255}, std::byte{0}, std::byte{255}};
    ProviderState provider_state{.pixels = source};

    const ExternalMeshMapImportResult import_result =
        import_external_mesh_map(imported, ao_import(set, source));
    const BakeProvider provider{.name = "comparison-provider",
                                .user_data = &provider_state,
                                .can_produce = can_produce_ao,
                                .request = produce_ao};
    const BakeRequestResult provider_result =
        request_bake(provider, provided, MeshMapKind::ambient_occlusion, 2, 2);
    source.assign(source.size(), std::byte{0});

    const MeshMapReadResult imported_sample =
        imported.sample(MeshMapKind::ambient_occlusion, 0.25, 0.75);
    const MeshMapReadResult provided_sample =
        provided.sample(MeshMapKind::ambient_occlusion, 0.25, 0.75);
    return expect(import_result.binding.resolution_mismatch &&
                      import_result.channel_meaning == MeshMapChannelMeaning::scalar_data &&
                      import_result.declared_color_space == image::ColorSpace::srgb_rec709 &&
                      import_result.storage_color_space == image::working_color_space() &&
                      !import_result.converted_to_working_space,
                  "external AO import lost its declarations or bind report") &&
           expect(provider_result.status == BakeRequestStatus::completed &&
                      provider_result.binding &&
                      provider_result.binding->resolution_mismatch ==
                          import_result.binding.resolution_mismatch &&
                      provider_result.binding->staleness == import_result.binding.staleness &&
                      imported_sample == provided_sample &&
                      near(imported_sample.sample.values[0], 0.25),
                  "external AO did not behave like copied provider output");
}

bool colour_import_converts_rgb_and_preserves_alpha() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    const std::vector<std::byte> source{std::byte{128}, std::byte{64}, std::byte{255},
                                        std::byte{32}};
    const ExternalMeshMapImport request{.kind = MeshMapKind::vertex_colour,
                                        .channel_meaning = MeshMapChannelMeaning::colour_rgba,
                                        .color_space = image::ColorSpace::srgb_rec709,
                                        .texture_set_id = set.id(),
                                        .uv_set = "paint",
                                        .mesh_revision = fixture_mesh_revision,
                                        .buffer = view(source, 1, 1, 4)};
    const ExternalMeshMapImportResult result = import_external_mesh_map(maps, request);
    const MeshMapSample sample = maps.sample(MeshMapKind::vertex_colour, 0.5, 0.5).sample;
    return expect(result.converted_to_working_space &&
                      result.storage_color_space == image::ColorSpace::linear_rec709,
                  "sRGB vertex colour was not reported as converted to working space") &&
           expect(sample.component_count == 4 &&
                      near(sample.values[0], image::srgb_to_linear(128.0 / 255.0), 1.0 / 255.0) &&
                      near(sample.values[1], image::srgb_to_linear(64.0 / 255.0), 1.0 / 255.0) &&
                      near(sample.values[2], 1.0) && near(sample.values[3], 32.0 / 255.0),
                  "vertex-colour import converted the wrong components");
}

bool directx_normal_import_records_and_converts_its_green_channel() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision, tangent_frame());
    const std::vector<std::byte> source{std::byte{128}, std::byte{64}, std::byte{255}};
    const ExternalMeshMapImport request{.kind = MeshMapKind::tangent_space_normal,
                                        .channel_meaning = MeshMapChannelMeaning::normal_xyz,
                                        .color_space = image::ColorSpace::linear_rec709,
                                        .texture_set_id = set.id(),
                                        .uv_set = "paint",
                                        .mesh_revision = fixture_mesh_revision,
                                        .normal_convention = NormalMapConvention::direct_x,
                                        .tangent_frame = tangent_frame(),
                                        .buffer = view(source, 1, 1, 3)};
    const ExternalMeshMapImportResult result = import_external_mesh_map(maps, request);
    const MeshMapSample sample = maps.sample(MeshMapKind::tangent_space_normal, 0.5, 0.5).sample;
    return expect(
        result.normal_convention == NormalMapConvention::direct_x &&
            result.tangent_frame == tangent_frame() &&
            maps.map(MeshMapKind::tangent_space_normal).normal_convention ==
                NormalMapConvention::direct_x &&
            maps.map(MeshMapKind::tangent_space_normal).tangent_frame == tangent_frame() &&
            near(sample.values[0], 128.0 / 255.0) && near(sample.values[1], 191.0 / 255.0) &&
            near(sample.values[2], 1.0),
        "external DirectX normal was not recorded and read as canonical OpenGL");
}

template <typename Operation>
bool refused(Operation operation) {
    try {
        operation();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

bool invalid_or_missing_declarations_and_buffers_are_transactional() {
    doc::TextureDocument document;
    const doc::TextureSet& set = texture_set(document);
    MeshMapSet maps(set, fixture_mesh_revision);
    const std::vector<std::byte> source{std::byte{0}, std::byte{64}, std::byte{128},
                                        std::byte{255}};

    ExternalMeshMapImport missing_meaning = ao_import(set, source);
    missing_meaning.channel_meaning.reset();
    ExternalMeshMapImport missing_colour_space = ao_import(set, source);
    missing_colour_space.color_space.reset();
    ExternalMeshMapImport wrong_meaning = ao_import(set, source);
    wrong_meaning.channel_meaning = MeshMapChannelMeaning::normal_xyz;
    ExternalMeshMapImport invalid_meaning = ao_import(set, source);
    invalid_meaning.channel_meaning = static_cast<MeshMapChannelMeaning>(255);
    ExternalMeshMapImport invalid_colour_space = ao_import(set, source);
    invalid_colour_space.color_space = static_cast<image::ColorSpace>(255);
    ExternalMeshMapImport short_buffer = ao_import(set, source);
    short_buffer.buffer.pixel_bytes = 1;
    ExternalMeshMapImport wrong_set = ao_import(set, source);
    wrong_set.texture_set_id = "material/other";
    ExternalMeshMapImport convention_on_data = ao_import(set, source);
    convention_on_data.normal_convention = NormalMapConvention::open_gl;
    ExternalMeshMapImport short_colour_channels{
        .kind = MeshMapKind::vertex_colour,
        .channel_meaning = MeshMapChannelMeaning::colour_rgb,
        .color_space = image::ColorSpace::srgb_rec709,
        .texture_set_id = set.id(),
        .uv_set = "paint",
        .mesh_revision = fixture_mesh_revision,
        .buffer = view(source, 2, 2, 1)};
    const std::array<float, 3> non_finite_colour{std::numeric_limits<float>::quiet_NaN(), 0.5F,
                                                 1.0F};
    ExternalMeshMapImport non_finite{
        .kind = MeshMapKind::vertex_colour,
        .channel_meaning = MeshMapChannelMeaning::colour_rgb,
        .color_space = image::ColorSpace::srgb_rec709,
        .texture_set_id = set.id(),
        .uv_set = "paint",
        .mesh_revision = fixture_mesh_revision,
        .buffer = {.width = 1,
                   .height = 1,
                   .format = {.channel_type = image::ChannelType::float32, .channel_count = 3},
                   .row_stride_bytes = sizeof(non_finite_colour),
                   .pixels = non_finite_colour.data(),
                   .pixel_bytes = sizeof(non_finite_colour)}};
    const std::vector<std::byte> normal_pixels{std::byte{128}, std::byte{128}, std::byte{255}};
    ExternalMeshMapImport missing_normal_convention{
        .kind = MeshMapKind::tangent_space_normal,
        .channel_meaning = MeshMapChannelMeaning::normal_xyz,
        .color_space = image::ColorSpace::linear_rec709,
        .texture_set_id = set.id(),
        .uv_set = "paint",
        .mesh_revision = fixture_mesh_revision,
        .normal_convention = std::nullopt,
        .buffer = view(normal_pixels, 1, 1, 3)};

    const bool all_refused =
        refused([&] { static_cast<void>(import_external_mesh_map(maps, missing_meaning)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, missing_colour_space)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, wrong_meaning)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, invalid_meaning)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, invalid_colour_space)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, short_buffer)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, wrong_set)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, convention_on_data)); }) &&
        refused(
            [&] { static_cast<void>(import_external_mesh_map(maps, short_colour_channels)); }) &&
        refused([&] { static_cast<void>(import_external_mesh_map(maps, non_finite)); }) &&
        refused(
            [&] { static_cast<void>(import_external_mesh_map(maps, missing_normal_convention)); });
    return expect(all_refused && maps.size() == 0,
                  "invalid external map declaration or buffer changed the target map set");
}

}  // namespace

int main() {
    return external_ao_matches_provider_output_and_owns_its_copy() &&
                   colour_import_converts_rgb_and_preserves_alpha() &&
                   directx_normal_import_records_and_converts_its_green_channel() &&
                   invalid_or_missing_declarations_and_buffers_are_transactional()
               ? 0
               : 1;
}
