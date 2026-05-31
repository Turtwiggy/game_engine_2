
struct Input
{
  float4 Data0     : TEXCOORD0;
  float4 Data1     : TEXCOORD1;
  float4 Data2     : TEXCOORD2;
  float4 Data3     : TEXCOORD3;
  float4 Data4     : TEXCOORD4; 
};

float4 main(Input input) : SV_Target0
{
  float2 v_uv = input.Data0.xy;
  float2 v_sprite_max = input.Data0.zw;
  float2 v_sprite_pos = input.Data1.xy;
  float2 v_sprite_wh = input.Data1.zw;
  float4 v_col = input.Data2;
  float2 v_eao = input.Data3.xy;

  float isEmitter = v_eao.x;
  float isOccluder = v_eao.y;

  return float4(isOccluder, 0.0f, 0.0, 1.0);
}
