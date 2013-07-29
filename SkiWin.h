/*
 * Copyright (C) 2007 The Android Open Source Project
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

#ifndef ANDROID_SKIWIN_H
#define ANDROID_SKIWIN_H

#include <stdint.h>
#include <sys/types.h>

#include <androidfw/AssetManager.h>
#include <utils/threads.h>
#include <utils/Log.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <utils/RefBase.h>
#include <utils/KeyedVector.h>
#include <utils/List.h>
#include <input/EventHub.h>
#include <input/InputReader.h>
#include <input/InputApplication.h>
#include <input/InputWindow.h>

#include <SkWindow.h>
#include <SkApplication.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkRegion.h>

#include "SkiWinInputEventSink.h"
#include "MessageQueue.h"

class SkBitmap;
class SkCanvas;

namespace android {

class Surface;
class SurfaceComposerClient;
class SurfaceControl;


extern int SkiWinInputEventReceiverInit(sp<SkiWinInputEventSink>& receiverObj,
        			   sp<InputChannel>& inputChannelObj, 
        			   sp<MessageQueue> messageQueueObj);

// ---------------------------------------------------------------------------

class SkiWin : public Thread, public IBinder::DeathRecipient
{
public:
                SkiWin();
    virtual     ~SkiWin();

    sp<SurfaceComposerClient> session() const;
    
#ifdef USE_RAW_EVENT_HUB
struct TouchEvent {
		enum TouchEventType {
			Down, Up, Moving
		};
		int32_t x;
		int32_t y;
		enum TouchEventType type;
	};
#else
    void notifySwitch(nsecs_t when,
        uint32_t switchValues, uint32_t switchMask);
    void notifyConfigurationChanged(nsecs_t when);
    nsecs_t notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
        const sp<InputWindowHandle>& inputWindowHandle);
    void notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle);
    bool filterInputEvent(const KeyEvent*keyEvent);
    bool filterInputEvent(const MotionEvent*motionEvent);
    int interceptKeyBeforeQueueing(const KeyEvent* keyEvent,
        uint32_t& policyFlags, bool screenOn);
    nsecs_t interceptKeyBeforeDispatching(
        const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags);
    int interceptMotionBeforeQueueingWhenScreenOff(uint32_t& policyFlags);
    KeyEvent* dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags);
    bool checkInjectEventsPermission(
        int32_t injectorPid, int32_t injectorUid);

    bool updateInputWindowInfo(struct InputWindowInfo *mInfo);
#endif /* USE_RAW_EVENT_HUB */

private:
    virtual bool        threadLoop();
    virtual status_t    readyToRun();
    virtual void        onFirstRef();
    virtual void        binderDied(const wp<IBinder>& who);

#ifndef USE_RAW_EVENT_HUB
	bool dispatchBatchedInputEventPending(); 
	bool dispatchInputEvent(int seq, KeyEvent* event);
	bool dispatchInputEvent(int seq, MotionEvent* event); 
#else    
    void processEvent(const RawEvent& rawEvent);
    void consumeEvent(const RawEvent& rawEvent);
    void printTouchEventType();
#endif /* USE_RAW_EVENT_HUB */

    SkBitmap::Config convertPixelFormat(PixelFormat format);
    SkCanvas* lockCanvas(const Rect& dirtyRect);
    void unlockCanvasAndPost();

    bool android();

    void checkExit();

    sp<SurfaceComposerClient>       mSession;
  
    int         mWidth;
    int         mHeight;
   
    sp<SurfaceControl> mFlingerSurfaceControl;
    sp<Surface> mFlingerSurface;

#ifdef USE_RAW_EVENT_HUB
    sp<EventHub> mEventHub;
    bool waiting;
    TouchEvent mTouchEvent;
    List<TouchEvent> eventBuffer;
#else
    sp<InputChannel> serverChannel, clientChannel;
    InputPublisher* mPublisher;
    InputConsumer* mConsumer;
    PreallocatedInputEventFactory mEventFactory;
    sp<MessageQueue> mMessageQueue;
    sp<SkiWinInputEventSink> mInputEventSink;

    int mSkiInputManager;
    Vector<sp<InputWindowHandle> > windowHandles;
    sp<InputApplicationHandle> appHandle ;
    sp<InputWindowHandle> winHandle ;
#endif /* USE_RAW_EVENT_HUB */

    SkCanvas canvas;
    int saveCount;
};

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SKIWIN_H
