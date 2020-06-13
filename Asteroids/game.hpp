#pragma once

#include <Windows.h>
#include <memory>

#include "scene.hpp"
#include "common_graphics.hpp"


class scene;


class game
{
public:
    game (HINSTANCE h_instance, HWND h_wnd);
    ~game ();

    void set_current_scene (SCENE_TYPE type);

    void process_keyboard_input (WPARAM WParam, LPARAM LParam);
    void main_loop ();

private:
    std::unique_ptr<common_graphics> graphics = nullptr;
    std::unique_ptr<scene> current_scene = nullptr;
};