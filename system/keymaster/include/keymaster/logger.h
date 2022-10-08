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

#ifndef SYSTEM_KEYMASTER_LOGGER_H_
#define SYSTEM_KEYMASTER_LOGGER_H_

namespace keymaster {

class Logger {
  public:
    Logger() {}
    virtual ~Logger() {}
    virtual int debug(const char* fmt, ...) const = 0;
    virtual int info(const char* fmt, ...) const = 0;
    virtual int error(const char* fmt, ...) const = 0;
    virtual int severe(const char* fmt, ...) const = 0;

  private:
    // Disallow copying.
    Logger(const Logger&);
    void operator=(const Logger&);
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_LOGGER_H_
