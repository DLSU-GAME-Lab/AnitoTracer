$input a_position
$output

#include <bgfx_shader.sh>

void main()
{
    // Transform vertex position using bgfx's built-in global view-projection matrix
    gl_Position = mul(u_viewProj, vec4(a_position, 1.0));
}