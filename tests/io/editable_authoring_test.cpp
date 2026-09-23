#include <ctex/doc/editable_authoring.hpp>
#include <ctex/io/editable_authoring.hpp>
#include <ctex/io/project_container.hpp>
#include <ctex/paint/editable_surface_path.hpp>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

ctex::doc::EditableAuthoringEntry text_entry() {
    return {.identifier = "title",
            .kind = ctex::doc::EditableEntryKind::text,
            .revision = 1,
            .placement = {.position = {1.0, 2.0, 3.0},
                          .normal = {0.0, 0.0, 1.0},
                          .rotation_radians = 0.25,
                          .uniform_scale = 2.0,
                          .axis_scale = {1.0, 0.5}},
            .material_identity = "paint/gold",
            .material_parameters = {{.identifier = "roughness",
                                     .component_count = 1,
                                     .value = {0.35, 0.0, 0.0, 0.0}}},
            .text = "CyberTexel",
            .font_identity = "fonts/inter-bold-v4",
            .mesh_revision = 0,
            .surface_points = {},
            .dependent_tiles = {{.semantic_id = "base-color", .tile_x = 2, .tile_y = 1}}};
}

ctex::doc::EditableAuthoringEntry path_entry() {
    return {.identifier = "seam-line",
            .kind = ctex::doc::EditableEntryKind::surface_path,
            .revision = 1,
            .placement = {},
            .material_identity = "paint/thread",
            .material_parameters = {{.identifier = "opacity",
                                     .component_count = 1,
                                     .value = {0.8, 0.0, 0.0, 0.0}}},
            .text = {},
            .font_identity = {},
            .mesh_revision = 44,
            .surface_points = {{.position = {0.0, 0.0, 0.0},
                                .normal = {0.0, 0.0, 1.0},
                                .triangle = 7,
                                .barycentric = {1.0, 0.0, 0.0},
                                .width = 0.5},
                               {.position = {2.0, 0.0, 0.0},
                                .normal = {0.0, 0.0, 1.0},
                                .triangle = 8,
                                .barycentric = {0.0, 1.0, 0.0},
                                .width = 1.0}},
            .dependent_tiles = {{.semantic_id = "base-color", .tile_x = 0, .tile_y = 0},
                                {.semantic_id = "base-color", .tile_x = 1, .tile_y = 0}}};
}

void test_edit_undo_and_explicit_rasterization() {
    ctex::doc::EditableAuthoringStore store;
    const auto added = store.add(text_entry());
    expect(added.invalidated_tiles.size() == 1 && store.undo_step_count() == 1,
           "adding editable text must invalidate its tile and create one undo step");

    auto edited = store.entry("title");
    edited.text = "CyberTexel 2";
    edited.placement.position[0] = 9.0;
    edited.dependent_tiles.push_back({.semantic_id = "base-color", .tile_x = 3, .tile_y = 1});
    const auto report = store.edit(edited, 1);
    expect(report.entry_revision == 2 && report.invalidated_tiles.size() == 2,
           "editing text must invalidate the union of old and new dependent tiles");
    expect(store.entry("title").text == "CyberTexel 2" && store.undo_step_count() == 2,
           "text edit must remain editable and create exactly one undo step");

    const auto undone = store.undo();
    expect(undone.entry_present && store.entry("title").text == "CyberTexel" &&
               store.entry("title").placement.position[0] == 1.0,
           "undo must restore text and placement without flattening");
    const auto redone = store.redo();
    expect(redone.entry_revision == 2 && store.entry("title").text == "CyberTexel 2",
           "redo must restore the edited entry");

    const auto plan = store.rasterization_plan("title");
    expect(plan.entry.text == "CyberTexel 2" && plan.invalidated_tiles.size() == 2,
           "rasterization must be an explicit plan that leaves the entry editable");
    expect(store.entry("title").text == "CyberTexel 2",
           "planning rasterization must not flatten the entry");
}

void test_project_round_trip() {
    ctex::doc::EditableAuthoringStore store;
    static_cast<void>(store.add(text_entry()));
    static_cast<void>(store.add(path_entry()));
    ctex::io::ProjectContainer project;
    ctex::io::upsert_editable_authoring(project, "set/body/editable", store);
    const auto encoded = ctex::io::write_project_container(project);
    const auto reopened = ctex::io::read_project_container(encoded);
    expect(reopened.container.assets.size() == 1,
           "project must retain the editable-authoring asset");
    auto restored = ctex::io::unpack_editable_authoring(reopened.container.assets.front());
    expect(restored.entries().size() == 2 && restored.entry("title") == text_entry() &&
               restored.entry("seam-line") == path_entry(),
           "placement, text, font, path and material parameters must round-trip losslessly");

    auto saved_text = restored.entry("title");
    saved_text.text = "Reopened edit";
    static_cast<void>(restored.edit(saved_text, saved_text.revision));
    static_cast<void>(restored.undo());
    expect(restored.entry("title").text == "CyberTexel",
           "a reopened text entry must remain editable and undoable");
}

void test_surface_path_uses_stroke_model() {
    ctex::paint::StrokeSettings settings;
    settings.spacing_fraction = 0.5;
    settings.tip_resource_identity = "tips/round";
    const auto resolved = ctex::paint::evaluate_editable_surface_path(path_entry(), settings);
    expect(resolved.stamps.size() >= 2 && !resolved.swept_segments.empty(),
           "surface path must evaluate through continuous stroke reconstruction");
    expect(resolved.stamps.front().radius == 0.5 && resolved.stamps.back().radius == 1.0,
           "surface path widths must drive stroke-model radii");
    expect(resolved.stamps.front().frame.normal.z == 1.0,
           "surface attachment normals must define the resolved stroke frame");

    bool non_path_refused = false;
    try {
        static_cast<void>(ctex::paint::evaluate_editable_surface_path(text_entry(), settings));
    } catch (const ctex::paint::StrokeResolutionError&) {
        non_path_refused = true;
    }
    expect(non_path_refused, "surface-path evaluation must validate the entry kind first");
}

void test_bounded_malformed_input() {
    ctex::doc::EditableAuthoringStore store;
    static_cast<void>(store.add(text_entry()));
    auto encoded = ctex::io::serialize_editable_authoring(store);
    encoded.pop_back();
    bool refused = false;
    try {
        static_cast<void>(ctex::io::deserialize_editable_authoring(encoded));
    } catch (const ctex::io::EditableAuthoringIoError&) {
        refused = true;
    }
    expect(refused, "truncated editable-authoring data must be refused");

    ctex::io::EditableAuthoringReadLimits limits;
    limits.maximum_parameters = 0;
    refused = false;
    try {
        static_cast<void>(ctex::io::deserialize_editable_authoring(
            ctex::io::serialize_editable_authoring(store), limits));
    } catch (const ctex::io::EditableAuthoringIoError&) {
        refused = true;
    }
    expect(refused, "editable-authoring aggregate read limits must be enforced");
}

}  // namespace

int main() {
    try {
        test_edit_undo_and_explicit_rasterization();
        test_project_round_trip();
        test_surface_path_uses_stroke_model();
        test_bounded_malformed_input();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
