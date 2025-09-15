cbuffer type_OceanUniforms : register(b0)
{
    float OceanUniforms_time : packoffset(c0);
    float OceanUniforms_wave_speed : packoffset(c0.y);
    float OceanUniforms_wave_amplitude : packoffset(c0.z);
    float OceanUniforms_wave_frequency : packoffset(c0.w);
};

Texture2D<float4> u_texture : register(t0);
SamplerState u_sampler : register(s0);

static float4 in_var_COLOR0;
static float2 in_var_TEXCOORD0;
static float4 out_var_SV_Target;

struct SPIRV_Cross_Input
{
    float4 in_var_COLOR0 : TEXCOORD0;
    float2 in_var_TEXCOORD0 : TEXCOORD1;
};

struct SPIRV_Cross_Output
{
    float4 out_var_SV_Target : SV_Target0;
};

void frag_main()
{
    float2 _43 = in_var_TEXCOORD0;
    float _47 = _43.x * OceanUniforms_wave_frequency;
    float _52 = OceanUniforms_time * OceanUniforms_wave_speed;
    float2 _68 = _43;
    _68.y = _43.y + ((sin(_47 + _52) * OceanUniforms_wave_amplitude) + ((cos(((_43.y * OceanUniforms_wave_frequency) * 0.5f) + (_52 * 0.800000011920928955078125f)) * OceanUniforms_wave_amplitude) * 0.5f));
    float4 _73 = u_texture.Sample(u_sampler, _68) * in_var_COLOR0;
    float3 _83 = lerp((_73.xyz * ((sin((_47 * 2.0f) + (_52 * 1.5f)) * 0.20000000298023223876953125f) + 0.800000011920928955078125f)).xyz, float3(0.20000000298023223876953125f, 0.4000000059604644775390625f, 0.800000011920928955078125f), 0.300000011920928955078125f.xxx);
    out_var_SV_Target = float4(_83.x, _83.y, _83.z, _73.w);
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    in_var_COLOR0 = stage_input.in_var_COLOR0;
    in_var_TEXCOORD0 = stage_input.in_var_TEXCOORD0;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.out_var_SV_Target = out_var_SV_Target;
    return stage_output;
}
