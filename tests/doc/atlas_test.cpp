#include <ctex/doc/document.hpp>
#include <ctex/io/export_plan.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex::doc;
using namespace ctex::io;

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Exception, typename Callable>
bool expect_error(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const Exception&) {
        return true;
    } catch (...) {
    }
    return expect(false, message);
}

TextureSetDescriptor texture_set_descriptor(std::string name, std::string partition) {
    return {.display_name = std::move(name),
            .partition_kind = PartitionSourceKind::material,
            .partition_key = std::move(partition),
            .uv_set = "uv0",
            .width = 2,
            .height = 2,
            .default_bit_depth = 8};
}

struct TwoPropDocument {
    TextureDocument document;
    std::string crate;
    std::string barrel;

    TwoPropDocument()
        : crate(document.create_texture_set(texture_set_descriptor("Crate", "crate")).id()),
          barrel(document.create_texture_set(texture_set_descriptor("Barrel", "barrel")).id()) {}

    AtlasDescriptor descriptor() const {
        return {.identifier = "props",
                .display_name = "Props",
                .width = 4,
                .height = 2,
                .regions = {
                    {.texture_set_identifier = crate, .x = 0, .y = 0, .width = 2, .height = 2},
                    {.texture_set_identifier = barrel, .x = 2, .y = 0, .width = 2, .height = 2}}};
    }
};

bool two_props_plan_one_region_aware_output() {
    TwoPropDocument fixture;
    const AtlasDescriptor expected = fixture.descriptor();
    const AtlasDescriptor& atlas = fixture.document.create_atlas(expected);
    const ExportSourceCatalogue sources = export_source_catalogue("Prop Project", fixture.document);
    ExportPlanRequest request;
    request.spatial_scope = ExportSpatialScope::atlas;
    const auto plan = plan_texture_export(sources, built_in_export_preset("base-color"), request);
    return expect(atlas == expected && fixture.document.atlas_count() == 1 &&
                      fixture.document.atlas_ids() == std::vector<std::string>{"props"},
                  "document did not retain the declared atlas and its regions") &&
           expect(plan.size() == 1 && plan.front().atlas_identifier == "props" &&
                      plan.front().texture_set_identifiers ==
                          std::vector<std::string>{fixture.crate, fixture.barrel} &&
                      plan.front().atlas_regions.size() == 2 && plan.front().width == 4 &&
                      plan.front().height == 2,
                  "two props did not produce one region-aware atlas output");
}

bool selection_and_resolution_transform_regions() {
    TwoPropDocument fixture;
    static_cast<void>(fixture.document.create_atlas(fixture.descriptor()));
    const ExportSourceCatalogue sources = export_source_catalogue("Prop Project", fixture.document);
    ExportPlanRequest request;
    request.texture_set_selection = ExportTextureSetSelection::selected;
    request.selected_texture_set_identifiers = {fixture.barrel};
    request.spatial_scope = ExportSpatialScope::atlas;
    request.output_resolution = ExportResolution{2, 1};
    const auto plan = plan_texture_export(sources, built_in_export_preset("base-color"), request);
    const ExportAtlasRegion expected{
        .texture_set_identifier = fixture.barrel, .x = 1, .y = 0, .width = 1, .height = 1};
    return expect(
        plan.size() == 1 &&
            plan.front().texture_set_identifiers == std::vector<std::string>{fixture.barrel} &&
            plan.front().atlas_regions == std::vector<ExportAtlasRegion>{expected} &&
            plan.front().width == 2 && plan.front().height == 1,
        "atlas selection or output-resolution scaling changed the declared region");
}

bool invalid_regions_are_atomic() {
    TwoPropDocument fixture;
    AtlasDescriptor overlap = fixture.descriptor();
    overlap.regions[1].x = 1;
    const bool overlap_refused = expect_error<std::invalid_argument>(
        [&] { static_cast<void>(fixture.document.create_atlas(overlap)); },
        "overlapping atlas regions were accepted");
    AtlasDescriptor outside = fixture.descriptor();
    outside.regions[1].x = 3;
    const bool outside_refused = expect_error<std::out_of_range>(
        [&] { static_cast<void>(fixture.document.create_atlas(outside)); },
        "out-of-bounds atlas region was accepted");
    AtlasDescriptor missing = fixture.descriptor();
    missing.regions[1].texture_set_identifier = "missing";
    const bool missing_refused = expect_error<std::invalid_argument>(
        [&] { static_cast<void>(fixture.document.create_atlas(missing)); },
        "atlas region accepted a missing texture set");
    return overlap_refused && outside_refused && missing_refused &&
           expect(fixture.document.atlas_count() == 0,
                  "invalid atlas declaration partially changed the document");
}

bool texture_set_belongs_to_only_one_atlas() {
    TwoPropDocument fixture;
    static_cast<void>(fixture.document.create_atlas(fixture.descriptor()));
    AtlasDescriptor duplicate{
        .identifier = "duplicate",
        .display_name = "Duplicate",
        .width = 2,
        .height = 2,
        .regions = {
            {.texture_set_identifier = fixture.crate, .x = 0, .y = 0, .width = 2, .height = 2}}};
    return expect_error<std::invalid_argument>(
               [&] { static_cast<void>(fixture.document.create_atlas(duplicate)); },
               "texture set was assigned to two atlases") &&
           expect(fixture.document.atlas_count() == 1,
                  "duplicate atlas assignment changed the document");
}

bool collapsed_export_region_is_refused() {
    TwoPropDocument fixture;
    static_cast<void>(fixture.document.create_atlas(fixture.descriptor()));
    ExportPlanRequest request;
    request.spatial_scope = ExportSpatialScope::atlas;
    request.output_resolution = ExportResolution{1, 1};
    try {
        static_cast<void>(
            plan_texture_export(export_source_catalogue("Prop Project", fixture.document),
                                built_in_export_preset("base-color"), request));
    } catch (const ExportPlanError& error) {
        return expect(error.code() == ExportPlanErrorCode::invalid_scope,
                      "collapsed atlas region returned the wrong error code");
    }
    return expect(false, "output resolution silently collapsed an atlas region");
}

}  // namespace

int main() {
    return two_props_plan_one_region_aware_output() &&
                   selection_and_resolution_transform_regions() && invalid_regions_are_atomic() &&
                   texture_set_belongs_to_only_one_atlas() && collapsed_export_region_is_refused()
               ? 0
               : 1;
}
