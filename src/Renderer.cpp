#include "Renderer.hpp"
#include "MathUtils.h"
#include "Shared.h"
#include <iostream>
#include <vector>

Renderer::Renderer(SDL_Window *window) {
  _width = Config::WINDOW_WIDTH;
  _height = Config::WINDOW_HEIGHT;
  _settings = Config::Settings();

  _device = MTL::CreateSystemDefaultDevice();
  if (!_device)
    throw std::runtime_error("No Metal Device");
  _commandQueue = _device->newCommandQueue();

  SDL_MetalView view = SDL_Metal_CreateView(window);
  _layer = reinterpret_cast<CA::MetalLayer *>(SDL_Metal_GetLayer(view));
  _layer->setDevice(_device);
  _layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

  buildShaders();
  buildBuffers();
}

Renderer::~Renderer() {
  if (_pheromoneTextures[0])
    _pheromoneTextures[0]->release();
  if (_pheromoneTextures[1])
    _pheromoneTextures[1]->release();
  if (_msaaTexture)
    _msaaTexture->release();
  if (_depthTexture)
    _depthTexture->release();

  if (_antBuffer)
    _antBuffer->release();
  if (_uniformBuffer)
    _uniformBuffer->release();

  if (_renderAntsPSO)
    _renderAntsPSO->release();
  if (_renderPheromonesPSO)
    _renderPheromonesPSO->release();
  if (_computeMovePSO)
    _computeMovePSO->release();
  if (_computeDiffusePSO)
    _computeDiffusePSO->release();

  if (_depthStencilState)
    _depthStencilState->release();
  if (_commandQueue)
    _commandQueue->release();
  if (_device)
    _device->release();
}

void Renderer::buildShaders() {
  NS::Error *error = nullptr;
  MTL::Library *lib = _device->newLibrary(
      NS::String::string("./build/default.metallib", NS::UTF8StringEncoding),
      &error);
  if (!lib) {
    std::cerr << "Erreur Library: "
              << error->localizedDescription()->utf8String() << std::endl;
    return;
  }

  MTL::Function *vertAnt = lib->newFunction(
      NS::String::string("ant_vertex", NS::UTF8StringEncoding));
  MTL::Function *fragAnt = lib->newFunction(
      NS::String::string("ant_fragment", NS::UTF8StringEncoding));

  MTL::RenderPipelineDescriptor *desc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  desc->setVertexFunction(vertAnt);
  desc->setFragmentFunction(fragAnt);
  desc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormatBGRA8Unorm);
  desc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
  desc->setRasterSampleCount(_sampleCount);

  // Additive Blending pour que les fourmis s'additionnent visuellement
  auto ca = desc->colorAttachments()->object(0);
  ca->setBlendingEnabled(true);
  ca->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
  ca->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);

  _renderAntsPSO = _device->newRenderPipelineState(desc, &error);
  if (!_renderAntsPSO)
    std::cerr << "Error Render Ants: "
              << error->localizedDescription()->utf8String() << std::endl;
  desc->release();

  MTL::Function *vertPhero = lib->newFunction(
      NS::String::string("pheromone_vertex", NS::UTF8StringEncoding));
  MTL::Function *fragPhero = lib->newFunction(
      NS::String::string("pheromone_fragment", NS::UTF8StringEncoding));

  MTL::RenderPipelineDescriptor *pheroDesc =
      MTL::RenderPipelineDescriptor::alloc()->init();
  pheroDesc->setVertexFunction(vertPhero);
  pheroDesc->setFragmentFunction(fragPhero);
  pheroDesc->colorAttachments()->object(0)->setPixelFormat(
      MTL::PixelFormatBGRA8Unorm);
  pheroDesc->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
  pheroDesc->setRasterSampleCount(_sampleCount);

  _renderPheromonesPSO = _device->newRenderPipelineState(pheroDesc, &error);
  if (!_renderPheromonesPSO)
    std::cerr << "Error Render Pheromones: "
              << error->localizedDescription()->utf8String() << std::endl;
  pheroDesc->release();

  MTL::Function *kernelMove = lib->newFunction(
      NS::String::string("update_ants", NS::UTF8StringEncoding));
  _computeMovePSO = _device->newComputePipelineState(kernelMove, &error);
  if (!_computeMovePSO)
    std::cerr << "Error Kernel Move" << std::endl;

  MTL::Function *kernelDiffuse = lib->newFunction(
      NS::String::string("diffuse_decay", NS::UTF8StringEncoding));
  _computeDiffusePSO = _device->newComputePipelineState(kernelDiffuse, &error);
  if (!_computeDiffusePSO)
    std::cerr << "Error Kernel Diffuse" << std::endl;

  MTL::DepthStencilDescriptor *depthDesc =
      MTL::DepthStencilDescriptor::alloc()->init();
  depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
  depthDesc->setDepthWriteEnabled(true);
  _depthStencilState = _device->newDepthStencilState(depthDesc);
  depthDesc->release();

  vertAnt->release();
  fragAnt->release();
  vertPhero->release();
  fragPhero->release();
  kernelMove->release();
  kernelDiffuse->release();
  lib->release();
}

void Renderer::buildBuffers() {
  int count = _settings.antCount;
  std::vector<AntData> ants(count);

  for (int i = 0; i < count; i++) {
    float angle = (float)(rand() % 360) * M_PI / 180.0f;
    float dist = (float)(rand() % 100);
    ants[i].position = {_width * 0.5f + cos(angle) * dist,
                        _height * 0.5f + sin(angle) * dist};
    ants[i].angle = angle;
  }

  size_t bufferSize = count * sizeof(AntData);
  _antBuffer = _device->newBuffer(ants.data(), bufferSize,
                                  MTL::ResourceStorageModeShared);
  _uniformBuffer = _device->newBuffer(sizeof(SimulationUniforms),
                                      MTL::ResourceStorageModeShared);
}

void Renderer::resize(int width, int height) {
  _width = width;
  _height = height;
  _layer->setDrawableSize(CGSizeMake(width, height));

  if (_msaaTexture)
    _msaaTexture->release();
  if (_depthTexture)
    _depthTexture->release();
  if (_pheromoneTextures[0])
    _pheromoneTextures[0]->release();
  if (_pheromoneTextures[1])
    _pheromoneTextures[1]->release();

  MTL::TextureDescriptor *simDesc = MTL::TextureDescriptor::texture2DDescriptor(
      MTL::PixelFormatRGBA16Float, width, height, false);
  simDesc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);

  _pheromoneTextures[0] = _device->newTexture(simDesc);
  _pheromoneTextures[1] = _device->newTexture(simDesc);

  MTL::TextureDescriptor *msaaDesc = MTL::TextureDescriptor::alloc()->init();
  msaaDesc->setTextureType(MTL::TextureType2DMultisample);
  msaaDesc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
  msaaDesc->setWidth(width);
  msaaDesc->setHeight(height);
  msaaDesc->setSampleCount(_sampleCount);
  msaaDesc->setUsage(MTL::TextureUsageRenderTarget);
  msaaDesc->setStorageMode(MTL::StorageModePrivate);
  _msaaTexture = _device->newTexture(msaaDesc);
  msaaDesc->release();

  MTL::TextureDescriptor *depthDesc = MTL::TextureDescriptor::alloc()->init();
  depthDesc->setTextureType(MTL::TextureType2DMultisample);
  depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
  depthDesc->setWidth(width);
  depthDesc->setHeight(height);
  depthDesc->setSampleCount(_sampleCount);
  depthDesc->setUsage(MTL::TextureUsageRenderTarget);
  depthDesc->setStorageMode(MTL::StorageModePrivate);
  _depthTexture = _device->newTexture(depthDesc);
  depthDesc->release();
}

void Renderer::updateUniforms() {
  SimulationUniforms uniforms;
  uniforms.antCount = _settings.antCount;
  uniforms.antSpeed = _settings.antSpeed;
  uniforms.sensorAngle = _settings.sensorAngle;
  uniforms.sensorDist = _settings.sensorDist;
  uniforms.evapSpeed = _settings.evapSpeed;
  uniforms.worldSize = {(float)_width, (float)_height};
  uniforms.time += 0.01f;
  uniforms.deltaTime = 1.0f;
  memcpy(_uniformBuffer->contents(), &uniforms, sizeof(SimulationUniforms));
}

void Renderer::renderFrame() {
  if (!_pheromoneTextures[0] || !_antBuffer)
    return;
  updateUniforms();

  NS::AutoreleasePool *pool = NS::AutoreleasePool::alloc()->init();

  MTL::Texture *texRead = _pheromoneTextures[_frameIndex % 2];
  MTL::Texture *texWrite = _pheromoneTextures[(_frameIndex + 1) % 2];

  MTL::CommandBuffer *buffer = _commandQueue->commandBuffer();

  MTL::ComputeCommandEncoder *compute = buffer->computeCommandEncoder();

  compute->setComputePipelineState(_computeDiffusePSO);
  compute->setTexture(texRead, 0);
  compute->setTexture(texWrite, 1);
  compute->setBuffer(_uniformBuffer, 0, 2);

  {
    int w = _width;
    int h = _height;
    MTL::Size grp = MTL::Size::Make(16, 16, 1);
    MTL::Size grd = MTL::Size::Make((w + 15) / 16, (h + 15) / 16, 1);
    compute->dispatchThreadgroups(grd, grp);
  }

  compute->setComputePipelineState(_computeMovePSO);
  compute->setBuffer(_antBuffer, 0, 0);
  compute->setBuffer(_uniformBuffer, 0, 1);
  compute->setTexture(texWrite, 0);
  compute->setTexture(texWrite, 1);

  {
    int cnt = _settings.antCount;
    int grpSize = _computeMovePSO->maxTotalThreadsPerThreadgroup();
    if (grpSize > cnt)
      grpSize = cnt;
    MTL::Size grd = MTL::Size::Make(cnt, 1, 1);
    MTL::Size grp = MTL::Size::Make(grpSize, 1, 1);
    compute->dispatchThreads(grd, grp);
  }

  compute->endEncoding();

  CA::MetalDrawable *drawable = _layer->nextDrawable();
  if (drawable) {
    MTL::RenderPassDescriptor *pass =
        MTL::RenderPassDescriptor::renderPassDescriptor();

    auto ca = pass->colorAttachments()->object(0);
    ca->setTexture(_msaaTexture);
    ca->setResolveTexture(drawable->texture());
    ca->setLoadAction(MTL::LoadActionClear);
    ca->setClearColor(MTL::ClearColor::Make(0, 0, 0, 1));
    ca->setStoreAction(MTL::StoreActionMultisampleResolve);

    auto da = pass->depthAttachment();
    da->setTexture(_depthTexture);
    da->setLoadAction(MTL::LoadActionClear);
    da->setClearDepth(1.0);
    da->setStoreAction(MTL::StoreActionDontCare);

    MTL::RenderCommandEncoder *render = buffer->renderCommandEncoder(pass);

    render->setRenderPipelineState(_renderPheromonesPSO);
    render->setFragmentTexture(texWrite, 0);
    render->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 6, 1);

    render->setRenderPipelineState(_renderAntsPSO);
    render->setDepthStencilState(_depthStencilState);
    render->setVertexBuffer(_antBuffer, 0, 0);
    render->setVertexBuffer(_uniformBuffer, 0, 1);
    render->drawPrimitives(MTL::PrimitiveTypeTriangle, 0, 6,
                           _settings.antCount);

    render->endEncoding();
    buffer->presentDrawable(drawable);
    buffer->commit();
  }

  _frameIndex++;
  pool->release();
}