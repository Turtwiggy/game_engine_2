Texture2D<float4> Texture : register(t0, space2);
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

float2 F16_V2(float f) { return float2(floor(f * 255.0) / 255.0, frac(f * 255.0)); }

float2 main(Input input) : SV_Target0
{
  float2 v_uv = input.TexCoord;
  float4 tex = Texture.Sample(Sampler, v_uv);

  // if(tex.a > 0.0){
    //   return float4(1.0, 1.0, 1.0, 1.0);
  // }
  // else {
    //   return float4(0.0, 0.0, 0.0, 1.0);
  // }

  // return float4(F16_V2(v_uv.x * tex.a), F16_V2(v_uv.y * tex.a));
  return float2(v_uv.x * tex.a, v_uv.y * tex.a);
}
