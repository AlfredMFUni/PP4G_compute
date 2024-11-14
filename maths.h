#pragma once

#include <glm/glm.hpp>                  // for glm types
#include <glm/gtc/matrix_transform.hpp> // for glm::ortho...


using u16 = glm::u16;
using i32 = glm::i32;
using u32 = glm::u32;
using f32 = glm::f32;
using vec2 = glm::vec2;
using uvec2 = glm::uvec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;


struct int_rect
{
  i32 left;
  i32 top;
  i32 width;
  i32 height;
};
using texture_rect = int_rect;

struct uint_rect
{
  u32 left;
  u32 top;
  u32 width;
  u32 height;
};

struct float_rect
{
  f32 left;
  f32 top;
  f32 width;
  f32 height;
};


int_rect convert_rect (float_rect const& a);
float_rect convert_rect (int_rect const& a);

vec2 vec2_div (vec2 const& a, vec2 const& b);
vec2 vec2_div (vec2 const& a, f32 S);

f32 vec2_magnitude_squared (vec2 const& a);
f32 vec2_magnitude (vec2 const& a);

f32 vec2_distance_squared (vec2 const& a, vec2 const& b);
f32 vec2_distance (vec2 const& a, vec2 const& b);

f32 vec2_normalise (vec2& a);
vec2 vec2_set_magnitude (vec2 const& a, f32 length);
vec2 vec2_limit (vec2 const& a, f32 max_length);

/// <summary>
/// glm matrix maths is slow, so manually create the matrix instead
/// </summary>
/// <returns>mat4x4 containing 2D transformation matrix for the given inputs</returns>
mat4 fast_transform_2D (f32 position_x, f32 position_y,
  f32 angle_degrees,
  f32 origin_x, f32 origin_y,
  f32 scale_x, f32 scale_y);
