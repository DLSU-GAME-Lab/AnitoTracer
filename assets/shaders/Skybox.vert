#version 460

layout(location = 0) out vec3 fragTexCoord;

vec3 positions[36] = vec3[](
    // Front face
    vec3(-1, -1,  1), vec3( 1, -1,  1), vec3( 1,  1,  1),
    vec3( 1,  1,  1), vec3(-1,  1,  1), vec3(-1, -1,  1),

    // Back face
    vec3(-1, -1, -1), vec3( 1,  1, -1), vec3( 1, -1, -1),
    vec3( 1,  1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),

    // Left face
    vec3(-1,  1, -1), vec3(-1, -1, -1), vec3(-1,  1,  1),
    vec3(-1, -1,  1), vec3(-1,  1,  1), vec3(-1, -1, -1),

    // Right face
    vec3( 1,  1,  1), vec3( 1, -1, -1), vec3( 1,  1, -1),
    vec3( 1, -1, -1), vec3( 1,  1,  1), vec3( 1, -1,  1),

    // Top face 
    vec3(-1,  1, -1), vec3( 1,  1,  1), vec3( 1,  1, -1),
    vec3( 1,  1,  1), vec3(-1,  1, -1), vec3(-1,  1,  1),

    // Bottom face 
    vec3(-1, -1, -1), vec3( 1, -1, -1), vec3( 1, -1,  1),
    vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, -1, -1)
);


layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    vec3 pos = positions[gl_VertexIndex];
    
    mat4 rotView = mat4(mat3(ubo.view));
    
    vec4 worldPos = rotView * vec4(pos, 0.0);
    fragTexCoord = pos;
    
    gl_Position = ubo.proj * worldPos;
    gl_Position.z = gl_Position.w; // set depth to max (far plane)
}
