cbuffer cbPerObject
{
    matrix gWorldViewProj;
};

struct VertexIn
{
    float3 PosL : POSITION;
  //  float3 Normal : NORMAL;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
	// Transform to homogeneous clip space.
    matrix mvp = transpose(gWorldViewProj);
    vout.PosH = mul(float4(vin.PosL, 1.0f), mvp);
	
    vin.Color.a = 0.05f;
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}


float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;
}