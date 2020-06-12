#pragma once

#include <tiny_gltf.h>
#include <vulkan/vulkan.hpp>

class vk_texture
{
public:
    vk_texture (const tinygltf::Image& image);
    ~vk_texture ();

    std::string name;

    std::vector<unsigned char> image_data;
    vk::DeviceSize image_data_offset;

    vk::DeviceSize width;
    vk::DeviceSize height;

    vk::Image image;
    vk::ImageView image_view;
};