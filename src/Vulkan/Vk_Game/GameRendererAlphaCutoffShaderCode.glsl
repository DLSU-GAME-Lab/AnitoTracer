// ============================================================================
// Alpha Cutoff / Transparency Support for Game Renderer Fragment Shader
// ============================================================================
// 
// This file contains GLSL helper functions for alpha cutoff and material
// transparency handling. Include this content in game_frag.frag alongside
// the existing normal mapping and PBR functions.
//
// Alpha Blend Modes:
//   0 = Opaque      → Alpha cutoff applied, no blending
//   1 = Transparent → Alpha blending enabled, no cutoff
//   2 = Additive    → Additive blending, no cutoff
// ============================================================================

/// @brief Samples alpha from the appropriate texture or material channel.
/// 
/// Priority order:
///   1. If AlphaMapTextureId >= 0: use that texture (R channel, or RGBA avg)
///   2. Else if DiffuseTextureId >= 0: use diffuse texture alpha channel
///   3. Else: return 1.0 (fully opaque)
///
/// @param materialIndex Index into materialProperties buffer
/// @param mat           Material from materials buffer
/// @param texCoord      UV coordinates for texture sampling
/// @return Alpha value in [0, 1] range
float SampleAlpha(int materialIndex, in Material mat, vec2 texCoord)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	// First priority: dedicated alpha map texture
	if (matProps.AlphaMapTextureId >= 0)
	{
		// Sample dedicated alpha map (typically uses R channel for grayscale)
		float sampledAlpha = texture(alphaMaps[nonuniformEXT(matProps.AlphaMapTextureId)], texCoord).r;
		return sampledAlpha;
	}

	// Second priority: alpha channel from diffuse texture
	if (mat.DiffuseTextureId >= 0)
	{
		// Sample diffuse texture and extract alpha channel
		float diffuseAlpha = texture(textures[nonuniformEXT(mat.DiffuseTextureId)], texCoord).a;
		return diffuseAlpha;
	}

	// Default: fully opaque
	return 1.0;
}

/// @brief Applies alpha cutoff test and handles transparency blending.
///
/// This function should be called early in main() after computing alpha value.
/// 
/// Behavior:
///   - AlphaBlendMode 0 (Opaque):      Discard if alpha < threshold, else set alpha=1.0
///   - AlphaBlendMode 1 (Transparent): Allow partial alpha (no cutoff)
///   - AlphaBlendMode 2 (Additive):    Allow partial alpha (no cutoff), prepare for additive
///   - Unknown mode:                    Treat as Opaque (safest default)
///
/// @param alpha         Sampled alpha value [0, 1]
/// @param materialIndex Index into materialProperties buffer
/// @return Final alpha value to use for output color
float ApplyAlphaCutoff(float alpha, int materialIndex)
{
	GameRenderMaterialProperties matProps = materialProperties[materialIndex];

	uint blendMode = matProps.AlphaBlendMode;

	if (blendMode == 0u) {
		// Opaque mode: apply alpha cutoff (alpha test)
		if (alpha < matProps.AlphaCutoffThreshold) {
			discard;  // Fragment is transparent, remove it
		}
		return 1.0;  // Force fully opaque for opaque materials
	}
	else if (blendMode == 1u) {
		// Transparent mode: preserve alpha for blending
		return alpha;
	}
	else if (blendMode == 2u) {
		// Additive mode: preserve alpha for additive blending
		return alpha;
	}
	else {
		// Unknown/invalid mode: default to opaque for safety
		if (alpha < matProps.AlphaCutoffThreshold) {
			discard;
		}
		return 1.0;
	}
}

/// @brief Convenience function to sample alpha and apply cutoff in one call.
///
/// Recommended usage in main():
///   float finalAlpha = SampleAndApplyAlpha(inMaterialIndex, mat, inTexCoord);
///
/// @param materialIndex Index into materialProperties buffer
/// @param mat           Material from materials buffer
/// @param texCoord      UV coordinates for texture sampling
/// @return Final alpha value after cutoff processing
float SampleAndApplyAlpha(int materialIndex, in Material mat, vec2 texCoord)
{
	float alpha = SampleAlpha(materialIndex, mat, texCoord);
	return ApplyAlphaCutoff(alpha, materialIndex);
}
