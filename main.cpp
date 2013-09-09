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

#include <cutils/properties.h>

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <utils/Log.h>
#include <utils/threads.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
# include <sys/resource.h>
#endif

#include <SkTypeface.h>
#include <SkTemplates.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkEvent.h>
#include <SkCanvas.h>
#include <SkWindow.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include "SkiWin.h"

using namespace android;


///////////////////////////////////////////
/////////////// SkEvent impl //////////////
///////////////////////////////////////////

void SkEvent::SignalQueueTimer(SkMSec) {}

void SkEvent::SignalNonEmptyQueue() {}

///////////////////////////////////////////
///////////// SkOSWindow impl /////////////
///////////////////////////////////////////

void SkOSWindow::onSetTitle(const char title[])
    {
    printf("View Title %s\n", title);
    }

void SkOSWindow::onHandleInval(const SkIRect& rect)
    {
    }


void SkOSWindow::onPDFSaved(const char title[], const char desc[],
                            const char path[])
    {

    }

// ---------------------------------------------------------------------------

int main(int argc, char** argv)
    {
#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
#endif

    sp<ProcessState> proc(ProcessState::self());
    ProcessState::self()->startThreadPool();

    // create the SkiWin object
    sp<SkiWin> skiwin = new SkiWin();

    IPCThreadState::self()->joinThreadPool();

    return 0;
    }
