
Texture2D<float4> Screen_SpriteTexture : register(t0, space2);
SamplerState SpriteSampler : register(s0, space2);

Texture2D<float2> Scene_DistanceTexture : register(t1, space2);
SamplerState DistanceSampler : register(s1, space2);

// Texture2D<float4> Scene_EmittersAndOccluders : register(t2, space2);
// SamplerState EmittersAndOccludersSampler : register(s2, space2);

// https://www.reddit.com/r/sdl/comments/1ir4kq0/heads_up_about_sets_and_bindings_if_youre_using/
struct LightData
{
    float3 position;
    float enabled;
};
StructuredBuffer<LightData> LightBuffer : register(t2, space2);

// https://www.reddit.com/r/sdl/comments/1ir4kq0/heads_up_about_sets_and_bindings_if_youre_using/
cbuffer UBO : register(b0, space3)
{
    float2 MousePos; 
    float2 ScreenWH;
}

struct Input
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
};

float V2_F16(float2 v) { return v.x + (v.y / 255.0); }
float sceneDist(float2 pix) {

    float2 uv = pix / ScreenWH;
    float2 distTex = Scene_DistanceTexture.Sample(DistanceSampler, uv);

    // float dist = V2_F16(distTex.rg);
    float dist = distTex.r;
    float sign = distTex.g;
    return dist * sign;
}

// lighting and shadows

float shadow(float2 p, float2 pos, float radius)
{
    float2 dir = normalize(pos - p);
    float dl = length(p - pos);
    
    // fraction of light visible, starts at one radius (second half added in the end);
    float lf = radius * dl;
    
    // distance traveled
    float dt = 0.01;

    for (int i = 0; i < 64; ++i)
    {				
        // distance to scene at current position
        float sd = sceneDist(p + dir * dt);

        // early out when this ray is guaranteed to be full shadow
        if (sd < -radius) 
        return 0.0;
        
        // width of cone-overlap at light
        // 0 in center, so 50% overlap: add one radius outside of loop to get total coverage
        // should be '(sd / dt) * dl', but '*dl' outside of loop
        lf = min(lf, sd / dt);
        
        // move ahead
        dt += max(1.0, abs(sd));
        if (dt > dl) break;
    }

    // multiply by dl to get the real projected overlap (moved out of loop)
    // add one radius, before between -radius and + radius
    // normalize to 1 ( / 2*radius)
    lf = clamp((lf*dl + radius) / (2.0 * radius), 0.0, 1.0);
    lf = smoothstep(0.0, 1.0, lf);
    return lf;
}

float4 drawLight(float2 p, float2 pos, float4 color, float dist, float range, float radius)
{
    // distance to light
    float ld = length(p - pos);
    
    // out of range
    if (ld > range) return float4(0.0, 0.0, 0.0, 0.0);
    
    // shadow and falloff
    float shad = shadow(p, pos, radius);
    float fall = (range - ld)/range;
    fall *= fall;
    return (shad * fall) * color;
}

float luminance(float4 col)
{
    return 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
}

void setLuminance(inout float4 col, float lum)
{
    lum /= luminance(col);
    col *= lum;
}

// dist will be a value between 0 and 1
float AO(float dist, float radius, float intensity)
{
    float a = clamp(dist / radius, 0.0, 1.0) - 1.0;
    return 1.0 - (pow(abs(a), 5.0) + 1.0) * intensity + (1.0 - intensity);
    return smoothstep(0.0, 1.0, dist / radius);
}

// https://www.shadertoy.com/view/4dfXDn
float4 main(Input input) : SV_Target0
{    
    // fragCoord : is a float2 that is between 0 > 640 on the X axis and 0 > 360 on the Y axis
    // iResolution : is a float2 with an X value of 640 and a Y value of 360
    float2 v_uv = input.TexCoord;
    float2 viewport_wh = ScreenWH;
    float2 fragCoord = (v_uv * viewport_wh);
    float2 iResolution = viewport_wh;
    float2 center = iResolution.xy * 0.5;
    // float2 p = ((fragCoord - center) * zoom + center + float2(0.5));
    // float2 p = ((fragCoord - center) + center + float2(0.5, 0.5));
    float2 p = fragCoord.xy + float2(0.5, 0.5);
    float2 c = iResolution.xy / 2.0;
    
    float dist = sceneDist(p);
    // float4 hmm = float4(dist/10, dist, dist, 1.0);
    // return hmm;
    
    float4 col = float4(0.0, 0.0, 0.0, 1.0);
    
    // ambient occlusion
    // col = float4(0.3, 0.3, 0.3, 1.0);
    // col *= AO(dist, 40.0f, 0.4f);
    // col = float4(0.3f, 0.3f, 0.3f, 1.0f);
    // col *= 1.0f - AO(dist, 1.0f, 0.8f);

    uint lightCount, stride;
    LightBuffer.GetDimensions(lightCount, stride);

    for(int i = 0; i < lightCount; i++){
        LightData l = LightBuffer[i];

        if(l.enabled < 1.0) continue;

        float2 lightPos = l.position.xy;
        float4 lightCol = float4(1.0, 1.0, 1.0, 1.0);
        setLuminance(lightCol, 1.25);

        col += drawLight(p, lightPos, lightCol, dist, 250.0, 1.0 );

        // Light l0;
        // l0.position = center;
        // l0.colour = float4(1.0, 1.0, 1.0, 1.0);
        // l0.luminance = 1.25;
        // setLuminance(l0.colour, l0.luminance);
        // col += drawLight(p, l0.position, l0.colour, dist, 1000.0, 6.0);
        // col += drawLight(p, l1.position, l1.colour, dist, 250.0, 6.0);
    }

    float4 scene_col = Screen_SpriteTexture.Sample(SpriteSampler, input.TexCoord);
    if(all(scene_col.rgb == 0.0)){
        scene_col = float4(0.3, 0.3, 0.3, 1.0);
    }

    float4 lighting_col = col;

    float4 final_col = scene_col * lighting_col;
    return final_col;

    // float3 normal_tex = Screen_NormalTexture.Sample(NormalSampler, input.TexCoord).xyz;
    // float3 normal = normalize(normal_tex * 2.0 - 1.0); // [0, 1] to [-1, 1]

    // // float3 frag_pos = input.Position.xyz;
    // float3 frag_pos = input.FragPos.xyz;

    // float light_radius = 500.0f; // pixels
    // float3 light_col = float3(1.0, 1.0, 1.0);
    // float3 light_pos = float3(MousePos.x, MousePos.y, 0.0);
    // float3 light_dir = normalize( light_pos - frag_pos );
    // light_dir.y *= -1;

    // // float4 albedo = sprite_col;
    // // float3 diffuse = max(dot(normal_col, light_dir), 0.0f) * albedo * light_col;

    // float dist = distance(frag_pos, light_pos.xyz);
    // float attenuation = 1.0 - smoothstep(0.0, light_radius, dist);
    // // float attenuation = (1.0 - dist / (light_radius * res.x));

    // float3 albedo = sprite_col.rgb;
    // float3 diffuse = max(dot(normal, light_dir), 0.0) * albedo * light_col;
    // float3 ambient = float3(0.7, 0.7, 0.7) * albedo;
    // float3 lighting = (ambient + diffuse) * attenuation;

    // return float4(lighting.xyz, 1.0f);
}
