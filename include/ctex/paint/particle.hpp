#ifndef CTEX_PAINT_PARTICLE_HPP
#define CTEX_PAINT_PARTICLE_HPP

#include <cstddef>
#include <cstdint>
#include <ctex/paint/brush.hpp>
#include <ctex/paint/surface_cache.hpp>
#include <ctex/pick/hit.hpp>
#include <ctex/pick/spatial_index.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace ctex::paint {

inline constexpr std::uint32_t particle_simulation_steps_per_second = 120;
inline constexpr std::uint32_t maximum_particle_count = 100'000;
inline constexpr double maximum_particle_lifetime_seconds = 60.0;
inline constexpr std::uint32_t maximum_particle_collisions = 16;
inline constexpr std::size_t no_particle_texel = static_cast<std::size_t>(-1);

struct ParticleEmitter {
    Vec3d position;
    Vec3d direction{0.0, 0.0, -1.0};
};

struct ParticleSettings {
    std::uint32_t count{1};
    double lifetime_seconds{1.0};
    double initial_speed{1.0};
    double mass{1.0};
    Vec3d gravity{0.0, -9.81, 0.0};
    double friction{0.5};
    double restitution{};
    double randomness{};
    std::uint64_t seed{};
};

struct ParticleContact {
    std::uint32_t particle_ordinal{};
    std::uint32_t collision_ordinal{};
    double time_seconds{};
    Vec3d position;
    Vec3d normal;
    Vec2d uv;
    std::string texture_set_id;
    std::uint32_t triangle{};
    double impact_speed{};
    double impulse{};
    double strength{};
    friend bool operator==(const ParticleContact&, const ParticleContact&) = default;
};

struct ParticleState {
    Vec3d position;
    Vec3d velocity;
    double simulated_seconds{};
    std::uint32_t collision_count{};
    bool resting{};
    friend constexpr bool operator==(ParticleState, ParticleState) noexcept = default;
};

struct ParticleSimulationResult {
    std::uint64_t seed{};
    std::uint32_t emitted_count{};
    std::vector<ParticleContact> contacts;
    std::vector<ParticleState> final_states;
    friend bool operator==(const ParticleSimulationResult&,
                           const ParticleSimulationResult&) = default;
};

[[nodiscard]] ParticleSimulationResult simulate_particles(
    pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
    std::span<const pick::TextureSetBindingView> texture_sets, const ParticleEmitter& emitter,
    const ParticleSettings& settings);

struct ParticlePaintSettings {
    std::string_view blend_mode{"normal"};
    PaintMaskInputs masks;
    std::optional<PaintMaskView> rejection_acceptance;
};

struct ParticleResult {
    ParticleSimulationResult simulation;
    std::vector<std::size_t> contact_texels;
    std::size_t mapped_contact_count{};
    std::vector<double> strength;
    std::vector<PaintToolChannelRaster> channels;
    std::vector<std::string> applied_channel_ids;
};

[[nodiscard]] ParticleResult apply_particles(
    pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
    std::span<const pick::TextureSetBindingView> texture_sets, const CachedSurfaceMaps& surface,
    std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
    std::span<const PaintToolChannelRaster> material, const ParticleEmitter& emitter,
    const ParticleSettings& simulation_settings, const ParticlePaintSettings& paint_settings = {});

}  // namespace ctex::paint

#endif
