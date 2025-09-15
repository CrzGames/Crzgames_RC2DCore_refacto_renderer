cbuffer type_OceanUniforms : register(b0)
{
    float OceanUniforms_time : packoffset(c0);
    float OceanUniforms_waveSpeed : packoffset(c0.y);
    float OceanUniforms_waveAmplitude : packoffset(c0.z);
    float OceanUniforms_waveFrequency : packoffset(c0.w);
    float2 OceanUniforms_scrollSpeed : packoffset(c1);
};

Texture2D<float4> waterTexture : register(t0);
SamplerState waterSampler : register(s0);

static float2 in_var_TEXCOORD0;
static float4 out_var_SV_Target;

struct SPIRV_Cross_Input
{
    float2 in_var_TEXCOORD0 : TEXCOORD0;
};

struct SPIRV_Cross_Output
{
    float4 out_var_SV_Target : SV_Target0;
};

void frag_main()
{
    float _44 = OceanUniforms_time * OceanUniforms_waveSpeed;
    out_var_SV_Target = waterTexture.Sample(waterSampler, float2((in_var_TEXCOORD0.x + (sin(_44 + (in_var_TEXCOORD0.y * OceanUniforms_waveFrequency)) * OceanUniforms_waveAmplitude)) + ((sin((_44 * 1.5f) + (((in_var_TEXCOORD0.x + in_var_TEXCOORD0.y) * OceanUniforms_waveFrequency) * 2.0f)) * OceanUniforms_waveAmplitude) * 0.300000011920928955078125f), (in_var_TEXCOORD0.y + ((cos((_44 * 0.699999988079071044921875f) + ((in_var_TEXCOORD0.x * OceanUniforms_waveFrequency) * 1.2000000476837158203125f)) * OceanUniforms_waveAmplitude) * 0.5f)) + ((cos((_44 * 1.2000000476837158203125f) + (((in_var_TEXCOORD0.x - in_var_TEXCOORD0.y) * OceanUniforms_waveFrequency) * 1.5f)) * OceanUniforms_waveAmplitude) * 0.20000000298023223876953125f)) + (OceanUniforms_scrollSpeed * OceanUniforms_time));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    in_var_TEXCOORD0 = stage_input.in_var_TEXCOORD0;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.out_var_SV_Target = out_var_SV_Target;
    return stage_output;
}
