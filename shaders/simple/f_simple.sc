$input

#include <bgfx_shader.sh>

// Define a uniform parameter to pass color from C++ code (4 floats: R, G, B, A)
uniform vec4 u_color;

void main()
{
    // Output the solid color assigned via the uniform
    gl_FragColor = u_color;
}