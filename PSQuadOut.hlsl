Texture2D shaderTexture;
SamplerState SampleType;

struct VSQuadOut
{
	float4 position : SV_Position;
	float2 texcoord : TexCoord;
};

float4 PSQuadOut(VSQuadOut input) : SV_TARGET
{
	float4 textureColor;
	textureColor = shaderTexture.Sample(SampleType, input.texcoord);
	textureColor.x += input.texcoord.x;
	textureColor.y += input.texcoord.y;
	return textureColor;
}
