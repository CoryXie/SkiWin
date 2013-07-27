/*
 * Copyright (C) 2010 The Android Open Source Project
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

#ifndef _ANDROID_OS_MESSAGEQUEUE_H
#define _ANDROID_OS_MESSAGEQUEUE_H

#include <utils/Looper.h>

namespace android {

class MessageQueue : public RefBase {
public:
    /* Gets the message queue's looper. */
    inline sp<Looper> getLooper() const {
        return mLooper;
    }

    MessageQueue();
    virtual ~MessageQueue();

protected:
    sp<Looper> mLooper;
};

class SkiWinMessageQueue : public MessageQueue {
public:
    SkiWinMessageQueue();
    virtual ~SkiWinMessageQueue();

    void pollOnce(int timeoutMillis);

    void wake();

private:
    bool mInCallback;
};

} // namespace android

#endif
