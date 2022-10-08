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

#include <keymaster/serializable.h>
#include <keymaster/google_keymaster_utils.h>

namespace keymaster {

uint8_t* append_to_buf(uint8_t* buf, const uint8_t* end, const void* data, size_t data_len) {
    if (buf + data_len <= end)
        memcpy(buf, data, data_len);
    return buf + data_len;
}

bool copy_from_buf(const uint8_t** buf_ptr, const uint8_t* end, void* dest, size_t size) {
    if (end < *buf_ptr + size)
        return false;
    memcpy(dest, *buf_ptr, size);
    *buf_ptr += size;
    return true;
}

bool copy_size_and_data_from_buf(const uint8_t** buf_ptr, const uint8_t* end, size_t* size,
                                 UniquePtr<uint8_t[]>* dest) {
    if (!copy_uint32_from_buf(buf_ptr, end, size) || *buf_ptr + *size > end) {
        return false;
    }
    if (*size == 0) {
        dest->reset();
        return true;
    }
    dest->reset(new uint8_t[*size]);
    if (dest->get() == NULL)
        return false;
    return copy_from_buf(buf_ptr, end, dest->get(), *size);
}

bool Buffer::reserve(size_t size) {
    if (available_write() < size) {
        size_t new_size = buffer_size_ + size - available_write();
        uint8_t* new_buffer = new uint8_t[new_size];
        if (!new_buffer)
            return false;
        memcpy(new_buffer, buffer_.get() + read_position_, available_read());
        memset_s(buffer_.get(), 0, buffer_size_);
        buffer_.reset(new_buffer);
        buffer_size_ = new_size;
        write_position_ -= read_position_;
        read_position_ = 0;
    }
    return true;
}

bool Buffer::Reinitialize(size_t size) {
    Clear();
    buffer_.reset(new uint8_t[size]);
    if (buffer_.get() == NULL)
        return false;
    buffer_size_ = size;
    read_position_ = 0;
    write_position_ = 0;
    return true;
}

bool Buffer::Reinitialize(const void* data, size_t data_len) {
    Clear();
    buffer_.reset(new uint8_t[data_len]);
    if (buffer_.get() == NULL)
        return false;
    buffer_size_ = data_len;
    memcpy(buffer_.get(), data, data_len);
    read_position_ = 0;
    write_position_ = buffer_size_;
    return true;
}

size_t Buffer::available_write() const {
    return buffer_size_ - write_position_;
}

size_t Buffer::available_read() const {
    return write_position_ - read_position_;
}

bool Buffer::write(const uint8_t* src, size_t write_length) {
    if (available_write() < write_length)
        return false;
    memcpy(buffer_.get() + write_position_, src, write_length);
    write_position_ += write_length;
    return true;
}

bool Buffer::read(uint8_t* dest, size_t read_length) {
    if (available_read() < read_length)
        return false;
    memcpy(dest, buffer_.get() + read_position_, read_length);
    read_position_ += read_length;
    return true;
}

size_t Buffer::SerializedSize() const {
    return sizeof(uint32_t) + available_read();
}

uint8_t* Buffer::Serialize(uint8_t* buf, const uint8_t* end) const {
    return append_size_and_data_to_buf(buf, end, peek_read(), available_read());
}

bool Buffer::Deserialize(const uint8_t** buf_ptr, const uint8_t* end) {
    Clear();
    if (!copy_size_and_data_from_buf(buf_ptr, end, &buffer_size_, &buffer_)) {
        buffer_.reset();
        buffer_size_ = 0;
        return false;
    }
    write_position_ = buffer_size_;
    return true;
}

void Buffer::Clear() {
    if (buffer_.get())
        memset_s(buffer_.get(), 0, buffer_size_);
    buffer_.reset();
    read_position_ = 0;
    write_position_ = 0;
    buffer_size_ = 0;
}

}  // namespace keymaster
