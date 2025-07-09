#include "Box.hpp"

#include "Model.hpp"

using namespace glm;

void Assets::Box::Create(const glm::vec3& p0, const glm::vec3& p1, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
{
    vec3 center = (p0 + p1) * 0.5f;

    vertices =
    {
        Vertex{vec3(p0.x, p0.y, p0.z) - center, vec3(-1, 0, 0), vec2(0), 0},
        Vertex{vec3(p0.x, p0.y, p1.z) - center, vec3(-1, 0, 0), vec2(0,1), 0},
        Vertex{vec3(p0.x, p1.y, p1.z) - center, vec3(-1, 0, 0), vec2(1,1), 0},
        Vertex{vec3(p0.x, p1.y, p0.z) - center, vec3(-1, 0, 0), vec2(1,0), 0},

        Vertex{vec3(p1.x, p0.y, p1.z) - center, vec3(1, 0, 0), vec2(0), 0},
        Vertex{vec3(p1.x, p0.y, p0.z) - center, vec3(1, 0, 0), vec2(0,1), 0},
        Vertex{vec3(p1.x, p1.y, p0.z) - center, vec3(1, 0, 0), vec2(1,1), 0},
        Vertex{vec3(p1.x, p1.y, p1.z) - center, vec3(1, 0, 0), vec2(1,0), 0},

        Vertex{vec3(p1.x, p0.y, p0.z) - center, vec3(0, 0, -1), vec2(0), 0},
        Vertex{vec3(p0.x, p0.y, p0.z) - center, vec3(0, 0, -1), vec2(0,1), 0},
        Vertex{vec3(p0.x, p1.y, p0.z) - center, vec3(0, 0, -1), vec2(1,1), 0},
        Vertex{vec3(p1.x, p1.y, p0.z) - center, vec3(0, 0, -1), vec2(1,0), 0},

        Vertex{vec3(p0.x, p0.y, p1.z) - center, vec3(0, 0, 1), vec2(0), 0},
        Vertex{vec3(p1.x, p0.y, p1.z) - center, vec3(0, 0, 1), vec2(0,1), 0},
        Vertex{vec3(p1.x, p1.y, p1.z) - center, vec3(0, 0, 1), vec2(1,1), 0},
        Vertex{vec3(p0.x, p1.y, p1.z) - center, vec3(0, 0, 1), vec2(1,0), 0},

        Vertex{vec3(p0.x, p0.y, p0.z) - center, vec3(0, -1, 0), vec2(0), 0},
        Vertex{vec3(p1.x, p0.y, p0.z) - center, vec3(0, -1, 0), vec2(0,1), 0},
        Vertex{vec3(p1.x, p0.y, p1.z) - center, vec3(0, -1, 0), vec2(1,1), 0},
        Vertex{vec3(p0.x, p0.y, p1.z) - center, vec3(0, -1, 0), vec2(1,0), 0},

        Vertex{vec3(p1.x, p1.y, p0.z) - center, vec3(0, 1, 0), vec2(0), 0},
        Vertex{vec3(p0.x, p1.y, p0.z) - center, vec3(0, 1, 0), vec2(0,1), 0},
        Vertex{vec3(p0.x, p1.y, p1.z) - center, vec3(0, 1, 0), vec2(1,1), 0},
        Vertex{vec3(p1.x, p1.y, p1.z) - center, vec3(0, 1, 0), vec2(1,0), 0},
    };
 

    indices =
    {
        0, 1, 2, 0, 2, 3,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };
}
