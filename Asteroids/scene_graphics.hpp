#pragma once


class static_meshes;
class common_graphics;

class scene_graphics
{
public:
    scene_graphics (const static_meshes* meshes, const common_graphics* c_graphics);

private:
    std::unique_ptr<vk_buffer> vb_ib;
    std::unique_ptr<vk_device_memory> vb_ib_memory;

    std::unique_ptr<vk_device_memory> image_memory;
};