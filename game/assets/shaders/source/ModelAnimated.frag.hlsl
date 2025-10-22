
struct Input
{
    float2 TexCoord : TEXCOORD0;
};

float4 main(Input input) : SV_Target0
{
    // return float4(255/255.0f, 200/255.0f, 139/255.0f, 1.0f);
    return float4(input.TexCoord.x, input.TexCoord.y, 0.0f, 1.0f);
}
