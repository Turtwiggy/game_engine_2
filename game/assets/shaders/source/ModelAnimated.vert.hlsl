cbuffer UniformBlock : register(b0, space1)
{
  float4x4 MVPMatrix : packoffset(c0);
};

struct Input
{
  int4 BoneIds : TEXCOORD0;
  float4 Weights : TEXCOORD1;
  float4 Position : TEXCOORD2;
  float2 UV : TEXCOORD3;
  // float3 Normal : TEXCOORD1;
};

struct Output
{
  float2 TexCoord : TEXCOORD0;
  float4 Position : SV_Position;
};

struct BoneData
{
  float4x4 bone_matrix;
};
StructuredBuffer<BoneData> DataBuffer : register(t0, space0);

Output main(Input input)
{
  const int max_bones = 100;
  const int max_bone_influence = 4;
  float4 total_pos = float4(0.0f, 0.0f, 0.0f, 0.0f);

  /*
  // pack bone ids & weights into arrays for simpler indexing
  int boneIds[4] = { (int)input.BoneIds.x, (int)input.BoneIds.y, (int)input.BoneIds.z, (int)input.BoneIds.w };
  float weights[4] = { input.Weights.x, input.Weights.y, input.Weights.z, input.Weights.w };

  for(int i = 0; i < max_bone_influence; i++){

    int bone_id = boneIds[i];
    if(bone_id == -1)
    continue;
    if(bone_id >= max_bones){
      // total_pos = float4(input.Position.xyz, 1.0f);
      break;
    }

    float4x4 bone_matrix = DataBuffer[bone_id].bone_matrix;
    float4 local_pos = mul(bone_matrix, float4(input.Position.xyz, 1.0f));
    total_pos += local_pos * weights[i];
  }

  float epsilon = 1e-5;
  if (all(abs(total_pos) < epsilon))
  total_pos = float4(input.Position.xyz, 1.0f);
  */

  Output output;
  output.TexCoord = input.UV;
  output.Position = mul(MVPMatrix, float4(input.Position.xyz, 1.0f));

  return output;
}
