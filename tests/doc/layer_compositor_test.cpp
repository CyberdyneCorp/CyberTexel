#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctex/doc/document.hpp>
#include <ctex/graph/portable_nodes.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::doc;
using ctex::graph::ColourValue;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool close(float actual, float expected) { return std::abs(actual - expected) <= 1.0e-6F; }

bool close(ColourValue actual, ColourValue expected) {
    return close(actual.r, expected.r) && close(actual.g, expected.g) &&
           close(actual.b, expected.b) && close(actual.a, expected.a);
}

TextureSet texture_set() {
    return TextureSet({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = 2,
                       .height = 1,
                       .default_bit_depth = 8});
}

LayerEntry entry(std::string identifier, LayerEntryKind kind = LayerEntryKind::paint_layer) {
    return {.identifier = identifier,
            .display_name = identifier,
            .kind = kind,
            .parent_identifier = {},
            .target_identifier = {},
            .source_identifier = {},
            .enabled = true,
            .opacity = 1.0,
            .blend_mode = "normal",
            .channels = {},
            .graph = std::nullopt,
            .content_revision = 1};
}

void enable_channel(LayerEntry& value, std::string semantic_id, double opacity = 1.0) {
    value.channels.push_back(
        {.semantic_id = std::move(semantic_id), .enabled = true, .opacity = opacity});
}

LayerCompositeRaster raster(std::string entry_identifier, std::string semantic_id,
                            ColourValue first, ColourValue second = {}) {
    return {.entry_identifier = std::move(entry_identifier),
            .semantic_id = std::move(semantic_id),
            .width = 2,
            .height = 1,
            .pixels = {first, second},
            .coverage = {}};
}

LayerCompositeMaskRaster mask(std::string identifier, double first, double second) {
    return {.mask_identifier = std::move(identifier),
            .width = 2,
            .height = 1,
            .values = {first, second}};
}

bool bottom_to_top_order_uses_shared_blend_formulas() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    LayerEntry bottom = entry("bottom");
    enable_channel(bottom, "pbr.base_color");
    set.layer_stack().append(std::move(bottom));
    LayerEntry top = entry("top");
    top.opacity = 0.5;
    top.blend_mode = "multiply";
    enable_channel(top, "pbr.base_color");
    set.layer_stack().append(std::move(top));

    constexpr ColourValue red{0.8F, 0.2F, 0.1F, 0.0F};
    constexpr ColourValue blue{0.25F, 0.5F, 0.75F, 0.0F};
    LayerCompositeRaster top_content = raster("top", "pbr.base_color", blue, red);
    top_content.coverage = {0.25F, 0.75F};
    const LayerCompositeRequest request{
        .width = 2,
        .height = 1,
        .content = {raster("bottom", "pbr.base_color", red, blue), std::move(top_content)},
        .masks = {}};
    const LayerCompositeResult result = set.composite_cpu(request);
    const auto& pixels = result.channel("pbr.base_color").pixels;
    const ColourValue expected_first = ctex::graph::blend_colour("multiply", red, blue, 0.125);
    const ColourValue expected_second = ctex::graph::blend_colour("multiply", blue, red, 0.375);
    return expect(
        pixels.size() == 2 && close(pixels[0], expected_first) && close(pixels[1], expected_second),
        "CPU layer compositor did not evaluate bottom-to-top with shared formulas");
}

bool per_channel_participation_leaves_other_channels_unchanged() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    set.channels().enable("pbr.roughness");
    LayerEntry roughness = entry("roughness");
    enable_channel(roughness, "pbr.roughness", 0.5);
    set.layer_stack().append(std::move(roughness));
    const LayerCompositeRequest request{
        .width = 2,
        .height = 1,
        .content = {raster("roughness", "pbr.roughness", {0.9F, 0.0F, 0.0F, 0.0F},
                           {0.1F, 0.0F, 0.0F, 0.0F})},
        .masks = {}};
    const LayerCompositeResult result = set.composite_cpu(request);
    const auto& base = result.channel("pbr.base_color").pixels;
    const auto& rough = result.channel("pbr.roughness").pixels;
    return expect(base == std::vector<ColourValue>(2, {0.5F, 0.5F, 0.5F, 0.0F}),
                  "roughness-only layer changed base colour") &&
           expect(close(rough[0].r, 0.7F) && close(rough[1].r, 0.3F),
                  "roughness channel opacity was not composited independently");
}

TextureSet grouped_set(std::string blend_mode) {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    LayerEntry group = entry("group", LayerEntryKind::group);
    group.opacity = 0.5;
    group.blend_mode = std::move(blend_mode);
    set.layer_stack().append(std::move(group));
    LayerEntry low = entry("low");
    low.parent_identifier = "group";
    enable_channel(low, "pbr.base_color");
    set.layer_stack().append(std::move(low));
    LayerEntry high = entry("high");
    high.parent_identifier = "group";
    enable_channel(high, "pbr.base_color");
    set.layer_stack().append(std::move(high));
    return set;
}

LayerCompositeRequest grouped_request() {
    return {.width = 2,
            .height = 1,
            .content = {raster("low", "pbr.base_color", {0.2F, 0.2F, 0.2F, 0.0F},
                               {0.2F, 0.2F, 0.2F, 0.0F}),
                        raster("high", "pbr.base_color", {0.8F, 0.8F, 0.8F, 0.0F},
                               {0.8F, 0.8F, 0.8F, 0.0F})},
            .masks = {}};
}

bool isolated_and_pass_through_groups_are_distinct() {
    const LayerCompositeResult isolated = grouped_set("multiply").composite_cpu(grouped_request());
    const LayerCompositeResult passed =
        grouped_set("pass_through").composite_cpu(grouped_request());
    const float isolated_value = isolated.channel("pbr.base_color").pixels[0].r;
    const float passed_value = passed.channel("pbr.base_color").pixels[0].r;
    return expect(close(isolated_value, 0.45F), "isolated group result was not blended once") &&
           expect(close(passed_value, 0.575F),
                  "Pass Through children did not blend directly into the enclosing accumulator") &&
           expect(!close(isolated_value, passed_value),
                  "isolated and Pass Through group semantics collapsed together");
}

bool filter_on_pass_through_group_forces_an_isolated_input() {
    TextureSet set = grouped_set("pass_through");
    LayerEntry filter = entry("group-filter", LayerEntryKind::filter);
    filter.target_identifier = "group";
    enable_channel(filter, "pbr.base_color");
    set.layer_stack().append(std::move(filter));
    LayerCompositeRequest request = grouped_request();
    request.content.push_back(raster("group-filter", "pbr.base_color", {0.1F, 0.1F, 0.1F, 0.0F},
                                     {0.1F, 0.1F, 0.1F, 0.0F}));
    const LayerCompositeResult result = set.composite_cpu(request);
    return expect(close(result.channel("pbr.base_color").pixels[0].r, 0.3F),
                  "Pass Through group filter did not consume and publish an isolated result");
}

bool direct_and_group_masks_modulate_at_their_scope() {
    TextureSet set = grouped_set("normal");
    LayerEntry group_mask = entry("group-mask", LayerEntryKind::mask);
    group_mask.target_identifier = "group";
    set.layer_stack().append(std::move(group_mask));
    LayerEntry layer_mask = entry("layer-mask", LayerEntryKind::mask);
    layer_mask.target_identifier = "high";
    set.layer_stack().append(std::move(layer_mask));
    LayerCompositeRequest request = grouped_request();
    request.masks = {mask("group-mask", 0.5, 1.0), mask("layer-mask", 0.5, 0.0)};
    const LayerCompositeResult result = set.composite_cpu(request);
    const auto& pixels = result.channel("pbr.base_color").pixels;
    return expect(close(pixels[0].r, 0.5F) && close(pixels[1].r, 0.35F),
                  "direct and group mask rasters were not applied at their own scopes: " +
                      std::to_string(pixels[0].r) + ", " + std::to_string(pixels[1].r));
}

bool instances_and_filters_use_resolved_content() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    LayerEntry source = entry("source");
    source.enabled = false;
    enable_channel(source, "pbr.base_color");
    set.layer_stack().append(std::move(source));
    LayerEntry instance = entry("instance", LayerEntryKind::instance);
    instance.source_identifier = "source";
    instance.opacity = 0.5;
    enable_channel(instance, "pbr.base_color");
    set.layer_stack().append(std::move(instance));
    LayerEntry filter = entry("filter", LayerEntryKind::filter);
    filter.target_identifier = "instance";
    filter.opacity = 0.5;
    enable_channel(filter, "pbr.base_color");
    set.layer_stack().append(std::move(filter));
    const LayerCompositeRequest request{
        .width = 2,
        .height = 1,
        .content = {raster("source", "pbr.base_color", {0.9F, 0.9F, 0.9F, 0.0F},
                           {0.9F, 0.9F, 0.9F, 0.0F}),
                    raster("filter", "pbr.base_color", {0.3F, 0.3F, 0.3F, 0.0F},
                           {0.3F, 0.3F, 0.3F, 0.0F})},
        .masks = {}};
    const LayerCompositeResult result = set.composite_cpu(request);
    const auto& pixels = result.channel("pbr.base_color").pixels;
    return expect(close(pixels[0].r, 0.55F) && close(pixels[1].r, 0.55F),
                  "instance did not resolve source content or filter its isolated output");
}

bool channel_blending_policies_are_applied() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.height");
    set.channels().enable("pbr.normal");
    LayerEntry low = entry("low");
    enable_channel(low, "pbr.height");
    enable_channel(low, "pbr.normal", 0.5);
    set.layer_stack().append(std::move(low));
    LayerEntry high = entry("high");
    enable_channel(high, "pbr.height");
    set.layer_stack().append(std::move(high));
    const LayerCompositeRequest request{
        .width = 2,
        .height = 1,
        .content = {raster("low", "pbr.height", {0.6F, 0.0F, 0.0F, 0.0F}, {0.6F, 0.0F, 0.0F, 0.0F}),
                    raster("high", "pbr.height", {0.7F, 0.0F, 0.0F, 0.0F},
                           {0.7F, 0.0F, 0.0F, 0.0F}),
                    raster("low", "pbr.normal", {1.0F, 0.5F, 0.5F, 0.0F},
                           {1.0F, 0.5F, 0.5F, 0.0F})},
        .masks = {}};
    const LayerCompositeResult result = set.composite_cpu(request);
    const ColourValue normal = result.channel("pbr.normal").pixels[0];
    return expect(close(result.channel("pbr.height").pixels[0].r, 1.0F),
                  "additive channel policy did not accumulate and clamp height") &&
           expect(
               close(normal.r, 0.8535534F) && close(normal.g, 0.5F) && close(normal.b, 0.8535534F),
               "normal-vector channel policy did not normalize the blended vector");
}

bool repeat_composite_is_bit_identical() {
    const TextureSet set = grouped_set("screen");
    const LayerCompositeRequest request = grouped_request();
    const LayerCompositeResult first = set.composite_cpu(request);
    const LayerCompositeResult second = set.composite_cpu(request);
    const auto& first_pixels = first.channel("pbr.base_color").pixels;
    const auto& second_pixels = second.channel("pbr.base_color").pixels;
    return expect(first == second && std::memcmp(first_pixels.data(), second_pixels.data(),
                                                 first_pixels.size() * sizeof(ColourValue)) == 0,
                  "unchanged CPU composites were not bit-identical");
}

template <typename Value>
void write_binary(std::ofstream& output, const Value& value) {
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

bool write_determinism_output() {
    const char* directory = std::getenv("CTEX_DETERMINISM_OUTPUT_DIR");
    if (directory == nullptr) {
        return true;
    }
    const LayerCompositeResult result = grouped_set("screen").composite_cpu(grouped_request());
    const std::filesystem::path path = std::filesystem::path(directory) / "layer-composite.bin";
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    write_binary(output, result.width);
    write_binary(output, result.height);
    const std::uint64_t channel_count = result.channels.size();
    write_binary(output, channel_count);
    for (const LayerCompositeChannel& channel : result.channels) {
        const std::uint64_t name_size = channel.semantic_id.size();
        const std::uint64_t pixel_count = channel.pixels.size();
        write_binary(output, name_size);
        output.write(channel.semantic_id.data(), static_cast<std::streamsize>(name_size));
        write_binary(output, channel.component_count);
        write_binary(output, pixel_count);
        output.write(reinterpret_cast<const char*>(channel.pixels.data()),
                     static_cast<std::streamsize>(pixel_count * sizeof(ColourValue)));
    }
    return expect(output.good(), "could not write layer-composite determinism output");
}

template <typename Callable>
bool expect_error(Callable&& callable, LayerCompositeErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const LayerCompositeError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

bool invalid_inputs_are_refused() {
    TextureSet set = texture_set();
    set.channels().enable("pbr.base_color");
    LayerEntry layer = entry("layer");
    enable_channel(layer, "pbr.base_color");
    set.layer_stack().append(std::move(layer));
    const LayerCompositeRequest missing{.width = 2, .height = 1, .content = {}, .masks = {}};
    LayerCompositeRequest duplicate{
        .width = 2,
        .height = 1,
        .content = {raster("layer", "pbr.base_color", {}), raster("layer", "pbr.base_color", {})},
        .masks = {}};
    LayerCompositeRequest invalid{
        .width = 2, .height = 1, .content = {raster("layer", "pbr.base_color", {})}, .masks = {}};
    invalid.content.front().pixels[0].r = std::numeric_limits<float>::quiet_NaN();
    LayerEntry layer_mask = entry("layer-mask", LayerEntryKind::mask);
    layer_mask.target_identifier = "layer";
    set.layer_stack().append(std::move(layer_mask));
    const LayerCompositeRequest missing_mask{
        .width = 2, .height = 1, .content = {raster("layer", "pbr.base_color", {})}, .masks = {}};
    return expect_error([&] { static_cast<void>(set.composite_cpu(missing)); },
                        LayerCompositeErrorCode::missing_content,
                        "missing layer content was accepted") &&
           expect_error([&] { static_cast<void>(set.composite_cpu(duplicate)); },
                        LayerCompositeErrorCode::duplicate_input,
                        "duplicate layer content was accepted") &&
           expect_error([&] { static_cast<void>(set.composite_cpu(invalid)); },
                        LayerCompositeErrorCode::invalid_request,
                        "non-finite layer content was accepted") &&
           expect_error([&] { static_cast<void>(set.composite_cpu(missing_mask)); },
                        LayerCompositeErrorCode::missing_mask,
                        "missing composite mask raster was accepted");
}

bool unevaluable_enabled_channel_is_refused() {
    TextureSet set = texture_set();
    set.channels().register_descriptor({
        .semantic_id = "custom.unsupported",
        .component_count = 1,
        .scalar_representation = ScalarRepresentation::unsigned_normalized,
        .preferred_bit_depth = 8,
        .default_value = {0.0},
        .classification = ChannelClassification::data,
        .blending_policy = BlendingPolicy::scalar,
        .export_mapping = "unsupported",
        .evaluable = false,
    });
    set.channels().enable("custom.unsupported");
    const LayerCompositeRequest request{.width = 2, .height = 1, .content = {}, .masks = {}};
    return expect_error([&] { static_cast<void>(set.composite_cpu(request)); },
                        LayerCompositeErrorCode::unevaluable_channel,
                        "CPU compositor approximated an unevaluable channel");
}

}  // namespace

int main() {
    return bottom_to_top_order_uses_shared_blend_formulas() &&
                   per_channel_participation_leaves_other_channels_unchanged() &&
                   isolated_and_pass_through_groups_are_distinct() &&
                   filter_on_pass_through_group_forces_an_isolated_input() &&
                   direct_and_group_masks_modulate_at_their_scope() &&
                   instances_and_filters_use_resolved_content() &&
                   channel_blending_policies_are_applied() && repeat_composite_is_bit_identical() &&
                   invalid_inputs_are_refused() && unevaluable_enabled_channel_is_refused() &&
                   write_determinism_output()
               ? 0
               : 1;
}
