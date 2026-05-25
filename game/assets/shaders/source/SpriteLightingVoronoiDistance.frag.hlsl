Texture2D<float2> Texture_Jumpflood : register(t0, space2);
SamplerState Sampler_Jumpflood : register(s0, space2);

Texture2D<float2> Texture_EmittersAndOccluders : register(t1, space2);
SamplerState Sampler_EmittersAndOccluders : register(s1, space2);

struct Input
{
  float4 Data0     : TEXCOORD0;
  float4 Data1     : TEXCOORD1;
  float4 Data2     : TEXCOORD2;
  float4 Data3     : TEXCOORD3;
};

cbuffer UniformBlock : register(b0, space3)
{
  float2 screen_wh;
  float2 padding;
}

float V2_F16(float2 v) { return v.x + (v.y / 255.0); }
float2 F16_V2(float f) { return float2(floor(f * 255.0) / 255.0, frac(f * 255.0)); }

float4 main(Input input) : SV_Target0
// float2 main(Input input) : SV_Target0
{
  float2 v_uv = input.Data0.xy;
  float2 v_sprite_max = input.Data0.zw;
  float2 v_sprite_pos = input.Data1.xy;
  float2 v_sprite_wh = input.Data1.zw;
  float4 v_col = input.Data2;
  float2 v_eao = input.Data3.xy;
  
  float2 jfuv = Texture_Jumpflood.Sample(Sampler_Jumpflood, v_uv).rg;

  // float2 jumpflood = float2(V2_F16(jfuv.rg),V2_F16(jfuv.ba));
  // float2 dist = F16_V2(distance(v_uv, jumpflood));
  float dist = distance(jfuv * screen_wh, v_uv * screen_wh);

  float2 eao = Texture_EmittersAndOccluders.Sample(Sampler_EmittersAndOccluders, v_uv).rg;

  // note: *0 because calculating the distance as inside the occluder/emitter as -1 is wrong
  float dist_sign = eao.r > 0.0 ? 0.0 : 1.0;
  
  return float4(dist, dist_sign, 0.0f, 1.0f);
  // return float2(dist, 1.0);
}
