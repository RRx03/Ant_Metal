#pragma once
#include <simd/simd.h>

namespace Config {

constexpr int WINDOW_WIDTH = 800;
constexpr int WINDOW_HEIGHT = 800;

constexpr int MAX_ANTS = 500000;

struct Settings {
  int antCount = 1000000;
  float antSize = 0.2f;
  float antSpeed = 1.0f;
  float sensorAngle = 0.78f;
  float sensorDist = 3.0f;
  float evapSpeed = 0.50f;
  float depositAmount = 0.1f;
  float turnAngle = 0.4f;

  int colonyCount = 3;
};
} // namespace Config