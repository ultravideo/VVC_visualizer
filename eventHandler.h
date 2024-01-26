//
// Created by jovasa on 15.1.2024.
//

#ifndef VISUALIZER_EVENTHANDLER_H
#define VISUALIZER_EVENTHANDLER_H


#include <cstdint>
#include <SFML/Window/Event.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include "config.h"

class EventHandler {
    bool shift_pressed = false;
    bool ctrl_pressed = false;
    bool alt_pressed = false;

    uint64_t last_click = 0;
    uint64_t last_click2 = 0;

    uint32_t width;
    uint32_t height;

    void *control_socket = nullptr;
public:
    EventHandler(uint32_t width, uint32_t height, void* socket);

    EventHandler(const EventHandler &) = delete;

    EventHandler &operator=(const EventHandler &) = delete;

    bool handle(sf::Event &event, config &cfg, sf::RenderWindow &window);

    void toggleFullscreen(config &cfg, sf::RenderWindow &window) const;
};


#endif //VISUALIZER_EVENTHANDLER_H
