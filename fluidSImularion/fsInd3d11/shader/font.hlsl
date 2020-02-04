cbuffer PerFameBuffer : register(b0)
{
    row_major matrix gmvpMatrix;
};
cbuffer PixelBuffer : register(b1)
{
    float4 pixelColor;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};
struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

PixelInputType FontVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;

    output.position = mul(input.position, gmvpMatrix);

    output.tex = input.tex;
    return output;
}

float4 FontPixelShader(PixelInputType input) : SV_TARGET
{
    float4 color;
    color = shaderTexture.Sample(SampleType, input.tex);

    if (color.r == 0.0f)
        color.a = 0.0f;
    else
    {
        color.a = 1.0f;
        color = color * pixelColor;
    }
    return color;
}