struct PSInput
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};

void main_vs(in uint VertId : SV_VertexID, out PSInput Out)
{
    // Generate a fullscreen triangle using just the vertex ID
    float2 texcoords = float2((VertId << 1) & 2, VertId & 2);
    Out.UV = texcoords;
    
    // Convert UV to NDC (-1..1) 
    // Diligent Engine normalizes coordinate systems, so standard D3D Y-inversion works perfectly!
    Out.Pos = float4(texcoords.x * 2.0 - 1.0, 1.0 - 2.0 * texcoords.y, 0.0, 1.0);
}