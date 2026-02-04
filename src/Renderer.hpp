#pragma once
#include "Config.h"
#include "Shared.h"
#include "metal-cpp/Metal/Metal.hpp"
#include "metal-cpp/QuartzCore/QuartzCore.hpp"
#include <SDL2/SDL.h>

class Renderer {
public:
  Renderer(SDL_Window *window);
  ~Renderer();
  void renderFrame();
  void resize(int width, int height);
  Config::Settings &getSettings() { return _settings; }

private:
  void buildShaders();
  void buildBuffers();
  void updateUniforms();

  Config::Settings _settings;
  int _width, _height;
  int _frameIndex = 0;

  MTL::Device *_device;
  MTL::CommandQueue *_commandQueue;
  CA::MetalLayer *_layer;

  MTL::Buffer *_antBuffer = nullptr;
  MTL::Buffer *_uniformBuffer = nullptr;

  MTL::Texture *_pheromoneTextures[2] = {nullptr, nullptr};

  MTL::Texture *_msaaTexture = nullptr;
  MTL::Texture *_depthTexture = nullptr;

  MTL::RenderPipelineState *_renderAntsPSO = nullptr;
  MTL::RenderPipelineState *_renderPheromonesPSO = nullptr;

  MTL::ComputePipelineState *_computeMovePSO = nullptr;
  MTL::ComputePipelineState *_computeDiffusePSO = nullptr;

  MTL::DepthStencilState *_depthStencilState = nullptr;
  const int _sampleCount = 4;
};