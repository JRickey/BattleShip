#pragma once

#include <span>

#include <fast/interpreter.h>

namespace ssb64 {

// Fast3D permutations observed across the US release's opening, menus,
// character select, VS gameplay, and attract-mode matches. This catalog is a
// warmup hint, not a correctness boundary: unseen permutations continue to
// use LibUltraShip's normal lazy compilation path.
std::span<const Fast::ShaderPermutation> GetFast3dShaderManifest();

} // namespace ssb64
