Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

cbuffer OceanUniforms : register(b0, space3) {
    float time;
    float wave_speed;
    float wave_amplitude;
    float wave_frequency;
};

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv : TEXCOORD0;
};

struct PSOutput {
    float4 o_color : SV_Target;
};

static const float PI = 3.14159265f;

PSOutput main(PSInput input) {
    PSOutput output;

    float2 uv = input.v_uv;

    // Déformation des UV pour simuler les vagues
    float wave = sin(uv.x * wave_frequency + time * wave_speed) * wave_amplitude;
    wave += cos(uv.y * wave_frequency * 0.5 + time * wave_speed * 0.8) * wave_amplitude * 0.5;
    uv.y += wave;

    // Échantillonner la texture avec les UV déformés
    float4 color = u_texture.Sample(u_sampler, uv) * input.v_color;

    // Ajouter un effet de reflet
    float reflection = sin(uv.x * wave_frequency * 2.0 + time * wave_speed * 1.5) * 0.2 + 0.8;
    color.rgb *= reflection;

    // Teinte bleutée pour l'eau
    color.rgb = lerp(color.rgb, float3(0.2, 0.4, 0.8), 0.3);

    output.o_color = color;
    return output;
}