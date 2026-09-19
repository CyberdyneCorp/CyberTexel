#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <ctex/paint/preview.hpp>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace ctex::paint {
namespace {

double decode_component(std::span<const std::byte> pixel, image::ChannelType type,
                        std::size_t component) {
    if (type == image::ChannelType::uint8_unorm) {
        std::uint8_t encoded{};
        std::memcpy(&encoded, pixel.data() + component, sizeof(encoded));
        return static_cast<double>(encoded) / std::numeric_limits<std::uint8_t>::max();
    }
    if (type == image::ChannelType::uint16_unorm) {
        std::uint16_t encoded{};
        std::memcpy(&encoded, pixel.data() + component * sizeof(encoded), sizeof(encoded));
        return static_cast<double>(encoded) / std::numeric_limits<std::uint16_t>::max();
    }
    float encoded{};
    std::memcpy(&encoded, pixel.data() + component * sizeof(encoded), sizeof(encoded));
    return encoded;
}

SeamDilationRaster decode_raster(const image::TiledImage& pixels) {
    const image::PixelFormat format = pixels.format();
    SeamDilationRaster raster{.width = pixels.width(),
                              .height = pixels.height(),
                              .component_count = format.channel_count,
                              .pixels = {}};
    const std::size_t texel_count = static_cast<std::size_t>(raster.width) * raster.height;
    if (texel_count > std::numeric_limits<std::size_t>::max() / raster.component_count) {
        throw std::overflow_error("paint preview component count overflows address space");
    }
    raster.pixels.reserve(texel_count * raster.component_count);
    for (std::uint32_t y = 0; y < raster.height; ++y) {
        for (std::uint32_t x = 0; x < raster.width; ++x) {
            const std::span<const std::byte> pixel = pixels.read_pixel(x, y);
            for (std::size_t component = 0; component < raster.component_count; ++component) {
                raster.pixels.push_back(decode_component(pixel, format.channel_type, component));
            }
        }
    }
    return raster;
}

void encode_component(double value, image::ChannelType type, std::size_t component,
                      std::span<std::byte> pixel) {
    if (type == image::ChannelType::uint8_unorm) {
        const auto encoded = static_cast<std::uint8_t>(
            std::round(std::clamp(value, 0.0, 1.0) * std::numeric_limits<std::uint8_t>::max()));
        std::memcpy(pixel.data() + component, &encoded, sizeof(encoded));
        return;
    }
    if (type == image::ChannelType::uint16_unorm) {
        const auto encoded = static_cast<std::uint16_t>(
            std::round(std::clamp(value, 0.0, 1.0) * std::numeric_limits<std::uint16_t>::max()));
        std::memcpy(pixel.data() + component * sizeof(encoded), &encoded, sizeof(encoded));
        return;
    }
    const float encoded = static_cast<float>(value);
    if (!std::isfinite(encoded)) {
        throw std::overflow_error("paint preview value exceeds float storage");
    }
    std::memcpy(pixel.data() + component * sizeof(encoded), &encoded, sizeof(encoded));
}

void encode_raster(const SeamDilationRaster& raster, image::TiledImage& pixels) {
    const image::PixelFormat format = pixels.format();
    std::vector<std::byte> pixel(format.bytes_per_pixel());
    for (std::uint32_t y = 0; y < raster.height; ++y) {
        for (std::uint32_t x = 0; x < raster.width; ++x) {
            const std::size_t texel = static_cast<std::size_t>(y) * raster.width + x;
            for (std::size_t component = 0; component < raster.component_count; ++component) {
                encode_component(raster.pixels[texel * raster.component_count + component],
                                 format.channel_type, component, pixel);
            }
            pixels.write_pixel(x, y, pixel);
        }
    }
}

std::vector<image::TileCoordinate> changed_tiles(const image::TiledImage& preview,
                                                 image::RevisionCursor baseline) {
    if (preview.revision_cursor().epoch == baseline.epoch) {
        return preview.changed_tiles_after(baseline.revision).coordinates;
    }
    std::vector<image::TileCoordinate> result;
    result.reserve(static_cast<std::size_t>(preview.tile_columns()) * preview.tile_rows());
    for (std::uint32_t y = 0; y < preview.tile_rows(); ++y) {
        for (std::uint32_t x = 0; x < preview.tile_columns(); ++x) {
            result.push_back({x, y});
        }
    }
    return result;
}

void require_state(PaintPreviewState actual, PaintPreviewState expected, std::string_view action) {
    if (actual != expected) {
        throw std::logic_error("cannot " + std::string(action) + " in the current preview state");
    }
}

const image::TiledImage& source_pixels(const doc::TextureChannels& channels,
                                       std::string_view channel_semantic) {
    if (channel_semantic.empty()) {
        throw std::invalid_argument("paint preview channel semantic must not be empty");
    }
    return channels.pixels(channel_semantic);
}

}  // namespace

class PaintPreviewSession::Impl {
public:
    Impl(const doc::TextureChannels& channels, std::string_view channel_semantic,
         std::pmr::memory_resource* memory_resource)
        : channels_identity_(&channels),
          source_pixels_identity_(&source_pixels(channels, channel_semantic)),
          channel_semantic_(channel_semantic, memory_resource),
          baseline_(source_pixels_identity_->revision_cursor()),
          preview_(*source_pixels_identity_),
          parameter_report_(memory_resource) {}

    void write_pixel(std::uint32_t x, std::uint32_t y, std::span<const std::byte> pixel) {
        require_state(state_, PaintPreviewState::provisional, "write a preview pixel");
        preview_.write_pixel(x, y, pixel);
    }

    const image::TiledImage& provisional_preview() const {
        require_state(state_, PaintPreviewState::provisional, "read a provisional preview");
        return preview_;
    }

    const image::TiledImage& finalize(std::span<const std::uint8_t> coverage,
                                      std::uint32_t dilation_radius) {
        require_state(state_, PaintPreviewState::provisional, "finalize a preview");
        const SeamDilationResult dilation =
            dilate_uv_seams(decode_raster(preview_), coverage, dilation_radius);
        image::TiledImage finalized = preview_;
        encode_raster(dilation.raster, finalized);
        preview_ = std::move(finalized);
        dilated_texel_count_ = dilation.dilated_texel_count;
        zero_gradient_texel_count_ = dilation.zero_gradient_texel_count;
        dilation_radius_ = dilation.radius;
        parameter_report_.clamps.clear();
        for (const ToolParameterClamp& clamp : dilation.parameter_report.clamps) {
            parameter_report_.clamps.push_back(
                {.name = std::pmr::string(clamp.name,
                                          parameter_report_.clamps.get_allocator().resource()),
                 .supplied = clamp.supplied,
                 .resolved = clamp.resolved});
        }
        state_ = PaintPreviewState::final;
        return preview_;
    }

    PaintPreviewCommitReport commit(doc::TextureChannels& channels) {
        require_state(state_, PaintPreviewState::final, "commit a preview");
        image::TiledImage& target = channels.pixels(channel_semantic_);
        if (&channels != channels_identity_ || &target != source_pixels_identity_ ||
            target.revision_cursor() != baseline_) {
            throw std::logic_error("paint preview source channel changed before commit");
        }
        PaintPreviewCommitReport report{.baseline = baseline_,
                                        .preview = preview_.revision_cursor(),
                                        .committed = {},
                                        .changed_tiles = changed_tiles(preview_, baseline_),
                                        .maximum_component_error = paint_preview_commit_tolerance};
        static_assert(std::is_nothrow_move_assignable_v<image::TiledImage>);
        image::TiledImage published = preview_;
        target = std::move(published);
        report.committed = target.revision_cursor();
        state_ = PaintPreviewState::committed;
        return report;
    }

    void cancel() {
        if (state_ == PaintPreviewState::committed) {
            throw std::logic_error("cannot cancel a committed paint preview");
        }
        state_ = PaintPreviewState::cancelled;
    }

    const image::TiledImage& preview_pixels() const noexcept { return preview_; }
    std::string_view channel_semantic() const noexcept { return channel_semantic_; }
    PaintPreviewState state() const noexcept { return state_; }
    std::size_t dilated_texel_count() const noexcept { return dilated_texel_count_; }
    std::size_t zero_gradient_texel_count() const noexcept { return zero_gradient_texel_count_; }
    std::uint32_t dilation_radius() const noexcept { return dilation_radius_; }
    const ToolParameterReport& parameter_report() const noexcept { return parameter_report_; }

private:
    const doc::TextureChannels* channels_identity_;
    const image::TiledImage* source_pixels_identity_;
    std::pmr::string channel_semantic_;
    image::RevisionCursor baseline_;
    image::TiledImage preview_;
    PaintPreviewState state_{PaintPreviewState::provisional};
    std::size_t dilated_texel_count_{};
    std::size_t zero_gradient_texel_count_{};
    std::uint32_t dilation_radius_{};
    ToolParameterReport parameter_report_;
};

PaintPreviewSession::PaintPreviewSession(const doc::TextureChannels& document_channels,
                                         std::string_view channel_semantic,
                                         std::pmr::memory_resource* memory_resource) {
    static_cast<void>(source_pixels(document_channels, channel_semantic));
    memory_resource_ =
        memory_resource != nullptr ? memory_resource : std::pmr::get_default_resource();
    std::pmr::polymorphic_allocator<Impl> allocator(memory_resource_);
    impl_ = allocator.allocate(1);
    try {
        std::construct_at(impl_, document_channels, channel_semantic, memory_resource_);
    } catch (...) {
        allocator.deallocate(impl_, 1);
        impl_ = nullptr;
        throw;
    }
}

PaintPreviewSession::~PaintPreviewSession() {
    if (impl_ != nullptr) {
        std::destroy_at(impl_);
        std::pmr::polymorphic_allocator<Impl>(memory_resource_).deallocate(impl_, 1);
    }
}

PaintPreviewSession::PaintPreviewSession(PaintPreviewSession&& other) noexcept
    : impl_(std::exchange(other.impl_, nullptr)),
      memory_resource_(std::exchange(other.memory_resource_, nullptr)) {}

PaintPreviewSession& PaintPreviewSession::operator=(PaintPreviewSession&& other) noexcept {
    if (this != &other) {
        this->~PaintPreviewSession();
        ::new (this) PaintPreviewSession(std::move(other));
    }
    return *this;
}

void PaintPreviewSession::write_pixel(std::uint32_t x, std::uint32_t y,
                                      std::span<const std::byte> pixel) {
    impl_->write_pixel(x, y, pixel);
}

const image::TiledImage& PaintPreviewSession::provisional_preview() const {
    return impl_->provisional_preview();
}

const image::TiledImage& PaintPreviewSession::finalize(std::span<const std::uint8_t> coverage,
                                                       std::uint32_t dilation_radius) {
    return impl_->finalize(coverage, dilation_radius);
}

PaintPreviewCommitReport PaintPreviewSession::commit(doc::TextureChannels& document_channels) {
    return impl_->commit(document_channels);
}

void PaintPreviewSession::cancel() { impl_->cancel(); }

const image::TiledImage& PaintPreviewSession::preview_pixels() const noexcept {
    return impl_->preview_pixels();
}

std::string_view PaintPreviewSession::channel_semantic() const noexcept {
    return impl_->channel_semantic();
}

PaintPreviewState PaintPreviewSession::state() const noexcept { return impl_->state(); }

std::size_t PaintPreviewSession::dilated_texel_count() const noexcept {
    return impl_->dilated_texel_count();
}

std::size_t PaintPreviewSession::zero_gradient_texel_count() const noexcept {
    return impl_->zero_gradient_texel_count();
}

std::uint32_t PaintPreviewSession::dilation_radius() const noexcept {
    return impl_->dilation_radius();
}

const ToolParameterReport& PaintPreviewSession::parameter_report() const noexcept {
    return impl_->parameter_report();
}

}  // namespace ctex::paint
