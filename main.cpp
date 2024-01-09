#include <opencv2/opencv.hpp>
#include <SFML/Graphics.hpp>
#include <fstream>


struct sub_image_stats {
    uint64_t timestamp;
    uint8_t frame_num;
    uint16_t x;
    uint16_t y;
    uint8_t width;
    uint8_t height;

    uint32_t split_tree;

    uint8_t qp;

    uint8_t intra_mode;
    uint8_t is_mip;
    uint8_t mip_transpose;
    uint8_t mrl;
    uint8_t isp;

    uint8_t lfnst;
    uint8_t tr_idx;
} __attribute__((packed));

struct sub_image {
    sub_image_stats stats;
    cv::Rect rect;
    cv::Mat image;
};


sub_image readOneCU(std::ifstream &data_file) {
    sub_image cu;
    // Skip type and timestamp
    char temp_buffer[9];
    data_file.read(temp_buffer, 1);
    // data_file.seekg(9, std::ios::cur);
    data_file.read(reinterpret_cast<char *>(&cu.stats), sizeof(cu.stats));

    if (data_file.eof() || !data_file.good()) {
        cu.stats.width = 0;
        cu.stats.height = 0;
        return cu;
    }
    cu.rect = cv::Rect(cu.stats.x, cu.stats.y, cu.stats.width, cu.stats.height);
    uint8_t *yuv420 = new uint8_t[cu.stats.width * cu.stats.height * 3 / 2];
    data_file.read(reinterpret_cast<char *>(yuv420), cu.stats.width * cu.stats.height * 3 / 2);
    cv::Mat yuvMat(cu.stats.height + cu.stats.height / 2, cu.stats.width, CV_8UC1, const_cast<unsigned char *>(yuv420));
    cu.image = cv::Mat::zeros(cu.stats.height, cu.stats.width, CV_8UC3);
    cv::cvtColor(yuvMat, cu.image, cv::COLOR_YUV2RGBA_I420);
    delete[] yuv420;
    return cu;
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
    imageTexture.create(1280, 720);
    imageTexture.clear();

    sf::Texture cuEdgeTexture;
    cuEdgeTexture.create(1280, 720);

    std::ifstream data_file("data4.dat", std::ios::binary);
    // check if the file is open
    if (!data_file.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        return 1;
    }

    // Create a window
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Moving Line");

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
        sf::RenderTexture cuEdgeRenderTexture;
        sf::Image image = cuEdgeTexture.copyToImage();
        image.flipVertically();

        sf::Image newImage;
        newImage.create(1280, 720, sf::Color::Transparent);
        while (current_cu.stats.timestamp - timestamp < 33'000'000) {
            current_cu = readOneCU(data_file);
            if (data_file.eof() || !data_file.good() || current_cu.stats.width == 0 || current_cu.stats.height == 0) {
                break;
            }
            temp_timestamp = current_cu.stats.timestamp;

            sf::Image cuImage;
            cuImage.create(current_cu.stats.width, current_cu.stats.height, current_cu.image.data);

            newImage.copy(cuImage, current_cu.stats.x, current_cu.stats.y);

            for (int y = current_cu.rect.y; y < current_cu.rect.y + current_cu.rect.height - 1; y++) {
                for (int x = current_cu.rect.x; x < current_cu.rect.x + current_cu.rect.width - 1; x++) {
                    image.setPixel(x, y, sf::Color::Transparent);
                }
            }

            // Draw the lines to the image
            for (int x = current_cu.rect.x + 1; x < current_cu.rect.x + current_cu.rect.width; x++) {
                image.setPixel(x - 1, current_cu.rect.y + current_cu.rect.height - 1,
                               colors[current_cu.stats.frame_num % 4]);
            }
            for (int y = current_cu.rect.y; y < current_cu.rect.y + current_cu.rect.height; y++) {
                image.setPixel(current_cu.rect.x + current_cu.rect.width - 1, y,
                               colors[current_cu.stats.frame_num % 4]);
            }

        }
        image.flipVertically();
        cuEdgeTexture.update(image);

        sf::Texture newTexture;
        newTexture.loadFromImage(newImage);
        sf::Sprite newSprite(newTexture);
        imageTexture.draw(newSprite);

        {
            cuEdgeRenderTexture.create(1280, 720);
            sf::Sprite sprite(cuEdgeTexture);
            cuEdgeRenderTexture.draw(sprite);
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
                        window.create(sf::VideoMode(1280, 720), "Moving Line", sf::Style::Default);
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