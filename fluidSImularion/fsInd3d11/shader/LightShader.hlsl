#include "LightHelper.hlsl"

cbuffer cbMatrixBufferType : register(b0)
{
    matrix g_mViewProjection;
};

struct VertexIn
{
    float3 Pos : POSITION;
    float2 Tex : TEXCOORD0;
};

struct VertexOut
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0;
};


VertexOut LightShaderVS(VertexIn vIn)
{
    VertexOut vOut;

    vOut.Pos = mul(float4(vIn.Pos, 1.0f), g_mViewProjection);
    vOut.Tex = vIn.Tex;

    return vOut;
}

cbuffer cbLightBuffer : register(b1)
{
    int gPointLightCount;
    int gDirLightCount;
    int gSpotLightCount;
    int padding;
    
    float3 gEyePosW;
    float gSkipLighting;
    
    matrix gCamViewProjInv;
};
StructuredBuffer<DirectionalLight> gDirLight : register(t3);
StructuredBuffer<PointLight> gPointLight : register(t4);
StructuredBuffer<SpotLight> gSpotLight : register(t5);


Texture2D gDiffuseTexture : register(t0);
Texture2D gNormalTexture : register(t1);
Texture2D gDepthTexture : register(t2);

SamplerState samLinear : register(s0);
SamplerState samPoint : register(s1);


float4 LightShaderPS(VertexOut pIn) : SV_Target
{
    float4 finalColor;
    
    float4 diffuse;
    float3 normal;
    float4 specular;
    float3 positionW;
    float shadowFactor;
    int materialIndex;

    diffuse.xyz = gDiffuseTexture.Sample(samLinear, pIn.Tex).xyz;
    normal = gNormalTexture.Sample(samLinear, pIn.Tex).xyz;
    shadowFactor = gNormalTexture.Sample(samLinear, pIn.Tex).w;
    float matFloat = gDiffuseTexture.Sample(samPoint, pIn.Tex).w * 255.0f;
    materialIndex = round(matFloat);

    Material curMat;

    curMat.Ambient = float4(0.1f, 0.1f, 0.1f, 0.1f);
    curMat.Diffuse = float4(1.0f, 1.0f, 1.0f, 1.0f);
    curMat.Specular = float4(0.0f, 0.0f, 0.0f, 0.0f);
    curMat.Reflect = float4(0.0f, 0.0f, 0.0f, 0.0f);
       
    diffuse.w = 1.0f;

	// World pos reconstruction
    float depth = gDepthTexture.Sample(samLinear, pIn.Tex).x;

    float4 H = float4(pIn.Tex.x * 2.0f - 1.0f, (1.0f - pIn.Tex.y) * 2.0f - 1.0f, depth, 1.0f);

    float4 D_transformed = mul(H, gCamViewProjInv);

    positionW = (D_transformed / D_transformed.w).xyz;

	// The toEye vector is used in lighting
    float3 toEye = gEyePosW - positionW;

	// Direction of eye to pixel world position
    float3 eyeDir = positionW - gEyePosW;

	// Cache the distance to the eye from this surface point.
    float distToEye = length(toEye);

	// Normalize
    toEye /= distToEye;

	//--------------------------------------------------
	// Lighting
	//--------------------------------------------------
    finalColor = diffuse;

    float4 ambient_Lights = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 diffuse_Lights = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float4 specular_Lights = float4(0.0f, 0.0f, 0.0f, 0.0f);

    if (gSkipLighting == 1)
        diffuse_Lights = float4(1.0f, 1.0f, 1.0f, 1.0f);

    float4 A, D, S;

    if (gSkipLighting == 0)
    {
		// Begin calculating lights
        for (int i = 0; i < gDirLightCount; ++i)
        {
            ComputeDirectionalLight(curMat, gDirLight[i], normal, toEye, A, D, S);
            ambient_Lights += A * shadowFactor;
            diffuse_Lights += D * shadowFactor;
            specular_Lights += S * shadowFactor;
        }

        for (int j = 0; j < gPointLightCount; ++j)
        {
            ComputePointLight(curMat, gPointLight[j], positionW, normal, toEye, A, D, S);
            ambient_Lights += A;
            diffuse_Lights += D;
            specular_Lights += S;
        }

        for (int k = 0; k < gSpotLightCount; ++k)
        {
			//ComputeSLight_Deferred(specular, gSpotLights[k], positionW, normal, toEye, A, D, S);
            ComputeSpotLight(curMat, gSpotLight[k], positionW, normal, toEye, A, D, S);
            ambient_Lights += A;
            diffuse_Lights += D;
            specular_Lights += S;
        }
    }

    finalColor = float4(diffuse.xyz * (ambient_Lights.xyz + diffuse_Lights.xyz) + specular_Lights.xyz, 1.0f);

	// Tone mapping
    finalColor.xyz = Uncharted2Tonemap(finalColor.xyz);

	// Gamma encode final lit color
    finalColor.xyz = pow(abs(finalColor.xyz), 1.0f / 2.2f);
    return finalColor;
}