#pragma once
#include <simd/simd.h>

namespace Config {

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr int MAX_ANTS = 500000;

struct Settings {
  int antCount = 2000000;
  float antSize = 0.2f;
  float antSpeed = 3.0f;
  float sensorAngle = 0.78f;
  float sensorDist = 5.0f;
  float evapSpeed = 0.50f;
  float depositAmount = 0.1f;
  float turnAngle = 0.4f;
};
} // namespace Config