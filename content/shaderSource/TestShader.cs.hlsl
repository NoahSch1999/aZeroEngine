struct Dummy
{
    int idk;
};

ConstantBuffer<Dummy> x : register(b0);
ConstantBuffer<Dummy> y : register(b1);
StructuredBuffer<Dummy> z : register(t0);
RWStructuredBuffer<float3> BufferOut : register(u7);

[numthreads(2, 3, 4)]
void main( uint3 DTid : SV_DispatchThreadID )
{
    Dummy idkx = x;
    Dummy idky = y;
    Dummy idkz = z.Load(0);
    BufferOut[DTid.x] = idkx.idk;
    BufferOut[DTid.y] = idky.idk;
    BufferOut[DTid.z] = idkz.idk;
}