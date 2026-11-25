#pragma once

#include <vector>

#include "ModelStructs.hpp"

// What the Anime renderer needs to know about each vertex beyond the standard Vertex: its smooth normal and how
// sharply the mesh bends there. Stored in a second vertex buffer (Model::pAnimeVertexBuffer), same order as the
// vertices. Mirrors AnimeVertexInput::SmoothData in Shaders/Anime/anime_common.hlsli.
struct AnimeVertexData {
    float3 smoothNormal;    // Unit length, facing the same way as the vertex's own normal
    float curvature;        // 0 (flat) .. 1 (sharp crease)
};
static_assert(sizeof(AnimeVertexData) == 16, "AnimeVertexData must match AnimeVertexInput::SmoothData (a float4)");

// Builds the data for a whole model's vertices and indices (triangle list).
//
// Vertices at the same position are welded first, so UV seams and hard edges, where the mesh splits its vertices,
// share one smooth normal and one curvature: no cracks in the lighting band or the inner edge lines along them.
//   smooth normal: the welded vertices' area-weighted face normals
//   curvature:     how far the normals around the position differ from the neighboring positions' (the same measure
//                  as Weeb_Big2's, averaged once with the neighbors), raised to the crease's size where the vertices
//                  at one position disagree
std::vector<AnimeVertexData> BuildAnimeVertexData(const std::vector<Vertex>& vertices, const std::vector<Uint32>& indices);
