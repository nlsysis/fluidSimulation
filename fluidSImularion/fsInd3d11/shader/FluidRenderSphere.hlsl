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
    float4 color : COLOR;
};

struct GSParticleOut
{
    float4 position : SV_Position;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

Texture2D diffuseMap : register(t2);
SamplerState linearSample : register(s0);

//--------------------------------------------------------------------------------------
// Visualization Helper
//--------------------------------------------------------------------------------------

static const float4 Rainbow[5] =
{
    float4(1, 0, 0, 1), // red
    float4(1, 1, 0, 1), // orange
    float4(0, 1, 0, 1), // green
    float4(0, 1, 1, 1), // teal
    float4(0, 0, 1, 1), // blue
};

float4 VisualizeNumber(float n)
{
    return lerp(Rainbow[floor(n * 4.0f)], Rainbow[ceil(n * 4.0f)], frac(n * 4.0f));
}

float4 VisualizeNumber(float n, float lower, float upper)
{
    return VisualizeNumber(saturate((n - lower) / (upper - lower)));
}

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VSParticleOut ParticleVS(uint ID : SV_VertexID)
{
    VSParticleOut Out = (VSParticleOut) 0;
    Out.position = ParticlesRO[ID].position;
    Out.color = VisualizeNumber(ParticleDensityRO[ID].density, 0.01f, 1.3f);
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
        float4 position = float4(In[0].position, 1) + g_fParticleSize * g_positions[i].x * float4(up, 0.0f) + g_fParticleSize * g_positions[i].y * float4(right, 0.0f);
        Out.position = mul(position, g_mViewProjection);
        Out.color = In[0].color;
        Out.texcoord = g_texcoords[i];
        SpriteStream.Append(Out);
    }
    SpriteStream.RestartStrip();
}

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 ParticlePS(GSParticleOut In) : SV_TARGET
{
    float4 finalColor;
    finalColor = diffuseMap.Sample(linearSample, In.texcoord);
    clip(finalColor.w < 0.9f ? -1 : 1);
    //calculate eye-space sphere normal from texture coordinates
    float3 normal = float3(0.0f, 0.0f, 0.0f);
    normal.xy = In.texcoord * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float r2 = dot(normal.xy, normal.xy);
    if (r2 > 1.0f)
        discard; //kill pixel outside circle
    
    finalColor.xyz = In.color.xyz * finalColor.xyz;
    return finalColor;
}

