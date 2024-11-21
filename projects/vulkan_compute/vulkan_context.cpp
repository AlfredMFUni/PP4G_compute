#include "vulkan_context.h"

#include "utility.h"          // for dprintf, DBG_ASSERT, DBG_ASSERT_...
#include "vulkan_resources.h" // for create_image_view_2d_default

#include <array>              // for std::array
#include <optional>           // for std::optional
#include <set>                // for std::set
#include <string>             // for std::string
#include <vector>             // for std::vector

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h> // for glfwGetWin32Window


#define ENABLE_VULKAN_DEBUG // enable code to get vulkan to tell us if we did anything wrong


#pragma region vulkan_window
GLFWwindow* s_window_handle = NULL;


static void glfw_error_callback (int error, char const* description)
{
  dprintf ("[GLFW] Error %d: %s\n", error, description);
}
bool create_window (char const*const title)
{
  DBG_ASSERT (s_window_handle == NULL);


  glfwSetErrorCallback (glfw_error_callback);

  if (glfwInit () == GLFW_FALSE)
  {
    return DBG_ASSERT_MSG (false, "GLFW: failed to initialise\n");
  }

  glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API); // required for surface creation later
  glfwWindowHint (GLFW_RESIZABLE, GL_FALSE); // prevent window resizing, would require a whole bunch of callbacks...
  s_window_handle = glfwCreateWindow (1280, 720, title, nullptr, nullptr);
  if (s_window_handle == NULL)
  {
    return DBG_ASSERT_MSG (false, "GLFW: failed to create window\n");
  }

  if (glfwVulkanSupported () == GLFW_FALSE)
  {
    return DBG_ASSERT_MSG (false, "GLFW: vulkan not supported\n");
  }

  return true;
}
#pragma endregion


#pragma region vulkan_instance
static VkInstance s_instance = VK_NULL_HANDLE;
#ifdef ENABLE_VULKAN_DEBUG
static VkDebugReportCallbackEXT s_debug_report_callback = VK_NULL_HANDLE;
static VkDebugUtilsMessengerEXT s_debug_messenger = VK_NULL_HANDLE;
#endif // ENABLE_VULKAN_DEBUG


static std::vector <char const*> const INSTANCE_LAYERS = {};
#ifdef ENABLE_VULKAN_DEBUG
static std::vector <char const*> const INSTANCE_LAYERS_DEBUG =
{
  "VK_LAYER_KHRONOS_synchronization2",
  "VK_LAYER_KHRONOS_validation"
};
#endif // ENABLE_VULKAN_DEBUG
static std::vector <char const*> const INSTANCE_EXTENSIONS = {};
#ifdef ENABLE_VULKAN_DEBUG
static std::vector <char const*> const INSTANCE_EXTENSIONS_DEBUG =
{
  VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
  VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};
#endif // ENABLE_VULKAN_DEBUG


#ifdef ENABLE_VULKAN_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_messenger_callback (VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
  VkDebugUtilsMessageTypeFlagsEXT message_type,
  VkDebugUtilsMessengerCallbackDataEXT const* callback_data,
  void*)
{
  dprintf ("Debug Messenger\n");

  dprintf ("Type: ");
  if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
  {
    dprintf ("GENERAL\n");
  }
  else
  {
    if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
    {
      if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
      {
        dprintf ("PERFORMANCE | VALIDATION\n");
      }
      else
      {
        dprintf ("PERFORMANCE\n");
      }
    }
    else if (message_type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
    {
      dprintf ("VALIDATION\n");
    }
  }

  dprintf ("Severity: ");
  if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
  {
    dprintf ("VERBOSE\n");
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    dprintf ("INFO\n");
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    dprintf ("WARNING\n");
  }
  else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    dprintf ("ERROR\n");
  }

  if (callback_data->objectCount > 0u)
  {
    dprintf ("Num Objects: %d\n", callback_data->objectCount);

    for (u32 object = 0u; object < callback_data->objectCount; ++object)
    {
      if (callback_data->pObjects [object].pObjectName != nullptr && strlen (callback_data->pObjects [object].pObjectName) > 0)
      {
        dprintf ("\tObject: %d - Type ID %d, Handle %p, Name %s\n", object,
          callback_data->pObjects [object].objectType,
          (void*)(callback_data->pObjects [object].objectHandle), callback_data->pObjects [object].pObjectName);
      }
      else
      {
        dprintf ("\tObject: %d - Type ID %d, Handle %p\n", object,
          callback_data->pObjects [object].objectType,
          (void*)(callback_data->pObjects [object].objectHandle));
      }
    }
  }

  if (callback_data->cmdBufLabelCount > 0u)
  {
    dprintf ("Num Command Buffer Labels: %d\n", callback_data->cmdBufLabelCount);

    for (u32 cmd_buf_label = 0u; cmd_buf_label < callback_data->cmdBufLabelCount; ++cmd_buf_label)
    {
      dprintf ("\tLabel: %d - %s { %f, %f, %f, %f }\n", cmd_buf_label,
        callback_data->pCmdBufLabels [cmd_buf_label].pLabelName,
        callback_data->pCmdBufLabels [cmd_buf_label].color [0], callback_data->pCmdBufLabels [cmd_buf_label].color [1],
        callback_data->pCmdBufLabels [cmd_buf_label].color [2], callback_data->pCmdBufLabels [cmd_buf_label].color [3]);
    }
  }

  dprintf ("Message ID Number: %d\n", callback_data->messageIdNumber);
  dprintf ("Message ID Name  : %s\n", callback_data->pMessageIdName);

  dprintf ("Message: %s\n", callback_data->pMessage);

  return VK_FALSE;
}
static void populate_debug_messenger_create_info (VkDebugUtilsMessengerCreateInfoEXT& out_dumci)
{
  out_dumci = {};
  out_dumci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  //out_dumci.pNext = VK_NULL_HANDLE;
  //out_dumci.flags = 0u;
  out_dumci.messageSeverity = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | */VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  out_dumci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  out_dumci.pfnUserCallback = debug_messenger_callback;
  //out_dumci.pUserData = nullptr;
}
#endif // ENABLE_VULKAN_DEBUG
static void print_available_instance_layers ()
{
#ifndef NDEBUG
  // get available instance layers
  u32 layer_count;
  VkResult result = vkEnumerateInstanceLayerProperties (&layer_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  std::vector <VkLayerProperties> available_layers (layer_count);
  result = vkEnumerateInstanceLayerProperties (&layer_count, available_layers.data ());
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // print available instance layers
  dprintf ("AVAILABLE INSTANCE LAYERS:\n");
  for (VkLayerProperties const& layer_properties : available_layers)
  {
    dprintf ("%s\n", layer_properties.layerName);
  }
#endif // NDEBUG
}
static void print_available_instance_extensions ()
{
#ifndef NDEBUG
  // get available instance extensions
  u32 extension_count;
  VkResult result = vkEnumerateInstanceExtensionProperties (VK_NULL_HANDLE, &extension_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  std::vector <VkExtensionProperties> available_extensions (extension_count);
  result = vkEnumerateInstanceExtensionProperties (VK_NULL_HANDLE, &extension_count, available_extensions.data ());
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // print available instance extensions
  dprintf ("AVAILABLE INSTANCE EXTENSIONS:\n");
  for (VkExtensionProperties const& entension_properties : available_extensions)
  {
    dprintf ("%s\n", entension_properties.extensionName);
  }
#endif // NDEBUG
}
static std::vector <char const*> get_required_instance_layers ()
{
  std::vector <char const*> layers = INSTANCE_LAYERS;

#ifdef ENABLE_VULKAN_DEBUG
  layers.insert (layers.begin (), INSTANCE_LAYERS_DEBUG.begin (), INSTANCE_LAYERS_DEBUG.end ());
#endif // ENABLE_VULKAN_DEBUG

  return layers;
}
static std::vector <char const*> get_required_instance_extensions ()
{
  std::vector <char const*> extensions = INSTANCE_EXTENSIONS;

#ifdef ENABLE_VULKAN_DEBUG
  extensions.insert (extensions.begin (), INSTANCE_EXTENSIONS_DEBUG.begin (), INSTANCE_EXTENSIONS_DEBUG.end ());
#endif // ENABLE_VULKAN_DEBUG

  u32 glfw_required_extensions_count = 0u;
  char const** glfw_required_extensions = glfwGetRequiredInstanceExtensions (&glfw_required_extensions_count);
  for (u32 i = 0u; i < glfw_required_extensions_count; ++i)
  {
    extensions.push_back (glfw_required_extensions [i]);
  }

  return extensions;
}
#ifdef ENABLE_VULKAN_DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback (VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT,
  uint64_t object, size_t, int32_t, char const* layer_prefix, char const* message, void*)
{
  dprintf ("Debug Report\n");
  dprintf ("ObjectType: %i\nFrom Layer: %s\nMessage: %s\n", object, layer_prefix, message);

  return VK_FALSE;
}
#endif // ENABLE_VULKAN_DEBUG
static bool create_instance ()
{
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_instance));


  VkApplicationInfo const application_info =
  {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    //.pNext = VK_NULL_HANDLE,
    .pApplicationName = "vulkan_app", // TODO: change to name you want :)
    .engineVersion = 0u,
    .apiVersion = VK_API_VERSION_1_2
  };

#ifdef ENABLE_VULKAN_DEBUG
  // This is info to add a temporary callback to use during vkCreateInstance
  // After the instance is created, we use the instance-based function to register the final callback
  VkDebugUtilsMessengerCreateInfoEXT dumci = {};
  populate_debug_messenger_create_info (dumci);
#endif // ENABLE_VULKAN_DEBUG

  print_available_instance_layers ();
  print_available_instance_extensions ();
  std::vector <char const*> layers = get_required_instance_layers ();
  std::vector <char const*> extensions = get_required_instance_extensions ();

  // TODO: fix VkInstanceCreateInfo
  VkInstanceCreateInfo const ici =
  {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
#ifdef ENABLE_VULKAN_DEBUG
    .pNext = &dumci,
#endif // ENABLE_VULKAN_DEBUG
    //.flags = 0u,
    .pApplicationInfo = &application_info,          
    .enabledLayerCount = (u32)layers.size(),            // number of layers
    .ppEnabledLayerNames = layers.data(),               // layer names
    .enabledExtensionCount = (u32)extensions.size(),    // number of extensions
    .ppEnabledExtensionNames = extensions.data()        // extension names
  };
  
  VkResult const result = vkCreateInstance(&ici,    // pCreateInfo
      VK_NULL_HANDLE,                               // pAllocator
      &s_instance);                                 // pInstance

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (s_instance))
  {
    return DBG_ASSERT_MSG (false, "failed to create instance\n");
  }

  return true;
}

static bool create_debug_messenger ()
{
#ifdef ENABLE_VULKAN_DEBUG
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_debug_report_callback));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_debug_messenger));


  // create Debug Report Callback

  // as the VK_EXT_debug_report instance extension is not a core feature,the addresses
  // of the entry points has to be acquired through via the vkGetInstanceProcAddr function
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = VK_NULL_HANDLE;
  *(void**)&vkCreateDebugReportCallbackEXT = vkGetInstanceProcAddr (s_instance, "vkCreateDebugReportCallbackEXT");
  if (!vkCreateDebugReportCallbackEXT)
  {
    //VK_ERROR_EXTENSION_NOT_PRESENT
    return DBG_ASSERT_MSG (false, "Extension 'VK_EXT_debug_report' is not loaded\n");
  }

  // we could separate this e.g. errors go to 'callback a' & warnings go to 'callback b'
  VkDebugReportCallbackCreateInfoEXT const cbci =
  {
    .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
    //.pNext = VK_NULL_HANDLE,
    .flags = VK_DEBUG_REPORT_WARNING_BIT_EXT
      | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
      | VK_DEBUG_REPORT_ERROR_BIT_EXT,
    .pfnCallback = &debug_report_callback,
    //.pUserData = VK_NULL_HANDLE
  };

  VkResult result = vkCreateDebugReportCallbackEXT (s_instance,
    &cbci,
    VK_NULL_HANDLE,
    &s_debug_report_callback);
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (s_debug_report_callback))
  {
    return DBG_ASSERT_MSG (false, "VkDebugReportCallbackCreateInfoEXT failed\n");
  }


  // create Debug Utils Messenger

  // use same error callback function here that we used when creating 'VkInstanceCreateInfo'
  VkDebugUtilsMessengerCreateInfoEXT dumci = {};
  populate_debug_messenger_create_info (dumci);

  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr (s_instance, "vkCreateDebugUtilsMessengerEXT");
  if (!func)
  {
    //VK_ERROR_EXTENSION_NOT_PRESENT
    return DBG_ASSERT_MSG (false, "Extension 'VK_EXT_debug_utils' is not loaded\n");
  }

  result = func (s_instance, &dumci, VK_NULL_HANDLE, &s_debug_messenger);
  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (s_debug_messenger))
  {
    return DBG_ASSERT_MSG (false, "vkCreateDebugUtilsMessengerEXT failed\n");
  }
#endif // ENABLE_VULKAN_DEBUG

  return true;
}

bool create_vulkan_instance ()
{
  if (!create_instance ()) return false;
  return create_debug_messenger ();
}
#pragma endregion


#pragma region vulkan_surface
VkSurfaceKHR s_surface = VK_NULL_HANDLE;


bool create_vulkan_surface ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));
  DBG_ASSERT_MSG (CHECK_VULKAN_HANDLE (s_window_handle),
    "you must call 'create_window' before 'create_vulkan_surface'\n");
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_surface));


  // TODO: ask Andy for graphics starter code

  return true;
}
#pragma endregion


#pragma region vulkan_device
struct queue_family_indices
{
  std::optional <u32> graphics_family;
  std::optional <u32> present_family;
  std::optional <u32> compute_family;
  //std::optional <u32> transfer_family;

  bool is_complete (VkQueueFlags requested_queue_types) const
  {
    bool const found_graphics_queue = requested_queue_types & VK_QUEUE_GRAPHICS_BIT ? graphics_family.has_value () && present_family.has_value () : true;
    bool const found_compute_queue = requested_queue_types & VK_QUEUE_COMPUTE_BIT ? compute_family.has_value () : true;
    //bool const found_transfer_queue = requested_queue_types & VK_QUEUE_TRANSFER_BIT ? transfer_family.has_value () : true;

    return found_graphics_queue && found_compute_queue;//&& found_transfer_queue;
  }
  void clear ()
  {
    graphics_family.reset ();
    present_family.reset ();
    compute_family.reset ();
    //transfer_family.reset ();
  }
};
struct swapchain_support
{
  VkSurfaceCapabilitiesKHR capabilities = {};
  std::vector <VkSurfaceFormatKHR> formats;
  std::vector <VkPresentModeKHR> present_modes;
};


static queue_family_indices s_queue_indices;
static VkPhysicalDevice s_physical_device = VK_NULL_HANDLE;
static VkDevice s_device = VK_NULL_HANDLE;


static std::vector <char const*> const DEVICE_EXTENSIONS = {};
#ifdef ENABLE_VULKAN_DEBUG
static std::vector <char const*> const DEVICE_EXTENSIONS_DEBUG = {};
#endif // ENABLE_VULKAN_DEBUG


static void print_device_info (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


#ifndef NDEBUG
  VkPhysicalDeviceProperties device_properties = {};
  vkGetPhysicalDeviceProperties (pd, &device_properties);

  dprintf ("Device Name:    %s\n", device_properties.deviceName);
  dprintf ("API Version:    %d.%d.%d\n",
    VK_API_VERSION_MAJOR (device_properties.apiVersion),
    VK_API_VERSION_MINOR (device_properties.apiVersion),
    VK_API_VERSION_PATCH (device_properties.apiVersion));
  dprintf ("Driver Version: %d.%d.%d\n",
    (device_properties.driverVersion >> 22),
    (device_properties.driverVersion >> 12) & 0x3ff,
    (device_properties.driverVersion) & 0xfff);
  //dprintf ("Device Type:    %d\n", device_properties.deviceType);
#endif // NDEBUG
}
static void print_available_device_layers (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


#ifndef NDEBUG
  // get num available device layers
  u32 layer_count;
  VkResult result = vkEnumerateDeviceLayerProperties (pd, &layer_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // get available device layers
  std::vector <VkLayerProperties> available_layers (layer_count);
  result = vkEnumerateDeviceLayerProperties (pd, &layer_count, available_layers.data ());
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // print available device layers
  dprintf ("AVAILABLE DEVICE LAYERS:\n");
  for (VkLayerProperties const& layer_properties : available_layers)
  {
    dprintf ("%s\n", layer_properties.layerName);
  }
#endif // NDEBUG
}
static void print_available_device_extensions (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


#ifndef NDEBUG
  // get num available device extensions
  u32 extension_count;
  VkResult result = vkEnumerateDeviceExtensionProperties (pd, VK_NULL_HANDLE, &extension_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // get available device extensions
  std::vector <VkExtensionProperties> available_extensions (extension_count);
  result = vkEnumerateDeviceExtensionProperties (pd, VK_NULL_HANDLE, &extension_count, available_extensions.data ());
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // print available device extensions
  dprintf ("AVAILABLE DEVICE EXTENSIONS:\n");
  for (VkExtensionProperties const& entension_properties : available_extensions)
  {
    dprintf ("%s\n", entension_properties.extensionName);
  }
#endif // NDEBUG
}
static void print_queue_family_capabilities (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


#ifndef NDEBUG
  // get num queue family capabilities
  u32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties (pd, // physicalDevice
    &queue_family_count,                        // pQueueFamilyPropertyCount
    VK_NULL_HANDLE);                            // pQueueFamilyProperties

  // get queue family capabilities
  std::vector <VkQueueFamilyProperties> family_properties (queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties (pd, // physicalDevice
    &queue_family_count,                        // pQueueFamilyPropertyCount
    family_properties.data ());                 // pQueueFamilyProperties

  // print queue family capabilities
  for (u32 i = 0u; i < queue_family_count; ++i)
  {
    dprintf ("queue family: %d\n", i);
    dprintf ("queue count: %d\n", family_properties [i].queueCount);
    dprintf ("supported operations on these queues:\n");
    if (family_properties [i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      dprintf ("- Graphics\n");
    }
    if (family_properties [i].queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      dprintf ("- Compute\n");
    }
    if (family_properties [i].queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
      dprintf ("- Transfer\n");
    }
    if (family_properties [i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT)
    {
      dprintf ("- Sparse Binding\n");
    }
  }
#endif // NDEBUG
}
static bool find_compatible_queue_families (VkPhysicalDevice pd, VkQueueFlags requested_queue_types,
  queue_family_indices& out_indices)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


  print_queue_family_capabilities (pd);

  // get num queue family capabilities
  u32 queue_family_count = 0u;
  vkGetPhysicalDeviceQueueFamilyProperties (pd, // physicalDevice
    &queue_family_count,                        // pQueueFamilyPropertyCount
    VK_NULL_HANDLE);                            // pQueueFamilyProperties

  // get queue family capabilities
  std::vector <VkQueueFamilyProperties> queue_families (queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties (pd, // physicalDevice
    &queue_family_count,                        // pQueueFamilyPropertyCount
    queue_families.data ());                    // pQueueFamilyProperties

  out_indices.clear ();
  u32 i = 0u;
  while (i < queue_families.size () && !out_indices.is_complete (requested_queue_types))
  {
    VkQueueFamilyProperties const& queue_family = queue_families [i];

    if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT // it's available
      && requested_queue_types & VK_QUEUE_GRAPHICS_BIT) // and was requested
    {
      out_indices.graphics_family = i;

      // check if phyical device supports 'present' for the surface on this queue
      DBG_ASSERT (CHECK_VULKAN_HANDLE (s_surface));
      VkBool32 present_support = VK_FALSE;
      VkResult const result = vkGetPhysicalDeviceSurfaceSupportKHR (pd, i, s_surface, &present_support);
      DBG_ASSERT (CHECK_VULKAN_RESULT (result));
      if (present_support == VK_TRUE)
      {
        out_indices.present_family = i;
      }
    }
    if (queue_family.queueFlags & VK_QUEUE_COMPUTE_BIT
      && requested_queue_types & VK_QUEUE_COMPUTE_BIT)
    {
      out_indices.compute_family = i;
    }

    // for transfer: check 'queue_family.queueFlags' & VK_QUEUE_TRANSFER_BIT'

    ++i;
  }

  return out_indices.is_complete (requested_queue_types);
}
static std::vector <char const*> get_required_device_extensions ()
{
  std::vector <char const*> extensions = DEVICE_EXTENSIONS;

#ifdef ENABLE_VULKAN_DEBUG
  extensions.insert (extensions.begin (), DEVICE_EXTENSIONS_DEBUG.begin (), DEVICE_EXTENSIONS_DEBUG.end ());
#endif // ENABLE_VULKAN_DEBUG

  if (s_queue_indices.graphics_family.has_value () && s_queue_indices.present_family.has_value ())
  {
    extensions.push_back (VK_KHR_SWAPCHAIN_EXTENSION_NAME);
  }

  return extensions;
}
static bool check_device_extension_support (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


  // get num available device extensions
  u32 extension_count;
  VkResult result = vkEnumerateDeviceExtensionProperties (pd, VK_NULL_HANDLE, &extension_count, VK_NULL_HANDLE);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }

  // get available device extensions
  std::vector <VkExtensionProperties> available_extensions (extension_count);
  result = vkEnumerateDeviceExtensionProperties (pd, VK_NULL_HANDLE, &extension_count, available_extensions.data ());
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }

  std::vector <char const*> required_extensions = get_required_device_extensions ();
  std::set <std::string> required_extensions_set (required_extensions.begin (), required_extensions.end ());
  for (VkExtensionProperties const& extension : available_extensions)
  {
    required_extensions_set.erase (extension.extensionName);
  }

  return required_extensions_set.empty ();
}
static swapchain_support query_swapchain_support (VkPhysicalDevice pd)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_surface));


  swapchain_support details;

  VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (pd, s_surface, &details.capabilities);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));

  // get num supported surface formats
  u32 format_count = {};
  result = vkGetPhysicalDeviceSurfaceFormatsKHR (pd, s_surface, &format_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));
  if (format_count != 0u)
  {
    // get supported surface formats
    details.formats.resize (format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR (pd, s_surface, &format_count, details.formats.data ());
    DBG_ASSERT (CHECK_VULKAN_RESULT (result));
  }

  // get num supported present modes
  u32 present_mode_count = {};
  result = vkGetPhysicalDeviceSurfacePresentModesKHR (pd, s_surface, &present_mode_count, VK_NULL_HANDLE);
  DBG_ASSERT (CHECK_VULKAN_RESULT (result));
  if (present_mode_count != 0u)
  {
    // get supported present modes
    details.present_modes.resize (present_mode_count);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR (pd, s_surface, &present_mode_count, details.present_modes.data ());
    DBG_ASSERT (CHECK_VULKAN_RESULT (result));
  }

  return details;
}
static bool is_physical_device_suitable (VkPhysicalDevice pd,
  VkQueueFlags requested_queue_types)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (pd));


  // see whether this physical device can support all the features we are going to request

  queue_family_indices indices;
  bool const supports_requested_queue_types = find_compatible_queue_families (pd, requested_queue_types, indices);

  bool const extensions_supported = check_device_extension_support (pd);

  bool sc_adequate = true;
  if ((requested_queue_types & VK_QUEUE_GRAPHICS_BIT) > 0 && extensions_supported)
  {
    swapchain_support sc_support = query_swapchain_support (pd);
    sc_adequate = !sc_support.formats.empty () && !sc_support.present_modes.empty ();
  }

  return supports_requested_queue_types && extensions_supported && sc_adequate;
}
static bool create_physical_device (VkQueueFlags requested_queue_types)
{
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_physical_device));


  // query how many devices are present and available
  u32 device_count = 0u;

  VkResult result = vkEnumeratePhysicalDevices (s_instance, // VkInstance
    &device_count,                                          // pPhysicalDeviceCount
    VK_NULL_HANDLE);                                        // VkPhysicalDevice
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "failed to enumerate physical devices present\n");
  }
  if (device_count == 0)
  {
    return DBG_ASSERT_MSG (false, "couldn't detect any device present with Vulkan support\n");
  }

  // array to store our physical device info
  std::vector <VkPhysicalDevice> physical_devices (device_count);
  // get the physical devices
  result = vkEnumeratePhysicalDevices (s_instance, // VkInstance
    &device_count,                                 // pPhysicalDeviceCount
    physical_devices.data ());                     // pPhysicalDevices
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "failed to enumerate physical devices present\n");
  }
  if (physical_devices.size () == 0) // ensure there is atleast 1 device
  {
    return DBG_ASSERT (false);
  }


  // select first suitable device
  for (VkPhysicalDevice& pd : physical_devices)
  {
    print_device_info (pd);
    print_available_device_layers (pd);
    print_available_device_extensions (pd);

    if (is_physical_device_suitable (pd, requested_queue_types))
    {
      s_physical_device = pd;
      break;
    }
  }

  return true;
}

static bool create_logical_device (VkQueueFlags requested_queue_types)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_physical_device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_device));


  // double check this physical device supports the queue types (and surface) we need
  // and stash queue indices
  bool const supports_requested_queue_types = find_compatible_queue_families (s_physical_device,
    requested_queue_types, s_queue_indices);
  if (!supports_requested_queue_types)
  {
    return DBG_ASSERT_MSG (false, "failed to find requested queue types\n");
  }

  // get the queue types family indices the device supports
  std::set <u32> unique_queue_families;
  if (s_queue_indices.graphics_family.has_value () && s_queue_indices.present_family.has_value ())
  {
    unique_queue_families.insert (s_queue_indices.graphics_family.value ());
    unique_queue_families.insert (s_queue_indices.present_family.value ());
  }
  if (s_queue_indices.compute_family.has_value ())
  {
    unique_queue_families.insert (s_queue_indices.compute_family.value ());
  }
  // add queue indices for present and transfer here also (if you want them)

  float const queue_priority = 1.f; // give all queues equal poriority
  std::vector <VkDeviceQueueCreateInfo> queue_create_infos;
  for (u32 const& queue_family : unique_queue_families)
  {
    VkDeviceQueueCreateInfo const qci =
    {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      //.flags = 0u,
      .queueFamilyIndex = queue_family,
      .queueCount = 1u,
      .pQueuePriorities = &queue_priority
    };
    queue_create_infos.push_back (qci);
  }

  std::vector <char const*> extensions = get_required_device_extensions ();

  // only request the features if they are supported!
  VkPhysicalDeviceFeatures supported_device_features = {};
  vkGetPhysicalDeviceFeatures (s_physical_device, &supported_device_features);
  VkPhysicalDeviceFeatures const device_features =
  {
    // add features you want to enable here
    // e.g. samplerAnisotropy and shaderClipDistance
    // just make sure they are supported first!
  };

  
  VkDeviceCreateInfo const dci =
  {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
    .queueCreateInfoCount = (u32)queue_create_infos.size (),
    .pQueueCreateInfos = queue_create_infos.data (),
    //.enabledLayerCount                              // deprecated: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#extendingvulkan-layers-devicelayerdeprecation
    //.ppEnabledLayerNames                            // deprecated
    .enabledExtensionCount = (u32)extensions.size(),  //
    .ppEnabledExtensionNames = extensions.data(),     //
    .pEnabledFeatures = &device_features              //
  };

  VkResult const result = vkCreateDevice(s_physical_device,     // physicalDevice
      &dci,                                                     // pCreateInfo
      VK_NULL_HANDLE,                                           // pAllocator
      &s_device);                                               // pDevice

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (s_device))
  {
    return DBG_ASSERT_MSG (false, "failed to create logical device\n");
  }

  return true;
}

bool create_vulkan_device (VkQueueFlags requested_queue_types,
  VkPhysicalDevice& out_physical_device, VkDevice& out_device)
{
  if ((requested_queue_types & VK_QUEUE_GRAPHICS_BIT) > 0)
  {
    DBG_ASSERT_MSG (CHECK_VULKAN_HANDLE (s_surface),
      "you must call 'create_vulkan_surface' to request 'VK_QUEUE_GRAPHICS_BIT'\n");
  }


  if (!create_physical_device (requested_queue_types)) return false;
  if (!create_logical_device (requested_queue_types)) return false;

  out_physical_device = s_physical_device;
  out_device = s_device;

  return true;
}
#pragma endregion


#pragma region vulkan_swapchain
static VkSwapchainKHR s_swapchain = VK_NULL_HANDLE;
static VkExtent2D s_framebuffer_extent = {};
static std::array <VkImage, NUM_SWAPCHAIN_IMAGES> s_framebuffer_images = { VK_NULL_HANDLE, VK_NULL_HANDLE };
static VkFormat s_framebuffer_image_format = VK_FORMAT_UNDEFINED;
static std::array <VkImageView, NUM_SWAPCHAIN_IMAGES> s_framebuffer_image_views = { VK_NULL_HANDLE, VK_NULL_HANDLE };
static std::array <VkFramebuffer, NUM_SWAPCHAIN_IMAGES> s_framebuffers = { VK_NULL_HANDLE, VK_NULL_HANDLE };
static u32 s_swapchain_index = {};
static VkRenderPass s_render_pass = VK_NULL_HANDLE;


static VkExtent2D choose_swap_extent ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_physical_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_window_handle));


  swapchain_support sc_support = query_swapchain_support (s_physical_device);
  if (sc_support.capabilities.currentExtent.width != UINT32_MAX)
  {
    return sc_support.capabilities.currentExtent;
  }

  int width, height;
  glfwGetFramebufferSize (s_window_handle, &width, &height);
  VkExtent2D const actual_extent =
  {
    .width = glm::clamp ((u32)width, sc_support.capabilities.minImageExtent.width, sc_support.capabilities.maxImageExtent.width),
    .height = glm::clamp ((u32)height, sc_support.capabilities.minImageExtent.height, sc_support.capabilities.maxImageExtent.height)
  };

  return actual_extent;
}
static VkSurfaceFormatKHR choose_swap_surface_format (std::vector <VkSurfaceFormatKHR> const& available_formats)
{
  for (VkSurfaceFormatKHR const& available_format : available_formats)
  {
    if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    {
      return available_format;
    }
  }

  return available_formats [0];
}
static VkPresentModeKHR choose_swap_present_mode (std::vector <VkPresentModeKHR> const& available_present_modes, bool vsync)
{
  // https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html

  if (vsync)
  {
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  for (VkPresentModeKHR const& available_present_mode : available_present_modes)
  {
    if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) // no vsync, if available
    {
      return available_present_mode;
    }
  }

  for (VkPresentModeKHR const& available_present_mode : available_present_modes)
  {
    if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) // best of the rest
    {
      return available_present_mode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR; // must be supported by all GPUs that support Vulkan
}
static bool create_swapchain ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_physical_device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_swapchain));


  // TODO: ask Andy for graphics starter code

  return true;
}

static bool create_framebuffer_image_views ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));


  // TODO: ask Andy for graphics starter code

  return true;
}

static bool create_render_pass ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (s_framebuffer_image_format != 0u);
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (s_render_pass));


  // TODO: ask Andy for graphics starter code

  return true;
}

static bool create_framebuffers ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_render_pass));


  // TODO: ask Andy for graphics starter code

  return true;
}


bool create_vulkan_swapchain (VkExtent2D& out_extent, VkRenderPass& out_render_pass)
{
  if (!create_swapchain ()) return false;
  if (!create_framebuffer_image_views ()) return false;
  if (!create_render_pass ()) return false;
  if (!create_framebuffers ()) return false;

  out_extent = s_framebuffer_extent;
  out_render_pass = s_render_pass;

  return true;
}
#pragma endregion


#pragma region vulkan_queue
bool get_vulkan_queue_compute (VkQueue& out_queue)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (s_queue_indices.compute_family.has_value ());
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_queue));


  vkGetDeviceQueue (s_device, s_queue_indices.compute_family.value (), 0u, &out_queue);
  if (!CHECK_VULKAN_HANDLE (out_queue))
  {
    return DBG_ASSERT_MSG (false, "failed to get queue\n");
  }

  return true;
}
bool get_vulkan_queue_graphics (VkQueue& out_queue)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (s_queue_indices.graphics_family.has_value ());
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_queue));


  vkGetDeviceQueue (s_device, s_queue_indices.graphics_family.value (), 0u, &out_queue);
  if (!CHECK_VULKAN_HANDLE (out_queue))
  {
    return DBG_ASSERT_MSG (false, "failed to get queue\n");
  }

  return true;
}
#pragma endregion


#pragma region vulkan_sync
bool create_vulkan_semaphores (u32 num, VkSemaphore* out_semaphores)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (num > 0u);
  DBG_ASSERT (out_semaphores != nullptr);


  VkSemaphoreCreateInfo const sci =
  {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.flags = 0u,
  };
  for (u32 i = 0u; i < num; ++i)
  {
    if (CHECK_VULKAN_HANDLE (out_semaphores [i]))
    {
      return DBG_ASSERT (false);
    }

    VkResult const result = vkCreateSemaphore (s_device, // device
      &sci,                                              // pCreateInfo
      nullptr,                                           // pAllocator
      &out_semaphores [i]);                              // pSemaphore
    if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_semaphores [i]))
    {
      return DBG_ASSERT_MSG (false, "failed to create semaphore\n");
    }
  }

  return true;
}
bool create_vulkan_fences (u32 num, VkFenceCreateFlags flags, VkFence* out_fences)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (num > 0u);
  DBG_ASSERT (out_fences != nullptr);


  VkFenceCreateInfo const fci =
  {
    .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    .flags = flags
  };
  for (u32 i = 0u; i < num; ++i)
  {
    if (CHECK_VULKAN_HANDLE (out_fences [i]))
    {
      return DBG_ASSERT (false);
    }

    VkResult const result = vkCreateFence (s_device, // device
      &fci,                                          // pCreateInfo
      VK_NULL_HANDLE,                                // pAllocator
      &out_fences [i]);                              // pFence
    if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_fences [i]))
    {
      return DBG_ASSERT_MSG (false, "failed to create fence\n");
    }
  }

  return true;
}
#pragma endregion


#pragma region vulkan_command
bool create_vulkan_command_pool (VkQueueFlags requested_queue_type, VkCommandPool& out_command_pool)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_command_pool));


  // a 'VkCommandPool' can be associated with several 'VkCommandBuffer's
  // but must be in the same queue family
  // see 'VkCommandPoolCreateInfo' below


  std::optional <u32> family;
  if ((requested_queue_type & VK_QUEUE_GRAPHICS_BIT) > 0)
  {
    family = s_queue_indices.graphics_family;
  }
  else if ((requested_queue_type & VK_QUEUE_COMPUTE_BIT) > 0)
  {
    family = s_queue_indices.compute_family;
  }
  if (!family.has_value ())
  {
    return DBG_ASSERT_MSG (false, "there is no valid queue for the requested queue type\n");
  }

  VkCommandPoolCreateInfo const cpci =
  {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    //.pNext = VK_NULL_HANDLE,
    .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, // look me up!
    .queueFamilyIndex = family.value ()
  };

  VkResult const result = vkCreateCommandPool(s_device, // device
      &cpci,                                            // pCreateInfo
      VK_NULL_HANDLE,                                   // pAllocator
      &out_command_pool);                               // pCommandPool

  if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_command_pool))
  {
    return DBG_ASSERT_MSG (false, "failed create command pool\n");
  }

  return true;
}
bool create_vulkan_command_buffers (u32 num, VkCommandPool command_pool, VkCommandBuffer* out_command_buffers)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (num > 0u);
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_pool));
  DBG_ASSERT (out_command_buffers != nullptr);


  VkCommandBufferAllocateInfo const cbai
  {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      //.pNext = VK_NULL_HANDLE,
      .commandPool = command_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = num,
  };
  // make sure it is a PRIMARY level command buffer

  for (u32 i = 0u; i < num; ++i)
  {
    DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_command_buffers [i]));


    VkResult const result = vkAllocateCommandBuffers (s_device, // device
      &cbai,                                                    // pAllocateInfo
      &out_command_buffers [i]);                                // pCommandBuffers
    if (!CHECK_VULKAN_RESULT (result) || !CHECK_VULKAN_HANDLE (out_command_buffers [i]))
    {
      return DBG_ASSERT_MSG (false, "failed create command buffer\n");
    }
  }

  return true;
}
#pragma endregion


bool process_os_messages ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_window_handle));


  if (glfwWindowShouldClose (s_window_handle)) return false;
  glfwPollEvents ();
  return true;
}
bool acquire_next_swapchain_image (VkSemaphore swapchain_image_available_semaphore)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_swapchain));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (swapchain_image_available_semaphore));


  // TODO: ask Andy for graphics starter code

  return true;
}
void begin_render_pass (VkCommandBuffer command_buffer, vec3 const& clear_colour)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_render_pass));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_framebuffers [s_swapchain_index]));


  VkClearColorValue const ccv = {clear_colour.x, clear_colour.y, clear_colour.z, 1.f};
  VkClearValue const clear_values [] = // would need more if you wanted to support a depth buffer
  {
    {
      .color = ccv
    }
  };

  VkRenderPassBeginInfo const rpbi =
  {
    .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
    //.pNext = VK_NULL_HANDLE,
    .renderPass = s_render_pass,
    .framebuffer = s_framebuffers [s_swapchain_index],
    .renderArea = { { 0, 0 }, s_framebuffer_extent },
    .clearValueCount = 1u,
    .pClearValues = clear_values
  };

  vkCmdBeginRenderPass (command_buffer, // commandBuffer
    &rpbi,                              // pRenderPassBegin
    VK_SUBPASS_CONTENTS_INLINE);        // contents


  VkViewport const viewport =
  {
    .x = 0.f,
    .y = 0.f,
    .width = (f32)s_framebuffer_extent.width,
    .height = (f32)s_framebuffer_extent.height,
    .minDepth = 0.f,
    .maxDepth = 1.f
  };

  vkCmdSetViewport (command_buffer, // commandBuffer
    0u,                             // firstViewport
    1u,                             // viewportCount
    &viewport);                     // pViewports


  VkRect2D const scissor =
  {
    .offset = { 0, 0 },
    .extent = s_framebuffer_extent
  };
  vkCmdSetScissor (command_buffer, // commandBuffer
    0u,                            // firstScissor
    1u,                            // scissorCount
    &scissor);                     // pScissors
}
void end_render_pass (VkCommandBuffer command_buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffer));


  vkCmdEndRenderPass (command_buffer);
}
bool present ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_swapchain));


  // TODO: ask Andy for graphics starter code

  return true;
}


#pragma region vulkan_single_time_commands
static VkCommandPool s_single_time_command_pool = VK_NULL_HANDLE;
static VkCommandBuffer s_single_time_command_buffer = VK_NULL_HANDLE;
static VkQueue s_single_time_queue = VK_NULL_HANDLE;


bool begin_single_time_commands (VkCommandBuffer& out_command_buffer)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT_MSG (!CHECK_VULKAN_HANDLE (s_single_time_command_pool),
    "must call 'end_single_time_commands' before calling 'begin_single_time_commands' again!\n");
  DBG_ASSERT (!CHECK_VULKAN_HANDLE (out_command_buffer));


  if (!create_vulkan_command_pool (VK_QUEUE_COMPUTE_BIT, s_single_time_command_pool))
  {
    s_single_time_command_pool = VK_NULL_HANDLE;
    return false;
  }

  if (!create_vulkan_command_buffers (1u, s_single_time_command_pool, &s_single_time_command_buffer))
  {
    release_vulkan_command_pool (s_single_time_command_pool);
    s_single_time_command_buffer = VK_NULL_HANDLE;
    return false;
  }

  if (!get_vulkan_queue_compute (s_single_time_queue))
  {
    release_vulkan_command_buffers (1u, s_single_time_command_pool, &s_single_time_command_buffer);
    release_vulkan_command_pool (s_single_time_command_pool);
    return false;
  }

  out_command_buffer = s_single_time_command_buffer;

  return true;
}
bool end_single_time_commands ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT_MSG (CHECK_VULKAN_HANDLE (s_single_time_command_pool),
    "must call 'begin_single_time_commands' before calling 'end_single_time_commands'!\n");
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_single_time_command_buffer));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_single_time_queue));


  // Create fence to ensure that the command buffer has finished executing
  VkFence fence = VK_NULL_HANDLE;
  if (!create_vulkan_fences (1u, 0u, &fence))
  {
    return false;
  }

  VkSubmitInfo const submit_info =
  {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
    //.pNext = VK_NULL_HANDLE,
    //.waitSemaphoreCount = {},
    //.pWaitSemaphores = VK_NULL_HANDLE,
    //.pWaitDstStageMask = VK_NULL_HANDLE,
    .commandBufferCount = 1u,
    .pCommandBuffers = &s_single_time_command_buffer,
    //.signalSemaphoreCount = {},
    //.pSignalSemaphores = VK_NULL_HANDLE
  };
  VkResult result = vkQueueSubmit (s_single_time_queue, 1u, &submit_info, fence);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT_MSG (false, "submit failed\n");
  }

  // Wait for the fence to signal that command buffer has finished executing
  result = vkWaitForFences (s_device,
    1u,
    &fence,
    VK_TRUE,
    UINT64_MAX);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }
  result = vkQueueWaitIdle (s_single_time_queue);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }
  result = vkDeviceWaitIdle (s_device);
  if (!CHECK_VULKAN_RESULT (result))
  {
    return DBG_ASSERT (false);
  }

  release_vulkan_fences (1u, &fence);
  s_single_time_queue = VK_NULL_HANDLE;
  release_vulkan_command_buffers (1u, s_single_time_command_pool, &s_single_time_command_buffer);
  release_vulkan_command_pool (s_single_time_command_pool);

  return true;
}
#pragma endregion


#pragma region vulkan_release
void release_vulkan_fences (u32 num, VkFence* fences)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (num > 0u);
  DBG_ASSERT (fences != nullptr);


  for (u32 i = 0u; i < num; ++i)
  {
    vkDestroyFence (s_device, fences [i], VK_NULL_HANDLE);
    fences [i] = VK_NULL_HANDLE;
  }
}
void release_vulkan_semaphores (u32 num, VkSemaphore* semaphores)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (num > 0u);
  DBG_ASSERT (semaphores != nullptr);


  for (u32 i = 0u; i < num; ++i)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (semaphores [i]));
    vkDestroySemaphore (s_device, semaphores [i], VK_NULL_HANDLE);
    semaphores [i] = VK_NULL_HANDLE;
  }
}

void release_vulkan_command_buffers (u32 num, VkCommandPool command_pool, VkCommandBuffer* command_buffers)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_pool));
  DBG_ASSERT (command_buffers != nullptr);


  for (u32 i = 0u; i < num; ++i)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (command_buffers [i]));
    vkFreeCommandBuffers (s_device, command_pool, 1u, &command_buffers [i]);
    command_buffers [i] = VK_NULL_HANDLE;
  }
}
void release_vulkan_command_pool (VkCommandPool& command_pool)
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (command_pool));


  vkDestroyCommandPool (s_device, command_pool, VK_NULL_HANDLE);
  command_pool = VK_NULL_HANDLE;
}

void release_vulkan_swapchain ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_swapchain));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_render_pass));


  for (VkFramebuffer& framebuffer : s_framebuffers)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (framebuffer));
    vkDestroyFramebuffer (s_device, framebuffer, VK_NULL_HANDLE);
  }
  s_framebuffers.fill (VK_NULL_HANDLE);

  vkDestroyRenderPass (s_device, s_render_pass, VK_NULL_HANDLE);
  s_render_pass = VK_NULL_HANDLE;

  for (VkImageView& image_view : s_framebuffer_image_views)
  {
    DBG_ASSERT (CHECK_VULKAN_HANDLE (image_view));
    vkDestroyImageView (s_device, image_view, VK_NULL_HANDLE);
  }
  s_framebuffer_image_views.fill (VK_NULL_HANDLE);

  s_framebuffer_images.fill (VK_NULL_HANDLE);
  vkDestroySwapchainKHR (s_device, s_swapchain, VK_NULL_HANDLE);
  s_swapchain = VK_NULL_HANDLE;
}
void release_vulkan_device ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_device));


  vkDeviceWaitIdle (s_device);

  vkDestroyDevice (s_device, VK_NULL_HANDLE);
  s_device = VK_NULL_HANDLE;
}
void release_vulkan_surface ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_surface));


  vkDestroySurfaceKHR (s_instance, s_surface, VK_NULL_HANDLE);
  s_surface = VK_NULL_HANDLE;
}
#ifdef ENABLE_VULKAN_DEBUG
static void destroy_debug_report_callback ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_debug_report_callback));


  auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr (s_instance, "vkDestroyDebugReportCallbackEXT");
  if (func)
  {
    func (s_instance, s_debug_report_callback, VK_NULL_HANDLE);
  }
  s_debug_report_callback = VK_NULL_HANDLE;
}
static void destroy_debug_messenger ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_debug_messenger));


  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr (s_instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func)
  {
    func (s_instance, s_debug_messenger, VK_NULL_HANDLE);
  }
  s_debug_messenger = VK_NULL_HANDLE;
}
#endif // ENABLE_VULKAN_DEBUG
void release_vulkan_instance ()
{
  DBG_ASSERT (CHECK_VULKAN_HANDLE (s_instance));


#ifdef ENABLE_VULKAN_DEBUG
  destroy_debug_messenger ();
  destroy_debug_report_callback ();
#endif // ENABLE_VULKAN_DEBUG
  vkDestroyInstance (s_instance, VK_NULL_HANDLE);
  s_instance = VK_NULL_HANDLE;
}
void release_window ()
{
  DBG_ASSERT (s_window_handle != NULL);


  glfwDestroyWindow (s_window_handle);
  glfwTerminate ();
  s_window_handle = NULL;
}
#pragma endregion
