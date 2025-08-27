Texture2D<float4> SpriteTexture : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
};

float4 main(Input input) : SV_Target0
{
    // e.g. spritemap config
    // float px_total = 768;
    // float py_total = 352;
    // float px = 16;
    // float py = 16;
    // desired sprite

    // float2 v_sprite_pos = float2(16,1); // square
    // float2 v_sprite_pos = float2(12,0); // circle
    float2 v_sprite_pos = float2(6,6); // fish

    float2 v_sprite_wh = float2(2, 2);
    float2 v_sprite_max = float2(32, 32);
    float2 v_uv = input.TexCoord;

    float2 sprite_uv = float2(
        (v_sprite_wh.x * v_uv.x) / v_sprite_max.x + v_sprite_pos.x * (1.0f/v_sprite_max.x),
        (v_sprite_wh.y * v_uv.y) / v_sprite_max.y + v_sprite_pos.y * (1.0f/v_sprite_max.y)
    );
    
    // float4 sprite_col = SpriteTexture.Sample(SpriteSampler, v_uv);
    float4 sprite_col = SpriteTexture.Sample(SpriteSampler, sprite_uv);
    return sprite_col;
}
