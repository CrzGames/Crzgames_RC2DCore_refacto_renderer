// Struct d'entrée pour matcher la sortie du vertex shader par défaut de SDL3
struct PSInput {
    float4 v_color : COLOR0;  // Couleur du vertex (blanche par défaut)
    float2 v_uv : TEXCOORD0;  // Coordonnées UV
};

Texture2D u_texture : register(t0, space2);
SamplerState u_sampler : register(s0, space2);

float4 main(PSInput input) : SV_TARGET {
    return u_texture.Sample(u_sampler, input.v_uv);
}