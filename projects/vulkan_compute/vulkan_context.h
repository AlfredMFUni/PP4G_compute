#pragma once

#include "maths.h"                // for standard types

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>           // for glfw...


constexpr u32 NUM_SWAPCHAIN_IMAGES = 2u; // vulkan requires at least 2 swapchain images


/// <summary>
/// create window (using glfw)
/// </summary>
/// <returns>true, if successful</returns>
bool create_window (char const*const title);
/// <summary>
/// create vulkan instance and debug messenger
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_instance ();
/// <summary>
/// create vulkan surface (using glfw)
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_surface ();
/// <summary>
/// create vulkan physical & logical device
/// </summary>
/// <param name="requested_queue_types">types of queues you want the device to support</param>
/// <returns>true, if successful</returns>
bool create_vulkan_device (VkQueueFlags requested_queue_types,
  VkPhysicalDevice& out_physical_device, VkDevice& out_device);
/// <summary>
/// create vulkan swapchain, framebuffer image views, render pass and frame buffers
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_swapchain (VkExtent2D& out_extent, VkRenderPass& out_render_pass);

/// <summary>
/// get queue to submit compute command buffers to
/// </summary>
/// <returns>true, if successful</returns>
bool get_vulkan_queue_compute (VkQueue& out_queue);
/// <summary>
/// get queue to submit graphics command buffers to
/// </summary>
/// <returns>true, if successful</returns>
bool get_vulkan_queue_graphics (VkQueue& out_queue);


/// <summary>
/// create vulkan semaphores
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_semaphores (u32 num, VkSemaphore* out_semaphores);
/// <summary>
/// create vulkan fences
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_fences (u32 num, VkFenceCreateFlags flags, VkFence* out_fences);


/// <summary>
/// create a new command pool
/// use a separate command pool for each queue (queue family may differ between queues)
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_command_pool (VkQueueFlags requested_queue_type, VkCommandPool& out_command_pool);
/// <summary>
/// create a new command buffer
/// </summary>
/// <param name="command_pool">command pool to create the new command buffer from</param>
/// <returns>true, if successful</returns>
bool create_vulkan_command_buffers (u32 num, VkCommandPool command_pool, VkCommandBuffer* out_command_buffers);


bool begin_single_time_commands (VkCommandBuffer& out_command_buffer);
bool end_single_time_commands ();


bool process_os_messages ();
bool acquire_next_swapchain_image (VkSemaphore swapchain_image_available_semaphore);
/// <summary>
/// MUST be called after 'acquire_next_swapchain_image', so swapchain image index is correct!
/// record begin render pass commands
/// also record set viewport and scissor commands
/// </summary>
void begin_render_pass (VkCommandBuffer command_buffer, vec3 const& clear_colour);
void end_render_pass (VkCommandBuffer command_buffer);
bool present ();


void release_vulkan_fences (u32 num, VkFence* fences);
void release_vulkan_semaphores (u32 num, VkSemaphore* semaphores);

void release_vulkan_command_buffers (u32 num, VkCommandPool command_pool, VkCommandBuffer* command_buffers);
void release_vulkan_command_pool (VkCommandPool& command_pool);

void release_vulkan_swapchain ();
void release_vulkan_device ();
void release_vulkan_surface ();
void release_vulkan_instance ();
void release_window ();
