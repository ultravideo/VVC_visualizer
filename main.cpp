#include <opencv2/opencv.hpp>
#include <fstream>


#pragma pack(push, 1)
struct sub_image_stats {
    uint64_t timestamp;
    uint16_t x;
    uint16_t y;
    uint8_t width;
    uint8_t height;
};
#pragma pack(pop)

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
    cu.rect = cv::Rect(cu.stats.x, cu.stats.y, cu.stats.width, cu.stats.height);
    uint8_t *yuv420 = new uint8_t[cu.stats.width * cu.stats.height * 3 / 2];
    data_file.read(reinterpret_cast<char *>(yuv420), cu.stats.width * cu.stats.height * 3 / 2);
    cv::Mat yuvMat(cu.stats.height + cu.stats.height / 2, cu.stats.width, CV_8UC1, const_cast<unsigned char*>(yuv420));
    cu.image = cv::Mat::zeros(cu.stats.height, cu.stats.width, CV_8UC3);
    cv::cvtColor(yuvMat, cu.image, cv::COLOR_YUV2BGR_I420);
    delete[] yuv420;
    return cu;
}


int main() {
    // Create a blank image
    cv::Mat image = cv::Mat::zeros(720, 1280, CV_8UC3);

    std::ifstream data_file("data4.dat", std::ios::binary);
    // check if the file is open
    if (!data_file.is_open()) {
        std::cout << "Could not open the file" << std::endl;
        return 1;
    }

    // Create a window with the FULLSCREEN flag
    cv::namedWindow("Moving Line", cv::WINDOW_NORMAL | cv::WINDOW_FULLSCREEN);
    uint64_t timestamp = 0;
    // Draw and display the line in each frame
    bool fullscreen = true;
    for (int i = 0;; ++i) {
        if(data_file.eof() || !data_file.good()) {
            break;
        }
        // Read one CU from the data file
        uint64_t temp_timestamp = 0;
        do {
            sub_image cu = readOneCU(data_file);
            if (data_file.eof() || !data_file.good()) {
                break;
            }
            temp_timestamp = cu.stats.timestamp;

            // Copy the CU into the ROI of the image
            cu.image.copyTo(image(cu.rect));

            cv::Point top_right(cu.rect.x + cu.rect.width - 1, cu.rect.y - 1);
            cv::Point bottom_left(cu.rect.x - 1, cu.rect.y + cu.rect.height - 1);
            cv::Point bottom_right(cu.rect.x + cu.rect.width - 1, cu.rect.y + cu.rect.height - 1);

            cv::line(image, top_right, bottom_right, cv::Scalar(245, 24, 245), 1);
            cv::line(image, bottom_right, bottom_left, cv::Scalar(245, 24, 245), 1);

        } while (temp_timestamp - timestamp < 100000000 );
        timestamp = temp_timestamp;

        // Display the frame
        cv::imshow("Moving Line", image);

        // Toggle fullscreen on 'f' key press
        int key = cv::waitKey(1);
        if (key == 'f' || key == 'F') {
            if (fullscreen) {
                cv::setWindowProperty("Moving Line", cv::WND_PROP_FULLSCREEN, cv::WINDOW_NORMAL);
            } else {
                cv::setWindowProperty("Moving Line", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
            }
            fullscreen = !fullscreen;
        }
        // Break the loop and end the program if the user closes the window
        if (cv::getWindowProperty("Moving Line", cv::WND_PROP_VISIBLE) < 1) {
            break;
        }
    }

    // Close the window after the loop
    cv::destroyAllWindows();

    return 0;
}
