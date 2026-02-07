#include <metal_stdlib>
using namespace metal;
#include "../src/Shared.h"


constant float2 quadVertices[] = {
    {-0.5, -0.5}, { 0.5, -0.5}, {-0.5,  0.5},
    { 0.5, -0.5}, { 0.5,  0.5}, {-0.5,  0.5}
};

float2x2 rotMat(float a) { return float2x2(cos(a), -sin(a), sin(a), cos(a)); }

kernel void diffuse_decay(
    texture2d<float, access::read> inTex [[texture(0)]],
    texture2d<float, access::write> outTex [[texture(1)]],
    constant SimulationUniforms& u [[buffer(2)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= inTex.get_width() || gid.y >= inTex.get_height()) {return;}


    float4 sum = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            uint2 pos = uint2(gid.x + dx, gid.y + dy);
            sum += inTex.read(pos);
        }
    }
    float4 avg = sum / 9.0;


    float4 result = avg * u.evapSpeed; 
    

    result.a = 1.0; 

    outTex.write(result, gid);
}

float sense(texture2d<float, access::read> grid, float2 pos, float angle, float dist, float sizeX, float sizeY) {

    float2 sensorDir = float2(cos(angle), sin(angle));
    float2 sensorPos = pos + sensorDir * dist;
    uint2 texPos = uint2(sensorPos);

    if (texPos.x >= sizeX || texPos.y >= sizeY) return 0.0;
    

    return grid.read(texPos).r;
}


uint hash(uint state) {
    state ^= 2747636419u;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    state ^= state >> 16;
    state *= 2654435769u;
    return state;
}
kernel void update_ants(
    device AntData* ants [[buffer(0)]],
    constant SimulationUniforms& u [[buffer(1)]],
    device ColonyData* colonies [[buffer(3)]], // <-- Accès aux colonies (RW pour le stock de nourriture)
    texture2d<float, access::read> inTex [[texture(0)]],
    texture2d<float, access::write> outTex [[texture(1)]],
    uint id [[thread_position_in_grid]]
) {
    if (id >= u.antCount) return;

    AntData ant = ants[id];
    
    // --- GESTION ÉNERGIE ---
    // Coût basal + Coût de mouvement
    float energyCost = 0.1; 
    ant.energy -= energyCost;

    // --- MORT & RESPAWN ---
    if (ant.energy <= 0.0) {
        // La fourmi meurt.
        // Option A: Elle reste sur place (cadavre).
        // Option B (Choisie): La colonie pond une nouvelle fourmi (Respawn)
        
        // On remet la fourmi à la base
        ColonyData colony = colonies[ant.colonyID];
        ant.position = colony.position;
        ant.energy = 100.0; // Recharge
        ant.angle = float(id) * 123.45; // Angle pseudo-random
        
        // TODO futur: Diminuer colony.foodStock ici
    }

    // --- SENSING (Logique existante) ---
    // (Note: Pour l'instant, toutes les fourmis suivent le canal Rouge.
    // Plus tard, on fera en sorte qu'elles suivent leur propre phéromone ou la nourriture)
    float width = u.worldSize.x;
    float height = u.worldSize.y;
    // ... (Code sensing inchangé) ...

    // --- DEPLACEMENT ---
    float2 pos = ant.position;
    float angle = ant.angle;
    
    float2 direction = float2(cos(angle), sin(angle));
    float2 nextPos = pos + direction * u.antSpeed;

    // Rebond bords (Simple)
    if (nextPos.x <= 0 || nextPos.x >= width) { nextPos.x = clamp(nextPos.x, 1.0, width-1.0); angle = M_PI_F - angle; }
    if (nextPos.y <= 0 || nextPos.y >= height) { nextPos.y = clamp(nextPos.y, 1.0, height-1.0); angle = -angle; }

    // --- DEPOT PHEROMONE (Marquage Territoire) ---
    uint2 texPos = uint2(nextPos);
    if(texPos.x < width && texPos.y < height) {
        // Pour l'instant, tout le monde écrit en ROUGE.
        // A l'avenir: colonies[ant.colonyID].color
        // Mais attention, la texture est RGBA16Float, on peut encoder des infos dans R, G, B.
        // Pour visualiser simple: On écrit 1.0 dans le canal Rouge.
        outTex.write(float4(1.0, 0.0, 0.0, 1.0), texPos);
    }

    // Save
    ant.position = nextPos;
    ant.angle = angle;
    ants[id] = ant;
}
struct VertexOut {
    float4 position [[position]];
    float2 uv;
    float4 color; // <--- On ajoute la couleur
};

vertex VertexOut ant_vertex(
    uint vid [[vertex_id]],
    uint iid [[instance_id]],
    constant AntData* ants [[buffer(0)]],
    constant SimulationUniforms& u [[buffer(1)]],
    constant ColonyData* colonies [[buffer(2)]] // <--- Accès lecture seule aux colonies
) {
    VertexOut out;
    float2 local = quadVertices[vid];
    AntData ant = ants[iid];
    
    // Récupérer la couleur de la colonie de cette fourmi
    out.color = colonies[ant.colonyID].color; 
    
    // Si l'énergie est faible, on assombrit la fourmi (Visual Feedback !)
    float vitality = clamp(ant.energy / 100.0, 0.2, 1.0);
    out.color.rgb *= vitality;

    float2 worldPos = rotMat(ant.angle) * (local * u.antSize) + ant.position;
    float2 clip = (worldPos / u.worldSize) * 2.0 - 1.0;
    out.position = float4(clip.x, -clip.y, 0.0, 1.0);
    out.uv = local + 0.5;
    return out;
}

fragment float4 ant_fragment(VertexOut in [[stage_in]]) {
    float dist = length(in.uv - 0.5);
    if (dist > 0.5) discard_fragment();
    return in.color;
}

struct PheroOut {
    float4 position [[position]];
    float2 uv;
};


constant float2 fullQuad[] = {
    {-1, -1}, { 1, -1}, {-1,  1},
    { 1, -1}, { 1,  1}, {-1,  1}
};

vertex PheroOut pheromone_vertex(uint vid [[vertex_id]]) {
    PheroOut out;
    out.position = float4(fullQuad[vid], 0.0, 1.0);
    out.uv = fullQuad[vid] * 0.5 + 0.5;
    out.uv.y = 1.0 - out.uv.y;
    return out;
}

fragment float4 pheromone_fragment(
    PheroOut in [[stage_in]],
    texture2d<float> pheroTex [[texture(0)]]
) {
    constexpr sampler s(mag_filter::linear, min_filter::linear);
    float4 val = pheroTex.sample(s, in.uv);
 
    return float4(val.r, 0.0, 0.0, 1.0);
}