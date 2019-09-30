struct Particle
{
    float3 Position;
    float4 Colour;
    float4 OriginalColour;

    double3 Velocity;
    double3 Forces;
    
    double Mass;
};

cbuffer FrameBuffer : register(b0)
{
    int NumParticles;
    float FrameTime;
    double Scale;
};

StructuredBuffer<Particle> InBuffer : register(t0);
RWStructuredBuffer<Particle> OutBuffer : register(u0);

double Gravity(const Particle a, const Particle b)
{
    double3 ap = double3(a.Position.x, a.Position.y, a.Position.z);
    double3 bp = double3(b.Position.x, b.Position.y, b.Position.z);

    ap *= SCALE;
    bp *= SCALE;

    double d = (double)dot(a.Position, b.Position);
    return (G * a.Mass * b.Mass) / (d + S * S);
}

[numthreads(1, 1, 1)]
void CSMain(uint3 ID : SV_DispatchThreadID)
{
    OutBuffer[ID.x].Position        = InBuffer[ID.x].Position;
    OutBuffer[ID.x].Colour          = InBuffer[ID.x].Colour;
    OutBuffer[ID.x].OriginalColour  = InBuffer[ID.x].OriginalColour;
    OutBuffer[ID.x].Velocity        = InBuffer[ID.x].Velocity;
    OutBuffer[ID.x].Mass            = InBuffer[ID.x].Mass;

    for(int i = 0; i < NumParticles; i += 256)
    {
        for(int j = 0; j < 256; ++j)
        {
            int id = i + SV_GroupThreadID;

            double3 a = InBuffer[ID.x].Position;
            double3 b = InBuffer[id].Position;

            double3 d = double3(b - a);
            double l = length(d);

            double f = Gravity(InBuffer[id], InBuffer[ID.x]);
            
            OutBuffer[ID.x].Forces += double3(f * d.x / l, f * d.y / l, f * d.z / l);
        }
    }
}