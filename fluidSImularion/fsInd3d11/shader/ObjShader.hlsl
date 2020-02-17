#include "LightHelper.hlsl"

SamplerState SampleType : register(s0);
Texture2D ObjTexture : register(t0);
StructuredBuffer<DirectionalLight> gDirLight : register(t1);

cbuffer cbMatrix : register(b0)
{
    matrix mvp;
    matrix world;
    matrix worldInvTranspose;
};
 
cbuffer cbPerFrame : register(b1)
{
    float3 gEyePosW;
    float padding;
};

cbuffer cbMaterial : register(b2)
{
    Material gMaterial;
}

struct VertexIn
{
    float3 pos : POSITIONT;
    float2 Tex : TEXCOORD0;
    float3 Normal : NORMAL;
};

struct VertexOut
{
    float4 Pos : SV_Position;
    float2 Tex : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 Pos_W : POSITIONT;
};

VertexOut VS(VertexIn In)
{
    VertexOut Out;
    Out.Pos = mul(float4(In.pos, 1.0f), mvp);
    Out.Pos_W = mul(float4(In.pos, 1.0f), world).xyz;
    Out.normal = mul(In.Normal, (float3x3) worldInvTranspose);

    Out.normal = normalize(Out.normal);
    Out.Tex = In.Tex;

    return Out;
}

float4 PS(VertexOut In) : SV_Target
{
    float3 toEyeW = normalize(gEyePosW -In.Pos_W);

	// Start with a sum of zero. 
    float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.
    float4 A, D, S;

    ComputeDirectionalLight(gMaterial, gDirLight[0], In.normal, toEyeW, A, D, S);
    ambient += A;
    diffuse += D;
    spec += S;
    
    float4 tex = ObjTexture.Sample(SampleType, In.Tex);
    float4 litColor = tex * (ambient + diffuse) + spec;
    
    litColor.a = gMaterial.Diffuse.a * tex.a;
    return litColor;
}
