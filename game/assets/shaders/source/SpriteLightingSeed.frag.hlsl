Texture2D<float2> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);

struct Input
{
  float2 TexCoord : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float2 SpriteMax: TEXCOORD2; 
  float2 SpritePos: TEXCOORD3;
  float2 SpriteWH: TEXCOORD4;
  float2 IsEmitterAndIsOccluder: TEXCOORD5;
};

// float2 F16_V2(float f) { return float2(floor(f * 255.0) / 255.0, frac(f * 255.0)); }

float4 main(Input input) : SV_Target0
{
  float2 v_uv = input.TexCoord;
  float2 tex = Texture.Sample(Sampler, v_uv).rg;

  // return float4(F16_V2(v_uv.x * tex.a), F16_V2(v_uv.y * tex.a));
  return float4(v_uv.x * tex.r, v_uv.y * tex.r, 0.0, 1.0);
}
