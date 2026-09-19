#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <ctex/image/tiled_image.hpp>
#include <exception>
#include <iostream>
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
    image.clear_dirty();
    passed &= expect(image.dirty_tiles().empty(), "dirty state did not clear");
    return passed;
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
    return passed;
}

}  // namespace

int main() {
    return test_formats_preserve_bytes() && test_sparse_clear_and_dirty_tracking() &&
                   test_validation()
               ? 0
               : 1;
}
