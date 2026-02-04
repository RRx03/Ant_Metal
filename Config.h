#pragma once
#include <simd/simd.h>

namespace Config {

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

constexpr int MAX_ANTS = 500000;

struct Settings {
  int antCount = 1000;
  float antSize = 10.0f;
  float antSpeed = 2.0f;
  float sensorAngle = 0.78f;
  float sensorDist = 15.0f;
  float evapSpeed = 0.98f;
  float depositAmount = 0.1f;
  float turnAngle = 0.3f;
};
} // namespace Config