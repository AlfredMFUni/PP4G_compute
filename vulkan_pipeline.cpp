#include "vulkan_pipeline.h"

#include "utility.h" // DBG_ASSERT, DBG_ASSERT_VULKAN

#include <vector>    // for std::vector


bool create_vulkan_descriptor_set_layouts (VkDevice device,
  u32 num_desc_set_layout_binding_spans, std::span <const VkDescriptorSetLayoutBinding> const* desc_set_layout_binding_spans,
  VkDescriptorSetLayout* out_desc_set_layouts)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (num_desc_set_layout_binding_spans > 0u && desc_set_layout_binding_spans != nullptr);
  DBG_ASSERT (out_desc_set_layouts != nullptr);


  for (u32 i = 0u; i < num_desc_set_layout_binding_spans; ++i)
  {
    DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_desc_set_layouts [i]));

    // TODO: fix VkDescriptorSetLayoutCreateInfo
    VkDescriptorSetLayoutCreateInfo const dslci =
    {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .bindingCount = //???
      .pBindings = //???
    };

    VkResult const result = // TODO: add call to vkCreateDescriptorSetLayout here

    if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_desc_set_layouts [i]))
    {
      return DBG_ASSERT_MSG (false, "failed to create descriptor set layout\n");
    }
  }

  return true;
}
bool create_vulkan_pipeline_layout (VkDevice device,
  u32 num_desc_set_layouts, VkDescriptorSetLayout const* desc_set_layouts,
  u32 num_push_constant_ranges, VkPushConstantRange const* push_constant_ranges,
  VkPipelineLayout& out_pipeline_layout)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  if (num_desc_set_layouts > 0u) DBG_ASSERT (desc_set_layouts != nullptr);
  if (num_push_constant_ranges > 0u) DBG_ASSERT (push_constant_ranges != nullptr);
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_pipeline_layout));


  // TODO: fix VkPipelineLayoutCreateInfo
  VkPipelineLayoutCreateInfo const plci =
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .setLayoutCount = //???
    .pSetLayouts = desc_set_layouts,
    .pushConstantRangeCount = num_push_constant_ranges,
    .pPushConstantRanges = //???
  };

  VkResult const result = // TODO: add call to vkCreatePipelineLayout here

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_pipeline_layout))
  {
    return DBG_ASSERT_MSG (false, "failed to create pipeline layout\n");
  }

  return true;
}
bool create_vulkan_shader (VkDevice device,
  char const* compiled_shader_path,
  VkShaderModule& out_shader_module)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (compiled_shader_path != nullptr);
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_shader_module));


  std::vector <char> compiled_shader_code;
  if (!read_file (compiled_shader_path, compiled_shader_code))
  {
    return false;
  }

  VkShaderModuleCreateInfo const smci =
  {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .codeSize = compiled_shader_code.size (),
    .pCode = reinterpret_cast <u32 const*> (compiled_shader_code.data ())
  };

  VkResult const result = vkCreateShaderModule (device, // device
    &smci,                                              // pCreateInfo
    VK_NULL_HANDLE,                                     // pAllocator
    &out_shader_module);                                // pShaderModule
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_shader_module))
  {
    return DBG_ASSERT_MSG (false, "failed to create shader module\n");
  }

  return true;
}
bool create_vulkan_pipeline_compute (VkDevice device,
  VkShaderModule shader_module, char const* shader_entry_point,
  VkPipelineLayout pipeline_layout,
  VkPipeline& out_pipeline)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (shader_module));
  DBG_ASSERT (shader_entry_point != nullptr);
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pipeline_layout));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_pipeline));


  VkPipelineShaderStageCreateInfo const pssci =
  {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = shader_module,
    .pName = shader_entry_point,
    //.pSpecializationInfo = VK_NULL_HANDLE
  };

  // TODO: fix VkComputePipelineCreateInfo
  VkComputePipelineCreateInfo const cpci =
  {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .stage = //???
    .layout = //???
    //.basePipelineHandle = VK_NULL_HANDLE,
    //.basePipelineIndex = 0u
  };

  VkResult const result = // TODO: add call to vkCreateComputePipelines here

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_pipeline))
  {
    return DBG_ASSERT_MSG (false, "failed to create compute pipeline\n");
  }

  return true;
}
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
  VkPipeline& out_pipeline)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (shader_module_vert));
  DBG_ASSERT (shader_entry_point_vert != nullptr);
  DBG_ASSERT (CHECK_VULKAN_HANDLE (shader_module_frag));
  DBG_ASSERT (shader_entry_point_frag != nullptr);
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pipeline_layout));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (render_pass));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_pipeline));


  // TODO: ask Andy for graphics starter code

  return true;
}
void release_vulkan_shader (VkDevice device,
  VkShaderModule& shader_module)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (shader_module));


  vkDestroyShaderModule (device, shader_module, VK_NULL_HANDLE);
  shader_module = VK_NULL_HANDLE;
}
bool create_vulkan_descriptor_pool (VkDevice device,
  u32 max_sets,
  u32 num_desc_pool_sizes, VkDescriptorPoolSize const* desc_pool_sizes,
  VkDescriptorPool& out_desc_pool)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (max_sets > 0u);
  DBG_ASSERT (num_desc_pool_sizes > 0u && desc_pool_sizes != nullptr);
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_desc_pool));


  // TODO: fix VkDescriptorPoolCreateInfo
  VkDescriptorPoolCreateInfo const dpci =
  {
    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    .maxSets = max_sets,
    .poolSizeCount = //???
    .pPoolSizes = //???
  };

  VkResult const result = // TODO: add call to vkCreateDescriptorPool here

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_desc_pool))
  {
    return DBG_ASSERT_MSG (false, "failed to create descriptor pool\n");
  }

  return true;
}
bool create_vulkan_descriptor_sets (VkDevice device,
  u32 num_desc_set_infos, vulkan_descriptor_set_info const* desc_set_infos)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (num_desc_set_infos > 0u && desc_set_infos != nullptr);


  for (u32 i = 0u; i < num_desc_set_infos; ++i)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_set_infos [i].desc_pool));
    DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_set_infos [i].layout));
    DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_set_infos [i].out_set));
    DBG_ASSERT (!CHECK_VULKAN_HANDLE (desc_set_infos [i].out_set->desc_set));


    VkDescriptorSetAllocateInfo const dsai =
    {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      .descriptorPool = *desc_set_infos [i].desc_pool,
      .descriptorSetCount = 1u,
      .pSetLayouts = desc_set_infos [i].layout
    };

    // TODO: fix call to vkAllocateDescriptorSets
    VkResult const result = vkAllocateDescriptorSets (
      &desc_set_infos [i].out_set->desc_set);                 // pDescriptorSets

    if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (desc_set_infos [i].out_set->desc_set))
    {
      return DBG_ASSERT_MSG (false, "failed to allocate descriptor set\n");
    }

    desc_set_infos [i].out_set->set_index = desc_set_infos[i].set_index;
    desc_set_infos [i].out_set->desc_pool = desc_set_infos[i].desc_pool;
  }

  return true;
}


bool begin_command_buffer (VkCommandBuffer command_buffer, VkCommandBufferUsageFlags flags)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffer));


  VkCommandBufferBeginInfo const begin_info =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .flags = flags,
    //.pInheritanceInfo = VK_NULL_HANDLE
  };

  VkResult const result = vkBeginCommandBuffer (command_buffer, &begin_info);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }

  return true;
}
bool end_command_buffer (VkCommandBuffer command_buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffer));


  VkResult const result = vkEndCommandBuffer (command_buffer);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }

  return true;
}


void release_vulkan_descriptor_sets (VkDevice device,
  u32 num_desc_sets, vulkan_descriptor_set* desc_sets)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (num_desc_sets > 0u && desc_sets != nullptr);


  for (u32 i = 0u; i < num_desc_sets; ++i)
  {
    DBG_ASSERT (desc_sets[i].desc_pool != nullptr);
    DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_sets[i].desc_set));


    vkFreeDescriptorSets (device, *desc_sets [i].desc_pool, 1u, &desc_sets[i].desc_set);
    desc_sets [i].desc_pool = nullptr;
    desc_sets [i].desc_set = VK_NULL_HANDLE;
  }
}
void release_vulkan_descriptor_pool (VkDevice device, VkDescriptorPool& desc_pool)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_pool));


  vkDestroyDescriptorPool (device, desc_pool, VK_NULL_HANDLE);
  desc_pool = VK_NULL_HANDLE;
}
void release_vulkan_pipeline (VkDevice device,
  VkPipeline& pipeline)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pipeline));


  vkDestroyPipeline (device, pipeline, VK_NULL_HANDLE);
  pipeline = VK_NULL_HANDLE;
}
void release_vulkan_pipeline_layout (VkDevice device,
  VkPipelineLayout& pipeline_layout)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pipeline_layout));


  vkDestroyPipelineLayout (device, pipeline_layout, VK_NULL_HANDLE);
  pipeline_layout = VK_NULL_HANDLE;
}
void release_vulkan_descriptor_set_layouts (VkDevice device,
  u32 num_set_layouts, VkDescriptorSetLayout* desc_set_layouts)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (device));
  DBG_ASSERT (num_set_layouts > 0u && desc_set_layouts != nullptr);


  for (u32 i = 0u; i < num_set_layouts; ++i)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (desc_set_layouts [i]));


    vkDestroyDescriptorSetLayout (device, desc_set_layouts [i], VK_NULL_HANDLE);
    desc_set_layouts [i] = VK_NULL_HANDLE;
  }
}
