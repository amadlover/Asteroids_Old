#include "scene_graphics.hpp"
#include "scene_assets.hpp"
#include "scene_assets.hpp"
#include "common_graphics.hpp"
#include "vk_utils.hpp"

#include <iostream>
#include <Windows.h>

scene_graphics::scene_graphics (const scene_assets* assets, 
                                const common_graphics* c_graphics) : graphics_device (c_graphics->graphics_device.get ()), 
                                                                     graphics_queue (c_graphics->device_queues->graphics_queue.get ()), 
                                                                     swapchain (c_graphics->swapchain.get ()),
                                                                     assets (assets)
{
    //OutputDebugString (L"scene_graphics::scene_graphics assets c_graphics\n");

    vk::DeviceSize current_data_size = 0;
    std::vector<unsigned char> total_data;

    {
        for (const auto& mesh : assets->static_meshes)
        {
            for (auto& primitive : mesh.alpha_graphics_primitives->primitives)
            {
                total_data.resize (total_data.size () + 
                                   primitive.positions.size () + 
                                   primitive.normals.size () + 
                                   primitive.uv0s.size () + 
                                   primitive.indices.size ());

                primitive.positions_offset = current_data_size;
                std::copy (primitive.positions.begin (), 
                           primitive.positions.end (), 
                           total_data.begin () + static_cast<int>(current_data_size));
                current_data_size += primitive.positions.size ();

                primitive.normals_offset = current_data_size;
                std::copy (primitive.normals.begin (), 
                           primitive.normals.end (), 
                           total_data.begin () + static_cast<int>(current_data_size));
                current_data_size += primitive.normals.size ();

                primitive.uv0s_offset = current_data_size;
                std::copy (primitive.uv0s.begin (), 
                           primitive.uv0s.end (), 
                           total_data.begin () + static_cast<int>(current_data_size));
                current_data_size += primitive.uv0s.size ();

                primitive.indices_offset = current_data_size;
                std::copy (primitive.indices.begin (), 
                           primitive.indices.end (), 
                           total_data.begin () + static_cast<int>(current_data_size));
                current_data_size += primitive.indices.size ();
            }
        }

        if (current_data_size > 0)
        {
            vk_buffer staging_buffer (c_graphics->graphics_device->graphics_device, 
                                      current_data_size, 
                                      vk::BufferUsageFlagBits::eTransferSrc, 
                                      vk::SharingMode::eExclusive, 
                                      0);
            vk_device_memory staging_buffer_memory (c_graphics->graphics_device->graphics_device, 
                                                    staging_buffer.buffer, 
                                                    c_graphics->physical_device->memory_properties, 
                                                    vk::MemoryPropertyFlagBits::eHostVisible);

            staging_buffer_memory.bind_buffer (staging_buffer.buffer, 0);
            staging_buffer_memory.map_data (total_data, 0);

            vb_ib = std::make_unique<vk_buffer> (c_graphics->graphics_device->graphics_device, 
                                                 current_data_size, 
                                                 vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer, 
                                                 vk::SharingMode::eExclusive, 
                                                 c_graphics->queue_family_indices->transfer_queue_family_index);
            vb_ib_memory = std::make_unique<vk_device_memory> (c_graphics->graphics_device->graphics_device, 
                                                               vb_ib->buffer, 
                                                               c_graphics->physical_device->memory_properties, 
                                                               vk::MemoryPropertyFlagBits::eDeviceLocal);

            vb_ib->bind_memory (vb_ib_memory->device_memory, 0);
            vb_ib->copy_from_buffer (staging_buffer.buffer, 
                                     current_data_size, 
                                     c_graphics->transfer_command_pool->command_pool, 
                                     c_graphics->device_queues->transfer_queue->queue);
            
            current_data_size = 0;
            total_data.clear ();
            total_data.shrink_to_fit ();
        }
    }
    
    {
        for (const auto& mesh : assets->static_meshes)
        {
            for (auto& primitive : mesh.alpha_graphics_primitives->primitives)
            {
                primitive.mat->base_image->image_data_offset = current_data_size;
                total_data.resize (total_data.size () + primitive.mat->base_image->image_data.size ());
                std::copy (primitive.mat->base_image->image_data.begin (), 
                           primitive.mat->base_image->image_data.end (), 
                           total_data.begin () + static_cast<int> (current_data_size));
                current_data_size += primitive.mat->base_image->image_data.size ();
            }
        }

        if (current_data_size > 0)
        {
            std::vector<image*> images;
            images.reserve (5);

            std::map<vk::Image, vk::DeviceSize> images_offsets;

            for (const auto& mesh : assets->static_meshes)
            {
                for (auto& primitive : mesh.alpha_graphics_primitives->primitives)
                {
                    primitive.mat->base_image->img = std::make_unique<vk_image> (c_graphics->graphics_device->graphics_device, 
                                                                                 c_graphics->queue_family_indices->graphics_queue_family_index, 
                                                                                 vk::Extent3D (primitive.mat->base_image->width, primitive.mat->base_image->height, 1), 
                                                                                 1, 
                                                                                 vk::Format::eR8G8B8A8Unorm, 
                                                                                 vk::ImageLayout::eUndefined, 
                                                                                 vk::SharingMode::eExclusive, 
                                                                                 vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
                    images.push_back (primitive.mat->base_image.get ());

                    images_offsets.insert (std::make_pair (primitive.mat->base_image->img->image, primitive.mat->base_image->image_data_offset));
                }
            }

            vk_buffer staging_image_buffer (c_graphics->graphics_device->graphics_device, 
                                            current_data_size, 
                                            vk::BufferUsageFlagBits::eTransferSrc, 
                                            vk::SharingMode::eExclusive, 
                                            0);
            vk_device_memory staging_image_memory (c_graphics->graphics_device->graphics_device, 
                                                   staging_image_buffer.buffer, 
                                                   c_graphics->physical_device->memory_properties, 
                                                   vk::MemoryPropertyFlagBits::eHostVisible);

            staging_image_buffer.bind_memory (staging_image_memory.device_memory, 0);
            staging_image_memory.map_data (total_data, 0);

            image_memory = std::make_unique<vk_device_memory> (c_graphics->graphics_device->graphics_device, 
                                                               images, 
                                                               c_graphics->physical_device->memory_properties, 
                                                               vk::MemoryPropertyFlagBits::eDeviceLocal);
            image_memory->bind_images (images_offsets);

            for (const auto& image : images)
            {
                image->img->change_layout (c_graphics->device_queues->transfer_queue->queue, 
                                           c_graphics->transfer_command_pool->command_pool, 
                                           c_graphics->queue_family_indices->transfer_queue_family_index, 
                                           c_graphics->queue_family_indices->transfer_queue_family_index, 
                                           image->img->image, 
                                           1, 
                                           vk::ImageLayout::eUndefined, 
                                           vk::ImageLayout::eTransferDstOptimal, 
                                           vk::AccessFlagBits::eMemoryRead, 
                                           vk::AccessFlagBits::eTransferWrite, 
                                           vk::PipelineStageFlagBits::eTopOfPipe, 
                                           vk::PipelineStageFlagBits::eTransfer);
                image->img->copy_from_buffer (c_graphics->device_queues->transfer_queue->queue, 
                                              c_graphics->transfer_command_pool->command_pool, 
                                              image->image_data_offset, 
                                              staging_image_buffer.buffer, 
                                              vk::Extent3D (image->width, image->height, 1), 
                                              1);
                image->img->change_layout(c_graphics->device_queues->transfer_queue->queue, 
                                          c_graphics->transfer_command_pool->command_pool, 
                                          c_graphics->queue_family_indices->transfer_queue_family_index, 
                                          c_graphics->queue_family_indices->graphics_queue_family_index, 
                                          image->img->image, 
                                          1, 
                                          vk::ImageLayout::eUndefined, 
                                          vk::ImageLayout::eShaderReadOnlyOptimal, 
                                          vk::AccessFlagBits::eTransferWrite, 
                                          vk::AccessFlagBits::eShaderRead, 
                                          vk::PipelineStageFlagBits::eTransfer, 
                                          vk::PipelineStageFlagBits::eFragmentShader);
                image->img_view = std::make_unique<vk_image_view> (c_graphics->graphics_device->graphics_device, 
                                                                   image->img->image, 
                                                                   vk::Format::eR8G8B8A8Unorm);
            }
        }
    }

    render_pass = std::make_unique<vk_render_pass> (c_graphics->graphics_device->graphics_device, c_graphics->surface->surface_format.format);
    framebuffers = std::make_unique<vk_framebuffers> (c_graphics->graphics_device->graphics_device, 
                                                      c_graphics->swapchain->image_views, 
                                                      render_pass->render_pass, 
                                                      c_graphics->surface->surface_extent);
    signal_semaphores = std::make_unique<vk_semaphores> (c_graphics->graphics_device->graphics_device, c_graphics->swapchain->images.size ());
    wait_semaphore = std::make_unique<vk_semaphore> (c_graphics->graphics_device->graphics_device);

    graphics_command_pool = std::make_unique<vk_command_pool> (c_graphics->graphics_device->graphics_device, 
                                                               c_graphics->queue_family_indices->graphics_queue_family_index, 
                                                               vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    
    swapchain_command_buffers = std::make_unique<vk_command_buffers> (c_graphics->graphics_device->graphics_device, 
                                                                      graphics_command_pool->command_pool, 
                                                                      c_graphics->swapchain->images.size ());

    vertex_shader_module = std::make_unique<vk_shader_module> (full_file_path ("pbr_opaque.vert.spv"), c_graphics->graphics_device->graphics_device, vk::ShaderStageFlagBits::eVertex);
    fragment_shader_module = std::make_unique<vk_shader_module> (full_file_path ("pbr_opaque.frag.spv"), c_graphics->graphics_device->graphics_device, vk::ShaderStageFlagBits::eFragment);
}

void scene_graphics::update_command_buffers (const vk::Extent2D& surface_extent)
{
    swapchain_command_buffers->begin (vk::CommandBufferUsageFlagBits::eSimultaneousUse);
    swapchain_command_buffers->draw (render_pass->render_pass, framebuffers->framebuffers, vk::Rect2D ({}, { surface_extent }));
    swapchain_command_buffers->end ();
}

void scene_graphics::update_command_buffers (const vk::Extent2D& extent, const std::string& actor_name)
{
    for (const auto& asset : assets->static_meshes)
    {
        std::cout << asset.name << "\n";
    }
}

void scene_graphics::main_loop ()
{
    vk::ResultValue<uint32_t> result = graphics_device->graphics_device.acquireNextImageKHR (swapchain->swapchain, UINT64_MAX, wait_semaphore->semaphore, nullptr);

    if (result.result != vk::Result::eSuccess)
    {
        if (result.result != vk::Result::eSuboptimalKHR && result.result != vk::Result::eErrorOutOfDateKHR)
        {
            throw "Acquire next image";
        }
        else
        {
            graphics_queue->queue.waitIdle ();
        }
    }

    graphics_queue->submit (vk::PipelineStageFlagBits::eColorAttachmentOutput, 
                            swapchain_command_buffers->command_buffers.at (result.value), 
                            wait_semaphore->semaphore, 
                            signal_semaphores->semaphores.at (result.value).semaphore);
    graphics_queue->present (swapchain->swapchain, 
                             result.value, 
                             signal_semaphores->semaphores.at (result.value).semaphore);
}

scene_graphics::~scene_graphics ()
{
    //OutputDebugString (L"scene_graphics::~scene_graphics\n");

    graphics_queue->queue.waitIdle ();
}
