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

#ifndef ANDROID_GUI_BUFFERQUEUE_H
#define ANDROID_GUI_BUFFERQUEUE_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gui/IGraphicBufferAlloc.h>
#include <gui/ISurfaceTexture.h>

#include <ui/Fence.h>
#include <ui/GraphicBuffer.h>

#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/threads.h>

namespace android {
// ----------------------------------------------------------------------------

class BufferQueue : public BnSurfaceTexture {
public:
    enum { MIN_UNDEQUEUED_BUFFERS = 2 };
    enum { NUM_BUFFER_SLOTS = 32 };
    enum { NO_CONNECTED_API = 0 };
    enum { INVALID_BUFFER_SLOT = -1 };
    enum { STALE_BUFFER_SLOT = 1, NO_BUFFER_AVAILABLE };

    // When in async mode we reserve two slots in order to guarantee that the
    // producer and consumer can run asynchronously.
    enum { MAX_MAX_ACQUIRED_BUFFERS = NUM_BUFFER_SLOTS - 2 };

    // ConsumerListener is the interface through which the BufferQueue notifies
    // the consumer of events that the consumer may wish to react to.  Because
    // the consumer will generally have a mutex that is locked during calls from
    // teh consumer to the BufferQueue, these calls from the BufferQueue to the
    // consumer *MUST* be called only when the BufferQueue mutex is NOT locked.
    struct ConsumerListener : public virtual RefBase {
        // onFrameAvailable is called from queueBuffer each time an additional
        // frame becomes available for consumption. This means that frames that
        // are queued while in asynchronous mode only trigger the callback if no
        // previous frames are pending. Frames queued while in synchronous mode
        // always trigger the callback.
        //
        // This is called without any lock held and can be called concurrently
        // by multiple threads.
        virtual void onFrameAvailable() = 0;

        // onBuffersReleased is called to notify the buffer consumer that the
        // BufferQueue has released its references to one or more GraphicBuffers
        // contained in its slots.  The buffer consumer should then call
        // BufferQueue::getReleasedBuffers to retrieve the list of buffers
        //
        // This is called without any lock held and can be called concurrently
        // by multiple threads.
        virtual void onBuffersReleased() = 0;
    };

    // ProxyConsumerListener is a ConsumerListener implementation that keeps a weak
    // reference to the actual consumer object.  It forwards all calls to that
    // consumer object so long as it exists.
    //
    // This class exists to avoid having a circular reference between the
    // BufferQueue object and the consumer object.  The reason this can't be a weak
    // reference in the BufferQueue class is because we're planning to expose the
    // consumer side of a BufferQueue as a binder interface, which doesn't support
    // weak references.
    class ProxyConsumerListener : public BufferQueue::ConsumerListener {
    public:

        ProxyConsumerListener(const wp<BufferQueue::ConsumerListener>& consumerListener);
        virtual ~ProxyConsumerListener();
        virtual void onFrameAvailable();
        virtual void onBuffersReleased();

    private:

        // mConsumerListener is a weak reference to the ConsumerListener.  This is
        // the raison d'etre of ProxyConsumerListener.
        wp<BufferQueue::ConsumerListener> mConsumerListener;
    };


    // BufferQueue manages a pool of gralloc memory slots to be used by
    // producers and consumers. allowSynchronousMode specifies whether or not
    // synchronous mode can be enabled by the producer. allocator is used to
    // allocate all the needed gralloc buffers.
    BufferQueue(bool allowSynchronousMode = true,
            const sp<IGraphicBufferAlloc>& allocator = NULL);
    virtual ~BufferQueue();

    virtual int query(int what, int* value);

    // setBufferCount updates the number of available buffer slots.  After
    // calling this all buffer slots are both unallocated and owned by the
    // BufferQueue object (i.e. they are not owned by the client).
    virtual status_t setBufferCount(int bufferCount);

    virtual status_t requestBuffer(int slot, sp<GraphicBuffer>* buf);

    // dequeueBuffer gets the next buffer slot index for the client to use. If a
    // buffer slot is available then that slot index is written to the location
    // pointed to by the buf argument and a status of OK is returned.  If no
    // slot is available then a status of -EBUSY is returned and buf is
    // unmodified.
    //
    // The fence parameter will be updated to hold the fence associated with
    // the buffer. The contents of the buffer must not be overwritten until the
    // fence signals. If the fence is NULL, the buffer may be written
    // immediately.
    //
    // The width and height parameters must be no greater than the minimum of
    // GL_MAX_VIEWPORT_DIMS and GL_MAX_TEXTURE_SIZE (see: glGetIntegerv).
    // An error due to invalid dimensions might not be reported until
    // updateTexImage() is called.
    virtual status_t dequeueBuffer(int *buf, sp<Fence>& fence,
            uint32_t width, uint32_t height, uint32_t format, uint32_t usage);

    // queueBuffer returns a filled buffer to the BufferQueue. In addition, a
    // timestamp must be provided for the buffer. The timestamp is in
    // nanoseconds, and must be monotonically increasing. Its other semantics
    // (zero point, etc) are client-dependent and should be documented by the
    // client.
    virtual status_t queueBuffer(int buf,
            const QueueBufferInput& input, QueueBufferOutput* output);

    virtual void cancelBuffer(int buf, sp<Fence> fence);

    // setSynchronousMode set whether dequeueBuffer is synchronous or
    // asynchronous. In synchronous mode, dequeueBuffer blocks until
    // a buffer is available, the currently bound buffer can be dequeued and
    // queued buffers will be retired in order.
    // The default mode is asynchronous.
    virtual status_t setSynchronousMode(bool enabled);

    // connect attempts to connect a producer client API to the BufferQueue.
    // This must be called before any other ISurfaceTexture methods are called
    // except for getAllocator.
    //
    // This method will fail if the connect was previously called on the
    // BufferQueue and no corresponding disconnect call was made.
    virtual status_t connect(int api, QueueBufferOutput* output);

    // disconnect attempts to disconnect a producer client API from the
    // BufferQueue. Calling this method will cause any subsequent calls to other
    // ISurfaceTexture methods to fail except for getAllocator and connect.
    // Successfully calling connect after this will allow the other methods to
    // succeed again.
    //
    // This method will fail if the the BufferQueue is not currently
    // connected to the specified client API.
    virtual status_t disconnect(int api);

    // dump our state in a String
    virtual void dump(String8& result) const;
    virtual void dump(String8& result, const char* prefix, char* buffer, size_t SIZE) const;

    // public facing structure for BufferSlot
    struct BufferItem {

        BufferItem()
         :
           mTransform(0),
           mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
           mTimestamp(0),
           mFrameNumber(0),
           mBuf(INVALID_BUFFER_SLOT) {
             mCrop.makeInvalid();
         }
        // mGraphicBuffer points to the buffer allocated for this slot or is NULL
        // if no buffer has been allocated.
        sp<GraphicBuffer> mGraphicBuffer;

        // mCrop is the current crop rectangle for this buffer slot.
        Rect mCrop;

        // mTransform is the current transform flags for this buffer slot.
        uint32_t mTransform;

        // mScalingMode is the current scaling mode for this buffer slot.
        uint32_t mScalingMode;

        // mTimestamp is the current timestamp for this buffer slot. This gets
        // to set by queueBuffer each time this slot is queued.
        int64_t mTimestamp;

        // mFrameNumber is the number of the queued frame for this slot.
        uint64_t mFrameNumber;

        // mBuf is the slot index of this buffer
        int mBuf;

        // mFence is a fence that will signal when the buffer is idle.
        sp<Fence> mFence;
    };

    // The following public functions is the consumer facing interface

    // acquireBuffer attempts to acquire ownership of the next pending buffer in
    // the BufferQueue.  If no buffer is pending then it returns -EINVAL.  If a
    // buffer is successfully acquired, the information about the buffer is
    // returned in BufferItem.  If the buffer returned had previously been
    // acquired then the BufferItem::mGraphicBuffer field of buffer is set to
    // NULL and it is assumed that the consumer still holds a reference to the
    // buffer.
    status_t acquireBuffer(BufferItem *buffer);

    // releaseBuffer releases a buffer slot from the consumer back to the
    // BufferQueue pending a fence sync.
    //
    // If releaseBuffer returns STALE_BUFFER_SLOT, then the consumer must free
    // any references to the just-released buffer that it might have, as if it
    // had received a onBuffersReleased() call with a mask set for the released
    // buffer.
    //
    // Note that the dependencies on EGL will be removed once we switch to using
    // the Android HW Sync HAL.
    status_t releaseBuffer(int buf, EGLDisplay display, EGLSyncKHR fence,
            const sp<Fence>& releaseFence);

    // consumerConnect connects a consumer to the BufferQueue.  Only one
    // consumer may be connected, and when that consumer disconnects the
    // BufferQueue is placed into the "abandoned" state, causing most
    // interactions with the BufferQueue by the producer to fail.
    status_t consumerConnect(const sp<ConsumerListener>& consumer);

    // consumerDisconnect disconnects a consumer from the BufferQueue. All
    // buffers will be freed and the BufferQueue is placed in the "abandoned"
    // state, causing most interactions with the BufferQueue by the producer to
    // fail.
    status_t consumerDisconnect();

    // getReleasedBuffers sets the value pointed to by slotMask to a bit mask
    // indicating which buffer slots the have been released by the BufferQueue
    // but have not yet been released by the consumer.
    status_t getReleasedBuffers(uint32_t* slotMask);

    // setDefaultBufferSize is used to set the size of buffers returned by
    // requestBuffers when a with and height of zero is requested.
    status_t setDefaultBufferSize(uint32_t w, uint32_t h);

    // setDefaultBufferCount set the buffer count. If the client has requested
    // a buffer count using setBufferCount, the server-buffer count will
    // take effect once the client sets the count back to zero.
    status_t setDefaultMaxBufferCount(int bufferCount);

    // setMaxAcquiredBufferCount sets the maximum number of buffers that can
    // be acquired by the consumer at one time.  This call will fail if a
    // producer is connected to the BufferQueue.
    status_t setMaxAcquiredBufferCount(int maxAcquiredBuffers);

    // isSynchronousMode returns whether the SurfaceTexture is currently in
    // synchronous mode.
    bool isSynchronousMode() const;

    // setConsumerName sets the name used in logging
    void setConsumerName(const String8& name);

    // setDefaultBufferFormat allows the BufferQueue to create
    // GraphicBuffers of a defaultFormat if no format is specified
    // in dequeueBuffer
    status_t setDefaultBufferFormat(uint32_t defaultFormat);

    // setConsumerUsageBits will turn on additional usage bits for dequeueBuffer
    status_t setConsumerUsageBits(uint32_t usage);

    // setTransformHint bakes in rotation to buffers so overlays can be used
    status_t setTransformHint(uint32_t hint);

private:
    // freeBufferLocked frees the resources (both GraphicBuffer and EGLImage)
    // for the given slot.
    void freeBufferLocked(int index);

    // freeAllBuffersLocked frees the resources (both GraphicBuffer and
    // EGLImage) for all slots.
    void freeAllBuffersLocked();

    // freeAllBuffersExceptHeadLocked frees the resources (both GraphicBuffer
    // and EGLImage) for all slots except the head of mQueue
    void freeAllBuffersExceptHeadLocked();

    // drainQueueLocked drains the buffer queue if we're in synchronous mode
    // returns immediately otherwise. It returns NO_INIT if the BufferQueue
    // became abandoned or disconnected during this call.
    status_t drainQueueLocked();

    // drainQueueAndFreeBuffersLocked drains the buffer queue if we're in
    // synchronous mode and free all buffers. In asynchronous mode, all buffers
    // are freed except the current buffer.
    status_t drainQueueAndFreeBuffersLocked();

    // setDefaultMaxBufferCountLocked sets the maximum number of buffer slots
    // that will be used if the producer does not override the buffer slot
    // count.
    status_t setDefaultMaxBufferCountLocked(int count);

    // getMinBufferCountLocked returns the minimum number of buffers allowed
    // given the current BufferQueue state.
    int getMinMaxBufferCountLocked() const;

    // getMinUndequeuedBufferCountLocked returns the minimum number of buffers
    // that must remain in a state other than DEQUEUED.
    int getMinUndequeuedBufferCountLocked() const;

    // getMaxBufferCountLocked returns the maximum number of buffers that can
    // be allocated at once.  This value depends upon the following member
    // variables:
    //
    //      mSynchronousMode
    //      mMaxAcquiredBufferCount
    //      mDefaultMaxBufferCount
    //      mOverrideMaxBufferCount
    //
    // Any time one of these member variables is changed while a producer is
    // connected, mDequeueCondition must be broadcast.
    int getMaxBufferCountLocked() const;

    struct BufferSlot {

        BufferSlot()
        : mEglDisplay(EGL_NO_DISPLAY),
          mBufferState(BufferSlot::FREE),
          mRequestBufferCalled(false),
          mTransform(0),
          mScalingMode(NATIVE_WINDOW_SCALING_MODE_FREEZE),
          mTimestamp(0),
          mFrameNumber(0),
          mEglFence(EGL_NO_SYNC_KHR),
          mAcquireCalled(false),
          mNeedsCleanupOnRelease(false) {
            mCrop.makeInvalid();
        }

        // mGraphicBuffer points to the buffer allocated for this slot or is NULL
        // if no buffer has been allocated.
        sp<GraphicBuffer> mGraphicBuffer;

        // mEglDisplay is the EGLDisplay used to create mEglImage.
        EGLDisplay mEglDisplay;

        // BufferState represents the different states in which a buffer slot
        // can be.
        enum BufferState {
            // FREE indicates that the buffer is not currently being used and
            // will not be used in the future until it gets dequeued and
            // subsequently queued by the client.
            // aka "owned by BufferQueue, ready to be dequeued"
            FREE = 0,

            // DEQUEUED indicates that the buffer has been dequeued by the
            // client, but has not yet been queued or canceled. The buffer is
            // considered 'owned' by the client, and the server should not use
            // it for anything.
            //
            // Note that when in synchronous-mode (mSynchronousMode == true),
            // the buffer that's currently attached to the texture may be
            // dequeued by the client.  That means that the current buffer can
            // be in either the DEQUEUED or QUEUED state.  In asynchronous mode,
            // however, the current buffer is always in the QUEUED state.
            // aka "owned by producer, ready to be queued"
            DEQUEUED = 1,

            // QUEUED indicates that the buffer has been queued by the client,
            // and has not since been made available for the client to dequeue.
            // Attaching the buffer to the texture does NOT transition the
            // buffer away from the QUEUED state. However, in Synchronous mode
            // the current buffer may be dequeued by the client under some
            // circumstances. See the note about the current buffer in the
            // documentation for DEQUEUED.
            // aka "owned by BufferQueue, ready to be acquired"
            QUEUED = 2,

            // aka "owned by consumer, ready to be released"
            ACQUIRED = 3
        };

        // mBufferState is the current state of this buffer slot.
        BufferState mBufferState;

        // mRequestBufferCalled is used for validating that the client did
        // call requestBuffer() when told to do so. Technically this is not
        // needed but useful for debugging and catching client bugs.
        bool mRequestBufferCalled;

        // mCrop is the current crop rectangle for this buffer slot.
        Rect mCrop;

        // mTransform is the current transform flags for this buffer slot.
        // (example: NATIVE_WINDOW_TRANSFORM_ROT_90)
        uint32_t mTransform;

        // mScalingMode is the current scaling mode for this buffer slot.
        // (example: NATIVE_WINDOW_SCALING_MODE_FREEZE)
        uint32_t mScalingMode;

        // mTimestamp is the current timestamp for this buffer slot. This gets
        // to set by queueBuffer each time this slot is queued.
        int64_t mTimestamp;

        // mFrameNumber is the number of the queued frame for this slot.
        uint64_t mFrameNumber;

        // mEglFence is the EGL sync object that must signal before the buffer
        // associated with this buffer slot may be dequeued. It is initialized
        // to EGL_NO_SYNC_KHR when the buffer is created and (optionally, based
        // on a compile-time option) set to a new sync object in updateTexImage.
        EGLSyncKHR mEglFence;

        // mFence is a fence which will signal when work initiated by the
        // previous owner of the buffer is finished. When the buffer is FREE,
        // the fence indicates when the consumer has finished reading
        // from the buffer, or when the producer has finished writing if it
        // called cancelBuffer after queueing some writes. When the buffer is
        // QUEUED, it indicates when the producer has finished filling the
        // buffer. When the buffer is DEQUEUED or ACQUIRED, the fence has been
        // passed to the consumer or producer along with ownership of the
        // buffer, and mFence is empty.
        sp<Fence> mFence;

        // Indicates whether this buffer has been seen by a consumer yet
        bool mAcquireCalled;

        // Indicates whether this buffer needs to be cleaned up by consumer
        bool mNeedsCleanupOnRelease;
    };

    // mSlots is the array of buffer slots that must be mirrored on the client
    // side. This allows buffer ownership to be transferred between the client
    // and server without sending a GraphicBuffer over binder. The entire array
    // is initialized to NULL at construction time, and buffers are allocated
    // for a slot when requestBuffer is called with that slot's index.
    BufferSlot mSlots[NUM_BUFFER_SLOTS];

    // mDefaultWidth holds the default width of allocated buffers. It is used
    // in requestBuffers() if a width and height of zero is specified.
    uint32_t mDefaultWidth;

    // mDefaultHeight holds the default height of allocated buffers. It is used
    // in requestBuffers() if a width and height of zero is specified.
    uint32_t mDefaultHeight;

    // mMaxAcquiredBufferCount is the number of buffers that the consumer may
    // acquire at one time.  It defaults to 1 and can be changed by the
    // consumer via the setMaxAcquiredBufferCount method, but this may only be
    // done when no producer is connected to the BufferQueue.
    //
    // This value is used to derive the value returned for the
    // MIN_UNDEQUEUED_BUFFERS query by the producer.
    int mMaxAcquiredBufferCount;

    // mDefaultMaxBufferCount is the default limit on the number of buffers
    // that will be allocated at one time.  This default limit is set by the
    // consumer.  The limit (as opposed to the default limit) may be
    // overridden by the producer.
    int mDefaultMaxBufferCount;

    // mOverrideMaxBufferCount is the limit on the number of buffers that will
    // be allocated at one time. This value is set by the image producer by
    // calling setBufferCount. The default is zero, which means the producer
    // doesn't care about the number of buffers in the pool. In that case
    // mDefaultMaxBufferCount is used as the limit.
    int mOverrideMaxBufferCount;

    // mGraphicBufferAlloc is the connection to SurfaceFlinger that is used to
    // allocate new GraphicBuffer objects.
    sp<IGraphicBufferAlloc> mGraphicBufferAlloc;

    // mConsumerListener is used to notify the connected consumer of
    // asynchronous events that it may wish to react to.  It is initially set
    // to NULL and is written by consumerConnect and consumerDisconnect.
    sp<ConsumerListener> mConsumerListener;

    // mSynchronousMode whether we're in synchronous mode or not
    bool mSynchronousMode;

    // mAllowSynchronousMode whether we allow synchronous mode or not
    const bool mAllowSynchronousMode;

    // mConnectedApi indicates the API that is currently connected to this
    // BufferQueue.  It defaults to NO_CONNECTED_API (= 0), and gets updated
    // by the connect and disconnect methods.
    int mConnectedApi;

    // mDequeueCondition condition used for dequeueBuffer in synchronous mode
    mutable Condition mDequeueCondition;

    // mQueue is a FIFO of queued buffers used in synchronous mode
    typedef Vector<int> Fifo;
    Fifo mQueue;

    // mAbandoned indicates that the BufferQueue will no longer be used to
    // consume images buffers pushed to it using the ISurfaceTexture interface.
    // It is initialized to false, and set to true in the abandon method.  A
    // BufferQueue that has been abandoned will return the NO_INIT error from
    // all ISurfaceTexture methods capable of returning an error.
    bool mAbandoned;

    // mName is a string used to identify the BufferQueue in log messages.
    // It is set by the setName method.
    String8 mConsumerName;

    // mMutex is the mutex used to prevent concurrent access to the member
    // variables of BufferQueue objects. It must be locked whenever the
    // member variables are accessed.
    mutable Mutex mMutex;

    // mFrameCounter is the free running counter, incremented for every buffer queued
    // with the surface Texture.
    uint64_t mFrameCounter;

    // mBufferHasBeenQueued is true once a buffer has been queued.  It is reset
    // by changing the buffer count.
    bool mBufferHasBeenQueued;

    // mDefaultBufferFormat can be set so it will override
    // the buffer format when it isn't specified in dequeueBuffer
    uint32_t mDefaultBufferFormat;

    // mConsumerUsageBits contains flags the consumer wants for GraphicBuffers
    uint32_t mConsumerUsageBits;

    // mTransformHint is used to optimize for screen rotations
    uint32_t mTransformHint;
};

// ----------------------------------------------------------------------------
}; // namespace android

#endif // ANDROID_GUI_BUFFERQUEUE_H
