// preamble:
// a pipeline is the conceptual gathering of a shader, options and layout of the resources it requires
//
// resources (images, buffers etc) are organised in to groups called a 'descriptor set' (a.k.a. slots)
// a descriptor set consists of n 'descriptor's (a.k.a bindings)
// each descriptor refers to a SINGLE buffer/texture/array etc
// each pipeline can support n descriptor sets
//
// resources are bound to a descriptor in a descriptor set
// (but that resource can be bound to several separate descriptor sets across separate pipelines...)
// each descriptor set is then bound to a pipeline
//
// descriptor set resources should be organised on frequency of change, e.g.:
// for a pipeline resposible for rendering a model
// view/project matrix -> set 0 - constant for whole frame
// model data/texture  -> set 1 - changes per model
// world matrix        -> set 2 - changes per instance
//
// in general, our goal is to reduce the number of *any* resource binds per frame


// UBO = Uniform Buffer Object
// Good old fashioned, read-only data.

// SBO = Storage Buffer Object
// Source: https://vkguide.dev/docs/chapter-4/storage_buffers/
// Uniform buffers are great for small, read only data.
// But what if you want data you don't know the size of in the shader? Or data that can be writable.
// Storage buffers are usually slightly slower than uniform buffers, but they can be much, much bigger...
// With storage buffers, you can have an unsized array in a shader with whatever data you want.


#include "../maths.h"             // for standard types
#include "../utility.h"           // for DBG_ASSERT
#include "../vulkan_context.h"
#include "../vulkan_pipeline.h"
#include "../vulkan_resources.h"

#include <cmath>                  // for std::ceilf
#include <random>                 // for std::random_device, std::uniform_real_distribution

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan
#define WIN32_LEAN_AND_MEAN       // exclude rarely-used content from the Windows headers
#define NOMINMAX                  // prevent Windows macros defining their own min and max macros
#include <Windows.h>              // for WinMain


constexpr char const* COMPILED_COMPUTE_SHADER_PATH = "data/shaders/glsl/vulkan_compute_buffer/vulkan_compute_buffer.comp.spv";
constexpr u32 NUM_ELEMENTS = 1u << 16;
constexpr u32 ELEMENT_SIZE = sizeof (f32);
constexpr f32 MULTIPLIER = 5.f;


struct compute_UBO_info_buffer
{
  u32 num_elements;
};

struct compute_push_constants
{
  f32 multiplier;
};


int WINAPI WinMain (_In_ HINSTANCE/* hInstance*/,
  _In_opt_ HINSTANCE/* hPrevInstance*/,
  _In_ LPSTR/* lpCmdLine*/,
  _In_ int/* nShowCmd*/)
  // I know this is not a graphics app, but the build system sets 'SubSystem' to 'SUBSYSTEM:WINDOWS' by default, so we need this :(
{
  // CONTEXT

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue queue_compute = VK_NULL_HANDLE;

  // create vulkan instance, device etc
  // one per app
  {
    if (!create_vulkan_instance ())
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_device (VK_QUEUE_COMPUTE_BIT,
      physical_device, device))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // get vulkan queues
  // once per app
  {
    if (!get_vulkan_queue_compute (queue_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }


  // COMPUTE PIPELINE

  constexpr u32 NUM_SETS_COMPUTE = 1u;

  constexpr u32 NUM_RESOURCES_COMPUTE_SET_0 = 4u;
  constexpr u32 BINDING_ID_SET_0_SBO_INPUT_0 = 0u;
  constexpr u32 BINDING_ID_SET_0_SBO_INPUT_1 = 1u;
  constexpr u32 BINDING_ID_SET_0_SBO_OUTPUT = 2u;
  constexpr u32 BINDING_ID_SET_0_UBO_INFO = 3u;

  std::array <VkDescriptorSetLayout, NUM_SETS_COMPUTE> descriptor_set_layouts_compute = { VK_NULL_HANDLE };
  VkPipelineLayout pipeline_layout_compute = VK_NULL_HANDLE;
  VkPipeline pipeline_compute = VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool_compute = VK_NULL_HANDLE;
  vulkan_descriptor_set desc_set_0_compute; // for buffer_input_0 + buffer_input_1 + buffer_output + buffer_info

  vulkan_buffer buffer_input_0, buffer_input_1, buffer_output, buffer_info;

  VkCommandPool command_pool_compute = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_compute = VK_NULL_HANDLE;

  VkFence fence_compute = VK_NULL_HANDLE;


  {
    // describe the descriptors in set 0
    std::array <VkDescriptorSetLayoutBinding, NUM_RESOURCES_COMPUTE_SET_0> const descriptor_set_layout_binding_info_0 =
    {
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_SBO_INPUT_0,             // at binding point 0 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // an SBO (input)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_SBO_INPUT_1,             // at binding point 1 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // an SBO (input)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_SBO_OUTPUT,              // at binding point 2 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // an SBO (output)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_INFO,                // at binding point 3 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // an UBO (input)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      }
    };
    // group up all descriptor layout descriptions
    std::array <std::span <const VkDescriptorSetLayoutBinding>, NUM_SETS_COMPUTE> const descriptor_set_layout_bindings =
    {
      descriptor_set_layout_binding_info_0 // set 0
                                           // set 1...
    };
    if (!create_vulkan_descriptor_set_layouts (device,
      NUM_SETS_COMPUTE,
      descriptor_set_layout_bindings.data (),
      descriptor_set_layouts_compute.data ()))
    {
      DBG_ASSERT (false);
      return -1;
    }

    // describe push constants structure
    VkPushConstantRange const push_constant_ranges [] =
    {
      {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0u,
        .size = sizeof (compute_push_constants)
      }
    };

    if (!create_vulkan_pipeline_layout (device,
      descriptor_set_layouts_compute.size (), descriptor_set_layouts_compute.data (),
      1u, push_constant_ranges,
      pipeline_layout_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }

    VkShaderModule shader_module_compute = VK_NULL_HANDLE;
    if (!create_vulkan_shader (device,
      COMPILED_COMPUTE_SHADER_PATH,
      shader_module_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }

    if (!create_vulkan_pipeline_compute (device,
      shader_module_compute, "main",
      pipeline_layout_compute,
      pipeline_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }

    release_vulkan_shader (device,
      shader_module_compute); // don't need shader object now we have the pipeline
  }

  // at least one per pipeline...
  {
    // create pool of descriptors from which our descriptor sets will draw from
    //
    // we could have multiple descriptor pools, or one huge one
    // we are going to have one per pipeline in order to keep it simple
    std::array <VkDescriptorPoolSize, 2u> const pool_sizes =
    {{ // yes this is deliberate!
      {
        // we have 1 x descriptor set that consists of 3 x SBO descriptors...
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 3u
      },
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1u
      }
    }};
    if (!create_vulkan_descriptor_pool (device,
      1u, // how many descriptor sets will we make from the sets in the pool?
      pool_sizes.size (), pool_sizes.data (),
      descriptor_pool_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }

    std::array <vulkan_descriptor_set_info, NUM_SETS_COMPUTE> const descriptor_set_infos =
    {{
      // set 0
      {
        .desc_pool = &descriptor_pool_compute,          // the pool from which to allocate the individual descriptors from
        .layout = &descriptor_set_layouts_compute[0],   // layout of the descriptor set
        .set_index = 0u,                                // index of the descriptor set
        .out_set = &desc_set_0_compute                  // pointer to where to instantiate the descriptor set to
      }
      // set 1...
    }};
    if (!create_vulkan_descriptor_sets (device,
      descriptor_set_infos.size (), descriptor_set_infos.data ()))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }

  {
    if (!create_vulkan_buffer (physical_device, device,
      NUM_ELEMENTS * ELEMENT_SIZE, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        VK_SHARING_MODE_EXCLUSIVE, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      buffer_input_0))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_buffer (physical_device, device,
      NUM_ELEMENTS * ELEMENT_SIZE, 
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 
        VK_SHARING_MODE_EXCLUSIVE, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      buffer_input_1))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_buffer(physical_device, device,
        NUM_ELEMENTS * ELEMENT_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        buffer_output))
    {
        DBG_ASSERT(false);
        return -1;
    }
    if (!create_vulkan_buffer(physical_device, device,
        sizeof(compute_UBO_info_buffer),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_SHARING_MODE_EXCLUSIVE,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        buffer_info))
    {
        DBG_ASSERT(false);
        return -1;
    }
  }
  // Bind resources to descriptor set
  {
    // desc_set_0_compute = buffer_input_0, buffer_input_1, buffer_output, buffer_info

    VkDescriptorBufferInfo const buffer_infos [NUM_RESOURCES_COMPUTE_SET_0] =
    {
      {
        .buffer = buffer_input_0.buffer,
        .offset = 0u,
        .range = VK_WHOLE_SIZE
      },
      {
        .buffer = buffer_input_1.buffer,
        .offset = 0u,
        .range = VK_WHOLE_SIZE
      },
      {
        .buffer = buffer_output.buffer,
        .offset = 0u,
        .range = VK_WHOLE_SIZE
      },
      {
        .buffer = buffer_info.buffer,
        .offset = 0u,
        .range = VK_WHOLE_SIZE
      }
    };

    VkWriteDescriptorSet const write_descriptors [NUM_RESOURCES_COMPUTE_SET_0] =
    {
      // desc_set_0_compute
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_compute.desc_set,
        .dstBinding = BINDING_ID_SET_0_SBO_INPUT_0,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos [0], // buffer_input_0
        .pTexelBufferView = VK_NULL_HANDLE
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_compute.desc_set,
        .dstBinding = BINDING_ID_SET_0_SBO_INPUT_1,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[1], // buffer_input_1
        .pTexelBufferView = VK_NULL_HANDLE
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_compute.desc_set,
        .dstBinding = BINDING_ID_SET_0_SBO_OUTPUT,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos [2], // buffer_output
        .pTexelBufferView = VK_NULL_HANDLE
      },
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_compute.desc_set,
        .dstBinding = BINDING_ID_SET_0_UBO_INFO,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[3], // buffer_info
        .pTexelBufferView = VK_NULL_HANDLE
      }
    };

    vkUpdateDescriptorSets (device, // device
      NUM_RESOURCES_COMPUTE_SET_0,  // descriptorWriteCount
      write_descriptors,            // pDescriptorWrites
      0u,                           // descriptorCopyCount
      VK_NULL_HANDLE);              // pDescriptorCopies
  }
  // create command buffers
  // at least one per pipeline
  {
    if (!create_vulkan_command_pool (VK_QUEUE_COMPUTE_BIT, command_pool_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_command_buffers (1u, command_pool_compute, &command_buffer_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // create sync objects
  // at least one per submit (can be reused if in loop)
  {
    if (!create_vulkan_fences (1u, 0u, &fence_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }


  // SET INPUT/INFO BUFFERS
  {
    std::random_device rd;
    std::ranlux24_base re (rd ()); // std::default_engine (std::mersenne_twister_engine) uses 5K bytes on the stack!!!
    std::uniform_real_distribution <f32> distribution (0.f, 100.f);

    if (!map_and_unmap_memory (device,
      buffer_input_0.memory, [&rd, &distribution](void* mapped_memory)
      {
        f32* data = (f32*)mapped_memory;
        for (u32 i = 0u; i < NUM_ELEMENTS; ++i)
        {
          data [i] = distribution (rd);
        }
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!map_and_unmap_memory(device,
        buffer_input_1.memory, [&rd, &distribution](void* mapped_memory)
        {
            f32* data = (f32*)mapped_memory;
            for (u32 i = 0u; i < NUM_ELEMENTS; ++i)
            {
                data[i] = distribution(rd);
            }
        }))
    {
        DBG_ASSERT(false);
        return -1;
    }
    // no need to set/initialise 'buffer_output' as its content will be completely overwritten by the compute shader

    if (!map_and_unmap_memory(device,
        buffer_info.memory, [&rd, &distribution](void* mapped_memory)
        {
            u32* data = (u32*)mapped_memory;
            (*data) = NUM_ELEMENTS;
        }))
    {
        DBG_ASSERT(false);
        return -1;
    }
  }

 
  {
    if (!begin_command_buffer (command_buffer_compute, 0u))
    {
      DBG_ASSERT (false);
      return -1;
    }
    {
      // any compute related command after this point is attached to this pipeline (on this command buffer)
        vkCmdBindPipeline(command_buffer_compute,  // Command Buffer
            VK_PIPELINE_BIND_POINT_COMPUTE,        // Pipeline Bind Point
            pipeline_compute);                     // Pipeline

      // bind descriptor set - buffer_input_0 + buffer_input_1 + buffer_output + buffer_info
        vkCmdBindDescriptorSets(command_buffer_compute, //  commandBufffer
            VK_PIPELINE_BIND_POINT_COMPUTE,             //  pipelineBindPoint
            pipeline_layout_compute,                    //  layout
            desc_set_0_compute.set_index,               //  firstSet
            1u,                                         //  descriptorSetCount
            &desc_set_0_compute.desc_set,               //  pDescriptorSets
            0u,                                         //  dynamicOffsetCount
            VK_NULL_HANDLE);                            //  pDynamicOffsets

      compute_push_constants const data =
      {
        .multiplier = MULTIPLIER
      };

      vkCmdPushConstants(command_buffer_compute, // commandBuffer
          pipeline_layout_compute,               // layout
          VK_SHADER_STAGE_COMPUTE_BIT,           // stageFlags
          0u,                                    // offset
          sizeof(compute_push_constants),        // size
          &data);                                // pValues

      // trigger compute shader
      u32 const thread_group_dim = 256u; // this is set in the shader

      //                           Integer division ceiling
      u32 const group_count_x = ((NUM_ELEMENTS - 1u) / thread_group_dim) + 1;

      vkCmdDispatch(command_buffer_compute, // commandBuffer
          group_count_x, 1u, 1u);           // Group count X, Y, and Z
    }
    if (!end_command_buffer (command_buffer_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }

  // TODO: SUBMIT
  {
      if (vkResetFences(device, // device
          1u,                   // fenceCount
          &fence_compute) < 0)  // pFences
      {
          DBG_ASSERT(false);
          return -1;
      }

    // submit compute commands
    VkSubmitInfo const submit_info =
    {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      //.pNext = VK_NULL_HANDLE,
      .waitSemaphoreCount = 0u,
      .pWaitSemaphores = VK_NULL_HANDLE,          // wait for these semaphores to be signalled before submitting command buffers
      .pWaitDstStageMask = VK_NULL_HANDLE,
      .commandBufferCount = 1u,
      .pCommandBuffers = &command_buffer_compute,
      .signalSemaphoreCount = 0u,
      .pSignalSemaphores = VK_NULL_HANDLE         // semaphores to trigger when command buffer has finished executing
    };
    if (!CHECK_VULKAN_RESULT (vkQueueSubmit (queue_compute, 1u, &submit_info,
      fence_compute))) // fence to signal when complete
    {
      DBG_ASSERT (false);
      return -1;
    }


    // wait for fence to be triggered via vkQueueSubmit (slow!)

    if (vkWaitForFences(device, // device
        1u,                      // fenceCount
        &fence_compute,          // pFences
        VK_TRUE,                 // waitAll
        UINT64_MAX) < 0)         // timeout
    {
        DBG_ASSERT(false);
        return -1;
    }
  }


  // OUTPUT RESULTS
  {
    dprintf ("output[i] = (input a[i] * multiplier) + input b [i]\n");
    dprintf ("multiplier = %.2f\n", MULTIPLIER);


    if (!map_and_unmap_memory (device,
      buffer_input_0.memory, [](void* mapped_memory)
      {
        f32* data = (f32*)mapped_memory;

        dprintf ("input a: ");
        for (u32 i = 0u; i < NUM_ELEMENTS - 1u; ++i)
        {
          dprintf ("%.2f, ", data [i]);
        }
        dprintf ("%.2f\n", data [NUM_ELEMENTS - 1u]);

        write_file ("input_a.txt", data, NUM_ELEMENTS, "%.2f");
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }

    if (!map_and_unmap_memory (device,
      buffer_input_1.memory, [](void* mapped_memory)
      {
        f32* data = (f32*)mapped_memory;

        dprintf ("input b: ");
        for (u32 i = 0u; i < NUM_ELEMENTS - 1u; ++i)
        {
          dprintf ("%.2f, ", data [i]);
        }
        dprintf ("%.2f\n", data [NUM_ELEMENTS - 1u]);

        write_file ("input_b.txt", data, NUM_ELEMENTS, "%.2f");
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }

    if (!map_and_unmap_memory (device,
      buffer_output.memory, [](void* mapped_memory)
      {
        f32* data = (f32*)mapped_memory;

        dprintf ("output : ");
        for (u32 i = 0u; i < NUM_ELEMENTS - 1u; ++i)
        {
          dprintf ("%.2f, ", data [i]);
        }
        dprintf ("%.2f\n", data [NUM_ELEMENTS - 1u]);

        write_file ("output.txt", data, NUM_ELEMENTS, "%.2f");
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }


  // RELEASE
  {
    // COMPUTE PIPELINE
    {
      release_vulkan_fences (1u, &fence_compute);

      release_vulkan_command_buffers (1u, command_pool_compute, &command_buffer_compute);
      release_vulkan_command_pool (command_pool_compute);

      release_vulkan_buffer (device, buffer_info);
      release_vulkan_buffer (device, buffer_output);
      release_vulkan_buffer (device, buffer_input_1);
      release_vulkan_buffer (device, buffer_input_0);

      release_vulkan_descriptor_sets (device,
        1u, &desc_set_0_compute);
      release_vulkan_descriptor_pool (device, descriptor_pool_compute);

      release_vulkan_pipeline (device, pipeline_compute);
      release_vulkan_pipeline_layout (device, pipeline_layout_compute);
      release_vulkan_descriptor_set_layouts (device,
        NUM_SETS_COMPUTE, descriptor_set_layouts_compute.data ());
    }

    // CONTEXT
    {
      release_vulkan_device ();
      release_vulkan_instance ();
      queue_compute = VK_NULL_HANDLE;
      device = VK_NULL_HANDLE;
      physical_device = VK_NULL_HANDLE;
    }
  }

  return 0;
}
