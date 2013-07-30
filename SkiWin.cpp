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

#define LOG_TAG "SkiWin"

#include <stdint.h>
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <utils/misc.h>
#include <signal.h>
#include <unistd.h>

#include <cutils/properties.h>

#include <androidfw/AssetManager.h>
#include <binder/IPCThreadState.h>
#include <utils/Atomic.h>
#include <utils/Errors.h>
#include <utils/Log.h>
#include <utils/threads.h>

#include <ui/PixelFormat.h>
#include <ui/Rect.h>
#include <ui/Region.h>
#include <ui/DisplayInfo.h>
#include <ui/FramebufferNativeWindow.h>

#include <gui/ISurfaceComposer.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>

#include <input/InputWindow.h>
#include <input/InputReader.h>

#include <core/SkBitmap.h>
#include <core/SkStream.h>
#include <images/SkImageDecoder.h>
#include <utils/android/AndroidKeyToSkKey.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include "SkiWin.h"
#include "SkiWinConfig.h"
#include "MessageQueue.h"
#include "SkiWinEventListener.h"

#define LOGW printf
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                           const struct timespec *request,
                           struct timespec *remain);
#define EXIT_PROP_NAME "service.skiwin.exit"
namespace android {

SkOSWindow* gWindow;

// ---dd------------------------------------------------------------------------

SkiWin::SkiWin() : Thread(false)
{
    mSession = new SurfaceComposerClient();

    application_init();

    gWindow = create_sk_window(NULL, 0, 0);

}

SkiWin::~SkiWin() {
}

void SkiWin::onFirstRef() {
    status_t err = mSession->linkToComposerDeath(this);
    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));
    if (err == NO_ERROR) {
        run("SkiWin", PRIORITY_DISPLAY);
    }
}

sp<SurfaceComposerClient> SkiWin::session() const {
    return mSession;
}


void SkiWin::binderDied(const wp<IBinder>& who)
{
    // woah, surfaceflinger died!
    ALOGD("SurfaceFlinger died, exiting...");

    // calling requestExit() is not enough here because the Surface code
    // might be blocked on a condition variable that will never be updated.
    kill( getpid(), SIGKILL );
    requestExit();
}

void SkiWinNotifyKeyCallback(const NotifyKeyArgs* args, void* context)
{
    printf("%s\n", __PRETTY_FUNCTION__);
	ALOGD("notifyKey - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, action=0x%x, "
			"flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%lld",
			args->eventTime, args->deviceId, args->source, args->policyFlags,
			args->action, args->flags, args->keyCode, args->scanCode,
			args->metaState, args->downTime);

	if (args->action == AKEY_EVENT_ACTION_DOWN) 
		{
		gWindow->handleKey(AndroidKeycodeToSkKey(args->keyCode));
		/* gWindow->handleChar((SkUnichar) uni); */
		} 
	else if (args->action == AKEY_EVENT_ACTION_UP) 
		{
		gWindow->handleKeyUp(AndroidKeycodeToSkKey(args->keyCode));
		}
}

void SkiWinNotifyMotionCallback(const NotifyMotionArgs* args, void* context)
{
    printf("%s\n", __PRETTY_FUNCTION__);
	
    ALOGD("notifyMotion - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
            "action=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x, edgeFlags=0x%x, "
            "xPrecision=%f, yPrecision=%f, downTime=%lld",
            args->eventTime, args->deviceId, args->source, args->policyFlags,
            args->action, args->flags, args->metaState, args->buttonState,
            args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime);
    for (uint32_t i = 0; i < args->pointerCount; i++) {
        ALOGD("  Pointer %d: id=%d, toolType=%d, "
                "x=%f, y=%f, pressure=%f, size=%f, "
                "touchMajor=%f, touchMinor=%f, toolMajor=%f, toolMinor=%f, "
                "orientation=%f",
                i, args->pointerProperties[i].id,
                args->pointerProperties[i].toolType,
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_X),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_Y),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MAJOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOOL_MINOR),
                args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION));
            }

	int32_t x = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X));
	int32_t y = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));

   SkView::Click::State state;
   switch(args->buttonState) {
	   case 0:	   // MotionEvent.ACTION_DOWN
		   state = SkView::Click::kDown_State;
		   break;
	   case 1:	   // MotionEvent.ACTION_UP
	   case 3:	   // MotionEvent.ACTION_CANCEL
		   state = SkView::Click::kUp_State;
		   break;
	   case 2:	   // MotionEvent.ACTION_MOVE
		   state = SkView::Click::kMoved_State;
		   break;
	   default:
		   SkDebugf("motion event ignored\n");
		   return;
   }
   gWindow->handleClick(x, y, state);
}

void SkiWinNotifySwitchCallback(const NotifySwitchArgs* args, void* context)
{
    printf("%s\n", __PRETTY_FUNCTION__);

}
 
SkiWinEventCallback gInputEventCallback;

status_t SkiWin::readyToRun() {

    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
            ISurfaceComposer::eDisplayIdMain));
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    if (status)
        return -1;

    // create the native surface
    sp<SurfaceControl> control = session()->createSurface(String8("SkiWin"),
            dinfo.w, dinfo.h, PIXEL_FORMAT_RGB_565);

    SurfaceComposerClient::openGlobalTransaction();
    control->setLayer(0x40000000);
    SurfaceComposerClient::closeGlobalTransaction();

    sp<Surface> surface = control->getSurface();

    mWidth = dinfo.w;
    mHeight = dinfo.h;
    mFlingerSurfaceControl = control;
    mFlingerSurface = surface;

#ifdef USE_RAW_EVENT_HUB

    gInputEventCallback.pfNotifyKey = SkiWinNotifyKeyCallback;
    gInputEventCallback.pfNotifyMotion = SkiWinNotifyMotionCallback;
    gInputEventCallback.pfNotifySwitch = SkiWinNotifySwitchCallback;
    gInputEventCallback.context = NULL;

    SkiWinInputConfiguration config =
    {
	touchPointerVisible : true,
    touchPointerLayer : 0x40000001
    };

    SkiWinInputManagerInit(&gInputEventCallback, &config);
    SkiWinInputManagerStart();

#else /* USE_RAW_EVENT_HUB */
	status_t result = InputChannel::openInputChannelPair(String8("SkiWin"),
			serverChannel, clientChannel);

	mMessageQueue = new SkiWinMessageQueue();

	appHandle = new SkiWinInputApplicationHandle(reinterpret_cast<int>(this));
	winHandle = new SkiWinInputWindowHandle(reinterpret_cast<int>(this), appHandle);

	windowHandles.add(winHandle);

	mSkiInputManager = SkiWinInputManagerInit(NULL, NULL, mMessageQueue);

	SkiWinInputManagerSetWin(mSkiInputManager, reinterpret_cast<int>(this));

	SkiWinInputManagerRegisterInputChannel(mSkiInputManager,
										   clientChannel,
										   winHandle,
										   true);

	SkiWinInputManagerStart(mSkiInputManager);

#endif /* USE_RAW_EVENT_HUB */
    return NO_ERROR;
}

bool SkiWin::threadLoop()
	{
	bool r;

	r = android();

	// No need to force exit anymore
	property_set(EXIT_PROP_NAME, "0");

	mFlingerSurface.clear();
	mFlingerSurfaceControl.clear();

	IPCThreadState::self()->stopProcess();
	return r;
	}

#ifdef USE_RAW_EVENT_HUB

#else

SkiWinInputEventSink::SkiWinInputEventSink(const void * win)
	{
  	mWin = win;
	}

bool SkiWinInputEventSink::dispatchBatchedInputEventPending()
	{
	return false;
	}

bool SkiWinInputEventSink::dispatchInputEvent(int seq, KeyEvent* event)
	{
	LOGW("dispatchInputEvent KeyEvent seq-%d\n",seq);

	return true;
	}

bool SkiWinInputEventSink::dispatchInputEvent(int seq, MotionEvent* event)
	{
	LOGW("dispatchInputEvent MotionEvent seq-%d\n",seq);

	return true;
	}

bool SkiWin::updateInputWindowInfo(struct InputWindowInfo *mInfo)
	{
	LOGW("updateInputWindowInfo %p\n",mInfo);

    mInfo->inputChannel = clientChannel;

    mInfo->name.setTo("SkiWin");

    mInfo->layoutParamsFlags =    FLAG_KEEP_SCREEN_ON
	   							| FLAG_LAYOUT_IN_SCREEN
	   							| FLAG_LAYOUT_NO_LIMITS;
    mInfo->layoutParamsType = TYPE_BASE_APPLICATION;
    mInfo->dispatchingTimeout = DEFAULT_INPUT_DISPATCHING_TIMEOUT_NANOS;
    mInfo->frameLeft = 0;
    mInfo->frameTop = 0;
    mInfo->frameRight = mWidth;
    mInfo->frameBottom = mHeight;
    mInfo->scaleFactor = 1;

	SkIRect r;

	r.set(0, 0, mWidth, mHeight);

    SkRegion region(r); ;
    mInfo->touchableRegion.set(region);

    mInfo->visible = true;
    mInfo->canReceiveKeys = true;
    mInfo->hasFocus = true;
    mInfo->hasWallpaper = false;
    mInfo->paused = false;
    mInfo->layer = -1;
    mInfo->ownerPid = getpid();
    mInfo->ownerUid = getuid();
    mInfo->inputFeatures = 0;
    mInfo->displayId = 0;

	return true;
	}

void SkiWin::notifySwitch(nsecs_t when,
	uint32_t switchValues, uint32_t switchMask)
	{
	LOGW("notifySwitch when %d switchValues %d switchMask %d\n",
		when, switchValues, switchMask);
	return;
	}

void SkiWin::notifyConfigurationChanged(nsecs_t when)
	{
	LOGW("notifyConfigurationChanged when %d \n", when);

	return;
	}

nsecs_t SkiWin::notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
	const sp<InputWindowHandle>& inputWindowHandle)
	{
	LOGW("notifyANR inputApplicationHandle %p inputWindowHandle %p\n",
		inputApplicationHandle.get(), inputWindowHandle.get());
	return 0;
	}

nsecs_t SkiWin::interceptKeyBeforeDispatching(
        const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags)
	{
	LOGW("interceptKeyBeforeDispatching inputWindowHandle %p keyEvent %p policyFlags 0x%x\n",
		inputWindowHandle.get(), keyEvent, policyFlags);
	return 0;
	}

void SkiWin::notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle)
	{
	LOGW("notifyInputChannelBroken inputWindowHandle %p\n",
		inputWindowHandle.get());
	return ;
	}

bool SkiWin::filterInputEvent(const KeyEvent* keyEvent)
	{
	LOGW("filterInputEvent KeyEvent %p\n",
		keyEvent);

	return true;
	}

bool SkiWin::filterInputEvent(const MotionEvent* motionEvent)
	{
	LOGW("filterInputEvent MotionEvent %p\n",
		motionEvent);

	return true;
	}

int SkiWin::interceptKeyBeforeQueueing(const KeyEvent* keyEvent,
	uint32_t& policyFlags, bool screenOn)
	{
	LOGW("interceptKeyBeforeQueueing keyEvent %p policyFlags 0x%x screenOn %d\n",
		keyEvent, policyFlags, screenOn);

	return 0;
	}

int SkiWin::interceptMotionBeforeQueueingWhenScreenOff(uint32_t& policyFlags)
	{
	LOGW("interceptMotionBeforeQueueingWhenScreenOff policyFlags 0x%x\n",
		policyFlags);

	return 0;
	}

KeyEvent* SkiWin::dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
	const KeyEvent* keyEvent, uint32_t policyFlags)
	{
	LOGW("dispatchUnhandledKey inputWindowHandle %p keyEvent %p policyFlags 0x%x\n",
		inputWindowHandle.get(),keyEvent, policyFlags);

	return NULL;
	}

bool SkiWin::checkInjectEventsPermission(
	int32_t injectorPid, int32_t injectorUid)
	{
	LOGW("checkInjectEventsPermission injectorPid 0x%x injectorUid 0x%x\n",
		injectorPid, injectorUid);

	return true;
	}

#endif /* USE_RAW_EVENT_HUB */

SkBitmap::Config SkiWin::convertPixelFormat(PixelFormat format) {
	/* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
	 we can map to SkBitmap::kARGB_8888_Config, and optionally call
	 bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
	 */
	switch (format) {
	case PIXEL_FORMAT_RGBX_8888:
		return SkBitmap::kARGB_8888_Config;
	case PIXEL_FORMAT_RGBA_8888:
		return SkBitmap::kARGB_8888_Config;
	case PIXEL_FORMAT_RGBA_4444:
		return SkBitmap::kARGB_4444_Config;
	case PIXEL_FORMAT_RGB_565:
		return SkBitmap::kRGB_565_Config;
	case PIXEL_FORMAT_A_8:
		return SkBitmap::kA8_Config;
	default:
		return SkBitmap::kNo_Config;
	}
}

SkCanvas* SkiWin::lockCanvas(const Rect& dirtyRect) {
	// get dirty region
	Region dirtyRegion;
	if (!dirtyRect.isEmpty()) {
		dirtyRegion.set(dirtyRect);
	}
	Surface::SurfaceInfo info;
	status_t err = mFlingerSurface->lock(&info, &dirtyRegion);
	assert(err == 0);
	SkBitmap bitmap;
	ssize_t bpr = info.s * bytesPerPixel(info.format);
	bitmap.setConfig(convertPixelFormat(info.format), info.w, info.h, bpr);
	if (info.format == PIXEL_FORMAT_RGBX_8888) {
		bitmap.setIsOpaque(true);
	}
	if (info.w > 0 && info.h > 0) {
		bitmap.setPixels(info.bits);
	} else {
		// be safe with an empty bitmap.
		bitmap.setPixels(NULL);
	}
	canvas.setBitmapDevice(bitmap);

	saveCount = canvas.save();
	return &canvas;
}

void SkiWin::unlockCanvasAndPost() {
	// detach the canvas from the surface
	canvas.restoreToCount(saveCount);
	canvas.setBitmapDevice(SkBitmap());
	// unlock surface
	status_t err = mFlingerSurface->unlockAndPost();
	assert(err == 0);
}
bool SkiWin::android()
{

    const nsecs_t startTime = systemTime();

    Rect rect(mWidth, mHeight);

    gWindow->resize(mWidth, mHeight);

    int numViews = getSampleCount(gWindow);
    int i = 0;
    int index = 0;

    do {
        nsecs_t now = systemTime();
        double time = now - startTime;

	#ifdef USE_RAW_EVENT_HUB
		//RawEvent event;
		//mEventHub->getEvents(-1, &event, 1);
		//processEvent(event);
	#else
	    //SkiWinInputManagerSetFocusedApplication(mSkiInputManager, appHandle);
	    //SkiWinInputManagerSetInputWindows(mSkiInputManager, windowHandles);
	#endif

		SkCanvas* canvas = lockCanvas(rect);

		index = i++ % numViews;

    	//loadSample(gWindow, index);

    	gWindow->update(NULL);

    	canvas->drawBitmap(gWindow->getBitmap(), 0, 0);

		unlockCanvasAndPost();

              // 12fps: don't animate too fast to preserve CPU
        const nsecs_t sleepTime = 83333 - ns2us(systemTime() - now);
        if (sleepTime > 0)
            usleep(sleepTime);

        checkExit();
    } while (!exitPending());

    return false;
}


void SkiWin::checkExit() {
    // Allow surface flinger to gracefully request shutdown
    char value[PROPERTY_VALUE_MAX];
    property_get(EXIT_PROP_NAME, value, "0");
    int exitnow = atoi(value);
    if (exitnow) {
        requestExit();
    }
}
// ---------------------------------------------------------------------------

}
; // namespace android

