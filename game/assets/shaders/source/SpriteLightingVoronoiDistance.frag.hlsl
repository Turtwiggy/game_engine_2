Texture2D<float4> Texture_Jumpflood : register(t0, space2);
SamplerState Sampler_Jumpflood : register(s0, space2);

Texture2D<float4> Texture_EmittersAndOccluders : register(t1, space2);
SamplerState Sampler_EmittersAndOccluders : register(s1, space2);

struct Input
{
  float2 TexCoord : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float2 SpriteMax: TEXCOORD2; 
  float2 SpritePos: TEXCOORD3;
  float2 SpriteWH: TEXCOORD4;
  float2 IsEmitterAndIsOccluder: TEXCOORD5;
};

float V2_F16(float2 v) { return v.x + (v.y / 255.0); }
float2 F16_V2(float f) { return float2(floor(f * 255.0) / 255.0, frac(f * 255.0)); }

float2 main(Input input) : SV_Target0
{
  float2 v_uv = input.TexCoord;
  float4 jfuv = Texture_Jumpflood.Sample(Sampler_Jumpflood, v_uv);

  // float2 jumpflood = float2(V2_F16(jfuv.rg),V2_F16(jfuv.ba));
  // float2 dist = F16_V2(distance(v_uv, jumpflood));
  float2 ScreenWH = float2(1280, 720); // TODO: replace this
  float dist = distance(jfuv.xy * ScreenWH, v_uv * ScreenWH);

  float4 eao = Texture_EmittersAndOccluders.Sample(Sampler_EmittersAndOccluders, v_uv);

  // note: *0 because calculating the distance as inside the occluder/emitter as -1 is wrong
  float dist_sign = eao.r > 0.0 ? 0.0 : 1.0;
  
  return float2(dist, dist_sign);
}
