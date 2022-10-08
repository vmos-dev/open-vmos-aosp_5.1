/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

#include "DebugUtils.h"

#include "utils/Debug.h"




namespace Ti {




// shared const buffer with spaces for indentation string
extern const char sIndentStringBuffer[] =
        "                                                                "
        "                                                                ";
template class android::CompileTimeAssert<sizeof(sIndentStringBuffer) - 1 == kIndentStringMaxLength>;




static const int kDebugThreadInfoGrowSize = 16;




Debug Debug::sInstance;




Debug::Debug()
{
    grow();
}


void Debug::grow()
{
    android::AutoMutex locker(mMutex);
    (void)locker;

    const int size = kDebugThreadInfoGrowSize;

    const int newSize = (mData.get() ? mData->threads.size() : 0) + size;

    Data * const newData = new Data;
    newData->threads.setCapacity(newSize);

    // insert previous thread info pointers
    if ( mData.get() )
        newData->threads.insertVectorAt(mData->threads, 0);

    // populate data with new thread infos
    for ( int i = 0; i < size; ++i )
        newData->threads.add(new ThreadInfo);

    // replace old data with new one
    mData = newData;
}


Debug::ThreadInfo * Debug::registerThread(Data * const data, const int32_t threadId)
{
    const int size = data->threads.size();
    for ( int i = 0; i < size; ++i )
    {
        ThreadInfo * const threadInfo = data->threads.itemAt(i);
        if ( android_atomic_acquire_cas(0, threadId, &threadInfo->threadId) == 0 )
            return threadInfo;
    }

    // failed to find empty slot for thread
    return 0;
}




} // namespace Ti
