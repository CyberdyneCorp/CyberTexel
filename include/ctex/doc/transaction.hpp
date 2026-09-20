#ifndef CTEX_DOC_TRANSACTION_HPP
#define CTEX_DOC_TRANSACTION_HPP

#include <cstdint>
#include <ctex/doc/layer_operations.hpp>
#include <ctex/doc/tile_history.hpp>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ctex::doc {

class TextureSet;

enum class TextureSetTransactionErrorCode : std::uint8_t {
    inactive,
    undeclared_target,
};

class TextureSetTransactionError final : public std::invalid_argument {
public:
    TextureSetTransactionError(TextureSetTransactionErrorCode code, std::string message);
    [[nodiscard]] TextureSetTransactionErrorCode code() const noexcept { return code_; }

private:
    TextureSetTransactionErrorCode code_;
};

class TextureSetTransaction {
public:
    ~TextureSetTransaction();
    TextureSetTransaction(const TextureSetTransaction&) = delete;
    TextureSetTransaction& operator=(const TextureSetTransaction&) = delete;
    TextureSetTransaction(TextureSetTransaction&&) noexcept;
    TextureSetTransaction& operator=(TextureSetTransaction&&) noexcept;

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] const TextureChannels& channels() const;
    [[nodiscard]] const LayerStack& layer_stack() const;
    void write_pixel(std::string_view semantic_id, std::uint32_t x, std::uint32_t y,
                     std::span<const std::byte> pixel);
    [[nodiscard]] LayerOperationResult apply_layer_operation(LayerOperationRequest request);
    [[nodiscard]] TileHistoryCommitResult commit();
    void cancel() noexcept;

private:
    friend class TextureSet;
    struct State;
    TextureSetTransaction(TextureSet& owner, std::string step_identifier,
                          std::span<const TileHistoryTarget> targets);
    [[nodiscard]] State& require_active();
    [[nodiscard]] const State& require_active() const;

    std::unique_ptr<State> state_;
};

}  // namespace ctex::doc

#endif
