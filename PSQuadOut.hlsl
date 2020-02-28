Texture2D shaderTexture;
SamplerState SampleType;

struct VSQuadOut
{
  float4 position : SV_Position;
  float2 texcoord : TexCoord;
};

float4 PSQuadOut(VSQuadOut input) : SV_TARGET
{
  return shaderTexture.Sample(SampleType, input.texcoord);
}
