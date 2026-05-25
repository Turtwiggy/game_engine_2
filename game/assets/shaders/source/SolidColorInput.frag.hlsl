// Texture2D<float4> Texture : register(t0, space2);
// SamplerState Sampler : register(s0, space2);

struct Input
{
    float4 UV_and_SpriteMax : TEXCOORD0;
    float4 SpritePos_and_SpriteWH : TEXCOORD1;
    float4 Colour : TEXCOORD2;
    float2 IsEmitterAndIsOccluder: TEXCOORD3;
};

float4 main(Input input) : SV_Target0
{
    // float2 v_uv = input.UV_and_SpriteMax.xy;
    float4 v_col = input.Colour;
    // float2 v_sprite_max = input.UV_and_SpriteMax.zw;
    // float2 v_sprite_pos = input.SpritePos_and_SpriteWH.xy;
    // float2 v_sprite_wh = input.SpritePos_and_SpriteWH.zw;
    // float2 v_eao = input.IsEmitterAndIsOccluder;
    
    return v_col;
}
