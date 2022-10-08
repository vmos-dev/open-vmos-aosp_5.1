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

#ifndef TI_UTILS_COMMON_H
#define TI_UTILS_COMMON_H

#include <android/api-level.h>
#include <android/log.h>



namespace Ti {




// default float point type
typedef float real;




template <typename T>
int floor(T x);

template <typename T>
int round(T x);

template <typename T>
const T & min(const T & a, const T & b);

template <typename T>
const T & max(const T & a, const T & b);

template <typename T>
const T & bound(const T & min, const T & x, const T & max);

template <typename T>
T abs(const T & x);




template <typename T>
inline int floor(const T x) {
    return static_cast<int>(x);
}

template <typename T>
inline int round(const T x) {
    if ( x >= 0 ) {
        return floor(x + T(0.5));
    } else {
        return floor(x - floor(x - T(1)) + T(0.5)) + floor(x - T(1));
    }
}

template <typename T>
inline const T & min(const T & a, const T & b) {
    return a < b ? a : b;
}

template <typename T>
inline const T & max(const T & a, const T & b) {
    return a < b ? b : a;
}

template <typename T>
inline const T & bound(const T & min, const T & x, const T & max) {
    return x < min ? min : x > max ? max : x;
}

template <typename T>
inline T abs(const T & x) {
    return x >= 0 ? x : -x;
}




} // namespace Ti




#endif // TI_UTILS_COMMON_H
