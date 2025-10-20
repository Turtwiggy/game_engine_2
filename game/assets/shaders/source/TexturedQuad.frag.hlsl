Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

float4 main(float2 TexCoord : TEXCOORD0) : SV_Target0
{
    float4 col = Texture.Sample(Sampler, TexCoord);
    // return float4(TexCoord.x, TexCoord.y, 0.0, 1.0);
    return col;
}
