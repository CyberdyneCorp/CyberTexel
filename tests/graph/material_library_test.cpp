#include <ctex/graph/catalogue.hpp>
#include <ctex/graph/material_library.hpp>
#include <iostream>
#include <string_view>

namespace {

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

ctex::graph::GraphDocument material(double value) {
    ctex::graph::GraphNode output{
        .role = ctex::graph::NodeRole::output,
        .type_id = "test.output",
        .type_version = 1,
        .display_name = "Output",
        .position = {},
        .inputs = {{"roughness", "Roughness", ctex::graph::SocketType::scalar, 0.5}},
        .outputs = {},
        .properties = {},
    };
    ctex::graph::GraphDocument graph(std::move(output));
    auto constant = ctex::graph::make_builtin_node("ctex.input.constant-value");
    constant.properties[0].value = value;
    const auto constant_id = graph.add_node(std::move(constant));
    static_cast<void>(graph.add_link({constant_id, "value", graph.output_node_id(), "roughness"}));
    return graph;
}

bool library_round_trip_is_canonical() {
    ctex::graph::MaterialLibrary first;
    first.add({"org.cybertexel.material.wood", "Wood", "thumbs/wood.png", material(0.7)});
    first.add({"org.cybertexel.material.metal", "Metal", "thumbs/metal.png", material(0.2)});

    ctex::graph::MaterialLibrary reverse;
    reverse.add({"org.cybertexel.material.metal", "Metal", "thumbs/metal.png", material(0.2)});
    reverse.add({"org.cybertexel.material.wood", "Wood", "thumbs/wood.png", material(0.7)});

    const std::string serialized = ctex::graph::serialize_material_library(first);
    const ctex::graph::MaterialLibrary reopened =
        ctex::graph::deserialize_material_library(serialized);
    return expect(serialized == ctex::graph::serialize_material_library(reverse),
                  "material library bytes depended on insertion order") &&
           expect(
               reopened == first && ctex::graph::serialize_material_library(reopened) == serialized,
               "material library did not round-trip canonically");
}

bool stable_identity_resolves_the_same_graph_on_another_machine() {
    ctex::graph::MaterialLibrary source;
    source.add({"org.cybertexel.material.wood", "Wood", "thumbs/wood.png", material(0.7)});
    const ctex::graph::MaterialLibrary destination =
        ctex::graph::deserialize_material_library(ctex::graph::serialize_material_library(source));

    ctex::graph::GraphDocument applied = destination.instantiate("org.cybertexel.material.wood");
    applied.set_node_position(2, {9.0F, 4.0F});
    return expect(destination.find("org.cybertexel.material.wood") != nullptr,
                  "stable preset identity was not found after transfer") &&
           expect(destination.instantiate("org.cybertexel.material.wood") == material(0.7),
                  "transferred preset resolved to a different graph") &&
           expect(applied != destination.instantiate("org.cybertexel.material.wood"),
                  "instantiating a preset did not return an independent graph");
}

bool invalid_and_duplicate_identities_are_refused() {
    ctex::graph::MaterialLibrary library;
    library.add({"stable", "Material", {}, material(0.5)});
    bool duplicate_refused = false;
    try {
        library.add({"stable", "Other", {}, material(0.4)});
    } catch (const ctex::graph::MaterialLibraryError& error) {
        duplicate_refused = std::string_view(error.what()).find("stable") != std::string_view::npos;
    }
    bool missing_refused = false;
    try {
        static_cast<void>(library.instantiate("missing"));
    } catch (const ctex::graph::MaterialLibraryError& error) {
        missing_refused = std::string_view(error.what()).find("missing") != std::string_view::npos;
    }
    return expect(duplicate_refused && missing_refused,
                  "duplicate or missing stable identities were not named and refused");
}

}  // namespace

int main() {
    return library_round_trip_is_canonical() &&
                   stable_identity_resolves_the_same_graph_on_another_machine() &&
                   invalid_and_duplicate_identities_are_refused()
               ? 0
               : 1;
}
