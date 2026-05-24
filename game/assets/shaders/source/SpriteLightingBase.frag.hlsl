
struct Input
{
  float2 TexCoord : TEXCOORD0;
  float4 Color : TEXCOORD1;
  float2 SpriteMax: TEXCOORD2; 
  float2 SpritePos: TEXCOORD3;
  float2 SpriteWH: TEXCOORD4;
  float2 IsEmitterAndIsOccluder: TEXCOORD5;
};

float4 main(Input input) : SV_Target0
{
  float isEmitter = input.IsEmitterAndIsOccluder.x;
  float isOccluder = input.IsEmitterAndIsOccluder.y;
  float2 v_uv = input.TexCoord;

  return float4(isOccluder, 0.0f, 0.0, 1.0);
}
