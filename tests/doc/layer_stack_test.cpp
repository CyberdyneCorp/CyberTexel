#include <ctex/doc/document.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

using namespace ctex::doc;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_rule(Callable&& callable, LayerStackRule rule, std::string_view message) {
    try {
        callable();
    } catch (const LayerStackError& error) {
        return expect(error.rule() == rule, message);
    } catch (...) {
    }
    return expect(false, message);
}

ctex::graph::GraphDocument graph(double value) {
    return ctex::graph::GraphDocument({.role = ctex::graph::NodeRole::output,
                                       .type_id = "ctex.output.layer-stack-test",
                                       .type_version = 1,
                                       .display_name = "Output",
                                       .position = {},
                                       .inputs = {{.identifier = "value",
                                                   .display_name = "Value",
                                                   .type = ctex::graph::SocketType::scalar,
                                                   .value = value}},
                                       .outputs = {},
                                       .properties = {}});
}

LayerEntry entry(std::string identifier, LayerEntryKind kind) {
    return {.identifier = identifier,
            .display_name = identifier,
            .kind = kind,
            .parent_identifier = {},
            .target_identifier = {},
            .enabled = true,
            .opacity = 1.0,
            .graph = std::nullopt,
            .content_revision = 1};
}

bool explicit_kinds_and_order_are_retained() {
    LayerStack stack;
    std::vector<LayerEntry> entries;
    entries.push_back(entry("paint", LayerEntryKind::paint_layer));
    LayerEntry fill = entry("fill", LayerEntryKind::fill_layer);
    fill.graph = graph(0.25);
    entries.push_back(std::move(fill));
    entries.push_back(entry("group", LayerEntryKind::group));
    LayerEntry child = entry("child", LayerEntryKind::paint_layer);
    child.parent_identifier = "group";
    entries.push_back(std::move(child));
    LayerEntry mask = entry("mask", LayerEntryKind::mask);
    mask.target_identifier = "group";
    entries.push_back(std::move(mask));
    LayerEntry filter = entry("filter", LayerEntryKind::filter);
    filter.target_identifier = "paint";
    entries.push_back(std::move(filter));
    entries.push_back(entry("instance", LayerEntryKind::instance));
    entries.push_back(entry("decal", LayerEntryKind::editable_decal));
    entries.push_back(entry("text", LayerEntryKind::editable_text));
    entries.push_back(entry("path", LayerEntryKind::surface_path));
    stack.append(entries);

    const std::vector<LayerEntryKind> expected{
        LayerEntryKind::paint_layer, LayerEntryKind::fill_layer,     LayerEntryKind::group,
        LayerEntryKind::paint_layer, LayerEntryKind::mask,           LayerEntryKind::filter,
        LayerEntryKind::instance,    LayerEntryKind::editable_decal, LayerEntryKind::editable_text,
        LayerEntryKind::surface_path};
    std::vector<LayerEntryKind> actual;
    for (const LayerEntry& stored : stack.entries()) {
        actual.push_back(stored.kind);
    }
    return expect(actual == expected, "layer-stack kinds or evaluation order changed") &&
           expect(stack.entry("mask").target_identifier == "group",
                  "mask did not retain its single group attachment");
}

bool refusals_are_transactional() {
    LayerStack stack;
    stack.append(entry("group", LayerEntryKind::group));
    LayerEntry child = entry("child", LayerEntryKind::paint_layer);
    child.parent_identifier = "group";
    stack.append(std::move(child));
    const LayerStack before = stack;

    const bool self_parent =
        expect_rule([&] { stack.set_layout("group", "group", {}); },
                    LayerStackRule::evaluation_order, "group accepted itself as a parent");
    LayerEntry unattached = entry("mask", LayerEntryKind::mask);
    const bool mask =
        expect_rule([&] { stack.append(std::move(unattached)); }, LayerStackRule::single_attachment,
                    "mask accepted no attachment target");
    LayerEntry late = entry("late-filter", LayerEntryKind::filter);
    late.target_identifier = "missing";
    const bool target =
        expect_rule([&] { stack.append(std::move(late)); }, LayerStackRule::evaluation_order,
                    "filter accepted a missing or later target");
    return self_parent && mask && target &&
           expect(stack == before, "nesting refusal changed the layer stack");
}

bool nesting_depth_and_fill_derivation_are_explicit() {
    LayerStack stack;
    for (std::size_t depth = 0; depth < LayerStack::maximum_group_depth; ++depth) {
        LayerEntry group = entry("group-" + std::to_string(depth), LayerEntryKind::group);
        if (depth != 0) {
            group.parent_identifier = "group-" + std::to_string(depth - 1);
        }
        stack.append(std::move(group));
    }
    LayerEntry deepest_content = entry("deepest-content", LayerEntryKind::paint_layer);
    deepest_content.parent_identifier =
        "group-" + std::to_string(LayerStack::maximum_group_depth - 1);
    stack.append(std::move(deepest_content));
    const LayerStack before = stack;
    LayerEntry too_deep = entry("too-deep", LayerEntryKind::group);
    too_deep.parent_identifier = "group-" + std::to_string(LayerStack::maximum_group_depth - 1);
    const bool depth_refused =
        expect_rule([&] { stack.append(std::move(too_deep)); }, LayerStackRule::maximum_group_depth,
                    "layer stack accepted a group beyond its documented nesting depth");

    LayerStack fill_stack;
    LayerEntry fill = entry("fill", LayerEntryKind::fill_layer);
    fill.graph = graph(0.1);
    fill_stack.append(std::move(fill));
    fill_stack.set_fill_graph("fill", graph(0.9));
    return depth_refused && expect(stack == before, "depth refusal changed the stack") &&
           expect(fill_stack.entry("fill").content_revision == 2,
                  "fill graph edit did not invalidate derived content");
}

bool each_texture_set_owns_an_independent_stack() {
    TextureDocument document;
    const auto descriptor = [](std::string name, std::string key) {
        return TextureSetDescriptor{.display_name = std::move(name),
                                    .partition_kind = PartitionSourceKind::material,
                                    .partition_key = std::move(key),
                                    .uv_set = "uv0",
                                    .width = 16,
                                    .height = 16,
                                    .default_bit_depth = 8};
    };
    TextureSet& first = document.create_texture_set(descriptor("First", "first"));
    const std::string first_id = first.id();
    const std::string second_id = document.create_texture_set(descriptor("Second", "second")).id();
    document.texture_set(first_id).layer_stack().append(
        entry("paint", LayerEntryKind::paint_layer));
    return expect(document.texture_set(first_id).layer_stack().size() == 1,
                  "first texture set lost its layer") &&
           expect(document.texture_set(second_id).layer_stack().empty(),
                  "layer leaked into another texture set");
}

}  // namespace

int main() {
    return explicit_kinds_and_order_are_retained() && refusals_are_transactional() &&
                   nesting_depth_and_fill_derivation_are_explicit() &&
                   each_texture_set_owns_an_independent_stack()
               ? 0
               : 1;
}
