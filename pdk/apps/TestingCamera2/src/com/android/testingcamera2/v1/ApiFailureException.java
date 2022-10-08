/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.testingcamera2.v1;

/**
 * Exception for the public API (mostly Camera2) failing in ways it shouldn't.
 */
class ApiFailureException extends Exception {
    public ApiFailureException(Throwable cause) {
        super(cause);
    }

    public ApiFailureException(String message, Throwable cause) {
        super(message, cause);
    }

    public ApiFailureException(String message) {
        super(message);
    }
}
