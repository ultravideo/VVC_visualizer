//
// Created by Jovasa on 9.1.2024.
//

#ifndef VISUALIZER_CU_H
#define VISUALIZER_CU_H

#include <fstream>
#include <opencv2/opencv.hpp>

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

#endif //VISUALIZER_CU_H
