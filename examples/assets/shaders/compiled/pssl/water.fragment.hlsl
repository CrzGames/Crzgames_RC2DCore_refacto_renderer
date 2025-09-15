cbuffer type_Context : register(b0)
{
    float Context_time : packoffset(c0);
    float2 Context_resolution : packoffset(c0.y);
    float Context_strength : packoffset(c0.w);
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
    float2 _61 = in_var_TEXCOORD0 + (float2(sin(((in_var_TEXCOORD0.y * 10.0f) + (Context_time * 1.2000000476837158203125f)) * 3.1415927410125732421875f) * 0.00200000009499490261077880859375f, sin(((in_var_TEXCOORD0.x * 14.0f) + (Context_time * 0.800000011920928955078125f)) * 3.1415927410125732421875f) * 0.0024999999441206455230712890625f) * Context_strength);
    float4 _66 = u_texture.Sample(u_sampler, _61) * in_var_COLOR0;
    float2 _78 = _66.xy + (0.004999999888241291046142578125f * sin(((Context_time * 2.0f) + (_61.x * 25.0f)) + (_61.y * 25.0f))).xx;
    out_var_SV_Target = float4(_78.x, _78.y, _66.z, _66.w);
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
