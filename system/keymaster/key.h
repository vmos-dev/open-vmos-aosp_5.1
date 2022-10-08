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

#ifndef SYSTEM_KEYMASTER_KEY_H_
#define SYSTEM_KEYMASTER_KEY_H_

#include <keymaster/authorization_set.h>
#include <keymaster/keymaster_defs.h>
#include <keymaster/logger.h>

namespace keymaster {

class KeyBlob;
class Operation;

class Key {
  public:
    static Key* CreateKey(const KeyBlob& blob, const Logger& logger, keymaster_error_t* error);
    static Key* GenerateKey(const AuthorizationSet& key_description, const Logger& logger,
                            keymaster_error_t* error);
    static Key* ImportKey(const AuthorizationSet& key_description,
                          keymaster_key_format_t key_format, const uint8_t* key_data,
                          size_t key_data_length, const Logger& logger, keymaster_error_t* error);

    virtual ~Key() {}
    virtual Operation* CreateOperation(keymaster_purpose_t purpose, keymaster_error_t* error) = 0;

    /**
     * Return a copy of raw key material, in the key's preferred binary format.
     */
    virtual keymaster_error_t key_material(UniquePtr<uint8_t[]>*, size_t* size) const = 0;

    /**
     * Return a copy of raw key material, in the specified format.
     */
    virtual keymaster_error_t formatted_key_material(keymaster_key_format_t format,
                                                     UniquePtr<uint8_t[]>* material,
                                                     size_t* size) const = 0;

    const AuthorizationSet& authorizations() const { return authorizations_; }

  protected:
    Key(const KeyBlob& blob, const Logger& logger);
    Key(const AuthorizationSet& authorizations, const Logger& logger)
        : logger_(logger), authorizations_(authorizations) {}

    const Logger& logger_;

  private:
    AuthorizationSet authorizations_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_KEY_H_
