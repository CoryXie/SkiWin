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

#define LOG_TAG "SkiWinView"

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

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglext.h>

#include "SkiWinView.h"

namespace android
{

SkiWinView::SkiWinView(sp<SurfaceComposerClient> & client, 
                       const String8 & name,
                       int x, int y, int w, int h, int l) : 
                       mSurfaceComposerClient(client),
                       mLeft(x), mTop(y), mWidth(w), mHeight(h), mLayer(l)
    {
    mSurfaceControl = mSurfaceComposerClient->createSurface(name,
                                 w, h, PIXEL_FORMAT_RGB_565);
    
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->setLayer(mLayer);
    mSurfaceControl->setPosition(x, y);
    mSurfaceControl->show();
    SurfaceComposerClient::closeGlobalTransaction();
    
    mSurface = mSurfaceControl->getSurface();

    mContext = NULL;
    }

SkiWinView::~SkiWinView()
    {
    mSurfaceComposerClient = NULL;
    mSurfaceControl = NULL;
    mSurface = NULL;
    }

void SkiWinView::clear()
    {
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->clear();
    SurfaceComposerClient::closeGlobalTransaction();
    }

void SkiWinView::hide()
    {
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->hide();
    SurfaceComposerClient::closeGlobalTransaction();
    }

void SkiWinView::show()
    {
    SurfaceComposerClient::openGlobalTransaction();
    mSurfaceControl->show();
    SurfaceComposerClient::closeGlobalTransaction();
    }

SkBitmap::Config SkiWinView::convertPixelFormat(PixelFormat format)
    {
    /* 
     * Note: If PIXEL_FORMAT_RGBX_8888 means that all alpha bytes are 0xFF, 
     * then we can map to SkBitmap::kARGB_8888_Config, and optionally call
     * bitmap.setIsOpaque(true) on the resulting SkBitmap (as an accelerator)
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

SkCanvas* SkiWinView::lockCanvas(const Rect& dirtyRect)
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

    status_t err = mSurface->lock(&info, &dirtyRegion);
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
    
    bitmap.eraseARGB(0, 0, 0, 0);

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

void SkiWinView::unlockCanvasAndPost()
    {

    // detach the canvas from the surface
    mCanvas.restoreToCount(mCanvasSaveCount);

    mCanvas.setBitmapDevice(SkBitmap());

    // unlock surface
    status_t err = mSurface->unlockAndPost();

    assert(err == 0);
    }

bool SkiWinView::isFocus(int x, int y)
    {
    if ((x >= mLeft) && (x <= (mLeft + mWidth)) &&
        (y >= mTop) && (y <= (mTop + mHeight)))
        return true;
    else
        return false;
    }

void SkiWinView::setContext(void * ctx)
    {
    mContext = ctx;
    }

void* SkiWinView::getContext()
    {
    return mContext;
    }

void SkiWinView::screenToViewSpace (int x, int y, int *x0, int* y0)
    {
    *x0 = (x - mLeft);
    *y0 = (y - mTop);
    }

void SkiWinView::viewToScreenSpace (int x0, int y0, int *x, int* y)
    {
    *x = (x0 + mLeft);
    *y = (y0 + mTop);
    }

}; // namespace android

