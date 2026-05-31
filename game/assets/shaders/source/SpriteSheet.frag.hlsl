
Texture2D<float4> SpriteTextures[2] : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

struct Input
{
    float4 Data0     : TEXCOORD0; // uv, sprite_max
    float4 Data1     : TEXCOORD1; // sprite_pos, sprite_wh
    float4 Data2     : TEXCOORD2; // col
    float4 Data3     : TEXCOORD3; // eao
    float4 Data4     : TEXCOORD4;  // spriteidx, padding2, padding3, padding4
};

float4 main(Input input) : SV_Target0
{
    // e.g. spritemap config
    // float px_total = 768;
    // float py_total = 352;
    // float px = 16;
    // float py = 16;
    // float2 SpriteMax; // e.g. 768, 352
    // float2 SpritePos; // e.g. 0, 1
    // float2 SpriteWH;  // e.g. commonly: {1, 1}, {2, 2}

    float2 v_uv = input.Data0.xy;
    float2 v_sprite_max = input.Data0.zw; // e.g. 29, 8
    float2 v_sprite_pos = input.Data1.xy; // e.g. 0, 0
    float2 v_sprite_wh = input.Data1.zw; // e.g. 1, 1
    float4 v_col = input.Data2;
    float2 v_eao = input.Data3.xy;
    float v_spritesheet_idx = input.Data4.r;

    uint tex_w, tex_h;
    SpriteTextures[v_spritesheet_idx].GetDimensions(tex_w, tex_h);
    float2 tex_size = float2(tex_w, tex_h);
    
    // float2 texel_size = rcp(float2(tex_w, tex_h));
    // float2 sprite_pixels = float2(tex_w, tex_h) / v_sprite_max; // e.g. (16, 16)

    float2 sprite_uv = (v_sprite_pos + v_uv * v_sprite_wh) / v_sprite_max;

    // float2 uv = sprite_uv * tex_size;
    // uv = floor(uv) + .5;
    // float2 seam = floor(uv + 0.5);
    // float2 dudv = fwidth(uv);
    // uv = seam + clamp((uv - seam) / dudv, -0.5, 0.5);
    // uv /= tex_size;
    
    float4 sprite_col = v_col * SpriteTextures[v_spritesheet_idx].Sample(SpriteSampler, sprite_uv);
    return sprite_col;
}
