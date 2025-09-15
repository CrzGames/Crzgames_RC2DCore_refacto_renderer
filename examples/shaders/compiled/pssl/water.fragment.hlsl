cbuffer type_Context : register(b0)
{
    float4 Context_params0 : packoffset(c0);
    float4 Context_params1 : packoffset(c1);
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
    float2 _69 = frac((in_var_TEXCOORD0 + float2((Context_params0.x * 0.0199999995529651641845703125f) * Context_params1.z, (Context_params0.x * 0.0130000002682209014892578125f) * Context_params1.z)) * Context_params0.w);
    float _74 = _69.x;
    float _80 = _69.y;
    float _85 = sin(((_80 * 14.0f) - (Context_params0.x * 0.89999997615814208984375f)) * 3.1415927410125732421875f);
    float _86 = _74 + _80;
    float4 _103 = u_texture.Sample(u_sampler, frac(_69 + ((float2(sin(((_74 * 10.0f) + (Context_params0.x * 1.2000000476837158203125f)) * 3.1415927410125732421875f) - _85, _85 + sin(((_86 * 8.0f) + (Context_params0.x * 0.64999997615814208984375f)) * 3.1415927410125732421875f)) * 0.5f) * (((1.0f.xx / max(Context_params1.xy, 1.0f.xx)) * Context_params0.z) * Context_params0.y)))) * in_var_COLOR0;
    float2 _111 = _103.xy + (0.008000000379979610443115234375f * sin((Context_params0.x * 1.7000000476837158203125f) + (_86 * 20.0f))).xx;
    out_var_SV_Target = float4(_111.x, _111.y, _103.z, _103.w);
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
