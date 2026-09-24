#include <algorithm>
#include <ctex/doc/document.hpp>
#include <limits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ctex::doc {
namespace {

[[noreturn]] void fail(TextureSetTransactionErrorCode code, std::string message) {
    throw TextureSetTransactionError(code, std::move(message));
}

}  // namespace

struct TextureSetTransaction::State {
    State(TextureSet& value, std::string step_identifier,
          std::span<const TileHistoryTarget> declared_targets)
        : owner(&value),
          staged(value.descriptor(), value.memory_resource_),
          targets(declared_targets.begin(), declared_targets.end()),
          capture(value.tile_history_.begin_transaction(value.channels_, value.layer_stack_,
                                                        std::move(step_identifier), targets)) {
        staged.channels_ = value.channels_;
        staged.layer_stack_ = value.layer_stack_;
    }

    TextureSet* owner;
    TextureSet staged;
    std::vector<TileHistoryTarget> targets;
    TileHistoryCapture capture;
};

TextureSetTransactionError::TextureSetTransactionError(TextureSetTransactionErrorCode code,
                                                       std::string message)
    : std::invalid_argument(std::move(message)), code_(code) {}

TextureSetTransaction::TextureSetTransaction(TextureSet& owner, std::string step_identifier,
                                             std::span<const TileHistoryTarget> targets)
    : state_(std::make_unique<State>(owner, std::move(step_identifier), targets)) {}

TextureSetTransaction::~TextureSetTransaction() = default;
TextureSetTransaction::TextureSetTransaction(TextureSetTransaction&&) noexcept = default;
TextureSetTransaction& TextureSetTransaction::operator=(TextureSetTransaction&&) noexcept = default;

TextureSetTransaction::State& TextureSetTransaction::require_active() {
    if (!state_) {
        fail(TextureSetTransactionErrorCode::inactive,
             "texture-set transaction is no longer active");
    }
    return *state_;
}

const TextureSetTransaction::State& TextureSetTransaction::require_active() const {
    if (!state_) {
        fail(TextureSetTransactionErrorCode::inactive,
             "texture-set transaction is no longer active");
    }
    return *state_;
}

bool TextureSetTransaction::active() const noexcept { return state_ != nullptr; }

const TextureChannels& TextureSetTransaction::channels() const {
    return require_active().staged.channels();
}

LayerStack& TextureSetTransaction::layer_stack() { return require_active().staged.layer_stack(); }

const LayerStack& TextureSetTransaction::layer_stack() const {
    return require_active().staged.layer_stack();
}

void TextureSetTransaction::write_pixel(std::string_view semantic_id, std::uint32_t x,
                                        std::uint32_t y, std::span<const std::byte> pixel) {
    State& state = require_active();
    image::TiledImage& image = state.staged.channels().pixels(semantic_id);
    if (x >= image.width() || y >= image.height()) {
        throw std::out_of_range("transaction pixel coordinate is outside the image");
    }
    const TileHistoryTarget target{
        .semantic_id = std::string(semantic_id),
        .coordinate = {.x = x / image.tile_size(), .y = y / image.tile_size()},
    };
    if (std::ranges::find(state.targets, target) == state.targets.end()) {
        fail(TextureSetTransactionErrorCode::undeclared_target,
             "transaction pixel write is outside its declared channel tile set");
    }
    image.write_pixel(x, y, pixel);
}

namespace {

void validate_region(const image::TiledImage& image, Rect region,
                     std::span<const std::byte> pixels, std::size_t row_pitch) {
    if (region.width == 0 || region.height == 0 || region.x >= image.width() ||
        region.y >= image.height() || region.width > image.width() - region.x ||
        region.height > image.height() - region.y) {
        throw std::out_of_range("transaction region is outside the image");
    }
    const std::size_t pixel_bytes = image.pixel_bytes();
    if (region.width > std::numeric_limits<std::size_t>::max() / pixel_bytes) {
        throw std::invalid_argument("transaction region row size overflows");
    }
    const std::size_t row_bytes = static_cast<std::size_t>(region.width) * pixel_bytes;
    if (row_pitch < row_bytes ||
        (region.height - 1 > (std::numeric_limits<std::size_t>::max() - row_bytes) / row_pitch) ||
        pixels.size() != (static_cast<std::size_t>(region.height) - 1) * row_pitch + row_bytes) {
        throw std::invalid_argument("transaction region byte count or row pitch is invalid");
    }
}

void validate_region_targets(std::span<const TileHistoryTarget> targets,
                             std::string_view semantic_id, image::TileCoordinate first,
                             image::TileCoordinate last) {
    std::unordered_set<std::uint64_t> declared;
    declared.reserve(targets.size());
    for (const TileHistoryTarget& target : targets) {
        if (target.semantic_id == semantic_id) {
            declared.insert((static_cast<std::uint64_t>(target.coordinate.y) << 32) |
                            target.coordinate.x);
        }
    }
    for (std::uint32_t ty = first.y; ty <= last.y; ++ty) {
        for (std::uint32_t tx = first.x; tx <= last.x; ++tx) {
            if (!declared.contains((static_cast<std::uint64_t>(ty) << 32) | tx)) {
                fail(TextureSetTransactionErrorCode::undeclared_target,
                     "transaction region write is outside its declared channel tile set");
            }
        }
    }
}

void write_region_tile(image::TiledImage& image, image::TileCoordinate tile, Rect region,
                       std::span<const std::byte> pixels, std::size_t row_pitch) {
    const image::TileExtent extent = image.tile_extent(tile);
    const std::uint32_t tile_x = tile.x * image.tile_size();
    const std::uint32_t tile_y = tile.y * image.tile_size();
    const std::uint32_t left = std::max(tile_x, region.x);
    const std::uint32_t top = std::max(tile_y, region.y);
    const std::uint32_t right = std::min(tile_x + extent.width, region.x + region.width);
    const std::uint32_t bottom = std::min(tile_y + extent.height, region.y + region.height);
    const std::size_t pixel_bytes = image.pixel_bytes();
    if (left == tile_x && top == tile_y && right == tile_x + extent.width &&
        bottom == tile_y + extent.height) {
        const std::size_t tile_row_bytes = static_cast<std::size_t>(extent.width) * pixel_bytes;
        std::vector<std::byte> tile_pixels(tile_row_bytes * extent.height);
        for (std::uint32_t y = 0; y < extent.height; ++y) {
            const std::size_t source = static_cast<std::size_t>(tile_y + y - region.y) * row_pitch +
                                       static_cast<std::size_t>(tile_x - region.x) * pixel_bytes;
            std::ranges::copy(pixels.subspan(source, tile_row_bytes),
                              tile_pixels.begin() + static_cast<std::size_t>(y) * tile_row_bytes);
        }
        image.write_tile(tile, tile_pixels);
        return;
    }
    for (std::uint32_t y = top; y < bottom; ++y) {
        for (std::uint32_t x = left; x < right; ++x) {
            const std::size_t source = static_cast<std::size_t>(y - region.y) * row_pitch +
                                       static_cast<std::size_t>(x - region.x) * pixel_bytes;
            image.write_pixel(x, y, pixels.subspan(source, pixel_bytes));
        }
    }
}

}  // namespace

void TextureSetTransaction::write_region(std::string_view semantic_id, Rect region,
                                         std::span<const std::byte> pixels,
                                         std::size_t row_pitch) {
    State& state = require_active();
    image::TiledImage& image = state.staged.channels().pixels(semantic_id);
    validate_region(image, region, pixels, row_pitch);
    const std::uint32_t tile_size = image.tile_size();
    const image::TileCoordinate first{region.x / tile_size, region.y / tile_size};
    const image::TileCoordinate last{(region.x + region.width - 1) / tile_size,
                                     (region.y + region.height - 1) / tile_size};
    validate_region_targets(state.targets, semantic_id, first, last);
    for (std::uint32_t ty = first.y; ty <= last.y; ++ty) {
        for (std::uint32_t tx = first.x; tx <= last.x; ++tx) {
            write_region_tile(image, {tx, ty}, region, pixels, row_pitch);
        }
    }
}

LayerOperationResult TextureSetTransaction::apply_layer_operation(LayerOperationRequest request) {
    return require_active().staged.apply_layer_operation(std::move(request));
}

TileHistoryCommitResult TextureSetTransaction::commit() {
    State& state = require_active();
    try {
        TileHistoryCommitResult result = state.owner->tile_history_.commit_transaction(
            state.owner->channels_, state.owner->layer_stack_, state.staged.channels_,
            std::move(state.staged.layer_stack_), std::move(state.capture));
        state_.reset();
        return result;
    } catch (...) {
        state_.reset();
        throw;
    }
}

void TextureSetTransaction::cancel() noexcept { state_.reset(); }

TextureSetTransaction TextureSet::begin_transaction(std::string step_identifier,
                                                    std::span<const TileHistoryTarget> targets) {
    return TextureSetTransaction(*this, std::move(step_identifier), targets);
}

}  // namespace ctex::doc
