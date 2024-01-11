//
// Created by jovasa on 9.1.2024.
//

#include "cu.h"

// Return the next aligned address for *p. Result is at most alignment larger than p.
#define ALIGNED_POINTER(p, alignment) (void*)((intptr_t)(p) + (alignment) - ((intptr_t)(p) % (alignment)))
// 32 bytes is enough for AVX2
#define SIMD_ALIGNMENT 32


//int yuv420_to_rgb_i_avx2(uint8_t* input, uint8_t* output, uint16_t width, uint16_t height)
//{
//    const int mini[8] = { 0,0,0,0,0,0,0,0 };
//    const int middle[8] = { 128, 128, 128, 128,128, 128, 128, 128 };
//    const int maxi[8] = { 255, 255, 255, 255,255, 255, 255, 255 };
//
//    const __m256i min_val = _mm256_loadu_si256((__m256i const*)mini);
//    const __m256i middle_val = _mm256_loadu_si256((__m256i const*)middle);
//    const __m256i max_val = _mm256_loadu_si256((__m256i const*)maxi);
//
//    //*output = (uint8_t *)malloc(width*height*4);
//
//    uint8_t *row_r_temp = (uint8_t *)malloc(width * 4+ SIMD_ALIGNMENT);
//    uint8_t *row_g_temp = (uint8_t *)malloc(width * 4+ SIMD_ALIGNMENT);
//    uint8_t *row_b_temp = (uint8_t *)malloc(width * 4+ SIMD_ALIGNMENT);
//    uint8_t *row_r = (uint8_t*)ALIGNED_POINTER(row_r_temp, SIMD_ALIGNMENT);
//    uint8_t *row_g = (uint8_t*)ALIGNED_POINTER(row_g_temp, SIMD_ALIGNMENT);
//    uint8_t *row_b = (uint8_t*)ALIGNED_POINTER(row_b_temp, SIMD_ALIGNMENT);
//
//
//    uint8_t *in_y = &input[0];
//    uint8_t *in_u = &input[width*height];
//    uint8_t *in_v = &input[width*height + (width*height >> 2)];
//    uint8_t *out = output;
//
//    int8_t row = 0;
//    int32_t pix = 0;
//
//    __m128i luma_shufflemask_lo = _mm_set_epi8(-1, -1, -1, 3, -1, -1, -1, 2, -1, -1, -1, 1, -1, -1, -1, 0);
//    __m128i luma_shufflemask_hi = _mm_set_epi8(-1, -1, -1, 7, -1, -1, -1, 6, -1, -1, -1, 5, -1, -1, -1, 4);
//    __m128i chroma_shufflemask_lo = _mm_set_epi8(-1, -1, -1, 1, -1, -1, -1, 1, -1, -1, -1, 0, -1, -1, -1, 0);
//    __m128i chroma_shufflemask_hi = _mm_set_epi8(-1, -1, -1, 3, -1, -1, -1, 3, -1, -1, -1, 2, -1, -1, -1, 2);
//
//    for (int32_t i = 0; i < width*height; i += 16) {
//
//        // Load 16 bytes (16 luma pixels)
//        __m128i y_a = _mm_loadu_si128((__m128i const*) in_y);
//        in_y += 16;
//
//        __m128i luma_lo = _mm_shuffle_epi8(y_a, luma_shufflemask_lo);
//        __m128i luma_hi = _mm_shuffle_epi8(y_a, luma_shufflemask_hi);
//        __m256i luma_a = _mm256_set_m128i(luma_hi, luma_lo);
//        __m256i chroma_u, chroma_v;
//
//        __m128i u_a  = _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
//        __m128i v_a  = _mm_set_epi8(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0);
//
//        // For every second row
//        if (!row) {
//
//            u_a = _mm_loadl_epi64((__m128i const*) in_u);
//            in_u += 8;
//
//            v_a = _mm_loadl_epi64((__m128i const*) in_v);
//            in_v += 8;
//
//
//            __m128i chroma_u_lo = _mm_shuffle_epi8(u_a, chroma_shufflemask_lo);
//            __m128i chroma_u_hi = _mm_shuffle_epi8(u_a, chroma_shufflemask_hi);
//            chroma_u = _mm256_set_m128i(chroma_u_hi, chroma_u_lo);
//
//            __m128i chroma_v_lo = _mm_shuffle_epi8(v_a, chroma_shufflemask_lo);
//            __m128i chroma_v_hi = _mm_shuffle_epi8(v_a, chroma_shufflemask_hi);
//
//            chroma_v = _mm256_set_m128i(chroma_v_hi, chroma_v_lo);
//
//        }
//        __m256i r_pix_temp, temp_a, temp_b, g_pix_temp, b_pix_temp;
//
//        for (int ii = 0; ii < 2; ii++) {
//
//
//            // We use the same chroma for two rows
//            if (row) {
//                r_pix_temp = _mm256_loadu_si256((__m256i const*)&row_r[pix * 4 + ii * 32]);
//                g_pix_temp = _mm256_loadu_si256((__m256i const*)&row_g[pix * 4 + ii * 32]);
//                b_pix_temp = _mm256_loadu_si256((__m256i const*)&row_b[pix * 4 + ii * 32]);
//            }
//            else {
//                chroma_u = _mm256_sub_epi32(chroma_u, middle_val);
//                chroma_v = _mm256_sub_epi32(chroma_v, middle_val);
//
//                r_pix_temp = _mm256_add_epi32(chroma_v, _mm256_add_epi32(_mm256_srai_epi32(chroma_v, 2), _mm256_add_epi32(_mm256_srai_epi32(chroma_v, 3), _mm256_srai_epi32(chroma_v, 5))));
//                temp_a = _mm256_add_epi32(_mm256_srai_epi32(chroma_u, 2), _mm256_add_epi32(_mm256_srai_epi32(chroma_u, 4), _mm256_srai_epi32(chroma_u, 5)));
//                temp_b = _mm256_add_epi32(_mm256_srai_epi32(chroma_v, 1), _mm256_add_epi32(_mm256_srai_epi32(chroma_v, 3), _mm256_add_epi32(_mm256_srai_epi32(chroma_v, 4), _mm256_srai_epi32(chroma_v, 5))));
//                g_pix_temp = _mm256_add_epi32(temp_a, temp_b);
//                b_pix_temp = _mm256_add_epi32(chroma_u, _mm256_add_epi32(_mm256_srai_epi32(chroma_u, 1), _mm256_add_epi32(_mm256_srai_epi32(chroma_u, 2), _mm256_srai_epi32(chroma_u, 6))));
//
//                // Store results to be used for the next row
//                _mm256_storeu_si256((__m256i*)&row_r[pix * 4 + ii * 32], r_pix_temp);
//                _mm256_storeu_si256((__m256i*)&row_g[pix * 4 + ii * 32], g_pix_temp);
//                _mm256_storeu_si256((__m256i*)&row_b[pix * 4 + ii * 32], b_pix_temp);
//            }
//
//
//            __m256i r_pix = _mm256_slli_epi32(_mm256_max_epi32(min_val, _mm256_min_epi32(max_val, _mm256_add_epi32(luma_a, r_pix_temp))), 16);
//            __m256i g_pix = _mm256_slli_epi32(_mm256_max_epi32(min_val, _mm256_min_epi32(max_val, _mm256_sub_epi32(luma_a, g_pix_temp))), 8);
//            __m256i b_pix = _mm256_max_epi32(min_val, _mm256_min_epi32(max_val, _mm256_add_epi32(luma_a, b_pix_temp)));
//
//
//            __m256i rgb = _mm256_adds_epu8(r_pix, _mm256_adds_epu8(g_pix, b_pix));
//
//            _mm256_storeu_si256((__m256i*)out, rgb);
//            out += 32;
//
//            if (ii != 1) {
//                u_a = _mm_srli_si128(u_a, 4);
//                v_a = _mm_srli_si128(v_a, 4);
//                __m128i chroma_u_lo = _mm_shuffle_epi8(u_a, chroma_shufflemask_lo);
//                __m128i chroma_u_hi = _mm_shuffle_epi8(u_a, chroma_shufflemask_hi);
//                chroma_u = _mm256_set_m128i(chroma_u_hi, chroma_u_lo);
//
//                __m128i chroma_v_lo = _mm_shuffle_epi8(v_a, chroma_shufflemask_lo);
//                __m128i chroma_v_hi = _mm_shuffle_epi8(v_a, chroma_shufflemask_hi);
//
//                chroma_v = _mm256_set_m128i(chroma_v_hi, chroma_v_lo);
//
//
//                y_a = _mm_srli_si128(y_a, 8);
//                __m128i luma_lo = _mm_shuffle_epi8(y_a, luma_shufflemask_lo);
//                __m128i luma_hi = _mm_shuffle_epi8(y_a, luma_shufflemask_hi);
//                luma_a = _mm256_set_m128i(luma_hi, luma_lo);
//            }
//        }
//
//
//        // Track rows for chroma
//        pix += 16;
//        if (pix == width) {
//            row = !row;
//            pix = 0;
//        }
//    }
//
//    free(row_r_temp);
//    free(row_g_temp);
//    free(row_b_temp);
//    return 1;
//}

uint8_t clamp_8bit(int32_t input)
{
    if(input & ~255)
    {
        return (-input) >> 31;
    }
    return input;
}

#define CLAMP_8BIT(input) ((input) & ~255 ? (-input) >> 31 : (input))


void yuv420_to_rgb_i_c(uint8_t* input, uint8_t* output, uint16_t width, uint16_t height)
{
    // Luma pixels
    for(int i = 0; i < width*height; ++i)
    {
        output[i*4] = input[i];
        output[i*4+1] = input[i];
        output[i*4+2] = input[i];
        output[i*4+3] = 255;
    }

    uint32_t u_offset = width*height;
    uint32_t v_offset = width*height + height*width/4;

    for(int y = 0; y < height/2; ++y)
    {
        for(int x = 0; x < width/2; ++x)
        {
            int32_t cr = input[x + y*width/2 + u_offset] - 128;
            int32_t cb = input[x + y*width/2 + v_offset] - 128;

            int32_t rpixel = cr + (cr >> 2) + (cr >> 3) + (cr >> 5);
            int32_t gpixel = - ((cb >> 2) + (cb >> 4) + (cb >> 5)) - ((cr >> 1)+(cr >> 3)+(cr >> 4)+(cr >> 5));
            int32_t bpixel = cb + (cb >> 1)+(cb >> 2)+(cb >> 6);

            int32_t row      = 8*y*width;
            int32_t next_row = row + 4*width;

            // add chroma components to rgb pixels
            // R
            int32_t pixel_value = 0;

            pixel_value                = output[8*x + row ] + rpixel;
            output[8*x + row + 2]          = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x + 4 + row ] + rpixel;
            output[8*x + 4 + row + 2]      = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x +     next_row] + rpixel;
            output[8*x + next_row + 2]     = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x + 4 + next_row] + rpixel;
            output[8*x + 4 + next_row + 2] = CLAMP_8BIT(pixel_value);

            // G
            pixel_value                = output[8*x + row + 1] + gpixel;
            output[8*x + row + 1]      = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x + 4 + row + 1] + gpixel;
            output[8*x + 4 + row + 1]  = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x +  next_row + 1] + gpixel;
            output[8*x + next_row + 1] = CLAMP_8BIT(pixel_value);

            pixel_value                    = output[8*x + 4 + next_row + 1] + gpixel;
            output[8*x + 4 + next_row + 1] = CLAMP_8BIT(pixel_value);

            // B
            pixel_value                = output[8*x + row + 2] + bpixel;
            output[8*x + row + 0]      = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x + 4 + row + 2] + bpixel;
            output[8*x + 4 + row + 0]  = CLAMP_8BIT(pixel_value);

            pixel_value                = output[8*x + next_row + 2] + bpixel;
            output[8*x + next_row + 0] = CLAMP_8BIT(pixel_value);

            pixel_value                    = output[8*x + 4 + next_row + 2] + bpixel;
            output[8*x + 4 + next_row + 0] = CLAMP_8BIT(pixel_value);
        }
    }
}



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
    cu.rect = sf::Rect<uint32_t>(cu.stats.x, cu.stats.y, cu.stats.width, cu.stats.height);
    uint8_t *yuv420 = new uint8_t[cu.stats.width * cu.stats.height * 3 / 2];
    data_file.read(reinterpret_cast<char *>(yuv420), cu.stats.width * cu.stats.height * 3 / 2);
    yuv420_to_rgb_i_c(yuv420, cu.image, cu.stats.width, cu.stats.height);
    delete[] yuv420;
    return cu;
}

void uvg_cu_loc_ctor(cu_loc_t *loc, int x, int y, int width, int height) {
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