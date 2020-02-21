#include "../Volumetric/Scatter.hlsl"
#include "../Common.hlsl"

cbuffer cb0
{
    row_major matrix WorldViewProj;
	row_major matrix World;
};

struct VS_Input
{
    float3 Position : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
};

struct VS_Output
{
    float4 Position : SV_POSITION;
	float3 WorldPos : POSITION;
    float3 Normal : NORMAL;
    float2 UV : TEXCOORD;
	float4 Colour1	: COLOR0;
	float4 Colour2	: COLOR1;
	float  Intensity : COLOR2;
};

void main(in VS_Input v_in, out VS_Output v_out)
{
	float3 pos = mul(float4(v_in.Position, 1.0f), World);
	scatterGroundFromAtmosphere(pos);
	
    v_out.Position = mul(float4(v_in.Position, 1.0f), WorldViewProj);
	v_out.WorldPos = pos;
    v_out.Normal = normalize(v_in.Normal);
    v_out.UV = v_in.UV;
    v_out.Colour1 = float4(PrimaryColour, 0.0f);
    v_out.Colour2 = float4(SecondaryColour, 0.0f);
	v_out.Intensity = (dot(v3LightPos, v_in.Normal) + 1.0) * 0.5;
    v_out.Position.z = LogDepthBuffer(v_out.Position.w);
}