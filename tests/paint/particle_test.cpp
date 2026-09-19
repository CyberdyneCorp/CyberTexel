#include <array>
#include <cmath>
#include <ctex/paint/particle.hpp>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using namespace ctex;
using namespace ctex::paint;

struct ParticleMesh {
    std::array<mesh::Vec3f, 3> positions{
        mesh::Vec3f{0.0F, 0.0F, 0.0F},
        mesh::Vec3f{4.0F, 0.0F, 0.0F},
        mesh::Vec3f{0.0F, 4.0F, 0.0F},
    };
    std::array<mesh::Vec3f, 3> normals{
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
        mesh::Vec3f{0.0F, 0.0F, 1.0F},
    };
    std::array<mesh::Vec2f, 3> uv{
        mesh::Vec2f{0.0F, 0.0F},
        mesh::Vec2f{1.0F, 0.0F},
        mesh::Vec2f{0.0F, 1.0F},
    };
    std::array<std::uint32_t, 3> indices{0, 1, 2};
    std::array<mesh::UvSetView, 1> uv_sets{mesh::UvSetView{"paint", uv}};
    std::array<mesh::MeshPartition, 1> partitions{
        mesh::MeshPartition{mesh::PartitionKind::material, "body", "Body"}};
    std::array<std::uint32_t, 1> face_partitions{0};
    std::array<std::uint32_t, 1> face_materials{7};

    [[nodiscard]] mesh::MeshDescriptor descriptor() const {
        return {.positions = positions,
                .normals = normals,
                .vertex_colors = {},
                .triangle_indices = indices,
                .uv_sets = uv_sets,
                .default_uv_set = "paint",
                .partitions = partitions,
                .face_partition_indices = face_partitions,
                .face_material_ids = face_materials};
    }
};

bool expect(bool condition, std::string_view message) {
    if (!condition) {
        std::cerr << message << '\n';
    }
    return condition;
}

bool near(double actual, double expected, double tolerance = 1.0e-5) {
    return std::abs(actual - expected) <= tolerance;
}

ParticleSettings settings() {
    return {.count = 1,
            .lifetime_seconds = 1.0,
            .initial_speed = 2.0,
            .mass = 1.0,
            .gravity = {},
            .friction = 0.0,
            .restitution = 0.0,
            .randomness = 0.0,
            .seed = 42};
}

constexpr std::array bindings{pick::TextureSetBindingView{0, "paint"}};

ParticleSimulationResult simulate(const ParticleMesh& buffers, const ParticleEmitter& emitter,
                                  const ParticleSettings& requested) {
    const mesh::MeshBinding mesh_binding(buffers.descriptor());
    pick::SpatialIndex index(mesh_binding);
    return simulate_particles(index, mesh_binding, bindings, emitter, requested);
}

// parameter-audit: particle.count
// parameter-audit: particle.randomness
bool deterministic_seed_controls_the_complete_simulation() {
    const ParticleMesh buffers;
    ParticleSettings randomized = settings();
    randomized.count = 4;
    randomized.randomness = 0.35;
    randomized.restitution = 0.25;
    const ParticleEmitter emitter{.position = {1.0, 1.0, 1.0}, .direction = {0.0, 0.0, -1.0}};
    const ParticleSimulationResult first = simulate(buffers, emitter, randomized);
    const ParticleSimulationResult replay = simulate(buffers, emitter, randomized);
    randomized.seed = 43;
    const ParticleSimulationResult other_seed = simulate(buffers, emitter, randomized);

    return expect(first == replay, "the same particle seed did not replay byte-identical state") &&
           expect(first != other_seed, "changing the particle seed did not affect randomness") &&
           expect(first.emitted_count == 4 && first.final_states.size() == 4,
                  "configured particle count was not emitted");
}

// parameter-audit: particle.lifetime_seconds
// parameter-audit: particle.initial_speed
// parameter-audit: particle.mass
// parameter-audit: particle.gravity.x
// parameter-audit: particle.gravity.y
// parameter-audit: particle.gravity.z
// parameter-audit: particle.friction
// parameter-audit: particle.restitution
bool lifetime_speed_mass_gravity_friction_and_restitution_are_active() {
    const ParticleMesh buffers;
    const ParticleEmitter vertical{.position = {1.0, 1.0, 1.0}, .direction = {0.0, 0.0, -1.0}};
    const ParticleSimulationResult base = simulate(buffers, vertical, settings());

    ParticleSettings short_lived = settings();
    short_lived.lifetime_seconds = 0.25;
    const ParticleSimulationResult expired = simulate(buffers, vertical, short_lived);

    ParticleSettings heavy = settings();
    heavy.mass = 2.0;
    const ParticleSimulationResult heavy_result = simulate(buffers, vertical, heavy);

    ParticleSettings falling = settings();
    falling.initial_speed = 0.0;
    falling.gravity = {0.0, 0.0, -4.0};
    const ParticleSimulationResult gravity_result = simulate(buffers, vertical, falling);

    const ParticleEmitter axis_emitter{.position = {1.0, 1.0, 1.03}, .direction = {0.0, 0.0, -1.0}};
    const ParticleSimulationResult axis_base = simulate(buffers, axis_emitter, settings());
    ParticleSettings gravity_x = settings();
    gravity_x.gravity.x = 0.25;
    const ParticleSimulationResult gravity_x_result = simulate(buffers, axis_emitter, gravity_x);
    ParticleSettings gravity_y = settings();
    gravity_y.gravity.y = 0.25;
    const ParticleSimulationResult gravity_y_result = simulate(buffers, axis_emitter, gravity_y);

    const ParticleEmitter angled{.position = {1.0, 1.0, 1.0}, .direction = {1.0, 0.0, -1.0}};
    ParticleSettings slippery = settings();
    slippery.restitution = 0.5;
    const ParticleSimulationResult slippery_result = simulate(buffers, angled, slippery);
    ParticleSettings rough = slippery;
    rough.friction = 1.0;
    const ParticleSimulationResult rough_result = simulate(buffers, angled, rough);

    return expect(base.contacts.size() == 1 && base.final_states[0].resting &&
                      near(base.contacts[0].time_seconds, 0.5),
                  "initial speed did not produce the expected collision time") &&
           expect(expired.contacts.empty() && near(expired.final_states[0].simulated_seconds, 0.25),
                  "particle lifetime did not stop simulation before collision") &&
           expect(heavy_result.contacts[0].impulse > base.contacts[0].impulse &&
                      heavy_result.contacts[0].strength > base.contacts[0].strength,
                  "particle mass did not affect deposited collision impulse") &&
           expect(
               gravity_result.contacts.size() == 1 && gravity_result.contacts[0].time_seconds > 0.6,
               "particle gravity did not accelerate a stationary particle into the mesh") &&
           expect(axis_base.contacts.size() == 1 && gravity_x_result.contacts.size() == 1 &&
                      gravity_y_result.contacts.size() == 1,
                  "lateral-gravity fixture did not reach the particle surface") &&
           expect(
               gravity_x_result.contacts[0].position.x > axis_base.contacts[0].position.x &&
                   near(gravity_x_result.contacts[0].position.y,
                        axis_base.contacts[0].position.y) &&
                   gravity_y_result.contacts[0].position.y > axis_base.contacts[0].position.y &&
                   near(gravity_y_result.contacts[0].position.x, axis_base.contacts[0].position.x),
               "particle gravity axes did not independently change the contact position") &&
           expect(slippery_result.final_states[0].velocity.x > 1.0 &&
                      slippery_result.final_states[0].velocity.z > 0.5 &&
                      near(rough_result.final_states[0].velocity.x, 0.0) &&
                      rough_result.final_states[0].velocity.z > 0.5,
                  "particle friction or restitution did not affect bounce velocity");
}

PaintToolChannelRaster channel(std::string semantic_id, float red, std::size_t count) {
    return {.semantic_id = std::move(semantic_id),
            .component_count = 3,
            .pixels = std::vector<graph::ColourValue>(count, {red, 0.0F, 0.0F, 1.0F})};
}

CachedSurfaceMaps surface(mesh::MeshRevision revision) {
    std::vector<SurfaceTexel> texels(4);
    for (std::size_t index = 0; index < texels.size(); ++index) {
        texels[index] = {.position = {},
                         .normal = {0.0, 0.0, 1.0},
                         .geometric_normal = {0.0, 0.0, 1.0},
                         .uv = {},
                         .triangle = 0};
    }
    return {.texture_set_id =
                mesh::texture_set_stable_id(mesh::PartitionKind::material, "body", "paint"),
            .uv_set = "paint",
            .mesh_revision = revision,
            .surface = {.width = 2, .height = 2, .tile_origin = {}, .texels = std::move(texels)},
            .coverage = {1, 1, 1, 1},
            .triangle_identity = {0, 0, 0, 0},
            .uv_island_identity = {0, 0, 0, 0}};
}

bool contacts_deposit_through_masks_and_channel_shading() {
    const ParticleMesh buffers;
    const mesh::MeshBinding mesh_binding(buffers.descriptor());
    pick::SpatialIndex index(mesh_binding);
    const CachedSurfaceMaps maps = surface(mesh_binding.revision());
    const std::array layer{channel("pbr.base_color", 0.0F, 4)};
    const std::array material{channel("pbr.base_color", 1.0F, 4)};
    const std::array<double, 4> selection{0.5, 0.5, 0.5, 0.5};
    const std::array<double, 4> rejection{0.5, 0.5, 0.5, 0.5};
    const ParticleResult result =
        apply_particles(index, mesh_binding, bindings, maps, layer, material,
                        {.position = {1.0, 1.0, 1.0}, .direction = {0.0, 0.0, -1.0}}, settings(),
                        {.blend_mode = "normal",
                         .masks = {.active_layer_masks = {},
                                   .colour_id_selection = std::nullopt,
                                   .geometry_selection = PaintMaskView{selection},
                                   .screen_selection = std::nullopt,
                                   .uv_island_selection = std::nullopt},
                         .rejection_acceptance = PaintMaskView{rejection}});
    const double expected = (1.0 - std::exp(-2.0)) * 0.25;

    return expect(result.simulation.contacts.size() == 1 &&
                      result.contact_texels == std::vector<std::size_t>({2}) &&
                      result.mapped_contact_count == 1,
                  "particle contact did not map to its matching texture-set UV texel") &&
           expect(
               near(result.strength[2], expected) && near(result.channels[0].pixels[2].r, expected),
               "particle deposition did not compose masks, rejection and channel shading");
}

bool particle_parameters_are_bounded_and_reported() {
    const ParticleMesh buffers;
    ParticleSettings requested = settings();
    requested.count = 0;
    requested.lifetime_seconds = 0.0;
    requested.initial_speed = maximum_particle_speed + 1.0;
    requested.mass = 0.0;
    requested.gravity = {-maximum_particle_acceleration - 1.0, maximum_particle_acceleration + 1.0,
                         maximum_particle_acceleration + 1.0};
    requested.friction = 2.0;
    requested.restitution = 2.0;
    requested.randomness = 2.0;
    const ParticleSimulationResult resolved =
        simulate(buffers, {.position = {1.0, 1.0, 1.0}, .direction = {0.0, 0.0, -1.0}}, requested);
    const ParticleSettings& actual = resolved.resolved_settings;
    const ToolParameterReport& report = resolved.parameter_report;
    const bool values_resolved =
        actual.count == 1 && actual.lifetime_seconds == minimum_particle_lifetime_seconds &&
        actual.initial_speed == maximum_particle_speed && actual.mass == minimum_particle_mass &&
        actual.gravity.x == -maximum_particle_acceleration &&
        actual.gravity.y == maximum_particle_acceleration &&
        actual.gravity.z == maximum_particle_acceleration && actual.friction == 1.0 &&
        actual.restitution == 1.0 && actual.randomness == 1.0 && resolved.emitted_count == 1;
    const bool report_complete =
        report.clamps.size() == 10 && report.clamp_for("particle.count") &&
        report.clamp_for("particle.lifetime_seconds") &&
        report.clamp_for("particle.initial_speed") && report.clamp_for("particle.mass") &&
        report.clamp_for("particle.gravity.x") && report.clamp_for("particle.gravity.y") &&
        report.clamp_for("particle.gravity.z") && report.clamp_for("particle.friction") &&
        report.clamp_for("particle.restitution") && report.clamp_for("particle.randomness");

    ParticleSettings non_finite = settings();
    non_finite.mass = std::numeric_limits<double>::infinity();
    bool non_finite_refused = false;
    try {
        static_cast<void>(simulate(buffers, {}, non_finite));
    } catch (const std::invalid_argument&) {
        non_finite_refused = true;
    }
    bool bindings_refused = false;
    try {
        const mesh::MeshBinding mesh_binding(buffers.descriptor());
        pick::SpatialIndex index(mesh_binding);
        static_cast<void>(simulate_particles(index, mesh_binding, {}, {}, settings()));
    } catch (const std::invalid_argument&) {
        bindings_refused = true;
    }
    bool stale_surface_refused = false;
    try {
        const mesh::MeshBinding mesh_binding(buffers.descriptor());
        pick::SpatialIndex index(mesh_binding);
        const CachedSurfaceMaps stale = surface(mesh_binding.revision() + 1);
        const std::array layer{channel("pbr.base_color", 0.0F, 4)};
        const std::array material{channel("pbr.base_color", 1.0F, 4)};
        static_cast<void>(
            apply_particles(index, mesh_binding, bindings, stale, layer, material, {}, settings()));
    } catch (const std::invalid_argument&) {
        stale_surface_refused = true;
    }
    return expect(values_resolved && report_complete,
                  "particle parameters were not bounded, reported, and used") &&
           expect(non_finite_refused && bindings_refused && stale_surface_refused,
                  "non-finite particle input, missing bindings or stale surface was not refused");
}

}  // namespace

int main() {
    return deterministic_seed_controls_the_complete_simulation() &&
                   lifetime_speed_mass_gravity_friction_and_restitution_are_active() &&
                   contacts_deposit_through_masks_and_channel_shading() &&
                   particle_parameters_are_bounded_and_reported()
               ? 0
               : 1;
}
