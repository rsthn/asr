
#include <asr/buffer>
#include <cstring>

namespace asr {

    Buffer::Buffer (int buffer_size)
    {
        if (buffer_size < 16) buffer_size = 16;

        this->buffer_size = buffer_size;
        this->buffer_level = 0;
        this->offset_top = 0;
        this->offset_bottom = 0;

        read_offset = 0;
        write_offset = 0;
        update_eof();

        this->data = (char *)asr::alloc(this->buffer_size);
        this->is_owner = true;
    }

    Buffer::Buffer (char *data, int buffer_size, int buffer_level, bool is_owner)
    {
        if (buffer_size < 16) buffer_size = 16;

        this->buffer_size = buffer_size;
        this->buffer_level = buffer_level;
        this->offset_top = 0;
        this->offset_bottom = buffer_level;

        read_offset = 0;
        write_offset = 0;
        update_eof();

        this->is_owner = is_owner;
        this->data = data;
    }

    Buffer::~Buffer()
    {
        if (is_owner && data != nullptr) {
            asr::dealloc(data);
            data = nullptr;
        }
    }

    void Buffer::flush()
    {
        while (bytes_available() > 0) {
            if (!drain_request(bytes_available()))
                break;
        }
        update_eof();
    }

    int Buffer::fill (const char *data, int length)
    {
        if (data == nullptr || length <= 0)
            return 0;

        const char *input_data = data;
        int bytes_written = 0;

        while (length > 0)
        {
            // Calculate amount of free space and trigger drain_request if required.
            int num_bytes = space_available();
            if (!num_bytes) {
                drain_request(bytes_available());
                num_bytes = space_available();
                if (!num_bytes) break;
            }

            // Calculate actual number of bytes to write this cycle.
            num_bytes = length < num_bytes ? length : num_bytes;

            // Copy data normally or in two parts if there is a buffer-overrun.
            if (offset_bottom + num_bytes > buffer_size)
            {
                int remaining = buffer_size - offset_bottom;

                memcpy (&this->data[offset_bottom], data, remaining);
                memcpy (&this->data[0], data+remaining, num_bytes-remaining);
            }
            else
            {
                memcpy (&this->data[offset_bottom], data, num_bytes);
            }

            offset_bottom += num_bytes;
            buffer_level += num_bytes;

            if (offset_bottom >= buffer_size) 
                offset_bottom -= buffer_size;

            length -= num_bytes;
            data += num_bytes;
            bytes_written += num_bytes;
        }

        update_eof();

        if (bytes_written)
            on_level_filled (input_data, bytes_written);

        write_offset += bytes_written;
        return bytes_written;
    }

    int Buffer::drain (char *data, int length, bool release_space)
    {
        if (length <= 0)
            return 0;

        if (release_space == false && bytes_available() < length)
        {
            if (space_available() < length - bytes_available())
                return 0;

            if (!fill_request(length - bytes_available())) {
                update_eof();
                return 0;
            }
        }

        int _buffer_level = buffer_level;
        int _offset_top = offset_top;
        int bytes_read = 0;

        while (length > 0)
        {
            // Calculate amount of bytes available and trigger `fill_request` if required.
            int num_bytes = bytes_available();
            if (!num_bytes) {
                fill_request(1, length);
                num_bytes = bytes_available();
                if (!num_bytes) break;
            }

            // Calculate actual number of bytes to read this cycle.
            num_bytes = length < num_bytes ? length : num_bytes;

            // Copy data normally or in two parts if there is a buffer-overrun.
            if (data != nullptr)
            {
                if (offset_top + num_bytes > buffer_size) {
                    int remaining = buffer_size - offset_top;
                    memcpy (data, &this->data[offset_top], remaining);
                    memcpy (data+remaining, &this->data[0], num_bytes-remaining);
                }
                else {
                    memcpy (data, &this->data[offset_top], num_bytes);
                }

                data += num_bytes;
            }

            offset_top += num_bytes;
            buffer_level -= num_bytes;

            if (offset_top >= buffer_size) 
                offset_top -= buffer_size;

            length -= num_bytes;
            bytes_read += num_bytes;
        }

        if (!release_space) {
            buffer_level = _buffer_level;
            offset_top = _offset_top;
        }
        else
            read_offset += bytes_read;

        update_eof(bytes_read);
        return bytes_read;
    }

    bool Buffer::write (const char *data, int length)
    {
        if (data == nullptr || length < 0)
            return false;

        if (!length) length = strlen(data);
        if (!length) return true;

        if (space_available() < length)
        {
            if (!drain_request(length, 0, true))
                return false;
        }

        return fill(data,length) == length;
    }

    char *Buffer::read (int length, char *buff, bool release_space, int extra_bytes)
    {
        if (length <= 0) return nullptr;

        if (release_space && bytes_available() < length) {
            if (!fill_request(length-bytes_available(), 0, true))
                return nullptr;
        }

        bool allocated = false;
        if (buff == nullptr) {
            buff = (char *)asr::alloc (length + extra_bytes);
            if (buff == nullptr) return nullptr;
            allocated = true;
        }

        if (drain(buff, length, release_space) != length) {
            if (allocated) asr::dealloc(buff);
            return nullptr;
        }

        if (extra_bytes > 0) buff[length] = '\0';
        return buff;
    }

    bool Buffer::write_uint8 (int value)
    {
        char tmp[1];
        write_uint8_to(tmp, value);
        return fill(tmp, 1) == 1;
    }

    void Buffer::write_uint8_to (char *buff, int value)
    {
        buff[0] = value;
    }

    bool Buffer::write_uint16 (int value)
    {
        char tmp[2];
        write_uint16_to(tmp, value);
        return fill(tmp, 2) == 2;
    }

    void Buffer::write_uint16_to (char *buff, int value)
    {
        buff[0] = value & 0xFF;
        buff[1] = (value >> 8) & 0xFF;
    }

    bool Buffer::write_uint16be (int value)
    {
        char tmp[2];
        write_uint16be_to(tmp, value);
        return fill(tmp, 2) == 2;
    }

    void Buffer::write_uint16be_to (char *buff, int value)
    {
        buff[1] = value & 0xFF;
        buff[0] = (value >> 8) & 0xFF;
    }

    bool Buffer::write_uint32 (int value)
    {
        char tmp[4];
        write_uint32_to(tmp, value);
        return fill(tmp, 4) == 4;
    }

    void Buffer::write_uint32_to (char *buff, int value)
    {
        buff[0] = value & 0xFF;
        buff[1] = (value >> 8) & 0xFF;
        buff[2] = (value >> 16) & 0xFF;
        buff[3] = (value >> 24) & 0xFF;
    }

    bool Buffer::write_uint32be (int value)
    {
        char tmp[4];
        write_uint32be_to(tmp, value);
        return fill(tmp, 4) == 4;
    }

    void Buffer::write_uint32be_to (char *buff, int value)
    {
        buff[3] = value & 0xFF;
        buff[2] = (value >> 8) & 0xFF;
        buff[1] = (value >> 16) & 0xFF;
        buff[0] = (value >> 24) & 0xFF;
    }

    bool Buffer::write_str (const char *value, int length)
    {
        if (value == nullptr) return false;
        if (length == -1) length = strlen(value);
        if (length > 255) return false;
        if (!write_uint8(length)) return false;
        return write(value, length) == length;
    }

    /* ********** */
    int Buffer::read_uint8 (bool peek) {
        char tmp[1];
        if (drain(tmp, 1, !peek) != 1) return 0;
        return read_uint8_from(tmp);
    }

    int Buffer::read_uint8_from (char *buff) {
        return (unsigned char)buff[0];
    }

    int Buffer::read_int8 (bool peek) {
        char tmp[1];
        if (drain(tmp, 1, !peek) != 1) return 0;
        return read_int8_from(tmp);
    }

    int Buffer::read_int8_from (char *buff) {
        return (char)buff[0];
    }

    /* ********** */
    int Buffer::read_uint16 (bool peek) {
        char tmp[2];
        if (drain(tmp, 2, !peek) != 2) { return 0; }
        return read_uint16_from(tmp);
    }

    int Buffer::read_uint16_from (char *buff) {
        return (((unsigned char)buff[1]) << 8) | (unsigned char)buff[0];
    }

    int Buffer::read_int16 (bool peek) {
        char tmp[2];
        if (drain(tmp, 2, !peek) != 2) { return 0; }
        return read_int16_from(tmp);
    }

    int Buffer::read_int16_from (char *buff) {
        return (int16_t)read_uint16_from(buff);
    }

    int Buffer::read_uint16be (bool peek) {
        char tmp[2];
        if (drain(tmp, 2, !peek) != 2) { return 0; }
        return read_uint16be_from(tmp);
    }

    int Buffer::read_uint16be_from (char *buff) {
        return (((unsigned char)buff[0]) << 8) | (unsigned char)buff[1];
    }

    int Buffer::read_int16be (bool peek) {
        char tmp[2];
        if (drain(tmp, 2, !peek) != 2) { return 0; }
        return read_int16be_from(tmp);
    }

    int Buffer::read_int16be_from (char *buff) {
        return (int16_t)read_uint16be_from(buff);
    }

    /* ********** */
    int Buffer::read_uint32 (bool peek) {
        char tmp[4];
        if (drain(tmp, 4, !peek) != 4) { return 0; }
        return read_uint32_from(tmp);
    }

    int Buffer::read_uint32_from (char *buff) {
        int i = (unsigned char)buff[3];
        i <<= 8; i |= (unsigned char)buff[2];
        i <<= 8; i |= (unsigned char)buff[1];
        i <<= 8; i |= (unsigned char)buff[0];
        return i;
    }

    int Buffer::read_int32 (bool peek) {
        char tmp[4];
        if (drain(tmp, 4, !peek) != 4) { return 0; }
        return read_int32_from(tmp);
    }

    int Buffer::read_int32_from (char *buff) {
        return (int32_t)read_uint32_from(buff);
    }

    int Buffer::read_uint32be (bool peek) {
        char tmp[4];
        if (drain(tmp, 4, !peek) != 4) { return 0; }
        return read_uint32be_from(tmp);
    }

    int Buffer::read_uint32be_from (char *buff) {
        int i = (unsigned char)buff[0];
        i <<= 8; i |= (unsigned char)buff[1];
        i <<= 8; i |= (unsigned char)buff[2];
        i <<= 8; i |= (unsigned char)buff[3];
        return i;
    }

    int Buffer::read_int32be (bool peek) {
        char tmp[4];
        if (drain(tmp, 4, !peek) != 4) { return 0; }
        return read_int32be_from(tmp);
    }

    int Buffer::read_int32be_from (char *buff) {
        return (int32_t)read_uint32be_from(buff);
    }

    /* ********** */
    char *Buffer::read_line (char *buffer, int length, char nl)
    {
        int n = 0;

        if (buffer == nullptr || length-- <= 0)
            return nullptr;

        while (!_eof && n < length) {
            int ch = read_uint8();
            if (ch == '\n') break;
            buffer[n++] = ch;
        }

        if (_eof && n == 0) {
            return nullptr;
        }

        buffer[n] = '\0';
        return buffer;
    }

};
