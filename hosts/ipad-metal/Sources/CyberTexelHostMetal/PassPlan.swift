import Foundation

/// The subset of a CyberTexel pass plan this reference host consumes.
///
/// Decoding is strict about what it needs: a field this host acts on that the
/// plan stops emitting fails the run rather than being silently defaulted, so a
/// change to the emitted plan breaks the host and CI.
struct PassPlan: Decodable {
  struct ResourceVersion: Decodable {
    let logicalID: String
    let generation: UInt64

    enum CodingKeys: String, CodingKey {
      case logicalID = "logical_id"
      case generation
    }
  }

  struct Extent: Decodable {
    let width: UInt32
    let height: UInt32
    let layers: UInt32
  }

  struct TileShape: Decodable {
    let width: UInt32
    let height: UInt32
  }

  struct Resource: Decodable {
    let version: ResourceVersion
    let format: String
    let extent: Extent
    let role: String
    let mipLevels: UInt32
    let externallyInitialized: Bool
    let tileShape: TileShape

    enum CodingKeys: String, CodingKey {
      case version, format, extent, role
      case mipLevels = "mip_levels"
      case externallyInitialized = "externally_initialized"
      case tileShape = "tile_shape"
    }
  }

  struct EntryPoints: Decodable {
    let vertex: String
    let fragment: String
    let compute: String
  }

  struct RenderTarget: Decodable {
    let resource: ResourceVersion
    let format: String
    let width: UInt32
    let height: UInt32
    let load: String
    let clearColour: [Double]

    enum CodingKeys: String, CodingKey {
      case resource, format, width, height, load
      case clearColour = "clear_colour"
    }
  }

  struct Command: Decodable {
    let kind: String
    let topology: String
    let vertexCount: UInt32
    let instanceCount: UInt32
    let firstVertex: UInt32
    let indexed: Bool

    enum CodingKeys: String, CodingKey {
      case kind, topology, indexed
      case vertexCount = "vertex_count"
      case instanceCount = "instance_count"
      case firstVertex = "first_vertex"
    }
  }

  struct VertexAttribute: Decodable {
    let location: UInt32
    let offset: Int
    let format: String
    let semantic: String
  }

  struct VertexBuffer: Decodable {
    let slot: Int
    let stride: Int
    let stepMode: String
    let attributes: [VertexAttribute]

    enum CodingKeys: String, CodingKey {
      case slot, stride, attributes
      case stepMode = "step_mode"
    }
  }

  struct Pass: Decodable {
    let identifier: String
    let kind: String
    let entryPoints: EntryPoints
    let renderTargets: [RenderTarget]
    let command: Command
    let vertexBuffers: [VertexBuffer]
    let textureBindings: [AnyCodable]
    let samplerBindings: [AnyCodable]
    let uniformBlocks: [AnyCodable]
    /// Declared so a depth target the host does not carry is refused rather
    /// than silently dropped.
    let depthTarget: AnyCodable?
    let depthState: AnyCodable
    let accesses: [AnyCodable]
    let dependencies: [AnyCodable]

    enum CodingKeys: String, CodingKey {
      case identifier, kind, command, accesses, dependencies
      case entryPoints = "entry_points"
      case renderTargets = "render_targets"
      case vertexBuffers = "vertex_buffers"
      case textureBindings = "texture_bindings"
      case samplerBindings = "sampler_bindings"
      case uniformBlocks = "uniform_blocks"
      case depthTarget = "depth_target"
      case depthState = "depth_state"
    }
  }

  let stableIdentity: String
  let resources: [Resource]
  let passes: [Pass]
  let lifetimes: [AnyCodable]

  enum CodingKeys: String, CodingKey {
    case resources, passes, lifetimes
    case stableIdentity = "stable_identity"
  }
}

/// A placeholder for plan content this host does not act on but must see the
/// shape of, so an emitted binding is noticed rather than ignored.
struct AnyCodable: Decodable {}
