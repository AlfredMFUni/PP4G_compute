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

layout (set = 0, binding = 2) uniform info_buffer
{
  uint num_elements;
  float window_width;
  float window_height;
} UBO_info;

layout (std430, set = 0, binding = 3) buffer pos_x_buffer
{
  float data [];
} SBO_pos_x;

layout (std430, set = 0, binding = 4) buffer pos_y_buffer
{
  float data [];
} SBO_pos_y;



// input
layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_tex_coord;

// output
layout (location = 0) out vec2 out_tex_coord;


void main ()
{
  vec2 instancePos = vec2(SBO_pos_x.data[gl_InstanceIndex] / UBO_info.window_width, SBO_pos_y.data[gl_InstanceIndex] / UBO_info.window_height);

  gl_Position = (UBO_camera.vp_matrix * UBO_model.model_matrix * vec4 ((in_position + instancePos), 0.0, 1.0));

  out_tex_coord = in_tex_coord;
}
