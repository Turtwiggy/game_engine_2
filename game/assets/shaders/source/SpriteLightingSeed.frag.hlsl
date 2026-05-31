Texture2D<float2> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct Input
{
  float4 Data0     : TEXCOORD0;
  float4 Data1     : TEXCOORD1;
  float4 Data2     : TEXCOORD2;
  float4 Data3     : TEXCOORD3;
  float4 Data4     : TEXCOORD4;
};

// float2 F16_V2(float f) { return float2(floor(f * 255.0) / 255.0, frac(f * 255.0)); }

float4 main(Input input) : SV_Target0
{
  float2 v_uv = input.Data0.xy;
  float2 v_sprite_max = input.Data0.zw;
  float2 v_sprite_pos = input.Data1.xy;
  float2 v_sprite_wh = input.Data1.zw;
  float4 v_col = input.Data2;
  float2 v_eao = input.Data3.xy;
  
  float2 tex = Texture.Sample(Sampler, v_uv).rg;

  // return float4(F16_V2(v_uv.x * tex.a), F16_V2(v_uv.y * tex.a));
  return float4(v_uv.x * tex.r, v_uv.y * tex.r, 0.0, 1.0);
}
