//
// Created by jovasa on 15.1.2024.
//

#include <iostream>
#include "eventHandler.h"

#include "zmq.h"
#include "util.h"

void EventHandler::handle(sf::Event &event, config &cfg, sf::RenderWindow &window) {
    if (event.type == sf::Event::Closed) {
        window.close();
        cfg.running = false;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::F) {
            if (cfg.fullscreen) {
                window.create(sf::VideoMode(width, height), "VVC Visualizer", sf::Style::Default);
            } else {
                window.create(sf::VideoMode(2560, 1440), "VVC Visualizer", sf::Style::Fullscreen);
            }
            cfg.fullscreen = !cfg.fullscreen;
        }
        if (event.key.code == sf::Keyboard::Escape) {
            window.close();
            cfg.running = false;
        }
        if (event.key.code == sf::Keyboard::G) {
            cfg.show_grid = !cfg.show_grid;
        }
        if (event.key.code == sf::Keyboard::Z) {
            cfg.show_zoom = !cfg.show_zoom;
        }
        if (event.key.code == sf::Keyboard::I) {
            cfg.show_intra = !cfg.show_intra;
        }
        if (event.key.code == sf::Keyboard::D) {
            cfg.show_debug = !cfg.show_debug;
        }
        if (event.key.code == sf::Keyboard::H) {
            cfg.show_help = !cfg.show_help;
        }
        if(event.key.code == sf::Keyboard::S) {
            char msg[] = "S";
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::LControl || event.key.code == sf::Keyboard::RControl) {
            ctrl_pressed = true;
        }
        if(event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
            shift_pressed = true;
        }
        if(event.key.code == sf::Keyboard::LAlt || event.key.code == sf::Keyboard::RAlt) {
            alt_pressed = true;
        }
    }

    if (event.type == sf::Event::KeyReleased) {
        if(event.key.code == sf::Keyboard::LControl || event.key.code == sf::Keyboard::RControl) {
            ctrl_pressed = false;
        }
        if(event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
            shift_pressed = false;
        }
        if(event.key.code == sf::Keyboard::LAlt || event.key.code == sf::Keyboard::RAlt) {
            alt_pressed = false;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed) {
        TimeStamp ts;
        uint64_t now;
        GET_TIME(ts, now);
        if (event.mouseButton.button == sf::Mouse::Left) {
            if (now - last_click < 400'000'000) {
                if (cfg.fullscreen) {
                    window.create(sf::VideoMode(width, height), "VVC Visualizer", sf::Style::Default);
                } else {
                    window.create(sf::VideoMode(2560, 1440), "VVC Visualizer", sf::Style::Fullscreen);
                }
                cfg.fullscreen = !cfg.fullscreen;
            }
            last_click = now;
        }
        if (event.mouseButton.button == sf::Mouse::Right) {
            if (now - last_click2 < 200) {
            }
            last_click2 = now;
        }
    }
}
