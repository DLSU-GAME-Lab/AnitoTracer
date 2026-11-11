#ifndef SKY_COMMON_HLSLI
#define SKY_COMMON_HLSLI

// Direction math shared by everything that reads or builds the sky cubemaps
// (Skybox_ps.hlsl, sky_bake_ps.hlsl, hybrid_shading.hlsli and the cube filters in external/DiligentFX).

// Direction through a point on one face of a cubemap.
// face: 0..5 = +X, -X, +Y, -Y, +Z, -Z, the order of a cubemap's array slices.
// uv:   0..1 across the face, (0, 0) at its first texel (top left).
float3 CubeFaceDirection(uint face, float2 uv)
{
    float2 p = uv * 2.0 - 1.0;

    float3 dir;
    if (face == 0u)      dir = float3( 1.0, -p.y, -p.x);
    else if (face == 1u) dir = float3(-1.0, -p.y,  p.x);
    else if (face == 2u) dir = float3( p.x,  1.0,  p.y);
    else if (face == 3u) dir = float3( p.x, -1.0, -p.y);
    else if (face == 4u) dir = float3( p.x, -p.y,  1.0);
    else                 dir = float3(-p.x, -p.y, -1.0);
    return normalize(dir);
}

// Turns a world direction into the direction to look up in the sky cubemaps.
// rotation: (sin, cos) of the sky's rotation around the up axis, from the project's sky settings.
float3 RotateSkyDirection(float3 dir, float2 rotation)
{
    return float3(dir.x * rotation.y - dir.z * rotation.x,
                  dir.y,
                  dir.x * rotation.x + dir.z * rotation.y);
}

// Where a direction lands on a 360 degree panorama (equirectangular: one full turn across, straight up at the top).
// The middle of the image faces +Z, and moving right in the image turns toward +X.
float2 DirectionToPanoramaUV(float3 dir)
{
    const float INV_PI = 0.3183098862;
    return float2(atan2(dir.x, dir.z) * 0.5 * INV_PI + 0.5,
                  acos(clamp(dir.y, -1.0, 1.0)) * INV_PI);
}

#endif // SKY_COMMON_HLSLI
