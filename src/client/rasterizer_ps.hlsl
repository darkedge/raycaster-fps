struct BgfxSampler2DArray
{
  SamplerState m_sampler;
  Texture2DArray m_texture;
};

uniform SamplerState s_TextureArraySampler : register(s[0]);
uniform Texture2DArray s_TextureArrayTexture : register(t[0]);
static BgfxSampler2DArray s_TextureArray = { s_TextureArraySampler, s_TextureArrayTexture };

float4 bgfxTexture2DArrayLod(BgfxSampler2DArray _sampler, float3 _coord, float _lod)
{
  return _sampler.m_texture.SampleLevel(_sampler.m_sampler, _coord, _lod);
}

void main(float4 gl_FragCoord : SV_POSITION, float3 v_texcoord0 : TEXCOORD0, out float4 bgfx_FragData0 : SV_TARGET0)
{
  v_texcoord0.y  = (1.0 - v_texcoord0.y);
  bgfx_FragData0 = bgfxTexture2DArrayLod(s_TextureArray, v_texcoord0, 0);
}
