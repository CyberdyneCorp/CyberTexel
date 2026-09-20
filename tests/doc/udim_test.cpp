#include <array>
#include <cstddef>
#include <ctex/doc/document.hpp>
#include <iostream>
#include <string_view>

namespace {

using namespace ctex::doc;

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

TextureSet texture_set(bool udim = true) {
    TextureSet result({.display_name = "Material",
                       .partition_kind = PartitionSourceKind::material,
                       .partition_key = "material",
                       .uv_set = "uv0",
                       .width = 64,
                       .height = 64,
                       .default_bit_depth = 8,
                       .udim_tiling = udim});
    result.channels().enable("pbr.base_color");
    return result;
}

bool standard_addressing_is_checked() {
    return expect(udim_number({0, 0}) == 1001 && udim_number({9, 0}) == 1010 &&
                      udim_number({0, 1}) == 1011 && udim_coordinate(1024) == UdimCoordinate{3, 2},
                  "UDIM number and coordinate conversion is not canonical") &&
           expect_error<std::out_of_range>([] { static_cast<void>(udim_number({10, 0})); },
                                           "UDIM addressing accepted an eleventh U column") &&
           expect_error<std::out_of_range>([] { static_cast<void>(udim_coordinate(1000)); },
                                           "UDIM addressing accepted a number below 1001");
}

bool border_write_allocates_both_tiles() {
    TextureSet set = texture_set();
    const std::array left{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array right{std::byte{4}, std::byte{5}, std::byte{6}};
    const std::array writes{
        UdimPixelWrite{.u = 0.999, .v = 0.5, .pixel = left},
        UdimPixelWrite{.u = 1.001, .v = 0.5, .pixel = right},
    };
    const UdimWriteResult result = set.write_udim_pixels("pbr.base_color", writes);
    return expect(result.changed_tiles == std::vector<std::uint32_t>{1001, 1002} &&
                      result.allocated_tiles == result.changed_tiles &&
                      result.changed_pixel_count == 2 &&
                      set.occupied_udim_tiles() == result.changed_tiles,
                  "cross-border write did not allocate and report both UDIM tiles") &&
           expect(set.read_udim_pixel("pbr.base_color", 1001, 63, 32) ==
                          std::vector<std::byte>(left.begin(), left.end()) &&
                      set.read_udim_pixel("pbr.base_color", 1002, 0, 32) ==
                          std::vector<std::byte>(right.begin(), right.end()),
                  "cross-border pixels were not mapped to their local tile coordinates");
}

bool unused_tiles_have_no_pixel_storage() {
    TextureSet set = texture_set();
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array second{std::byte{4}, std::byte{5}, std::byte{6}};
    const std::array third{std::byte{7}, std::byte{8}, std::byte{9}};
    const std::array writes{
        UdimPixelWrite{.u = 0.25, .v = 0.25, .pixel = first},
        UdimPixelWrite{.u = 1.25, .v = 0.25, .pixel = second},
        UdimPixelWrite{.u = 0.25, .v = 1.25, .pixel = third},
    };
    static_cast<void>(set.write_udim_pixels("pbr.base_color", writes));
    const std::size_t one_tile_bytes =
        set.udim_channels(1001).pixels("pbr.base_color").tile_bytes();
    const std::array clear{std::byte{128}, std::byte{128}, std::byte{128}};
    const std::array no_change{
        UdimPixelWrite{.u = 2.25, .v = 0.25, .pixel = clear},
    };
    const UdimWriteResult unchanged = set.write_udim_pixels("pbr.base_color", no_change);
    return expect(set.occupied_udim_tiles() == std::vector<std::uint32_t>{1001, 1002, 1011},
                  "unused UDIM coordinates created occupied tiles") &&
           expect(set.memory_report().channel_pixel_bytes == one_tile_bytes * 3,
                  "UDIM storage was not proportional to three occupied tiles") &&
           expect(unchanged.changed_tiles.empty() && unchanged.allocated_tiles.empty(),
                  "writing a default pixel allocated an unused UDIM tile");
}

bool mesh_occupancy_declares_only_used_tiles() {
    TextureSet set = texture_set();
    const std::array occupied{1011U, 1001U, 1002U};
    const std::vector<std::uint32_t> allocated = set.ensure_udim_tiles(occupied);
    const bool duplicate = expect_error<std::invalid_argument>(
        [&] {
            const std::array repeated{1001U, 1001U};
            static_cast<void>(set.ensure_udim_tiles(repeated));
        },
        "UDIM occupancy accepted a duplicate tile declaration");
    return duplicate &&
           expect(allocated == std::vector<std::uint32_t>{1001, 1002, 1011} &&
                      set.occupied_udim_tiles() == allocated &&
                      set.memory_report().channel_pixel_bytes == 0,
                  "mesh occupancy allocated anything other than three sparse logical tiles");
}

bool clearing_preserves_logical_tiles_and_releases_pixels() {
    TextureSet set = texture_set();
    const std::array pixel{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array writes{
        UdimPixelWrite{.u = 0.25, .v = 0.25, .pixel = pixel},
        UdimPixelWrite{.u = 1.25, .v = 0.25, .pixel = pixel},
    };
    static_cast<void>(set.write_udim_pixels("pbr.base_color", writes));
    set.clear_channels();
    return expect(set.occupied_udim_tiles() == std::vector<std::uint32_t>{1001, 1002} &&
                      set.memory_report().channel_pixel_bytes == 0,
                  "clearing a UDIM set lost logical occupancy or retained physical pixels");
}

bool later_channel_configuration_reaches_existing_tiles() {
    TextureSet set = texture_set();
    const std::array base{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array initial{UdimPixelWrite{.u = 0.25, .v = 0.25, .pixel = base}};
    static_cast<void>(set.write_udim_pixels("pbr.base_color", initial));
    set.channels().register_descriptor({
        .semantic_id = "openpbr.coat_weight",
        .component_count = 1,
        .scalar_representation = ScalarRepresentation::unsigned_normalized,
        .preferred_bit_depth = 8,
        .default_value = {0.0},
        .classification = ChannelClassification::data,
        .blending_policy = BlendingPolicy::scalar,
        .export_mapping = "coatWeight",
    });
    set.channels().enable("openpbr.coat_weight");
    const std::array coat{std::byte{64}};
    const std::array update{UdimPixelWrite{.u = 0.5, .v = 0.5, .pixel = coat}};
    static_cast<void>(set.write_udim_pixels("openpbr.coat_weight", update));
    return expect(set.udim_channels(1001).is_enabled("openpbr.coat_weight") &&
                      set.read_udim_pixel("openpbr.coat_weight", 1001, 32, 32) ==
                          std::vector<std::byte>(coat.begin(), coat.end()),
                  "later channel configuration did not synchronize to an occupied UDIM tile");
}

bool non_udim_sets_refuse_udim_writes() {
    TextureSet set = texture_set(false);
    const std::array pixel{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array writes{UdimPixelWrite{.u = 0.5, .v = 0.5, .pixel = pixel}};
    return expect_error<std::logic_error>(
        [&] { static_cast<void>(set.write_udim_pixels("pbr.base_color", writes)); },
        "non-UDIM texture set accepted a UDIM write");
}

bool invalid_batch_is_refused_before_any_write() {
    TextureSet set = texture_set();
    const std::array pixel{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array writes{
        UdimPixelWrite{.u = 0.5, .v = 0.5, .pixel = pixel},
        UdimPixelWrite{.u = 10.0, .v = 0.5, .pixel = pixel},
    };
    const bool refused = expect_error<std::out_of_range>(
        [&] { static_cast<void>(set.write_udim_pixels("pbr.base_color", writes)); },
        "UDIM batch accepted a coordinate beyond the standard U range");
    return refused &&
           expect(set.occupied_udim_tiles().empty() && set.memory_report().channel_pixel_bytes == 0,
                  "invalid UDIM batch partially allocated or wrote an earlier tile");
}

}  // namespace

int main() {
    return standard_addressing_is_checked() && border_write_allocates_both_tiles() &&
                   unused_tiles_have_no_pixel_storage() &&
                   mesh_occupancy_declares_only_used_tiles() &&
                   clearing_preserves_logical_tiles_and_releases_pixels() &&
                   later_channel_configuration_reaches_existing_tiles() &&
                   non_udim_sets_refuse_udim_writes() && invalid_batch_is_refused_before_any_write()
               ? 0
               : 1;
}
