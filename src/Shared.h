
#pragma once
#include <simd/simd.h>

struct AntData {
  vector_float2 position;
  vector_float2 velocity;
  float angle;
  float padding;
};

struct SimulationUniforms {
  unsigned int antCount;
  float antSpeed;
  float antSize;
  float sensorAngle;
  float sensorDist;
  float evapSpeed;
  vector_float2 worldSize;
  float time;
  float deltaTime;
};