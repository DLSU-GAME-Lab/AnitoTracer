$input v_color0

#include <bgfx_shader.sh>

void main()
{
    // Output the solid color assigned via the uniform
    gl_FragColor = v_color0;
}