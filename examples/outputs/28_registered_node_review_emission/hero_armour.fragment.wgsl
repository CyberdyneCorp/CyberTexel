struct CtexFragmentInput {
    @location(0) uv: vec2<f32>,
};

@group(0) @binding(0) var ctex_resource_6c6962726172792f776561722f656467655f6d61736b2e706e67: texture_2d<f32>;
@group(0) @binding(1) var ctex_material_sampler: sampler;

@fragment
fn main(input: CtexFragmentInput) -> @location(0) vec4<f32> {
    // CyberTexel deterministic WGSL expression program
    // material output pbr.base_color
    let ctex_output_s7062722e626173655f636f6c6f72: vec4<f32> = vec4<f32>(5.000000000e-01, 5.000000000e-01, 5.000000000e-01, 1.000000000e+00);
    // material output pbr.opacity
    let ctex_output_s7062722e6f706163697479: f32 = 1.00000000000000000e+00;
    // node wear-tint[2]/ studio.wear.tint[3] output tint
    let ctex_g776561722d74696e74_i2_n3_s74696e74: f32 = (1.00000000000000000e+00 * 5.0);
    // node ctex.group-instance[2] output amount
    let ctex_n2_s616d6f756e74: f32 = ctex_g776561722d74696e74_i2_n3_s74696e74;
    // material output pbr.roughness
    let ctex_output_s7062722e726f7567686e657373: f32 = ctex_n2_s616d6f756e74;
    // material output pbr.metallic
    let ctex_output_s7062722e6d6574616c6c6963: f32 = 0.00000000000000000e+00;
    // material output pbr.normal
    let ctex_output_s7062722e6e6f726d616c: vec3<f32> = vec3<f32>(5.000000000e-01, 5.000000000e-01, 1.000000000e+00);
    // material output pbr.height
    let ctex_output_s7062722e686569676874: f32 = 0.00000000000000000e+00;
    // material output pbr.occlusion
    let ctex_output_s7062722e6f63636c7573696f6e: f32 = 1.00000000000000000e+00;
    // material output pbr.emission
    let ctex_output_s7062722e656d697373696f6e: vec4<f32> = vec4<f32>(0.000000000e+00, 0.000000000e+00, 0.000000000e+00, 1.000000000e+00);
    // material output pbr.subsurface
    let ctex_output_s7062722e73756273757266616365: f32 = 0.00000000000000000e+00;
    return vec4<f32>(ctex_output_s7062722e626173655f636f6c6f72.r, ctex_output_s7062722e626173655f636f6c6f72.g, ctex_output_s7062722e626173655f636f6c6f72.b, ctex_output_s7062722e626173655f636f6c6f72.a * ctex_output_s7062722e6f706163697479);
}
