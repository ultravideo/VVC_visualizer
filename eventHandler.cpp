//
// Created by jovasa on 15.1.2024.
//

#include <iostream>
#include "eventHandler.h"

#include "zmq.h"
#include "util.h"

bool EventHandler::handle(sf::Event &event, config &cfg, sf::RenderWindow &window) {
    if (event.type == sf::Event::Closed) {
        window.close();
        cfg.running = false;
    }

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::F) {
            toggleFullscreen(cfg, window);
            return true;
        }
        if (event.key.code == sf::Keyboard::Escape) {
            window.close();
            cfg.running = false;
        }
        if (event.key.code == sf::Keyboard::G) {
            cfg.show_grid = !cfg.show_grid;
            return true;
        }
        if (event.key.code == sf::Keyboard::Z) {
            cfg.show_zoom = !cfg.show_zoom;
        }
        if (event.key.code == sf::Keyboard::I) {
            cfg.show_intra = !cfg.show_intra;
            return true;
        }
        if (event.key.code == sf::Keyboard::D) {
            cfg.show_debug = !cfg.show_debug;
        }
        if (event.key.code == sf::Keyboard::H || event.key.code == sf::Keyboard::F1) {
            cfg.show_help = !cfg.show_help;
        }
        if (event.key.code == sf::Keyboard::Q) {
            cfg.show_qp = !cfg.show_qp;
            return true;
        }
        if (event.key.code == sf::Keyboard::W) {
            cfg.show_transform = !cfg.show_transform;
            return true;
        }
        if (event.key.code == sf::Keyboard::E) {
            cfg.show_isp = !cfg.show_isp;
            return true;
        }
        if (event.key.code == sf::Keyboard::Space) {
            cfg.paused = !cfg.paused;
            return true;
        }
        if(event.key.code == sf::Keyboard::S) {
            char *msg = const_cast<char *>(shift_pressed ? "s" : "S");
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::M) {
            char *msg = const_cast<char *>(shift_pressed ? "m" : "M");
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::P) {
            char *msg = const_cast<char *>(shift_pressed ? "p" : "P");
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::T) {
            char *msg = const_cast<char *>(shift_pressed ? "t" : "T");
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::L) {
            char *msg = const_cast<char *>(shift_pressed ? "l" : "L");
            zmq_send(control_socket, msg, 1, 0);
        }
        if(event.key.code == sf::Keyboard::Num0 || event.key.code == sf::Keyboard::Numpad0) {
            char *msg = const_cast<char *>(ctrl_pressed ? "C0" : "0");
            zmq_send(control_socket, msg, 2, 0);
        }
        if(event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::Numpad1) {
            char *msg = const_cast<char *>(ctrl_pressed ? "C1" : "1");
            zmq_send(control_socket, msg, 2, 0);
        }
        if(event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            char *msg = const_cast<char *>(ctrl_pressed ? "C2" : "2");
            zmq_send(control_socket, msg, 2, 0);
        }
        if(event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
            char *msg = const_cast<char *>(ctrl_pressed ? "C3" : "3");
            zmq_send(control_socket, msg, 2, 0);
        }
        if(event.key.code >= sf::Keyboard::Num4 && event.key.code < sf::Keyboard::Num9) {
            if(!ctrl_pressed && !shift_pressed) {
                const std::string temp = std::to_string(event.key.code - sf::Keyboard::Num0);
                char *msg = const_cast<char *>(temp.c_str());
                zmq_send(control_socket, msg, 1, 0);
            }
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
                toggleFullscreen(cfg, window);
                return true;
            }
            last_click = now;
        }
        if (event.mouseButton.button == sf::Mouse::Right) {
            if (now - last_click2 < 200) {
            }
            last_click2 = now;
        }
    }
    return false;
}

void EventHandler::toggleFullscreen(config &cfg, sf::RenderWindow &window) const {
    if (cfg.fullscreen) {
        window.create(sf::VideoMode(width, height), "VVC Visualizer", sf::Style::Default);
    } else {
        window.create(sf::VideoMode(2560, 1440), "VVC Visualizer", sf::Style::Fullscreen);
    }
    cfg.fullscreen = !cfg.fullscreen;
}
