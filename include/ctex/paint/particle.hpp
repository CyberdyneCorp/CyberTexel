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
inline constexpr double minimum_particle_lifetime_seconds = stroke_position_tolerance;
inline constexpr double maximum_particle_lifetime_seconds = 60.0;
inline constexpr double maximum_particle_speed = 1'000'000.0;
inline constexpr double minimum_particle_mass = stroke_position_tolerance;
inline constexpr double maximum_particle_mass = 1'000'000.0;
inline constexpr double maximum_particle_acceleration = 1'000'000.0;
inline constexpr std::uint32_t maximum_particle_collisions = 16;
inline constexpr std::size_t no_particle_texel = static_cast<std::size_t>(-1);
inline constexpr ToolParameterDescriptor particle_count_parameter{"particle.count", 1.0, 1.0,
                                                                  maximum_particle_count};
inline constexpr ToolParameterDescriptor particle_lifetime_parameter{
    "particle.lifetime_seconds", 1.0, minimum_particle_lifetime_seconds,
    maximum_particle_lifetime_seconds};
inline constexpr ToolParameterDescriptor particle_initial_speed_parameter{
    "particle.initial_speed", 1.0, 0.0, maximum_particle_speed};
inline constexpr ToolParameterDescriptor particle_mass_parameter{
    "particle.mass", 1.0, minimum_particle_mass, maximum_particle_mass};
inline constexpr ToolParameterDescriptor particle_gravity_x_parameter{
    "particle.gravity.x", 0.0, -maximum_particle_acceleration, maximum_particle_acceleration};
inline constexpr ToolParameterDescriptor particle_gravity_y_parameter{
    "particle.gravity.y", -9.81, -maximum_particle_acceleration, maximum_particle_acceleration};
inline constexpr ToolParameterDescriptor particle_gravity_z_parameter{
    "particle.gravity.z", 0.0, -maximum_particle_acceleration, maximum_particle_acceleration};
inline constexpr ToolParameterDescriptor particle_friction_parameter{"particle.friction", 0.5, 0.0,
                                                                     1.0};
inline constexpr ToolParameterDescriptor particle_restitution_parameter{"particle.restitution", 0.0,
                                                                        0.0, 1.0};
inline constexpr ToolParameterDescriptor particle_randomness_parameter{"particle.randomness", 0.0,
                                                                       0.0, 1.0};

struct ParticleEmitter {
    Vec3d position;
    Vec3d direction{0.0, 0.0, -1.0};
};

struct ParticleSettings {
    std::uint32_t count{static_cast<std::uint32_t>(particle_count_parameter.default_value)};
    double lifetime_seconds{particle_lifetime_parameter.default_value};
    double initial_speed{particle_initial_speed_parameter.default_value};
    double mass{particle_mass_parameter.default_value};
    Vec3d gravity{particle_gravity_x_parameter.default_value,
                  particle_gravity_y_parameter.default_value,
                  particle_gravity_z_parameter.default_value};
    double friction{particle_friction_parameter.default_value};
    double restitution{particle_restitution_parameter.default_value};
    double randomness{particle_randomness_parameter.default_value};
    std::uint64_t seed{};
    friend constexpr bool operator==(ParticleSettings, ParticleSettings) noexcept = default;
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
    ParticleSettings resolved_settings;
    ToolParameterReport parameter_report;
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
