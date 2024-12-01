// vertex shader
#version 430


layout (set = 0, binding = 0) uniform camera
{
  mat4 vp_matrix;
} UBO_camera;

layout (set = 0, binding = 1) uniform model
{
  mat4 model_matrix;
} UBO_model;


// input
layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_tex_coord;

// output
layout (location = 0) out vec2 out_tex_coord;


void main ()
{
//instanceIndex
  gl_Position = (UBO_camera.vp_matrix * UBO_model.model_matrix * vec4 (in_position.x - 1.0,  in_position.y - 1.0, 0.0, 1.0));// - vec4(0.9, 0.9, 0.0, 0.0));

  out_tex_coord = in_tex_coord;
}
