#include <algorithm>
#include <ctex/io/export_plan.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Callable>
bool expect_error(Callable&& callable, ExportPlanErrorCode code, std::string_view message_part) {
    try {
        callable();
    } catch (const ExportPlanError& error) {
        return expect(error.code() == code, "export plan returned the wrong error code") &&
               expect(std::string_view(error.what()).find(message_part) != std::string_view::npos,
                      "export plan diagnostic omitted the failing value");
    } catch (...) {
    }
    return expect(false, "expected export planning error was not thrown");
}

ExportSourceCatalogue catalogue() {
    return {
        .project_name = "Robot Project",
        .texture_sets =
            {
                {.identifier = "body",
                 .display_name = "Body",
                 .width = 2048,
                 .height = 2048,
                 .occupied_udim_tiles = {1002, 1001},
                 .layers =
                     {
                         {.identifier = "materials",
                          .display_name = "Materials",
                          .parent_identifier = {},
                          .kind = ExportLayerKind::group,
                          .visible = true},
                         {.identifier = "fill",
                          .display_name = "Fill",
                          .parent_identifier = "materials",
                          .kind = ExportLayerKind::content,
                          .visible = true},
                         {.identifier = "wear",
                          .display_name = "Hidden Wear",
                          .parent_identifier = "materials",
                          .kind = ExportLayerKind::content,
                          .visible = false},
                         {.identifier = "detail",
                          .display_name = "Detail",
                          .parent_identifier = {},
                          .kind = ExportLayerKind::content,
                          .visible = true},
                     }},
                {.identifier = "head",
                 .display_name = "Head",
                 .width = 1024,
                 .height = 512,
                 .occupied_udim_tiles = {1001},
                 .layers =
                     {
                         {.identifier = "head-base",
                          .display_name = "Head Base",
                          .parent_identifier = {},
                          .kind = ExportLayerKind::content,
                          .visible = true},
                     }},
            },
        .atlases =
            {
                {.identifier = "character-atlas",
                 .display_name = "Character Atlas",
                 .width = 4096,
                 .height = 4096,
                 .texture_set_identifiers = {"body", "head"}},
            },
    };
}

const ExportPreset& one_texture_preset() { return built_in_export_preset("base-color"); }

bool all_and_selected_texture_sets_plan_predictably() {
    const auto all = plan_texture_export(catalogue(), one_texture_preset());
    ExportPlanRequest selected_request;
    selected_request.texture_set_selection = ExportTextureSetSelection::selected;
    selected_request.selected_texture_set_identifiers = {"head"};
    const auto selected = plan_texture_export(catalogue(), one_texture_preset(), selected_request);
    ExportPlanRequest nested_request = selected_request;
    nested_request.filename_pattern = "exports/{texture_set}/{suffix}.{extension}";
    const auto nested = plan_texture_export(catalogue(), one_texture_preset(), nested_request);
    return expect(all.size() == 2, "all-texture-sets scope did not plan both sets") &&
           expect(all[0].relative_path == "Robot_Project_Body_BaseColor_2048_8_single_visible.png",
                  "default filename pattern did not expand predictably") &&
           expect(
               all[1].relative_path == "Robot_Project_Head_BaseColor_1024x512_8_single_visible.png",
               "texture-set and non-square resolution tokens were not expanded") &&
           expect(all[0].layer_identifiers.at("body") ==
                      std::vector<std::string>({"materials", "fill", "detail"}),
                  "flatten-visible scope did not apply effective visibility") &&
           expect(selected.size() == 1 && selected.front().texture_set_identifiers ==
                                              std::vector<std::string>({"head"}),
                  "selected-texture-sets scope included an unselected set") &&
           expect(nested.front().relative_path == "exports/Head/_BaseColor.png",
                  "overridden filename pattern did not preserve relative directories");
}

bool udim_and_atlas_scopes_are_composable() {
    ExportPlanRequest tile_request;
    tile_request.texture_set_selection = ExportTextureSetSelection::selected;
    tile_request.selected_texture_set_identifiers = {"body"};
    tile_request.spatial_scope = ExportSpatialScope::udim_tile;
    tile_request.layer_scope = ExportLayerScope::flatten_selected;
    tile_request.selected_layer_identifiers = {{"body", {"materials"}}};
    const auto per_tile = plan_texture_export(catalogue(), one_texture_preset(), tile_request);
    ExportPlanRequest atlas_request;
    atlas_request.spatial_scope = ExportSpatialScope::atlas;
    const auto per_atlas = plan_texture_export(catalogue(), one_texture_preset(), atlas_request);
    return expect(per_tile.size() == 2 && per_tile[0].udim_tile == 1001 &&
                      per_tile[1].udim_tile == 1002,
                  "per-UDIM scope did not emit occupied tiles in deterministic order") &&
           expect(per_tile[0].relative_path.find("_1001_") != std::string::npos &&
                      per_tile[1].relative_path.find("_1002_") != std::string::npos,
                  "per-UDIM filenames omitted the tile number") &&
           expect(per_tile[0].layer_identifiers.at("body") ==
                      std::vector<std::string>({"materials", "fill", "wear"}),
                  "per-UDIM scope did not compose with the selected-layer scope") &&
           expect(per_atlas.size() == 1 &&
                      per_atlas.front().atlas_identifier == "character-atlas" &&
                      per_atlas.front().texture_set_identifiers ==
                          std::vector<std::string>({"body", "head"}) &&
                      per_atlas.front().width == 4096,
                  "per-atlas scope did not combine its texture sets and dimensions");
}

bool groups_expand_and_separate_layers_remain_distinct() {
    ExportPlanRequest flattened;
    flattened.texture_set_selection = ExportTextureSetSelection::selected;
    flattened.selected_texture_set_identifiers = {"body"};
    flattened.layer_scope = ExportLayerScope::flatten_selected;
    flattened.selected_layer_identifiers = {{"body", {"materials"}}};
    const auto group = plan_texture_export(catalogue(), one_texture_preset(), flattened);
    ExportPlanRequest separate = flattened;
    separate.layer_scope = ExportLayerScope::each_selected;
    separate.selected_layer_identifiers.at("body") = {"materials", "fill", "detail"};
    const auto layers = plan_texture_export(catalogue(), one_texture_preset(), separate);
    return expect(group.size() == 1 && group.front().layer_identifiers.at("body") ==
                                           std::vector<std::string>({"materials", "fill", "wear"}),
                  "selected group did not include all children in the flattened result") &&
           expect(layers.size() == 2,
                  "selected group child was emitted twice in separate-layer scope") &&
           expect(layers[0].relative_path.find("Materials") != std::string::npos &&
                      layers[1].relative_path.find("Detail") != std::string::npos,
                  "separate-layer filenames did not include their layer token");
}

bool collisions_and_invalid_patterns_are_refused_before_output() {
    ExportPlanRequest collision;
    collision.filename_pattern = "{project}{suffix}.{extension}";
    const bool collision_refused = expect_error(
        [&] {
            static_cast<void>(plan_texture_export(catalogue(), one_texture_preset(), collision));
        },
        ExportPlanErrorCode::path_collision, "Robot_Project_BaseColor.png");
    ExportPlanRequest unknown_token;
    unknown_token.filename_pattern = "{project}_{unknown}.png";
    const bool token_refused = expect_error(
        [&] {
            static_cast<void>(
                plan_texture_export(catalogue(), one_texture_preset(), unknown_token));
        },
        ExportPlanErrorCode::invalid_pattern, "unknown");
    ExportPlanRequest parent_path;
    parent_path.filename_pattern = "../{project}.png";
    const bool parent_refused = expect_error(
        [&] {
            static_cast<void>(plan_texture_export(catalogue(), one_texture_preset(), parent_path));
        },
        ExportPlanErrorCode::invalid_pattern, "portable");
    ExportPlanRequest illegal_character;
    illegal_character.filename_pattern = "bad?/{project}.png";
    return collision_refused && token_refused && parent_refused &&
           expect_error(
               [&] {
                   static_cast<void>(
                       plan_texture_export(catalogue(), one_texture_preset(), illegal_character));
               },
               ExportPlanErrorCode::invalid_pattern, "portable");
}

bool invalid_scope_and_hierarchy_are_named() {
    ExportPlanRequest no_selection;
    no_selection.texture_set_selection = ExportTextureSetSelection::selected;
    const bool selection_refused = expect_error(
        [&] {
            static_cast<void>(plan_texture_export(catalogue(), one_texture_preset(), no_selection));
        },
        ExportPlanErrorCode::invalid_scope, "at least one");
    ExportSourceCatalogue cyclic = catalogue();
    cyclic.texture_sets.front().layers.front().parent_identifier = "materials";
    return selection_refused &&
           expect_error(
               [&] { static_cast<void>(plan_texture_export(cyclic, one_texture_preset())); },
               ExportPlanErrorCode::invalid_catalogue, "cycle");
}

}  // namespace

int main() {
    return all_and_selected_texture_sets_plan_predictably() &&
                   udim_and_atlas_scopes_are_composable() &&
                   groups_expand_and_separate_layers_remain_distinct() &&
                   collisions_and_invalid_patterns_are_refused_before_output() &&
                   invalid_scope_and_hierarchy_are_named()
               ? 0
               : 1;
}
