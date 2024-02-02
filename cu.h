//
// Created by Jovasa on 9.1.2024.
//

#ifndef VISUALIZER_CU_H
#define VISUALIZER_CU_H

#include <fstream>
#include <SFML/Graphics.hpp>
#include <functional>

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif
struct sub_image_stats {
    int64_t timestamp;
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

    float cost;
    float bits;
    float dist;
}
#ifdef _MSC_VER
;
#pragma pack(pop)
#else
__attribute__((packed));
#endif


struct sub_image {
    sub_image_stats stats;
    sf::Rect<uint32_t> rect;
    uint8_t image[64* 64 * 4];
};

sub_image readOneCU(std::ifstream &data_file);
std::vector<sub_image_stats> readOneCU(void* data_file, sf::Rect<uint32_t>& rect_out, uint8_t* image);

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define GET_SPLITDATA(CU, curDepth) ((CU)->split_tree >> ((MAX((curDepth), 0) * 3)) & 7)

enum split_type {
    NO_SPLIT = 0,
    QT_SPLIT = 1,
    BT_HOR_SPLIT = 2,
    BT_VER_SPLIT = 3,
    TT_HOR_SPLIT = 4,
    TT_VER_SPLIT = 5,
};

typedef struct {
    int16_t x;
    int16_t y;
    uint8_t local_x;
    uint8_t local_y;
    int8_t width;
    int8_t height;
    int8_t chroma_width;
    int8_t chroma_height;
} cu_loc_t;

#define LCU_WIDTH 64
void uvg_cu_loc_ctor(cu_loc_t *loc, int x, int y, int width, int height);

void walk_tree(const sub_image_stats * const tree, cu_loc_t const *const cuLoc, uint8_t depth, uint32_t image_width,
               uint32_t image_height, const std::vector<std::function<void(void *, const cu_loc_t *const,
                                                                           const sub_image_stats *const)> > &funcs,
               const std::vector<void *> &data);

#endif //VISUALIZER_CU_H
