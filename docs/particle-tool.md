# Particle tool

`simulate_particles` emits point particles from a position and direction and
advances them at a fixed 120 Hz. Each swept step ray-tests the existing
revision-aware mesh spatial index, so a fast particle cannot pass through a
triangle merely because no discrete state landed on it. Contacts record stable
particle/collision ordinals, time, position, oriented geometric normal, UV,
texture-set and triangle identity, impact speed, impulse and deposit strength.

The required controls are all active:

- count determines how many independently simulated states are emitted;
- lifetime bounds each state's simulated duration;
- initial speed sets the base launch velocity;
- mass scales contact impulse and therefore deposited paint;
- gravity is integrated as acceleration on every fixed step;
- friction removes a normalized fraction of tangential bounce velocity;
- restitution retains a normalized fraction of reflected normal velocity; and
- randomness perturbs launch direction and speed from the explicit 64-bit seed.

The random bit stream is implemented by the library rather than a standard
library distribution. Replaying identical mesh data, emitter, settings and seed
therefore produces exactly equal ordered contacts and final states. Contacts are
ordered first by particle ordinal and then by collision ordinal. Simulation is
bounded to 100,000 particles, 60 seconds per particle and 16 contacts per
particle. The complete numeric contract is:

| Control | Default | Minimum | Maximum |
|---|---:|---:|---:|
| Count | 1 | 1 | 100,000 |
| Lifetime (seconds) | 1 | 0.000001 | 60 |
| Initial speed | 1 | 0 | 1,000,000 |
| Mass | 1 | 0.000001 | 1,000,000 |
| Gravity, each axis | `(0, -9.81, 0)` | -1,000,000 | 1,000,000 |
| Friction | 0.5 | 0 | 1 |
| Restitution | 0 | 0 | 1 |
| Randomness | 0 | 0 | 1 |

Finite values outside these ranges are clamped. `ParticleSimulationResult`
returns both `resolved_settings` and `parameter_report`; those settings drive
the simulation. Non-finite values, an invalid emitter direction and missing
texture-set bindings are refused before simulation.

`apply_particles` maps contacts only onto the matching texture-set ID, UDIM tile
and source-triangle texel of a current `CachedSurfaceMaps` revision. Multiple
contacts accumulate with the canonical build-up union formula. The resulting
strength then passes through paint masks, optional rejection acceptance and the
shared stroke-start channel shader. The result exposes every contact-to-texel
mapping, including `no_particle_texel` for contacts outside the requested
surface.
