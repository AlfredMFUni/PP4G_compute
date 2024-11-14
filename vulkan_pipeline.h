#pragma once

#include "maths.h"                // for standard types

#include <array>                  // for std::array
#include <span>                   // for std::span

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan


/// <summary>
/// convenience object to group a descriptor set instance
/// with the descriptor set index to which it should be bound to
/// and the descriptor pool it was allocated from
/// </summary>
struct vulkan_descriptor_set
{
  VkDescriptorSet desc_set = VK_NULL_HANDLE;
  // could be an array of descriptor sets if you wished to support per frame resources
  // would also require you to store how many instances of the descriptor set you wanted here

  u32 set_index = {};
  VkDescriptorPool const* desc_pool = nullptr;
};
/// <summary>
/// convenience object to group up the data required to create a descriptor set, i.e.
/// the pool from which to allocate the individual descriptors from
/// the layout of the descriptor set
/// the index of the descriptor set
/// a pointer to where to instantiate the descriptor set to
/// </summary>
struct vulkan_descriptor_set_info
{
  VkDescriptorPool const* desc_pool = nullptr;
  VkDescriptorSetLayout const* layout = nullptr;
  u32 set_index = {};

  vulkan_descriptor_set* out_set = nullptr;
  // could be a pointer to an array of descriptor sets if you wished to support per frame resources
  // would also require you to pass how many instances of the descriptor set you wanted here
};


/// <summary>
/// create n vulkan descriptor set layouts
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_descriptor_set_layouts (VkDevice device,
  u32 num_desc_set_layout_binding_spans, std::span <const VkDescriptorSetLayoutBinding> const* desc_set_layout_binding_spans,
  VkDescriptorSetLayout* out_desc_set_layouts);
/// <summary>
/// create a vulkan pipeline layout
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_pipeline_layout (VkDevice device,
  u32 num_desc_set_layouts, VkDescriptorSetLayout const* desc_set_layouts,
  u32 num_push_constant_ranges, VkPushConstantRange const* push_constant_ranges,
  VkPipelineLayout& out_pipeline_layout);
/// <summary>
/// create a vulkan shader
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_shader (VkDevice device,
  char const* compiled_shader_path,
  VkShaderModule& out_shader_module);
/// <summary>
/// create a vulkan compute pipeline
/// will use default options for settings not passed in
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_pipeline_compute (VkDevice device,
  VkShaderModule shader_module, char const* shader_entry_point,
  VkPipelineLayout pipeline_layout,
  VkPipeline& out_pipeline);
/// <summary>
/// create a vulkan graphics pipeline
/// will use default options for settings not passed in
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_pipeline_graphics (VkDevice device,
  VkShaderModule shader_module_vert, char const* shader_entry_point_vert,
  VkShaderModule shader_module_frag, char const* shader_entry_point_frag,
  VkViewport const& viewport, VkRect2D const& scissor,
  VkPipelineVertexInputStateCreateInfo const& pvisci,
  VkPipelineInputAssemblyStateCreateInfo const& piasci,
  VkPipelineRasterizationStateCreateInfo const& prsci,
  VkPipelineMultisampleStateCreateInfo const& pmsci,
  VkPipelineDepthStencilStateCreateInfo const& pdssci,
  VkPipelineColorBlendStateCreateInfo const& pcbsci,
  VkPipelineLayout pipeline_layout, VkRenderPass render_pass, u32 subpass_id,
  VkPipeline& out_pipeline);
void release_vulkan_shader (VkDevice device,
  VkShaderModule& shader_module);
/// <summary>
/// create a vulkan descriptor pool
/// </summary>
/// <param name="max_sets">maximum number of descriptor sets that can be allocated from the pool
/// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPoolCreateInfo.html
/// i.e. how many descriptor sets will allocate there descriptors from here?</param>
/// <returns>true, if successful</returns>
bool create_vulkan_descriptor_pool (VkDevice device,
  u32 max_sets,
  u32 num_desc_pool_sizes, VkDescriptorPoolSize const* desc_pool_sizes,
  VkDescriptorPool& out_desc_pool);
/// <summary>
/// create n vulkan descriptor sets
/// </summary>
/// <returns>true, if successful</returns>
bool create_vulkan_descriptor_sets (VkDevice device,
  u32 num_desc_set_infos, vulkan_descriptor_set_info const* desc_set_infos);


bool begin_command_buffer (VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags);
bool end_command_buffer (VkCommandBuffer command_buffer);


void release_vulkan_descriptor_sets (VkDevice device,
  u32 num_desc_sets, vulkan_descriptor_set* desc_sets);
void release_vulkan_descriptor_pool (VkDevice device, VkDescriptorPool& descriptor_pool);
void release_vulkan_pipeline (VkDevice device, VkPipeline& pipeline);
void release_vulkan_pipeline_layout (VkDevice device, VkPipelineLayout& pipeline_layout);
void release_vulkan_descriptor_set_layouts (VkDevice device,
  u32 num_set_layouts, VkDescriptorSetLayout* desc_set_layouts);
