#define BITONIC_BLOCK_SIZE 512

#define TRANSPOSE_BLOCK_SIZE 16

//--------------------------------------------------------------------------------------
// Constant Buffers
//--------------------------------------------------------------------------------------
cbuffer CB : register(b0)
{
    unsigned int g_iLevel;      //2^sort Times
    unsigned int g_iLevelMask;
    unsigned int g_iWidth;
    unsigned int g_iHeight;
};
//--------------------------------------------------------------------------------------
// Structured Buffers
//--------------------------------------------------------------------------------------
StructuredBuffer<unsigned int> Input : register(t0);
RWStructuredBuffer<unsigned int> Data : register(u0);

//--------------------------------------------------------------------------------------
// Bitonic Sort Compute Shader
//--------------------------------------------------------------------------------------
groupshared unsigned int shared_data[BITONIC_BLOCK_SIZE];
[numthreads(BITONIC_BLOCK_SIZE, 1, 1)]
void BitonicSort(uint3 Gid : SV_GroupID,
                  uint3 DTid : SV_DispatchThreadID,
                  uint3 GTid : SV_GroupThreadID,
                  uint GI : SV_GroupIndex)
{
    // Load shared data
    shared_data[GI] = Data[DTid.x];
    GroupMemoryBarrierWithGroupSync();
    
    // Sort the shared data
    for (unsigned int j = g_iLevel >> 1; j > 0; j >>= 1)
    {
       //same as follow
       // uint smallerIndex = GI & ~j;
       // uint largerIndex = GI | j;
       // bool isDescending = (bool) (g_DescendMask & DTid.x);
       // bool isSmallerIndex = (GI == smallerIndex);
       // uint result = ((shared_data[smallerIndex] <= shared_data[largerIndex]) == (isDescending == isSmallerIndex)) ?
       //shared_data[largerIndex] : shared_data[smallerIndex];

        unsigned int result = ((shared_data[GI & ~j] <= shared_data[GI | j]) == (bool) (g_iLevelMask & DTid.x)) ? shared_data[GI ^ j] : shared_data[GI];
        GroupMemoryBarrierWithGroupSync();
        shared_data[GI] = result;
        GroupMemoryBarrierWithGroupSync();
    }
    
    // Store shared data
    Data[DTid.x] = shared_data[GI];
}

//--------------------------------------------------------------------------------------
// Matrix Transpose Compute Shader
//--------------------------------------------------------------------------------------
groupshared unsigned int transpose_shared_data[TRANSPOSE_BLOCK_SIZE * TRANSPOSE_BLOCK_SIZE];

[numthreads(TRANSPOSE_BLOCK_SIZE, TRANSPOSE_BLOCK_SIZE, 1)]
void MatrixTranspose(uint3 Gid : SV_GroupID,
                      uint3 DTid : SV_DispatchThreadID,
                      uint3 GTid : SV_GroupThreadID,
                      uint GI : SV_GroupIndex)
{
    transpose_shared_data[GI] = Input[DTid.y * g_iWidth + DTid.x];
    GroupMemoryBarrierWithGroupSync();
    uint2 XY = DTid.yx - GTid.yx + GTid.xy;
    Data[XY.y * g_iHeight + XY.x] = transpose_shared_data[GTid.x * TRANSPOSE_BLOCK_SIZE + GTid.y];
}
