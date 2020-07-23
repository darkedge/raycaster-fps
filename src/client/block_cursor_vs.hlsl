cbuffer cbPerObject
{
  float4x4 u_modelViewProj;
};

struct VS_OUT
{
  float4 position : SV_POSITION;
};

VS_OUT main(float3 a_position : POSITION)
{
  VS_OUT vsOut;
  vsOut.position = mul(u_modelViewProj, float4(a_position, 1.0));
  return vsOut;
}
