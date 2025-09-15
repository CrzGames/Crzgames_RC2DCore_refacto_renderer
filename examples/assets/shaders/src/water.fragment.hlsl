// ============== water.fragment.hlsl ==============
// Conventions SDL GPU (DXIL/DXBC):
// - Fragment: textures/samplers -> space2, uniforms -> space3
// - Un seul sampler attendu (t0/s0), un seul UBO (b0)

cbuffer Context : register(b0, space3) {
    float time;           // secondes
    float2 resolution;    // largeur, hauteur du render target courant
    float strength;       // intensité du warping (0.0 - 0.2 conseillé)
    float padding;        // alignement 16B (par prudence)
};

Texture2D    u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput {
    float4 v_color : COLOR0;
    float2 v_uv    : TEXCOORD0;
};

struct PSOutput {
    float4 o_color : SV_Target;
};

static const float PI = 3.14159265f;

PSOutput main(PSInput input) {
    PSOutput o;

    // UV animés (ripple léger)
    float2 uv = input.v_uv;
    float wave1 = sin((uv.y * 10.0 + time * 1.2) * PI) * 0.002;
    float wave2 = sin((uv.x * 14.0 + time * 0.8) * PI) * 0.0025;
    uv += strength * float2(wave1, wave2);

    // Échantillonnage + léger boost de saturation
    float4 c = u_texture.Sample(u_sampler, uv) * input.v_color;

    // Optionnel: petite variation chromatique
    float wiggle = 0.005 * sin(time * 2.0 + uv.x * 25.0 + uv.y * 25.0);
    c.rg += wiggle;

    o.o_color = c;
    return o;
}
