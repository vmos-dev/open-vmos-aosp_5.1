/*
// Copyright (c) 2014 Intel Corporation 
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#ifndef GRAPHIC_BUFFER_H
#define GRAPHIC_BUFFER_H

#include <DataBuffer.h>


namespace android {
namespace intel {

class GraphicBuffer : public DataBuffer {
public:
    enum {
        USAGE_INVALID = 0xffffffff,
    };

public:
    GraphicBuffer(uint32_t handle);
    virtual ~GraphicBuffer() {}

    virtual void resetBuffer(uint32_t handle);

    uint32_t getUsage() const { return mUsage; }
    uint32_t getBpp() const { return mBpp; }

    static bool isProtectedUsage(uint32_t usage);
    static bool isProtectedBuffer(GraphicBuffer *buffer);

    static bool isCompressionUsage(uint32_t usage);
    static bool isCompressionBuffer(GraphicBuffer *buffer);

private:
    void initBuffer(uint32_t handle);

protected:
    uint32_t mUsage;
    uint32_t mBpp;
};

} // namespace intel
} // namespace android

#endif /* GRAPHIC_BUFFER_H */
