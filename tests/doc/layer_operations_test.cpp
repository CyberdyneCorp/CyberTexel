#include <ctex/doc/document.hpp>
#include <iostream>
#include <string>
#include <string_view>

namespace {

using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, LayerOperationErrorCode code, std::string_view message) {
    try {
        callable();
    } catch (const LayerOperationError& error) {
        return expect(error.code() == code, message);
    } catch (...) {
    }
    return expect(false, message);
}

TextureSet texture_set() {
    TextureSet result({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = 2,
                       .height = 1,
                       .default_bit_depth = 8});
    result.channels().enable("pbr.base_color");
    return result;
}

LayerEntry entry(std::string identifier, LayerEntryKind kind = LayerEntryKind::paint_layer) {
    LayerEntry result{.identifier = identifier,
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
    if (kind != LayerEntryKind::group && kind != LayerEntryKind::mask) {
        result.channels.push_back(
            {.semantic_id = "pbr.base_color", .enabled = true, .opacity = 1.0});
    }
    return result;
}

LayerCompositeRaster raster(std::string identifier, float value, float coverage = 1.0F) {
    return {.entry_identifier = std::move(identifier),
            .semantic_id = "pbr.base_color",
            .width = 2,
            .height = 1,
            .pixels = {{value, value, value, 0.0F}, {value, value, value, 0.0F}},
            .coverage = {coverage, coverage}};
}

LayerCompositeRequest snapshot(std::initializer_list<LayerCompositeRaster> content,
                               std::initializer_list<LayerCompositeMaskRaster> masks = {}) {
    return {.width = 2, .height = 1, .content = content, .masks = masks};
}

LayerOperationResult apply(TextureSet& set, LayerOperation operation,
                           LayerCompositeRequest content) {
    return set.apply_layer_operation({.operation = std::move(operation),
                                      .resolved_content = std::move(content),
                                      .maximum_output_bytes = 1U << 20,
                                      .appearance_tolerance = 1.0e-6F});
}

ctex::graph::GraphDocument graph() {
    return ctex::graph::GraphDocument({.role = ctex::graph::NodeRole::output,
                                       .type_id = "ctex.output.layer-operation-test",
                                       .type_version = 1,
                                       .display_name = "Output",
                                       .position = {},
                                       .inputs = {},
                                       .outputs = {},
                                       .properties = {}});
}

bool create_duplicate_and_delete_transform_complete_state() {
    TextureSet set = texture_set();
    LayerOperationResult state =
        apply(set,
              CreateLayerOperation{
                  .entry = entry("original"), .content = {raster("original", 0.2F)}, .masks = {}},
              snapshot({}));
    state = apply(
        set, DuplicateLayerOperation{.identifier = "original", .duplicate_identifier = "duplicate"},
        std::move(state.resolved_content));
    const bool duplicated =
        expect(set.layer_stack().size() == 2 && set.layer_stack().contains("duplicate") &&
                   state.resolved_content.content.size() == 2,
               "duplicate did not clone the entry and its resolved content");
    state = apply(set, DeleteLayerOperation{.identifier = "duplicate"},
                  std::move(state.resolved_content));
    return duplicated &&
           expect(set.layer_stack().size() == 1 && !set.layer_stack().contains("duplicate") &&
                      state.resolved_content.content.size() == 1,
                  "delete did not remove the ownership subtree and its content");
}

bool duplicate_clones_group_ownership_subtree() {
    TextureSet set = texture_set();
    LayerEntry group = entry("group", LayerEntryKind::group);
    LayerEntry child = entry("child");
    child.parent_identifier = "group";
    LayerEntry mask = entry("mask", LayerEntryKind::mask);
    mask.target_identifier = "child";
    set.layer_stack().append(std::vector<LayerEntry>{group, child, mask});
    LayerOperationResult state = apply(
        set, DuplicateLayerOperation{.identifier = "group", .duplicate_identifier = "copy"},
        snapshot({raster("child", 0.4F)},
                 {{.mask_identifier = "mask", .width = 2, .height = 1, .values = {1.0, 1.0}}}));
    return expect(set.layer_stack().contains("copy") && set.layer_stack().contains("copy/child") &&
                      set.layer_stack().contains("copy/mask") &&
                      set.layer_stack().entry("copy/child").parent_identifier == "copy" &&
                      set.layer_stack().entry("copy/mask").target_identifier == "copy/child" &&
                      state.resolved_content.content.size() == 2 &&
                      state.resolved_content.masks.size() == 2,
                  "group duplicate did not clone and retarget its owned subtree");
}

bool reorder_and_reparent_move_the_whole_subtree() {
    TextureSet set = texture_set();
    LayerEntry group = entry("group", LayerEntryKind::group);
    LayerEntry first = entry("first");
    first.parent_identifier = "group";
    LayerEntry second = entry("second");
    set.layer_stack().append(std::vector<LayerEntry>{group, first, second});
    LayerOperationResult state =
        apply(set,
              ReparentLayerOperation{
                  .identifier = "second", .parent_identifier = "group", .before_identifier = {}},
              snapshot({raster("first", 0.2F), raster("second", 0.8F)}));
    state = apply(set, ReorderLayerOperation{.identifier = "second", .before_identifier = "first"},
                  std::move(state.resolved_content));
    const auto entries = set.layer_stack().entries();
    return expect(entries.size() == 3 && entries[0].identifier == "group" &&
                      entries[1].identifier == "second" && entries[2].identifier == "first" &&
                      entries[1].parent_identifier == "group",
                  "reorder or reparent did not publish the requested sibling order");
}

bool clear_and_invert_transform_pixels_and_revision() {
    TextureSet set = texture_set();
    set.layer_stack().append(entry("layer"));
    LayerOperationResult state =
        apply(set, InvertLayerOperation{.identifier = "layer", .semantic_id = {}},
              snapshot({raster("layer", 0.2F)}));
    const bool inverted =
        expect(state.resolved_content.content.front().pixels.front().r == 0.8F &&
                   set.layer_stack().entry("layer").content_revision == 2,
               "invert did not transform the resolved channel or advance its revision");
    state = apply(set, ClearLayerOperation{.identifier = "layer", .semantic_id = {}},
                  std::move(state.resolved_content));
    return inverted && expect(state.resolved_content.content.front().coverage ==
                                      std::vector<float>({0.0F, 0.0F}) &&
                                  set.layer_stack().entry("layer").content_revision == 3,
                              "clear did not make content transparent or advance its revision");
}

bool procedural_edit_rasterizes_and_instance_deletion_preserves_content() {
    TextureSet procedural = texture_set();
    LayerEntry fill = entry("fill", LayerEntryKind::fill_layer);
    fill.graph = graph();
    procedural.layer_stack().append(std::move(fill));
    LayerOperationResult inverted =
        apply(procedural, InvertLayerOperation{.identifier = "fill", .semantic_id = {}},
              snapshot({raster("fill", 0.25F)}));
    const bool rasterized =
        expect(procedural.layer_stack().entry("fill").kind == LayerEntryKind::paint_layer &&
                   !procedural.layer_stack().entry("fill").graph.has_value() &&
                   inverted.resolved_content.content.front().pixels.front().r == 0.75F,
               "destructive procedural edit did not become authored paint content");

    TextureSet instances = texture_set();
    instances.layer_stack().append(entry("source"));
    LayerEntry copy = entry("copy", LayerEntryKind::instance);
    copy.source_identifier = "source";
    instances.layer_stack().append(std::move(copy));
    LayerOperationResult independent =
        apply(instances,
              DeleteLayerOperation{
                  .identifier = "source",
                  .instance_policy = ReferencedSourceDeletionPolicy::make_instances_independent},
              snapshot({raster("source", 0.3F)}));
    return rasterized &&
           expect(!instances.layer_stack().contains("source") &&
                      instances.layer_stack().entry("copy").kind == LayerEntryKind::paint_layer &&
                      independent.resolved_content.content.size() == 1 &&
                      independent.resolved_content.content.front().entry_identifier == "copy",
                  "make-independent deletion did not retain resolved instance content");
}

bool merge_down_preserves_appearance_and_is_atomic() {
    TextureSet set = texture_set();
    set.layer_stack().append(entry("bottom"));
    set.layer_stack().append(entry("top"));
    const LayerCompositeRequest original = snapshot({raster("bottom", 0.2F), raster("top", 0.8F)});
    const LayerStack before = set.layer_stack();
    const bool refused = expect_error(
        [&] {
            static_cast<void>(
                apply(set,
                      MergeDownLayerOperation{.identifier = "top",
                                              .replacement_content = {raster("bottom", 0.1F)}},
                      original));
        },
        LayerOperationErrorCode::appearance_mismatch,
        "merge accepted replacement pixels that changed the composite");
    const bool unchanged =
        expect(set.layer_stack() == before, "failed merge changed the authoritative layer stack");
    LayerOperationResult state =
        apply(set,
              MergeDownLayerOperation{.identifier = "top",
                                      .replacement_content = {raster("bottom", 0.8F)}},
              original);
    return refused && unchanged &&
           expect(set.layer_stack().size() == 1 && set.layer_stack().contains("bottom") &&
                      !set.layer_stack().contains("top") &&
                      state.resolved_content.content.size() == 1,
                  "valid merge down did not replace the sibling pair atomically");
}

bool merge_group_and_flatten_preserve_appearance() {
    TextureSet grouped = texture_set();
    grouped.layer_stack().append(entry("group", LayerEntryKind::group));
    LayerEntry child = entry("child");
    child.parent_identifier = "group";
    grouped.layer_stack().append(std::move(child));
    LayerOperationResult merged =
        apply(grouped,
              MergeGroupLayerOperation{.identifier = "group",
                                       .replacement_content = {raster("group", 0.7F)}},
              snapshot({raster("child", 0.7F)}));
    const bool group_ok =
        expect(grouped.layer_stack().size() == 1 &&
                   grouped.layer_stack().entry("group").kind == LayerEntryKind::paint_layer,
               "merge group did not replace its ownership subtree");

    TextureSet flattened = texture_set();
    flattened.layer_stack().append(entry("low"));
    flattened.layer_stack().append(entry("high"));
    LayerOperationResult flat =
        apply(flattened,
              FlattenLayersOperation{.output_entry = entry("flat"),
                                     .replacement_content = {raster("flat", 0.9F)}},
              snapshot({raster("low", 0.1F), raster("high", 0.9F)}));
    return group_ok &&
           expect(flattened.layer_stack().size() == 1 && flattened.layer_stack().contains("flat") &&
                      flat.resolved_content.content.front().entry_identifier == "flat" &&
                      merged.resolved_content.content.front().entry_identifier == "group",
                  "flatten did not publish one appearance-equivalent paint layer");
}

bool convert_and_apply_mask_preserve_appearance() {
    TextureSet converted = texture_set();
    converted.layer_stack().append(entry("layer"));
    LayerOperationResult state =
        apply(converted,
              ConvertLayerOperation{.identifier = "layer",
                                    .target_kind = LayerEntryKind::fill_layer,
                                    .fill_graph = graph()},
              snapshot({raster("layer", 0.6F)}));
    const bool fill =
        expect(converted.layer_stack().entry("layer").kind == LayerEntryKind::fill_layer &&
                   converted.layer_stack().entry("layer").graph.has_value(),
               "paint-to-fill conversion did not retain its supplied graph");
    state = apply(converted,
                  ConvertLayerOperation{.identifier = "layer",
                                        .target_kind = LayerEntryKind::paint_layer,
                                        .fill_graph = std::nullopt},
                  std::move(state.resolved_content));

    TextureSet masked = texture_set();
    masked.layer_stack().append(entry("target"));
    LayerEntry mask = entry("mask", LayerEntryKind::mask);
    mask.target_identifier = "target";
    masked.layer_stack().append(mask);
    LayerOperationResult applied = apply(
        masked,
        ApplyMaskLayerOperation{.mask_identifier = "mask",
                                .replacement_content = {raster("target", 0.9F, 0.5F)}},
        snapshot({raster("target", 0.9F)},
                 {{.mask_identifier = "mask", .width = 2, .height = 1, .values = {0.5, 0.5}}}));
    return fill &&
           expect(converted.layer_stack().entry("layer").kind == LayerEntryKind::paint_layer &&
                      !converted.layer_stack().entry("layer").graph.has_value(),
                  "fill-to-paint conversion retained procedural state") &&
           expect(!masked.layer_stack().contains("mask") &&
                      applied.resolved_content.masks.empty() &&
                      applied.resolved_content.content.front().coverage.front() == 0.5F,
                  "apply mask did not bake coverage and remove the attachment");
}

bool applying_a_group_mask_rasterizes_the_owned_subtree() {
    TextureSet set = texture_set();
    set.layer_stack().append(entry("group", LayerEntryKind::group));
    LayerEntry child = entry("child");
    child.parent_identifier = "group";
    set.layer_stack().append(std::move(child));
    LayerEntry mask = entry("mask", LayerEntryKind::mask);
    mask.target_identifier = "group";
    set.layer_stack().append(std::move(mask));
    LayerOperationResult applied = apply(
        set,
        ApplyMaskLayerOperation{.mask_identifier = "mask",
                                .replacement_content = {raster("group", 0.9F, 0.5F)}},
        snapshot({raster("child", 0.9F)},
                 {{.mask_identifier = "mask", .width = 2, .height = 1, .values = {0.5, 0.5}}}));
    return expect(set.layer_stack().size() == 1 &&
                      set.layer_stack().entry("group").kind == LayerEntryKind::paint_layer &&
                      applied.resolved_content.content.size() == 1 &&
                      applied.resolved_content.masks.empty(),
                  "applying a group mask did not rasterize and replace its ownership subtree");
}

bool allocation_and_layout_refusals_are_atomic() {
    TextureSet set = texture_set();
    set.layer_stack().append(entry("first"));
    set.layer_stack().append(entry("second"));
    const LayerStack before = set.layer_stack();
    const LayerCompositeRequest content = snapshot({raster("first", 0.1F), raster("second", 0.2F)});
    const bool budget = expect_error(
        [&] {
            static_cast<void>(set.apply_layer_operation(
                {.operation = InvertLayerOperation{.identifier = "first", .semantic_id = {}},
                 .resolved_content = content,
                 .maximum_output_bytes = 1,
                 .appearance_tolerance = 1.0e-6F}));
        },
        LayerOperationErrorCode::allocation_limit,
        "operation accepted content beyond its declared byte limit");
    const bool parent = expect_error(
        [&] {
            static_cast<void>(apply(set,
                                    ReparentLayerOperation{.identifier = "second",
                                                           .parent_identifier = "missing",
                                                           .before_identifier = {}},
                                    content));
        },
        LayerOperationErrorCode::invalid_operation,
        "reparent accepted a missing destination group");
    return budget && parent &&
           expect(set.layer_stack() == before, "failed operation changed the layer stack");
}

}  // namespace

int main() {
    return create_duplicate_and_delete_transform_complete_state() &&
                   duplicate_clones_group_ownership_subtree() &&
                   reorder_and_reparent_move_the_whole_subtree() &&
                   clear_and_invert_transform_pixels_and_revision() &&
                   procedural_edit_rasterizes_and_instance_deletion_preserves_content() &&
                   merge_down_preserves_appearance_and_is_atomic() &&
                   merge_group_and_flatten_preserve_appearance() &&
                   convert_and_apply_mask_preserve_appearance() &&
                   applying_a_group_mask_rasterizes_the_owned_subtree() &&
                   allocation_and_layout_refusals_are_atomic()
               ? 0
               : 1;
}
