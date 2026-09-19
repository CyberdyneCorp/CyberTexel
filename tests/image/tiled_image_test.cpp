#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <exception>
#include <iostream>
#include <memory_resource>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using ctex::image::ChannelType;
using ctex::image::PixelFormat;
using ctex::image::TileCoordinate;
using ctex::image::TiledImage;

class CountingResource final : public std::pmr::memory_resource {
public:
    std::size_t allocations{};
    std::size_t deallocations{};

private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        return std::pmr::new_delete_resource()->allocate(bytes, alignment);
    }

    void do_deallocate(void* allocation, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        std::pmr::new_delete_resource()->deallocate(allocation, bytes, alignment);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

template <typename Exception, typename Callable>
bool expect_throws(Callable&& callable, std::string_view message) {
    try {
        callable();
    } catch (const Exception&) {
        return true;
    } catch (...) {
    }
    std::cerr << message << '\n';
    return false;
}

bool test_formats_preserve_bytes() {
    const std::array<std::byte, 4> float_value = std::bit_cast<std::array<std::byte, 4>>(2.5F);
    const std::array cases{
        std::pair{PixelFormat{ChannelType::uint8_unorm, 1}, std::vector<std::byte>{std::byte{37}}},
        std::pair{PixelFormat{ChannelType::uint16_unorm, 1},
                  std::vector<std::byte>{std::byte{0x34}, std::byte{0x12}}},
        std::pair{PixelFormat{ChannelType::float32, 1},
                  std::vector<std::byte>{float_value.begin(), float_value.end()}},
    };

    bool passed = true;
    for (const auto& [format, value] : cases) {
        TiledImage image(2, 2, format);
        image.write_pixel(1, 1, value);
        const auto actual = image.read_pixel(1, 1);
        passed &= expect(std::equal(actual.begin(), actual.end(), value.begin(), value.end()),
                         "pixel bytes changed during tiled storage");
    }
    return passed;
}

bool test_sparse_clear_and_dirty_tracking() {
    const std::array clear{std::byte{3}, std::byte{5}, std::byte{7}, std::byte{11}};
    TiledImage image(65, 70, PixelFormat{ChannelType::uint8_unorm, 4}, 64, clear);
    bool passed = true;
    passed &= expect(image.tile_columns() == 2 && image.tile_rows() == 2, "wrong tile grid");
    passed &= expect(image.resident_pixel_bytes() == 0, "clear tiles allocated pixel storage");
    const auto clear_actual = image.read_pixel(64, 69);
    passed &=
        expect(std::equal(clear_actual.begin(), clear_actual.end(), clear.begin(), clear.end()),
               "wrong clear pixel");

    const std::array value{std::byte{13}, std::byte{17}, std::byte{19}, std::byte{23}};
    image.write_pixel(64, 69, value);
    image.write_pixel(64, 69, value);
    const std::array expected_dirty{TileCoordinate{1, 1}};
    passed &= expect(image.is_tile_allocated({1, 1}), "written tile was not allocated");
    passed &= expect(image.dirty_tiles() ==
                         std::vector<TileCoordinate>(expected_dirty.begin(), expected_dirty.end()),
                     "dirty tile was duplicated or misidentified");
    passed &= expect(image.tile_extent({1, 1}).width == 1, "edge tile width is wrong");
    passed &= expect(image.tile_extent({1, 1}).height == 6, "edge tile height is wrong");
    const auto changes = image.changed_tiles_after(0);
    passed &= expect(changes.coordinates == std::vector<TileCoordinate>{{1, 1}} &&
                         changes.indexed_tiles_visited == 1,
                     "tile change index did not return only the changed tile");
    const auto no_changes = image.changed_tiles_after(image.revision());
    passed &= expect(no_changes.coordinates.empty() && no_changes.indexed_tiles_visited == 0,
                     "current-revision change query visited the image grid");
    image.clear_dirty();
    passed &= expect(image.dirty_tiles().empty(), "dirty state did not clear");
    return passed;
}

bool test_pinned_tile_storage_is_copy_on_write() {
    TiledImage image(2, 1, PixelFormat{ChannelType::uint8_unorm, 3}, 2);
    const std::array first{std::byte{1}, std::byte{2}, std::byte{3}};
    const std::array second{std::byte{5}, std::byte{8}, std::byte{13}};
    image.write_pixel(0, 0, first);
    const auto pinned = image.pin_tile_storage({0, 0});
    image.write_pixel(0, 0, second);
    const auto current = image.pin_tile_storage({0, 0});

    return expect(pinned && current && pinned.get() != current.get(),
                  "writing a pinned tile did not create a new allocation") &&
           expect(std::equal(first.begin(), first.end(), pinned->begin()),
                  "copy-on-write changed the pinned tile version") &&
           expect(std::equal(second.begin(), second.end(), image.read_pixel(0, 0).begin()),
                  "copy-on-write did not publish the new tile version") &&
           expect(image.resident_pixel_bytes() == image.tile_bytes(),
                  "old pinned storage was double-counted as current image residency");
}

bool test_persistent_storage_uses_supplied_resource() {
    CountingResource resource;
    {
        TiledImage image(65, 65, PixelFormat{ChannelType::uint8_unorm, 1}, 64, {}, &resource);
        const std::size_t metadata_allocations = resource.allocations;
        image.write_pixel(0, 0, std::array{std::byte{1}});
        const std::size_t tile_allocations = resource.allocations;
        TiledImage copied = image;
        TiledImage moved(1, 1, PixelFormat{ChannelType::uint8_unorm, 1});
        moved = std::move(copied);
        if (!expect(metadata_allocations > 0, "image metadata bypassed its memory resource") ||
            !expect(tile_allocations > metadata_allocations,
                    "tile storage bypassed its memory resource") ||
            !expect(resource.allocations > tile_allocations,
                    "image copy metadata bypassed its memory resource")) {
            return false;
        }
    }
    return expect(resource.allocations == resource.deallocations,
                  "image storage was not returned to its memory resource");
}

bool test_validation() {
    bool passed = true;
    passed &= expect_throws<std::invalid_argument>(
        [] { TiledImage image(0, 1, PixelFormat{ChannelType::uint8_unorm, 1}); },
        "zero width was accepted");
    passed &= expect_throws<std::invalid_argument>(
        [] { TiledImage image(1, 1, PixelFormat{ChannelType::uint8_unorm, 5}); },
        "five-channel format was accepted");
    passed &= expect_throws<std::out_of_range>(
        [] {
            TiledImage image(1, 1, PixelFormat{ChannelType::uint8_unorm, 1});
            static_cast<void>(image.read_pixel(1, 0));
        },
        "out-of-range pixel was accepted");
    passed &= expect_throws<std::invalid_argument>(
        [] { TiledImage image(1, 1, PixelFormat{ChannelType::uint8_unorm, 1}, 1, {}, nullptr); },
        "null memory resource was accepted");
    return passed;
}

}  // namespace

int main() {
    return test_formats_preserve_bytes() && test_sparse_clear_and_dirty_tracking() &&
                   test_pinned_tile_storage_is_copy_on_write() &&
                   test_persistent_storage_uses_supplied_resource() && test_validation()
               ? 0
               : 1;
}
