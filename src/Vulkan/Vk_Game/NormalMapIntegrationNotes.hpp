#pragma once

// ============================================================================
// CRITICAL INTEGRATION NOTES FOR NORMAL MAP SUPPORT
// ============================================================================
//
// This file documents important details and gotchas when integrating
// normal map support into the Game Renderer.
//

// ============================================================================
// 1. DESCRIPTOR BINDING LAYOUT
// ============================================================================
//
// EXISTING BINDINGS (0-11):
//   0  : UBO                           (vert + frag)
//   1  : Material buffer               (frag)
//   2  : Light buffer                  (frag)
//   3  : Texture array (conditional)   (frag)
//   4  : Skybox sampler                (frag)
//   5  : Shadow maps [4]               (frag)
//   6  : ShadowUBO                     (frag)
//   7  : Point shadow maps [4]         (frag)
//   8  : PointShadowUBO                (frag)
//   9  : IBL irradiance                (frag)
//   10 : IBL prefiltered               (frag)
//   11 : BRDF LUT                      (frag)
//
// NEW BINDINGS (12-16) - ONLY IF SCENE HAS TEXTURES:
//   12 : Normal maps array             (frag) - counts as (texCount) descriptors
//   13 : GameRenderer material props   (frag) - buffer with material properties
//   14 : Metallic maps array           (frag) - counts as (texCount) descriptors
//   15 : Roughness maps array          (frag) - counts as (texCount) descriptors
//   16 : AO maps array                 (frag) - counts as (texCount) descriptors
//
// IMPORTANT: All map arrays have the same SIZE (texCount).
// Reuse the same texture array for now; later can use distinct textures.
//

// ============================================================================
// 2. BUFFER ALIGNMENT AND SIZE CALCULATION
// ============================================================================
//
// GameRendererMaterialProperties is 48 bytes (3 vec4s):
//   - int   NormalMapTextureId    (4 bytes)
//   - float NormalMapStrength     (4 bytes)
//   - int   MetallicMapTextureId  (4 bytes)
//   - float MetallicValue         (4 bytes) <- FIRST VEC4 COMPLETE
//   - int   RoughnessMapTextureId (4 bytes)
//   - float RoughnessValue        (4 bytes)
//   - int   AOMapTextureId        (4 bytes)
//   - float AOStrength            (4 bytes) <- SECOND VEC4 COMPLETE
//   - float _pad[4]               (16 bytes) <- THIRD VEC4 PADDING
//
// Total: 48 bytes for std140 alignment
//
// Calculate buffer size:
//   size = num_materials * 48
//

// ============================================================================
// 3. SHADER VERIFICATION CHECKLIST
// ============================================================================
//
// game_vert.vert MUST:
//   [ ] Include push constant with WorldMatrix (mat4)
//   [ ] Output location 0: vec3 outWorldPos
//   [ ] Output location 1: vec3 outNormal (world-space)
//   [ ] Output location 2: vec2 outTexCoord
//   [ ] Output location 3: flat int outMaterialIndex
//   [ ] Output location 4: vec3 outTangent (NEW)
//   [ ] Compute outTangent as orthogonal to normal
//   [ ] NOT using normalized world normal in tangent computation (causes precision loss)
//
// game_frag.frag MUST:
//   [ ] Input location 0: vec3 inWorldPos (world-space position)
//   [ ] Input location 1: vec3 inNormal (world-space normal)
//   [ ] Input location 2: vec2 inTexCoord
//   [ ] Input location 3: flat int inMaterialIndex
//   [ ] Input location 4: vec3 inTangent (NEW)
//   [ ] Binding 12: sampler2D normalMaps[]
//   [ ] Binding 13: buffer with GameRenderMaterialProperties[]
//   [ ] Binding 14: sampler2D metallicMaps[]
//   [ ] Binding 15: sampler2D roughnessMaps[]
//   [ ] Binding 16: sampler2D aoMaps[]
//   [ ] Call ConstructTBNMatrix() to create tangent-space foundation
//   [ ] Call SampleAndApplyNormalMap() AFTER computing normal
//   [ ] Handle negative texture IDs (-1 = no map)
//
// CRITICAL: nonuniformEXT(index) REQUIRED for dynamic indexing!
//   Correct:   texture(normalMaps[nonuniformEXT(id)], uv)
//   Wrong:     texture(normalMaps[id], uv)
//

// ============================================================================
// 4. DESCRIPTOR SET UPDATE SEQUENCE
// ============================================================================
//
// In CreateDescriptorSets(), when writing descriptors for frame i:
//
// For each binding 12-16:
//   1. Check if textureInfos is NOT empty (scene has textures)
//   2. Set binding number correctly (12, 13, 14, 15, 16)
//   3. For buffer (binding 13):
//      - Use gameRendererMatPropsBuffer_->Handle()
//      - offset = 0
//      - range = VK_WHOLE_SIZE
//   4. For sampler arrays (12, 14, 15, 16):
//      - Use textureInfos.data() (or separate texture arrays in future)
//      - Descriptor count = textureInfos.size()
//
// GOTCHA: If textureInfos is EMPTY, don't write bindings 12-16!
// The bindings won't be added to the layout, so descriptor set update fails.
//

// ============================================================================
// 5. MATERIAL PROPERTIES INITIALIZATION
// ============================================================================
//
// When creating GameRendererMaterialProperties for each material:
//
// Default initialization (NO MAPS):
//   NormalMapTextureId = -1;
//   NormalMapStrength = 1.0f;
//   MetallicMapTextureId = -1;
//   MetallicValue = 0.0f;
//   RoughnessMapTextureId = -1;
//   RoughnessValue = 0.5f;
//   AOMapTextureId = -1;
//   AOStrength = 1.0f;
//
// To ENABLE normal map for material i:
//   properties[i].NormalMapTextureId = textureArrayIndex;
//   properties[i].NormalMapStrength = 0.5f;  // or 1.0f for full strength
//
// To ENABLE metallic map:
//   properties[i].MetallicMapTextureId = textureArrayIndex;
//
// IMPORTANT: All texture indices must be valid entries in the scene's
// texture array. Use -1 for "no texture" to trigger fallback to default values.
//

// ============================================================================
// 6. TBN MATRIX CONSTRUCTION
// ============================================================================
//
// The ConstructTBNMatrix() function creates an orthonormal basis
// for transforming normals from tangent space (texture space) to world space.
//
// Formula:
//   T = tangent (normalized, input from vertex shader)
//   N = normal (normalized, interpolated from vertices)
//   B = cross(N, T)  <- bitangent (right-hand rule)
//
// Gram-Schmidt orthogonalization (for safety):
//   T = normalize(T - dot(T, N) * N)  <- remove normal component
//   B = cross(N, T)                   <- orthogonal to both N and T
//
// Result: mat3(T, B, N)  <- column vectors for transformation
//
// Application: sampledNormalWorldSpace = TBN * sampledNormalTangentSpace
//
// GOTCHA: tangent-space normal expected in [0, 1] range from texture!
// Convert: normal = normalize(sampledRGB * 2.0 - 1.0)
//

// ============================================================================
// 7. NORMAL MAP STRENGTH AND BLENDING
// ============================================================================
//
// Normal map strength allows smooth transition between:
//   strength = 0.0 : use original normal (no effect)
//   strength = 1.0 : full normal map influence
//   strength = 0.5 : 50/50 blend of original + map
//
// Implementation:
//   vec3 defaultTangentNormal = vec3(0.0, 0.0, 1.0);  // Flat surface
//   vec3 blended = normalize(mix(defaultTangentNormal, sampledNormal, strength));
//   vec3 finalNormal = TBN * blended;
//
// This allows artistic control without reloading textures.
//

// ============================================================================
// 8. AMBIENT OCCLUSION INTEGRATION
// ============================================================================
//
// AO map reduces ambient/indirect lighting:
//
// Without AO:
//   L_ambient = IBL(N, V) * albedo
//
// With AO:
//   ao = SampleAOMap(materialIndex, texCoord);
//   L_ambient = IBL(N, V) * albedo * ao;
//
// AO strength parameter:
//   aoStrength = 1.0  : full occlusion effect
//   aoStrength = 0.0  : no effect (always return 1.0)
//   aoStrength = 0.5  : blend 50% AO
//
// Implementation:
//   float sampledAO = texture(aoMaps[id], uv).r;
//   float finalAO = mix(1.0, sampledAO, strength);
//

// ============================================================================
// 9. LIMITATIONS AND FUTURE IMPROVEMENTS
// ============================================================================
//
// CURRENT LIMITATIONS:
//   - Uses approximate tangent vectors (not from mesh data)
//   - All map textures drawn from same array (no dedicated maps)
//   - No parallax mapping or displacement
//   - Normal maps expected in standard OpenGL format (XYZ = RGB tangent space)
//
// FUTURE ENHANCEMENTS:
//   - Load proper tangent/bitangent from model files (extend Vertex struct)
//   - Support for different texture arrays per map type
//   - Parallax occlusion mapping (POM) for depth effect
//   - Support for DirectX normal map format (inverted green channel)
//   - Runtime normal map intensity adjustment via UI
//   - Material editor to assign maps per-object
//

// ============================================================================
// 10. COMPILATION
// ============================================================================
//
// After all changes, compile the shaders:
//   - game_vert.vert -> game_vert.vert.spv
//   - game_frag.frag -> game_frag.frag.spv
//
// Then rebuild the C++ project. The ProjectPath is:
//   C:\Users\g411_jr\Repos\AnitoTracer\
//
// Use CMake/Ninja as configured:
//   cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug
//   ninja
//
// If shader compilation fails, check:
//   - GLSL version compatibility (using 460)
//   - Extension requirements (GL_EXT_nonuniform_qualifier)
//   - Binding numbers match C++ layout
//   - Layout qualifiers (std140, etc.)
//

// ============================================================================
