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
#include "SkiWinEventListener.h"

#define LOGW printf
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                               const struct timespec *request,
                               struct timespec *remain);
#define EXIT_PROP_NAME "service.skiwin.exit"
namespace android
{

SkOSWindow* gWindow;

// ---dd------------------------------------------------------------------------

SkiWin::SkiWin() : Thread(false)
    {
    mSession = new SurfaceComposerClient();

    application_init();

    gWindow = create_sk_window(NULL, 0, 0);
    }

SkiWin::~SkiWin()
    {
    mSession = NULL;
    }

void SkiWin::onFirstRef()
    {
    status_t err = mSession->linkToComposerDeath(this);

    ALOGE_IF(err, "linkToComposerDeath failed (%s) ", strerror(-err));

    if (err == NO_ERROR)
        {
        ALOGD("Starting SkiWin thread...");

        run("SkiWin", PRIORITY_DISPLAY);
        }
    }

sp<SurfaceComposerClient> SkiWin::session() const
    {
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
#if 0
    printf("%s\n", __PRETTY_FUNCTION__);
    ALOGD("notifyKey - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, action=0x%x, "
          "flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%lld",
          args->eventTime, args->deviceId, args->source, args->policyFlags,
          args->action, args->flags, args->keyCode, args->scanCode,
          args->metaState, args->downTime);
#endif
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
#if 0
    printf("%s\n", __PRETTY_FUNCTION__);

    ALOGD("notifyMotion - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, "
          "action=0x%x, flags=0x%x, metaState=0x%x, buttonState=0x%x, edgeFlags=0x%x, "
          "xPrecision=%f, yPrecision=%f, downTime=%lld",
          args->eventTime, args->deviceId, args->source, args->policyFlags,
          args->action, args->flags, args->metaState, args->buttonState,
          args->edgeFlags, args->xPrecision, args->yPrecision, args->downTime);
    for (uint32_t i = 0; i < args->pointerCount; i++)
        {
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
#endif
    int32_t x = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X));
    int32_t y = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));

    SkView::Click::State state;

    switch(args->buttonState)
        {
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

status_t SkiWin::readyToRun()
    {

    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
                           ISurfaceComposer::eDisplayIdMain));

    DisplayInfo dinfo;

    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);

    if (status)
        {
        printf("getDisplayInfo failed in %s\n", __PRETTY_FUNCTION__);

        return -1;
        }

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

    gInputEventCallback.pfNotifyKey = SkiWinNotifyKeyCallback;
    gInputEventCallback.pfNotifyMotion = SkiWinNotifyMotionCallback;
    gInputEventCallback.pfNotifySwitch = SkiWinNotifySwitchCallback;
    gInputEventCallback.context = NULL;

    SkiWinInputConfiguration config =
        {
touchPointerVisible :
        true,
        touchPointerLayer : 0x40000001
        };

    SkiWinInputManagerInit(&gInputEventCallback, &config);
    SkiWinInputManagerStart();

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

SkBitmap::Config SkiWin::convertPixelFormat(PixelFormat format)
    {
    /* note: if PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, then
     we can map to SkBitmap::kARGB_8888_Config, and optionally call
     bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
     */
    switch (format)
        {
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

/**
 * lockCanvas - Start editing the pixels in the surface.
 *
 * Just like lockCanvas() but allows specification of a dirty rectangle.
 *
 * This is from android/4.2/frameworks/base/core/jni/android_view_Surface.cpp
 * nativeLockCanvas().
 */

SkCanvas* SkiWin::lockCanvas(const Rect& dirtyRect)
    {
    // get dirty region
    Region dirtyRegion;

    if (!dirtyRect.isEmpty())
        {
        dirtyRegion.set(dirtyRect);
        }
    else
        {
        dirtyRegion.set(Rect(0x3FFF, 0x3FFF));
        }

    Surface::SurfaceInfo info;

    status_t err = mFlingerSurface->lock(&info, &dirtyRegion);
    assert(err == 0);

    SkBitmap bitmap;

    ssize_t bpr = info.s * bytesPerPixel(info.format);

    bitmap.setConfig(convertPixelFormat(info.format), info.w, info.h, bpr);

    if (info.format == PIXEL_FORMAT_RGBX_8888)
        {
        bitmap.setIsOpaque(true);
        }

    if (info.w > 0 && info.h > 0)
        {
        bitmap.setPixels(info.bits);
        }
    else
        {
        // be safe with an empty bitmap.
        bitmap.setPixels(NULL);
        }

    mCanvas.setBitmapDevice(bitmap);

    SkRegion clipReg;
    if (dirtyRegion.isRect())   // very common case
        {
        const Rect b(dirtyRegion.getBounds());
        clipReg.setRect(b.left, b.top, b.right, b.bottom);
        }
    else
        {
        size_t count;
        Rect const* r = dirtyRegion.getArray(&count);
        while (count)
            {
            clipReg.op(r->left, r->top, r->right, r->bottom, SkRegion::kUnion_Op);
            r++, count--;
            }
        }

    mCanvas.clipRegion(clipReg);

    mCanvasSaveCount = mCanvas.save();

    return &mCanvas;
    }

/**
 * unlockCanvasAndPost - Finish editing pixels in the surface.
 *
 * This is from android/4.2/frameworks/base/core/jni/android_view_Surface.cpp
 * nativeLockCanvas().
 */

void SkiWin::unlockCanvasAndPost()
    {

    // detach the canvas from the surface
    mCanvas.restoreToCount(mCanvasSaveCount);

    mCanvas.setBitmapDevice(SkBitmap());

    // unlock surface
    status_t err = mFlingerSurface->unlockAndPost();

    assert(err == 0);
    }

void SkiWin::drawTitle(const SkString& string,
                       bool subpixelTextEnabled,
                       bool lcdRenderTextEnabled)
    {
    SkPaint paint;

    paint.setColor(SK_ColorWHITE);
    paint.setDither(true);
    paint.setAntiAlias(true);
    paint.setSubpixelText(subpixelTextEnabled);
    paint.setLCDRenderText(lcdRenderTextEnabled);
    paint.setTextSize(25);

    mCanvas.drawText(string.c_str(), string.size(), 15, 15, paint);
    }


bool SkiWin::android()
    {

    const nsecs_t startTime = systemTime();

    Rect rect(mWidth, mHeight);

    gWindow->resize(mWidth, mHeight);

    int numViews = getSampleCount(gWindow);
    int i = 0;
    int index = 0;

    do
        {
        nsecs_t now = systemTime();
        double time = now - startTime;

        SkCanvas* canvas = lockCanvas(rect);

        index = i++ % numViews;

        //loadSample(gWindow, index);

        gWindow->update(NULL);

        canvas->clear(SK_ColorBLACK);

        drawTitle(SkString(gWindow->getTitle()), true, true);

        canvas->drawBitmap(gWindow->getBitmap(), 0, 30);

        unlockCanvasAndPost();

        // 12fps: don't animate too fast to preserve CPU
        const nsecs_t sleepTime = 83333 - ns2us(systemTime() - now);
        if (sleepTime > 0)
            usleep(sleepTime);

        checkExit();
        }
    while (!exitPending());

    return false;
    }


void SkiWin::checkExit()
    {
    // Allow surface flinger to gracefully request shutdown
    char value[PROPERTY_VALUE_MAX];

    property_get(EXIT_PROP_NAME, value, "0");

    int exitnow = atoi(value);

    if (exitnow)
        {
        requestExit();
        }
    }
}; // namespace android

