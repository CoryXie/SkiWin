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
#include <sys/stat.h>

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
#include <images/SkImageRef_GlobalPool.h>
#include <core/SkRefCnt.h>
#include <utils/android/AndroidKeyToSkKey.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include "SkiWin.h"
#include "SkiWinEventListener.h"
#include "SkiWinView.h"

extern "C" int clock_nanosleep(clockid_t clock_id, int flags,
                               const struct timespec *request,
                               struct timespec *remain);
#define EXIT_PROP_NAME "service.skiwin.exit"
namespace android
{

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

    mFocusView = NULL;

    mSession = new SurfaceComposerClient();

    /*
    |----------------------|
    |    title view top    | 30
    |----------------------|
    |                      |
    |    content view top  | 150
    |                      |
    |----------------------|
    |                      |
    |    content view mid  | 150
    |                      |
    |-----|----------------|
    |title|                |
    |view |content view bot| 150
    |bot  |                |
    |-----|----------------|
      70         250
    */
    mTitleViewTop = new SkiWinView(mSession,
                                   String8("TitleViewTop"),
                                   0, 0, 320, 30, 0x40000000);

    mContentViewTop = new SkiWinView(mSession,
                                     String8("ContentViewTop"),
                                     0, 30, 320, 150, 0x40000001);

    mContentViewMid = new SkiWinView(mSession,
                                     String8("ContentViewMid"),
                                     0, 180, 320, 150, 0x40000002);

    mContentViewBot = new SkiWinView(mSession,
                                     String8("ContentViewBot"),
                                     70, 330, 250, 150, 0x40000003);

    mTitleViewBot = new SkiWinView(mSession,
                                   String8("TitleViewBot"),
                                   0, 330, 70, 150, 0x40000003);

    application_init();

    mWindowTop = create_sk_window(NULL, 0, 0);
    mWindowMid = create_sk_window(NULL, 0, 0);
    mWindowBot = create_sk_window(NULL, 0, 0);

    loadSampleByTitle(mWindowMid,"TextBox");

    mContentViewTop->setContext(reinterpret_cast<void *>(mWindowTop));
    mContentViewMid->setContext(reinterpret_cast<void *>(mWindowMid));
    mContentViewBot->setContext(reinterpret_cast<void *>(mWindowBot));
    }

SkiWin::~SkiWin()
    {
    mSession = NULL;
    mTitleViewTop = NULL;
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
    ALOGD("notifyKey - eventTime=%lld, deviceId=%d, source=0x%x, policyFlags=0x%x, action=0x%x, "
          "flags=0x%x, keyCode=0x%x, scanCode=0x%x, metaState=0x%x, downTime=%lld",
          args->eventTime, args->deviceId, args->source, args->policyFlags,
          args->action, args->flags, args->keyCode, args->scanCode,
          args->metaState, args->downTime);
#endif
    SkiWin* skiwin = reinterpret_cast<SkiWin*>(context);
    SkOSWindow* mFocusWindow = NULL;
    sp<SkiWinView> mFocusView = NULL;
#if 1
    printf("KeyCallback args->action %d %d\n",
           args->action, AndroidKeycodeToSkKey(args->keyCode));
#endif

    mFocusView = skiwin->getFocusView();

	if (AKEYCODE_HOME == args->keyCode)
		{
		printf("AKEYCODE_HOME pressed, exiting!\n");
		//exit(0);
		}
	
    if (mFocusView != NULL)
        {
        mFocusWindow = reinterpret_cast<SkOSWindow*>(mFocusView->getContext());

        if (mFocusWindow != NULL)
            {
            if (args->action == AKEY_EVENT_ACTION_DOWN)
                {
                mFocusWindow->handleKey(AndroidKeycodeToSkKey(args->keyCode));
                /* mFocusWindow->handleChar((SkUnichar) uni); */
                }
            else if (args->action == AKEY_EVENT_ACTION_UP)
                {
                mFocusWindow->handleKeyUp(AndroidKeycodeToSkKey(args->keyCode));
                }
            }
        }
    }

void SkiWinNotifyMotionCallback(const NotifyMotionArgs* args, void* context)
    {
    int32_t x = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_X));
    int32_t y = int32_t(args->pointerCoords[0].getAxisValue(AMOTION_EVENT_AXIS_Y));
    SkiWin* skiwin = reinterpret_cast<SkiWin*>(context);
    SkOSWindow* mFocusWindow = NULL;
    sp<SkiWinView> mFocusView = NULL;
    int32_t x0, y0;

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

    mFocusView = skiwin->updateFocusView (x, y);

    if (mFocusView != NULL)
        {
        mFocusWindow = reinterpret_cast<SkOSWindow*>(mFocusView->getContext());

        mFocusView->screenToViewSpace(x, y, &x0, &y0);

        if (mFocusWindow != NULL)
            mFocusWindow->handleClick(x0, y0, state);
        }
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

static const char gText[] =
    "When in the Course of human events it becomes necessary for one people "
    "to dissolve the political bands which have connected them with another "
    "and to assume among the powers of the earth, the separate and equal "
    "station to which the Laws of Nature and of Nature's God entitle them, "
    "a decent respect to the opinions of mankind requires that they should "
    "declare the causes which impel them to the separation.";

void SkiWin::drawImage(SkCanvas* canvas, const void* buffer, size_t size)
    {
    SkBitmap bitmap;
    SkPaint paint;
    SkRect r;
    SkMatrix m;

    SkImageDecoder::DecodeMemory(buffer, size, &bitmap);
    if (!bitmap.pixelRef())
        {
        return;
        }

    SkShader* s = SkShader::CreateBitmapShader(bitmap,
                  SkShader::kRepeat_TileMode,
                  SkShader::kRepeat_TileMode);
    paint.setShader(s)->unref();
    m.setTranslate(SkIntToScalar(250), SkIntToScalar(134));
    s->setLocalMatrix(m);

    r.set(SkIntToScalar(250),
          SkIntToScalar(134),
          SkIntToScalar(250 + 449),
          SkIntToScalar(134 + 701));
    paint.setFlags(2);

    canvas->drawBitmap(bitmap, 0, 0, &paint);
    }

void SkiWin::drawText(SkCanvas* canvas,
                      SkScalar w, SkScalar h,
                      SkColor fg, SkColor bg,
                      const char text[])
    {
    SkAutoCanvasRestore acr(canvas, true);

    canvas->clipRect(SkRect::MakeWH(w, h));
    canvas->drawColor(bg);
    SkScalar margin = 20;
    SkTextBox tbox;
    tbox.setMode(SkTextBox::kLineBreak_Mode);
    tbox.setBox(margin, margin,
                w - margin, h - margin);
    tbox.setSpacing(SkIntToScalar(3)/3, 0);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setLCDRenderText(true);
    paint.setColor(fg);
    tbox.setText(text, strlen(text), paint);

    for (int i = 9; i < 24; i += 2)
        {
        paint.setTextSize(SkIntToScalar(i));
        tbox.draw(canvas);
        canvas->translate(0, tbox.getTextHeight() + paint.getFontSpacing());
        }
    }

/* This routine returns the size of the file it is called with. */
int getFileSize(FILE *input)
    {
    int fileSizeBytes;
    fseek(input, 0, SEEK_END);
    fileSizeBytes = ftell(input);
    fseek(input, 0, SEEK_SET);
    return fileSizeBytes;
    }

/* This routine reads the entire file into memory. */

char * readWholeFile (const char * file_name, size_t * size)
    {
    unsigned s;
    char * contents;
    FILE * f;
    size_t bytes_read;
    int status;

    f = fopen (file_name, "rb");
    if (! f)
        {
        fprintf (stderr, "Could not open '%s': %s.\n", file_name,
                 strerror (errno));
        exit (EXIT_FAILURE);
        }
    s = getFileSize(f);
    contents = (char *)malloc (s + 1);
    if (! contents)
        {
        fprintf (stderr, "Not enough memory.\n");
        exit (EXIT_FAILURE);
        }
    bytes_read = fread (contents, sizeof (unsigned char), s, f);
    if (bytes_read != s)
        {
        fprintf (stderr, "Short read of '%s': expected %d bytes "
                 "but got %d: %s.\n", file_name, s, bytes_read,
                 strerror (errno));
        exit (EXIT_FAILURE);
        }
    status = fclose (f);
    if (status != 0)
        {
        fprintf (stderr, "Error closing '%s': %s.\n", file_name,
                 strerror (errno));
        exit (EXIT_FAILURE);
        }
    *size = bytes_read;
    return contents;
    }

bool SkiWin::android()
    {
    SkCanvas* contentCanvasTop;
    SkCanvas* contentCanvasMid;
    SkCanvas* contentCanvasBot;
    SkCanvas* titileCanvasTop;
    SkCanvas* titileCanvasBot;
    SkBitmap bitmap;
    size_t  bufferLen;
    char * buffer;
    size_t  imgBufLen;
    char * imgBuf;

    int fileno = 0;
    size_t  newScreenImgBufLen = 0;
    size_t  screenImgBufLen = 0;
    char * screenImgBuf = NULL;
    char filename[50];
    FILE *f;

    Rect rect(mWidth, mHeight);

    mWindowTop->resize(320, 150);
    mWindowMid->resize(320, 150);
    mWindowBot->resize(320, 150);
    mWindowMid->update(NULL);

    buffer = SkiWinURLResourceGet("www.baidu.com", &bufferLen);

    if (buffer == NULL) buffer = (char *)gText;

    imgBuf = SkiWinURLResourceGet("www.baidu.com/img/bdlogo.gif", &imgBufLen);

    do
        {
        mWindowTop->update(NULL);
        mWindowBot->update(NULL);

        titileCanvasTop = mTitleViewTop->lockCanvas(rect);
        if (titileCanvasTop)
            {
            SkPaint paint;
            const char * title = mWindowTop->getTitle();

            paint.setColor(SK_ColorWHITE);
            paint.setDither(true);
            paint.setAntiAlias(true);
            paint.setSubpixelText(true);
            paint.setLCDRenderText(true);
            paint.setTextSize(15);

            titileCanvasTop->clear(SK_ColorBLACK);

            titileCanvasTop->drawText(title, strlen(title), 15, 25, paint);
            }
        mTitleViewTop->unlockCanvasAndPost();

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

            //contentCanvasMid->drawRect(rect1, paint);
            //contentCanvasMid->drawBitmap(mWindowMid->getBitmap(), 0, 0);
            //contentCanvasMid->drawLine(100, 0, 100, 480, paint);
            drawText(contentCanvasMid, 320, 150, SK_ColorBLACK, SK_ColorWHITE, buffer);
            }
        mContentViewMid->unlockCanvasAndPost();

        titileCanvasBot = mTitleViewBot->lockCanvas(rect);
        if (titileCanvasBot)
            {
#ifdef DRAW_TITLE
            SkPaint paint;
            int remain;
            int ystart = 25;
            const char * title = mWindowBot->getTitle();

            paint.setColor(SK_ColorWHITE);
            paint.setDither(true);
            paint.setAntiAlias(true);
            paint.setSubpixelText(true);
            paint.setLCDRenderText(true);
            paint.setTextSize(10);

            titileCanvasBot->clear(SK_ColorBLACK);

            remain = strlen(title);

            while (remain > 10)
                {
                titileCanvasBot->drawText(title, 10, 2, ystart, paint);

                title += 10;
                ystart += 25;
                remain -= 10;
                }

            if (remain > 0)
                titileCanvasBot->drawText(title, 10, 2, ystart, paint);
#else
            {
            sprintf(filename, "/data/screenvideo/screen-%d.png", fileno++);
            screenImgBuf = readWholeFile(filename, &screenImgBufLen);
            if (screenImgBuf != NULL)
                {

                printf("Opened file %s with len %d buf %p\n", filename, screenImgBufLen, screenImgBuf);

                drawImage(titileCanvasBot, screenImgBuf, screenImgBufLen);
                
                free(screenImgBuf);
                }
            else
                fileno = 0;
            }
#endif
            }
        mTitleViewBot->unlockCanvasAndPost();

        contentCanvasBot = mContentViewBot->lockCanvas(rect);
        if (contentCanvasBot)
            {
            if (imgBuf == NULL)
                contentCanvasBot->drawBitmap(mWindowBot->getBitmap(), 0, 0);
            else
                drawImage(contentCanvasBot, imgBuf, imgBufLen);
            }
        mContentViewBot->unlockCanvasAndPost();

        usleep(100000);

        checkExit();
        }
    while (!exitPending());

    return false;
    }

sp<SkiWinView> SkiWin::updateFocusView(int x, int y)
    {
    if (mContentViewTop->isFocus(x, y) == true)
        mFocusView = mContentViewTop;
    else if (mContentViewBot->isFocus(x, y) == true)
        mFocusView = mContentViewBot;
    else
        mFocusView = NULL;

    return mFocusView;
    }

sp<SkiWinView> SkiWin::getFocusView()
    {
    return mFocusView;
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

