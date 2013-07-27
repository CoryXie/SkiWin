#define LOG_TAG "SkiWinMessageQueue"

#include <android_runtime/AndroidRuntime.h>

#include <utils/Looper.h>
#include <utils/Log.h>
#include "MessageQueue.h"

namespace android {


MessageQueue::MessageQueue() {
}

MessageQueue::~MessageQueue() {
}


SkiWinMessageQueue::SkiWinMessageQueue() : mInCallback(false){
    mLooper = Looper::getForThread();
    if (mLooper == NULL) {
        mLooper = new Looper(false);
        Looper::setForThread(mLooper);
    }
}

SkiWinMessageQueue::~SkiWinMessageQueue() {
}

void SkiWinMessageQueue::pollOnce(int timeoutMillis) {
    mInCallback = true;
    mLooper->pollOnce(timeoutMillis);
    mInCallback = false;
}

void SkiWinMessageQueue::wake() {
    mLooper->wake();
}

}

