#include "maths.h"

#include <cmath>   // for std::sqrtf
#include <limits>  // for std::numeric_limits <T>::epsilon


int_rect convert_rect (float_rect const& a)
{
  return {
    (i32)a.left, (i32)a.top,
    (i32)a.width, (i32)a.height
  };
}
float_rect convert_rect (int_rect const& a)
{
  return {
    (f32)a.left, (f32)a.top,
    (f32)a.width, (f32)a.height
  };
}

vec2 vec2_div (vec2 const& a, vec2 const& b)
{
  return { a.x / b.x, a.y / b.y };
}
vec2 vec2_div (vec2 const& a, f32 S)
{
  return { a.x / S, a.y / S };
}

f32 vec2_magnitude_squared (vec2 const& a)
{
  f32 const x = a.x;
  f32 const y = a.y;
  return x * x + y * y;
}
f32 vec2_magnitude (vec2 const& a)
{
  return std::sqrtf (vec2_magnitude_squared (a));
}

f32 vec2_distance_squared (vec2 const& a, vec2 const& b)
{
  return vec2_magnitude_squared (a - b);
}
f32 vec2_distance (vec2 const& a, vec2 const& b)
{
  return vec2_magnitude (a - b);
}

f32 vec2_normalise (vec2& a)
{
  f32 length = vec2_magnitude (a);
  if (length >= std::numeric_limits <float>::epsilon ())
  {
    f32 const inverse_length = 1.f / length;
    a.x *= inverse_length;
    a.y *= inverse_length;
  }
  else
  {
    length = 0.f;
  }

  return length;
}
vec2 vec2_set_magnitude (vec2 const& a, f32 length)
{
  vec2 result = a;
  vec2_normalise (result);
  return result * length;
}
vec2 vec2_limit (vec2 const& a, f32 max_length)
{
  f32 const mSq = vec2_magnitude_squared (a);
  return mSq > (max_length * max_length)
    ? vec2_set_magnitude (a, max_length)
    : a;
}

mat4 fast_transform_2D (f32 position_x, f32 position_y,
  f32 angle_degrees,
  f32 origin_x, f32 origin_y,
  f32 scale_x, f32 scale_y)
{
  // simulate the following glm code
  // glm::mat4 model_matrix = glm::identity <mat4> ();
  // model_matrix = glm::translate (model_matrix, glm::vec3 (position_x, position_y, 0.f));
  // model_matrix = glm::rotate (model_matrix, radians (angle_degrees), glm::vec3 (0.f, 0.f, 1.f)); // rotate about z axis
  // model_matrix = glm::translate (model_matrix, glm::vec3 (-origin_x, -origin_y, 0.f));
  // model_matrix = glm::scale (model_matrix, glm::vec3 (scale_x, scale_y, 1.f));
  //
  // but this is VERY SLOW, so use knowledge of 2D matrix math to manually create the resultant matrix
  // we need the result to be a mat4 because the sprite is technically in 3D space, we just draw it with a 3D orthographic camera
  // this glm code is for 3D objects but we only want a 2D transformation
  // so we can acheive great speed ups by taking advantage of this and not having to do unnecassary calculations for the z-axis which we know will be null anyway
  // rotate is particularly slow because it supports rotation about an arbitary axis, but we only want it about the z axis!

#define FAST
#ifdef FAST
  f32 const a = glm::radians (angle_degrees);
  f32 const c = std::cos (a);
  f32 const s = std::sin (a);

  mat4 model_matrix = {};
  f32* mat = (f32*)(&model_matrix);

  // translate -> postion
  // rotate -> about z axis

  mat [0] = c;
  mat [1] = -s;
  mat [2] = 0.f;
  mat [3] = 0.f;

  mat [4] = s;
  mat [5] = -c;
  mat [6] = 0.f;
  mat [7] = 0.f;

  mat [8] = 0.f;
  mat [9] = 0.f;
  mat [10] = 1.f;
  mat [11] = 0.f;

  mat [12] = position_x;
  mat [13] = position_y;
  mat [14] = 0.f;
  mat [15] = 1.f;

  // translate -> origin

  mat [12] = mat [0] * -origin_x + mat [1] * -origin_y + mat [12];
  mat [13] = mat [4] * -origin_x + mat [5] * origin_y + mat [13];

  // scale

  mat [0] = mat [0] * scale_x;
  mat [1] = mat [1] * -scale_x;

  mat [4] = mat [4] * -scale_y;
  mat [5] = mat [5] * -scale_y;
#else // FAST
  // #include <glm/ext/matrix_transform.hpp> // add above

  glm::mat4 model_matrix = glm::identity <mat4> ();
  model_matrix = glm::translate (model_matrix, glm::vec3 (position_x, position_y, 0.f));
  model_matrix = glm::rotate (model_matrix, radians (angle_degrees), glm::vec3 (0.f, 0.f, 1.f)); // rotate about z axis
  model_matrix = glm::translate (model_matrix, glm::vec3 (-origin_x, -origin_y, 0.f));
  model_matrix = glm::scale (model_matrix, glm::vec3 (scale_x, scale_y, 1.f));
#endif // FAST

  return model_matrix;
}
