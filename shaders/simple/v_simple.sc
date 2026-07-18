$input a_position, a_color
$output v_color0

#include <bgfx_shader.sh>

void main()
{
    // Transform vertex position using bgfx's built-in global view-projection matrix
    //gl_Position = mul(u_viewProj, vec4(a_position, 1.0));

    gl_Position = vec4(a_position, 1.0);
    
    // Forward the vertex color to the fragment shader
    v_color0 = a_color;
}