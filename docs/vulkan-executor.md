# Optional owned Vulkan executor

CyberTexel's first owned-GPU route is Vulkan. It exists for consumers such as a
headless CLI that want acceleration without an existing host renderer; the
primary host-executed route still never creates or receives a device.

The backend is disabled by default. Enable and verify it with:

```sh
cmake --preset vulkan
cmake --build --preset vulkan
just test-vulkan
```

The equivalent cache option is `CTEX_ENABLE_VULKAN_EXECUTOR=ON`. A default
build compiles a small factory stub, fetches no Vulkan source, links no Vulkan
loader, and returns no executor. Its diagnostic names the enabling option.

## Ownership and discovery

An enabled build pins Vulkan-Headers and Volk revisions recorded in the
dependency manifest. Volk dynamically locates the platform Vulkan loader, so
CyberTexel does not link a loader into its ordinary artifact. The executor
creates and owns a headless Vulkan instance, physical-device selection, logical
device, and graphics-plus-compute queue. No surface, swapchain, window-system
extension, or device handle enters the public API.

Physical devices are selected deterministically: discrete, integrated, virtual,
CPU/software, then other devices, with device name and numeric IDs breaking
ties. A software Vulkan implementation is valid for enabled-build CI. If the
loader, physical device, joint graphics/compute queue, or required texture
capabilities are absent, the compiled executor remains enumerable with
`device-unavailable` and a diagnostic; it is not silently counted as available.

## Capability report

The descriptor is derived from the selected physical device:

- binding budget is the conservative minimum of sampled-image and total
  per-stage resource limits;
- maximum texture dimension is `maxImageDimension2D`;
- every CyberTexel format is mapped to its Vulkan format and advertised only
  when optimal tiling supports the required sampled/render-target role;
- floating-point filtering is conservative and requires linear filtering for
  both RGBA16F and RGBA32F; and
- compute availability requires the selected queue family to support compute.

This is the lifecycle and capability foundation for later GPU implementations
of paint operations. It does not substitute CPU output under the Vulkan name;
the parity gate will measure Vulkan operation renderers as those operation APIs
land.
