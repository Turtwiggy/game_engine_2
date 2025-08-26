float4 main(float4 Color : TEXCOORD0) : SV_Target0
{
    // show a magenta pink to indicate error
    return float4(204/255.0f, 51/255.0f, 139/255.0f, 1.0f);
}
