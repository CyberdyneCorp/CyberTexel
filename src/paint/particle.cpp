#include <algorithm>
#include <cmath>
#include <ctex/paint/particle.hpp>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string>
#include <utility>

namespace ctex::paint {
namespace {

constexpr double vector_epsilon = 1.0e-12;
constexpr double contact_offset = 1.0e-5;
constexpr double resting_speed = 1.0e-6;

class ParticleRandom {
public:
    explicit ParticleRandom(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        state_ += 0x9E3779B97F4A7C15ULL;
        std::uint64_t value = state_;
        value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31U);
    }

    double unit() { return static_cast<double>(next() >> 11U) * 0x1.0p-53; }

private:
    std::uint64_t state_;
};

bool finite(Vec2d value) { return std::isfinite(value.x) && std::isfinite(value.y); }

bool finite(Vec3d value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

Vec3d add(Vec3d left, Vec3d right) {
    return {left.x + right.x, left.y + right.y, left.z + right.z};
}

Vec3d subtract(Vec3d left, Vec3d right) {
    return {left.x - right.x, left.y - right.y, left.z - right.z};
}

Vec3d multiply(Vec3d value, double scale) {
    return {value.x * scale, value.y * scale, value.z * scale};
}

double dot(Vec3d left, Vec3d right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

double length(Vec3d value) { return std::sqrt(dot(value, value)); }

Vec3d normalized(Vec3d value, std::string_view role) {
    const double magnitude = length(value);
    if (!finite(value) || !std::isfinite(magnitude) || magnitude <= vector_epsilon) {
        throw std::invalid_argument(std::string(role) + " must be finite and non-zero");
    }
    return multiply(value, 1.0 / magnitude);
}

void validate_settings(const ParticleEmitter& emitter, const ParticleSettings& settings,
                       std::span<const pick::TextureSetBindingView> texture_sets) {
    const bool scalar_valid =
        settings.count > 0 && settings.count <= maximum_particle_count &&
        std::isfinite(settings.lifetime_seconds) && settings.lifetime_seconds > 0.0 &&
        settings.lifetime_seconds <= maximum_particle_lifetime_seconds &&
        std::isfinite(settings.initial_speed) && settings.initial_speed >= 0.0 &&
        std::isfinite(settings.mass) && settings.mass > 0.0 && std::isfinite(settings.friction) &&
        settings.friction >= 0.0 && settings.friction <= 1.0 &&
        std::isfinite(settings.restitution) && settings.restitution >= 0.0 &&
        settings.restitution <= 1.0 && std::isfinite(settings.randomness) &&
        settings.randomness >= 0.0 && settings.randomness <= 1.0;
    if (!scalar_valid || !finite(emitter.position) || !finite(settings.gravity) ||
        texture_sets.empty()) {
        throw std::invalid_argument(
            "particle emitter, simulation settings or bindings are invalid");
    }
    static_cast<void>(normalized(emitter.direction, "particle emitter direction"));
}

ParticleSettings resolve_parameters(const ParticleSettings& requested,
                                    ToolParameterReport& report) {
    ParticleSettings resolved = requested;
    resolved.count = static_cast<std::uint32_t>(
        validate_tool_parameter(particle_count_parameter, requested.count, report));
    resolved.lifetime_seconds =
        validate_tool_parameter(particle_lifetime_parameter, requested.lifetime_seconds, report);
    resolved.initial_speed =
        validate_tool_parameter(particle_initial_speed_parameter, requested.initial_speed, report);
    resolved.mass = validate_tool_parameter(particle_mass_parameter, requested.mass, report);
    resolved.gravity.x =
        validate_tool_parameter(particle_gravity_x_parameter, requested.gravity.x, report);
    resolved.gravity.y =
        validate_tool_parameter(particle_gravity_y_parameter, requested.gravity.y, report);
    resolved.gravity.z =
        validate_tool_parameter(particle_gravity_z_parameter, requested.gravity.z, report);
    resolved.friction =
        validate_tool_parameter(particle_friction_parameter, requested.friction, report);
    resolved.restitution =
        validate_tool_parameter(particle_restitution_parameter, requested.restitution, report);
    resolved.randomness =
        validate_tool_parameter(particle_randomness_parameter, requested.randomness, report);
    return resolved;
}

Vec3d random_unit_vector(ParticleRandom& random) {
    const double z = random.unit() * 2.0 - 1.0;
    const double angle = random.unit() * 2.0 * std::numbers::pi;
    const double radius = std::sqrt(std::max(0.0, 1.0 - z * z));
    return {radius * std::cos(angle), radius * std::sin(angle), z};
}

Vec3d initial_velocity(Vec3d direction, const ParticleSettings& settings, ParticleRandom& random) {
    const Vec3d perturbed =
        add(direction, multiply(random_unit_vector(random), settings.randomness));
    const Vec3d resolved_direction = length(perturbed) <= vector_epsilon
                                         ? direction
                                         : normalized(perturbed, "particle direction");
    const double speed_scale = 1.0 + settings.randomness * (random.unit() * 2.0 - 1.0) * 0.5;
    return multiply(resolved_direction, settings.initial_speed * speed_scale);
}

mesh::Vec3f as_float(Vec3d value, std::string_view role) {
    const double limit = std::numeric_limits<float>::max();
    if (!finite(value) || std::abs(value.x) > limit || std::abs(value.y) > limit ||
        std::abs(value.z) > limit) {
        throw std::overflow_error(std::string(role) + " exceeds the picking coordinate range");
    }
    return {static_cast<float>(value.x), static_cast<float>(value.y), static_cast<float>(value.z)};
}

Vec3d as_double(mesh::Vec3f value) { return {value.x, value.y, value.z}; }

struct CollisionStep {
    std::optional<pick::HitRecord> hit;
    Vec3d accelerated_velocity;
    Vec3d displacement;
    double distance{};
};

CollisionStep trace_step(pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
                         std::span<const pick::TextureSetBindingView> texture_sets,
                         const ParticleState& state, Vec3d gravity, double duration) {
    const Vec3d accelerated = add(state.velocity, multiply(gravity, duration));
    const Vec3d displacement = multiply(add(state.velocity, accelerated), duration * 0.5);
    const double distance = length(displacement);
    if (!std::isfinite(distance) || distance > std::numeric_limits<float>::max()) {
        throw std::overflow_error("particle step exceeds the picking distance range");
    }
    if (distance <= vector_epsilon) {
        return {.hit = std::nullopt,
                .accelerated_velocity = accelerated,
                .displacement = displacement,
                .distance = distance};
    }
    const Vec3d direction = multiply(displacement, 1.0 / distance);
    float maximum_distance = static_cast<float>(distance);
    if (static_cast<double>(maximum_distance) < distance) {
        maximum_distance = std::nextafter(maximum_distance, std::numeric_limits<float>::infinity());
    }
    const pick::Ray ray{.origin = as_float(state.position, "particle position"),
                        .direction = as_float(direction, "particle direction")};
    return {.hit = pick::pick_nearest(index, mesh, ray, maximum_distance, texture_sets),
            .accelerated_velocity = accelerated,
            .displacement = displacement,
            .distance = distance};
}

Vec3d oriented_normal(const pick::HitRecord& hit, Vec3d velocity) {
    Vec3d result = normalized(as_double(hit.geometric_normal), "particle collision normal");
    if (dot(velocity, result) > 0.0) {
        result = multiply(result, -1.0);
    }
    return result;
}

Vec3d bounce_velocity(Vec3d impact, Vec3d normal, const ParticleSettings& settings) {
    const double normal_velocity = dot(impact, normal);
    const Vec3d tangent = subtract(impact, multiply(normal, normal_velocity));
    return add(multiply(tangent, 1.0 - settings.friction),
               multiply(normal, -normal_velocity * settings.restitution));
}

ParticleContact contact_from_hit(const pick::HitRecord& hit, std::uint32_t particle,
                                 std::uint32_t collision, double time, Vec3d impact, Vec3d normal,
                                 const ParticleSettings& settings) {
    const double normal_speed = std::max(0.0, -dot(impact, normal));
    const double impulse = settings.mass * (1.0 + settings.restitution) * normal_speed;
    return {.particle_ordinal = particle,
            .collision_ordinal = collision,
            .time_seconds = time,
            .position = as_double(hit.position),
            .normal = normal,
            .uv = {hit.uv.x, hit.uv.y},
            .texture_set_id = hit.texture_set_id,
            .triangle = hit.triangle_index,
            .impact_speed = length(impact),
            .impulse = impulse,
            .strength = 1.0 - std::exp(-impulse)};
}

void resolve_collision(ParticleState& state, const CollisionStep& step,
                       const ParticleSettings& settings, std::uint32_t particle,
                       std::vector<ParticleContact>& contacts, double duration) {
    const double fraction =
        std::clamp(static_cast<double>(step.hit->distance) / step.distance, 0.0, 1.0);
    const Vec3d impact = add(
        state.velocity, multiply(subtract(step.accelerated_velocity, state.velocity), fraction));
    const Vec3d normal = oriented_normal(*step.hit, impact);
    contacts.push_back(contact_from_hit(*step.hit, particle, state.collision_count,
                                        state.simulated_seconds + duration * fraction, impact,
                                        normal, settings));
    state.position = add(as_double(step.hit->position), multiply(normal, contact_offset));
    state.velocity = bounce_velocity(impact, normal, settings);
    ++state.collision_count;
    state.resting = length(state.velocity) <= resting_speed ||
                    state.collision_count >= maximum_particle_collisions;
}

ParticleState simulate_one(pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
                           std::span<const pick::TextureSetBindingView> texture_sets,
                           const ParticleEmitter& emitter, const ParticleSettings& settings,
                           std::uint32_t particle, ParticleRandom& random,
                           std::vector<ParticleContact>& contacts) {
    ParticleState state{.position = emitter.position,
                        .velocity = initial_velocity(
                            normalized(emitter.direction, "particle direction"), settings, random)};
    constexpr double fixed_step = 1.0 / particle_simulation_steps_per_second;
    while (!state.resting && state.simulated_seconds < settings.lifetime_seconds) {
        const double duration =
            std::min(fixed_step, settings.lifetime_seconds - state.simulated_seconds);
        const CollisionStep step =
            trace_step(index, mesh, texture_sets, state, settings.gravity, duration);
        if (step.hit) {
            resolve_collision(state, step, settings, particle, contacts, duration);
        } else {
            state.position = add(state.position, step.displacement);
            state.velocity = step.accelerated_velocity;
        }
        state.simulated_seconds += duration;
    }
    return state;
}

std::size_t checked_surface(const CachedSurfaceMaps& surface) {
    if (surface.surface.width == 0 || surface.surface.height == 0 ||
        static_cast<std::size_t>(surface.surface.width) >
            std::numeric_limits<std::size_t>::max() / surface.surface.height) {
        throw std::invalid_argument("particle paint surface dimensions are invalid");
    }
    const std::size_t count =
        static_cast<std::size_t>(surface.surface.width) * surface.surface.height;
    if (surface.surface.texels.size() != count || surface.coverage.size() != count ||
        surface.triangle_identity.size() != count) {
        throw std::invalid_argument("particle paint surface maps are incomplete");
    }
    return count;
}

std::size_t contact_texel(const CachedSurfaceMaps& surface, const ParticleContact& contact) {
    if (contact.texture_set_id != surface.texture_set_id || !finite(contact.uv)) {
        return no_particle_texel;
    }
    const double u = contact.uv.x - surface.surface.tile_origin.x;
    const double v = contact.uv.y - surface.surface.tile_origin.y;
    if (u < 0.0 || u >= 1.0 || v < 0.0 || v >= 1.0) {
        return no_particle_texel;
    }
    const auto x = static_cast<std::uint32_t>(std::floor(u * surface.surface.width));
    const auto y =
        std::min(static_cast<std::uint32_t>(std::floor((1.0 - v) * surface.surface.height)),
                 surface.surface.height - 1);
    const std::size_t result = static_cast<std::size_t>(y) * surface.surface.width + x;
    if (surface.coverage[result] == 0 || surface.triangle_identity[result] != contact.triangle) {
        return no_particle_texel;
    }
    return result;
}

void multiply_mask(std::span<double> values, PaintMaskView mask, std::string_view role) {
    if (mask.values.size() != values.size()) {
        throw std::invalid_argument(std::string(role) + " dimensions are inconsistent");
    }
    for (std::size_t texel = 0; texel < values.size(); ++texel) {
        const double value = mask.values[texel];
        if (!std::isfinite(value) || value < 0.0 || value > 1.0) {
            throw std::invalid_argument(std::string(role) + " values are invalid");
        }
        values[texel] *= value;
    }
}

}  // namespace

ParticleSimulationResult simulate_particles(
    pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
    std::span<const pick::TextureSetBindingView> texture_sets, const ParticleEmitter& emitter,
    const ParticleSettings& requested_settings) {
    ToolParameterReport parameter_report;
    const ParticleSettings settings = resolve_parameters(requested_settings, parameter_report);
    validate_settings(emitter, settings, texture_sets);
    ParticleSimulationResult result{.resolved_settings = settings,
                                    .parameter_report = std::move(parameter_report),
                                    .seed = settings.seed,
                                    .emitted_count = settings.count,
                                    .contacts = {},
                                    .final_states = {}};
    result.final_states.reserve(settings.count);
    ParticleRandom random(settings.seed);
    for (std::uint32_t particle = 0; particle < settings.count; ++particle) {
        result.final_states.push_back(simulate_one(index, mesh, texture_sets, emitter, settings,
                                                   particle, random, result.contacts));
    }
    return result;
}

ParticleResult apply_particles(pick::SpatialIndex& index, const mesh::MeshBinding& mesh,
                               std::span<const pick::TextureSetBindingView> texture_sets,
                               const CachedSurfaceMaps& surface,
                               std::span<const PaintToolChannelRaster> enabled_layer_snapshot,
                               std::span<const PaintToolChannelRaster> material,
                               const ParticleEmitter& emitter,
                               const ParticleSettings& simulation_settings,
                               const ParticlePaintSettings& paint_settings) {
    if (surface.mesh_revision != mesh.revision()) {
        throw std::invalid_argument("particle paint surface mesh revision is stale");
    }
    const std::size_t texel_count = checked_surface(surface);
    ParticleSimulationResult simulation =
        simulate_particles(index, mesh, texture_sets, emitter, simulation_settings);
    std::vector<std::size_t> contact_texels(simulation.contacts.size(), no_particle_texel);
    std::vector<double> strength(texel_count, 0.0);
    std::size_t deposited = 0;
    for (std::size_t contact = 0; contact < simulation.contacts.size(); ++contact) {
        const std::size_t texel = contact_texel(surface, simulation.contacts[contact]);
        contact_texels[contact] = texel;
        if (texel != no_particle_texel) {
            strength[texel] =
                1.0 - (1.0 - strength[texel]) * (1.0 - simulation.contacts[contact].strength);
            ++deposited;
        }
    }
    const CombinedPaintMask masks =
        combine_paint_masks(surface.surface.width, surface.surface.height, paint_settings.masks);
    multiply_mask(strength, PaintMaskView{masks.values}, "particle masks");
    if (paint_settings.rejection_acceptance) {
        multiply_mask(strength, *paint_settings.rejection_acceptance,
                      "particle rejection acceptance");
    }
    PaintToolShadeResult shaded = shade_paint_tool_channels(
        surface.surface.width, surface.surface.height, enabled_layer_snapshot, material, strength,
        paint_settings.blend_mode);
    return {.simulation = std::move(simulation),
            .contact_texels = std::move(contact_texels),
            .mapped_contact_count = deposited,
            .strength = std::move(strength),
            .channels = std::move(shaded.channels),
            .applied_channel_ids = std::move(shaded.applied_channel_ids)};
}

}  // namespace ctex::paint
