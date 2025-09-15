// water.fragment.hlsl
// Fragment shader for animating an ocean tile with simple wave distortion.
// This assumes the input texture is a seamless water tile (e.g., turquoise with subtle patterns).
// Distorts UVs using multiple sine waves for a wavy, flowing effect similar to Seafight's ocean.
// Requires a uniform buffer for time and parameters.

// Register bindings as per SDL_GPU docs for fragment shaders in HLSL:
// - Textures: t[n], space2 (sampled textures first)
// - Samplers: s[n], space2
// - Uniform buffers: b[n], space3

Texture2D waterTexture : register(t0, space2);  // The ocean tile texture (e.g., tile.png)
SamplerState waterSampler : register(s0, space2);  // Linear/repeat sampler for tiling

// Uniform buffer for animation parameters
cbuffer OceanUniforms : register(b0, space3)
{
    float time;          // Current time in seconds (for animation)
    float waveSpeed;     // Speed of wave movement (e.g., 0.5)
    float waveAmplitude; // Strength of distortion (e.g., 0.02)
    float waveFrequency; // Frequency of waves (e.g., 5.0)
    float2 scrollSpeed;  // Horizontal/vertical scrolling for flow (e.g., float2(0.1, 0.05))
};

// Input from vertex shader: assumes simple pass-through with TEXCOORD0 for UVs.
// In a full pipeline, this would come from the vertex shader output.
struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;  // Texture coordinates (0-1, but can be >1 for tiling)
};

// Entry point: main (as per SDL docs for DXIL/SPIR-V)
float4 main(PSInput input) : SV_Target
{
    // Animate UVs with multiple layered sine waves for natural wave effect
    float2 distortedUV = input.uv;

    // Layer 1: Horizontal waves
    distortedUV.x += sin(time * waveSpeed + input.uv.y * waveFrequency) * waveAmplitude;
    distortedUV.y += cos(time * waveSpeed * 0.7 + input.uv.x * waveFrequency * 1.2) * waveAmplitude * 0.5;

    // Layer 2: Diagonal waves for more complexity (like small ripples)
    distortedUV.x += sin(time * waveSpeed * 1.5 + (input.uv.x + input.uv.y) * waveFrequency * 2.0) * waveAmplitude * 0.3;
    distortedUV.y += cos(time * waveSpeed * 1.2 + (input.uv.x - input.uv.y) * waveFrequency * 1.5) * waveAmplitude * 0.2;

    // Add subtle scrolling for overall flow (mimics current in Seafight)
    distortedUV += time * scrollSpeed;

    // Sample the texture with distorted UVs (repeat wrapping handles tiling)
    float4 color = waterTexture.Sample(waterSampler, distortedUV);

    // Optional: Add a slight tint or foam effect (e.g., brighten peaks)
    // float foam = saturate(sin(distortedUV.x * 10.0 + time) * 0.5 + 0.5);  // Simple foam simulation
    // color.rgb += float3(0.1, 0.1, 0.1) * foam;  // Add white foam highlights

    return color;
}