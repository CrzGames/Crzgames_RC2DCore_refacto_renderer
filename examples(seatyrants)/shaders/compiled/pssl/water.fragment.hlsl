cbuffer type_Context : register(b0)
{
    float4 Context_params0 : packoffset(c0);
    float4 Context_params1 : packoffset(c1);
};

Texture2D<float4> u_texture0 : register(t0);
SamplerState s0 : register(s0);
Texture2D<float4> u_texture1 : register(t1);
SamplerState s1 : register(s1);

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
    float2 _77 = (in_var_TEXCOORD0 + float2((Context_params0.x * 0.0199999995529651641845703125f) * Context_params1.z, (Context_params0.x * 0.0130000002682209014892578125f) * Context_params1.z)) * Context_params0.w;
    float _86 = frac(sin(dot(floor(_77), float2(12.98980045318603515625f, 78.233001708984375f))) * 43758.546875f);
    float _88 = _77.x;
    float _95 = _77.y;
    float _102 = sin((((_95 * 14.0f) - (Context_params0.x * 0.89999997615814208984375f)) * 3.1415927410125732421875f) + (_86 * 4.3982295989990234375f));
    float _103 = _88 + _95;
    float4 _121 = u_texture1.Sample(s1, _77 + ((float2(sin((((_88 * 10.0f) + (Context_params0.x * 1.2000000476837158203125f)) * 3.1415927410125732421875f) + (_86 * 6.283185482025146484375f)) - _102, _102 + sin((((_103 * 8.0f) + (Context_params0.x * 0.64999997615814208984375f)) * 3.1415927410125732421875f) + (_86 * 8.168140411376953125f))) * 0.5f) * (((1.0f.xx / max(Context_params1.xy, 1.0f.xx)) * Context_params0.z) * Context_params0.y))) * in_var_COLOR0;
    float2 _129 = _121.xy + (0.008000000379979610443115234375f * sin((Context_params0.x * 1.7000000476837158203125f) + (_103 * 20.0f))).xx;
    out_var_SV_Target = float4(_129.x, _129.y, _121.z, _121.w);
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
