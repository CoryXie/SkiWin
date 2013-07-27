#include <android_runtime/AndroidRuntime.h>
#include <utils/Log.h>
#include <utils/Looper.h>
#include <utils/threads.h>
#include <androidfw/InputTransport.h>

#include "SkiWinInputEventSink.h"
#include "MessageQueue.h"

namespace android {

class SkiWinInputEventReceiver : public LooperCallback {
public:
    SkiWinInputEventReceiver(const sp<SkiWinInputEventSink> & receiverObj, 
			     const sp<InputChannel>& inputChannel,
            		     const sp<MessageQueue>& messageQueue);

    status_t initialize();
    void dispose();
    status_t finishInputEvent(uint32_t seq, bool handled);
    status_t consumeEvents(bool consumeBatches, nsecs_t frameTime);

protected:
    virtual ~SkiWinInputEventReceiver();

private:
    sp<SkiWinInputEventSink> mReceiverObjGlobal;
    InputConsumer mInputConsumer;
    sp<MessageQueue> mMessageQueue;
    PreallocatedInputEventFactory mInputEventFactory;
    bool mBatchedInputEventPending;

    const char* getInputChannelName() {
        return mInputConsumer.getChannel()->getName().string();
    }

    virtual int handleEvent(int receiveFd, int events, void* data);
};


SkiWinInputEventReceiver::SkiWinInputEventReceiver(const sp<SkiWinInputEventSink>&   receiverObj, 
			     const sp<InputChannel>& inputChannel,
            		     const sp<MessageQueue>& messageQueue) :
        mReceiverObjGlobal(receiverObj),
        mInputConsumer(inputChannel), 
        mMessageQueue(messageQueue),
        mBatchedInputEventPending(false) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ Initializing input event receiver.", 
    getInputChannelName());
#endif
}

SkiWinInputEventReceiver::~SkiWinInputEventReceiver() {
}

status_t SkiWinInputEventReceiver::initialize() {
    int receiveFd = mInputConsumer.getChannel()->getFd();
    mMessageQueue->getLooper()->addFd(receiveFd, 
								      0, ALOOPER_EVENT_INPUT, this, NULL);
    return OK;
}

void SkiWinInputEventReceiver::dispose() {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ Disposing input event receiver.", 
    getInputChannelName());
#endif

    mMessageQueue->getLooper()->removeFd(mInputConsumer.getChannel()->getFd());
}

status_t SkiWinInputEventReceiver::finishInputEvent(uint32_t seq, bool handled) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ Finished input event.", getInputChannelName());
#endif

    status_t status = mInputConsumer.sendFinishedSignal(seq, handled);
    if (status) {
        ALOGW("Failed to send finished signal on channel '%s'.  status=%d",
                getInputChannelName(), status);
    }
    return status;
}

int SkiWinInputEventReceiver::handleEvent(int receiveFd, int events, void* data) {
    if (events & (ALOOPER_EVENT_ERROR | ALOOPER_EVENT_HANGUP)) {
        ALOGE("channel '%s' ~ Publisher closed input channel or an error occurred.  "
                "events=0x%x", getInputChannelName(), events);
        return 0; // remove the callback
    }

    if (!(events & ALOOPER_EVENT_INPUT)) {
        ALOGW("channel '%s' ~ Received spurious callback for unhandled poll event.  "
                "events=0x%x", getInputChannelName(), events);
        return 1;
    }

    status_t status = consumeEvents(false /*consumeBatches*/, -1);
    return status == OK || status == NO_MEMORY ? 1 : 0;
}

status_t SkiWinInputEventReceiver::consumeEvents(
        bool consumeBatches, nsecs_t frameTime) {
#if DEBUG_DISPATCH_CYCLE
    ALOGD("channel '%s' ~ Consuming input events, consumeBatches=%s, frameTime=%lld.",
            getInputChannelName(), consumeBatches ? "true" : "false", frameTime);
#endif

    if (consumeBatches) {
        mBatchedInputEventPending = false;
    }

    bool skipCallbacks = false;
    for (;;) {
        uint32_t seq;
        InputEvent* inputEvent = NULL;
        status_t status = mInputConsumer.consume(&mInputEventFactory,
                consumeBatches, frameTime, &seq, &inputEvent);
        if (status) {
            if (status == WOULD_BLOCK) {
                if (!skipCallbacks && !mBatchedInputEventPending
                        && mInputConsumer.hasPendingBatch()) {
                    // There is a pending batch.  Come back later.
                    mBatchedInputEventPending = true;
#if DEBUG_DISPATCH_CYCLE
                    ALOGD("channel '%s' ~ Dispatching batched input event pending notification.",
                            getInputChannelName());
#endif
                    mBatchedInputEventPending = 
                    mReceiverObjGlobal->dispatchBatchedInputEventPending();
                }
                return OK;
            }
            ALOGE("channel '%s' ~ Failed to consume input event.  status=%d",
                    getInputChannelName(), status);
            return status;
        }
        assert(inputEvent);

        if (!skipCallbacks) {
            switch (inputEvent->getType()) {
            case AINPUT_EVENT_TYPE_KEY:
#if DEBUG_DISPATCH_CYCLE
                ALOGD("channel '%s' ~ Received key event.", getInputChannelName());
#endif
                skipCallbacks = mReceiverObjGlobal->dispatchInputEvent(seq, 

					    static_cast<KeyEvent*>(inputEvent));

                break;

            case AINPUT_EVENT_TYPE_MOTION:
#if DEBUG_DISPATCH_CYCLE
                ALOGD("channel '%s' ~ Received motion event.", getInputChannelName());
#endif
                skipCallbacks = mReceiverObjGlobal->dispatchInputEvent(seq, 
						static_cast<MotionEvent*>(inputEvent));
                break;

            default:
                assert(false); // InputConsumer should prevent this from ever happening
                ALOGW("channel '%s' ~ Failed to obtain event object.", getInputChannelName());
                skipCallbacks = true;
            }
        }

        if (skipCallbacks) {
            mInputConsumer.sendFinishedSignal(seq, false);
        }
    }
}


int SkiWinInputEventReceiverInit(sp<SkiWinInputEventSink>& receiverObj,
        			   sp<InputChannel>& inputChannelObj, 
        			   sp<MessageQueue> messageQueueObj) {
    sp<InputChannel> inputChannel = inputChannelObj;
    if (inputChannel == NULL) {
        ALOGW("InputChannel is not initialized.");
        return 0;
    }

    sp<MessageQueue> messageQueue = messageQueueObj;
    if (messageQueue == NULL) {
        ALOGW("MessageQueue is not initialized.");
        return 0;
    }

    sp<SkiWinInputEventReceiver> receiver = new SkiWinInputEventReceiver(
            receiverObj, inputChannel, messageQueue);
    status_t status = receiver->initialize();
    if (status) {

        ALOGW("Failed to initialize input event receiver.  status=%d", status);
        return 0;
    }

    return reinterpret_cast<int>(receiver.get());
}

void SkiWinInputEventReceiverDispose(int receiverPtr) {
    sp<SkiWinInputEventReceiver> receiver =
            reinterpret_cast<SkiWinInputEventReceiver*>(receiverPtr);
    receiver->dispose();
}

void SkiWinInputEventReceiverFinishInputEvent(int receiverPtr,
        int seq, bool handled) {
    sp<SkiWinInputEventReceiver> receiver =
            reinterpret_cast<SkiWinInputEventReceiver*>(receiverPtr);
    status_t status = receiver->finishInputEvent(seq, handled);
    if (status && status != DEAD_OBJECT) {
        
        ALOGW("Failed to finish input event.  status=%d", status);   
    }
}

void SkiWinInputEventReceiverConsumeBatchedInputEvents(int receiverPtr,
        long frameTimeNanos) {
    sp<SkiWinInputEventReceiver> receiver =
            reinterpret_cast<SkiWinInputEventReceiver*>(receiverPtr);
    status_t status = receiver->consumeEvents(true /*consumeBatches*/, frameTimeNanos);
    if (status && status != DEAD_OBJECT) {
        ALOGW("Failed to consume batched input event.  status=%d", status);    
    }
}

} // namespace android


