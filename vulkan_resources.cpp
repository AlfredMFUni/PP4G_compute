#include "vulkan_resources.h"

#include "vulkan_context.h"  // begin_single_time_commands, ...
#include "vulkan_pipeline.h" // begin_command_buffer, ...
#include "utility.h"         // DBG_ASSERT, DBG_ASSERT_VULKAN

#include <cstring> // for std::memcpy

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>            // for stbi_load, stbi_image_free


#pragma region vulkan_buffer_support
static u32 find_suitable_memory_type_index (VkPhysicalDevice physical_device,
  VkMemoryPropertyFlags desired_memory_flags, u32 memory_type_bits)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));


  // memory_type_bits is a bitfield where if bit n is set, it means that
  // the VkMemoryType n of the VkPhysicalDeviceMemoryProperties structure
  // satisfies the memory requirements.

  VkPhysicalDeviceMemoryProperties memory_properties = {};
  vkGetPhysicalDeviceMemoryProperties (physical_device, // physcalDevice
    &memory_properties);                                // pMemoryProperties

  for (uint32_t i = 0u; i < VK_MAX_MEMORY_TYPES; ++i)
  {
    VkMemoryType memory_type = memory_properties.memoryTypes [i];
    if (memory_type_bits & 1u)
    {
      if ((memory_type.propertyFlags & desired_memory_flags) == desired_memory_flags)
      {
        return i; // memory type index
      }
    }

    memory_type_bits = memory_type_bits >> 1;
  }

  DBG_ASSERT_MSG (false, "could not find suitable memory type\n");
  return ~0u;
}

static bool create_buffer (VkDevice device,
  VkBufferCreateInfo const& bci,
  VkBuffer& out_buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_buffer));


  VkResult const result = vkCreateBuffer (device, // device
    &bci,                                         // pCreateInfo
    VK_NULL_HANDLE,                               // pAllocator
    &out_buffer);                                 // pBuffer
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_buffer))
  {
    return DBG_ASSERT_MSG (false, "failed to create buffer\n");
  }

  return true;
}
static bool allocate_buffer_memory (VkPhysicalDevice physical_device, VkDevice device,
  VkBuffer buffer,
  VkMemoryPropertyFlags desired_memory_flags, void const* extension_settings,
  VkDeviceMemory& out_memory)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_memory));


  VkMemoryRequirements memory_requirements = {};
  vkGetBufferMemoryRequirements (device, // device
    buffer,                              // buffer
    &memory_requirements);               // pMemoryRequirements

  VkMemoryAllocateInfo const mai =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    .pNext = extension_settings,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = find_suitable_memory_type_index (physical_device, desired_memory_flags, memory_requirements.memoryTypeBits)
  };

  VkResult const result = vkAllocateMemory (device, // device
    &mai,                                           // pAllocateInfo
    nullptr,                                        // pAllocator
    &out_memory);                                   // pMemory
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_memory))
  {
    return DBG_ASSERT_MSG (false, "failed to allocate device memory for buffer\n");
  }

  return true;
}
static bool bind_buffer_to_memory (VkDevice device,
  VkBuffer buffer, VkDeviceMemory memory)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (memory));


  VkResult const result = vkBindBufferMemory (device, // device
    buffer,                                           // buffer
    memory,                                           // memory
    0u);                                              // memoryOffset
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "failed to bind buffer to memory\n");
  }

  return true;
}

static bool copy_buffer_to_buffer (VkBuffer buffer_a, VkBuffer buffer_b, VkDeviceSize size)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer_a));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer_b));


  VkCommandBuffer cb = VK_NULL_HANDLE;
  if (!begin_single_time_commands (cb))
  {
    return false;
  }
  if (!begin_command_buffer (cb, 0u))
  {
    return false;
  }


  VkBufferCopy const copy_region =
  {
    .srcOffset = 0u, // optional
    .dstOffset = 0u, // optional
    .size = size
  };
  vkCmdCopyBuffer (cb, buffer_a, buffer_b, 1u, &copy_region);


  if (!end_command_buffer (cb))
  {
    return false;
  }
  return end_single_time_commands ();
}
static bool set_device_buffer (VkPhysicalDevice physical_device, VkDevice device,
  VkBuffer device_buffer, VkDeviceSize size, std::function <void (void*)> func)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device_buffer));


  // using the 'staging buffer' approach allows us to have our final buffer have the most optimal properties
  // i.e. device accessible only, NOT host visible!

  vulkan_buffer staging_buffer;

  if (!create_vulkan_buffer (physical_device, device,
    size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    staging_buffer))
  {
    return false;
  }

  if (!map_and_unmap_memory (device, staging_buffer.memory, func))
  {
    return false;
  }

  if (!copy_buffer_to_buffer (staging_buffer.buffer, device_buffer, size))
  {
    return false;
  }

  release_vulkan_buffer (device, staging_buffer);

  return true;
}
#pragma endregion


bool create_vulkan_buffer (VkPhysicalDevice physical_device, VkDevice device,
  VkDeviceSize size, VkBufferUsageFlags buffer_usage_flags, VkSharingMode sharing_mode, VkMemoryPropertyFlags desired_memory_flags,
  vulkan_buffer& out_buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_buffer.buffer));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_buffer.memory));


  VkBufferCreateInfo const bci =
  {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .size = size,
    .usage = buffer_usage_flags,
    .sharingMode = sharing_mode,
    //.queueFamilyIndexCount = 0u,
    //.pQueueFamilyIndices = VK_NULL_HANDLE
  };

  if (!create_buffer (device, bci, out_buffer.buffer))
  {
    return false;
  }

  if (!allocate_buffer_memory (physical_device, device,
    out_buffer.buffer,
    desired_memory_flags, nullptr,
    out_buffer.memory))
  {
    return false;
  }

  out_buffer.size = size;

  return bind_buffer_to_memory (device, out_buffer.buffer, out_buffer.memory);
}

bool map_and_unmap_memory (VkDevice device,
  VkDeviceMemory memory, std::function <void (void*)> func)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (memory));


  void* mapped_memory = nullptr;
  VkResult const result = vkMapMemory (device, // device
    memory,                                    // memory
    0u,                                        // offset
    VK_WHOLE_SIZE,                             // size
    0u,                                        // flags
    &mapped_memory);                           // ppData
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "failed to map buffer memory\n");
  }

  func (mapped_memory);

  vkUnmapMemory (device, memory);

  return true;
}


#pragma region vulkan_texture_support
static bool create_image_2d (VkDevice device,
  VkExtent2D const& size,
  VkFormat image_format, VkImageUsageFlags image_usage_flags, VkImageLayout initial_image_layout,
  VkImageTiling image_tiling,
  VkImage& out_image)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_image));


  // TODO: ask Andy for graphics starter code

  return true;
}
static bool allocate_image_memory (VkPhysicalDevice physical_device, VkDevice device,
  VkImage image, VkMemoryPropertyFlags desired_memory_flags,
  VkDeviceSize& out_size, VkDeviceMemory& out_memory)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (image));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_memory));


  VkMemoryRequirements memory_requirements = {};
  vkGetImageMemoryRequirements (device, // device
    image,                              // image
    &memory_requirements);              // pMemoryRequirements

  VkMemoryAllocateInfo const memory_allocate_info =
  {
    .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    .allocationSize = memory_requirements.size,
    .memoryTypeIndex = find_suitable_memory_type_index (physical_device,
      desired_memory_flags, memory_requirements.memoryTypeBits)
  };

  VkResult const result = vkAllocateMemory (device, // device
    &memory_allocate_info,                          // pAllocateInfo
    nullptr,                                        // pAllocator
    &out_memory);                                   // pMemory
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_memory))
  {
    return DBG_ASSERT_MSG (false, "failed to allocate device memory for image\n");
  }

  out_size = memory_requirements.size;

  return true;
}
static bool bind_image_to_memory (VkDevice device,
  VkImage image, VkDeviceMemory memory)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (image));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (memory));


  VkResult const result = vkBindImageMemory (device, // device
    image,                                           // image
    memory,                                          // memory
    0u);                                             // memoryOffset
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "failed to bind image to memory\n");
  }

  return true;
}

static bool copy_buffer_to_image (VkBuffer buffer, VkImage image, VkExtent2D const& size)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (image));


  VkCommandBuffer cb = VK_NULL_HANDLE;
  if (!begin_single_time_commands (cb))
  {
    return false;
  }
  if (!begin_command_buffer (cb, 0u))
  {
    return false;
  }

  VkImageSubresourceLayers const image_subresource =
  {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .mipLevel = 0u,
    .baseArrayLayer = 0u,
    .layerCount = 1u
  };
  VkBufferImageCopy const buffer_image_copy =
  {
    .bufferOffset = 0u,
    .bufferRowLength = 0u,
    .bufferImageHeight = 0u,
    .imageSubresource = image_subresource,
    .imageOffset = { 0u, 0u, 0u },
    .imageExtent = { size.width, size.height, 1u }
  };
  vkCmdCopyBufferToImage (cb, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1u, &buffer_image_copy);

  if (!end_command_buffer (cb))
  {
    return false;
  }

  return end_single_time_commands ();
}

static bool create_vulkan_image_view (VkDevice device,
  VkImageViewCreateInfo const& ivci,
  VkImageView& out_image_view)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_image_view));


  VkResult const result = vkCreateImageView (device, // device
    &ivci,                                           // pCreateInfo
    VK_NULL_HANDLE,                                  // pAllocator
    &out_image_view);                                // pView
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_image_view))
  {
    return DBG_ASSERT_MSG (false, "failed to create image view\n");
  }

  return true;
}

static bool create_vulkan_sampler (VkDevice device,
  VkSamplerCreateInfo const& sci,
  VkSampler& out_sampler)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_sampler));


  VkResult const result = vkCreateSampler (device, // device
    &sci,                                          // pCreateInfo
    VK_NULL_HANDLE,                                // pAllocator
    &out_sampler);                                 // pSampler
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_sampler))
  {
    return DBG_ASSERT_MSG (false, "failed to create sampler\n");
  }

  return true;
}
static bool create_vulkan_sampler_basic (VkDevice device,
  VkSampler& out_sampler)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_sampler));


  //VkPhysicalDeviceFeatures supported_device_features = {};
  //vkGetPhysicalDeviceFeatures (physical_device, &supported_device_features);
  //VkPhysicalDeviceProperties supported_device_properties = {};
  //vkGetPhysicalDeviceProperties (physical_device, &supported_device_properties);

  VkSamplerCreateInfo const sci =
  {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_FALSE,
    //.anisotropyEnable = supported_device_features.samplerAnisotropy,
    //.maxAnisotropy = supported_device_features.samplerAnisotropy == VK_TRUE ? supported_device_properties.limits.maxSamplerAnisotropy : 1.f,
    .compareEnable = VK_FALSE,
    .compareOp = VK_COMPARE_OP_ALWAYS,
    .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
    .unnormalizedCoordinates = VK_FALSE
  };
  return create_vulkan_sampler (device, sci, out_sampler);
}
static void release_vulkan_sampler (VkDevice device, VkSampler& sampler)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (sampler));


  vkDestroySampler (device, sampler, VK_NULL_HANDLE);
  sampler = VK_NULL_HANDLE;
}
#pragma endregion


bool create_vulkan_texture (VkPhysicalDevice physical_device, VkDevice device,
  char const* texture_path,
  VkFormat image_format, VkImageLayout image_layout, VkImageUsageFlags image_usage_flags,
  vulkan_texture& out_texture)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (texture_path != nullptr);
  DBG_ASSERT (image_format != VK_FORMAT_UNDEFINED);


  // TODO: ask Andy for graphics starter code

  return true;
}
bool create_vulkan_texture_empty (VkPhysicalDevice physical_device, VkDevice device,
  uvec2 const& dim,
  VkFormat image_format, VkImageLayout image_layout, VkImageUsageFlags image_usage_flags,
  vulkan_texture& out_texture)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (image_format != VK_FORMAT_UNDEFINED);


  // TODO: ask Andy for graphics starter code

  return true;
}

bool transition_image_layout (VkCommandBuffer command_buffer,
  VkImage image, VkImageLayout old_layout, VkImageLayout new_layout,
  VkPipelineStageFlags src_stage_mask/* = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT*/,
  VkPipelineStageFlags dst_stage_mask/* = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT*/,
  VkImageAspectFlags aspect_mask/* = VK_IMAGE_ASPECT_COLOR_BIT*/)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (image));
  DBG_ASSERT (new_layout != VK_IMAGE_LAYOUT_UNDEFINED);


  // there is no straight forward way to transition an image layout type
  // however, a function of vkCmdPipelineBarrier gives us the ability to do so
  // usually vkCmdPipelineBarrier is submitted to a queue when we want to define a memory dependancy between
  // the commands prior to it on the queue with those after it
  // e.g. image x must be complete before continuing, and change its layout before continuing
  // we are however not in a frame, so we will manually create a command buffer
  // insert a vkCmdPipelineBarrier command
  // end the buffer
  // then submit it to the GPU and wait for it to complete!

  VkImageSubresourceRange const subresource_range =
  {
    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .baseMipLevel = 0u,
    .levelCount = 1u,
    .baseArrayLayer = 0u,
    .layerCount = 1u
  };
  VkImageMemoryBarrier image_memory_barrier =
  {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    //.pNext = VK_NULL_HANDLE,
    //.srcAccessMask = ...,
    //.dstAccessMask = ...,
    .oldLayout = old_layout,
    .newLayout = new_layout,
    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
    .image = image,
    .subresourceRange = subresource_range
  };

  // see https://github.com/SaschaWillems/Vulkan -> VulkanTools.cpp

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old layout
  // before it will be transitioned to the new layout
  switch (old_layout)
  {
    case VK_IMAGE_LAYOUT_UNDEFINED:
      // Image layout is undefined (or does not matter)
      // Only valid as initial layout
      // No flags required, listed only for completeness
      image_memory_barrier.srcAccessMask = 0u;
      break;

    case VK_IMAGE_LAYOUT_PREINITIALIZED:
      // Image is preinitialized
      // Only valid as initial layout for linear images, preserves memory contents
      // Make sure host writes have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      // Image is a color attachment
      // Make sure any writes to the color buffer have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      // Image is a depth/stencil attachment
      // Make sure any writes to the depth/stencil buffer have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      // Image is a transfer source
      // Make sure any reads from the image have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      // Image is a transfer destination
      // Make sure any writes to the image have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      // Image is read by a shader
      // Make sure any shader reads from the image have been finished
      image_memory_barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      dprintf ("transition_image_layout: old layout: potentially unsupported layout transition!\n");
      break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (new_layout)
  {
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
      // Image will be used as a transfer destination
      // Make sure any writes to the image have been finished
      image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
      // Image will be used as a transfer source
      // Make sure any reads from the image have been finished
      image_memory_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      // Image will be used as a color attachment
      // Make sure any writes to the color buffer have been finished
      image_memory_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      // Image layout will be used as a depth/stencil attachment
      // Make sure any writes to depth/stencil buffer have been finished
      image_memory_barrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      // Image will be read in a shader (sampler, input attachment)
      // Make sure any writes to the image have been finished
      if (image_memory_barrier.srcAccessMask == 0u)
      {
        image_memory_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      image_memory_barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
       dprintf ("transition_image_layout: new layout: potentially unsupported layout transition!\n");
      break;
  }

  vkCmdPipelineBarrier (command_buffer, // commandBuffer
    src_stage_mask,                     // srcStageMasks
    dst_stage_mask,                     // dstStageMasks
    0u,                                 // dependencyFlags
    0u,                                 // memoryBarrierCount
    VK_NULL_HANDLE,                     // pMemoryBariers
    0u,                                 // bufferMemoryBarrierCount
    VK_NULL_HANDLE,                     // pBufferMemoryBarriers
    1u,                                 // imageMemoryBarrierCount
    &image_memory_barrier);             // pImageMemoryBarriers

  return true;
}
bool transition_image_layout (VkImageLayout new_layout, vulkan_texture& texture)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (texture.image));
  DBG_ASSERT (new_layout != VK_IMAGE_LAYOUT_UNDEFINED);


  VkCommandBuffer cb = VK_NULL_HANDLE;
  if (!begin_single_time_commands (cb))
  {
    return false;
  }
  if (!begin_command_buffer (cb, 0u))
  {
    return false;
  }

  if (!transition_image_layout (cb,
    texture.image, texture.layout, new_layout))
  {
    return false;
  }

  if (!end_command_buffer (cb))
  {
    return false;
  }
  if (!end_single_time_commands ())
  {
    return false;
  }

  texture.layout = new_layout;

  return true;
}


bool create_vulkan_mesh (VkPhysicalDevice physical_device, VkDevice device,
  void* vertices, u32 num_vertices, u32 vertex_size,
  void* indices, u32 num_indices, u32 index_size,
  vulkan_mesh& out_mesh)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (vertices != nullptr);
  DBG_ASSERT (num_vertices > 0u);
  DBG_ASSERT (vertex_size > 0u);
  DBG_ASSERT (indices != nullptr);
  DBG_ASSERT (num_indices > 0u);
  DBG_ASSERT (index_size > 0u);
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_mesh.buffer_vertex));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_mesh.memory_vertex));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_mesh.buffer_index));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_mesh.memory_index));


  // TODO: ask Andy for graphics starter code

  return true;
}


void release_vulkan_mesh (VkDevice device, vulkan_mesh& mesh)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (mesh.buffer_vertex));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (mesh.memory_vertex));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (mesh.buffer_index));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (mesh.memory_index));


  vkFreeMemory (device, mesh.memory_index, VK_NULL_HANDLE);
  mesh.memory_index = VK_NULL_HANDLE;

  vkDestroyBuffer (device, mesh.buffer_index, VK_NULL_HANDLE);
  mesh.buffer_index = VK_NULL_HANDLE;

  vkFreeMemory (device, mesh.memory_vertex, VK_NULL_HANDLE);
  mesh.memory_vertex = VK_NULL_HANDLE;

  vkDestroyBuffer (device, mesh.buffer_vertex, VK_NULL_HANDLE);
  mesh.buffer_vertex = VK_NULL_HANDLE;

  mesh = {};
}
void release_vulkan_image_view (VkDevice device, VkImageView& image_view)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (image_view));


  vkDestroyImageView (device, image_view, VK_NULL_HANDLE);
  image_view = VK_NULL_HANDLE;
}
void release_vulkan_texture (VkDevice device, vulkan_texture& texture)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (texture.image));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (texture.memory));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (texture.sampler));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (texture.view));


  release_vulkan_image_view (device, texture.view);
  release_vulkan_sampler (device, texture.sampler);

  vkDestroyImage (device, texture.image, VK_NULL_HANDLE);
  texture.image = VK_NULL_HANDLE;

  vkFreeMemory (device, texture.memory, VK_NULL_HANDLE);
  texture.memory = VK_NULL_HANDLE;

  texture = {};
}
void release_vulkan_buffer (VkDevice device, vulkan_buffer& buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer.buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (buffer.memory));


  vkFreeMemory (device, buffer.memory, VK_NULL_HANDLE);
  buffer.memory = VK_NULL_HANDLE;

  vkDestroyBuffer (device, buffer.buffer, VK_NULL_HANDLE);
  buffer.buffer = VK_NULL_HANDLE;

  buffer = {};
}
