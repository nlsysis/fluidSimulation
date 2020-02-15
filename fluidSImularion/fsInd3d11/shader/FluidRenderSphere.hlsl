//--------------------------------------------------------------------------------------
// Particle Rendering in sphere 
// actually use point sprite(use of billoard discard the pixel out the circle)
//--------------------------------------------------------------------------------------

struct Particle
{
    float3 position;
    float3 velocity;
};

struct ParticleDensity
{
    float density;
};

StructuredBuffer<Particle> ParticlesRO : register(t0);
StructuredBuffer<ParticleDensity> ParticleDensityRO : register(t1);

cbuffer cbRenderConstants : register(b0)
{
    matrix g_mViewProjection;
    float g_fParticleSize;
};

cbuffer cbEye : register(b1)
{
    float3 eyePosW;
    float padding;
}

struct VSParticleOut
{
    float3 position : POSITION;
};

struct GSParticleOut
{
    float4 position : SV_Position;
    float2 texcoord : TEXCOORD;
};

Texture2D diffuseMap : register(t2);
SamplerState linearSample : register(s0);

struct PixelOut
{
    float4 Color : SV_Target0;
    float4 Normal : SV_Target1;
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VSParticleOut ParticleVS(uint ID : SV_VertexID)
{
    VSParticleOut Out = (VSParticleOut) 0;
    Out.position = ParticlesRO[ID].position;
    return Out;
}


//--------------------------------------------------------------------------------------
// Particle Geometry Shader
//--------------------------------------------------------------------------------------

static const float2 g_positions[4] = { float2(-1, 1), float2(1, 1), float2(-1, -1), float2(1, -1) };
static const float2 g_texcoords[4] = { float2(0, 1), float2(1, 1), float2(0, 0), float2(1, 0) };

[maxvertexcount(4)]
void ParticleGS(point VSParticleOut In[1], inout TriangleStream<GSParticleOut> SpriteStream)
{
    float3 look = normalize(eyePosW - In[0].position);
    float3 right = normalize(cross(float3(0.0f, 1.0f, 0.0f), look));
    float3 up = cross(look, right);
    
    [unroll]
    for (int i = 0; i < 4; i++)
    {
        GSParticleOut Out = (GSParticleOut) 0;
        float4 position = float4(In[0].position, 1) + g_fParticleSize * float4(g_positions[i], 0, 0) * float4(right, 1.0f) * float4(up,1.0f);
        Out.position = mul(position, g_mViewProjection);
        Out.texcoord = g_texcoords[i];
        SpriteStream.Append(Out);
    }
    SpriteStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

PixelOut ParticlePS(GSParticleOut In)
{
    PixelOut pOut;
    
    pOut.Color = diffuseMap.Sample(linearSample, In.texcoord);
   // clip(pOut.Color.w-0.05f);
    clip(pOut.Color.w < 0.9f ? -1 : 1);
    
    //calculate eye-space sphere normal from texture coordinates
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    normal.xy = In.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float r2 = dot(normal.xy, normal.xy);
    if (r2 > 1.0f)
        discard;                //kill pixel outside circle
    
    //calculate depth
    //float4 pixelPos = float4((eyePosW + normal * g_fParticleSize), 1.0f);
    //float4 clipSpacePos = mul(pixelPos, g_mViewProjection);
    //float fragDepth = clipSpacePos.z / clipSpacePos.w;
    
    //float diffuse = max(0.0, dot(normal, lightDir));
    //float4 = diffuse * color;
    
    normal.z = sqrt(1.0 - r2);
    pOut.Normal = float4(normal, 1.0f);
   
    return pOut;
}

