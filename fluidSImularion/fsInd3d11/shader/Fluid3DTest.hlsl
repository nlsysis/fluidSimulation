//--------------------------------------------------------------------------------------
// Smoothed Particle Hydrodynamics Algorithm Based Upon:
// Particle-Based Fluid Simulation for Interactive Applications
// Matthias M Eler
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Optimized Grid Algorithm Based Upon:
// Broad-Phase Collision Detection with CUDA
// Scott Le Grand
//--------------------------------------------------------------------------------------
struct Particle
{
    float3 position;
    float3 velocity;
};

struct ParticleForces
{
    float3 acceleration;
};

struct ParticleDensity
{
    float density;
};

//cbuffer cbSimulationConstants : register(b0)
//{
//    uint g_iNumParticles;
//    float g_fTimeStep;
//    float g_fSmoothlen;
//    float g_fPressureStiffness;
//    float g_fRestDensity;
//    float g_fDensityCoef;
//    float g_fGradPressureCoef;
//    float g_fLapViscosityCoef;
//    float g_fWallStiffness;
    
//    float4 g_vGravity;
//    float4 g_vGridDim; //cell size
//    float4 g_vGridSize; //grid size
//    float4 g_originPosW;
    
//};

cbuffer cbSimulationConstants : register(b0)
{
    uint g_iNumParticles;
    float3 g_vGravity;
    float g_fTimeStep;
    float g_fSmoothlen;
    float g_fPressureStiffness;
    float g_fRestDensity;
    float g_fDensityCoef;
    float g_fGradPressureCoef;
    float g_fLapViscosityCoef;
    float g_fWallStiffness;
    
    float g_fParticleRadius;
    float3 g_vGridDim; //cell size
    float3 g_vGridSize; //grid size
    float boundaryDampening;
    float4 g_originPosW;
    
};

cbuffer cbPlane : register(b1)
{
    float4 g_vPlanes[6];
};
//--------------------------------------------------------------------------------------
// Fluid Simulation
//--------------------------------------------------------------------------------------

#define SIMULATION_BLOCK_SIZE 256


//--------------------------------------------------------------------------------------
// Structured Buffers
//--------------------------------------------------------------------------------------
RWStructuredBuffer<Particle> ParticlesRW : register(u0);
StructuredBuffer<Particle> ParticlesRO : register(t0);

RWStructuredBuffer<ParticleDensity> ParticlesDensityRW : register(u0);
StructuredBuffer<ParticleDensity> ParticlesDensityRO : register(t1);

RWStructuredBuffer<ParticleForces> ParticlesForcesRW : register(u0);
StructuredBuffer<ParticleForces> ParticlesForcesRO : register(t2);

RWStructuredBuffer<uint2> GridRW : register(u0);
StructuredBuffer<uint2> GridRO : register(t3);

RWStructuredBuffer<uint2> GridIndicesRW : register(u0);
StructuredBuffer<uint2> GridIndicesRO : register(t4);

//--------------------------------------------------------------------------------------
// optimisim Grid Construction
//--------------------------------------------------------------------------------------
// For simplicity, this sample uses a 16-bit hash based on the grid cell and
// a 16-bit particle ID to keep track of the particles while sorting

//Calculates the grid position from input position[0-GRIDSIZE]
int3 CalcGridPos(float3 position)
{
    int3 gridPos;
    float3 originPosW = g_originPosW.xyz;
    float3 cellSize = g_vGridDim.xyz;
    gridPos.x = floor((position.x - originPosW.x) / cellSize.x);
    gridPos.y = floor((position.y - originPosW.y) / cellSize.y);
    gridPos.z = floor((position.z - originPosW.z) / cellSize.z);

    return gridPos;
}

uint CalcGridHash(int3 gridPos)
{
    uint3 gridSize = (uint3) g_vGridSize.xyz;
	// Assume grid size is the power of 2
    gridPos.x = gridPos.x & (gridSize.x - 1);
    gridPos.y = gridPos.y & (gridSize.y - 1);
    gridPos.z = gridPos.z & (gridSize.z - 1);
    return ((gridPos.z * gridSize.y) * gridSize.x) + (gridPos.y * gridSize.x) + gridPos.x;
}


unsigned int GridConstructKey(uint hash)
{
	// Bit pack [-----UNUSED-----][-----HASH-----]
	//                16-bit          16-bit
    return dot(hash, 256);
}

unsigned int GridConstructKeyValuePair(uint hash, uint value)
{
	// Bit pack [----HASH----][-----VALUE------]
	//              16-bit        16-bit
    return dot(uint2(hash, value), uint2(256 * 256, 1)); // 256 * 256
}

unsigned int GridGetKey(unsigned int keyvaluepair)
{
    return (keyvaluepair >> 16);
}

unsigned int GridGetValue(unsigned int keyvaluepair)
{
    return (keyvaluepair & 0xFFFF); // (256 * 256 ) - 1
}

//--------------------------------------------------------------------------------------
// Build Grid
//--------------------------------------------------------------------------------------
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void BuildGridCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x; // Particle ID to operate on
    
    float3 position = ParticlesRO[P_ID].position;
    
    int3 gridPos = CalcGridPos(position);
    uint gridHash = CalcGridHash(gridPos);
    
    GridRW[P_ID] = uint2(gridHash, P_ID);
}

//--------------------------------------------------------------------------------------
// Build Grid Indices
//--------------------------------------------------------------------------------------
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ClearGridIndicesCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    GridIndicesRW[DTid.x] = uint2(0, 0);
}

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void BuildGridIndicesCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    //the grid key value pair is now a sorted list consisting of (grid hash, particle id)
    const unsigned int G_ID = DTid.x; // Grid ID to operate on
    unsigned int G_ID_PREV = (G_ID == 0) ? g_iNumParticles : G_ID;
    G_ID_PREV--;
    unsigned int G_ID_NEXT = G_ID + 1;
    if (G_ID_NEXT == g_iNumParticles)
    {
        G_ID_NEXT = 0;
    }
    
    unsigned int cell = GridRO[G_ID].x;
    unsigned int cell_prev = GridRO[G_ID_PREV].x;
    unsigned int cell_next = GridRO[G_ID_NEXT].x;
    if (cell != cell_prev)
    {
        // I'm the start of a cell
        GridIndicesRW[cell].x = G_ID;
    }
    if (cell != cell_next)
    {
        // I'm the end of a cell
        GridIndicesRW[cell].y = G_ID + 1;
    }
}
//--------------------------------------------------------------------------------------
// Force Calculation
//--------------------------------------------------------------------------------------

float CalculatePressure(float density)
{
    // Implements this equation:
    // Pressure = B * ((rho / rho_0)^y  - 1)
    return g_fPressureStiffness * max(pow(density / g_fRestDensity, 3) - 1, 0);
}

float3 CalculateGradPressure(float r, float P_pressure, float N_pressure, float N_density, float3 diff)
{
    const float h = g_fSmoothlen;
    float avg_pressure = 0.5f * (N_pressure + P_pressure);
    // Implements this equation:
    // W_spkiey(r, h) = 15 / (pi * h^6) * (h - r)^3
    // GRAD( W_spikey(r, h) ) = -45 / (pi * h^6) * (h - r)^2
    // g_fGradPressureCoef = fParticleMass * -45.0f / (PI * fSmoothlen^6)
    return g_fGradPressureCoef * avg_pressure / N_density * (h - r) * (h - r) / r * (diff);
}

float3 CalculateLapVelocity(float r, float3 P_velocity, float3 N_velocity, float N_density)
{
    const float h = g_fSmoothlen;
    float3 vel_diff = (N_velocity - P_velocity);
    // Implements this equation:
    // W_viscosity(r, h) = 15 / (2 * pi * h^3) * (-r^3 / (2 * h^3) + r^2 / h^2 + h / (2 * r) - 1)
    // LAPLACIAN( W_viscosity(r, h) ) = 45 / (pi * h^6) * (h - r)
    // g_fLapViscosityCoef = fParticleMass * fViscosity * 45.0f / (PI * fSmoothlen^6)
    return g_fLapViscosityCoef / N_density * (h - r) * vel_diff;
}

//--------------------------------------------------------------------------------------
// Density Calculation
//--------------------------------------------------------------------------------------
float CalculateDensity(float r_sq)
{
    const float h_sq = g_fSmoothlen * g_fSmoothlen;
    // Implements this equation:
    // W_poly6(r, h) = 315 / (64 * pi * h^9) * (h^2 - r^2)^3
    // g_fDensityCoef = fParticleMass * 315.0f / (64.0f * PI * fSmoothlen^9)
    return g_fDensityCoef * (h_sq - r_sq) * (h_sq - r_sq) * (h_sq - r_sq);
}
//--------------------------------------------------------------------------------------
// Rearrange Particles
//--------------------------------------------------------------------------------------

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void RearrangeParticlesCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int ID = DTid.x; // Particle ID to operate on
    const unsigned int G_ID = GridRO[ID].y;
    ParticlesRW[ID] = ParticlesRO[G_ID];
}

//--------------------------------------------------------------------------------------
// Optimized Grid + Sort Algorithm
//--------------------------------------------------------------------------------------
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void DensityCS_Grid(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x;
    const float h_sq = g_fSmoothlen * g_fSmoothlen;
    float3 P_position = ParticlesRO[P_ID].position;
    
    float density = 0;
    
    //calculate acceleration based on neighbours from the 8 adjacent cells + current cell
    int3 gridPos = CalcGridPos(P_position);
    uint gridHash = CalcGridHash(gridPos);
    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 N_position = gridPos + int3(x, y, z);
                uint N_gridHash = CalcGridHash(N_position);
                
                uint2 start_end = GridIndicesRO[N_gridHash];
                
                for (unsigned int i = start_end.x; i < start_end.y; ++i)
                {
                    float3 neighborPos = ParticlesRO[i].position;
                    float3 diff = neighborPos - P_position;
                    float r_sq = dot(diff, diff);
                    
                    if (r_sq < h_sq)
                    {
                        density += CalculateDensity(r_sq);
                    }

                }
                
            }
        }
    }
    ParticlesDensityRW[P_ID].density = density;
}

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ForceCS_Grid(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x; // Particle ID to operate on
    //Get current particle properties.
    float3 P_position = ParticlesRO[P_ID].position;
    float3 P_velocity = ParticlesRO[P_ID].velocity;
    float P_density = ParticlesDensityRO[P_ID].density;
    float P_pressure = CalculatePressure(P_density);
    
    const float h_sq = g_fSmoothlen * g_fSmoothlen;
    
    float3 acceleration = float3(0.0f, 0.0f, 0.0f);

    //calculate acceleration based on neighbours from the 8 adjacent cells + current cell
    int3 gridPos = CalcGridPos(P_position);
    
    for (int z = -1; z <= 1; ++z)
    {
        for (int y = -1; y <= 1; ++y)
        {
            for (int x = -1; x <= 1; ++x)
            {
                int3 N_position = gridPos + int3(x, y, z);
                uint N_gridHash = CalcGridHash(N_position);

                uint2 start_end = GridIndicesRO[N_gridHash];
                
                for (unsigned int i = start_end.x; i < start_end.y; ++i)
                {
                    if (P_ID != i)
                    {
                        float3 neighborPos = ParticlesRO[i].position;
                        float3 diff = neighborPos - P_position;
                        float r_sq = dot(diff, diff);
                        
                        if (r_sq < h_sq)
                        {
                            float3 neighborVel = ParticlesRO[i].velocity;
                            float neighborDensity = ParticlesDensityRO[i].density;
                            float neighborPressure = CalculatePressure(neighborDensity);
                            float r = sqrt(r_sq);
                        
                         // Pressure Term
                            acceleration += CalculateGradPressure(r, P_pressure, neighborPressure, neighborDensity, diff);
                        
                        // Viscosity Term
                            acceleration += CalculateLapVelocity(r, P_velocity, neighborVel, neighborDensity);
                        }
                    }                                  
                }

            }

        }

    }
    //Update force with the calculate ones
    ParticlesForcesRW[P_ID].acceleration = acceleration / P_density;
    
}

//--------------------------------------------------------------------------------------
// Integration
//--------------------------------------------------------------------------------------

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void IntegrateCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x; // Particle ID to operate on
    
    float3 position = ParticlesRO[P_ID].position;
    float3 velocity = ParticlesRO[P_ID].velocity;
    float3 acceleration = ParticlesForcesRO[P_ID].acceleration;
  
    // Apply the forces from the map walls
    [unroll]
    for (unsigned int i = 0; i < 6; i++)
    {
        float dist = dot(float4(position, 1), g_vPlanes[i]);
        acceleration += min(dist, 0) * -g_fWallStiffness * g_vPlanes[i].xyz;
        //if (g_vPlanes[i].x != 0)
        //{
        //    position.x = g_vPlanes[i].w - g_fTimeStep * velocity.x;
        //}
        //if (g_vPlanes[i].y != 0)
        //{
        //    position.y = g_vPlanes[i].w - g_fTimeStep * velocity.y;
        //}
        //if (g_vPlanes[i].z != 0)
        //{
        //    position.z = g_vPlanes[i].w - g_fTimeStep * velocity.z;
        //}
    }
    
    // Apply gravity
    acceleration += g_vGravity.xyz;
    
    // Integrate
    velocity += g_fTimeStep * acceleration;
    position += g_fTimeStep * velocity;
    
    // Update
    ParticlesRW[P_ID].position = position;
    ParticlesRW[P_ID].velocity = velocity;
}

//--------------------------------------------------------------------------------------
// Simple N^2 Algorithm
//--------------------------------------------------------------------------------------
[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void DensityCS_Simple(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x;
    const float h_sq = g_fSmoothlen * g_fSmoothlen;
    float3 P_position = ParticlesRO[P_ID].position;
    
    float density = 0;
    
    // Calculate the density based on all neighbors
    for (uint N_ID = 0; N_ID < g_iNumParticles; N_ID++)
    {
        float3 N_position = ParticlesRO[N_ID].position;
        
        float3 diff = N_position - P_position;
        float r_sq = dot(diff, diff);
        if (r_sq < h_sq)
        {
            density += CalculateDensity(r_sq);
        }
    }
    
    ParticlesDensityRW[P_ID].density = density;
}

//--------------------------------------------------------------------------------------
// Simple N^2 Algorithm
//--------------------------------------------------------------------------------------

[numthreads(SIMULATION_BLOCK_SIZE, 1, 1)]
void ForceCS_Simple(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    const unsigned int P_ID = DTid.x; // Particle ID to operate on
    
    float3 P_position = ParticlesRO[P_ID].position;
    float3 P_velocity = ParticlesRO[P_ID].velocity;
    float P_density = ParticlesDensityRO[P_ID].density;
    float P_pressure = CalculatePressure(P_density);
    
    const float h_sq = g_fSmoothlen * g_fSmoothlen;
    
    float3 acceleration = float3(0, 0,0);

    // Calculate the acceleration based on all neighbors
    for (uint N_ID = 0; N_ID < g_iNumParticles; N_ID++)
    {
        float3 N_position = ParticlesRO[N_ID].position;
        
        float3 diff = N_position - P_position;
        float r_sq = dot(diff, diff);
        if (r_sq < h_sq && P_ID != N_ID)
        {
            float3 N_velocity = ParticlesRO[N_ID].velocity;
            float N_density = ParticlesDensityRO[N_ID].density;
            float N_pressure = CalculatePressure(N_density);
            float r = sqrt(r_sq);

            // Pressure Term
            acceleration += CalculateGradPressure(r, P_pressure, N_pressure, N_density, diff);
            
            // Viscosity Term
            acceleration += CalculateLapVelocity(r, P_velocity, N_velocity, N_density);
        }
    }
    
    ParticlesForcesRW[P_ID].acceleration = acceleration / P_density;
}
