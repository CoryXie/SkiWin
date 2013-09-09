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
#include "SkiWinView.h"

#define LOGW printf
extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                               const struct timespec *request,
                               struct timespec *remain);
#define EXIT_PROP_NAME "service.skiwin.exit"
namespace android
{

// ---dd------------------------------------------------------------------------

SkiWin::SkiWin() : Thread(false)
    {
    DisplayInfo dinfo;
    
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
                           ISurfaceComposer::eDisplayIdMain));

    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);

    if (status)
        {
        printf("getDisplayInfo failed in %s\n", __PRETTY_FUNCTION__);

        return;
        }

    mWidth = dinfo.w;
    mHeight = dinfo.h;

    mSession = new SurfaceComposerClient();

    /*
    |----------------------|
    |    title view        | 30
    |----------------------|
    |                      |
    |    content view top  | 150
    |                      |
    |----------------------|
    |                      |
    |    content view mid  | 150
    |                      |
    |----------------------|
    |                      |
    |    content view bot  | 150
    |                      |
    |----------------------|
    */
    mTitleView = new SkiWinView(mSession, 
                   String8("TitleView"),
                   0, 0, 320, 30, 0x40000000);
    
    mContentViewTop = new SkiWinView(mSession, 
                   String8("ContentViewTop"),
                   0, 30, 320, 150, 0x40000001);

    mContentViewMid = new SkiWinView(mSession, 
                   String8("ContentViewMid"),
                   0, 180, 320, 150, 0x40000002);

    mContentViewBot = new SkiWinView(mSession, 
                   String8("ContentViewBot"),
                   0, 330, 320, 150, 0x40000003);
    
    application_init();

    mWindowTop = create_sk_window(NULL, 0, 0);
    mWindowBot = create_sk_window(NULL, 0, 0);
    }

SkiWin::~SkiWin()
    {
    mSession = NULL;
    mTitleView = NULL;
    mContentViewTop = NULL;
    mContentViewMid = NULL;
    mContentViewBot = NULL;
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
    SkiWin* skiwin = reinterpret_cast<SkiWin*>(context);
    SkOSWindow* mWindowTop = skiwin->mWindowTop;
    SkOSWindow* mWindowBot = skiwin->mWindowBot;
#if 0
    printf("args->action %d %d\n", 
        args->action, AndroidKeycodeToSkKey(args->keyCode));
#endif
    
    if (args->action == AKEY_EVENT_ACTION_DOWN)
        {
        mWindowTop->handleKey(AndroidKeycodeToSkKey(args->keyCode));
        /* mWindowTop->handleChar((SkUnichar) uni); */

        mWindowBot->handleKey(AndroidKeycodeToSkKey(args->keyCode));
        /* mWindowBot->handleChar((SkUnichar) uni); */
        }
    else if (args->action == AKEY_EVENT_ACTION_UP)
        {
        mWindowTop->handleKeyUp(AndroidKeycodeToSkKey(args->keyCode));
        mWindowBot->handleKeyUp(AndroidKeycodeToSkKey(args->keyCode));
        }
    }

void SkiWinNotifyMotionCallback(const NotifyMotionArgs* args, void* context)
    {
#if 0
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
    SkiWin* skiwin = reinterpret_cast<SkiWin*>(context);
    SkOSWindow* mWindowTop = skiwin->mWindowTop;
    SkOSWindow* mWindowBot = skiwin->mWindowBot;

    SkView::Click::State state;

    switch(args->action)
        {
        case AMOTION_EVENT_ACTION_DOWN:	   // MotionEvent.ACTION_DOWN
            state = SkView::Click::kDown_State;
            break;
        case AMOTION_EVENT_ACTION_UP:	   // MotionEvent.ACTION_UP
        case AMOTION_EVENT_ACTION_CANCEL:  // MotionEvent.ACTION_CANCEL
            state = SkView::Click::kUp_State;
            break;
        case AMOTION_EVENT_ACTION_MOVE:	   // MotionEvent.ACTION_MOVE
            state = SkView::Click::kMoved_State;
            break;
        default:
            SkDebugf("motion event ignored\n");
            return;
        }
    #if 0
    printf("args->action %d (x,y) = (%d, %d)\n", args->action, x, y);
    #endif
    
    if (y < 330)
        mWindowTop->handleClick(x, y, state);
    else
        mWindowBot->handleClick(x, y - 330, state);    
    }

void SkiWinNotifySwitchCallback(const NotifySwitchArgs* args, void* context)
    {    
    SkiWin* skiwin = reinterpret_cast<SkiWin*>(context);
    SkOSWindow* gWindow = skiwin->mWindowTop;
    printf("%s\n", __PRETTY_FUNCTION__);
    }

SkiWinEventCallback gInputEventCallback;

status_t SkiWin::readyToRun()
    {
    gInputEventCallback.pfNotifyKey = SkiWinNotifyKeyCallback;
    gInputEventCallback.pfNotifyMotion = SkiWinNotifyMotionCallback;
    gInputEventCallback.pfNotifySwitch = SkiWinNotifySwitchCallback;
    gInputEventCallback.context = this;

    SkiWinInputConfiguration config =
        {
        touchPointerVisible : true,
        touchPointerLayer : 0x40000005
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

    IPCThreadState::self()->stopProcess();

    return r;
    }

bool SkiWin::android()
    {
    SkCanvas* contentCanvasTop;
    SkCanvas* contentCanvasMid;
    SkCanvas* contentCanvasBot;
    SkBitmap bitmap;

    Rect rect(mWidth, mHeight);

    mWindowTop->resize(320, 150);
    mWindowBot->resize(320, 150);
    
    do
        {
        mWindowTop->update(NULL);
        mWindowBot->update(NULL);

        SkCanvas* titileCanvas = mTitleView->lockCanvas(rect);
        if (titileCanvas)
            {
            SkPaint paint;
            const char * title = mWindowTop->getTitle();
            
            paint.setColor(SK_ColorWHITE);
            paint.setDither(true);
            paint.setAntiAlias(true);
            paint.setSubpixelText(true);
            paint.setLCDRenderText(true);
            paint.setTextSize(25);
                        
            titileCanvas->clear(SK_ColorBLACK);
            
            titileCanvas->drawText(title, strlen(title), 15, 25, paint);
            }
        mTitleView->unlockCanvasAndPost();
        
        contentCanvasTop = mContentViewTop->lockCanvas(rect);
        if (contentCanvasTop)
            {            
            contentCanvasTop->drawBitmap(mWindowTop->getBitmap(), 0, 30);
            }
        mContentViewTop->unlockCanvasAndPost();
        
        contentCanvasMid = mContentViewMid->lockCanvas(rect);
        if (contentCanvasMid)
            {    
            SkPaint paint;
            int color = 0xff00ff77;
            paint.setARGB(color>>24 & 0xff, 
                          color>>16 & 0xff, 
                          color>>8 & 0xff, 
                          color & 0xff);
            
            SkRect rect1;
            rect1.fLeft = 100;
            rect1.fTop = 0;
            rect1.fRight = 300;
            rect1.fBottom = 120;

            contentCanvasMid->drawRect(rect1, paint);         
            //contentCanvasMid->drawBitmap(mWindowTop->getBitmap(), 0, 0);
            contentCanvasMid->drawLine(100, 0, 100, 480, paint);
            }
        mContentViewMid->unlockCanvasAndPost();   

        contentCanvasBot = mContentViewBot->lockCanvas(rect);
        if (contentCanvasBot)
            {            
            contentCanvasBot->drawBitmap(mWindowBot->getBitmap(), 0, 0);
            }
        mContentViewBot->unlockCanvasAndPost(); 

        usleep(100000);        

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

