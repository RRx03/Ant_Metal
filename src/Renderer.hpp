#pragma once

#include "Config.h"
#include "metal-cpp/Metal/Metal.hpp"
#include "metal-cpp/QuartzCore/QuartzCore.hpp"
#include <SDL.h>
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

  MTL::Device *_device = nullptr;
  MTL::CommandQueue *_commandQueue = nullptr;
  CA::MetalLayer *_layer = nullptr;

  MTL::RenderPipelineState *_renderPSO = nullptr;
  MTL::Texture *_depthTexture = nullptr;
  MTL::DepthStencilState *_depthStencilState = nullptr;

  MTL::Buffer *_antBuffer = nullptr;
  MTL::Buffer *_uniformBuffer = nullptr;

  int _width, _height; // Pour le ratio d'aspect

  MTL::Texture *_msaaTexture = nullptr;
  const int _sampleCount = 4;

  MTL::Texture *_pheromoneTextureA = nullptr;
  MTL::Texture *_pheromoneTextureB = nullptr;
};