/*
 * Copyright (c) 2015-2023 The Khronos Group Inc.
 * Copyright (c) 2015-2023 Valve Corporation
 * Copyright (c) 2015-2023 LunarG, Inc.
 * Copyright (c) 2015-2023 Google, Inc.
 * Modifications Copyright (C) 2020 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#include "utils/cast_utils.h"
#include "enum_flag_bits.h"
#include "../framework/layer_validation_tests.h"
#include "utils/vk_layer_utils.h"

TEST_F(VkLayerTest, CreateBufferIncompatibleExternalHandleTypes) {
    TEST_DESCRIPTION("Creating buffer with incompatible external memory handle types");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    // Try all flags first. It's unlikely all of them are compatible.
    auto external_memory_info = LvlInitStruct<VkExternalMemoryBufferCreateInfo>();
    external_memory_info.handleTypes = AllVkExternalMemoryHandleTypeFlagBits;
    auto buffer_create_info = LvlInitStruct<VkBufferCreateInfo>(&external_memory_info);
    buffer_create_info.size = 1024;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    CreateBufferTest(*this, &buffer_create_info, "VUID-VkBufferCreateInfo-pNext-00920");

    // Get all exportable handle types supported by the platform.
    VkExternalMemoryHandleTypeFlags supported_handle_types = 0;
    VkExternalMemoryHandleTypeFlags any_compatible_group = 0;
    IterateFlags<VkExternalMemoryHandleTypeFlagBits>(
        AllVkExternalMemoryHandleTypeFlagBits, [&](VkExternalMemoryHandleTypeFlagBits flag) {
            auto external_buffer_info = LvlInitStruct<VkPhysicalDeviceExternalBufferInfo>();
            external_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            external_buffer_info.handleType = flag;
            auto external_buffer_properties = LvlInitStruct<VkExternalBufferProperties>();
            vk::GetPhysicalDeviceExternalBufferProperties(gpu(), &external_buffer_info, &external_buffer_properties);
            const auto external_features = external_buffer_properties.externalMemoryProperties.externalMemoryFeatures;
            if (external_features & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT) {
                supported_handle_types |= external_buffer_info.handleType;
                any_compatible_group = external_buffer_properties.externalMemoryProperties.compatibleHandleTypes;
            }
        });

    // Main test case. Handle types are supported but not compatible with each other
    if ((supported_handle_types & any_compatible_group) != supported_handle_types) {
        external_memory_info.handleTypes = supported_handle_types;
        CreateBufferTest(*this, &buffer_create_info, "VUID-VkBufferCreateInfo-pNext-00920");
    }
}

TEST_F(VkLayerTest, CreateImageIncompatibleExternalHandleTypes) {
    TEST_DESCRIPTION("Creating image with incompatible external memory handle types");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    // Try all flags first. It's unlikely all of them are compatible.
    auto external_memory_info = LvlInitStruct<VkExternalMemoryImageCreateInfo>();
    external_memory_info.handleTypes = AllVkExternalMemoryHandleTypeFlagBits;
    auto image_create_info = LvlInitStruct<VkImageCreateInfo>(&external_memory_info);
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CreateImageTest(*this, &image_create_info, "VUID-VkImageCreateInfo-pNext-00990");

    // Get all exportable handle types supported by the platform.
    VkExternalMemoryHandleTypeFlags supported_handle_types = 0;
    VkExternalMemoryHandleTypeFlags any_compatible_group = 0;

    auto external_image_info = LvlInitStruct<VkPhysicalDeviceExternalImageFormatInfo>();
    auto image_info = LvlInitStruct<VkPhysicalDeviceImageFormatInfo2>(&external_image_info);
    image_info.format = image_create_info.format;
    image_info.type = image_create_info.imageType;
    image_info.tiling = image_create_info.tiling;
    image_info.usage = image_create_info.usage;
    image_info.flags = image_create_info.flags;

    IterateFlags<VkExternalMemoryHandleTypeFlagBits>(
        AllVkExternalMemoryHandleTypeFlagBits, [&](VkExternalMemoryHandleTypeFlagBits flag) {
            external_image_info.handleType = flag;
            auto external_image_properties = LvlInitStruct<VkExternalImageFormatProperties>();
            auto image_properties = LvlInitStruct<VkImageFormatProperties2>(&external_image_properties);
            VkResult result = vk::GetPhysicalDeviceImageFormatProperties2(gpu(), &image_info, &image_properties);
            const auto external_features = external_image_properties.externalMemoryProperties.externalMemoryFeatures;
            if (result == VK_SUCCESS && (external_features & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT)) {
                supported_handle_types |= external_image_info.handleType;
                any_compatible_group = external_image_properties.externalMemoryProperties.compatibleHandleTypes;
            }
        });

    // Main test case. Handle types are supported but not compatible with each other
    if ((supported_handle_types & any_compatible_group) != supported_handle_types) {
        external_memory_info.handleTypes = supported_handle_types;
        CreateImageTest(*this, &image_create_info, "VUID-VkImageCreateInfo-pNext-00990");
    }
}

TEST_F(VkLayerTest, CreateImageIncompatibleExternalHandleTypesNV) {
    TEST_DESCRIPTION("Creating image with incompatible external memory handle types from NVIDIA extension");

    AddRequiredExtensions(VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    auto external_memory_info = LvlInitStruct<VkExternalMemoryImageCreateInfoNV>();
    auto image_create_info = LvlInitStruct<VkImageCreateInfo>(&external_memory_info);
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_create_info.extent.width = 32;
    image_create_info.extent.height = 32;
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // Get all exportable handle types supported by the platform.
    VkExternalMemoryHandleTypeFlagsNV supported_handle_types = 0;
    VkExternalMemoryHandleTypeFlagsNV any_compatible_group = 0;

    auto vkGetPhysicalDeviceExternalImageFormatPropertiesNV =
        GetInstanceProcAddr<PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV>(
            "vkGetPhysicalDeviceExternalImageFormatPropertiesNV");

    IterateFlags<VkExternalMemoryHandleTypeFlagBitsNV>(
        AllVkExternalMemoryHandleTypeFlagBitsNV, [&](VkExternalMemoryHandleTypeFlagBitsNV flag) {
            VkExternalImageFormatPropertiesNV external_image_properties = {};
            VkResult result = vkGetPhysicalDeviceExternalImageFormatPropertiesNV(
                gpu(), image_create_info.format, image_create_info.imageType, image_create_info.tiling, image_create_info.usage,
                image_create_info.flags, flag, &external_image_properties);
            if (result == VK_SUCCESS &&
                (external_image_properties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV)) {
                supported_handle_types |= flag;
                any_compatible_group = external_image_properties.compatibleHandleTypes;
            }
        });

    // Main test case. Handle types are supported but not compatible with each other
    if ((supported_handle_types & any_compatible_group) != supported_handle_types) {
        external_memory_info.handleTypes = supported_handle_types;
        CreateImageTest(*this, &image_create_info, "VUID-VkImageCreateInfo-pNext-00991");
    }
}

TEST_F(VkLayerTest, InvalidExportExternalImageHandleType) {
    TEST_DESCRIPTION("Test exporting memory with mismatching handleTypes.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create export image
    auto external_image_info = LvlInitStruct<VkExternalMemoryImageCreateInfo>();
    auto image_info = LvlInitStruct<VkImageCreateInfo>(&external_image_info);
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.arrayLayers = 1;
    image_info.extent = {64, 64, 1};
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.mipLevels = 1;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    auto exportable_types = FindSupportedExternalMemoryHandleTypes(gpu(), image_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (GetBitSetCount(exportable_types) < 2) {
        GTEST_SKIP() << "Cannot find two distinct exportable handle types, skipping test";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    exportable_types &= ~handle_type;
    const auto handle_type2 = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    assert(handle_type != handle_type2);

    // Create an image with one of the handle types
    external_image_info.handleTypes = handle_type;
    vk_testing::Image image(*m_device, image_info, vk_testing::NoMemT{});

    // Create export memory with a different handle type
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = handle_type2;
    VkMemoryRequirements image_mem_reqs;
    vk::GetImageMemoryRequirements(device(), image.handle(), &image_mem_reqs);
    const auto alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(
        *m_device, image_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    const auto memory = vk_testing::DeviceMemory(*m_device, alloc_info);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkBindImageMemory-memory-02728");
    vk::BindImageMemory(device(), image.handle(), memory.handle(), 0);
    m_errorMonitor->VerifyFound();

    auto bind_image_info = LvlInitStruct<VkBindImageMemoryInfo>();
    bind_image_info.image = image.handle();
    bind_image_info.memory = memory.handle();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkBindImageMemoryInfo-memory-02728");
    vk::BindImageMemory2(device(), 1, &bind_image_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, BufferMemoryWithUnsupportedExternalHandleType) {
    TEST_DESCRIPTION("Bind buffer memory with unsupported external memory handle type.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    auto external_buffer_info = LvlInitStruct<VkExternalMemoryBufferCreateInfo>();
    const auto buffer_info =
        vk_testing::Buffer::create_info(4096, VK_BUFFER_USAGE_TRANSFER_DST_BIT, nullptr, &external_buffer_info);
    const auto exportable_types =
        FindSupportedExternalMemoryHandleTypes(gpu(), buffer_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    if (exportable_types == AllVkExternalMemoryHandleTypeFlagBits) {
        GTEST_SKIP() << "This test requires at least one unsupported handle type, but all handle types are supported";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    external_buffer_info.handleTypes = handle_type;

    // Create memory object with unsupported handle type
    const auto not_supported_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(~exportable_types);
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = handle_type | not_supported_type;

    vk_testing::Buffer buffer;
    buffer.init_no_mem(*m_device, buffer_info);
    auto alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, buffer.memory_requirements(),
                                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    VkResult result = buffer.memory().try_init(*m_device, alloc_info);
    if (result != VK_SUCCESS) {
        GTEST_SKIP() << "vkAllocateMemory failed (probably due to unsupported handle type). Unable to reach vkBindBufferMemory to "
                        "run valdiation";
    }

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportMemoryAllocateInfo-handleTypes-00656");
    buffer.bind_memory(buffer.memory(), 0);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, BufferMemoryWithIncompatibleExternalHandleTypes) {
    TEST_DESCRIPTION("Bind buffer memory with incompatible external memory handle types.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    auto external_buffer_info = LvlInitStruct<VkExternalMemoryBufferCreateInfo>();
    const auto buffer_info =
        vk_testing::Buffer::create_info(4096, VK_BUFFER_USAGE_TRANSFER_DST_BIT, nullptr, &external_buffer_info);
    const auto exportable_types =
        FindSupportedExternalMemoryHandleTypes(gpu(), buffer_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    const auto compatible_types = GetCompatibleHandleTypes(gpu(), buffer_info, handle_type);
    if ((exportable_types & compatible_types) == exportable_types) {
        GTEST_SKIP() << "Cannot find handle types that are supported but not compatible with each other";
    }
    external_buffer_info.handleTypes = handle_type;

    // Create memory object with incompatible handle types
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = exportable_types;

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportMemoryAllocateInfo-handleTypes-00656");
    vk_testing::Buffer buffer(*m_device, buffer_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ImageMemoryWithUnsupportedExternalHandleType) {
    TEST_DESCRIPTION("Bind image memory with unsupported external memory handle type.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    auto external_image_info = LvlInitStruct<VkExternalMemoryImageCreateInfo>();
    auto image_info = LvlInitStruct<VkImageCreateInfo>(&external_image_info);
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.arrayLayers = 1;
    image_info.extent = {64, 64, 1};
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.mipLevels = 1;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    auto exportable_types = FindSupportedExternalMemoryHandleTypes(gpu(), image_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    // This test does not support the AHB handle type, which does not
    // allow to query memory requirements before memory is bound
    exportable_types &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    if (exportable_types == AllVkExternalMemoryHandleTypeFlagBits) {
        GTEST_SKIP() << "This test requires at least one unsupported handle type, but all handle types are supported";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    external_image_info.handleTypes = handle_type;

    // Create memory object with unsupported handle type
    const auto not_supported_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(~exportable_types);
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = handle_type | not_supported_type;

    vk_testing::Image image;
    image.init_no_mem(*m_device, image_info);
    auto alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, image.memory_requirements(),
                                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    VkResult result = image.memory().try_init(*m_device, alloc_info);
    if (result != VK_SUCCESS) {
        GTEST_SKIP() << "vkAllocateMemory failed (probably due to unsupported handle type). Unable to reach vkBindImageMemory to "
                        "run valdiation";
    }

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportMemoryAllocateInfo-handleTypes-00656");
    image.bind_memory(image.memory(), 0);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ImageMemoryWithIncompatibleExternalHandleTypes) {
    TEST_DESCRIPTION("Bind image memory with incompatible external memory handle types.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create export image
    auto external_image_info = LvlInitStruct<VkExternalMemoryImageCreateInfo>();
    auto image_info = LvlInitStruct<VkImageCreateInfo>(&external_image_info);
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.arrayLayers = 1;
    image_info.extent = {64, 64, 1};
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.mipLevels = 1;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    auto exportable_types = FindSupportedExternalMemoryHandleTypes(gpu(), image_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    // This test does not support the AHB handle type, which does not
    // allow to query memory requirements before memory is bound
    exportable_types &= ~VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    const auto compatible_types = GetCompatibleHandleTypes(gpu(), image_info, handle_type);
    if ((exportable_types & compatible_types) == exportable_types) {
        GTEST_SKIP() << "Cannot find handle types that are supported but not compatible with each other";
    }
    external_image_info.handleTypes = handle_type;

    // Create memory object with incompatible handle types
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = exportable_types;

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportMemoryAllocateInfo-handleTypes-00656");
    vk_testing::Image image(*m_device, image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, InvalidExportExternalBufferHandleType) {
    TEST_DESCRIPTION("Test exporting memory with mismatching handleTypes.");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    // Create export buffer
    auto external_info = LvlInitStruct<VkExternalMemoryBufferCreateInfo>();
    auto buffer_info = LvlInitStruct<VkBufferCreateInfo>(&external_info);
    buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    buffer_info.size = 4096;

    auto exportable_types = FindSupportedExternalMemoryHandleTypes(gpu(), buffer_info, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (GetBitSetCount(exportable_types) < 2) {
        GTEST_SKIP() << "Cannot find two distinct exportable handle types, skipping test";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    exportable_types &= ~handle_type;
    const auto handle_type2 = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(exportable_types);
    assert(handle_type != handle_type2);

    // Create a buffer with one of the handle types
    external_info.handleTypes = handle_type;
    vk_testing::Buffer buffer(*m_device, buffer_info, vk_testing::NoMemT{});

    // Create export memory with a different handle type
    auto export_memory_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
    export_memory_info.handleTypes = handle_type2;
    VkMemoryRequirements buffer_mem_reqs;
    vk::GetBufferMemoryRequirements(device(), buffer.handle(), &buffer_mem_reqs);
    const auto alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(
        *m_device, buffer_mem_reqs, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &export_memory_info);
    const auto memory = vk_testing::DeviceMemory(*m_device, alloc_info);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkBindBufferMemory-memory-02726");
    vk::BindBufferMemory(device(), buffer.handle(), memory.handle(), 0);
    m_errorMonitor->VerifyFound();

    auto bind_buffer_info = LvlInitStruct<VkBindBufferMemoryInfo>();
    bind_buffer_info.buffer = buffer.handle();
    bind_buffer_info.memory = memory.handle();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkBindBufferMemoryInfo-memory-02726");
    vk::BindBufferMemory2(device(), 1, &bind_buffer_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ExternalTimelineSemaphore) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    const char *no_tempory_tl_vuid = "VUID-VkImportSemaphoreWin32HandleInfoKHR-flags-03322";
#else
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    const char *no_tempory_tl_vuid = "VUID-VkImportSemaphoreFdInfoKHR-flags-03323";
#endif
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(extension_name);
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    auto timeline_semaphore_features = LvlInitStruct<VkPhysicalDeviceTimelineSemaphoreFeatures>();
    GetPhysicalDeviceFeatures2(timeline_semaphore_features);
    if (!timeline_semaphore_features.timelineSemaphore) {
        GTEST_SKIP() << "timelineSemaphore not supported";
    }
    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &timeline_semaphore_features));

    // Check for external semaphore import and export capability
    {
        auto sti = LvlInitStruct<VkSemaphoreTypeCreateInfo>();
        sti.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        auto esi = LvlInitStruct<VkPhysicalDeviceExternalSemaphoreInfoKHR>(&sti);
        esi.handleType = handle_type;

        auto esp = LvlInitStruct<VkExternalSemaphorePropertiesKHR>();

        auto vkGetPhysicalDeviceExternalSemaphorePropertiesKHR =
            (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)vk::GetInstanceProcAddr(
                instance(), "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
        vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(gpu(), &esi, &esp);

        if (!(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) ||
            !(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR)) {
            GTEST_SKIP() << "External semaphore does not support importing and exporting, skipping test";
        }
    }

    VkResult err;

    // Create a semaphore to export payload from
    auto esci = LvlInitStruct<VkExportSemaphoreCreateInfoKHR>();
    esci.handleTypes = handle_type;
    auto stci = LvlInitStruct<VkSemaphoreTypeCreateInfoKHR>(&esci);
    stci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
    auto sci = LvlInitStruct<VkSemaphoreCreateInfo>(&stci);

    vk_testing::Semaphore export_semaphore(*m_device, sci);

    // Create a semaphore to import payload into
    stci.pNext = nullptr;
    vk_testing::Semaphore import_semaphore(*m_device, sci);

    vk_testing::Semaphore::ExternalHandle ext_handle{};
    err = export_semaphore.export_handle(ext_handle, handle_type);
    ASSERT_VK_SUCCESS(err);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, no_tempory_tl_vuid);
    err = import_semaphore.import_handle(ext_handle, handle_type, VK_SEMAPHORE_IMPORT_TEMPORARY_BIT_KHR);
    m_errorMonitor->VerifyFound();

    err = import_semaphore.import_handle(ext_handle, handle_type);
    ASSERT_VK_SUCCESS(err);
}

TEST_F(VkLayerTest, ExternalSyncFdSemaphore) {
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;

    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    auto timeline_semaphore_features = LvlInitStruct<VkPhysicalDeviceTimelineSemaphoreFeatures>();
    GetPhysicalDeviceFeatures2(timeline_semaphore_features);
    if (!timeline_semaphore_features.timelineSemaphore) {
        GTEST_SKIP() << "timelineSemaphore not supported";
    }
    ASSERT_NO_FATAL_FAILURE(InitState(nullptr, &timeline_semaphore_features, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT));

    // Check for external semaphore import and export capability
    auto esi = LvlInitStruct<VkPhysicalDeviceExternalSemaphoreInfoKHR>();
    esi.handleType = handle_type;

    auto esp = LvlInitStruct<VkExternalSemaphorePropertiesKHR>();

    auto vkGetPhysicalDeviceExternalSemaphorePropertiesKHR =
        (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)vk::GetInstanceProcAddr(
            instance(), "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
    vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(gpu(), &esi, &esp);

    if (!(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External semaphore does not support importing and exporting";
    }

    if (!(esp.compatibleHandleTypes & handle_type)) {
        GTEST_SKIP() << "External semaphore does not support VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT";
    }

    VkResult err;

    // create a timeline semaphore.
    // Note that adding a sync fd VkExportSemaphoreCreateInfo will cause creation to fail.
    auto stci = LvlInitStruct<VkSemaphoreTypeCreateInfoKHR>();
    stci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    auto sci = LvlInitStruct<VkSemaphoreCreateInfo>(&stci);

    vk_testing::Semaphore timeline_sem(*m_device, sci);

    // binary semaphore works fine.
    auto esci = LvlInitStruct<VkExportSemaphoreCreateInfo>();
    esci.handleTypes = handle_type;
    stci.pNext = &esci;

    stci.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
    vk_testing::Semaphore binary_sem(*m_device, sci);

    // Create a semaphore to import payload into
    vk_testing::Semaphore import_semaphore(*m_device);

    vk_testing::Semaphore::ExternalHandle ext_handle{};

    // timeline not allowed
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkSemaphoreGetFdInfoKHR-handleType-01132");
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkSemaphoreGetFdInfoKHR-handleType-03253");
    timeline_sem.export_handle(ext_handle, handle_type);
    m_errorMonitor->VerifyFound();

    // must have pending signal
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkSemaphoreGetFdInfoKHR-handleType-03254");
    binary_sem.export_handle(ext_handle, handle_type);
    m_errorMonitor->VerifyFound();

    auto si = LvlInitStruct<VkSubmitInfo>();
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores = &binary_sem.handle();

    err = vk::QueueSubmit(m_device->m_queue, 1, &si, VK_NULL_HANDLE);
    ASSERT_VK_SUCCESS(err);

    err = binary_sem.export_handle(ext_handle, handle_type);
    ASSERT_VK_SUCCESS(err);

    // must be temporary
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImportSemaphoreFdInfoKHR-handleType-07307");
    import_semaphore.import_handle(ext_handle, handle_type);
    m_errorMonitor->VerifyFound();

    err = import_semaphore.import_handle(ext_handle, handle_type, VK_SEMAPHORE_IMPORT_TEMPORARY_BIT);
    ASSERT_VK_SUCCESS(err);

    err = vk::QueueWaitIdle(m_device->m_queue);
    ASSERT_VK_SUCCESS(err);
}

TEST_F(VkLayerTest, TemporaryExternalFence) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    const auto extension_name = VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
    const auto extension_name = VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
    AddRequiredExtensions(extension_name);
    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));
    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    // Check for external fence import and export capability
    auto efi = LvlInitStruct<VkPhysicalDeviceExternalFenceInfoKHR>();
    efi.handleType = handle_type;
    auto efp = LvlInitStruct<VkExternalFencePropertiesKHR>();
    auto vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)vk::GetInstanceProcAddr(
        instance(), "vkGetPhysicalDeviceExternalFencePropertiesKHR");
    vkGetPhysicalDeviceExternalFencePropertiesKHR(gpu(), &efi, &efp);

    if (!(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External fence does not support importing and exporting, skipping test.";
    }

    VkResult err;

    // Create a fence to export payload from
    auto efci = LvlInitStruct<VkExportFenceCreateInfoKHR>();
    efci.handleTypes = handle_type;
    auto fci = LvlInitStruct<VkFenceCreateInfo>(&efci);
    vk_testing::Fence export_fence(*m_device, fci);

    // Create a fence to import payload into
    fci.pNext = nullptr;
    vk_testing::Fence import_fence(*m_device, fci);

    // Export fence payload to an opaque handle
    vk_testing::Fence::ExternalHandle ext_fence{};
    err = export_fence.export_handle(ext_fence, handle_type);
    ASSERT_VK_SUCCESS(err);
    err = import_fence.import_handle(ext_fence, handle_type, VK_FENCE_IMPORT_TEMPORARY_BIT_KHR);
    ASSERT_VK_SUCCESS(err);

    // Undo the temporary import
    vk::ResetFences(m_device->device(), 1, &import_fence.handle());

    // Signal the previously imported fence twice, the second signal should produce a validation error
    vk::QueueSubmit(m_device->m_queue, 0, nullptr, import_fence.handle());
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkQueueSubmit-fence-00064");
    vk::QueueSubmit(m_device->m_queue, 0, nullptr, import_fence.handle());
    m_errorMonitor->VerifyFound();

    err = vk::QueueWaitIdle(m_device->m_queue);
    ASSERT_VK_SUCCESS(err);

    // Signal without reseting
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkQueueSubmit-fence-00063");
    vk::QueueSubmit(m_device->m_queue, 0, nullptr, import_fence.handle());
    m_errorMonitor->VerifyFound();
    err = vk::QueueWaitIdle(m_device->m_queue);
    ASSERT_VK_SUCCESS(err);
}

TEST_F(VkLayerTest, InvalidExternalFence) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    const auto extension_name = VK_KHR_EXTERNAL_FENCE_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    const auto other_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT;
    const auto bad_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    const char *bad_export_type_vuid = "VUID-VkFenceGetWin32HandleInfoKHR-handleType-01452";
    const char *other_export_type_vuid = "VUID-VkFenceGetWin32HandleInfoKHR-handleType-01448";
    const char *bad_import_type_vuid = "VUID-VkImportFenceWin32HandleInfoKHR-handleType-01457";
#else
    const auto extension_name = VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    const auto other_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT_KHR;
    const auto bad_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    const char *bad_export_type_vuid = "VUID-VkFenceGetFdInfoKHR-handleType-01456";
    const char *other_export_type_vuid = "VUID-VkFenceGetFdInfoKHR-handleType-01453";
    const char *bad_import_type_vuid = "VUID-VkImportFenceFdInfoKHR-handleType-01464";
#endif
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
    AddRequiredExtensions(extension_name);
    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));
    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    // Check for external fence import and export capability
    auto efi = LvlInitStruct<VkPhysicalDeviceExternalFenceInfoKHR>();
    efi.handleType = handle_type;
    auto efp = LvlInitStruct<VkExternalFencePropertiesKHR>();
    auto vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)vk::GetInstanceProcAddr(
        instance(), "vkGetPhysicalDeviceExternalFencePropertiesKHR");
    vkGetPhysicalDeviceExternalFencePropertiesKHR(gpu(), &efi, &efp);

    if (!(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External fence does not support importing and exporting, skipping test.";
    }

    // Create a fence to export payload from
    auto efci = LvlInitStruct<VkExportFenceCreateInfoKHR>();
    efci.handleTypes = handle_type;
    auto fci = LvlInitStruct<VkFenceCreateInfo>(&efci);
    vk_testing::Fence export_fence(*m_device, fci);

    // Create a fence to import payload into
    fci.pNext = nullptr;
    vk_testing::Fence import_fence(*m_device, fci);

    vk_testing::Fence::ExternalHandle ext_handle{};

    // windows vs unix mismatch
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, bad_export_type_vuid);
    export_fence.export_handle(ext_handle, bad_type);
    m_errorMonitor->VerifyFound();

    // a valid type for the platform which we didn't ask for during create
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, other_export_type_vuid);
    if (other_type == VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT_KHR) {
        // SYNC_FD is a special snowflake
        m_errorMonitor->SetAllowedFailureMsg("VUID-VkFenceGetFdInfoKHR-handleType-01454");
    }
    export_fence.export_handle(ext_handle, other_type);
    m_errorMonitor->VerifyFound();

    export_fence.export_handle(ext_handle, handle_type);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, bad_import_type_vuid);
    import_fence.import_handle(ext_handle, bad_type);
    m_errorMonitor->VerifyFound();
#ifdef VK_USE_PLATFORM_WIN32_KHR
    auto ifi = LvlInitStruct<VkImportFenceWin32HandleInfoKHR>();
    ifi.fence = import_fence.handle();
    ifi.handleType = handle_type;
    ifi.handle = ext_handle;
    ifi.flags = 0;
    ifi.name = L"something";

    auto vkImportFenceWin32HandleKHR =
        reinterpret_cast<PFN_vkImportFenceWin32HandleKHR>(vk::GetDeviceProcAddr(device(), "vkImportFenceWin32HandleKHR"));

    // If handleType is not VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT, name must be NULL
    // However, it looks like at least some windows drivers don't support exporting KMT handles for fences
    if (handle_type != VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_WIN32_BIT) {
        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImportFenceWin32HandleInfoKHR-handleType-01459");
    }
    // If handle is not NULL, name must be NULL
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImportFenceWin32HandleInfoKHR-handle-01462");
    vkImportFenceWin32HandleKHR(m_device->device(), &ifi);
    m_errorMonitor->VerifyFound();
#endif
    auto err = import_fence.import_handle(ext_handle, handle_type);
    ASSERT_VK_SUCCESS(err);
}

TEST_F(VkLayerTest, ExternalSyncFdFence) {
    const auto handle_type = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT_KHR;

    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));
    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    // Check for external fence import and export capability
    auto efi = LvlInitStruct<VkPhysicalDeviceExternalFenceInfoKHR>();
    efi.handleType = handle_type;
    auto efp = LvlInitStruct<VkExternalFencePropertiesKHR>();
    auto vkGetPhysicalDeviceExternalFencePropertiesKHR = (PFN_vkGetPhysicalDeviceExternalFencePropertiesKHR)vk::GetInstanceProcAddr(
        instance(), "vkGetPhysicalDeviceExternalFencePropertiesKHR");
    vkGetPhysicalDeviceExternalFencePropertiesKHR(gpu(), &efi, &efp);

    if (!(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(efp.externalFenceFeatures & VK_EXTERNAL_FENCE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External fence does not support importing and exporting, skipping test.";
    }

    // Create a fence to export payload from
    auto efci = LvlInitStruct<VkExportFenceCreateInfoKHR>();
    efci.handleTypes = handle_type;
    auto fci = LvlInitStruct<VkFenceCreateInfo>(&efci);
    vk_testing::Fence export_fence(*m_device, fci);

    // Create a fence to import payload into
    fci.pNext = nullptr;
    vk_testing::Fence import_fence(*m_device, fci);

    vk_testing::Fence::ExternalHandle ext_handle{};

    // SYNC_FD must have a pending signal for export
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkFenceGetFdInfoKHR-handleType-01454");
    export_fence.export_handle(ext_handle, handle_type);
    m_errorMonitor->VerifyFound();

    vk::QueueSubmit(m_device->m_queue, 0, nullptr, export_fence.handle());

    export_fence.export_handle(ext_handle, handle_type);

    // must be temporary
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImportFenceFdInfoKHR-handleType-07306");
    import_fence.import_handle(ext_handle, handle_type);
    m_errorMonitor->VerifyFound();

    import_fence.import_handle(ext_handle, handle_type, VK_FENCE_IMPORT_TEMPORARY_BIT);

    import_fence.wait(1000000000);
}

TEST_F(VkLayerTest, TemporaryExternalSemaphore) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
#else
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(extension_name);
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    // Check for external semaphore import and export capability
    auto esi = LvlInitStruct<VkPhysicalDeviceExternalSemaphoreInfoKHR>();
    esi.handleType = handle_type;

    auto esp = LvlInitStruct<VkExternalSemaphorePropertiesKHR>();

    auto vkGetPhysicalDeviceExternalSemaphorePropertiesKHR =
        (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)vk::GetInstanceProcAddr(
            instance(), "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
    vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(gpu(), &esi, &esp);

    if (!(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External semaphore does not support importing and exporting, skipping test";
    }

    VkResult err;

    // Create a semaphore to export payload from
    auto esci = LvlInitStruct<VkExportSemaphoreCreateInfoKHR>();
    esci.handleTypes = handle_type;
    auto sci = LvlInitStruct<VkSemaphoreCreateInfo>(&esci);

    vk_testing::Semaphore export_semaphore(*m_device, sci);

    // Create a semaphore to import payload into
    sci.pNext = nullptr;
    vk_testing::Semaphore import_semaphore(*m_device, sci);

    vk_testing::Semaphore::ExternalHandle ext_handle{};
    err = export_semaphore.export_handle(ext_handle, handle_type);
    ASSERT_VK_SUCCESS(err);
    err = import_semaphore.import_handle(ext_handle, handle_type, VK_SEMAPHORE_IMPORT_TEMPORARY_BIT_KHR);
    ASSERT_VK_SUCCESS(err);

    // Wait on the imported semaphore twice in vk::QueueSubmit, the second wait should be an error
    VkPipelineStageFlags flags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    std::vector<VkSubmitInfo> si(4, LvlInitStruct<VkSubmitInfo>());
    si[0].signalSemaphoreCount = 1;
    si[0].pSignalSemaphores = &export_semaphore.handle();

    si[1].waitSemaphoreCount = 1;
    si[1].pWaitSemaphores = &import_semaphore.handle();
    si[1].pWaitDstStageMask = &flags;

    si[2] = si[0];
    si[3] = si[1];

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-CoreValidation-DrawState-QueueForwardProgress");
    vk::QueueSubmit(m_device->m_queue, si.size(), si.data(), VK_NULL_HANDLE);
    m_errorMonitor->VerifyFound();

    auto index = m_device->graphics_queue_node_index_;
    if (m_device->queue_props[index].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
        // Wait on the imported semaphore twice in vk::QueueBindSparse, the second wait should be an error
        std::vector<VkBindSparseInfo> bi(4, LvlInitStruct<VkBindSparseInfo>());
        bi[0].signalSemaphoreCount = 1;
        bi[0].pSignalSemaphores = &export_semaphore.handle();

        bi[1].waitSemaphoreCount = 1;
        bi[1].pWaitSemaphores = &import_semaphore.handle();

        bi[2] = bi[0];
        bi[3] = bi[1];
        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "UNASSIGNED-CoreValidation-DrawState-QueueForwardProgress");
        vk::QueueBindSparse(m_device->m_queue, bi.size(), bi.data(), VK_NULL_HANDLE);
        m_errorMonitor->VerifyFound();
    }

    // Cleanup
    err = vk::QueueWaitIdle(m_device->m_queue);
    ASSERT_VK_SUCCESS(err);
}

TEST_F(VkLayerTest, InvalidExternalSemaphore) {
    TEST_DESCRIPTION("Import and export invalid external semaphores, no queue sumbits involved.");
#ifdef VK_USE_PLATFORM_WIN32_KHR
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    const auto bad_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    const auto other_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
    const char *bad_export_type_vuid = "VUID-VkSemaphoreGetWin32HandleInfoKHR-handleType-01131";
    const char *other_export_type_vuid = "VUID-VkSemaphoreGetWin32HandleInfoKHR-handleType-01126";
    const char *bad_import_type_vuid = "VUID-VkImportSemaphoreWin32HandleInfoKHR-handleType-01140";
#else
    const auto extension_name = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
    const auto bad_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT_KHR;
    const auto other_type = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR;
    const char *bad_export_type_vuid = "VUID-VkSemaphoreGetFdInfoKHR-handleType-01136";
    const char *other_export_type_vuid = "VUID-VkSemaphoreGetFdInfoKHR-handleType-01132";
    const char *bad_import_type_vuid = "VUID-VkImportSemaphoreFdInfoKHR-handleType-01143";
#endif
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(extension_name);
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    ASSERT_NO_FATAL_FAILURE(InitState());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    // Check for external semaphore import and export capability
    auto esi = LvlInitStruct<VkPhysicalDeviceExternalSemaphoreInfoKHR>();
    esi.handleType = handle_type;

    auto esp = LvlInitStruct<VkExternalSemaphorePropertiesKHR>();

    auto vkGetPhysicalDeviceExternalSemaphorePropertiesKHR =
        (PFN_vkGetPhysicalDeviceExternalSemaphorePropertiesKHR)vk::GetInstanceProcAddr(
            instance(), "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR");
    vkGetPhysicalDeviceExternalSemaphorePropertiesKHR(gpu(), &esi, &esp);

    if (!(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(esp.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External semaphore does not support importing and exporting, skipping test";
    }
    // Create a semaphore to export payload from
    auto esci = LvlInitStruct<VkExportSemaphoreCreateInfoKHR>();
    esci.handleTypes = handle_type;
    auto sci = LvlInitStruct<VkSemaphoreCreateInfo>(&esci);

    vk_testing::Semaphore export_semaphore(*m_device, sci);

    // Create a semaphore for importing
    vk_testing::Semaphore import_semaphore(*m_device);

    vk_testing::Semaphore::ExternalHandle ext_handle{};

    // windows vs unix mismatch
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, bad_export_type_vuid);
    export_semaphore.export_handle(ext_handle, bad_type);
    m_errorMonitor->VerifyFound();

    // not specified during create
    if (other_type == VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT_KHR) {
        // SYNC_FD must have pending signal
        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkSemaphoreGetFdInfoKHR-handleType-03254");
    }
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, other_export_type_vuid);
    export_semaphore.export_handle(ext_handle, other_type);
    m_errorMonitor->VerifyFound();

    export_semaphore.export_handle(ext_handle, handle_type);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, bad_import_type_vuid);
    export_semaphore.import_handle(ext_handle, bad_type);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ValidateImportMemoryHandleType) {
    TEST_DESCRIPTION("Validate import memory handleType for buffers and images");

#ifdef _WIN32
    const auto ext_mem_extension_name = VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR;
#else
    const auto ext_mem_extension_name = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME;
    const auto handle_type = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR;
#endif

    AddRequiredExtensions(ext_mem_extension_name);
    AddRequiredExtensions(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);

    AddOptionalExtensions(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

    ASSERT_NO_FATAL_FAILURE(InitFramework(m_errorMonitor));

    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    if (IsPlatform(kMockICD)) {
        GTEST_SKIP() << "External tests are not supported by MockICD, skipping tests";
    }

    auto vkGetPhysicalDeviceExternalBufferPropertiesKHR = reinterpret_cast<PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR>(
        vk::GetInstanceProcAddr(instance(), "vkGetPhysicalDeviceExternalBufferPropertiesKHR"));
    ASSERT_TRUE(vkGetPhysicalDeviceExternalBufferPropertiesKHR != nullptr);

    // Check for import/export capability
    // export used to feed memory to test import
    auto ebi = LvlInitStruct<VkPhysicalDeviceExternalBufferInfo>();
    ebi.handleType = handle_type;
    ebi.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    auto ebp = LvlInitStruct<VkExternalBufferPropertiesKHR>();
    vkGetPhysicalDeviceExternalBufferPropertiesKHR(gpu(), &ebi, &ebp);
    if (!(ebp.externalMemoryProperties.compatibleHandleTypes & handle_type) ||
        !(ebp.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_KHR) ||
        !(ebp.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT_KHR)) {
        GTEST_SKIP() << "External buffer does not support importing and exporting, skipping test";
    }

    // Check if dedicated allocation is required
    const bool dedicated_allocation =
        ebp.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT;

    if (dedicated_allocation) {
        if (!IsExtensionsEnabled(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME)) {
            GTEST_SKIP() << "Dedicated allocation extension not supported, skipping test";
        }
    }

    ASSERT_NO_FATAL_FAILURE(InitState());

    PFN_vkBindBufferMemory2KHR vkBindBufferMemory2Function =
        reinterpret_cast<PFN_vkBindBufferMemory2KHR>(vk::GetDeviceProcAddr(m_device->handle(), "vkBindBufferMemory2KHR"));
    PFN_vkBindImageMemory2KHR vkBindImageMemory2Function =
        reinterpret_cast<PFN_vkBindImageMemory2KHR>(vk::GetDeviceProcAddr(m_device->handle(), "vkBindImageMemory2KHR"));

    constexpr VkMemoryPropertyFlags mem_flags = 0;
    constexpr VkDeviceSize buffer_size = 1024;

    // Create export and import buffers
    auto external_buffer_info = LvlInitStruct<VkExternalMemoryBufferCreateInfo>();
    external_buffer_info.handleTypes = handle_type;

    auto buffer_info = VkBufferObj::create_info(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    buffer_info.pNext = &external_buffer_info;
    VkBufferObj buffer_export;
    buffer_export.init_no_mem(*m_device, buffer_info);

    auto importable_buffer_types =
        FindSupportedExternalMemoryHandleTypes(gpu(), buffer_info, VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT);
    importable_buffer_types &= ~handle_type;  // we need to find a flag that is different from handle_type
    if (importable_buffer_types == 0) GTEST_SKIP() << "Cannot find two different buffer handle types, skipping test";
    auto wrong_buffer_handle_type =
        static_cast<VkExternalMemoryHandleTypeFlagBits>(1 << MostSignificantBit(importable_buffer_types));
    external_buffer_info.handleTypes = wrong_buffer_handle_type;

    VkBufferObj buffer_import;
    buffer_import.init_no_mem(*m_device, buffer_info);

    // Allocation info
    auto alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, buffer_export.memory_requirements(), mem_flags);

    // Add export allocation info to pNext chain
    auto export_info = LvlInitStruct<VkExportMemoryAllocateInfoKHR>();
    export_info.handleTypes = handle_type;

    alloc_info.pNext = &export_info;

    // Add dedicated allocation info to pNext chain if required
    auto dedicated_info = LvlInitStruct<VkMemoryDedicatedAllocateInfo>();
    dedicated_info.buffer = buffer_export.handle();

    if (dedicated_allocation) {
        export_info.pNext = &dedicated_info;
    }

    // Allocate memory to be exported
    vk_testing::DeviceMemory memory_buffer_export;
    memory_buffer_export.init(*m_device, alloc_info);

    // Bind exported memory
    buffer_export.bind_memory(memory_buffer_export, 0);

    auto external_image_info = LvlInitStruct<VkExternalMemoryImageCreateInfoKHR>();
    external_image_info.handleTypes = handle_type;

    VkImageCreateInfo image_info = LvlInitStruct<VkImageCreateInfo>(&external_image_info);
    image_info.extent = {64, 64, 1};
    image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.arrayLayers = 1;
    image_info.mipLevels = 1;
    VkImageObj image_export(m_device);
    image_export.init_no_mem(*m_device, image_info);

    auto importable_image_types =
        FindSupportedExternalMemoryHandleTypes(gpu(), image_info, VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT);
    importable_image_types &= ~handle_type;  // we need to find a flag that is different from handle_type
    if (importable_image_types == 0) GTEST_SKIP() << "Cannot find two different image handle types, skipping test";
    auto wrong_image_handle_type = static_cast<VkExternalMemoryHandleTypeFlagBits>(1 << MostSignificantBit(importable_image_types));
    external_image_info.handleTypes = wrong_image_handle_type;

    VkImageObj image_import(m_device);
    image_import.init_no_mem(*m_device, image_info);

    // Allocation info
    dedicated_info = {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR, nullptr, image_export.handle(), VK_NULL_HANDLE};
    alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, image_export.memory_requirements(), mem_flags);
    alloc_info.pNext = &export_info;

    // Allocate memory to be exported
    vk_testing::DeviceMemory memory_image_export;
    memory_image_export.init(*m_device, alloc_info);

    // Bind exported memory
    image_export.bind_memory(memory_image_export, 0);

#ifdef _WIN32
    // Export memory to handle
    auto vkGetMemoryWin32HandleKHR =
        (PFN_vkGetMemoryWin32HandleKHR)vk::GetInstanceProcAddr(instance(), "vkGetMemoryWin32HandleKHR");
    ASSERT_TRUE(vkGetMemoryWin32HandleKHR != nullptr);
    VkMemoryGetWin32HandleInfoKHR mghi_buffer = {VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR, nullptr,
                                                 memory_buffer_export.handle(), handle_type};
    VkMemoryGetWin32HandleInfoKHR mghi_image = {VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR, nullptr,
                                                memory_image_export.handle(), handle_type};
    HANDLE handle_buffer;
    HANDLE handle_image;
    ASSERT_VK_SUCCESS(vkGetMemoryWin32HandleKHR(m_device->device(), &mghi_buffer, &handle_buffer));
    ASSERT_VK_SUCCESS(vkGetMemoryWin32HandleKHR(m_device->device(), &mghi_image, &handle_image));

    VkImportMemoryWin32HandleInfoKHR import_info_buffer = {VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, nullptr,
                                                           handle_type, handle_buffer};
    VkImportMemoryWin32HandleInfoKHR import_info_image = {VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR, nullptr,
                                                          handle_type, handle_image};
#else
    // Export memory to fd
    auto vkGetMemoryFdKHR = reinterpret_cast<PFN_vkGetMemoryFdKHR>(vk::GetInstanceProcAddr(instance(), "vkGetMemoryFdKHR"));
    ASSERT_TRUE(vkGetMemoryFdKHR != nullptr);

    auto mgfi_buffer = LvlInitStruct<VkMemoryGetFdInfoKHR>();
    mgfi_buffer.handleType = handle_type;
    mgfi_buffer.memory = memory_buffer_export.handle();

    auto mgfi_image = LvlInitStruct<VkMemoryGetFdInfoKHR>();
    mgfi_image.handleType = handle_type;
    mgfi_image.memory = memory_image_export.handle();

    int fd_buffer;
    int fd_image;
    ASSERT_VK_SUCCESS(vkGetMemoryFdKHR(m_device->device(), &mgfi_buffer, &fd_buffer));
    ASSERT_VK_SUCCESS(vkGetMemoryFdKHR(m_device->device(), &mgfi_image, &fd_image));

    auto import_info_buffer = LvlInitStruct<VkImportMemoryFdInfoKHR>();
    import_info_buffer.handleType = handle_type;
    import_info_buffer.fd = fd_buffer;

    auto import_info_image = LvlInitStruct<VkImportMemoryFdInfoKHR>();
    import_info_image.handleType = handle_type;
    import_info_image.fd = fd_image;
#endif

    // Import memory
    VkMemoryRequirements buffer_import_reqs = buffer_import.memory_requirements();
    if (buffer_import_reqs.memoryTypeBits == 0) {
        GTEST_SKIP() << "no suitable memory found, skipping test";
    }
    alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, buffer_import_reqs, mem_flags);
    alloc_info.pNext = &import_info_buffer;
    vk_testing::DeviceMemory memory_buffer_import;
    memory_buffer_import.init(*m_device, alloc_info);
    ASSERT_TRUE(memory_buffer_import.initialized());

    VkMemoryRequirements image_import_reqs = image_import.memory_requirements();
    if (image_import_reqs.memoryTypeBits == 0) {
        GTEST_SKIP() << "no suitable memory found, skipping test";
    }
    alloc_info = vk_testing::DeviceMemory::get_resource_alloc_info(*m_device, image_import_reqs, mem_flags);
    alloc_info.pNext = &import_info_image;
    vk_testing::DeviceMemory memory_image_import;
    memory_image_import.init(*m_device, alloc_info);

    // Bind imported memory with different handleType
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkBindBufferMemory-memory-02727");
    vk::BindBufferMemory(device(), buffer_import.handle(), memory_buffer_import.handle(), 0);
    m_errorMonitor->VerifyFound();

    VkBindBufferMemoryInfo bind_buffer_info = LvlInitStruct<VkBindBufferMemoryInfo>();
    bind_buffer_info.buffer = buffer_import.handle();
    bind_buffer_info.memory = memory_buffer_import.handle();
    bind_buffer_info.memoryOffset = 0;

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkBindBufferMemoryInfo-memory-02727");
    vkBindBufferMemory2Function(device(), 1, &bind_buffer_info);
    m_errorMonitor->VerifyFound();

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-vkBindImageMemory-memory-02729");
    m_errorMonitor->SetUnexpectedError("VUID-VkBindImageMemoryInfo-memory-01614");
    m_errorMonitor->SetUnexpectedError("VUID-VkBindImageMemoryInfo-memory-01612");
    vk::BindImageMemory(device(), image_import.handle(), memory_image_import.handle(), 0);
    m_errorMonitor->VerifyFound();

    VkBindImageMemoryInfo bind_image_info = LvlInitStruct<VkBindImageMemoryInfo>();
    bind_image_info.image = image_import.handle();
    bind_image_info.memory = memory_image_import.handle();
    bind_image_info.memoryOffset = 0;

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkBindImageMemoryInfo-memory-02729");
    m_errorMonitor->SetUnexpectedError("VUID-VkBindImageMemoryInfo-memory-01614");
    m_errorMonitor->SetUnexpectedError("VUID-VkBindImageMemoryInfo-memory-01612");
    vkBindImageMemory2Function(device(), 1, &bind_image_info);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, FenceExportWithUnsupportedHandleType) {
    TEST_DESCRIPTION("Create fence with unsupported external handle type in VkExportFenceCreateInfo");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    const auto exportable_types = FindSupportedExternalFenceHandleTypes(gpu(), VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    if (exportable_types == AllVkExternalFenceHandleTypeFlagBits) {
        GTEST_SKIP() << "This test requires at least one unsupported handle type, but all handle types are supported";
    }
    // Fence export with unsupported handle type
    const auto unsupported_type = LeastSignificantFlag<VkExternalFenceHandleTypeFlagBits>(~exportable_types);
    auto export_info = LvlInitStruct<VkExportFenceCreateInfo>();
    export_info.handleTypes = unsupported_type;

    const auto create_info = LvlInitStruct<VkFenceCreateInfo>(&export_info);
    VkFence fence = VK_NULL_HANDLE;
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportFenceCreateInfo-handleTypes-01446");
    vk::CreateFence(m_device->device(), &create_info, nullptr, &fence);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, FenceExportWithIncompatibleHandleType) {
    TEST_DESCRIPTION("Create fence with incompatible external handle types in VkExportFenceCreateInfo");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    const auto exportable_types = FindSupportedExternalFenceHandleTypes(gpu(), VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalFenceHandleTypeFlagBits>(exportable_types);
    const auto compatible_types = GetCompatibleHandleTypes(gpu(), handle_type);
    if ((exportable_types & compatible_types) == exportable_types) {
        GTEST_SKIP() << "Cannot find handle types that are supported but not compatible with each other";
    }

    // Fence export with incompatible handle types
    auto export_info = LvlInitStruct<VkExportFenceCreateInfo>();
    export_info.handleTypes = exportable_types;

    const auto create_info = LvlInitStruct<VkFenceCreateInfo>(&export_info);
    VkFence fence = VK_NULL_HANDLE;
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportFenceCreateInfo-handleTypes-01446");
    vk::CreateFence(m_device->device(), &create_info, nullptr, &fence);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, SemaphoreExportWithUnsupportedHandleType) {
    TEST_DESCRIPTION("Create semaphore with unsupported external handle type in VkExportSemaphoreCreateInfo");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    const auto exportable_types = FindSupportedExternalSemaphoreHandleTypes(gpu(), VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    if (exportable_types == AllVkExternalSemaphoreHandleTypeFlagBits) {
        GTEST_SKIP() << "This test requires at least one unsupported handle type, but all handle types are supported";
    }
    // Semaphore export with unsupported handle type
    const auto unsupported_type = LeastSignificantFlag<VkExternalSemaphoreHandleTypeFlagBits>(~exportable_types);
    auto export_info = LvlInitStruct<VkExportSemaphoreCreateInfo>();
    export_info.handleTypes = unsupported_type;

    const auto create_info = LvlInitStruct<VkSemaphoreCreateInfo>(&export_info);
    VkSemaphore semaphore = VK_NULL_HANDLE;
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportSemaphoreCreateInfo-handleTypes-01124");
    vk::CreateSemaphore(m_device->device(), &create_info, nullptr, &semaphore);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, SemaphoreExportWithIncompatibleHandleType) {
    TEST_DESCRIPTION("Create semaphore with incompatible external handle types in VkExportSemaphoreCreateInfo");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());

    const auto exportable_types = FindSupportedExternalSemaphoreHandleTypes(gpu(), VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT);
    if (!exportable_types) {
        GTEST_SKIP() << "Unable to find exportable handle type";
    }
    const auto handle_type = LeastSignificantFlag<VkExternalSemaphoreHandleTypeFlagBits>(exportable_types);
    const auto compatible_types = GetCompatibleHandleTypes(gpu(), handle_type);
    if ((exportable_types & compatible_types) == exportable_types) {
        GTEST_SKIP() << "Cannot find handle types that are supported but not compatible with each other";
    }

    // Semaphore export with incompatible handle types
    auto export_info = LvlInitStruct<VkExportSemaphoreCreateInfo>();
    export_info.handleTypes = exportable_types;

    const auto create_info = LvlInitStruct<VkSemaphoreCreateInfo>(&export_info);
    VkSemaphore semaphore = VK_NULL_HANDLE;
    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkExportSemaphoreCreateInfo-handleTypes-01124");
    vk::CreateSemaphore(m_device->device(), &create_info, nullptr, &semaphore);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ExternalMemoryAndExternalMemoryNV) {
    TEST_DESCRIPTION("Test for both external memory and external memory NV in image create pNext chain.");

    AddRequiredExtensions(VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME);
    AddRequiredExtensions(VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }

    ASSERT_NO_FATAL_FAILURE(InitState());

    auto external_mem_nv = LvlInitStruct<VkExternalMemoryImageCreateInfoNV>();
    auto external_mem = LvlInitStruct<VkExternalMemoryImageCreateInfo>(&external_mem_nv);
    VkImageCreateInfo ici = LvlInitStruct<VkImageCreateInfo>(&external_mem);
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.arrayLayers = 1;
    ici.extent = {64, 64, 1};
    ici.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici.mipLevels = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    const auto supported_types_nv =
        FindSupportedExternalMemoryHandleTypesNV(*this, ici, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV);
    const auto supported_types = FindSupportedExternalMemoryHandleTypes(gpu(), ici, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (!supported_types_nv || !supported_types) {
        GTEST_SKIP() << "Cannot find one regular handle type and one nvidia extension's handle type";
    }
    external_mem_nv.handleTypes = LeastSignificantFlag<VkExternalMemoryFeatureFlagBitsNV>(supported_types_nv);
    external_mem.handleTypes = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(supported_types);

    m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImageCreateInfo-pNext-00988");
    VkImage test_image;
    vk::CreateImage(device(), &ici, nullptr, &test_image);
    m_errorMonitor->VerifyFound();
}

TEST_F(VkLayerTest, ValidateExternalMemoryImageLayout) {
    TEST_DESCRIPTION("Validate layout of image with external memory");

    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddOptionalExtensions(VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(Init());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }

    VkExternalMemoryImageCreateInfo external_mem = LvlInitStruct<VkExternalMemoryImageCreateInfo>();
    VkImageCreateInfo ici = LvlInitStruct<VkImageCreateInfo>(&external_mem);
    ici.imageType = VK_IMAGE_TYPE_2D;
    ici.arrayLayers = 1;
    ici.extent = {32, 32, 1};
    ici.format = VK_FORMAT_R8G8B8A8_UNORM;
    ici.mipLevels = 1;
    ici.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    ici.samples = VK_SAMPLE_COUNT_1_BIT;
    ici.tiling = VK_IMAGE_TILING_OPTIMAL;
    ici.usage = VK_IMAGE_USAGE_SAMPLED_BIT;

    const auto supported_types = FindSupportedExternalMemoryHandleTypes(gpu(), ici, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT);
    if (supported_types) {
        external_mem.handleTypes = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBits>(supported_types);
        VkImage test_image;
        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImageCreateInfo-pNext-01443");
        vk::CreateImage(device(), &ici, nullptr, &test_image);
        m_errorMonitor->VerifyFound();
    }
    if (IsExtensionsEnabled(VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME)) {
        auto external_mem_nv = LvlInitStruct<VkExternalMemoryImageCreateInfoNV>();
        const auto supported_types_nv =
            FindSupportedExternalMemoryHandleTypesNV(*this, ici, VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT_NV);
        if (supported_types_nv) {
            external_mem_nv.handleTypes = LeastSignificantFlag<VkExternalMemoryHandleTypeFlagBitsNV>(supported_types_nv);
            ici.pNext = &external_mem_nv;
            VkImage test_image;
            m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkImageCreateInfo-pNext-01443");
            vk::CreateImage(device(), &ici, nullptr, &test_image);
            m_errorMonitor->VerifyFound();
        }
    }
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
TEST_F(VkLayerTest, InvalidD3D12FenceSubmitInfo) {
    TEST_DESCRIPTION("Test invalid D3D12FenceSubmitInfo");
    AddRequiredExtensions(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());
    vk_testing::Semaphore semaphore(*m_device);

    // VkD3D12FenceSubmitInfoKHR::waitSemaphoreValuesCount == 1 is different from VkSubmitInfo::waitSemaphoreCount == 0
    {
        const uint64_t waitSemaphoreValues = 0;
        auto d3d12_fence_submit_info = LvlInitStruct<VkD3D12FenceSubmitInfoKHR>();
        d3d12_fence_submit_info.waitSemaphoreValuesCount = 1;
        d3d12_fence_submit_info.pWaitSemaphoreValues = &waitSemaphoreValues;
        const auto submit_info = LvlInitStruct<VkSubmitInfo>(&d3d12_fence_submit_info);

        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkD3D12FenceSubmitInfoKHR-waitSemaphoreValuesCount-00079");
        vk::QueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
        m_errorMonitor->VerifyFound();
    }
    // VkD3D12FenceSubmitInfoKHR::signalSemaphoreCount == 0 is different from VkSubmitInfo::signalSemaphoreCount == 1
    {
        auto d3d12_fence_submit_info = LvlInitStruct<VkD3D12FenceSubmitInfoKHR>();
        auto submit_info = LvlInitStruct<VkSubmitInfo>(&d3d12_fence_submit_info);
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &semaphore.handle();

        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkD3D12FenceSubmitInfoKHR-signalSemaphoreValuesCount-00080");
        vk::QueueSubmit(m_device->m_queue, 1, &submit_info, VK_NULL_HANDLE);
        m_errorMonitor->VerifyFound();
    }
}
#endif  // VK_USE_PLATFORM_WIN32_KHR

TEST_F(VkLayerTest, GetMemoryFdHandle) {
    TEST_DESCRIPTION("Validate VkMemoryGetFdInfoKHR passed to vkGetMemoryFdKHR");
    SetTargetApiVersion(VK_API_VERSION_1_1);
    AddRequiredExtensions(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    ASSERT_NO_FATAL_FAILURE(InitFramework());
    if (DeviceValidationVersion() < VK_API_VERSION_1_1) {
        GTEST_SKIP() << "At least Vulkan version 1.1 is required";
    }
    if (!AreRequiredExtensionsEnabled()) {
        GTEST_SKIP() << RequiredExtensionsNotSupported() << " not supported";
    }
    ASSERT_NO_FATAL_FAILURE(InitState());
    auto vkGetMemoryFdKHR = GetInstanceProcAddr<PFN_vkGetMemoryFdKHR>("vkGetMemoryFdKHR");
    int fd = -1;

    // Allocate memory without VkExportMemoryAllocateInfo in the pNext chain
    {
        auto alloc_info = LvlInitStruct<VkMemoryAllocateInfo>();
        alloc_info.allocationSize = 32;
        alloc_info.memoryTypeIndex = 0;
        vk_testing::DeviceMemory memory;
        memory.init(*m_device, alloc_info);

        auto get_handle_info = LvlInitStruct<VkMemoryGetFdInfoKHR>();
        get_handle_info.memory = memory;
        get_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkMemoryGetFdInfoKHR-handleType-00671");
        vkGetMemoryFdKHR(*m_device, &get_handle_info, &fd);
        m_errorMonitor->VerifyFound();
    }
    // VkExportMemoryAllocateInfo::handleTypes does not include requested handle type
    {
        auto export_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
        export_info.handleTypes = 0;

        auto alloc_info = LvlInitStruct<VkMemoryAllocateInfo>(&export_info);
        alloc_info.allocationSize = 1024;
        alloc_info.memoryTypeIndex = 0;
        vk_testing::DeviceMemory memory;
        memory.init(*m_device, alloc_info);

        auto get_handle_info = LvlInitStruct<VkMemoryGetFdInfoKHR>();
        get_handle_info.memory = memory;
        get_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;

        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkMemoryGetFdInfoKHR-handleType-00671");
        vkGetMemoryFdKHR(*m_device, &get_handle_info, &fd);
        m_errorMonitor->VerifyFound();
    }
    // Request handle of the wrong type
    {
        auto export_info = LvlInitStruct<VkExportMemoryAllocateInfo>();
        export_info.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;

        auto alloc_info = LvlInitStruct<VkMemoryAllocateInfo>(&export_info);
        alloc_info.allocationSize = 1024;
        alloc_info.memoryTypeIndex = 0;

        vk_testing::DeviceMemory memory;
        memory.init(*m_device, alloc_info);
        auto get_handle_info = LvlInitStruct<VkMemoryGetFdInfoKHR>();
        get_handle_info.memory = memory;
        get_handle_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT;

        m_errorMonitor->SetDesiredFailureMsg(kErrorBit, "VUID-VkMemoryGetFdInfoKHR-handleType-00672");
        vkGetMemoryFdKHR(*m_device, &get_handle_info, &fd);
        m_errorMonitor->VerifyFound();
    }
}