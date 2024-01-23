//
// Created by jovasa on 16.1.2024.
//

#ifndef VISUALIZER_RENDERBUFFERMANAGER_H
#define VISUALIZER_RENDERBUFFERMANAGER_H


#include <SFML/Graphics/RenderTexture.hpp>
#include <cstdint>
#include <map>
#include <memory>
#include <iostream>
#include "util.h"

class RenderBufferManager {
    std::vector< std::unique_ptr< sf::RenderTexture> > renderBuffers_;

    std::map<uint32_t, uint32_t> bufferMap_;
    float scale_ = 1.0f;

public:
    explicit RenderBufferManager(float scale)  {
        for (int i = 0; i < 8; i++) {
            renderBuffers_.emplace_back( new sf::RenderTexture );
            renderBuffers_.back()->create(64 * scale, 64 * scale);
        }

    }

    void changeScale(float scale) {
        scale_ = scale;
        for (int i = 0; i < renderBuffers_.size(); i++) {
            renderBuffers_[i]->create(64 * scale, 64 * scale);
        }
    }

    void clear() {
//        for (int i = 0; i < bufferMap_.size(); i++) {
//            renderBuffers_[i]->clear(sf::Color::Transparent);
//        }
        bufferMap_.clear();
    }

    sf::RenderTexture* get_buffer(uint32_t width, uint32_t height) {
        width /= 64;
        height /= 64;
        uint32_t key = (width << 16u) | height;
        auto it = bufferMap_.find(key);
        if (it != bufferMap_.end()) {
            return renderBuffers_[it->second].get();
        }
        uint32_t index = bufferMap_.size();
        if (index >= renderBuffers_.size()) {
            renderBuffers_.emplace_back( new sf::RenderTexture );
            renderBuffers_.back()->create(64 * scale_, 64 * scale_);
        }
        bufferMap_[key] = index;
        renderBuffers_[index]->clear(sf::Color::Transparent);
        return renderBuffers_[index].get();
    }

    std::vector<std::tuple<uint32_t, uint32_t, sf::RenderTexture*> > get_modified_ctus() {
        std::vector<std::tuple<uint32_t, uint32_t, sf::RenderTexture*> > sizes;
        for (auto &it : bufferMap_) {
            sizes.emplace_back((it.first >> 16u) * 64, (it.first & 0xFFFFu) * 64, renderBuffers_[it.second].get());
        }
        return sizes;
    }

    RenderBufferManager(const RenderBufferManager &) = delete;
    RenderBufferManager &operator=(const RenderBufferManager &) = delete;
};


#endif //VISUALIZER_RENDERBUFFERMANAGER_H
