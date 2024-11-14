#pragma once

#include "maths.h"                // for standard types

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan

#include <functional> // for std::function


/// <summary>
/// convenience object to group a buffer object
/// and associated data
/// </summary>
struct vulkan_buffer
{
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkDeviceSize size = {};
};

/// <summary>
/// convenience object to group a texture object
/// and associated data
/// </summary>
struct vulkan_texture
{
  VkImage image = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;
  VkSampler sampler = VK_NULL_HANDLE;
  VkImageView view = VK_NULL_HANDLE;

  uvec2 dim = {};
  VkDeviceSize size = {}; // of memory allocation, not necassarilly the same as rgba8 * dim
  VkFormat format = VK_FORMAT_UNDEFINED;
  VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

/// <summary>
/// convenience object to group the buffers required to store a meshes vertex and index data
/// and associated data
/// </summary>
struct vulkan_mesh
{
  u32 num_vertices = {}, num_triangles = {};
  VkBuffer buffer_vertex = VK_NULL_HANDLE;
  VkDeviceMemory memory_vertex = VK_NULL_HANDLE;

  u32 num_indices = {};
  VkBuffer buffer_index = VK_NULL_HANDLE;
  VkDeviceMemory memory_index = VK_NULL_HANDLE;
};


/// <summary>
/// create a 'standard' vulkan buffer
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_buffer (VkPhysicalDevice physical_device, VkDevice device,
  VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkSharingMode sharing_mode, VkMemoryPropertyFlags desired_memory_flags,
  vulkan_buffer& out_buffer);

/// <summary>
/// map memory, pass pointer of mapped memory to user defined function, unmap memory
/// </summary>
/// <returns>true, if successful</returns>
bool map_and_unmap_memory (VkDevice device,
  VkDeviceMemory memory, std::function <void (void*)> func);


/// <summary>
/// create a vulkan texture from a source image file
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_texture (VkPhysicalDevice physical_device, VkDevice device,
  char const* texture_path,
  VkFormat image_format, VkImageLayout image_layout, VkImageUsageFlags image_usage_flags,
  vulkan_texture& out_texture);
/// <summary>
/// create an empty vulkan texture
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_texture_empty (VkPhysicalDevice physical_device, VkDevice device,
  uvec2 const& dim,
  VkFormat image_format, VkImageLayout image_layout, VkImageUsageFlags image_usage_flags,
  vulkan_texture& out_texture);

/// <summary>
/// create a 'standard' vulkan image view of a 2D image
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_image_view_2d_basic (VkDevice device,
  VkImage image, VkFormat image_format, VkImageAspectFlags aspect_mask,
  VkImageView& out_image_view);

/// <summary>
/// add the required commands to a vulkan command buffer to convert an image's layout type, via 'vkCmdPipelineBarrier'
/// </summary>
/// <returns>true, if successful</returns>
bool transition_image_layout (VkCommandBuffer command_buffer,
  VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
  VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
  VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
  VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT);
/// <summary>
/// manually convert an image's layout type
/// outside of an existing command buffer submission
/// </summary>
/// <returns>true, if successful</returns>
bool transition_image_layout (VkImageLayout new_layout, vulkan_texture& texture);


/// <summary>
/// create a 'standard' vulkan mesh
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_mesh (VkPhysicalDevice physical_device, VkDevice device,
  void* vertices, u32 num_vertices, u32 vertex_size,
  void* indices, u32 num_indices, u32 index_size,
  vulkan_mesh& out_mesh);


void release_vulkan_mesh (VkDevice device, vulkan_mesh& mesh);
void release_vulkan_image_view (VkDevice device, VkImageView& image_view);
void release_vulkan_texture (VkDevice device, vulkan_texture& texture);
void release_vulkan_buffer (VkDevice device, vulkan_buffer& buffer);
