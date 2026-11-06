#ifndef CROSSHATCH_COMMON_HLSLI
#define CROSSHATCH_COMMON_HLSLI

// Cross hatching style, shared by the hatch pass (crosshatch_ps.hlsl) and the outline pass (outline_vs/ps.hlsl).
// Mirrors CrossHatchConstants in PipelineDefs.hpp, filled from ProjectCrossHatchSettings.

static const int HATCH_MODE_NOISE = 0;          // World-space noise strokes, denser where darker
static const int HATCH_MODE_TOON_TEXTURE = 1;   // Cel-shaded paper with a stroke texture in UV space
static const int HATCH_MODE_TOON_NOISE = 2;     // Three-tone cel shading with noise strokes in the midtones

cbuffer CrossHatchConstants
{
    float4 g_InkColor;              // rgb: ink color, a: hatch opacity (0 = plain lit color)
    float4 g_CrossHatchCameraPos;   // xyz: camera position, for the outline pass which doesn't bind LightConstants

    // Lit layer: strokes that grow denser as the surface darkens
    float g_LitScale;
    float g_LitThickness;
    float g_LitDensity;
    float g_LitAngle;

    // Detail layer: a constant layer of strokes on top
    float g_DetailScale;
    float g_DetailDensity;
    float g_DetailAngle;
    float g_DetailThickness;

    // Shadow layer: strokes inside the key light's shadow
    float g_ShadowScale;
    float g_ShadowDensity;
    float g_ShadowAngle;
    float g_ShadowThickness;

    float g_LineSmoothness;
    float g_ShadowSmoothness;
    float g_ShadowCoreBias;         // < 1 keeps the shadow strokes dense up to the shadow's edge
    float g_ShadowIntensity;

    // Toon texture mode
    float g_TexScale;
    float g_TexPriority;            // Weight of the second (cross) stroke direction
    float g_TexBlackThreshold;      // Light level below which the surface turns solid ink
    float g_TexSmoothness;

    float g_TexRotation;
    float g_TexThickness;
    float g_TexLightBias;
    float g_SpecularThreshold;

    // Outline (inverted hull)
    float g_OutlineWidth;
    float g_OutlineMode;            // 0: extrude along normals, 1: extrude away from the object's origin
    float g_OutlineScaleWithDistance; // 1: width is multiplied by the distance to the camera, keeping it constant on screen
    float g_CrossHatchPadding0;

    int g_HatchMode;
    int g_InnerEdges;
    int g_SpecularEnabled;
    int g_CrossHatchPadding1;
};

#endif // CROSSHATCH_COMMON_HLSLI
