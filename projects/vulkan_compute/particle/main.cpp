// Summary
// a compute shader applies a 'post effect' to a texture
// AFTER this, the original (input) and altered (output) textures are rendered to the screen as 2D sprites
// NOTE: this example *DOES NOT* execute the compute shader in the update/render loop with the graphics shader!
// that is for you to do :)

// Compute Pipeline Layout
// Descriptor Set 0:
//   Binding 0: storage image - input texture (read only)
//   Binding 1: storage image - output texture (empty texture of the same size, write enabled)

// Graphics Pipeline Layout
// vertex buffer (2d position, uv)
// index buffer
// Descriptor Set 0: (used by vertex shader)
//   Binding 0: UBO - camera (holds view * projection matrix)
//   Binding 1: UBO - model (holds model/world matrix)
// Descriptor Set 1: (used by pixel/fragment shader)
//   Binding 0: combined image sampler - a texture


// CONTEXT
//      | 1) create window (glfw)
//      | 2) create vulkan instance
//      | 3) create vulkan surface
//      | 4) create vulkan device
//      |   returns reference to vulkan physical device & device
//      | 5) create vulkan swapchain
//      |   returns reference to vulkan swapchain extent (size of back buffers) & render_pass
//      | 6) create vulkan compute & graphics queues

// COMPUTE PIPELINE
// TODO | 1) create compute pipeline
//      |   layout = 1 x descriptor set
//      |     DSL 0 = 2 x storage image bindings
//      | 2) create 1 x descriptor sets for compute pipeline
//      |   DSI 0 = DSL 0
//      | 3) create compute resources
//      |   2 x storage images
//      |     0 = texture_compute_input  = image from file                         - layout = general, usage = sampled + storage
// TODO |     1 = texture_compute_output = empty texture, with matching dimensions - layout = general, usage = sampled + storage
//      | 4) bind compute resources to compute descriptor set
//      |   0 = desc_set_0_compute = texture_compute_input + texture_compute_output
//      | 5) create vulkan command buffer
//      | 6) create vulkan sync objects
//      |   1 x fence

// COMPUTE DISPATCH
//      | 1) begin command buffer
// TODO | 2) bind pipeline
// TODO | 3) bind descriptor set
// TODO | 4) dispatch
//      | 5) end command buffer
//      | 6) submit
//      | 7) wait for submit to complete (via fence)
//      | -----------------
//      | 8) transition image layouts
//      |   both textures are in general layout
//      |   need to be 'transitioned' to 'shader read only optimal' layout for graphics pipeline to read them

// GRAPHICS PIPELINE
//      | 1) create graphics pipeline
//      |   layout = 2 x descriptor set
//      |     DSL 0 = 2 x UBO bindings           = camera & model buffer
//      |     DSL 1 = 1 x combined image sampler = a texture
//      | 2) create 4 x descriptor sets for graphics pipeline
//      |   DSI 0 = DSL 0 = camera info + input sprite world matrix
//      |   DSI 1 = DSL 1 = input image
// TODO |   DSI 2 = DSL 0 = camera info + output sprite world matrix
// TODO |   DSI 3 = DSL 1 = output image
//      | 3) create graphics resources
//      |   textures created have already been made!
//      |   3 x UBO buffers
// TODO |     0 = buffer_graphics_camera       = camera_buffer - camera info                - usage = UBO, sharing = exclusive, flags = host visible
// TODO |     1 = buffer_graphics_model_input  = model_buffer  - input sprite world matrix  - usage = UBO, sharing = exclusive, flags = host visible
// TODO |     2 = buffer_graphics_model_output = model_buffer  - output sprite world matrix - usage = UBO, sharing = exclusive, flags = host visible
//      |   set buffers appropriately
// TODO | 4) bind graphics + compute resources to graphics descriptor sets
//      |   0 = desc_set_0_graphics_input  = buffer_graphics_camera + buffer_graphics_model_input  = for rendering input image
//      |   1 = desc_set_1_graphics_input  = texture_compute_input                                 = for rendering input image
//      |   2 = desc_set_0_graphics_output = buffer_graphics_camera + buffer_graphics_model_output = for rendering output image
//      |   3 = desc_set_1_graphics_output = texture_compute_output                                = for rendering output image
//      | 5) create model data
//      |   mesh_sprite:
//      |     1 x vertex buffer (xy = position, uv = tex coords)
//      |     1 x index buffer (16bit uint)
//      | 6) create vulkan command buffer
//      | 7) create vulkan sync objects
//      |   1 x semaphore
//      |   1 x fence

// GRAPHICS RENDER
//      | GAME LOOP
//      | 01) process os messages
//      |   check glfw if window should close
//      |   poll glfw input events
//      |   UPDATE
//      |   RENDER
//      |   02) acquire next swapchain image
//      |     sets 'swapchain_image_available_semaphore' semaphore which is triggered when an available presentable image (back buffer) is ready to use
//      |   03) reset command buffer
//      |   04) begin command buffer
//      |   05) begin render pass
// TODO |   06) bind pipeline
//      | -----------------
// TODO |     07) bind vertex buffer
// TODO |     08) bind index buffer
// TODO |     09) bind desc_set_0_graphics_input
// TODO |     10) bind desc_set_1_graphics_input
// TODO |     11) draw indexed
//      |       draw 'input' image on left
//      | -----------------
// TODO |     14) bind desc_set_0_graphics_output
// TODO |     15) bind desc_set_1_graphics_output
// TODO |     16) draw indexed
//      |       draw 'output' image on right
//      | -----------------
//      |   17) end render pass
//      |   18) end command buffer
//      |   19) reset fence
// TODO |   20) submit
//      |     wait on 'swapchain_image_available_semaphore' semaphore
//      |   21) wait for submit to complete (via fence)
//      |   22) present

// RELEASE
//      | 1) release graphics pipeline
//      | 2) release compute pipeline
//      | 3) release context


#include "../maths.h"            // for standard types, glm::ortho
#include "../utility.h"          // for DBG_ASSERT
#include "../vulkan_context.h"
#include "../vulkan_pipeline.h"
#include "../vulkan_resources.h"

#include <array>                  // for std::array
#include <cmath>                  // for std::ceilf
#include <span>                   // for std::span
#include <random>                 // for rng.

#define VK_USE_PLATFORM_WIN32_KHR // tell vulkan we are on Windows platform
#include <vulkan/vulkan.h>        // for everything vulkan
#define WIN32_LEAN_AND_MEAN       // exclude rarely-used content from the Windows headers
#define NOMINMAX                  // prevent Windows macros defining their own min and max macros
#include <Windows.h>              // for WinMain


constexpr char const* WINDOW_TITLE = "vulkan_compute_texture";
constexpr char const* COMPILED_COMPUTE_SHADER_PATH = "data/shaders/glsl/vulkan_compute_particle/vulkan_compute_particle.comp.spv";
constexpr char const* COMPILED_GRAPHICS_SHADER_PATH_VERT = "data/shaders/glsl/vulkan_compute_particle/sprite.vert.spv";
constexpr char const* COMPILED_GRAPHICS_SHADER_PATH_FRAG = "data/shaders/glsl/vulkan_compute_particle/sprite.frag.spv";
constexpr char const* TEXTURE_PATH = "data/textures/Particle.png";

constexpr unsigned int NUM_PARTICLES = 1u << 12;
constexpr unsigned int DATA_SIZE = sizeof(u32);

constexpr float left_bound = -120, right_bound = 121, top_bound = 67.f, bottom_bound = -67.f;
constexpr float MaxParticleSpeed = 1.f / (1u << 6u);
constexpr float particleScale = 1.f / (1u << 0u);

struct compute_UBO_info_buffer
{
    u32 num_particles;
    f32 bounds[4u];
};

struct compute_push_constants
{
    f32 deltatime;
};

int WINAPI WinMain (_In_ HINSTANCE/* hInstance*/,
  _In_opt_ HINSTANCE/* hPrevInstance*/,
  _In_ LPSTR/* lpCmdLine*/,
  _In_ int/* nShowCmd*/)
{
  // CONTEXT

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;
  VkExtent2D extent = {};
  VkRenderPass render_pass = VK_NULL_HANDLE;

  VkQueue queue_compute = VK_NULL_HANDLE, queue_graphics = VK_NULL_HANDLE;

  // create window, vulkan instance & device etc
  {
    if (!create_window (WINDOW_TITLE))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_instance ())
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_surface ())
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_device (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT,
      physical_device, device))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_swapchain (extent, render_pass))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // get vulkan queues
  {
    if (!get_vulkan_queue_compute (queue_compute))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!get_vulkan_queue_graphics (queue_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }


  // COMPUTE PIPELINE

  constexpr u32 NUM_SETS_COMPUTE = 1u;

  constexpr u32 NUM_RESOURCES_COMPUTE_SET_0 = 5u;
  constexpr u32 BINDING_ID_SET_0_X_POSITION = 0u;
  constexpr u32 BINDING_ID_SET_0_Y_POSITION = 1u;
  constexpr u32 BINDING_ID_SET_0_X_VELOCITY = 2u;
  constexpr u32 BINDING_ID_SET_0_Y_VELOCITY = 3u;
  constexpr u32 BINDING_ID_SET_0_INFO       = 4u;

  std::array <VkDescriptorSetLayout, NUM_SETS_COMPUTE> descriptor_set_layouts_compute = { VK_NULL_HANDLE };
  VkPipelineLayout pipeline_layout_compute = VK_NULL_HANDLE;
  VkPipeline pipeline_compute = VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool_compute = VK_NULL_HANDLE;
  vulkan_descriptor_set desc_set_0_compute; // for compute, X,Y - Pos,Vel

  vulkan_buffer buffer_pos_x, buffer_pos_y, buffer_vel_x, buffer_vel_y, buffer_info;

  VkCommandPool command_pool_compute = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_compute = VK_NULL_HANDLE;

  VkFence fence_compute = VK_NULL_HANDLE;


  // create compute pipeline
  {
    // describe the descriptors in set 0
    std::array <VkDescriptorSetLayoutBinding, NUM_RESOURCES_COMPUTE_SET_0> const descriptor_set_layout_binding_info_0 =
    {
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_X_POSITION,          // at binding point 0 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // an image (input)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_Y_POSITION,         // at binding point 1 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // another image (output)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_X_VELOCITY,         // at binding point 2 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // another image (output)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_Y_VELOCITY,         // at binding point 3 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // another image (output)
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_INFO,               // at binding point 4 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // another image (output)
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
    VkPushConstantRange const push_constant_ranges[] =
    {
      {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0u,
        .size = sizeof(compute_push_constants)
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
    // error: type mismatch storage image != storage buffer
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
  // create compute descriptor sets
  {
    // create pool of descriptors from which our descriptor sets will draw from
    //
    // we could have multiple descriptor pools, or one huge one
    // we are going to have one per pipeline in order to keep it simple
    std::array <VkDescriptorPoolSize, 2u> const pool_sizes =
    {{ // yes this is deliberate!
      {
        // we have 1 x descriptor set that consists of 4 x 'storage buffer' descriptors
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 4u
      },
      {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1u
      }
      // if you also needed n uniform buffer descriptors you would need to create a new VkDescriptorPoolSize here
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
        .desc_pool = &descriptor_pool_compute,         // the pool from which to allocate the individual descriptors from
        .layout = &descriptor_set_layouts_compute [0], // layout of the descriptor set
        .set_index = 0u,                               // index of the descriptor set
        .out_set = &desc_set_0_compute                 // pointer to where to instantiate the descriptor set to
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
  // create compute resources
  {
      {

          if (!create_vulkan_buffer(physical_device, device,
              NUM_PARTICLES * DATA_SIZE,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
              VK_SHARING_MODE_EXCLUSIVE,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
              buffer_pos_x))
          {
              DBG_ASSERT(false);
              return -1;
          }
          if (!create_vulkan_buffer(physical_device, device,
              NUM_PARTICLES * DATA_SIZE,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
              VK_SHARING_MODE_EXCLUSIVE,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
              buffer_pos_y))
          {
              DBG_ASSERT(false);
              return -1;
          }
          if (!create_vulkan_buffer(physical_device, device,
              NUM_PARTICLES * DATA_SIZE,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
              VK_SHARING_MODE_EXCLUSIVE,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
              buffer_vel_x))
          {
              DBG_ASSERT(false);
              return -1;
          }
          if (!create_vulkan_buffer(physical_device, device,
              NUM_PARTICLES * DATA_SIZE,
              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
              VK_SHARING_MODE_EXCLUSIVE,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
              buffer_vel_y))
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

    // needs to have the exact same size as texture_compute_input
    // needs general layout so it can be written to
    // usage = sampled and storage
    //   sampled = image can be sampled from later
    //   storage = so we can use it in the compute shader
  }
  // bind compute resources to compute descriptor set
  {
    // desc_set_0_compute = for compute = texture_compute_input + texture_compute_output
      VkDescriptorBufferInfo const buffer_infos[NUM_RESOURCES_COMPUTE_SET_0] =
      {
        {
          .buffer = buffer_pos_x.buffer,
          .offset = 0u,
          .range = VK_WHOLE_SIZE
        },
        {
          .buffer = buffer_pos_y.buffer,
          .offset = 0u,
          .range = VK_WHOLE_SIZE
        },
        {
          .buffer = buffer_vel_x.buffer,
          .offset = 0u,
          .range = VK_WHOLE_SIZE
        },
        {
          .buffer = buffer_vel_y.buffer,
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
        .dstBinding = BINDING_ID_SET_0_X_POSITION,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[0],
        .pTexelBufferView = VK_NULL_HANDLE
      },
      // desc_set_0_compute
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_compute.desc_set,
        .dstBinding = BINDING_ID_SET_0_Y_POSITION,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[1],
        .pTexelBufferView = VK_NULL_HANDLE
      },
        // desc_set_0_compute
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_compute.desc_set,
          .dstBinding = BINDING_ID_SET_0_X_VELOCITY,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[2],
          .pTexelBufferView = VK_NULL_HANDLE
        },
        // desc_set_0_compute
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_compute.desc_set,
          .dstBinding = BINDING_ID_SET_0_Y_VELOCITY,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[3],
          .pTexelBufferView = VK_NULL_HANDLE
        },
        // desc_set_0_compute
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_compute.desc_set,
          .dstBinding = BINDING_ID_SET_0_INFO,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[4],
          .pTexelBufferView = VK_NULL_HANDLE
        }
    };

    vkUpdateDescriptorSets (device, // device
      NUM_RESOURCES_COMPUTE_SET_0,  // descriptorWriteCount
      write_descriptors,            // pDescriptorWrites
      0u,                           // descriptorCopyCount
      VK_NULL_HANDLE);              // pDescriptorCopies
  }
  // create compute command buffers
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
  // create compute sync objects
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
      std::ranlux24_base re(rd()); // std::default_engine (std::mersenne_twister_engine) uses 5K bytes on the stack!!!
      std::uniform_real_distribution <f32> X_Pos_Dist(left_bound, right_bound);
      std::uniform_real_distribution <f32> Y_Pos_Dist(bottom_bound, top_bound);
      std::uniform_real_distribution <f32> Vel_Dist(-MaxParticleSpeed, MaxParticleSpeed);

      if (!map_and_unmap_memory(device,
          buffer_pos_x.memory, [&re, &X_Pos_Dist](void* mapped_memory)
          {
              f32* data = (f32*)mapped_memory;
              for (u32 i = 0u; i < NUM_PARTICLES; ++i)
              {
                  data[i] = X_Pos_Dist(re);
              }
          }))
      {
          DBG_ASSERT(false);
          return -1;
      }
      if (!map_and_unmap_memory(device,
          buffer_pos_y.memory, [&re, &Y_Pos_Dist](void* mapped_memory)
          {
              f32* data = (f32*)mapped_memory;
              for (u32 i = 0u; i < NUM_PARTICLES; ++i)
              {
                  data[i] = Y_Pos_Dist(re);
              }
          }))
      {
          DBG_ASSERT(false);
          return -1;
      }
      if (!map_and_unmap_memory(device,
          buffer_vel_x.memory, [&re, &Vel_Dist](void* mapped_memory)
          {
              f32* data = (f32*)mapped_memory;
              for (u32 i = 0u; i < NUM_PARTICLES; ++i)
              {
                  data[i] =  Vel_Dist(re);
              }
          }))
      {
          DBG_ASSERT(false);
          return -1;
      }
      if (!map_and_unmap_memory(device,
          buffer_vel_y.memory, [&re, &Vel_Dist](void* mapped_memory)
          {
              f32* data = (f32*)mapped_memory;
              for (u32 i = 0u; i < NUM_PARTICLES; ++i)
              {
                  data[i] =  Vel_Dist(re);
              }
          }))
      {
          DBG_ASSERT(false);
          return -1;
      }
      // no need to set/initialise 'buffer_output' as its content will be completely overwritten by the compute shader

      if (!map_and_unmap_memory(device,
          buffer_info.memory, [](void* mapped_memory)
          {
              u32* num_elements = (u32*)mapped_memory;
              (*num_elements) = NUM_PARTICLES;
              f32* bounds = (f32*)(num_elements + 1);
              bounds[0] = left_bound;
              bounds[1] = right_bound;
              bounds[2] = top_bound;
              bounds[3] = bottom_bound;
          }))
      {
          DBG_ASSERT(false);
          return -1;
      }
  }


  

  // GRAPHICS PIPELINE

  struct vertex_2d_uv
  {
    vec2 pos; // 2d pos
    vec2 uv;  // texture coordinates
  };
  struct camera_buffer
  {
    mat4 vp_matrix;
  };
  struct model_buffer
  {
    mat4 model_matrix;
  };


  constexpr u32 NUM_SETS_GRAPHICS = 2u;

  constexpr u32 NUM_RESOURCES_GRAPHICS_SET_0 = 5u;
  constexpr u32 BINDING_ID_SET_0_UBO_CAMERA = 0u;
  constexpr u32 BINDING_ID_SET_0_UBO_MODEL = 1u;
  constexpr u32 BINDING_ID_SET_0_UBO_INFO = 2u;
  constexpr u32 BINDING_ID_SET_0_UBO_POS_X = 3u;
  constexpr u32 BINDING_ID_SET_0_UBO_POS_Y = 4u;

  constexpr u32 NUM_RESOURCES_GRAPHICS_SET_1 = 1u;
  constexpr u32 BINDING_ID_SET_1_TEXTURE = 0u;

  std::array <VkDescriptorSetLayout, NUM_SETS_GRAPHICS> descriptor_set_layouts_graphics = { VK_NULL_HANDLE, VK_NULL_HANDLE };
  VkPipelineLayout pipeline_layout_graphics = VK_NULL_HANDLE;
  VkPipeline pipeline_graphics = VK_NULL_HANDLE;

  VkDescriptorPool descriptor_pool_graphics = VK_NULL_HANDLE;
  vulkan_descriptor_set desc_set_0_graphics_input,  // for rendering input image , buffer_graphics_camera + buffer_graphics_model_input
                        desc_set_1_graphics_input;  // for rendering input image , texture_compute_input

  vulkan_buffer buffer_graphics_camera,
    buffer_graphics_model_input,  // for when rendering the input texture
    buffer_graphics_model_output; // for when rendering the output texture

  vulkan_texture texture_particle;

  VkCommandPool command_pool_graphics = VK_NULL_HANDLE;
  VkCommandBuffer command_buffer_graphics = VK_NULL_HANDLE;

  vulkan_mesh mesh_sprite;

  VkSemaphore swapchain_image_available_semaphore = VK_NULL_HANDLE;
  VkFence fence_submit_graphics = VK_NULL_HANDLE;


  // create graphics pipeline
  {
    // describe the descriptors in set 0
    std::array <VkDescriptorSetLayoutBinding, NUM_RESOURCES_GRAPHICS_SET_0> const descriptor_set_layout_binding_info_0 =
    {
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_CAMERA,              // at binding point 0 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // a uniform buffer
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_MODEL,               // at binding point 1 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // another uniform buffer
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_INFO,               // at binding point 2 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // another uniform buffer
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_POS_X,               // at binding point 3 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // a storage buffer
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      },
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_0_UBO_POS_Y,               // at binding point 4 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // another storage buffer
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      }
    };
    // describe the descriptors in set 1
    std::array <VkDescriptorSetLayoutBinding, NUM_RESOURCES_GRAPHICS_SET_1> const descriptor_set_layout_binding_info_1 =
    {
      VkDescriptorSetLayoutBinding {
        .binding = BINDING_ID_SET_1_TEXTURE,                         // at binding point 1 we have
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // an image
        .descriptorCount = 1u,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = VK_NULL_HANDLE
      }
    };
    // group up all descriptor layout descriptions
    std::array <std::span <const VkDescriptorSetLayoutBinding>, NUM_SETS_GRAPHICS> const descriptor_set_layout_bindings =
    {
      descriptor_set_layout_binding_info_0, // set 0
      descriptor_set_layout_binding_info_1  // set 1
    };
    if (!create_vulkan_descriptor_set_layouts (device,
      NUM_SETS_GRAPHICS,
      descriptor_set_layout_bindings.data (),
      descriptor_set_layouts_graphics.data ()))
    {
      DBG_ASSERT (false);
      return -1;
    }


    if (!create_vulkan_pipeline_layout (device,
      descriptor_set_layouts_graphics.size (), descriptor_set_layouts_graphics.data (),
      0u, nullptr,
      pipeline_layout_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }

    VkShaderModule shader_module_graphics_vert = VK_NULL_HANDLE;
    if (!create_vulkan_shader (device,
      COMPILED_GRAPHICS_SHADER_PATH_VERT,
      shader_module_graphics_vert))
    {
      DBG_ASSERT (false);
      return -1;
    }

    VkShaderModule shader_module_graphics_frag = VK_NULL_HANDLE;
    if (!create_vulkan_shader (device,
      COMPILED_GRAPHICS_SHADER_PATH_FRAG,
      shader_module_graphics_frag))
    {
      DBG_ASSERT (false);
      return -1;
    }


    VkViewport const viewport =
    {
      .x = 0.f,
      .y = 0.f,
      .width = (f32)extent.width,
      .height = (f32)extent.height,
      .minDepth = 0.f,
      .maxDepth = 1.f
    };
    VkRect2D const scissor =
    {
      .offset = { 0, 0 },
      .extent = extent
    };


    VkVertexInputBindingDescription vertex_input_binding_descriptions [] =
    {
      {
        .binding = 0u,
        .stride = sizeof (vertex_2d_uv),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      }
    };

    VkVertexInputAttributeDescription vertex_input_attribute_descriptions [] =
    {
      // position
      {
        .location = 0u,
        .binding = 0u,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = 0u
      },
      // tex_coord
      {
        .location = 1u,
        .binding = 0u,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = sizeof (f32) * 2u
      }
    };

    VkPipelineVertexInputStateCreateInfo const pvisci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .vertexBindingDescriptionCount = 1u,
      .pVertexBindingDescriptions = vertex_input_binding_descriptions,
      .vertexAttributeDescriptionCount = 2u,
      .pVertexAttributeDescriptions = vertex_input_attribute_descriptions
    };

    VkPipelineInputAssemblyStateCreateInfo const piasci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineRasterizationStateCreateInfo const prsci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.f,
      .depthBiasClamp = 0.f,
      .depthBiasSlopeFactor = 0.f,
      .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo const pmsci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.f,
      .pSampleMask = nullptr,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE
    };

    VkPipelineDepthStencilStateCreateInfo const pdssci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
      .depthBoundsTestEnable = VK_FALSE,
      .stencilTestEnable = VK_FALSE,
      .minDepthBounds = 0.f,
      .maxDepthBounds = 1.f
    };

    VkPipelineColorBlendAttachmentState const pipeline_colour_blend_attachment_states [] =
    {
      {
#if 1
        .blendEnable = VK_TRUE,

        // if you want alpha blending
        // colour.rgb = (srcColor * SRC_ALPHA) ADD (dstColor * ONE_MINUS_SRC_ALPHA)
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
#else
        .blendEnable = VK_FALSE,

        // colour.rgb = (srcColor * ONE) ADD (dstColor * ZERO)
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
#endif

        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
          | VK_COLOR_COMPONENT_G_BIT
          | VK_COLOR_COMPONENT_B_BIT
          | VK_COLOR_COMPONENT_A_BIT
      }
    };
    VkPipelineColorBlendStateCreateInfo const pcbsci =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_NO_OP,
      .attachmentCount = 1u,
      .pAttachments = pipeline_colour_blend_attachment_states,
      .blendConstants = { 0.f, 0.f, 0.f, 0.f }
    };

    if (!create_vulkan_pipeline_graphics (device,
      shader_module_graphics_vert, "main",
      shader_module_graphics_frag, "main",
      viewport, scissor,
      pvisci,
      piasci,
      prsci,
      pmsci,
      pdssci,
      pcbsci,
      pipeline_layout_graphics, render_pass, 0u,
      pipeline_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }

    release_vulkan_shader (device,
      shader_module_graphics_frag); // don't need shader object now we have the pipeline
    release_vulkan_shader (device,
      shader_module_graphics_vert);
  }
  // create graphics descriptor sets
  {
    // to support n-buffering, each frame must have it's own instances of the descriptor sets, each with their own resources
    // i.e. resources (should) not be used simultaneously across frames
    // we are double buffering framebuffers, because Vulkan forces us to...
    // BUT, we are syncing between each frame to keep things simple, so one instance is enough

    // create pool of descriptors from which our descriptor sets will draw from
    //
    // we could have multiple descriptor pools, or one huge one
    // we are going to have one per pipeline in order to keep it simple
    std::array <VkDescriptorPoolSize, 3u> const pool_sizes =
    {{ // yes this is deliberate!
      {
        // we have 2 x descriptors set that consist of 3 x uniform buffers
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 4u
      },
      {
          // we have 2 x descriptors set that consist of 2 x storage buffers
          .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          .descriptorCount = 4u
      },
      {
        // we have 2 x descriptor set that consist of 1 x image descriptor
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1u
      }
    }};
    if (!create_vulkan_descriptor_pool (device,
      4u, // how many descriptor sets will we make from the sets in the pool?
      pool_sizes.size (), pool_sizes.data (),
      descriptor_pool_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }

    std::array <vulkan_descriptor_set_info, NUM_SETS_GRAPHICS> const descriptor_set_infos =
    {{
      // set 0 - input
      {
        .desc_pool = &descriptor_pool_graphics,         // the pool from which to allocate the individual descriptors from
        .layout = &descriptor_set_layouts_graphics [0], // layout of the descriptor set
        .set_index = 0u,                                // index of the descriptor set
        .out_set = &desc_set_0_graphics_input           // pointer to where to instantiate the descriptor set to
      },
      // set 1 - input
      {
        .desc_pool = &descriptor_pool_graphics,
        .layout = &descriptor_set_layouts_graphics [1],
        .set_index = 1u,
        .out_set = &desc_set_1_graphics_input
      }

    }};
    if (!create_vulkan_descriptor_sets (device,
      descriptor_set_infos.size (), descriptor_set_infos.data ()))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // create graphics resources
  {

      VkFormat const image_format = VK_FORMAT_R8G8B8A8_UNORM;

      VkFormatProperties format_properties = {};
      // Get device properties for the requested texture format
      vkGetPhysicalDeviceFormatProperties(physical_device, image_format, &format_properties);
      // NOTE: Check if requested image format supports image storage operations (VK_IMAGE_USAGE_STORAGE_BIT)
      if ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0u)
      {
          DBG_ASSERT_MSG(false, "device does not support VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT\n");
          return -1;
      }

      if (!create_vulkan_texture(physical_device, device,
          TEXTURE_PATH,
          image_format, VK_IMAGE_LAYOUT_GENERAL,
          // but can not be 'VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL' because
          // it will be used initially in a VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, // look me up!
          texture_particle))
      {
          DBG_ASSERT(false);
          return -1;
      }

      if (!create_vulkan_buffer(physical_device, device,
          sizeof(camera_buffer),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_SHARING_MODE_EXCLUSIVE,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          buffer_graphics_camera))
      {
          DBG_ASSERT(false);
          return -1;
      }
      if (!create_vulkan_buffer(physical_device, device,
          sizeof(model_buffer),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_SHARING_MODE_EXCLUSIVE,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          buffer_graphics_model_input))
      {
          DBG_ASSERT(false);
          return -1;
      }
      if (!create_vulkan_buffer(physical_device, device,
          sizeof(model_buffer),
          VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
          VK_SHARING_MODE_EXCLUSIVE,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
          buffer_graphics_model_output))
      {
          DBG_ASSERT(false);
          return -1;
      }

      // TRANSITION IMAGE LAYOUT
      {
          // NOTE: get output image ready for rendering via fragment shader
          // can only perform this now compute write pipeline has completed
          if (!transition_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture_particle))
          {
              DBG_ASSERT(false);
              return -1;
          }

          // NOTE: this function is overloaded
          // the above version performs the transition as a 'one off' outside a command buffer
          // the other overload will add the required commands to an EXISTING command buffer
      }
    // as the data will never change (because we are not actually double buffering and the camera/sprites will never move)
    // we can set the data in these buffers upfront

    if (!map_and_unmap_memory (device,
      buffer_graphics_camera.memory,
      [extent](void* mem)
      {
        camera_buffer* buf = (camera_buffer*)mem;

        // put camera origin in centre of screen

        f32 const screen_width_half = (f32)extent.width / 2.f;
        f32 const screen_height_half = (f32)extent.height / 2.f;

        f32 const left = -screen_width_half, right = screen_width_half;
        f32 const bottom = -screen_height_half, top = screen_height_half;
        f32 const z_near = 0.f, z_far = 1.f;

        buf->vp_matrix = glm::ortho (left, right, bottom, top, z_near, z_far);
        buf->vp_matrix [1][1] *= -1.f;
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }

    vec2 const texture_dim = (vec2)texture_particle.dim * 0.65f;
    if (!map_and_unmap_memory (device,
      buffer_graphics_model_input.memory,
      [&texture_dim](void* mem)
      {
        model_buffer* buf = (model_buffer*)mem;

        buf->model_matrix = fast_transform_2D (-(texture_dim.x * particleScale) / 2.f, 0.f,
          0.f,
          0.f, 0.f,
          texture_dim.x * particleScale, texture_dim.y * particleScale);
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }

    if (!map_and_unmap_memory (device,
      buffer_graphics_model_output.memory,
      [&texture_dim](void* mem)
      {
        model_buffer* buf = (model_buffer*)mem;

        buf->model_matrix = fast_transform_2D (texture_dim.x / 2.f, 0.f,
          0.f,
          0.f, 0.f,
          texture_dim.x, texture_dim.y);
      }))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // bind graphics + compute resources to graphics descriptor sets
  {
      
    // desc_set_0_graphics_input  = for rendering input image  = buffer_graphics_camera + buffer_graphics_model_input
    // desc_set_1_graphics_input  = for rendering input image  = texture_compute_input
    // desc_set_0_graphics_output = for rendering output image = buffer_graphics_camera + buffer_graphics_model_output
    // desc_set_1_graphics_output = for rendering output image = texture_compute_output

    VkDescriptorBufferInfo const buffer_infos [] =
    {
      // TODO: add 3 x buffer infos
      // 
      // buffer_graphics_model_input.buffer
      // buffer_graphics_model_output.buffer
        {
            .buffer = buffer_graphics_camera.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        },
        {
            .buffer = buffer_graphics_model_input.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        },
        {
            .buffer = buffer_info.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        },
        {
            .buffer = buffer_pos_x.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        },
        {
            .buffer = buffer_pos_y.buffer,
            .offset = 0u,
            .range = VK_WHOLE_SIZE
        }
    };
    VkDescriptorImageInfo const image_infos [] =
    {
      {
        .sampler = texture_particle.sampler,
        .imageView = texture_particle.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL // not in this format yet, but it will be!
      },
      {
        .sampler = texture_particle.sampler,
        .imageView = texture_particle.view,
        .imageLayout = texture_particle.layout 
      }
    };

    VkWriteDescriptorSet const write_descriptors [] =
    {
      // TODO: fill in the gaps
        
      // desc_set_0_graphics_input - binding 0
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_graphics_input.desc_set,
        .dstBinding = 0u,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[0], // buffer_graphics_camera
        .pTexelBufferView = VK_NULL_HANDLE
      },
      // desc_set_0_graphics_input - binding 1
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_0_graphics_input.desc_set,
        .dstBinding = 1u,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pImageInfo = VK_NULL_HANDLE,
        .pBufferInfo = &buffer_infos[1], // buffer_graphics_model_input
        .pTexelBufferView = VK_NULL_HANDLE
      },

      // desc_set_1_graphics_input - binding 0
      {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        //.pNext = VK_NULL_HANDLE,
        .dstSet = desc_set_1_graphics_input.desc_set,
        .dstBinding = 0u,
        .dstArrayElement = 0u,
        .descriptorCount = 1u,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // combined image sampler
        .pImageInfo = &image_infos [0], // texture_compute_input
        .pBufferInfo = VK_NULL_HANDLE,
        .pTexelBufferView = VK_NULL_HANDLE
      }
        ,

        // desc_set_1_graphics_input - binding 0
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_graphics_input.desc_set,
          .dstBinding = 2u,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // combined image sampler
          .pImageInfo = VK_NULL_HANDLE , 
          .pBufferInfo = &buffer_infos[2], // texture_compute_input
          .pTexelBufferView = VK_NULL_HANDLE
        },

        // desc_set_1_graphics_input - binding 0
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_graphics_input.desc_set,
          .dstBinding = 3u,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // combined image sampler
          .pImageInfo = VK_NULL_HANDLE ,
          .pBufferInfo = &buffer_infos[3], // texture_compute_input
          .pTexelBufferView = VK_NULL_HANDLE
        },

        // desc_set_1_graphics_input - binding 0
        {
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          //.pNext = VK_NULL_HANDLE,
          .dstSet = desc_set_0_graphics_input.desc_set,
          .dstBinding = 4u,
          .dstArrayElement = 0u,
          .descriptorCount = 1u,
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // combined image sampler
          .pImageInfo = VK_NULL_HANDLE ,
          .pBufferInfo = &buffer_infos[4], // texture_compute_input
          .pTexelBufferView = VK_NULL_HANDLE
        }
    };

    vkUpdateDescriptorSets (device, // device
      6u,                           // descriptorWriteCount
      write_descriptors,            // pDescriptorWrites
      0u,                           // descriptorCopyCount
      VK_NULL_HANDLE);              // pDescriptorCopies
  }
  // create model data
  {
    vertex_2d_uv const vertices [] =
    {
      { .pos = { -.5f,  .5f }, .uv { 0.f, 0.f } }, // 0, top left
      { .pos = {  .5f,  .5f }, .uv { 1.f, 0.f } }, // 1, top right
      { .pos = { -.5f, -.5f }, .uv { 0.f, 1.f } }, // 2, bottom left
      { .pos = {  .5f, -.5f }, .uv { 1.f, 1.f } }, // 3, bottom right
    };
    u16 const indices [] = // clockwise order
    {
      0u, 1u, 2u, // face 0
      2u, 1u, 3u, // face 1
    };

    if (!create_vulkan_mesh (physical_device, device,
      (void*)vertices, 4u, sizeof (vertex_2d_uv),
      (void*)indices, 6u, sizeof (u16),
      mesh_sprite))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // create graphics command buffers
  {
    // we can have several command buffers per pipeline
    // but we are going to keep it simple
    // 1 command buffer for graphics that we reuse every frame

    if (!create_vulkan_command_pool (VK_QUEUE_GRAPHICS_BIT, command_pool_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_command_buffers (1u, command_pool_graphics, &command_buffer_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }
  // create graphics sync objects
  // at least one per submit (can be reused if in loop)
  {
    if (!create_vulkan_semaphores (1u, &swapchain_image_available_semaphore))
    {
      DBG_ASSERT (false);
      return -1;
    }
    if (!create_vulkan_fences (1u, 0u, &fence_submit_graphics))
    {
      DBG_ASSERT (false);
      return -1;
    }
  }


  // GRAPHICS RENDER

  // GAME LOOP
  while (process_os_messages ())
  {
    // UPDATE
    // COMPUTE DISPATCH
      {
          // RECORD COMMAND BUFFER
          {
              if (!CHECK_VULKAN_RESULT(vkResetCommandBuffer(command_buffer_compute, 0u)))
              {
                  DBG_ASSERT(false);
                  return -1;
              }
              if (!begin_command_buffer(command_buffer_compute, 0u))
              {
                  DBG_ASSERT(false);
                  return -1;
              }
              {
                  // any compute related command after this point is attached to this pipeline (on this command buffer)
                  vkCmdBindPipeline(command_buffer_compute, // Command Buffer
                      VK_PIPELINE_BIND_POINT_COMPUTE,       // Pipeline Bind Point
                      pipeline_compute);                    // Pipeline

                  // bind descriptor set - texture_compute_input & texture_compute_output
                  vkCmdBindDescriptorSets(command_buffer_compute, //  commandBuffer
                      VK_PIPELINE_BIND_POINT_COMPUTE,             //  pipelineBindPoint
                      pipeline_layout_compute,                    //  layout
                      desc_set_0_compute.set_index,               //  firstSet
                      1u,                                         //  descriptorSetCount
                      &desc_set_0_compute.desc_set,               //  pDescriptorSets
                      0u,                                         //  dynamicOffsetCount
                      VK_NULL_HANDLE);                            //  pDynamicOffsets

                  compute_push_constants const data =
                  {
                    .deltatime = 0.5f
                  };

                  vkCmdPushConstants(command_buffer_compute, // commandBuffer
                      pipeline_layout_compute,               // layout
                      VK_SHADER_STAGE_COMPUTE_BIT,           // stageFlags
                      0u,                                    // offset
                      sizeof(compute_push_constants),        // size
                      &data);                                // pValues

                  u32 ThreadGroup_x = 256u;

                  vkCmdDispatch(command_buffer_compute,
                      ThreadGroup_x, 1u, 1u);
              }
              if (!end_command_buffer(command_buffer_compute))
              {
                  DBG_ASSERT(false);
                  return -1;
              }
          }

          // SUBMIT
          {
              if (!CHECK_VULKAN_RESULT(vkResetFences(device, 1u, &fence_compute)))
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
                .pWaitSemaphores = VK_NULL_HANDLE,
                .pWaitDstStageMask = VK_NULL_HANDLE,
                .commandBufferCount = 1u,
                .pCommandBuffers = &command_buffer_compute,
                .signalSemaphoreCount = 0u,
                .pSignalSemaphores = VK_NULL_HANDLE
              };

              if (!CHECK_VULKAN_RESULT(vkQueueSubmit(queue_compute, 1u, &submit_info,
                  fence_compute))) // fence to signal when complete
              {
                  DBG_ASSERT(false);
                  return -1;
              }


              // wait for fence to be triggered via vkQueueSubmit (slow!)
              if (!CHECK_VULKAN_RESULT(vkWaitForFences(device,
                  1u,
                  &fence_compute,
                  VK_TRUE,
                  UINT64_MAX)))
              {
                  DBG_ASSERT(false);
                  return -1;
              }
          }


      }


    // RENDER
    {
      // this semaphore will be triggered when a swapchain image is available to be rendered to
      if (!acquire_next_swapchain_image (swapchain_image_available_semaphore))
      {
        DBG_ASSERT (false);
        return -1;
      }

      // RECORD COMMAND BUFFER(S)
      {
        // we are reusing the graphics command buffer, so need to ensure it starts of empty
        if (!CHECK_VULKAN_RESULT (vkResetCommandBuffer (command_buffer_graphics, 0u)))
        {
          DBG_ASSERT (false);
          return -1;
        }

        if (!begin_command_buffer (command_buffer_graphics, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT))
        {
          DBG_ASSERT (false);
          return -1;
        }


        begin_render_pass (command_buffer_graphics, { 0.39f, 0.8f, 0.92f }); // cornflower blue

        // any graphics related command after this point is attached to this pipeline (on this command buffer)
        // TODO: bind graphics pipeline
        vkCmdBindPipeline(command_buffer_graphics,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline_graphics);
        // render input image
        {
          VkDeviceSize offsets = 0u; // use me for 'pOffsets' and 'offset'
          // TODO: bind vertices
          vkCmdBindVertexBuffers(command_buffer_graphics,
              0u, 
              1u, 
              &mesh_sprite.buffer_vertex, 
              &offsets);
          // TODO: bind indices
          vkCmdBindIndexBuffer(command_buffer_graphics,
              mesh_sprite.buffer_index,
              offsets,
              VK_INDEX_TYPE_UINT16);
          // TODO: bind graphics input descriptor set 0 - buffer_graphics_camera + buffer_graphics_model_input
          vkCmdBindDescriptorSets(command_buffer_graphics,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              pipeline_layout_graphics,
              desc_set_0_graphics_input.set_index,
              1u,
              &desc_set_0_graphics_input.desc_set,
              0u,
              VK_NULL_HANDLE);
          // TODO: bind graphics input descriptor set 1 - texture_compute_input

          vkCmdBindDescriptorSets(command_buffer_graphics,
              VK_PIPELINE_BIND_POINT_GRAPHICS,
              pipeline_layout_graphics,
              desc_set_1_graphics_input.set_index,
              1u,
              &desc_set_1_graphics_input.desc_set,
              0u,
              VK_NULL_HANDLE);

          // TODO: call vkCmdDrawIndexed
          // indexCount = number of indices in our mesh
          // look in 'mesh_sprite'
          vkCmdDrawIndexed(command_buffer_graphics, // CommandBuffer
              mesh_sprite.num_indices,              // indexCount
              NUM_PARTICLES,                        // instanceCount
              0u,                                   // firstIndex
              0u,                                   // vertexOffset
              0u);                                  // firstINstance
        }

        end_render_pass (command_buffer_graphics);


        if (!end_command_buffer (command_buffer_graphics))
        {
          DBG_ASSERT (false);
          return -1;
        }
      }

      // SUBMIT
      {
        if (!CHECK_VULKAN_RESULT (vkResetFences (device, 1u, &fence_submit_graphics)))
        {
          DBG_ASSERT (false);
          return -1;
        }


        // submit graphics commands
        VkPipelineStageFlags const wait_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // look me up!
        // TODO: fill in 'submit_info'
        // remember we must WAIT for 'swapchain_image_available_semaphore' semaphore to be triggered before we submit
        VkSubmitInfo const submit_info =
        {
          .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
          //.pNext = VK_NULL_HANDLE,
          .waitSemaphoreCount = 1u,
          .pWaitSemaphores = &swapchain_image_available_semaphore, // wait for these semaphores to be signalled before submitting command buffers
          .pWaitDstStageMask = &wait_stage_mask,
          .commandBufferCount = 1u,
          .pCommandBuffers = &command_buffer_graphics,
          .signalSemaphoreCount = 0u,
          .pSignalSemaphores = VK_NULL_HANDLE                     // semaphores to trigger when command buffer has finished executing
        };

        if (!CHECK_VULKAN_RESULT (vkQueueSubmit (queue_graphics, 1u, &submit_info,
          fence_submit_graphics))) // fence to signal when complete
        {
          DBG_ASSERT (false);
          return -1;
        }


        // wait for fence to be triggered via vkQueueSubmit
        // would not do this here if 'properly' double buffering...
        // research it
        if (!CHECK_VULKAN_RESULT (vkWaitForFences (device,
          1u,
          &fence_submit_graphics,
          VK_TRUE,
          UINT64_MAX)))
        {
          DBG_ASSERT (false);
          return -1;
        }


      }

      if (!present ())
      {
        DBG_ASSERT (false);
        return -1;
      }
    }
  }


  // RELEASE
  {
    // GRAPHICS PIPELINE
    {
      release_vulkan_fences (1u, &fence_submit_graphics);
      release_vulkan_semaphores (1u, &swapchain_image_available_semaphore);

      release_vulkan_mesh (device, mesh_sprite);

      release_vulkan_command_buffers (1u, command_pool_graphics, &command_buffer_graphics);
      release_vulkan_command_pool (command_pool_graphics);

      release_vulkan_buffer (device, buffer_graphics_model_output);
      release_vulkan_buffer (device, buffer_graphics_model_input);
      release_vulkan_buffer (device, buffer_graphics_camera);

      release_vulkan_descriptor_sets (device,
        1u, &desc_set_1_graphics_input);
      release_vulkan_descriptor_sets (device,
        1u, &desc_set_0_graphics_input);
      release_vulkan_descriptor_pool (device, descriptor_pool_graphics);

      release_vulkan_pipeline (device, pipeline_graphics);
      release_vulkan_pipeline_layout (device, pipeline_layout_graphics);
      release_vulkan_descriptor_set_layouts (device,
        NUM_SETS_GRAPHICS, descriptor_set_layouts_graphics.data ());
    }

    // COMPUTE PIPELINE
    {
      release_vulkan_fences (1u, &fence_compute);

      release_vulkan_command_buffers (1u, command_pool_compute, &command_buffer_compute);
      release_vulkan_command_pool (command_pool_compute);

      release_vulkan_texture (device, texture_particle);

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
      release_vulkan_swapchain ();
      release_vulkan_device ();
      release_vulkan_surface ();
      release_vulkan_instance ();
      release_window ();
      queue_compute = queue_graphics = VK_NULL_HANDLE;
      render_pass = VK_NULL_HANDLE;
      extent = {};
      device = VK_NULL_HANDLE;
      physical_device = VK_NULL_HANDLE;
    }
  }

  return 0;
}
