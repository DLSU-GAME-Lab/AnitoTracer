# External shaders

Shader code ported from other projects lives here, one folder per project, so its origin and licence stay easy to find.
Everything else under `Shaders/` is written for AnitoTracer.

Rules for this folder:

- Each ported file starts with a comment naming the project, its copyright line and licence, the source file(s) it came
  from, and what was changed.
- Each project folder holds a copy of that project's licence.
- Keep ported code close to its source (names, comments), so it can be compared against a newer upstream version.
  Code written for AnitoTracer that only *uses* a ported file belongs outside this folder.

## DiligentFX

Source: https://github.com/DiligentGraphics/DiligentFX (the copy fetched into `DiligentEngine/DiligentFX`).
Licence: Apache License 2.0, see `DiligentFX/LICENSE.txt`.

| File | Ported from | Used by |
|---|---|---|
| `tonemap.hlsli` | `Shaders/PostProcess/ToneMapping/public/ToneMapping.fxh`, `Shaders/Common/public/SRGBUtilities.fxh` | `PostProcess/color_correction_ps.hlsl` (the display pass) |
| `ibl_precompute_common.hlsli` | `Shaders/PBR/private/PBR_PrecomputeCommon.fxh`, `Shaders/Common/public/PBR_Common.fxh`, `Shaders/Common/public/ShaderUtilities.fxh` | the two sky cube filters below |
| `compute_irradiance_map_ps.hlsl` | `Shaders/PBR/private/ComputeIrradianceMap.psh` | `SkyEnvironment` (diffuse sky lighting cube) |
| `prefilter_env_map_ps.hlsl` | `Shaders/PBR/private/PrefilterEnvMap.psh` | `SkyEnvironment` (sky reflection cube) |

`tonemap.hlsli` also carries two curves that DiligentFX itself took from elsewhere: the minimal AgX by Benjamin Wrensch
(https://iolite-engine.com/blog_posts/minimal_agx_implementation) and Khronos PBR Neutral
(https://github.com/KhronosGroup/ToneMapping, Apache License 2.0).
