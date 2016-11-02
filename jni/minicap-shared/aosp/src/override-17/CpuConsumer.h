/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_GUI_CPUCONSUMER_H
#define ANDROID_GUI_CPUCONSUMER_H

#include <gui/ConsumerBase.h>

#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

#define ANDROID_GRAPHICS_CPUCONSUMER_JNI_ID "mCpuConsumer"

namespace android {

/**
 * CpuConsumer is a BufferQueue consumer endpoint that allows direct CPU
 * access to the underlying gralloc buffers provided by BufferQueue. Multiple
 * buffers may be acquired by it at once, to be used concurrently by the
 * CpuConsumer owner. Sets gralloc usage flags to be software-read-only.
 * This queue is synchronous by default.
 */

class CpuConsumer: public ConsumerBase
{
  public:
    typedef ConsumerBase::FrameAvailableListener FrameAvailableListener;

    struct LockedBuffer {
        uint8_t    *data;
        uint32_t    width;
        uint32_t    height;
        PixelFormat format;
        uint32_t    stride;
        Rect        crop;
        uint32_t    transform;
        uint32_t    scalingMode;
        int64_t     timestamp;
        uint64_t    frameNumber;
    };

    // Create a new CPU consumer. The maxLockedBuffers parameter specifies
    // how many buffers can be locked for user access at the same time.
    CpuConsumer(uint32_t maxLockedBuffers);

    virtual ~CpuConsumer();

    // set the name of the CpuConsumer that will be used to identify it in
    // log messages.
    void setName(const String8& name);

    // Gets the next graphics buffer from the producer and locks it for CPU use,
    // filling out the passed-in locked buffer structure with the native pointer
    // and metadata. Returns BAD_VALUE if no new buffer is available, and
    // INVALID_OPERATION if the maximum number of buffers is already locked.
    //
    // Only a fixed number of buffers can be locked at a time, determined by the
    // construction-time maxLockedBuffers parameter. If INVALID_OPERATION is
    // returned by lockNextBuffer, then old buffers must be returned to the queue
    // by calling unlockBuffer before more buffers can be acquired.
    status_t lockNextBuffer(LockedBuffer *nativeBuffer);

    // Returns a locked buffer to the queue, allowing it to be reused. Since
    // only a fixed number of buffers may be locked at a time, old buffers must
    // be released by calling unlockBuffer to ensure new buffers can be acquired by
    // lockNextBuffer.
    status_t unlockBuffer(const LockedBuffer &nativeBuffer);

    sp<ISurfaceTexture> getProducerInterface() const { return getBufferQueue(); }

  private:
    // Maximum number of buffers that can be locked at a time
    uint32_t mMaxLockedBuffers;

    virtual void freeBufferLocked(int slotIndex);

    // Array for tracking pointers passed to the consumer, matching the
    // mSlots indexing
    void *mBufferPointers[BufferQueue::NUM_BUFFER_SLOTS];
    // Count of currently locked buffers
    uint32_t mCurrentLockedBuffers;

};

} // namespace android

#endif // ANDROID_GUI_CPUCONSUMER_H
