struct SpriteData
{
    float3 Position;
    float Rotation;
    float2 Scale;
    float2 IsEmitterAndIsOccluder;
    float TexU, TexV, TexW, TexH;
    float4 Color;
    float2 SpriteMax;
    float2 SpritePos;
    float2 SpriteWH;
    float2 Padding2;
};

struct Output
{
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color : TEXCOORD1;
    float2 SpriteMax: TEXCOORD2; 
    float2 SpritePos: TEXCOORD3;
    float2 SpriteWH: TEXCOORD4;
    float2 IsEmitterAndIsOccluder: TEXCOORD5;
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
    output.TexCoord = texcoord[vert];
    output.Color = sprite.Color;
    output.SpriteMax = sprite.SpriteMax;
    output.SpritePos = sprite.SpritePos;
    output.SpriteWH = sprite.SpriteWH;
    output.IsEmitterAndIsOccluder = sprite.IsEmitterAndIsOccluder;
    return output;
}
