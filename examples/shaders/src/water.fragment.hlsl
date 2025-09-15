// ============== water.fragment.hlsl ==============
cbuffer Context : register(b0, space3) {
    // params0: x=time(s), y=strength(0..1), z=px_amp(= amplitude en pixels), w=tiling
    float4 params0;
    // params1: x=width, y=height, z=speed (scroll base uv), w=unused
    float4 params1;
};

Texture2D    u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

struct PSInput { float4 v_color : COLOR0; float2 v_uv : TEXCOORD0; };
struct PSOutput { float4 o_color : SV_Target; };
static const float PI = 3.14159265f;

PSOutput main(PSInput input) {
    PSOutput o;

    float  time      = params0.x;
    float  strength  = params0.y;         // 0..1
    float  px_amp    = params0.z;         // amplitude en pixels
    float  tiling    = params0.w;         // nb de répétitions
    float2 resolution= params1.xy;
    float  speed     = params1.z;

    // UV de base qui défilent doucement (donne une impression de courant)
    float2 baseUV = input.v_uv + float2(time * 0.02 * speed, time * 0.013 * speed);

    // Tiling shader-side (reste en [0,1] avec frac -> pas besoin de sampler REPEAT)
    float2 uv = frac(baseUV * tiling);

    // Amplitude UV en unités [0..1]
    float2 invRes = 1.0 / max(resolution, float2(1.0, 1.0));
    float2 ampUV  = px_amp * invRes * strength;

    // 3 ondes (directions/fréquences/vitesses différentes)
    float s1 = sin( (uv.x * 10.0 + time * 1.20) * PI );
    float s2 = sin( (uv.y * 14.0 - time * 0.90) * PI );
    float s3 = sin( ((uv.x+uv.y) * 8.0 + time * 0.65) * PI );

    // Combine et normalise
    float2 offset = float2(s1 - s2, s2 + s3) * 0.5 * ampUV;

    // Echantillonnage avec déformation
    float4 c = u_texture.Sample(u_sampler, frac(uv + offset)) * input.v_color;

    // Un soupçon de “respiration” chromatique (facultatif)
    float breathe = 0.008 * sin(time * 1.7 + (uv.x + uv.y) * 20.0);
    c.rg += breathe;

    o.o_color = c;
    return o;
}
