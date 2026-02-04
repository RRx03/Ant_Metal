#include <metal_stdlib>
using namespace metal;
#include "../src/Shared.h"

struct VertexOut {
    float4 position [[position]];
    float2 uv; // Coordonnées de texture pour le sprite
};


float random(float2 seed) {
    return fract(sin(dot(seed ,float2(12.9898,78.233))) * 43758.5453);
}
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

kernel void diffuse_decay(
    texture2d<float> textureA [[texture(0)]], 
    texture2d<float> textureB [[texture(1)]], 
    uint2 gid [[thread_position_in_grid]], 
    constant SimulationUniforms& uniforms [[buffer(1)]])
{
    uint2 gid = uint2(get_global_id(0), get_global_id(1));
    float sum = 0.0;
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            sum += textureA.read(uint2(gid.x + x, gid.y + y)).rotatedPos;
        }
    }
    float average = sum / 9.0;
    average *= uniforms.evapSpeed;
    textureB.write(float4(average, 0.0, 0.0, 1.0), gid);
}


kernel void update_ants(
    texture2d<float> texture [[texture(0)]], 
    uint2 gid [[thread_position_in_grid]],
    constant SimulationUniforms& uniforms [[buffer(1)]],
    AntData* ants [[buffer(0)]])
{
    //follow a drunkard walk influenced /weighted by pheromones
    AntData ant = ants[gid.x]; // assuming gid.x indexes the ants
    float2 forwardSensorOffset = float2(cos(ant.angle), sin(ant.angle)) * uniforms.sensorOffset;
    float2 leftSensorOffset = float2(cos(ant.angle + uniforms.sensorAngle), sin(ant.angle + uniforms.sensorAngle)) * uniforms.sensorOffset;
    float2 rightSensorOffset = float2(cos(ant.angle - uniforms.sensorAngle), sin(ant.angle - uniforms.sensorOffset )) * uniforms.sensorOffset;
    float forwardSample = texture.read(uint2(ant.position + forwardSensorOffset)).r;
    float leftSample = texture.read(uint2(ant.position + leftSensorOffset)).r;
    float rightSample = texture.read(uint2(ant.position + rightSensorOffset)).r;
    if (forwardSample > leftSample && forwardSample > rightSample) {
        // keep going straight
    } else if (leftSample > rightSample) {
        ant.angle += uniforms.turnAngle;
    } else if (rightSample > leftSample) {
        ant.angle -= uniforms.turnAngle;
    } else {
        // random turn
        ant.angle += (random(ant.position) - 0.5) * uniforms.turnAngle;
    }
    // move forward
    ant.position += float2(cos(ant.angle), sin(ant.angle)) * uniforms.antSpeed;
    // wrap around
    ant.position = fmod(ant.position + uniforms.worldSize, uniforms.worldSize);
    // deposit pheromone
    float currentPheromone = texture.read(uint2(ant.position)).r;
    texture.write(float4(currentPheromone + uniforms.depositAmount, 0.0, 0.0, 1.0), uint2(ant.position));
    // write back
    ants[gid.x] = ant;
    
}