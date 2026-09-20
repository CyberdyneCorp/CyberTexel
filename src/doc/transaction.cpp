#include <algorithm>
#include <ctex/doc/document.hpp>
#include <utility>

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
