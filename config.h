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
    bool fullscreen;
    bool show_debug;
    bool show_help;
    bool running;
    bool show_zoom;

    config() {
        show_grid = true;
        show_qp = false;
        show_intra = true;
        show_transform = true;
        fullscreen = false;
        show_debug = false;
        show_help = true;
        running = true;
        show_debug = false;
        show_zoom = false;
    }
};


#endif //VISUALIZER_CONFIG_H
