struct SpriteData
{
    float3 Position;
    float Rotation;
    float2 Scale;
    float2 IsEmitterAndIsOccluder;
    float TexU, TexV, TexW, TexH;
    float4 Colour;
    float2 SpriteMax;
    float2 SpritePos;
    float2 SpriteWH;
    float SpriteSheetIdx;
    float p4;
};

struct Output
{
    float4 Position : SV_Position;
    float4 Data0     : TEXCOORD0;  // xy=TexCoord,  zw=SpriteMax
    float4 Data1     : TEXCOORD1;  // xy=SpritePos, zw=SpriteWH
    float4 Data2     : TEXCOORD2;  // xyzw=Colour
    float4 Data3     : TEXCOORD3;  // xy=IsEmitterAndIsOccluder, zw=unused
    float4 Data4     : TEXCOORD4;  // spriteidx, sampleridx, padding3, padding4
};

StructuredBuffer<SpriteData> DataBuffer : register(t0, space0);

cbuffer UniformBlock : register(b0, space1)
{
    float4x4 ViewProjectionMatrix : packoffset(c0);
};

static const uint triangleIndices[6] = {0, 1, 2, 3, 2, 1};
static const float2 vertexPos[4] = {
    {0.0f, 0.0f},
    {1.0f, 0.0f},
    {0.0f, 1.0f},
    {1.0f, 1.0f}
};

Output main(uint id : SV_VertexID)
{
    uint spriteIndex = id / 6; // divide by 6 (number of verts)
    uint vert = triangleIndices[id % 6];
    SpriteData sprite = DataBuffer[spriteIndex];

    float2 texcoord[4] = {
        {sprite.TexU,               sprite.TexV              },
        {sprite.TexU + sprite.TexW, sprite.TexV              },
        {sprite.TexU,               sprite.TexV + sprite.TexH},
        {sprite.TexU + sprite.TexW, sprite.TexV + sprite.TexH}
    };

    float c = cos(sprite.Rotation);
    float s = sin(sprite.Rotation);

    float2 coord = vertexPos[vert];
    coord *= sprite.Scale;
    float2x2 rotation = {c, s, -s, c};
    coord = mul(coord, rotation);

    float3 world_pos = float3(coord + sprite.Position.xy, sprite.Position.z);

    Output output;
    output.Position = mul(ViewProjectionMatrix, float4(world_pos, 1.0f));
    output.Data0.xy = texcoord[vert];
    output.Data0.zw = sprite.SpriteMax;
    output.Data1.xy = sprite.SpritePos; 
    output.Data1.zw = sprite.SpriteWH; 
    output.Data2 = sprite.Colour;
    output.Data3.rg = sprite.IsEmitterAndIsOccluder;
    output.Data4.r = sprite.SpriteSheetIdx;
    return output;
}
