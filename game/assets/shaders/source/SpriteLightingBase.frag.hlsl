// Texture2D<float4> Texture : register(t0, space2);
// SamplerState Sampler : register(s0, space2);

struct Input
{
  float2 TexCoord : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float2 SpriteMax: TEXCOORD2; 
  float2 SpritePos: TEXCOORD3;
  float2 SpriteWH: TEXCOORD4;
  float2 IsEmitterAndIsOccluder: TEXCOORD5;
};

float2 main(Input input) : SV_Target0
{
  float isEmitter = input.IsEmitterAndIsOccluder.x;
  float isOccluder = input.IsEmitterAndIsOccluder.y;
  float2 v_uv = input.TexCoord;

  // return float4(v_uv.x, v_uv.y, 0.0, 1.0);
  // if(isEmitter == 1.0){
    //   return float4(0.0, 1.0, 0.0, 1.0);
  // }

  if(isOccluder == 1.0){
    return float2(1.0, 1.0);
  }

  // return float4(v_uv.x, v_uv.y, 0.0, 1.0);
  return float2(0.0, 0.0);
}
