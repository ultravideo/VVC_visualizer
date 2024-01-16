#include <fstream>
#include <iostream>
#include <cstring>

#include <zmq.h>
#include <SFML/Graphics.hpp>
#include <cstdint>

#include "cu.h"
#include "util.h"

#define _USE_MATH_DEFINES

#include "math.h"
#include "config.h"
#include "eventHandler.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif

struct func_parameters {
    sf::RenderTexture &edgeImage;
    const sf::Color *const colors;
    uint32_t top_left_x;
    uint32_t top_left_y;
    float scale;
};

void
drawZoomWindow(const sf::Color *const colors, const sf::RenderTexture &imageTexture, const int width, const int height,
               const sub_image_stats *const stat_array, sf::RenderTexture &zoomOverlayTexture, sf::RenderWindow &window,
               const sf::Vector2i &previous_mouse_position, sf::Image &zoomImage, const sf::Vector2i &mousePosition,
               const float scaleX, const float scaleY);

void draw_cu(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture &edgeImage = params->edgeImage;
    const sf::Color *const colors = params->colors;
    // Draw the lines on the RenderTexture
    int frame_index_modulo = current_cu->frame_num % 4;
    sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(
                    (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale - 1,
                    (cuLoc->y - params->top_left_y) * params->scale), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(
                    (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale,
                    (cuLoc->y + cuLoc->height - params->top_left_y) * params->scale - 1), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(
                    (cuLoc->x - params->top_left_x) * params->scale,
                    (cuLoc->y + cuLoc->height - params->top_left_y) * params->scale - 1), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(
                    (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale,
                    (cuLoc->y + cuLoc->height - params->top_left_y) * params->scale - 1), colors[frame_index_modulo])
    };
    edgeImage.draw(line, 4, sf::Lines);

}

void drawIntraModes(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture &edgeImage = params->edgeImage;

    // Visualize intra angle if the CU is big enough
    if (cuLoc->height * params->scale >= 7.5 && cuLoc->width * params->scale >= 7.5) {
        const sf::Vector2f center = sf::Vector2f(
                (cuLoc->x + cuLoc->width / 2 - params->top_left_x) * params->scale,
                (cuLoc->y + cuLoc->height / 2 - params->top_left_y) * params->scale);
        sf::Color color = sf::Color::White;
        if (current_cu->mrl == 1) {
            color = sf::Color::Red;
        } else if (current_cu->mrl == 2) {
            color = sf::Color::Blue;
        }
        if (!current_cu->is_mip && current_cu->intra_mode >= 2) {
            static const int16_t modedisp2sampledisp[32] = {0, 1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 18, 20, 23, 26, 29, 32,
                                                            35, 39, 45, 51, 57, 64, 73, 86, 102, 128, 171, 256, 341,
                                                            512, 1024};
            uint16_t pred_mode = current_cu->intra_mode;
            const bool vertical_mode = pred_mode >= 34;
            const int32_t mode_disp = vertical_mode ? pred_mode - 50 : -((int32_t) pred_mode - 18);
            const int32_t sample_disp = modedisp2sampledisp[abs(mode_disp)];
            double angle = (sample_disp / 32. * M_PI) / 4.;
            if (vertical_mode) {
                if (mode_disp > 0) {
                    angle = M_PI - angle;
                } else {
                    angle = M_PI + angle;
                }
            } else {
                if (mode_disp > 0) {
                    angle = M_PI / 2 + angle;
                } else {
                    angle = M_PI / 2 - angle;
                }
            }
            sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(center.x - 3 * cos(angle), center.y - 3 * sin(angle)), color),
                    sf::Vertex(sf::Vector2f(center.x + 3 * cos(angle), center.y + 3 * sin(angle)), color),
            };
            edgeImage.draw(line, 2, sf::Lines);
        } else if (!current_cu->is_mip) {
            sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(center.x - 3, center.y - 3), color),
                    current_cu->intra_mode == 0 ?
                    sf::Vertex(sf::Vector2f(center.x + 3, center.y + 3), color) :
                    sf::Vertex(center, color)
            };
            edgeImage.draw(line, 2, sf::Lines);
            sf::Vertex line2[] = {
                    sf::Vertex(sf::Vector2f(center.x - 3, center.y + 3), color),
                    sf::Vertex(sf::Vector2f(center.x + 3, center.y - 3), color),
            };
            edgeImage.draw(line2, 2, sf::Lines);
        } else {
            if (current_cu->mip_transpose) {
                sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y + 3), color),
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y + 3), color),
                };
                edgeImage.draw(line, 2, sf::Lines);
                sf::Vertex line2[] = {
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y - 3), color),
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y - 3), color),
                };
                edgeImage.draw(line2, 2, sf::Lines);
            } else {
                sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y - 3), color),
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y + 3), color),
                };
                edgeImage.draw(line, 2, sf::Lines);
                sf::Vertex line2[] = {
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y - 3), color),
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y + 3), color),
                };
                edgeImage.draw(line2, 2, sf::Lines);
            }
        }
    }
}


void drawZoomWindow(const sf::Color *const colors, const sf::RenderTexture &imageTexture, const int width, const int height,
                    const sub_image_stats *const stat_array, sf::RenderTexture &zoomOverlayTexture, sf::RenderWindow &window,
                    sf::Vector2i &previous_mouse_position, sf::Image &zoomImage, const sf::Vector2i &mousePosition,
                    const float scaleX, const float scaleY) {
    int top_right_x_of_zoom_area = clamp(static_cast<int>(mousePosition.x / scaleX - 32), 0, width - 64);
    int top_right_y_of_zoom_area = clamp(static_cast<int>(mousePosition.y / scaleY - 32), 0, height - 64);

    int top_left_needed_cu_x = clamp(floor_div(static_cast<int>(mousePosition.x / scaleX - 32), 64) * 64, 0,
                                     (width / 64 - 2) * 64);
    int top_left_needed_cu_y = clamp(floor_div(static_cast<int>(mousePosition.y / scaleY - 32), 64) * 64, 0,
                                     (height / 64 - 2) * 64);

    if(top_right_x_of_zoom_area != previous_mouse_position.x || top_right_y_of_zoom_area != previous_mouse_position.y) {
        previous_mouse_position.x = top_right_x_of_zoom_area;
        previous_mouse_position.y = top_right_y_of_zoom_area;
        zoomImage.copy(imageTexture.getTexture().copyToImage(), 0, 0,
                       sf::IntRect(
                               top_right_x_of_zoom_area,
                               top_right_y_of_zoom_area,
                               64, 64));

    }
    zoomOverlayTexture.clear(sf::Color::Transparent);

    func_parameters params = { zoomOverlayTexture, colors,
                              static_cast<uint32_t>(top_left_needed_cu_x),
                              static_cast<uint32_t>(top_left_needed_cu_y), 4 };
    std::vector<std::function<void(void*, const cu_loc_t* const, const sub_image_stats* const)> > funcs;
    std::vector<void*> data;
    funcs.emplace_back(draw_cu);
    data.push_back((void*)&params);
    funcs.emplace_back(drawIntraModes);
    data.push_back((void*)&params);

    for (int x = top_left_needed_cu_x; x < top_left_needed_cu_x + 64 * 2; x += 64) {
      for (int y = top_left_needed_cu_y; y < top_left_needed_cu_y + 64 * 2; y += 64) {
        cu_loc_t cuLoc;
        uvg_cu_loc_ctor(&cuLoc, x, y, 64, 64);
        walk_tree(stat_array, &cuLoc, 0, width, height, funcs, data);
      }
    }
    zoomOverlayTexture.display();

    sf::Texture zoomTexture;
    zoomTexture.loadFromImage(zoomImage);
    sf::Sprite zoomSprite(zoomTexture);
    zoomSprite.setPosition(mousePosition.x / scaleX > width / 2 ? 0 : width * scaleX - 64 * 4, 0);
    zoomSprite.setScale(4, 4);
    window.draw(zoomSprite);


    sf::Sprite zoomOverlaySprite(zoomOverlayTexture.getTexture());
    zoomOverlaySprite.setPosition(mousePosition.x / scaleX > width / 2 ? 0 : width * scaleX - 64 * 4, 0);
    zoomOverlaySprite.setTextureRect(sf::IntRect(
            (top_right_x_of_zoom_area - top_left_needed_cu_x) * 4,
            (top_right_y_of_zoom_area - top_left_needed_cu_y) * 4, 64 * 4, 64 * 4));
    window.draw(zoomOverlaySprite);
}


void visualizeInfo(const int width, const int height, sf::RenderTexture &cuEdgeRenderTexture,
                   const sub_image_stats *stat_array, sf::RenderWindow &window, const config &cfg,
                   float &previous_scale, const sf::Color* const colors, const float scaleX) {
    if (!cfg.show_grid && !cfg.show_intra) {
        return;
    }

    if (previous_scale != scaleX) {
        cuEdgeRenderTexture.create(width * scaleX, height * scaleX);
        previous_scale = scaleX;
    }
    std::vector<std::function<void(void *, const cu_loc_t *const, const sub_image_stats *const)> > funcs;
    std::vector<void *> data;
    func_parameters params = {cuEdgeRenderTexture, colors, 0, 0, scaleX};
    if (cfg.show_grid) {
        funcs.emplace_back(draw_cu);
        data.push_back((void *) &params);
    }
    if (cfg.show_intra) {
        funcs.emplace_back(drawIntraModes);
        data.push_back((void *) &params);
    }
    cuEdgeRenderTexture.clear(sf::Color::Transparent);
    for (int y = 0; y < height; y += 64) {
        for (int x = 0; x < width; x += 64) {
            cu_loc_t cuLoc;
            uvg_cu_loc_ctor(&cuLoc, x, y, 64, 64);
            walk_tree(stat_array, &cuLoc, 0, width, height, funcs, data);
        }
    }
    cuEdgeRenderTexture.display();
    sf::Sprite grid_sprite = sf::Sprite(cuEdgeRenderTexture.getTexture());
    window.draw(grid_sprite);
}


int main() {
    static const sf::Color colors[4] = {
            sf::Color(245, 24, 245),
            sf::Color(255, 255, 5),
            sf::Color(45, 255, 90),
            sf::Color(255, 255, 255)
    };

    // Create a blank image
    sf::RenderTexture imageTexture;
    const int width = 1280;
    const int height = 720;
    imageTexture.create(width, height);
    imageTexture.clear();

    sf::RenderTexture cuEdgeRenderTexture;
    cuEdgeRenderTexture.create(width, height);

    void*context = zmq_ctx_new();
    void *control_socket = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind(control_socket, "tcp://*:5555");
    for (int i = 0; i < 10; i++) {
        // sleep(1);
        // zmq_send(control_socket, "A", 1, ZMQ_SNDMORE);
        zmq_send(control_socket, "Hello", 5, 0);
    }

    void *receiver = zmq_socket(context, ZMQ_SUB);
    rc = zmq_bind(receiver, "tcp://*:5556");
    if(rc != 0) {
        std::cout << "Error binding to port 5556" << std::endl;
        return 1;
    }
    rc = zmq_setsockopt(receiver, ZMQ_SUBSCRIBE, "", 0);
    if(rc != 0) {
        std::cout << "Error setting socket options" << std::endl;
        return 1;
    }
    // zmq_recv(receiver, NULL, 0, 0);

    sub_image_stats *stat_array = new sub_image_stats[(width / 4) * (height / 4)];

    sf::RenderTexture zoomOverlayTexture;
    zoomOverlayTexture.create(64 * 4 + 4 * 64, 64 * 4 + 4 * 64);

    std::ifstream data_file("data4.dat", std::ios::binary);
    // check if the file is open
    if (!data_file.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        return 1;
    }

    sf::Font font;
    if (!font.loadFromFile(
            "Ubuntu-M.ttf")) {
        // Error handling: If the font fails to load, exit the program
        return EXIT_FAILURE;
    }

    sf::Text debugText;
    debugText.setFont(font);
    debugText.setCharacterSize(24);
    debugText.setFillColor(sf::Color::Magenta);
    debugText.setPosition(20, 20);

    sf::Text helpText;
    helpText.setFont(font);
    helpText.setCharacterSize(24);
    helpText.setFillColor(sf::Color::White);
    helpText.setOutlineColor(sf::Color::Black);
    helpText.setOutlineThickness(2);
    helpText.setPosition(20, 20);
    helpText.setString("F: Toggle fullscreen\n"
                       "H: Toggle help (this)\n"
                       "G: Toggle cu grid\n"
                       "I: Toggle intra modes\n"
                       "Z: Toggle zoom window\n"
                       "(Shift)+S: Toggle MRL\n"
                       "(Shift)+M: Toggle MIP\n"
                       "(Shift)+P: Toggle ISP\n"
                       "(Shift)+T: Toggle MTS\n"
                       "(Shift)+L: Toggle LFNST\n"
                       "CTRL+0-3: Toggle MTT depth\n"
                       "1-8: Toggle encoding speed\n"
                       "D: Toggle debug\n"
                       "ESC: Exit\n");

    // Create a window
    sf::RenderWindow window(sf::VideoMode(width, height), "VVC Visualizer");

    sf::Vector2i previous_mouse_position = sf::Vector2i(-1, -1);

    sf::Image zoomImage;
    zoomImage.create(64, 64, sf::Color::Transparent);

    TimeStamp ts;

    EventHandler eventHandler(width, height, control_socket);

    int64_t timestamp = 0;
    // Draw and display the line in each frame
    sub_image current_cu;
    current_cu.stats.timestamp = 0;
    config cfg;
    float previous_scale = 1;
    while (cfg.running) {
        if (data_file.eof() || !data_file.good()) {
            break;
        }
        // Read one CU from the data file
        int64_t temp_timestamp = current_cu.stats.timestamp;

        uint64_t render_start_timestamp;
        GET_TIME(ts, render_start_timestamp);

        sf::Image newImage;
        newImage.create(width, height, sf::Color::Transparent);
        while ((current_cu.stats.timestamp - 33'000'000) - timestamp < 33'000'000) {
            current_cu = readOneCU(receiver);
            if (data_file.eof() || !data_file.good() || current_cu.stats.width == 0 || current_cu.stats.height == 0) {
                break;
            }
            temp_timestamp = current_cu.stats.timestamp;
            // zmq_recv(receiver, &temp_timestamp, 8, 0);

            for (int y = current_cu.rect.top; y < current_cu.rect.top + current_cu.rect.height - 1; y += 4) {
                for (int x = current_cu.rect.left; x < current_cu.rect.left + current_cu.rect.width - 1; x += 4) {
                    int index = (y / 4) * (width / 4) + (x / 4);
                    memcpy(&stat_array[index], &current_cu.stats, sizeof(current_cu.stats));
                    break;
                }
                break;
            }

            sf::Image cuImage;
            cuImage.create(current_cu.stats.width, current_cu.stats.height, current_cu.image);

            newImage.copy(cuImage, current_cu.stats.x, current_cu.stats.y);
        }

        uint64_t data_process_end_timestamp;
        GET_TIME(ts, data_process_end_timestamp);

        sf::Texture newTexture;
        newTexture.loadFromImage(newImage);
        sf::Sprite newSprite(newTexture);
        imageTexture.draw(newSprite);

        timestamp = temp_timestamp;
        // Get the position of the cursor relative to the window
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        // Get the current size of the window
        sf::Vector2u windowSize = window.getSize();

        // Calculate the scale factors for the sprite
        float scaleX = cfg.fullscreen ? 2 : sqrt(static_cast<float>(windowSize.x) / imageTexture.getSize().x);
        float scaleY = cfg.fullscreen ? 2 : sqrt(static_cast<float>(windowSize.y) / imageTexture.getSize().y);


        // Display the frame
        imageTexture.display();
        sf::Sprite sprite(imageTexture.getTexture());

        // Set the scale of the sprite
        sprite.setScale(scaleX, scaleY);

        window.draw(sprite);
        visualizeInfo(width, height, cuEdgeRenderTexture, stat_array, window, cfg, previous_scale,
                       colors, scaleX);

        if (cfg.show_zoom) {
            drawZoomWindow(colors, imageTexture, width, height, stat_array, zoomOverlayTexture, window,
                                    previous_mouse_position,
                                    zoomImage, mousePosition, scaleX, scaleY);


        }

        uint64_t render_end_time_stamp;
        GET_TIME(ts, render_end_time_stamp);

        if (cfg.show_debug) {
            std::string text_string =
                    std::to_string((data_process_end_timestamp - render_start_timestamp) / 1000000) + " ms\n";
            text_string += std::to_string((render_end_time_stamp - data_process_end_timestamp) / 1000000) + " ms\n";
            text_string += std::to_string((render_end_time_stamp - render_start_timestamp) / 1000000) + " ms\n";
            debugText.setString(text_string);
            window.draw(debugText);
        }

        if (cfg.show_help) {
            window.draw(helpText);
        }
        window.display();

        // Toggle fullscreen on 'f' key press

        sf::Event event;
        while (window.pollEvent(event)) {
            eventHandler.handle(event, cfg, window);
        }
    }

    zmq_close(control_socket);
    zmq_close(receiver);
    zmq_ctx_destroy(context);

    // Close the window after the loop
    window.close();

    return 0;
}
