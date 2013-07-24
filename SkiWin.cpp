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

#include <core/SkBitmap.h>
#include <core/SkStream.h>
#include <images/SkImageDecoder.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include "SkiWin.h"

#define LOGW printf
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                           const struct timespec *request,
                           struct timespec *remain);
#define EXIT_PROP_NAME "service.skiwin.exit"
namespace android {

SkOSWindow* gWindow;

// ---dd------------------------------------------------------------------------

SkiWin::SkiWin() : Thread(false), mEventHub(new EventHub)
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

    sp<Surface> s = control->getSurface();

 
    mWidth = dinfo.w;
    mHeight = dinfo.h;
    mFlingerSurfaceControl = control;
    mFlingerSurface = s;

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

void SkiWin::processEvent(const RawEvent& rawEvent) {
	String8 name = mEventHub->getDeviceIdentifier(rawEvent.deviceId).name;
	switch (rawEvent.type) {
	case EventHubInterface::DEVICE_ADDED:
		LOGW("add:%s\n",name.string());
		break;

	case EventHubInterface::DEVICE_REMOVED:
		LOGW("add:%s\n",name.string());
		break;

	case EventHubInterface::FINISHED_DEVICE_SCAN:
		LOGW("finished scan:%s\n",name.string());
		break;

	default:
		consumeEvent(rawEvent);
		break;
	}
}
void SkiWin::consumeEvent(const RawEvent& rawEvent) {
	switch (rawEvent.type) {
	case EV_ABS:
		if (!waiting) {
			waiting = true;
			mTouchEvent.type = TouchEvent::Moving;
		}
		switch (rawEvent.code) {
		case ABS_X:
			mTouchEvent.x = rawEvent.value;
			break;
		case ABS_Y:
			mTouchEvent.y = rawEvent.value;
			break;
		case ABS_PRESSURE:
			mTouchEvent.type = (rawEvent.value == 1 ? TouchEvent::Down
					: TouchEvent::Up);
			break;
		}
		break;
	case EV_SYN:
		switch (rawEvent.code) {
		case SYN_REPORT:
			if (waiting) {
				printTouchEventType();
				eventBuffer.push_back(mTouchEvent);
				waiting = false;
			}
			break;
		}
		break;
	}
}

void SkiWin::printTouchEventType() {
	switch (mTouchEvent.type) {
	case TouchEvent::Down:
		LOGW("down:%d,%d\n",mTouchEvent.x,mTouchEvent.y);
                gWindow->handleClick(mTouchEvent.x, mTouchEvent.y, SkView::Click::kDown_State);
		break;
	case TouchEvent::Up:
		LOGW("up:%d,%d\n",mTouchEvent.x,mTouchEvent.y);
		gWindow->handleClick(mTouchEvent.x, mTouchEvent.y, SkView::Click::kUp_State);
		break;
	case TouchEvent::Moving:
		LOGW("moving:%d,%d\n",mTouchEvent.x,mTouchEvent.y);
		gWindow->handleClick(mTouchEvent.x, mTouchEvent.y, SkView::Click::kMoved_State);
		break;
	}
}

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
    do {
        nsecs_t now = systemTime();
        double time = now - startTime;
        ///LOGW("Try to get events...\n");
	RawEvent event;
	mEventHub->getEvents(-1, &event, 1);
 	//LOGW("Exit from get events.\n");
	processEvent(event);

	SkCanvas* canvas = lockCanvas(rect);

    	gWindow->update(NULL);

    	canvas->drawBitmap(gWindow->getBitmap(), 0, 0);

	unlockCanvasAndPost();

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
