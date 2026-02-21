cbuffer type_Params : register(b0)
{
    float4 Params_p0 : packoffset(c0);
    float4 Params_p1 : packoffset(c1);
    float4 Params_p2 : packoffset(c2);
    float4 Params_p3 : packoffset(c3);
};

Texture2D<float4> u_tex : register(t1);
SamplerState u_samp : register(s1);

static float2 in_var_TEXCOORD0;
static float4 out_var_SV_Target;

struct SPIRV_Cross_Input
{
    float2 in_var_TEXCOORD0 : TEXCOORD1;
};

struct SPIRV_Cross_Output
{
    float4 out_var_SV_Target : SV_Target0;
};

void frag_main()
{
    float4 _44 = u_tex.Sample(u_samp, in_var_TEXCOORD0 * max(Params_p0.x, 9.9999997473787516355514526367188e-05f));
    out_var_SV_Target = float4(_44.xyz * Params_p3.xyz, _44.w * clamp(Params_p0.y, 0.0f, 1.0f));
}

SPIRV_Cross_Output main(SPIRV_Cross_Input stage_input)
{
    in_var_TEXCOORD0 = stage_input.in_var_TEXCOORD0;
    frag_main();
    SPIRV_Cross_Output stage_output;
    stage_output.out_var_SV_Target = out_var_SV_Target;
    return stage_output;
}
