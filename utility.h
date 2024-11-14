#pragma once

#include "maths.h"                // for standard types

#include <cstdio>                 // for std::fopen, std::fprintf, std::fclose, vsprintf_s
#include <vector>                 // for std::vector

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan


// DEBUG

void dprintf (char const* const fmt, ...);

bool dbg_assert_impl (bool val,
  char const* const func, int const line, char const* const file);

template<class...Args>
bool dbg_assert_impl_variadic (bool val,
  char const* const func, int const line, char const* const file,
  char const* const fmt, Args&&...args)
{
  if (!val)
  {
    dprintf (fmt, std::forward <Args> (args)...);
  }

  return dbg_assert_impl (val, func, line, file);
}


#define DBG_ASSERT(val) dbg_assert_impl ((val), #val, __LINE__, __FILE__)
#define DBG_ASSERT_MSG(val, errmsg, ...) dbg_assert_impl_variadic ((val), #val, __LINE__, __FILE__, errmsg, __VA_ARGS__)

#define CHECK_VULKAN_RESULT(val) (val == VK_SUCCESS)
#define CHECK_VULKAN_HANDLE(val) (val != VK_NULL_HANDLE)


// HELPERS

/// <summary>
/// read a file in to vector of chars
/// </summary>
/// <returns>false if unable to open file</returns>
bool read_file (char const* path,
  class std::vector <char>& out_buffer);

template <class T>
void write_file (char const* path,
  T const* const data,
  u32 num_elements, char const* const format, char separator = '\n')
{
  FILE* fp = std::fopen (path, "w");

  for (u32 i = 0u; i < num_elements - 1u; ++i)
  {
    std::fprintf (fp, format, data [i]);
    std::fprintf (fp, "%c", separator);
  }
  std::fprintf (fp, format, data [num_elements - 1u]);

  std::fclose (fp);
}
