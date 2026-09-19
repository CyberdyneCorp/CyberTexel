#include <algorithm>
#include <ctex/graph/catalogue.hpp>
#include <ctex/graph/validation.hpp>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

namespace {

using ctex::graph::GraphDiagnosticCode;
using ctex::graph::SocketType;

ctex::graph::NodeSocket socket(std::string identifier, SocketType type,
                               ctex::graph::SocketValue value = {}) {
    return {.identifier = identifier,
            .display_name = std::move(identifier),
            .type = type,
            .value = std::move(value)};
}

ctex::graph::GraphNode colour_output() {
    return {.role = ctex::graph::NodeRole::output,
            .type_id = "test.output",
            .type_version = 1,
            .display_name = "Material Output",
            .position = {},
            .inputs = {socket("base_colour", SocketType::colour,
                              ctex::graph::ColourValue{0.0F, 0.0F, 0.0F, 1.0F}),
                       socket("roughness", SocketType::scalar, 0.5)},
            .outputs = {},
            .properties = {}};
}

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

const ctex::graph::GraphDiagnostic* find_diagnostic(
    const ctex::graph::GraphValidationReport& report, GraphDiagnosticCode code) {
    const auto found =
        std::find_if(report.diagnostics.begin(), report.diagnostics.end(),
                     [&](const auto& diagnostic) { return diagnostic.code == code; });
    return found == report.diagnostics.end() ? nullptr : &*found;
}

bool missing_downloaded_image_is_named_without_emission() {
    ctex::graph::GraphDocument graph(colour_output());
    auto image = ctex::graph::make_builtin_node("ctex.texture.image");
    std::get<ctex::graph::ImageValue>(image.properties[0].value).resource_id =
        "library/downloaded/albedo.png";
    const auto image_id = graph.add_node(std::move(image));
    static_cast<void>(graph.add_link({image_id, "colour", graph.output_node_id(), "base_colour"}));

    const auto missing = ctex::graph::validate_graph(graph);
    const auto* diagnostic = find_diagnostic(missing, GraphDiagnosticCode::missing_image_resource);
    if (!expect(!missing.valid() && missing.error_count() == 1 && missing.warning_count() == 0 &&
                    diagnostic != nullptr &&
                    diagnostic->subject == "library/downloaded/albedo.png" &&
                    diagnostic->message.find("library/downloaded/albedo.png") != std::string::npos,
                "validation did not name the unavailable downloaded image")) {
        return false;
    }
    const ctex::graph::GraphValidationResources resources{
        .image_resources = {"library/downloaded/albedo.png"}, .mesh_maps = {}};
    return expect(ctex::graph::validate_graph(graph, resources).valid(),
                  "available downloaded image was still reported missing");
}

bool missing_mesh_maps_are_named() {
    ctex::graph::GraphDocument graph(colour_output());
    auto mesh_map = ctex::graph::make_builtin_node("ctex.input.mesh-map");
    const auto node_id = graph.add_node(std::move(mesh_map));
    static_cast<void>(graph.add_link({node_id, "value", graph.output_node_id(), "roughness"}));

    const auto missing = ctex::graph::validate_graph(graph);
    const auto* diagnostic = find_diagnostic(missing, GraphDiagnosticCode::missing_mesh_map);
    if (!expect(diagnostic != nullptr && diagnostic->subject == "ambient_occlusion" &&
                    diagnostic->message.find("ambient_occlusion") != std::string::npos,
                "validation did not name the unavailable mesh map")) {
        return false;
    }
    const ctex::graph::GraphValidationResources resources{.image_resources = {},
                                                          .mesh_maps = {"ambient_occlusion"}};
    return expect(ctex::graph::validate_graph(graph, resources).valid(),
                  "available mesh map was still reported missing");
}

bool required_inputs_and_unreachable_nodes_are_distinct() {
    ctex::graph::GraphDocument graph(colour_output());
    const auto levels = graph.add_node(ctex::graph::make_builtin_node("ctex.colour.levels"));
    static_cast<void>(graph.add_link({levels, "colour", graph.output_node_id(), "base_colour"}));

    const auto incomplete = ctex::graph::validate_graph(graph);
    const auto* required =
        find_diagnostic(incomplete, GraphDiagnosticCode::unconnected_required_input);
    if (!expect(!incomplete.valid() && required != nullptr && required->node_id == levels &&
                    required->subject == "colour",
                "required unconnected input was not reported by socket name")) {
        return false;
    }

    const auto constant =
        graph.add_node(ctex::graph::make_builtin_node("ctex.input.constant-colour"));
    static_cast<void>(graph.add_link({constant, "colour", levels, "colour"}));
    const auto unused = graph.add_node(ctex::graph::make_builtin_node("ctex.input.constant-value"));
    const auto complete = ctex::graph::validate_graph(graph);
    const auto* unreachable = find_diagnostic(complete, GraphDiagnosticCode::unreachable_node);
    return expect(complete.valid() && complete.error_count() == 0 &&
                      complete.warning_count() == 1 && unreachable != nullptr &&
                      unreachable->node_id == unused,
                  "unreachable node was not a non-blocking, precisely located warning");
}

bool image_values_in_sockets_are_checked() {
    ctex::graph::GraphDocument graph(colour_output());
    ctex::graph::GraphNode custom{
        .role = ctex::graph::NodeRole::regular,
        .type_id = "host.image-source",
        .type_version = 1,
        .display_name = "Host Image",
        .position = {},
        .inputs = {socket("image", SocketType::image, ctex::graph::ImageValue{"host/missing.exr"})},
        .outputs = {socket("colour", SocketType::colour)},
        .properties = {},
    };
    const auto node_id = graph.add_node(std::move(custom));
    static_cast<void>(graph.add_link({node_id, "colour", graph.output_node_id(), "base_colour"}));
    const auto report = ctex::graph::validate_graph(graph);
    const auto* diagnostic = find_diagnostic(report, GraphDiagnosticCode::missing_image_resource);
    return expect(diagnostic != nullptr && diagnostic->subject == "host/missing.exr" &&
                      diagnostic->message.find("host/missing.exr") != std::string::npos,
                  "image-valued socket resource was not validated");
}

bool workspace_diagnostics_retain_ownership_and_missing_groups() {
    ctex::graph::GraphWorkspace workspace;
    ctex::graph::GraphDocument material(colour_output());
    ctex::graph::GraphNode missing_group{
        .role = ctex::graph::NodeRole::regular,
        .type_id = "ctex.group-instance",
        .type_version = 1,
        .display_name = "Missing Group",
        .position = {},
        .inputs = {},
        .outputs = {socket("colour", SocketType::colour)},
        .properties = {{"group_id", std::string("library.weathering")}},
    };
    const auto instance = material.add_node(std::move(missing_group));
    static_cast<void>(
        material.add_link({instance, "colour", material.output_node_id(), "base_colour"}));
    workspace.add_material("downloaded-material", std::move(material));
    workspace.create_group("unfinished", "Unfinished", {}, {socket("value", SocketType::scalar)});

    const auto report = ctex::graph::validate_workspace(workspace);
    const auto missing =
        std::find_if(report.diagnostics.begin(), report.diagnostics.end(), [](const auto& owned) {
            return owned.diagnostic.code == GraphDiagnosticCode::missing_group;
        });
    const auto required =
        std::find_if(report.diagnostics.begin(), report.diagnostics.end(), [](const auto& owned) {
            return owned.diagnostic.code == GraphDiagnosticCode::unconnected_required_input;
        });
    return expect(!report.valid() && missing != report.diagnostics.end() &&
                      missing->owner_kind == ctex::graph::GraphOwnerKind::material &&
                      missing->owner_identifier == "downloaded-material" &&
                      missing->diagnostic.subject == "library.weathering",
                  "workspace did not retain missing-group material ownership") &&
           expect(required != report.diagnostics.end() &&
                      required->owner_kind == ctex::graph::GraphOwnerKind::group &&
                      required->owner_identifier == "unfinished",
                  "workspace did not retain group-subgraph diagnostic ownership");
}

}  // namespace

int main() {
    return missing_downloaded_image_is_named_without_emission() && missing_mesh_maps_are_named() &&
                   required_inputs_and_unreachable_nodes_are_distinct() &&
                   image_values_in_sockets_are_checked() &&
                   workspace_diagnostics_retain_ownership_and_missing_groups()
               ? 0
               : 1;
}
