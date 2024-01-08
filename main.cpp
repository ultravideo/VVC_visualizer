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
    // Skip the data
    data_file.seekg(9, std::ios::cur);
    if (data_file.eof() || !data_file.good()) {
        cu.stats.width = 0;
        cu.stats.height = 0;
        return cu;
    }
    cu.rect = cv::Rect(cu.stats.x, cu.stats.y, cu.stats.width, cu.stats.height);
    uint8_t *yuv420 = new uint8_t[cu.stats.width * cu.stats.height * 3 / 2];
    data_file.read(reinterpret_cast<char *>(yuv420), cu.stats.width * cu.stats.height * 3 / 2);
    cv::Mat yuvMat(cu.stats.height + cu.stats.height / 2, cu.stats.width, CV_8UC1, const_cast<unsigned char*>(yuv420));
    cu.image = cv::Mat::zeros(cu.stats.height, cu.stats.width, CV_8UC3);
    cv::cvtColor(yuvMat, cu.image, cv::COLOR_YUV2RGBA_I420);
    delete[] yuv420;
    return cu;
}

int main() {
    // RBG -> BGR
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
    while (running) {
        if(data_file.eof() || !data_file.good()) {
            break;
        }
        // Read one CU from the data file
        uint64_t temp_timestamp = current_cu.stats.timestamp;
        while (current_cu.stats.timestamp - timestamp < 33'000'000) {
            current_cu = readOneCU(data_file);
            if (data_file.eof() || !data_file.good() || current_cu.stats.width == 0 || current_cu.stats.height == 0 ) {
                break;
            }
            temp_timestamp = current_cu.stats.timestamp;

            sf::Image cuImage;
            cuImage.create(current_cu.stats.width, current_cu.stats.height, current_cu.image.data);

            // Copy the CU into the RenderTexture
            sf::Texture cuTexture;
            cuTexture.loadFromImage(cuImage);
            sf::Sprite cuSprite(cuTexture);
            cuSprite.setPosition(current_cu.rect.x, current_cu.rect.y);
            imageTexture.draw(cuSprite);

            // Draw the lines on the RenderTexture
            sf::Vertex line[] = {
                    sf::Vertex(sf::Vector2f(current_cu.rect.x + current_cu.rect.width - 1, current_cu.rect.y), colors[current_cu.stats.frame_num % 4]),
                    sf::Vertex(sf::Vector2f(current_cu.rect.x + current_cu.rect.width, current_cu.rect.y + current_cu.rect.height - 1), colors[current_cu.stats.frame_num % 4]),
                    sf::Vertex(sf::Vector2f(current_cu.rect.x, current_cu.rect.y + current_cu.rect.height - 1), colors[current_cu.stats.frame_num % 4]),
                    sf::Vertex(sf::Vector2f(current_cu.rect.x + current_cu.rect.width, current_cu.rect.y + current_cu.rect.height - 1), colors[current_cu.stats.frame_num % 4])
            };
            imageTexture.draw(line, 4, sf::Lines);
        }

        timestamp = temp_timestamp;

        // Get the current size of the window
        sf::Vector2u windowSize = window.getSize();

        // Calculate the scale factors for the sprite
        float scaleX = fullscreen ? 2 : imageTexture.getSize().x / static_cast<float>(windowSize.x);
        float scaleY = fullscreen ? 2 : imageTexture.getSize().y / static_cast<float>(windowSize.y);


        // Display the frame
        imageTexture.display();
        sf::Sprite sprite(imageTexture.getTexture());

        // Set the scale of the sprite
        sprite.setScale(scaleX, scaleY);

        window.draw(sprite);
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
            }
        }
    }

    // Close the window after the loop
    window.close();

    return 0;
}