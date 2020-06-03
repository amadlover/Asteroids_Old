#pragma once

#include "scene.hpp"


class test_scene : public scene
{
public:
    test_scene ();
    ~test_scene ();

    void init () override;
    void process_keyboard_input (WPARAM WParam, LPARAM LParam) override;
    void main_loop () override;
    void shutdown () override;
};