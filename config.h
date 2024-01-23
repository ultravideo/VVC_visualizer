//
// Created by jovasa on 15.1.2024.
//

#ifndef VISUALIZER_CONFIG_H
#define VISUALIZER_CONFIG_H

struct config {
    bool show_grid;
    bool show_qp;
    bool show_intra;
    bool show_transform;
    bool show_isp;
    bool fullscreen;
    bool show_debug;
    bool show_help;
    bool running;
    bool show_zoom;
    bool paused;

    config() {
        show_grid = true;
        show_qp = false;
        show_intra = true;
        show_transform = true;
        show_isp = true;
        fullscreen = false;
        show_debug = false;
        show_help = true;
        running = true;
        show_debug = false;
        show_zoom = false;
        paused = false;
    }
};


#endif //VISUALIZER_CONFIG_H
