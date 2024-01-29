#include <fstream>
#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>

#include <zmq.h>
#include <SFML/Graphics.hpp>
#include <cstdint>
#include <unordered_set>

#include "cu.h"
#include "util.h"

#define _USE_MATH_DEFINES

#include "math.h"
#include "config.h"
#include "eventHandler.h"
#include "RenderBufferManager.h"

#ifdef _MSC_VER
#include <Windows.h>
#endif

struct func_parameters {
    sf::RenderTexture *edgeImage;
    const sf::Color *const colors;
    uint32_t top_left_x;
    uint32_t top_left_y;
    float scale;
};
struct heatmap_parameters {
    sf::RenderTexture *edgeImage;
    uint32_t top_left_x;
    uint32_t top_left_y;
    float scale;
    float max_value;
    float min_value;
};


struct renderFrameData {
    const sub_image_stats *stat_array;
    sf::Image *newImage;
    std::unordered_set<uint32_t> *modified_ctus;
    float max_values[6];
};

#define MAX_FRAME_COUNT 8
std::atomic_uint32_t frame_in_index = 0;
std::atomic_uint32_t frame_out_index = 0;


void readInput(const int width, const int height,  void *receiver, std::vector<renderFrameData> &renderFrameDataVector) {
    sub_image current_cu;
    current_cu.stats.timestamp = 0;
    int64_t timestamp = 0;
    bool reset = false;
    for(;;) {
        int64_t temp_timestamp = current_cu.stats.timestamp;
        renderFrameData &currentRenderFrameData = renderFrameDataVector.at(frame_out_index);
        if(reset) {
            currentRenderFrameData.modified_ctus->clear();
            currentRenderFrameData.newImage->create(width, height, sf::Color::Transparent);
            memcpy((void *) currentRenderFrameData.stat_array,
                   renderFrameDataVector.at((frame_out_index - 1) & (MAX_FRAME_COUNT - 1)).stat_array,
                   sizeof(sub_image_stats) * (width / 4) * (height / 4));
            memset((void *) currentRenderFrameData.max_values, 0, sizeof(float) * 6);
            for(int i = 0; i < width * height / 16; i++){
                if(currentRenderFrameData.stat_array[i].height == 0) continue;
                currentRenderFrameData.max_values[0] = std::max(currentRenderFrameData.max_values[0], currentRenderFrameData.stat_array[i].cost);
                currentRenderFrameData.max_values[1] = std::max(currentRenderFrameData.max_values[1], currentRenderFrameData.stat_array[i].cost / currentRenderFrameData.stat_array[i].width / currentRenderFrameData.stat_array[i].height);
                currentRenderFrameData.max_values[2] = std::max(currentRenderFrameData.max_values[2], currentRenderFrameData.stat_array[i].dist);
                currentRenderFrameData.max_values[3] = std::max(currentRenderFrameData.max_values[3], currentRenderFrameData.stat_array[i].dist / currentRenderFrameData.stat_array[i].width / currentRenderFrameData.stat_array[i].height);
                currentRenderFrameData.max_values[4] = std::max(currentRenderFrameData.max_values[4], currentRenderFrameData.stat_array[i].bits);
                currentRenderFrameData.max_values[5] = std::max(currentRenderFrameData.max_values[5], currentRenderFrameData.stat_array[i].bits / currentRenderFrameData.stat_array[i].width / currentRenderFrameData.stat_array[i].height);
            }
        }
        reset = true;

        while ((current_cu.stats.timestamp - 33'000'000) - timestamp < 33'000'000) {
            current_cu = readOneCU(receiver);
            temp_timestamp = current_cu.stats.timestamp;

            for (int y = current_cu.rect.top; y < current_cu.rect.top + current_cu.rect.height - 1; y += 4) {
                for (int x = current_cu.rect.left; x < current_cu.rect.left + current_cu.rect.width - 1; x += 4) {
                    int index = (y / 4) * (width / 4) + (x / 4);
                    memcpy((void *) &currentRenderFrameData.stat_array[index], &current_cu.stats, sizeof(current_cu.stats));
                    
                }
                
            }
            currentRenderFrameData.max_values[0] = std::max(currentRenderFrameData.max_values[0], current_cu.stats.cost);
            currentRenderFrameData.max_values[1] = std::max(currentRenderFrameData.max_values[1], current_cu.stats.cost / current_cu.stats.width / current_cu.stats.height);
            currentRenderFrameData.max_values[2] = std::max(currentRenderFrameData.max_values[2], current_cu.stats.dist);
            currentRenderFrameData.max_values[3] = std::max(currentRenderFrameData.max_values[3], current_cu.stats.dist / current_cu.stats.width / current_cu.stats.height);
            currentRenderFrameData.max_values[4] = std::max(currentRenderFrameData.max_values[4], current_cu.stats.bits);
            currentRenderFrameData.max_values[5] = std::max(currentRenderFrameData.max_values[5], current_cu.stats.bits / current_cu.stats.width / current_cu.stats.height);

            currentRenderFrameData.modified_ctus->insert(((current_cu.stats.y / 64) << 16) | (current_cu.stats.x / 64));

            sf::Image cuImage;
            cuImage.create(current_cu.stats.width, current_cu.stats.height, current_cu.image);

            currentRenderFrameData.newImage->copy(cuImage, current_cu.stats.x, current_cu.stats.y);
        }
        timestamp = temp_timestamp;

        frame_out_index.fetch_add(1);
        frame_out_index.fetch_and(MAX_FRAME_COUNT - 1);
        // In case we have the same frame in and out index we just write on the same frame on the next loop
        if(frame_out_index == frame_in_index) {
            frame_out_index.fetch_sub(1);
            frame_out_index.fetch_and(MAX_FRAME_COUNT - 1);
            reset = false;
        }
    }
}

void draw_colormap(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    static const uint8_t r[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,5,9,13,17,21,25,29,33,37,41,45,49,53,57,61,65,69,73,77,81,85,89,93,97,101,105,109,113,117,121,125,129,133,137,141,145,149,153,157,161,165,169,173,177,181,185,189,193,197,201,205,209,213,217,221,225,229,233,237,241,245,249,253,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,251,247,243,239,235,231,227,223,219,215,211,207,203,199,195,191,187,183,179,175,171,167,163,159,155,151,147,143,139,135,131,127};
    static const uint8_t g[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84,88,92,96,100,104,108,112,116,120,124,128,132,136,140,144,148,152,156,160,164,168,172,176,180,184,188,192,196,200,204,208,212,216,220,224,228,232,236,240,244,248,252,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,252,248,244,240,236,232,228,224,220,216,212,208,204,200,196,192,188,184,180,176,172,168,164,160,156,152,148,144,140,136,132,128,124,120,116,112,108,104,100,96,92,88,84,80,76,72,68,64,60,56,52,48,44,40,36,32,28,24,20,16,12,8,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    static const uint8_t b[] = {127,131,135,139,143,147,151,155,159,163,167,171,175,179,183,187,191,195,199,203,207,211,215,219,223,227,231,235,239,243,247,251,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,253,249,245,241,237,233,229,225,221,217,213,209,205,201,197,193,189,185,181,177,173,169,165,161,157,153,149,145,141,137,133,129,125,121,117,113,109,105,101,97,93,89,85,81,77,73,69,65,61,57,53,49,45,41,37,33,29,25,21,17,13,9,5,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    heatmap_parameters *params = (heatmap_parameters *) data;
    sf::RenderTexture *edgeImage = params->edgeImage;

    const float value = current_cu->cost / current_cu->width / current_cu->height;
    const float max_value = params->max_value;
    const float min_value = params->min_value;
    const float range = max_value - min_value;
    const float normalized_value = (value - min_value) / range;
    const uint8_t index = normalized_value * 255;
    const sf::Color color = sf::Color(r[index], g[index], b[index], 128);

    // Fill the area of the CU with the color
    sf::RectangleShape rectangle(sf::Vector2f(current_cu->width * params->scale, current_cu->height * params->scale));
    rectangle.setPosition((current_cu->x - params->top_left_x) * params->scale,
                          (current_cu->y - params->top_left_y) * params->scale);
    rectangle.setFillColor(color);
    edgeImage->draw(rectangle);
}


void draw_cu(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture *edgeImage = params->edgeImage;
    const sf::Color *const colors = params->colors;
    // Draw the lines on the RenderTexture
    int frame_index_modulo = current_cu->frame_num % 4;
    sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(
                    (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale,
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
    edgeImage->draw(line, 4, sf::Lines);

}

void drawIntraModes(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture *edgeImage = params->edgeImage;

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
            if (!vertical_mode) {
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
            edgeImage->draw(line, 2, sf::Lines);
        } else if (!current_cu->is_mip) {
            sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(center.x - 3, center.y - 3), color),
                    current_cu->intra_mode == 0 ?
                    sf::Vertex(sf::Vector2f(center.x + 3, center.y + 3), color) :
                    sf::Vertex(center, color)
            };
            edgeImage->draw(line, 2, sf::Lines);
            sf::Vertex line2[] = {
                    sf::Vertex(sf::Vector2f(center.x - 3, center.y + 3), color),
                    sf::Vertex(sf::Vector2f(center.x + 3, center.y - 3), color),
            };
            edgeImage->draw(line2, 2, sf::Lines);
        } else {
            if (current_cu->mip_transpose) {
                sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y + 2), color),
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y + 2), color),
                };
                edgeImage->draw(line, 2, sf::Lines);
                sf::Vertex line2[] = {
                        sf::Vertex(sf::Vector2f(center.x - 3, center.y - 2), color),
                        sf::Vertex(sf::Vector2f(center.x + 3, center.y - 2), color),
                };
                edgeImage->draw(line2, 2, sf::Lines);
            } else {
                sf::Vertex line[] = {
                        sf::Vertex(sf::Vector2f(center.x - 2, center.y - 4), color),
                        sf::Vertex(sf::Vector2f(center.x - 2, center.y + 2), color),
                };
                edgeImage->draw(line, 2, sf::Lines);
                sf::Vertex line2[] = {
                        sf::Vertex(sf::Vector2f(center.x + 2, center.y - 4), color),
                        sf::Vertex(sf::Vector2f(center.x + 2, center.y + 2), color),
                };
                edgeImage->draw(line2, 2, sf::Lines);
            }
        }
    }
}

void drawTransforms(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture *edgeImage = params->edgeImage;

    if (cuLoc->width * params->scale < 8 || cuLoc->height * params->scale < 8) {
        return;
    }

    if (current_cu->tr_idx == 0 && current_cu->lfnst == 0) {
        return;
    }

    const sf::Vector2f center = sf::Vector2f(
            (cuLoc->x + cuLoc->width / 2 - params->top_left_x) * params->scale,
            (cuLoc->y + cuLoc->height / 2 - params->top_left_y) * params->scale);

    sf::Color color = sf::Color::Red;
    if (current_cu->tr_idx == 2) {
        const sf::Vector2f bottom_right = sf::Vector2f(
                (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale - 2,
                (cuLoc->y + cuLoc->height - params->top_left_y) * params->scale - 2);
        sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(center.x, bottom_right.y), color),
                sf::Vertex(sf::Vector2f(bottom_right.x, bottom_right.y), color),
                sf::Vertex(sf::Vector2f(bottom_right.x, center.y), color),
                sf::Vertex(sf::Vector2f(bottom_right.x, bottom_right.y), color),
        };
        edgeImage->draw(line, 4, sf::Lines);
    } else if (current_cu->tr_idx == 3) {
        const sf::Vector2f top_right = sf::Vector2f(
                (cuLoc->x + cuLoc->width - params->top_left_x) * params->scale - 2,
                (cuLoc->y - params->top_left_y) * params->scale + 2);
        sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(center.x, top_right.y), color),
                sf::Vertex(sf::Vector2f(top_right.x, top_right.y), color),
                sf::Vertex(sf::Vector2f(top_right.x, center.y), color),
                sf::Vertex(sf::Vector2f(top_right.x, top_right.y), color),
        };
        edgeImage->draw(line, 4, sf::Lines);
    } else if (current_cu->tr_idx == 4) {
        const sf::Vector2f bottom_left = sf::Vector2f(
                (cuLoc->x - params->top_left_x) * params->scale + 2,
                (cuLoc->y + cuLoc->height - params->top_left_y) * params->scale - 2);
        sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(center.x, bottom_left.y), color),
                sf::Vertex(sf::Vector2f(bottom_left.x, bottom_left.y), color),
                sf::Vertex(sf::Vector2f(bottom_left.x, center.y), color),
                sf::Vertex(sf::Vector2f(bottom_left.x, bottom_left.y), color),
        };
        edgeImage->draw(line, 4, sf::Lines);
    } else if (current_cu->tr_idx == 5) {
        const sf::Vector2f top_left = sf::Vector2f(
                (cuLoc->x - params->top_left_x) * params->scale + 2,
                (cuLoc->y - params->top_left_y) * params->scale + 2);
        sf::Vertex line[] = {
                sf::Vertex(sf::Vector2f(center.x, top_left.y), color),
                sf::Vertex(sf::Vector2f(top_left.x, top_left.y), color),
                sf::Vertex(sf::Vector2f(top_left.x, center.y), color),
                sf::Vertex(sf::Vector2f(top_left.x, top_left.y), color),
        };
        edgeImage->draw(line, 4, sf::Lines);
    }

}


void drawISP(void *data, const cu_loc_t *const cuLoc, const sub_image_stats *const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture *edgeImage = params->edgeImage;

    if (cuLoc->width * params->scale < 8 || cuLoc->height * params->scale < 8) {
        return;
    }
    if (current_cu->isp == 0) {
        return;
    }

    int num_splits = cuLoc->width * cuLoc->height == 32 ? 2 : 4;

    const sf::Vector2f top_left = sf::Vector2f(
            (cuLoc->x - params->top_left_x) * params->scale,
            (cuLoc->y - params->top_left_y) * params->scale);

    sf::Color color = sf::Color::Blue;
    if (current_cu->isp == 1) {
        int offset = cuLoc->height / num_splits * params->scale;
        sf::Vertex line[6];
        for (int i = 1; i < num_splits; i++) {
            line[(i - 1) * 2] = sf::Vertex(sf::Vector2f(top_left.x, top_left.y + offset * i), color);
            line[(i - 1) * 2 + 1] = sf::Vertex(
                    sf::Vector2f(top_left.x + cuLoc->width * params->scale - 1, top_left.y + offset * i),
                    color);
        }
        edgeImage->draw(line, 6, sf::Lines);
    } else if (current_cu->isp == 2) {
        int offset = cuLoc->width / num_splits * params->scale;
        sf::Vertex line[6];
        for (int i = 1; i < num_splits; i++) {
            line[(i - 1) * 2] = sf::Vertex(sf::Vector2f(top_left.x + offset * i, top_left.y), color);
            line[(i - 1) * 2 + 1] = sf::Vertex(
                    sf::Vector2f(top_left.x + offset * i, top_left.y + cuLoc->height * params->scale - 1),
                    color);
        }
        edgeImage->draw(line, 6, sf::Lines);
    }
}


void
drawZoomWindow(const sf::Color *const colors, const sf::RenderTexture &imageTexture, const int width, const int height,
               const sub_image_stats *const stat_array, sf::RenderTexture &zoomOverlayTexture, sf::RenderWindow &window,
               sf::Vector2i &previous_mouse_position, sf::Image &zoomImage, const sf::Vector2i &mousePosition,
               const float scaleX, const float scaleY) {
    int top_right_x_of_zoom_area = clamp(static_cast<int>(mousePosition.x / scaleX - 32), 0, width - 64);
    int top_right_y_of_zoom_area = clamp(static_cast<int>(mousePosition.y / scaleY - 32), 0, height - 64);

    int top_left_needed_cu_x = clamp(floor_div(static_cast<int>(mousePosition.x / scaleX - 32), 64) * 64, 0,
                                     (width / 64 - 2) * 64);
    int top_left_needed_cu_y = clamp(floor_div(static_cast<int>(mousePosition.y / scaleY - 32), 64) * 64, 0,
                                     (height / 64 - 2) * 64);

    if (top_right_x_of_zoom_area != previous_mouse_position.x ||
        top_right_y_of_zoom_area != previous_mouse_position.y) {
        previous_mouse_position.x = top_right_x_of_zoom_area;
        previous_mouse_position.y = top_right_y_of_zoom_area;
        zoomImage.copy(imageTexture.getTexture().copyToImage(), 0, 0,
                       sf::IntRect(
                               top_right_x_of_zoom_area,
                               top_right_y_of_zoom_area,
                               64, 64));

    }
    zoomOverlayTexture.clear(sf::Color::Transparent);

    func_parameters params = {&zoomOverlayTexture, colors,
                              static_cast<uint32_t>(top_left_needed_cu_x),
                              static_cast<uint32_t>(top_left_needed_cu_y), 4};
    std::vector<std::function<void(void *, const cu_loc_t *const, const sub_image_stats *const)> > funcs;
    std::vector<void *> data;
    funcs.emplace_back(draw_cu);
    data.push_back((void *) &params);
    funcs.emplace_back(drawIntraModes);
    data.push_back((void *) &params);

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
                   float &previous_scale, const sf::Color *const colors, const float scaleX,
                   RenderBufferManager &renderBufferManager, const std::unordered_set<uint32_t> * const modified_ctus,
                   bool setting_changed, const float *max_value) {
    if (!cfg.show_grid && !cfg.show_intra && !cfg.show_transform && !cfg.show_isp) {
        return;
    }

    if (previous_scale != scaleX) {
        cuEdgeRenderTexture.create(width * scaleX, height * scaleX);
        previous_scale = scaleX;
    }
    std::vector<std::function<void(void *, const cu_loc_t *const, const sub_image_stats *const)> > funcs;
    std::vector<void *> data;
    func_parameters params = {&cuEdgeRenderTexture, colors, 0, 0, scaleX};
    heatmap_parameters heatmap_params = {&cuEdgeRenderTexture, 0, 0, scaleX, max_value[1], 0};
    if (cfg.show_heatmap) {
        funcs.emplace_back(draw_colormap);
        data.push_back((void *) &heatmap_params);
    }
    if (cfg.show_grid) {
        funcs.emplace_back(draw_cu);
        data.push_back((void *) &params);
    }
    if (cfg.show_isp) {
        funcs.emplace_back(drawISP);
        data.push_back((void *) &params);
    }
    if (cfg.show_transform) {
        funcs.emplace_back(drawTransforms);
        data.push_back((void *) &params);
    }
    if (cfg.show_intra) {
        funcs.emplace_back(drawIntraModes);
        data.push_back((void *) &params);
    }
    // cuEdgeRenderTexture.clear(sf::Color::Transparent);
    if (!cfg.paused) {
        if (!setting_changed) {
            for (uint32_t tempx: *modified_ctus) {
                uint32_t x = tempx & 0xFFFFu;
                uint32_t y = tempx >> 16u;
                cu_loc_t cuLoc;
                params.top_left_x = x * 64;
                params.top_left_y = y * 64;
                params.edgeImage = renderBufferManager.get_buffer(x * 64, y * 64);
                heatmap_params.edgeImage = params.edgeImage;
                heatmap_params.top_left_x = params.top_left_x;
                heatmap_params.top_left_y = params.top_left_y;
                uvg_cu_loc_ctor(&cuLoc, x * 64, y * 64, 64, 64);
                walk_tree(stat_array, &cuLoc, 0, width, height, funcs, data);
            }

            const sf::BlendMode blendMode = sf::BlendMode(sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero,
                                                          sf::BlendMode::Equation::Add,
                                                          sf::BlendMode::Factor::One, sf::BlendMode::Factor::Zero,
                                                          sf::BlendMode::Equation::Add);
            const sf::RenderStates states = sf::RenderStates(blendMode);

            for (auto [x, y, buffer]: renderBufferManager.get_modified_ctus()) {
                buffer->display();
                sf::Sprite grid_sprite = sf::Sprite(buffer->getTexture());
                grid_sprite.setPosition(x * scaleX, y * scaleX);
                cuEdgeRenderTexture.draw(grid_sprite, states);
            }
        } else {
            cuEdgeRenderTexture.clear(sf::Color::Transparent);
            for (int y = 0; y < height; y += 64) {
                for (int x = 0; x < width; x += 64) {
                    cu_loc_t cuLoc;
                    uvg_cu_loc_ctor(&cuLoc, x, y, 64, 64);
                    walk_tree(stat_array, &cuLoc, 0, width, height, funcs, data);
                }
            }
        }
        renderBufferManager.clear();
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

    void *context = zmq_ctx_new();
    void *control_socket = zmq_socket(context, ZMQ_PUB);
    int rc = zmq_bind(control_socket, "tcp://*:5555");
    for (int i = 0; i < 10; i++) {
        // sleep(1);
        // zmq_send(control_socket, "A", 1, ZMQ_SNDMORE);
        zmq_send(control_socket, "Hello", 5, 0);
    }

    void *receiver = zmq_socket(context, ZMQ_SUB);
    rc = zmq_bind(receiver, "tcp://*:5556");
    if (rc != 0) {
        std::cout << "Error binding to port 5556" << std::endl;
        return 1;
    }
    rc = zmq_setsockopt(receiver, ZMQ_SUBSCRIBE, "", 0);
    if (rc != 0) {
        std::cout << "Error setting socket options" << std::endl;
        return 1;
    }
    // zmq_recv(receiver, NULL, 0, 0);

    sf::RenderTexture zoomOverlayTexture;
    zoomOverlayTexture.create(64 * 4 + 4 * 64, 64 * 4 + 4 * 64);


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
    helpText.setString(
            "Visualizer controls:\n"
            "  F: Toggle fullscreen\n"
            "  F1: Toggle help (this)\n"
            "  G: Toggle cu grid\n"
            "  I: Toggle intra modes\n"
            "  W: Toggle transforms\n"
            "  E: Toggle ISP\n"
            "  H: Toggle Heatmap\n"
            "  Space: (Un)pause\n"
            "  Z: Toggle zoom window\n"
            "  D: Toggle debug\n"
            "Encoder controls:\n"
            "  (Shift)+S: Toggle MRL\n"
            "  (Shift)+M: Toggle MIP\n"
            "  (Shift)+P: Toggle ISP\n"
            "  (Shift)+T: Toggle MTS\n"
            "  (Shift)+L: Toggle LFNST\n"
            "  CTRL+0-3: Toggle MTT depth\n"
            "  1-8: Toggle encoding speed\n"
            "ESC: Exit\n");

    // Create a window
    sf::RenderWindow window(sf::VideoMode(width, height), "VVC Visualizer");

    sf::Vector2i previous_mouse_position = sf::Vector2i(-1, -1);

    sf::Image zoomImage;
    zoomImage.create(64, 64, sf::Color::Transparent);

    TimeStamp ts;

    RenderBufferManager renderBufferManager(1);

    EventHandler eventHandler(width, height, control_socket);

    config cfg;
    float previous_scale = 1;
    bool setting_changed = false;

    std::vector<renderFrameData> renderFrameDataVector;
    renderFrameDataVector.reserve(MAX_FRAME_COUNT);
    for (int i = 0; i < MAX_FRAME_COUNT; ++i) {
        renderFrameDataVector.emplace_back();
        renderFrameDataVector.at(i).stat_array = new sub_image_stats[(width / 4) * (height / 4)];
        renderFrameDataVector.at(i).modified_ctus = new std::unordered_set<uint32_t>();
        renderFrameDataVector.at(i).newImage = new sf::Image();
        renderFrameDataVector.at(i).newImage->create(width, height, sf::Color::Transparent);
    }
#ifdef _MSC_VER
    QueryPerformanceFrequency(&Frequency);
#endif

    std::thread reader_thread(
            readInput,
            width,
            height,
            receiver,
            std::ref(renderFrameDataVector));
    //readInput(width, height, receiver, renderFrameDataVector);

    while (cfg.running) {
        while (frame_out_index == frame_in_index) {
            std::chrono::milliseconds timespan(1);
            std::this_thread::sleep_for(timespan);
        }

        renderFrameData &currentFrameData = renderFrameDataVector.at(frame_in_index);

        uint64_t render_start_timestamp;
        GET_TIME(ts, render_start_timestamp);

        // readInput(width, receiver, stat_array, timestamp, newImage, modified_ctus, current_cu, temp_timestamp);

        uint64_t data_process_end_timestamp;
        GET_TIME(ts, data_process_end_timestamp);

        if (!cfg.paused) {
            sf::Texture newTexture;
            newTexture.loadFromImage(*currentFrameData.newImage);
            sf::Sprite newSprite(newTexture);
            imageTexture.draw(newSprite);
            // newImage.create(width, height, sf::Color::Transparent);
        }

        // Get the position of the cursor relative to the window
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        // Get the current size of the window
        sf::Vector2u windowSize = window.getSize();

        // Calculate the scale factors for the sprite
        float scaleX = cfg.fullscreen ? 2 : sqrt(static_cast<float>(windowSize.x) / imageTexture.getSize().x);
        float scaleY = cfg.fullscreen ? 2 : sqrt(static_cast<float>(windowSize.y) / imageTexture.getSize().y);

        if (previous_scale != scaleX) {
            renderBufferManager.changeScale(scaleX);
        }

        // Display the frame
        imageTexture.display();
        sf::Sprite sprite(imageTexture.getTexture());


        // Set the scale of the sprite
        sprite.setScale(scaleX, scaleY);

        window.draw(sprite);
        visualizeInfo(width, height, cuEdgeRenderTexture, currentFrameData.stat_array, window, cfg, previous_scale,
                      colors, scaleX, renderBufferManager, currentFrameData.modified_ctus, setting_changed, currentFrameData.max_values);

        if (cfg.show_zoom) {
            drawZoomWindow(colors, imageTexture, width, height, currentFrameData.stat_array, zoomOverlayTexture, window,
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
        setting_changed = false;
        sf::Event event;
        while (window.pollEvent(event)) {
            setting_changed |= eventHandler.handle(event, cfg, window);
        }
        frame_in_index.fetch_add(1);
        frame_in_index.fetch_and(MAX_FRAME_COUNT - 1);
    }
    reader_thread.join();
    for(int i = 0; i < MAX_FRAME_COUNT; ++i) {
        delete[] renderFrameDataVector.at(i).stat_array;
        delete renderFrameDataVector.at(i).modified_ctus;
        delete renderFrameDataVector.at(i).newImage;
    }
    zmq_close(control_socket);
    zmq_close(receiver);
    zmq_ctx_destroy(context);

    // Close the window after the loop
    window.close();

    return 0;
}
