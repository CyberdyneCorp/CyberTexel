// swift-tools-version: 5.9

import PackageDescription

// The mobile MSL reference host. It consumes the published CyberTexel package
// from the repository rather than reaching into its sources, so it exercises
// the same surface an application would resolve.
let package = Package(
    name: "CyberTexelHostMetal",
    platforms: [.macOS(.v13), .iOS(.v16)],
    products: [
        .executable(name: "cybertexel-host-metal", targets: ["CyberTexelHostMetal"])
    ],
    dependencies: [
        .package(path: "../../swift")
    ],
    targets: [
        .executableTarget(
            name: "CyberTexelHostMetal",
            dependencies: [.product(name: "CyberTexel", package: "swift")]
        )
    ]
)
