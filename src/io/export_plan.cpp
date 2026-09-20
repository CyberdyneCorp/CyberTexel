#include <algorithm>
#include <array>
#include <ctex/doc/document.hpp>
#include <ctex/io/export_plan.hpp>
#include <map>
#include <set>
#include <string_view>
#include <utility>

namespace ctex::io {
namespace {

[[noreturn]] void plan_error(ExportPlanErrorCode code, std::string message) {
    throw ExportPlanError(code, std::move(message));
}

using TextureSetLookup = std::map<std::string_view, const ExportTextureSetSource*, std::less<>>;
using LayerLookup = std::map<std::string_view, const ExportLayerSource*, std::less<>>;

bool atlas_regions_overlap(const ExportAtlasRegion& left, const ExportAtlasRegion& right) {
    const std::uint64_t left_right = static_cast<std::uint64_t>(left.x) + left.width;
    const std::uint64_t right_right = static_cast<std::uint64_t>(right.x) + right.width;
    const std::uint64_t left_bottom = static_cast<std::uint64_t>(left.y) + left.height;
    const std::uint64_t right_bottom = static_cast<std::uint64_t>(right.y) + right.height;
    return left.x < right_right && right.x < left_right && left.y < right_bottom &&
           right.y < left_bottom;
}

std::vector<std::string_view> validate_atlas_regions(const ExportAtlasSource& atlas,
                                                     const TextureSetLookup& texture_sets) {
    std::vector<std::string_view> members;
    members.reserve(atlas.regions.size());
    std::set<std::string_view> seen;
    for (std::size_t index = 0; index < atlas.regions.size(); ++index) {
        const ExportAtlasRegion& region = atlas.regions[index];
        const std::uint64_t right = static_cast<std::uint64_t>(region.x) + region.width;
        const std::uint64_t bottom = static_cast<std::uint64_t>(region.y) + region.height;
        if (region.texture_set_identifier.empty() || region.width == 0 || region.height == 0 ||
            right > atlas.width || bottom > atlas.height) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export atlas has an invalid region: " + region.texture_set_identifier);
        }
        if (!texture_sets.contains(region.texture_set_identifier)) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export atlas region names a missing texture set: " +
                           region.texture_set_identifier);
        }
        if (!seen.insert(region.texture_set_identifier).second) {
            plan_error(
                ExportPlanErrorCode::invalid_catalogue,
                "export atlas repeats a texture-set region: " + region.texture_set_identifier);
        }
        for (std::size_t previous = 0; previous < index; ++previous) {
            if (atlas_regions_overlap(region, atlas.regions[previous])) {
                plan_error(ExportPlanErrorCode::invalid_catalogue,
                           "export atlas regions overlap: " + region.texture_set_identifier);
            }
        }
        members.push_back(region.texture_set_identifier);
    }
    return members;
}

std::vector<std::string_view> atlas_members(const ExportAtlasSource& atlas,
                                            const TextureSetLookup& texture_sets) {
    if (!atlas.regions.empty()) {
        std::vector<std::string_view> members = validate_atlas_regions(atlas, texture_sets);
        if (!atlas.texture_set_identifiers.empty() &&
            !std::equal(members.begin(), members.end(), atlas.texture_set_identifiers.begin(),
                        atlas.texture_set_identifiers.end())) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export atlas membership and regions disagree: " + atlas.identifier);
        }
        return members;
    }
    std::vector<std::string_view> members;
    members.reserve(atlas.texture_set_identifiers.size());
    for (const std::string& identifier : atlas.texture_set_identifiers) {
        members.push_back(identifier);
    }
    return members;
}

LayerLookup validate_layers(const ExportTextureSetSource& texture_set) {
    LayerLookup layers;
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (layer.identifier.empty() || layer.display_name.empty()) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export layers require non-empty identifiers and display names");
        }
        if (layer.kind != ExportLayerKind::content && layer.kind != ExportLayerKind::group) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export layer kind is invalid: " + layer.identifier);
        }
        if (!layers.emplace(layer.identifier, &layer).second) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "duplicate export layer identifier: " + layer.identifier);
        }
    }
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (layer.parent_identifier.empty()) {
            continue;
        }
        const auto parent = layers.find(layer.parent_identifier);
        if (parent == layers.end()) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export layer parent is missing: " + layer.parent_identifier);
        }
        if (parent->second->kind != ExportLayerKind::group) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export layer parent is not a group: " + layer.parent_identifier);
        }
    }
    for (const ExportLayerSource& layer : texture_set.layers) {
        const ExportLayerSource* current = &layer;
        for (std::size_t depth = 0; !current->parent_identifier.empty(); ++depth) {
            if (depth >= texture_set.layers.size()) {
                plan_error(ExportPlanErrorCode::invalid_catalogue,
                           "export layer hierarchy contains a cycle: " + layer.identifier);
            }
            current = layers.at(current->parent_identifier);
        }
    }
    return layers;
}

TextureSetLookup validate_catalogue(const ExportSourceCatalogue& catalogue) {
    if (catalogue.project_name.empty() || catalogue.texture_sets.empty()) {
        plan_error(ExportPlanErrorCode::invalid_catalogue,
                   "export catalogue requires a project name and at least one texture set");
    }
    TextureSetLookup texture_sets;
    for (const ExportTextureSetSource& texture_set : catalogue.texture_sets) {
        if (texture_set.identifier.empty() || texture_set.display_name.empty() ||
            texture_set.width == 0 || texture_set.height == 0) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export texture sets require identity, name, and non-zero dimensions");
        }
        if (!texture_sets.emplace(texture_set.identifier, &texture_set).second) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "duplicate export texture-set identifier: " + texture_set.identifier);
        }
        std::set<std::uint32_t> tiles;
        for (const std::uint32_t tile : texture_set.occupied_udim_tiles) {
            if (tile < 1001 || !tiles.insert(tile).second) {
                plan_error(ExportPlanErrorCode::invalid_catalogue,
                           "occupied UDIM tiles must be unique numbers from 1001 upward");
            }
        }
        static_cast<void>(validate_layers(texture_set));
    }

    std::set<std::string_view> atlas_ids;
    std::set<std::string_view> assigned_texture_sets;
    for (const ExportAtlasSource& atlas : catalogue.atlases) {
        if (atlas.identifier.empty() || atlas.display_name.empty() || atlas.width == 0 ||
            atlas.height == 0 || (atlas.texture_set_identifiers.empty() && atlas.regions.empty())) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "export atlases require identity, name, dimensions, and texture sets");
        }
        if (!atlas_ids.insert(atlas.identifier).second) {
            plan_error(ExportPlanErrorCode::invalid_catalogue,
                       "duplicate export atlas identifier: " + atlas.identifier);
        }
        std::set<std::string_view> members;
        for (const std::string_view identifier : atlas_members(atlas, texture_sets)) {
            if (!texture_sets.contains(identifier)) {
                plan_error(ExportPlanErrorCode::invalid_catalogue,
                           "export atlas names a missing texture set: " + std::string(identifier));
            }
            if (!members.insert(identifier).second ||
                !assigned_texture_sets.insert(identifier).second) {
                plan_error(
                    ExportPlanErrorCode::invalid_catalogue,
                    "texture set has a duplicate atlas assignment: " + std::string(identifier));
            }
        }
    }
    return texture_sets;
}

std::vector<const ExportTextureSetSource*> select_texture_sets(
    const ExportSourceCatalogue& catalogue, const ExportPlanRequest& request,
    const TextureSetLookup& lookup) {
    if (request.texture_set_selection != ExportTextureSetSelection::all &&
        request.texture_set_selection != ExportTextureSetSelection::selected) {
        plan_error(ExportPlanErrorCode::invalid_scope, "export texture-set selection is invalid");
    }
    if (request.texture_set_selection == ExportTextureSetSelection::all) {
        if (!request.selected_texture_set_identifiers.empty()) {
            plan_error(ExportPlanErrorCode::invalid_scope,
                       "all-texture-sets scope cannot also carry a selected-set list");
        }
        std::vector<const ExportTextureSetSource*> result;
        result.reserve(catalogue.texture_sets.size());
        for (const ExportTextureSetSource& texture_set : catalogue.texture_sets) {
            result.push_back(&texture_set);
        }
        return result;
    }
    if (request.selected_texture_set_identifiers.empty()) {
        plan_error(ExportPlanErrorCode::invalid_scope,
                   "selected-texture-sets scope requires at least one selection");
    }
    std::set<std::string_view> selected;
    for (const std::string& identifier : request.selected_texture_set_identifiers) {
        if (!lookup.contains(identifier)) {
            plan_error(ExportPlanErrorCode::invalid_scope,
                       "selected texture set is missing: " + identifier);
        }
        if (!selected.insert(identifier).second) {
            plan_error(ExportPlanErrorCode::invalid_scope,
                       "selected texture set is duplicated: " + identifier);
        }
    }
    std::vector<const ExportTextureSetSource*> result;
    for (const ExportTextureSetSource& texture_set : catalogue.texture_sets) {
        if (selected.contains(texture_set.identifier)) {
            result.push_back(&texture_set);
        }
    }
    return result;
}

struct ScopeUnit {
    std::string display_name;
    std::vector<const ExportTextureSetSource*> texture_sets;
    std::optional<std::uint32_t> udim_tile;
    const ExportAtlasSource* atlas{};
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<ExportAtlasRegion> atlas_regions;
};

std::vector<ScopeUnit> texture_set_units(
    const std::vector<const ExportTextureSetSource*>& texture_sets) {
    std::vector<ScopeUnit> result;
    result.reserve(texture_sets.size());
    for (const ExportTextureSetSource* texture_set : texture_sets) {
        result.push_back({.display_name = texture_set->display_name,
                          .texture_sets = {texture_set},
                          .udim_tile = std::nullopt,
                          .atlas = nullptr,
                          .width = texture_set->width,
                          .height = texture_set->height,
                          .atlas_regions = {}});
    }
    return result;
}

std::vector<ScopeUnit> udim_units(const std::vector<const ExportTextureSetSource*>& texture_sets) {
    std::vector<ScopeUnit> result;
    for (const ExportTextureSetSource* texture_set : texture_sets) {
        std::vector<std::uint32_t> tiles = texture_set->occupied_udim_tiles;
        std::sort(tiles.begin(), tiles.end());
        for (const std::uint32_t tile : tiles) {
            result.push_back({.display_name = texture_set->display_name,
                              .texture_sets = {texture_set},
                              .udim_tile = tile,
                              .atlas = nullptr,
                              .width = texture_set->width,
                              .height = texture_set->height,
                              .atlas_regions = {}});
        }
    }
    if (result.empty()) {
        plan_error(ExportPlanErrorCode::invalid_scope,
                   "per-UDIM export has no occupied tiles in the selected texture sets");
    }
    return result;
}

std::vector<ScopeUnit> atlas_units(const ExportSourceCatalogue& catalogue,
                                   const std::vector<const ExportTextureSetSource*>& texture_sets,
                                   const TextureSetLookup& lookup) {
    std::set<std::string_view> selected;
    for (const ExportTextureSetSource* texture_set : texture_sets) {
        selected.insert(texture_set->identifier);
    }
    std::set<std::string_view> assigned;
    std::vector<ScopeUnit> result;
    for (const ExportAtlasSource& atlas : catalogue.atlases) {
        ScopeUnit unit{.display_name = atlas.display_name,
                       .texture_sets = {},
                       .udim_tile = std::nullopt,
                       .atlas = &atlas,
                       .width = atlas.width,
                       .height = atlas.height,
                       .atlas_regions = {}};
        const std::vector<std::string_view> members = atlas_members(atlas, lookup);
        for (const std::string_view identifier : members) {
            if (selected.contains(identifier)) {
                unit.texture_sets.push_back(lookup.at(identifier));
                assigned.insert(identifier);
                const auto region =
                    std::find_if(atlas.regions.begin(), atlas.regions.end(),
                                 [&](const ExportAtlasRegion& value) {
                                     return value.texture_set_identifier == identifier;
                                 });
                if (region != atlas.regions.end()) {
                    unit.atlas_regions.push_back(*region);
                }
            }
        }
        if (!unit.texture_sets.empty()) {
            result.push_back(std::move(unit));
        }
    }
    for (const ExportTextureSetSource* texture_set : texture_sets) {
        if (!assigned.contains(texture_set->identifier)) {
            plan_error(ExportPlanErrorCode::invalid_scope,
                       "per-atlas export has no atlas for texture set: " + texture_set->identifier);
        }
    }
    return result;
}

std::vector<ScopeUnit> make_scope_units(
    const ExportSourceCatalogue& catalogue, const ExportPlanRequest& request,
    const std::vector<const ExportTextureSetSource*>& texture_sets,
    const TextureSetLookup& lookup) {
    switch (request.spatial_scope) {
        case ExportSpatialScope::texture_set:
            return texture_set_units(texture_sets);
        case ExportSpatialScope::udim_tile:
            return udim_units(texture_sets);
        case ExportSpatialScope::atlas:
            return atlas_units(catalogue, texture_sets, lookup);
    }
    plan_error(ExportPlanErrorCode::invalid_scope, "export spatial scope is invalid");
}

void apply_output_resolution(std::vector<ScopeUnit>& units, const ExportPlanRequest& request) {
    if (!request.output_resolution.has_value()) {
        return;
    }
    if (request.output_resolution->width == 0 || request.output_resolution->height == 0) {
        plan_error(ExportPlanErrorCode::invalid_scope,
                   "export resolution dimensions must both be non-zero");
    }
    for (ScopeUnit& unit : units) {
        for (ExportAtlasRegion& region : unit.atlas_regions) {
            const auto scale_edge = [](std::uint32_t edge, std::uint32_t output,
                                       std::uint32_t source) {
                return static_cast<std::uint32_t>(static_cast<std::uint64_t>(edge) * output /
                                                  source);
            };
            const std::uint32_t right =
                scale_edge(region.x + region.width, request.output_resolution->width, unit.width);
            const std::uint32_t bottom = scale_edge(region.y + region.height,
                                                    request.output_resolution->height, unit.height);
            region.x = scale_edge(region.x, request.output_resolution->width, unit.width);
            region.y = scale_edge(region.y, request.output_resolution->height, unit.height);
            if (right == region.x || bottom == region.y) {
                plan_error(ExportPlanErrorCode::invalid_scope,
                           "export resolution collapses an atlas region: " +
                               region.texture_set_identifier);
            }
            region.width = right - region.x;
            region.height = bottom - region.y;
        }
        unit.width = request.output_resolution->width;
        unit.height = request.output_resolution->height;
    }
}

bool effectively_visible(const ExportLayerSource& layer, const LayerLookup& lookup) {
    const ExportLayerSource* current = &layer;
    while (current != nullptr) {
        if (!current->visible) {
            return false;
        }
        current =
            current->parent_identifier.empty() ? nullptr : lookup.at(current->parent_identifier);
    }
    return true;
}

std::vector<std::string> visible_layers(const ExportTextureSetSource& texture_set) {
    const LayerLookup lookup = validate_layers(texture_set);
    std::vector<std::string> result;
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (effectively_visible(layer, lookup)) {
            result.push_back(layer.identifier);
        }
    }
    return result;
}

std::set<std::string_view> validate_selected_layers(const ExportTextureSetSource& texture_set,
                                                    const std::vector<std::string>& selected) {
    const LayerLookup lookup = validate_layers(texture_set);
    std::set<std::string_view> result;
    for (const std::string& identifier : selected) {
        if (!lookup.contains(identifier)) {
            plan_error(ExportPlanErrorCode::invalid_layer_selection,
                       "selected export layer is missing from texture set " +
                           texture_set.identifier + ": " + identifier);
        }
        if (!result.insert(identifier).second) {
            plan_error(ExportPlanErrorCode::invalid_layer_selection,
                       "selected export layer is duplicated: " + identifier);
        }
    }
    return result;
}

bool has_selected_group_ancestor(const ExportLayerSource& layer, const LayerLookup& lookup,
                                 const std::set<std::string_view>& selected) {
    const ExportLayerSource* current = &layer;
    while (!current->parent_identifier.empty()) {
        current = lookup.at(current->parent_identifier);
        if (current->kind == ExportLayerKind::group && selected.contains(current->identifier)) {
            return true;
        }
    }
    return false;
}

std::vector<const ExportLayerSource*> selected_roots(const ExportTextureSetSource& texture_set,
                                                     const std::vector<std::string>& selected) {
    const LayerLookup lookup = validate_layers(texture_set);
    const std::set<std::string_view> selected_set = validate_selected_layers(texture_set, selected);
    std::vector<const ExportLayerSource*> result;
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (selected_set.contains(layer.identifier) &&
            !has_selected_group_ancestor(layer, lookup, selected_set)) {
            result.push_back(&layer);
        }
    }
    return result;
}

bool is_descendant_of(const ExportLayerSource& layer, const ExportLayerSource& ancestor,
                      const LayerLookup& lookup) {
    const ExportLayerSource* current = &layer;
    while (!current->parent_identifier.empty()) {
        current = lookup.at(current->parent_identifier);
        if (current == &ancestor) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> layer_closure(const ExportTextureSetSource& texture_set,
                                       const ExportLayerSource& root) {
    const LayerLookup lookup = validate_layers(texture_set);
    std::vector<std::string> result;
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (&layer == &root ||
            (root.kind == ExportLayerKind::group && is_descendant_of(layer, root, lookup))) {
            result.push_back(layer.identifier);
        }
    }
    return result;
}

std::vector<std::string> selected_layer_closure(const ExportTextureSetSource& texture_set,
                                                const std::vector<std::string>& selected) {
    std::set<std::string> included;
    for (const ExportLayerSource* root : selected_roots(texture_set, selected)) {
        for (std::string identifier : layer_closure(texture_set, *root)) {
            included.insert(std::move(identifier));
        }
    }
    std::vector<std::string> result;
    for (const ExportLayerSource& layer : texture_set.layers) {
        if (included.contains(layer.identifier)) {
            result.push_back(layer.identifier);
        }
    }
    return result;
}

struct LayerVariant {
    std::string display_name;
    std::map<std::string, std::vector<std::string>, std::less<>> layers;
};

std::vector<LayerVariant> visible_layer_variants(const ScopeUnit& unit) {
    LayerVariant variant{.display_name = "visible", .layers = {}};
    for (const ExportTextureSetSource* texture_set : unit.texture_sets) {
        variant.layers.emplace(texture_set->identifier, visible_layers(*texture_set));
    }
    return {std::move(variant)};
}

std::vector<LayerVariant> flattened_selected_variant(const ScopeUnit& unit,
                                                     const ExportPlanRequest& request) {
    LayerVariant variant{.display_name = "selected", .layers = {}};
    for (const ExportTextureSetSource* texture_set : unit.texture_sets) {
        const auto found = request.selected_layer_identifiers.find(texture_set->identifier);
        if (found != request.selected_layer_identifiers.end()) {
            variant.layers.emplace(texture_set->identifier,
                                   selected_layer_closure(*texture_set, found->second));
        }
    }
    if (variant.layers.empty()) {
        return {};
    }
    return {std::move(variant)};
}

std::vector<LayerVariant> separate_layer_variants(const ScopeUnit& unit,
                                                  const ExportPlanRequest& request) {
    std::vector<LayerVariant> result;
    for (const ExportTextureSetSource* texture_set : unit.texture_sets) {
        const auto found = request.selected_layer_identifiers.find(texture_set->identifier);
        if (found == request.selected_layer_identifiers.end()) {
            continue;
        }
        for (const ExportLayerSource* root : selected_roots(*texture_set, found->second)) {
            std::string display_name = root->display_name;
            if (unit.texture_sets.size() > 1) {
                display_name = texture_set->display_name + "_" + display_name;
            }
            result.push_back(
                {.display_name = std::move(display_name),
                 .layers = {{texture_set->identifier, layer_closure(*texture_set, *root)}}});
        }
    }
    return result;
}

std::vector<LayerVariant> layer_variants(const ScopeUnit& unit, const ExportPlanRequest& request) {
    switch (request.layer_scope) {
        case ExportLayerScope::flatten_visible:
            return visible_layer_variants(unit);
        case ExportLayerScope::flatten_selected:
            return flattened_selected_variant(unit, request);
        case ExportLayerScope::each_selected:
            return separate_layer_variants(unit, request);
    }
    plan_error(ExportPlanErrorCode::invalid_layer_selection, "export layer scope is invalid");
}

std::string filename_atom(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const unsigned char character : value) {
        const bool ascii_alphanumeric = (character >= 'a' && character <= 'z') ||
                                        (character >= 'A' && character <= 'Z') ||
                                        (character >= '0' && character <= '9');
        const bool portable =
            ascii_alphanumeric || character == '-' || character == '_' || character == '.';
        result.push_back(portable ? static_cast<char>(character) : '_');
    }
    if (result == "." || result == "..") {
        result.insert(result.begin(), '_');
    }
    return result;
}

std::string extension(ExportImageFormat format) {
    switch (format) {
        case ExportImageFormat::png:
            return "png";
        case ExportImageFormat::jpeg:
            return "jpg";
        case ExportImageFormat::tga:
            return "tga";
        case ExportImageFormat::tiff:
            return "tiff";
        case ExportImageFormat::openexr:
            return "exr";
    }
    plan_error(ExportPlanErrorCode::invalid_pattern, "export format has no file extension");
}

std::string resolution_token(const ScopeUnit& unit) {
    if (unit.width == unit.height) {
        return std::to_string(unit.width);
    }
    return std::to_string(unit.width) + "x" + std::to_string(unit.height);
}

std::string udim_token(const ScopeUnit& unit) {
    if (unit.udim_tile.has_value()) {
        return std::to_string(*unit.udim_tile);
    }
    return unit.atlas == nullptr ? "single" : "atlas";
}

using FilenameTokens = std::array<std::pair<std::string_view, std::string>, 8>;

std::string expand_filename_pattern(std::string_view pattern, const FilenameTokens& tokens) {
    if (pattern.empty()) {
        plan_error(ExportPlanErrorCode::invalid_pattern, "export filename pattern is empty");
    }
    std::string result;
    for (std::size_t cursor = 0; cursor < pattern.size();) {
        if (pattern[cursor] == '}') {
            plan_error(ExportPlanErrorCode::invalid_pattern,
                       "export filename pattern contains an unmatched closing brace");
        }
        if (pattern[cursor] != '{') {
            result.push_back(pattern[cursor++]);
            continue;
        }
        const std::size_t end = pattern.find('}', cursor + 1);
        if (end == std::string_view::npos) {
            plan_error(ExportPlanErrorCode::invalid_pattern,
                       "export filename pattern contains an unclosed token");
        }
        const std::string_view name = pattern.substr(cursor + 1, end - cursor - 1);
        const auto token =
            std::find_if(tokens.begin(), tokens.end(),
                         [name](const auto& candidate) { return candidate.first == name; });
        if (token == tokens.end()) {
            plan_error(ExportPlanErrorCode::invalid_pattern,
                       "unknown export filename token: " + std::string(name));
        }
        result += token->second;
        cursor = end + 1;
    }
    return result;
}

bool portable_path_component(std::string_view component) {
    if (component.empty() || component == "." || component == ".." || component.back() == '.' ||
        component.back() == ' ') {
        return false;
    }
    for (const unsigned char character : component) {
        if (character < 32 || character > 126 || character == '<' || character == '>' ||
            character == '"' || character == '|' || character == '?' || character == '*') {
            return false;
        }
    }
    return true;
}

void validate_relative_path(std::string_view path) {
    if (path.empty() || path.front() == '/' || path.find('\\') != std::string_view::npos ||
        path.find('\0') != std::string_view::npos || path.find(':') != std::string_view::npos) {
        plan_error(ExportPlanErrorCode::invalid_pattern,
                   "export filename pattern did not produce a portable relative path");
    }
    for (std::size_t begin = 0; begin <= path.size();) {
        const std::size_t end = path.find('/', begin);
        const std::string_view component = path.substr(begin, end - begin);
        if (!portable_path_component(component)) {
            plan_error(ExportPlanErrorCode::invalid_pattern,
                       "export filename pattern contains a non-portable path component");
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1;
    }
}

std::string collision_key(std::string_view path) {
    std::string result(path);
    std::transform(result.begin(), result.end(), result.begin(), [](char character) {
        return character >= 'A' && character <= 'Z' ? static_cast<char>(character + ('a' - 'A'))
                                                    : character;
    });
    return result;
}

FilenameTokens filename_tokens(const ExportSourceCatalogue& catalogue, const ScopeUnit& unit,
                               const LayerVariant& layer, const ExportTexturePreset& texture) {
    return {{{"project", filename_atom(catalogue.project_name)},
             {"texture_set", filename_atom(unit.display_name)},
             {"udim", udim_token(unit)},
             {"suffix", filename_atom(texture.suffix)},
             {"resolution", resolution_token(unit)},
             {"bit_depth", std::to_string(static_cast<unsigned>(texture.bit_depth))},
             {"layer", filename_atom(layer.display_name)},
             {"extension", extension(texture.format)}}};
}

void validate_request_layers(const std::vector<const ExportTextureSetSource*>& texture_sets,
                             const ExportPlanRequest& request) {
    std::set<std::string_view> selected_sets;
    for (const ExportTextureSetSource* texture_set : texture_sets) {
        selected_sets.insert(texture_set->identifier);
    }
    if (request.layer_scope == ExportLayerScope::flatten_visible) {
        if (!request.selected_layer_identifiers.empty()) {
            plan_error(ExportPlanErrorCode::invalid_layer_selection,
                       "visible-layer scope cannot also carry selected layers");
        }
        return;
    }
    if (request.selected_layer_identifiers.empty()) {
        plan_error(ExportPlanErrorCode::invalid_layer_selection,
                   "selected-layer scope requires at least one layer selection");
    }
    for (const auto& [texture_set, layers] : request.selected_layer_identifiers) {
        if (!selected_sets.contains(texture_set)) {
            plan_error(ExportPlanErrorCode::invalid_layer_selection,
                       "layer selection names an out-of-scope texture set: " + texture_set);
        }
        const auto source = std::find_if(
            texture_sets.begin(), texture_sets.end(),
            [&texture_set](const auto* candidate) { return candidate->identifier == texture_set; });
        if (layers.empty()) {
            plan_error(ExportPlanErrorCode::invalid_layer_selection,
                       "selected-layer list is empty for texture set: " + texture_set);
        }
        static_cast<void>(validate_selected_layers(**source, layers));
    }
}

}  // namespace

ExportPlanError::ExportPlanError(ExportPlanErrorCode code, std::string message)
    : std::runtime_error(std::move(message)), code_(code) {}

std::vector<PlannedTextureExport> plan_texture_export(const ExportSourceCatalogue& catalogue,
                                                      const ExportPreset& preset,
                                                      const ExportPlanRequest& request) {
    validate_export_preset(preset);
    const TextureSetLookup lookup = validate_catalogue(catalogue);
    const std::vector<const ExportTextureSetSource*> texture_sets =
        select_texture_sets(catalogue, request, lookup);
    validate_request_layers(texture_sets, request);
    std::vector<ScopeUnit> units = make_scope_units(catalogue, request, texture_sets, lookup);
    apply_output_resolution(units, request);

    std::map<std::string, std::string, std::less<>> paths;
    std::vector<PlannedTextureExport> result;
    for (const ScopeUnit& unit : units) {
        for (const LayerVariant& layer : layer_variants(unit, request)) {
            for (std::size_t texture_index = 0; texture_index < preset.textures.size();
                 ++texture_index) {
                const ExportTexturePreset& texture = preset.textures[texture_index];
                const std::string path = expand_filename_pattern(
                    request.filename_pattern, filename_tokens(catalogue, unit, layer, texture));
                validate_relative_path(path);
                const std::string key = collision_key(path);
                const auto [existing, inserted] = paths.emplace(key, path);
                if (!inserted) {
                    plan_error(ExportPlanErrorCode::path_collision,
                               "export filename collision: " + existing->second + " and " + path);
                }
                PlannedTextureExport output{
                    .relative_path = path,
                    .texture_set_identifiers = {},
                    .udim_tile = unit.udim_tile,
                    .atlas_identifier = std::nullopt,
                    .layer_identifiers = layer.layers,
                    .preset_texture_index = texture_index,
                    .width = unit.width,
                    .height = unit.height,
                    .format = texture.format,
                    .bit_depth = texture.bit_depth,
                    .color_space = texture.color_space,
                    .atlas_regions = unit.atlas_regions,
                };
                if (unit.atlas != nullptr) {
                    output.atlas_identifier = unit.atlas->identifier;
                }
                for (const ExportTextureSetSource* texture_set : unit.texture_sets) {
                    output.texture_set_identifiers.push_back(texture_set->identifier);
                }
                result.push_back(std::move(output));
            }
        }
    }
    if (result.empty()) {
        plan_error(ExportPlanErrorCode::invalid_scope,
                   "export scope and layer selection produced no outputs");
    }
    return result;
}

ExportSourceCatalogue export_source_catalogue(std::string project_name,
                                              const doc::TextureDocument& document) {
    if (project_name.empty()) {
        plan_error(ExportPlanErrorCode::invalid_catalogue,
                   "export catalogue requires a project name");
    }
    ExportSourceCatalogue result{
        .project_name = std::move(project_name), .texture_sets = {}, .atlases = {}};
    result.texture_sets.reserve(document.texture_set_count());
    for (const std::string& identifier : document.texture_set_ids()) {
        const doc::TextureSet& texture_set = document.texture_set(identifier);
        const doc::TextureSetDescriptor descriptor = texture_set.descriptor();
        ExportTextureSetSource source{
            .identifier = identifier,
            .display_name = descriptor.display_name,
            .width = descriptor.width,
            .height = descriptor.height,
            .occupied_udim_tiles = texture_set.occupied_udim_tiles(),
            .layers = {},
        };
        source.layers.reserve(texture_set.layer_stack().size());
        for (const doc::LayerEntry& entry : texture_set.layer_stack().entries()) {
            source.layers.push_back({
                .identifier = entry.identifier,
                .display_name = entry.display_name,
                .parent_identifier = entry.parent_identifier,
                .kind = entry.kind == doc::LayerEntryKind::group ? ExportLayerKind::group
                                                                 : ExportLayerKind::content,
                .visible = entry.enabled,
            });
        }
        result.texture_sets.push_back(std::move(source));
    }
    result.atlases.reserve(document.atlas_count());
    for (const std::string& identifier : document.atlas_ids()) {
        const doc::AtlasDescriptor& descriptor = document.atlas(identifier);
        ExportAtlasSource source{
            .identifier = descriptor.identifier,
            .display_name = descriptor.display_name,
            .width = descriptor.width,
            .height = descriptor.height,
            .texture_set_identifiers = {},
            .regions = {},
        };
        source.texture_set_identifiers.reserve(descriptor.regions.size());
        source.regions.reserve(descriptor.regions.size());
        for (const doc::AtlasRegion& region : descriptor.regions) {
            source.texture_set_identifiers.push_back(region.texture_set_identifier);
            source.regions.push_back({.texture_set_identifier = region.texture_set_identifier,
                                      .x = region.x,
                                      .y = region.y,
                                      .width = region.width,
                                      .height = region.height});
        }
        result.atlases.push_back(std::move(source));
    }
    return result;
}

}  // namespace ctex::io
