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

cbuffer UniformBlock : register(b0, space3)
{
  float u_offset : packoffset(c0);
  float2 screen_wh : packoffset(c0.y);
  float pad : packoffset(c0.w);
};

float V2_F16(float2 v) { return v.x + (v.y / 255.0); }

float4 main(Input input) : SV_Target0
{
  float2 v_uv = input.TexCoord;

  // float4 tex = Texture.Sample(Sampler, v_uv);
  // return float4(tex.x, tex.y, 1.0, 1.0);

  float2 offsets[9];
  offsets[0] = float2(-1.0, -1.0);
  offsets[1] = float2(-1.0, 0.0);
  offsets[2] = float2(-1.0, 1.0);
  offsets[3] = float2(0.0, -1.0);
  offsets[4] = float2(0.0, 0.0);
  offsets[5] = float2(0.0, 1.0);
  offsets[6] = float2(1.0, -1.0);
  offsets[7] = float2(1.0, 0.0);
  offsets[8] = float2(1.0, 1.0);

  float closest_dist = 9999999.9;
  float2 closest_pos = float2(0.0, 0.0);

  for(int i = 0; i < 9; i++)
  {
    float2 jump = v_uv + (offsets[i] * ( u_offset / screen_wh ));
    float2 pos = Texture.Sample(Sampler, jump).xy;
    // float2 seedpos = float2(V2_F16(seed.xy), V2_F16(seed.zw));
    float dist = distance(pos, v_uv);

    if(pos.x != 0.0 && pos.y != 0.0 && dist < closest_dist){
      closest_dist = dist;
      closest_pos = pos;
    }
  }

  return float4(closest_pos, 0.0, 1.0);
}
