/*
 * Copyright 2014 The Android Open Source Project
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

#ifndef SYSTEM_KEYMASTER_OPERATION_H_
#define SYSTEM_KEYMASTER_OPERATION_H_

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <keymaster/google_keymaster_utils.h>
#include <keymaster/keymaster_defs.h>
#include <keymaster/logger.h>

namespace keymaster {

/**
 * Abstract base for all cryptographic operations.
 */
class Operation {
  public:
    Operation(keymaster_purpose_t purpose, const Logger& logger)
        : purpose_(purpose), logger_(logger) {}
    virtual ~Operation() {}

    keymaster_purpose_t purpose() const { return purpose_; }

    const Logger& logger() { return logger_; }

    virtual keymaster_error_t Begin() = 0;
    virtual keymaster_error_t Update(const Buffer& input, Buffer* output) = 0;
    virtual keymaster_error_t Finish(const Buffer& signature, Buffer* output) = 0;
    virtual keymaster_error_t Abort() = 0;

  private:
    const keymaster_purpose_t purpose_;
    const Logger& logger_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_OPERATION_H_
