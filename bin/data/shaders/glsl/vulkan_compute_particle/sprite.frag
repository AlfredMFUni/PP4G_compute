// frag shader
#version 430


layout (set = 1, binding = 0) uniform sampler2D tex_sampler;


// input
layout (location = 0) in vec2 in_tex_coord;

// output
layout (location = 0) out vec4 frag_colour;


void main ()
{

  frag_colour = texture (tex_sampler, in_tex_coord);
}
