cbuffer cbPerObject
{
  float4x4 u_modelViewProj;
};

struct VS_OUT
{
  float4 position : SV_POSITION;
  float3 texCoord : TEXCOORD0;
};

VS_OUT main(float3 a_position : POSITION, float3 a_texcoord0 : TEXCOORD0)
{
  VS_OUT vsOut;
  vsOut.position = mul(u_modelViewProj, float4(a_position, 1.0));
  vsOut.texCoord = a_texcoord0;
  return vsOut;
}
