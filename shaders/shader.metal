#include <metal_stdlib>
using namespace metal;
#include "../src/Shared.h"

struct VertexOut {
    float4 position [[position]];
    float2 uv; // Coordonnées de texture pour le sprite
};

constant float2 quadVertices[] = {
    {-0.5, -0.5}, { 0.5, -0.5}, {-0.5,  0.5},
    { 0.5, -0.5}, { 0.5,  0.5}, {-0.5,  0.5}
};

float2x2 rotationMatrix(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return float2x2(c, -s, s, c);
}

vertex VertexOut ant_vertex(
    uint vertexID [[vertex_id]],
    uint instanceID [[instance_id]],
    constant AntData* ants [[buffer(0)]],
    constant SimulationUniforms& uniforms [[buffer(1)]]
) {
    VertexOut out;
    
    AntData ant = ants[instanceID];
    
    float2 localPos = quadVertices[vertexID];
    
    float antSize = uniforms.antSize;
    float2 rotatedPos = rotationMatrix(ant.angle) * (localPos * antSize);
    
    float2 worldPos = rotatedPos + ant.position;
    


    float2 clipPos = (worldPos / uniforms.worldSize) * 2.0 - 1.0;
    
    out.position = float4(clipPos.x, -clipPos.y, 0.0, 1.0);
    

    out.uv = localPos + 0.5; 
    
    return out;
}

// fragment float4 ant_fragment(VertexOut in [[stage_in]],
//                              texture2d<float> spriteTexture [[texture(0)]]) {
//     constexpr sampler s(mag_filter::linear, min_filter::linear);
    

//     float4 color = spriteTexture.sample(s, in.uv);
    
//     if (color.a < 0.1) discard_fragment();
    
//     return color;
// }
fragment float4 ant_fragment(VertexOut in [[stage_in]]) {
    // VERSION DEBUG : On renvoie du blanc pur
    return float4(1.0, 1.0, 1.0, 1.0);
}

kernel void compute_main(
    device float* resultBuffer [[buffer(0)]],
    uint index [[thread_position_in_grid]]
) {
    resultBuffer[index] = float(index) * 0.5;
}