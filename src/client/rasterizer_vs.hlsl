uniform float4x4 u_modelViewProj;

struct Output
{
  float4 gl_Position : SV_POSITION;
  float3 v_texcoord0 : TEXCOORD0;
};

Output main(float3 a_position : POSITION, float3 a_texcoord0 : TEXCOORD0)
{
  Output _varying_;
  _varying_.v_texcoord0 = float3(0.0, 0.0, 0.0);
  ;
  {
    _varying_.gl_Position = mul(u_modelViewProj, float4(a_position, 1.0));
    _varying_.v_texcoord0 = a_texcoord0;
  }
  return _varying_;
}
