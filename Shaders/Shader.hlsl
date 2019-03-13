struct VsInput
{
    float2 Pos: Pos;
    float2 TexCoord: TexCoord;
    float4 Color: Color;
};

struct VsOutput
{
	float4 Pos: SV_Position;
    float2 TexCoord: TexCoord;
    float4 Color: Color;
};

void MainVS(VsInput input, out VsOutput output)
{
    output.Pos.xy = input.Pos;
    output.Pos.z = 0.5;
    output.Pos.w = 1.0;
    output.TexCoord = input.TexCoord;
    output.Color = input.Color;
}

Texture2D texture0: register(t0);
SamplerState sampler0: register(s0);

void MainPS(VsOutput input, out float4 output: SV_Target)
{
    output = input.Color * float4(1.0, 1.0, 1.0, texture0.Sample(sampler0, input.TexCoord).a);
}
