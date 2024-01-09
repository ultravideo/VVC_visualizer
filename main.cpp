#include <fstream>

#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>

#include "cu.h"
#include "util.h"


struct func_parameters {
    sf::RenderTexture &edgeImage;
    const sf::Color * const colors;
};

void draw_cu(void *data, const cu_loc_t *const cuLoc, const sub_image_stats * const current_cu) {
    func_parameters *params = (func_parameters *) data;
    sf::RenderTexture &edgeImage = params->edgeImage;
    const sf::Color * const colors = params->colors;
    // Draw the lines on the RenderTexture
    int frame_index_modulo = current_cu->frame_num % 4;
    sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(cuLoc->x + cuLoc->width - 1, cuLoc->y), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(cuLoc->x + cuLoc->width, cuLoc->y + cuLoc->height - 1), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(cuLoc->x, cuLoc->y + cuLoc->height - 1), colors[frame_index_modulo]),
            sf::Vertex(sf::Vector2f(cuLoc->x + cuLoc->width, cuLoc->y + cuLoc->height - 1), colors[frame_index_modulo])
    };
    edgeImage.draw(line, 4, sf::Lines);
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

    sub_image_stats *stat_array = new sub_image_stats[(width / 4) * (height / 4)];

    sf::Texture cuEdgeTexture;
    cuEdgeTexture.create(width, height);

    std::ifstream data_file("data4.dat", std::ios::binary);
    // check if the file is open
    if (!data_file.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        return 1;
    }

    // Create a window
    sf::RenderWindow window(sf::VideoMode(width, height), "Moving Line");

    uint64_t timestamp = 0;
    // Draw and display the line in each frame
    sub_image current_cu;
    current_cu.stats.timestamp = 0;
    bool fullscreen = false;
    bool running = true;
    bool show_grid = true;
    while (running) {
        if (data_file.eof() || !data_file.good()) {
            break;
        }
        // Read one CU from the data file
        uint64_t temp_timestamp = current_cu.stats.timestamp;

        sf::Image newImage;
        newImage.create(width, height, sf::Color::Transparent);
        while (current_cu.stats.timestamp - timestamp < 33'000'000) {
            current_cu = readOneCU(data_file);
            if (data_file.eof() || !data_file.good() || current_cu.stats.width == 0 || current_cu.stats.height == 0) {
                break;
            }
            temp_timestamp = current_cu.stats.timestamp;

            for (int y = current_cu.rect.y; y < current_cu.rect.y + current_cu.rect.height - 1; y+=4) {
                for (int x = current_cu.rect.x; x < current_cu.rect.x + current_cu.rect.width - 1; x+=4) {
                    int index = (y / 4) * (width / 4) + (x / 4);
                    memcpy(&stat_array[index], &current_cu.stats, sizeof(current_cu.stats));
                }
            }

            sf::Image cuImage;
            cuImage.create(current_cu.stats.width, current_cu.stats.height, current_cu.image.data);

            newImage.copy(cuImage, current_cu.stats.x, current_cu.stats.y);
        }


        sf::Texture newTexture;
        newTexture.loadFromImage(newImage);
        sf::Sprite newSprite(newTexture);
        imageTexture.draw(newSprite);

        {
            cuEdgeRenderTexture.clear(sf::Color::Transparent);
            func_parameters params = {cuEdgeRenderTexture, colors};
            for(int y = 0; y < height; y += 64) {
                for(int x = 0; x < width; x += 64) {
                    cu_loc_t cuLoc;
                    uvg_cu_loc_ctor(&cuLoc, x, y, 64, 64);
                    walk_tree(stat_array, &cuLoc, 0, width, draw_cu, (void *) &params);
                }
            }
        }
        timestamp = temp_timestamp;
        // Get the position of the cursor relative to the window
        sf::Vector2i mousePosition = sf::Mouse::getPosition(window);

        // Get the current size of the window
        sf::Vector2u windowSize = window.getSize();

        // Calculate the scale factors for the sprite
        float scaleX = fullscreen ? 2 : sqrt(static_cast<float>(windowSize.x) / imageTexture.getSize().x);
        float scaleY = fullscreen ? 2 : sqrt(static_cast<float>(windowSize.y) / imageTexture.getSize().y);

//        std::cout << "Mouse X: " << mousePosition.x << ", Y: " << mousePosition.y << std::endl;
//        std::cout << imageTexture.getSize().x << ", " << imageTexture.getSize().y << std::endl;
//        std::cout << windowSize.x << ", " << windowSize.y << std::endl;

        // Display the frame
        imageTexture.display();
        sf::Sprite sprite(imageTexture.getTexture());

        // Set the scale of the sprite
        sprite.setScale(scaleX, scaleY);

        window.draw(sprite);
        if (show_grid) {
            sf::Sprite grid_sprite = sf::Sprite(cuEdgeRenderTexture.getTexture());
            grid_sprite.setScale(scaleX, scaleY);
            window.draw(grid_sprite);
        }

        {
            sf::Image zoomImage;
            zoomImage.create(64, 64, sf::Color::Transparent);
            zoomImage.copy(imageTexture.getTexture().copyToImage(), 0,0,
                           sf::IntRect(
                                   clamp(static_cast<int>(mousePosition.x / scaleX - 32), 0, width - 64),
                                      clamp(static_cast<int>(mousePosition.y / scaleY - 32), 0, height - 64),
                                   64, 64));

            sf::Texture zoomTexture;
            zoomTexture.loadFromImage(zoomImage);
            sf::Sprite zoomSprite(zoomTexture);
            zoomSprite.setPosition(mousePosition.x / scaleX > width / 2 ? 0 : width - 64 * 4, 0);
            zoomSprite.setScale(4, 4);
            window.draw(zoomSprite);
        }

        window.display();

        // Toggle fullscreen on 'f' key press
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                running = false;
                break;
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::F) {
                    if (fullscreen) {
                        window.create(sf::VideoMode(width, height), "Moving Line", sf::Style::Default);
                    } else {
                        window.create(sf::VideoMode(2560, 1440), "Moving Line", sf::Style::Fullscreen);
                    }
                    fullscreen = !fullscreen;
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                    running = false;
                    break;
                }
                if (event.key.code == sf::Keyboard::G) {
                    show_grid = !show_grid;
                }
            }
        }
    }

    // Close the window after the loop
    window.close();

    return 0;
}