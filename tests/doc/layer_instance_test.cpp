#include <ctex/doc/layer_stack.hpp>
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
                                       .type_id = "ctex.output.layer-instance-test",
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

LayerEntry paint(std::string identifier) {
    return {.identifier = identifier,
            .display_name = identifier,
            .kind = LayerEntryKind::paint_layer,
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

LayerEntry fill(std::string identifier, double value) {
    LayerEntry result = paint(std::move(identifier));
    result.kind = LayerEntryKind::fill_layer;
    result.graph = graph(value);
    return result;
}

LayerEntry instance(std::string identifier, std::string source) {
    LayerEntry result = paint(std::move(identifier));
    result.kind = LayerEntryKind::instance;
    result.source_identifier = std::move(source);
    return result;
}

bool source_edits_are_shared_without_copying() {
    LayerStack stack;
    stack.append(paint("source"));
    stack.append(instance("first", "source"));
    stack.append(instance("second", "first"));

    const bool initially_shared = expect(
        stack.entry("first").graph == std::nullopt && stack.entry("second").graph == std::nullopt &&
            stack.resolved_content("first").identifier == "source" &&
            stack.resolved_content("second").identifier == "source",
        "instances copied content or did not resolve their shared source");
    stack.record_paint("source");
    return initially_shared &&
           expect(stack.resolved_content("first").content_revision == 2 &&
                      stack.resolved_content("second").content_revision == 2 &&
                      stack.entry("first").graph == std::nullopt,
                  "source graph edit did not update every instance dynamically");
}

bool modulation_and_masks_are_instance_owned() {
    LayerStack stack;
    LayerEntry source = paint("source");
    source.opacity = 0.8;
    source.blend_mode = "overlay";
    source.channels.push_back({.semantic_id = "pbr.base_color", .enabled = true, .opacity = 0.7});
    stack.append(std::move(source));
    LayerEntry copy = instance("instance", "source");
    copy.opacity = 0.4;
    copy.blend_mode = "screen";
    copy.channels.push_back({.semantic_id = "pbr.roughness", .enabled = true, .opacity = 0.6});
    stack.append(std::move(copy));
    LayerEntry mask = paint("instance-mask");
    mask.kind = LayerEntryKind::mask;
    mask.target_identifier = "instance";
    stack.append(std::move(mask));

    stack.set_opacity("instance", 0.25);
    stack.set_blend_mode("instance", "multiply");
    stack.set_channel_modulation(
        "instance", {.semantic_id = "pbr.roughness", .enabled = false, .opacity = 0.5});
    const LayerEntry& source_after = stack.entry("source");
    const LayerEntry& instance_after = stack.entry("instance");
    return expect(source_after.opacity == 0.8 && source_after.blend_mode == "overlay" &&
                      source_after.channels.front().opacity == 0.7,
                  "instance modulation changed its source") &&
           expect(instance_after.opacity == 0.25 && instance_after.blend_mode == "multiply" &&
                      !instance_after.channels.front().enabled &&
                      instance_after.channels.front().opacity == 0.5 &&
                      stack.entry("instance-mask").target_identifier == "instance",
                  "instance did not retain independent opacity, blend, channel, or mask state");
}

bool direct_paint_and_invalid_references_are_refused() {
    LayerStack stack;
    stack.append(paint("source"));
    stack.append(instance("instance", "source"));
    const LayerStack before = stack;
    bool named_source = false;
    try {
        static_cast<void>(stack.paint_target("instance"));
    } catch (const LayerStackError& error) {
        named_source = error.rule() == LayerStackRule::direct_instance_paint &&
                       std::string_view(error.what()).find("source") != std::string_view::npos &&
                       std::vector<std::string>(error.related_identifiers().begin(),
                                                error.related_identifiers().end()) ==
                           std::vector<std::string>{"source"};
    }
    const bool forward = expect_rule(
        [&] {
            LayerStack invalid;
            invalid.append(instance("early", "late"));
        },
        LayerStackRule::instance_source, "instance accepted a missing or later source");
    const bool self_cycle = expect_rule(
        [&] {
            LayerStack invalid;
            invalid.append(instance("cycle", "cycle"));
        },
        LayerStackRule::instance_cycle, "instance accepted a self-reference cycle");
    const std::vector<LayerEntry> cycle_entries{instance("a", "b"), instance("b", "a")};
    const bool pair_cycle = expect_rule(
        [&] {
            LayerStack invalid;
            invalid.append(cycle_entries);
        },
        LayerStackRule::instance_cycle, "instances accepted a two-entry reference cycle");
    const bool containment_cycle = expect_rule(
        [&] {
            LayerStack invalid;
            LayerEntry group = paint("group");
            group.kind = LayerEntryKind::group;
            invalid.append(std::move(group));
            LayerEntry child = instance("child", "group");
            child.parent_identifier = "group";
            invalid.append(std::move(child));
        },
        LayerStackRule::instance_cycle,
        "instance accepted its enclosing group as a recursive content source");
    return expect(stack.paint_target("source").identifier == "source",
                  "paint layer was not accepted as a paint target") &&
           expect(named_source, "instance paint refusal did not name its source") && forward &&
           self_cycle && pair_cycle && containment_cycle &&
           expect(stack == before, "reference refusal changed the valid stack");
}

bool deletion_policy_is_explicit_and_transactional() {
    LayerStack stack;
    stack.append(fill("source", 0.4));
    LayerEntry copy = instance("copy", "source");
    copy.opacity = 0.3;
    copy.blend_mode = "screen";
    copy.channels.push_back({.semantic_id = "pbr.height", .enabled = false, .opacity = 0.2});
    stack.append(std::move(copy));
    LayerEntry chained = instance("chained", "copy");
    stack.append(std::move(chained));
    LayerEntry mask = paint("copy-mask");
    mask.kind = LayerEntryKind::mask;
    mask.target_identifier = "copy";
    stack.append(std::move(mask));
    const LayerStack before = stack;
    const std::vector<std::string> source{"source"};
    bool named_instances = false;
    try {
        stack.remove(source, ReferencedSourceDeletionPolicy::refuse);
    } catch (const LayerStackError& error) {
        named_instances = error.rule() == LayerStackRule::live_instances &&
                          std::vector<std::string>(error.related_identifiers().begin(),
                                                   error.related_identifiers().end()) ==
                              std::vector<std::string>{"copy", "chained"};
    }
    const bool refused = expect(named_instances && stack == before,
                                "source deletion refusal did not name instances or changed state");

    stack.remove(source, ReferencedSourceDeletionPolicy::make_instances_independent);
    const LayerEntry& independent = stack.entry("copy");
    return refused && expect(!stack.contains("source"), "converted deletion retained the source") &&
           expect(independent.kind == LayerEntryKind::fill_layer &&
                      independent.source_identifier.empty() && independent.graph.has_value() &&
                      independent.opacity == 0.3 && independent.blend_mode == "screen" &&
                      !independent.channels.front().enabled &&
                      stack.entry("copy-mask").target_identifier == "copy",
                  "independent copy lost source content or instance-owned modulation") &&
           expect(stack.entry("chained").kind == LayerEntryKind::fill_layer &&
                      stack.entry("chained").source_identifier.empty(),
                  "transitive instance was not made independent");
}

bool group_instance_independence_requires_baked_content() {
    LayerStack stack;
    LayerEntry group = paint("group");
    group.kind = LayerEntryKind::group;
    stack.append(std::move(group));
    stack.append(instance("copy", "group"));
    const LayerStack before = stack;
    const std::vector<std::string> removed{"group"};
    return expect_rule(
               [&] {
                   stack.remove(removed,
                                ReferencedSourceDeletionPolicy::make_instances_independent);
               },
               LayerStackRule::live_instances,
               "group instance became an empty independent group without baked content") &&
           expect(stack == before, "refused group-instance independence changed the layer stack");
}

}  // namespace

int main() {
    return source_edits_are_shared_without_copying() && modulation_and_masks_are_instance_owned() &&
                   direct_paint_and_invalid_references_are_refused() &&
                   deletion_policy_is_explicit_and_transactional() &&
                   group_instance_independence_requires_baked_content()
               ? 0
               : 1;
}
