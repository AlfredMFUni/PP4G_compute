#include "utility.h"

#include <cassert> // for assert
#include <cmath>   // for std::cos, std::sin
#include <cstdarg> // for va_list, va_start
#include <fstream> // for std::ifstream

#ifndef NDEBUG
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // exclude rarely-used content from the Windows headers
#define NOMINMAX            // prevent windows macros defining their own min and max macros
#include <Windows.h>        // for DebugBreak, OutputDebugString
#endif // _WIN32
#endif // NDEBUG


//#define DPRINTF_FILE


// DEBUG

void dprintf (char const* const fmt, ...)
{
#ifndef NDEBUG
  static char buf [1 << 11] = { 0 };

  // format string
  va_list params;
  va_start (params, fmt);
  vsprintf_s (buf, fmt, params);
  va_end (params);

  // output to file
#ifdef DPRINTF_FILE
  FILE* fp = fopen ("log.txt", "a+");
  fprintf (fp, "%s", buf);
  fclose (fp);
#endif // DPRINTF_FILE

  // output to VS output window
#ifdef _WIN32
  OutputDebugString (buf);
#endif // _WIN32
#endif // NDEBUG
}

bool dbg_assert_impl (bool val,
  char const* const func, int const line, char const* const file)
{
  if (!val)
  {
    dprintf ("error: %s\nline %d - \"%s\"\n", func, line, file);

#ifdef _WIN32
    if (IsDebuggerPresent ())
    {
      DebugBreak ();
    }
    else
    {
      assert (false);
    }
#else // _WIN32
    assert (false);
#endif // _WIN32
  }

  return val;
}

bool dbg_assert_vulkan_impl (VkResult val,
  char const* const func, int const line, char const* const file)
{
  return dbg_assert_impl (val == VK_SUCCESS, func, line, file);
}


// HELPERS

bool read_file (char const* path,
  class std::vector <char>& out_buffer)
{
  // ate = seek to the end of stream immediately after open
  std::ifstream file (path, std::ios::ate | std::ios::binary);

  if (!file.is_open ())
  {
    return DBG_ASSERT_MSG (false, "failed to open file: %s\n", path);
  }

  std::size_t const file_size = (std::size_t)file.tellg ();
  out_buffer.resize (file_size);

  file.seekg (0);
  file.read (out_buffer.data (), file_size);

  file.close ();

  return true;
}
