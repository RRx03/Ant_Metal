
#pragma once
#include <simd/simd.h>

struct AntData {
  vector_float2 position;
  vector_float2 velocity;
  float angle;
  float energy;
  unsigned int colonyID;
  unsigned int state;

  float padding;
};

struct ColonyData {
  vector_float2 position;
  vector_float4 color;
  float foodStock;
  float padding;
};

struct SimulationUniforms {
  unsigned int antCount;
  float antSpeed;
  float antSize;
  float sensorAngle;
  float sensorDist;
  float evapSpeed;
  float depositAmount;
  float turnAngle;
  vector_float2 worldSize;
  float time;
  float deltaTime;
  unsigned int colonyCount;
  float energyCost;
  float initialEnergy;
  float maxEnergy;
};