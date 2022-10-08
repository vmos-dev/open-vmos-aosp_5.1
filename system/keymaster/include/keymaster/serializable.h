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

#ifndef SYSTEM_KEYMASTER_SERIALIZABLE_H_
#define SYSTEM_KEYMASTER_SERIALIZABLE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <cstddef>

#include <UniquePtr.h>

namespace keymaster {

class Serializable {
  public:
    Serializable() {}
    virtual ~Serializable() {}

    /**
     * Return the size of the serialized representation of this object.
     */
    virtual size_t SerializedSize() const = 0;

    /**
     * Serialize this object into the provided buffer.  Returns a pointer to the byte after the last
     * written.  Will not write past \p end, which should point to \p buf + size of the buffer
     * (i.e. one past the end of the buffer).
     */
    virtual uint8_t* Serialize(uint8_t* buf, const uint8_t* end) const = 0;

    /**
     * Deserialize from the provided buffer, copying the data into newly-allocated storage.  Returns
     * true if successful, and advances *buf past the bytes read.
     */
    virtual bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end) = 0;

  private:
    // Disallow copying and assignment.
    Serializable(const Serializable&);
    void operator=(const Serializable&);
};

/*
 * Utility functions for writing Serialize() methods
 */

/**
 * Append a byte array to a buffer.  Note that by itself this function isn't very useful, because it
 * provides no indication in the serialized buffer of what the array size is.  For writing arrays,
 * see \p append_size_and_data_to_buf().
 *
 * Returns a pointer to the first byte after the data written.
 */
uint8_t* append_to_buf(uint8_t* buf, const uint8_t* end, const void* data, size_t data_len);

/**
 * Append some type of value convertible to a uint32_t to a buffer.  This is primarily used for
 * writing enumerated values, and uint32_ts.
 *
 * Returns a pointer to the first byte after the data written.
 */
template <typename T>
inline uint8_t* append_uint32_to_buf(uint8_t* buf, const uint8_t* end, T value) {
    uint32_t val = static_cast<uint32_t>(value);
    return append_to_buf(buf, end, &val, sizeof(val));
}

/**
 * Append a uint64_t to a buffer.  Returns a pointer to the first byte after the data written.
 */
inline uint8_t* append_uint64_to_buf(uint8_t* buf, const uint8_t* end, uint64_t value) {
    return append_to_buf(buf, end, &value, sizeof(value));
}

/**
 * Appends a byte array to a buffer, prefixing it with a 32-bit size field.  Returns a pointer to
 * the first byte after the data written.
 *
 * See copy_size_and_data_from_buf().
 */
inline uint8_t* append_size_and_data_to_buf(uint8_t* buf, const uint8_t* end, const void* data,
                                            size_t data_len) {
    buf = append_uint32_to_buf(buf, end, data_len);
    return append_to_buf(buf, end, data, data_len);
}

/**
 * Appends an array of values that are convertible to uint32_t as uint32ts to a buffer, prefixing a
 * count so deserialization knows how many values to read.
 *
 * See copy_uint32_array_from_buf().
 */
template <typename T>
inline uint8_t* append_uint32_array_to_buf(uint8_t* buf, const uint8_t* end, const T* data,
                                           size_t count) {
    buf = append_uint32_to_buf(buf, end, count);
    for (size_t i = 0; i < count; ++i)
        buf = append_uint32_to_buf(buf, end, static_cast<uint32_t>(data[i]));
    return buf;
}

/*
 * Utility functions for writing Deserialize() methods.
 */

/**
 * Copy \p size bytes from \p *buf_ptr into \p dest.  If there are fewer than \p size bytes to read,
 * returns false.  Advances *buf_ptr to the next byte to be read.
 */
bool copy_from_buf(const uint8_t** buf_ptr, const uint8_t* end, void* dest, size_t size);

/**
 * Extracts a uint32_t size from *buf_ptr, placing it in \p *size, and then reads *size bytes from
 * *buf_ptr, placing them in newly-allocated storage in *dest.  If there aren't enough bytes in
 * *buf_ptr, returns false.  Advances \p *buf_ptr to the next byte to be read.
 *
 * See \p append_size_and_data_to_buf().
 */
bool copy_size_and_data_from_buf(const uint8_t** buf_ptr, const uint8_t* end, size_t* size,
                                 UniquePtr<uint8_t[]>* dest);

/**
 * Copies a value convertible from uint32_t from \p *buf_ptr.  Returns false if there are less than
 * four bytes remaining in \p *buf_ptr.  Advances \p *buf_ptr to the next byte to be read.
 */
template <typename T>
inline bool copy_uint32_from_buf(const uint8_t** buf_ptr, const uint8_t* end, T* value) {
    uint32_t val;
    if (!copy_from_buf(buf_ptr, end, &val, sizeof(val)))
        return false;
    *value = static_cast<T>(val);
    return true;
}

/**
 * Copies a uint64_t from \p *buf_ptr.  Returns false if there are less than eight bytes remaining
 * in \p *buf_ptr.  Advances \p *buf_ptr to the next byte to be read.
 */
inline bool copy_uint64_from_buf(const uint8_t** buf_ptr, const uint8_t* end, uint64_t* value) {
    return copy_from_buf(buf_ptr, end, value, sizeof(*value));
}

/**
 * Copies an array of values convertible to uint32_t from \p *buf_ptr, first reading a count of
 * values to read. The count is returned in \p *count and the values returned in newly-allocated
 * storage at *data.  Returns false if there are insufficient bytes at \p *buf_ptr.  Advances \p
 * *buf_ptr to the next byte to be read.
 */
template <typename T>
inline bool copy_uint32_array_from_buf(const uint8_t** buf_ptr, const uint8_t* end,
                                       UniquePtr<T[]>* data, size_t* count) {
    if (!copy_uint32_from_buf(buf_ptr, end, count) || *buf_ptr + *count * sizeof(uint32_t) > end)
        return false;
    data->reset(new T[*count]);
    for (size_t i = 0; i < *count; ++i)
        if (!copy_uint32_from_buf(buf_ptr, end, &(*data)[i]))
            return false;
    return true;
}

/**
 * A simple buffer that supports reading and writing.  Manages its own memory.
 */
class Buffer : public Serializable {
  public:
    Buffer() : buffer_(NULL), buffer_size_(0), read_position_(0), write_position_(0) {}
    Buffer(size_t size) : buffer_(NULL) { Reinitialize(size); }
    Buffer(const void* buf, size_t size) : buffer_(NULL) { Reinitialize(buf, size); }

    // Grow the buffer so that at least \p size bytes can be written.
    bool reserve(size_t size);

    bool Reinitialize(size_t size);
    bool Reinitialize(const void* buf, size_t size);

    // Reinitialize with a copy of the provided buffer's readable data.
    bool Reinitialize(const Buffer& buffer) {
        return Reinitialize(buffer.peek_read(), buffer.available_read());
    }

    void Clear();

    size_t available_write() const;
    size_t available_read() const;
    size_t buffer_size() const { return buffer_size_; }

    bool write(const uint8_t* src, size_t write_length);
    bool read(uint8_t* dest, size_t read_length);
    const uint8_t* peek_read() const { return buffer_.get() + read_position_; }
    void advance_read(int distance) { read_position_ += distance; }
    uint8_t* peek_write() { return buffer_.get() + write_position_; }
    void advance_write(int distance) { write_position_ += distance; }

    size_t SerializedSize() const;
    uint8_t* Serialize(uint8_t* buf, const uint8_t* end) const;
    bool Deserialize(const uint8_t** buf_ptr, const uint8_t* end);

  private:
    // Disallow copy construction and assignment.
    void operator=(const Buffer& other);
    Buffer(const Buffer&);

    UniquePtr<uint8_t[]> buffer_;
    size_t buffer_size_;
    int read_position_;
    int write_position_;
};

}  // namespace keymaster

#endif  // SYSTEM_KEYMASTER_SERIALIZABLE_H_
