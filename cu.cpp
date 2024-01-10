//
// Created by jovasa on 9.1.2024.
//

#include "cu.h"

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

void uvg_cu_loc_ctor(cu_loc_t *loc, int x, int y, int width, int height) {
    assert(x >= 0 && y >= 0 && width >= 0 && height >= 0 && "Cannot give negative coordinates or block dimensions.");
    assert(!(width > LCU_WIDTH || height > LCU_WIDTH) && "Luma CU dimension exceeds maximum (dim > LCU_WIDTH).");
    // This check is no longer valid. With non-square blocks and ISP enabled, even 1x16 and 16x1 (ISP needs at least 16 samples) blocks are valid
    //assert(!(width < 4 || height < 4) && "Luma CU dimension smaller than 4.");

    loc->x = x;
    loc->y = y;
    loc->local_x = x % LCU_WIDTH;
    loc->local_y = y % LCU_WIDTH;
    loc->width = width;
    loc->height = height;

    loc->chroma_width = width >> 1;
    loc->chroma_height = height >> 1;
}


static int get_split_locs(
        const cu_loc_t *const origin,
        enum split_type split,
        cu_loc_t out[4],
        uint8_t *separate_chroma) {
    const int half_width = origin->width >> 1;
    const int half_height = origin->height >> 1;
    const int quarter_width = origin->width >> 2;
    const int quarter_height = origin->height >> 2;
    if (origin->width == 4 && separate_chroma) *separate_chroma = 1;

    switch (split) {
        case NO_SPLIT:
            assert(0 && "trying to get split from no split");
            break;
        case QT_SPLIT:
            uvg_cu_loc_ctor(&out[0], origin->x, origin->y, half_width, half_height);
            uvg_cu_loc_ctor(&out[1], origin->x + half_width, origin->y, half_width, half_height);
            uvg_cu_loc_ctor(&out[2], origin->x, origin->y + half_height, half_width, half_height);
            uvg_cu_loc_ctor(&out[3], origin->x + half_width, origin->y + half_height, half_width, half_height);
            if (half_height == 4 && separate_chroma) *separate_chroma = 1;
            return 4;
        case BT_HOR_SPLIT:
            uvg_cu_loc_ctor(&out[0], origin->x, origin->y, origin->width, half_height);
            uvg_cu_loc_ctor(&out[1], origin->x, origin->y + half_height, origin->width, half_height);
            if (half_height * origin->width < 64 && separate_chroma) *separate_chroma = 1;
            return 2;
        case BT_VER_SPLIT:
            uvg_cu_loc_ctor(&out[0], origin->x, origin->y, half_width, origin->height);
            uvg_cu_loc_ctor(&out[1], origin->x + half_width, origin->y, half_width, origin->height);
            if ((half_width == 4 || half_width * origin->height < 64) && separate_chroma) *separate_chroma = 1;
            return 2;
        case TT_HOR_SPLIT:
            uvg_cu_loc_ctor(&out[0], origin->x, origin->y, origin->width, quarter_height);
            uvg_cu_loc_ctor(&out[1], origin->x, origin->y + quarter_height, origin->width, half_height);
            uvg_cu_loc_ctor(&out[2], origin->x, origin->y + quarter_height + half_height, origin->width,
                            quarter_height);
            if (quarter_height * origin->width < 64 && separate_chroma) *separate_chroma = 1;
            return 3;
        case TT_VER_SPLIT:
            uvg_cu_loc_ctor(&out[0], origin->x, origin->y, quarter_width, origin->height);
            uvg_cu_loc_ctor(&out[1], origin->x + quarter_width, origin->y, half_width, origin->height);
            uvg_cu_loc_ctor(&out[2], origin->x + quarter_width + half_width, origin->y, quarter_width, origin->height);
            if ((quarter_width == 4 || quarter_width * origin->height < 64) && separate_chroma) *separate_chroma = 1;
            return 3;
    }
    return 0;
}


void walk_tree(sub_image_stats *tree, cu_loc_t const *const cuLoc, uint8_t depth, uint32_t image_width,
               uint32_t image_height,
               const std::vector<std::function<void(void *, const cu_loc_t *const, const sub_image_stats *const)> > &funcs,
               const std::vector<void *> &data) {
    int x = cuLoc->x;
    int y = cuLoc->y;
    if (x < 0 || y < 0 || x >= image_width || y >= image_height) {
        return;
    }
    int index = (y / 4) * (image_width / 4) + x / 4;
    const sub_image_stats *current_node = &tree[index];
    if (current_node->width == 0 || current_node->height == 0) {
        return;
    }

    unsigned int split_data = GET_SPLITDATA(current_node, depth);

    if (split_data == NO_SPLIT) {
        for (int i = 0; i < funcs.size(); i++) {
            funcs[i](data[i], cuLoc, current_node);
        }
        return;
    }
    cu_loc_t split_locs[4];
    uint8_t separate_chroma = 0;
    int num_split_locs = get_split_locs(cuLoc, (enum split_type) split_data, split_locs, &separate_chroma);
    for (int i = 0; i < num_split_locs; i++) {
        walk_tree(tree, &split_locs[i], depth + 1, image_width, image_height, funcs, data);
    }
}