
Texture2D<float4> Screen_SpriteTexture : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

Texture2D<float4> Screen_NormalTexture : register(t1, space2);
SamplerState NormalSampler : register(s1, space2);

// https://www.reddit.com/r/sdl/comments/1ir4kq0/heads_up_about_sets_and_bindings_if_youre_using/
cbuffer UBO : register(b0, space3)
{
    float2 MousePos : packoffset(c0); 
}

struct Input
{
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float4 Position : SV_Position;
};

float4 main(Input input) : SV_Target0
{    
    uint SpriteTextureW, SpriteTextureH;
    Screen_SpriteTexture.GetDimensions(SpriteTextureW, SpriteTextureH);
    float2 res = float2(1280.0f, 720.0f); // todo: replace this

    float4 sprite_col = Screen_SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    float4 normal_tex = Screen_NormalTexture.Sample(NormalSampler, input.TexCoord);
    float4 normal_col = normalize(normal_tex * 2.0 - 1.0);
    float light_radius = 0.5f;
    float3 light_col = float3(1.0, 1.0, 1.0);
    float3 light_pos = float3(MousePos.x, MousePos.y, 0.0);

    float dist = distance(input.Position.xy, light_pos.xy);

    // phong lighting model.
    float3 frag_pos = input.Position.xyz;
    float3 light_dir = normalize(light_pos - frag_pos);
    light_dir.y *= -1.0f;

    float attenuation = (1.0 - dist / (light_radius * res.x));
    float4 albedo = sprite_col;
    float3 diffuse = max(dot(normal_col, light_dir), 0.0f) * albedo * light_col;
    float3 ambient = float3(0.7, 0.7, 0.7) * albedo;
    float3 lighting = (ambient + diffuse) * attenuation;
    
    return float4(lighting.xyz, 1.0f);
}
