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
#include <SkiaSamples/SampleApp.h>

#include "SkiWinEventListener.h"
#include "SkiWinView.h"

class SkBitmap;
class SkCanvas;

namespace android
{

class Surface;
class SurfaceComposerClient;
class SurfaceControl;

// ---------------------------------------------------------------------------

class SkiWin : public Thread, public IBinder::DeathRecipient
    {
    public:
        SkiWin();
        virtual     ~SkiWin();
        
        SkOSWindow* mWindowTop;
        SkOSWindow* mWindowBot;

    private:
        virtual bool        threadLoop();
        virtual status_t    readyToRun();
        virtual void        onFirstRef();
        virtual void        binderDied(const wp<IBinder>& who);

        bool android();

        void checkExit();

        sp<SurfaceComposerClient>       mSession;

        int         mWidth;
        int         mHeight;      

        sp<SkiWinView> mTitleView;
        sp<SkiWinView> mContentViewTop;
        sp<SkiWinView> mContentViewMid;
        sp<SkiWinView> mContentViewBot;
        
    };

// ---------------------------------------------------------------------------

}; // namespace android

#endif // ANDROID_SKIWIN_H
