#ifndef CTEX_PAINT_PREVIEW_HPP
#define CTEX_PAINT_PREVIEW_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/doc/channels.hpp>
#include <ctex/paint/seam_dilation.hpp>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace ctex::paint {

inline constexpr double paint_preview_commit_tolerance = 0.0;

enum class PaintPreviewState : std::uint8_t { provisional, final, committed, cancelled };

struct PaintPreviewCommitReport {
    image::RevisionCursor baseline;
    image::RevisionCursor preview;
    image::RevisionCursor committed;
    std::vector<image::TileCoordinate> changed_tiles;
    double maximum_component_error{};
};

class PaintPreviewSession {
public:
    PaintPreviewSession(const doc::TextureChannels& document_channels,
                        std::string_view channel_semantic);
    ~PaintPreviewSession();
    PaintPreviewSession(PaintPreviewSession&&) noexcept;
    PaintPreviewSession& operator=(PaintPreviewSession&&) noexcept;
    PaintPreviewSession(const PaintPreviewSession&) = delete;
    PaintPreviewSession& operator=(const PaintPreviewSession&) = delete;

    void write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel);
    [[nodiscard]] const image::TiledImage& provisional_preview() const;
    [[nodiscard]] const image::TiledImage& finalize(
        std::span<const std::uint8_t> coverage,
        std::uint32_t dilation_radius = default_seam_dilation_radius);
    [[nodiscard]] PaintPreviewCommitReport commit(doc::TextureChannels& document_channels);
    void cancel();

    [[nodiscard]] const image::TiledImage& preview_pixels() const noexcept;
    [[nodiscard]] std::string_view channel_semantic() const noexcept;
    [[nodiscard]] PaintPreviewState state() const noexcept;
    [[nodiscard]] std::size_t dilated_texel_count() const noexcept;
    [[nodiscard]] std::size_t zero_gradient_texel_count() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace ctex::paint

#endif
