#include <cmath>
#include <ctex/maps/bake_provider.hpp>
#include <limits>
#include <memory>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace ctex::maps {
namespace {

void validate_provider(const BakeProvider& provider) {
    if (provider.name == nullptr || std::string_view(provider.name).empty() ||
        provider.can_produce == nullptr || provider.request == nullptr) {
        throw std::invalid_argument("bake provider requires a name, capability query and request");
    }
}

void validate_request_dimensions(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("bake request resolution must be non-zero");
    }
}

std::string advertised_maps(const BakeProvider& provider) {
    std::string result;
    for (const MeshMapKind kind : all_mesh_map_kinds) {
        if (!provider.can_produce(provider.user_data, kind)) {
            continue;
        }
        if (!result.empty()) {
            result += ", ";
        }
        result += mesh_map_name(kind);
    }
    return result.empty() ? "none" : result;
}

bool cancelled(const BakeControl& control) {
    return control.is_cancelled != nullptr && control.is_cancelled(control.user_data);
}

void report(const BakeControl& control, double fraction) {
    if (control.report_progress != nullptr) {
        control.report_progress(control.user_data, {.fraction = fraction});
    }
}

struct ProviderControlState {
    BakeControl caller;
    double latest_fraction{};
    bool invalid_progress{};
};

bool provider_cancelled(void* user_data) noexcept {
    auto& state = *static_cast<ProviderControlState*>(user_data);
    return cancelled(state.caller);
}

void provider_progress(void* user_data, BakeProgress progress) noexcept {
    auto& state = *static_cast<ProviderControlState*>(user_data);
    if (!std::isfinite(progress.fraction) || progress.fraction < state.latest_fraction ||
        progress.fraction < 0.0 || progress.fraction > 1.0) {
        state.invalid_progress = true;
        return;
    }
    if (progress.fraction == state.latest_fraction) {
        return;
    }
    state.latest_fraction = progress.fraction;
    if (progress.fraction < 1.0) {
        report(state.caller, progress.fraction);
    }
}

std::size_t checked_row_bytes(const BakeImageView& image) {
    const std::size_t pixel_size = image.format.bytes_per_pixel();
    if (!image.format.is_valid() || image.width == 0 || image.height == 0 || pixel_size == 0 ||
        image.width > std::numeric_limits<std::size_t>::max() / pixel_size) {
        throw std::invalid_argument("bake provider returned an invalid image format or extent");
    }
    return static_cast<std::size_t>(image.width) * pixel_size;
}

std::shared_ptr<const image::TiledImage> copy_image(const BakeImageView& source,
                                                    std::uint32_t requested_width,
                                                    std::uint32_t requested_height) {
    if (source.width != requested_width || source.height != requested_height) {
        throw std::invalid_argument(
            "bake provider returned a resolution different from the request");
    }
    const std::size_t row_bytes = checked_row_bytes(source);
    if (source.row_stride_bytes < row_bytes ||
        source.height - 1 >
            (std::numeric_limits<std::size_t>::max() - row_bytes) / source.row_stride_bytes) {
        throw std::invalid_argument("bake provider returned an invalid image row stride");
    }
    const std::size_t required_bytes =
        static_cast<std::size_t>(source.height - 1) * source.row_stride_bytes + row_bytes;
    if (source.pixels == nullptr || source.pixel_bytes < required_bytes) {
        throw std::invalid_argument("bake provider returned an incomplete image buffer");
    }
    auto result = std::make_shared<image::TiledImage>(source.width, source.height, source.format);
    const auto* bytes = static_cast<const std::byte*>(source.pixels);
    const std::size_t pixel_size = source.format.bytes_per_pixel();
    for (std::uint32_t y = 0; y < source.height; ++y) {
        for (std::uint32_t x = 0; x < source.width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * source.row_stride_bytes +
                                       static_cast<std::size_t>(x) * pixel_size;
            result->write_pixel(x, y, std::span(bytes + offset, pixel_size));
        }
    }
    return result;
}

BakeRequestResult provider_failure(const BakeProvider& provider, const BakeProviderOutput& output,
                                   std::string fallback) {
    const std::string detail = output.detail == nullptr || std::string_view(output.detail).empty()
                                   ? std::move(fallback)
                                   : std::string(output.detail);
    return {.status = BakeRequestStatus::provider_failed,
            .binding = std::nullopt,
            .message = "bake provider '" + std::string(provider.name) + "' failed: " + detail};
}

}  // namespace

BakeRequestResult request_bake(const BakeProvider& provider, MeshMapSet& target, MeshMapKind kind,
                               std::uint32_t width, std::uint32_t height, BakeControl control) {
    validate_provider(provider);
    validate_request_dimensions(width, height);
    const std::string map_name(mesh_map_name(kind));
    if (!provider.can_produce(provider.user_data, kind)) {
        return {.status = BakeRequestStatus::unsupported,
                .binding = std::nullopt,
                .message = "bake provider '" + std::string(provider.name) + "' cannot produce '" +
                           map_name + "'; advertised maps: " + advertised_maps(provider)};
    }

    report(control, 0.0);
    if (cancelled(control)) {
        return {.status = BakeRequestStatus::cancelled,
                .binding = std::nullopt,
                .message = "bake request for '" + map_name + "' cancelled before provider work"};
    }

    ProviderControlState state{.caller = control};
    const BakeControl provider_control{.user_data = &state,
                                       .is_cancelled = provider_cancelled,
                                       .report_progress = provider_progress};
    const mesh::MeshRevision requested_mesh_revision = target.mesh_revision();
    const BakeRequest request{.kind = kind,
                              .texture_set_id = target.texture_set_id().c_str(),
                              .uv_set = target.uv_set().c_str(),
                              .mesh_revision = requested_mesh_revision,
                              .width = width,
                              .height = height};
    BakeProviderOutput output{};
    const BakeProviderStatus status =
        provider.request(provider.user_data, &request, &provider_control, &output);
    if (status == BakeProviderStatus::cancelled || cancelled(control)) {
        return {.status = BakeRequestStatus::cancelled,
                .binding = std::nullopt,
                .message = "bake request for '" + map_name + "' cancelled without binding output"};
    }
    if (status == BakeProviderStatus::failed) {
        return provider_failure(provider, output, "provider reported failure");
    }
    if (status != BakeProviderStatus::completed) {
        return provider_failure(provider, output, "provider returned an invalid status");
    }
    if (state.invalid_progress) {
        return provider_failure(provider, output, "provider reported invalid progress");
    }

    try {
        auto pixels = copy_image(output.image, width, height);
        MeshMapBindResult binding = target.bind({.kind = kind,
                                                 .texture_set_id = target.texture_set_id(),
                                                 .uv_set = target.uv_set(),
                                                 .mesh_revision = requested_mesh_revision,
                                                 .pixels = std::move(pixels)});
        report(control, 1.0);
        return {.status = BakeRequestStatus::completed,
                .binding = std::move(binding),
                .message = "bake provider '" + std::string(provider.name) + "' produced '" +
                           map_name + "'"};
    } catch (const std::exception& error) {
        return provider_failure(provider, output, error.what());
    }
}

}  // namespace ctex::maps
