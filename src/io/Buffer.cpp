
#include <asr/io/Buffer>
#include <asr/defs>
#include <string.h>

namespace asr {
namespace io {

    /**
     * @brief Constructs the object and allocates a buffer of the given size. If size of zero is provided no buffer will be
     * allocated, that would be useful for derived classes implementing unbuffered I/O.
     * 
     * @param bufferSize
     */
    Buffer::Buffer (int bufferSize)
    {
        if (bufferSize < 16) bufferSize = 16;

        this->bufferSize = bufferSize;
        this->bufferLevel = 0;
        this->offsetTop = 0;
        this->offsetBottom = 0;
        updateEOF();

        this->isOwner = true;
        this->data = (char *)asr::alloc (this->bufferSize);
    }

    /**
     * @brief Constructs a buffer that reads/writes to the specified array.
     *
     * @param data Buffer data.
     * @param bufferSize Size of the buffer.
     * @param bufferLevel Current buffer level.
     * @param isOwner Indicates if the object is the owner of the buffer data so that the destructor will deallocate it.
     */
    Buffer::Buffer (char *data, int bufferSize, int bufferLevel, bool isOwner)
    {
        if (bufferSize < 16) bufferSize = 16;

        this->bufferSize = bufferSize;
        this->bufferLevel = bufferLevel;
        this->offsetTop = 0;
        this->offsetBottom = bufferLevel;
        updateEOF();

        this->isOwner = isOwner;
        this->data = data;
    }

    /**
     * @brief Responsible for deallocating the data buffer.
     */
    Buffer::~Buffer()
    {
        if (isOwner && data != nullptr) {
            asr::dealloc (data);
            data = nullptr;
        }
    }

    /**
     * @brief Drains the buffer contents.
     */
    void Buffer::flush()
    {
        while (bytesAvailable() > 0) {
            if (!drainRequest(bytesAvailable()))
                break;
        }

        updateEOF();
    }

    /**
     * @brief Writes data to the buffer and returns the amount of bytes written.
     *
     * @param data Data to be written.
     * @param length Number of bytes to be written.
     * @return Number of bytes written.
     */
    int Buffer::fill (const char *data, int length)
    {
        if (data == nullptr || length <= 0)
            return 0;

        const char *inputData = data;
        int bytesWritten = 0;

        while (length > 0)
        {
            // Calculate amount of free space and trigger drainRequest if required.
            int numBytes = spaceAvailable();
            if (!numBytes) {
                drainRequest(bytesAvailable());
                numBytes = spaceAvailable();
                if (!numBytes) break;
            }

            // Calculate actual number of bytes to write this cycle.
            numBytes = length < numBytes ? length : numBytes;

            // Copy data normally or in two parts if there is a buffer-overrun.
            if (offsetBottom + numBytes > bufferSize)
            {
                int remaining = bufferSize - offsetBottom;

                memcpy (&this->data[offsetBottom], data, remaining);
                memcpy (&this->data[0], data+remaining, numBytes-remaining);
            }
            else
            {
                memcpy (&this->data[offsetBottom], data, numBytes);
            }

            offsetBottom += numBytes;
            bufferLevel += numBytes;

            if (offsetBottom >= bufferSize) 
                offsetBottom -= bufferSize;

            length -= numBytes;
            data += numBytes;
            bytesWritten += numBytes;
        }

        updateEOF();
        if (bytesWritten) onDataFilled (inputData, bytesWritten);
        return bytesWritten;
    }

    /**
    **	Reads data (drain-buffer) into the given buffer and returns the actual number of bytes read.
    **
    **	- If releaseSpace is false the data from the buffer will not be removed and the specified amount of bytes must be in the buffer (or at
    **	  least be able to fit in the free space by filling it using fillRequest) or it will return 0.
    **
    **	- If the data parameter is nullptr, data will be read but not stored in the output buffer.
    */
    int Buffer::drain (char *data, int length, bool releaseSpace)
    {
        if (length <= 0)
            return 0;

        if (releaseSpace == false && bytesAvailable() < length)
        {
            if (spaceAvailable() < length - bytesAvailable())
                return 0;

            if (!fillRequest(length - bytesAvailable())) {
                updateEOF();
                return 0;
            }
        }

        int _bufferLevel = bufferLevel;
        int _offsetTop = offsetTop;
        int bytesRead = 0;

        while (length > 0)
        {
            // Calculate amount of bytes available and trigger `fillRequest` if required.
            int numBytes = bytesAvailable();
            if (!numBytes) {
                fillRequest(1, length);
                numBytes = bytesAvailable();
                if (!numBytes) break;
            }

            // Calculate actual number of bytes to read this cycle.
            numBytes = length < numBytes ? length : numBytes;

            // Copy data normally or in two parts if there is a buffer-overrun.
            if (data != nullptr)
            {
                if (offsetTop + numBytes > bufferSize) {
                    int remaining = bufferSize - offsetTop;
                    memcpy (data, &this->data[offsetTop], remaining);
                    memcpy (data+remaining, &this->data[0], numBytes-remaining);
                }
                else {
                    memcpy (data, &this->data[offsetTop], numBytes);
                }

                data += numBytes;
            }

            offsetTop += numBytes;
            bufferLevel -= numBytes;

            if (offsetTop >= bufferSize) 
                offsetTop -= bufferSize;

            length -= numBytes;
            bytesRead += numBytes;
        }

        if (!releaseSpace) {
            bufferLevel = _bufferLevel;
            offsetTop = _offsetTop;
        }

        updateEOF(bytesRead);
        return bytesRead;
    }

    /**
     * @brief Writes data to the buffer (all or nothing).
     * @param data
     * @param length If -1 `strlen` will be used to get length.
     * @return `true` if successful
     */
    bool Buffer::write (const char *data, int length)
    {
        if (data == nullptr || length < 0)
            return false;

        if (!length) length = strlen(data);
        if (!length) return true;

        if (spaceAvailable() < length)
        {
            if (!drainRequest(length, 0, true))
                return false;
        }

        return fill(data,length) == length;
    }

    /**
    **	Reads data from the buffer (all-or-nothing).
    **
    **	- The byte in the output buffer at index 'length' is set to zero if extraBytes > 0.
    */

    /**
     * @brief Reads data from the buffer (all or nothing). The output byte at index 'length' will be 
     * set to zero if extraBytes > 0.
     *
     * @param length 
     * @param buff Output buffer. If not provided it will be allocated of size length+extraBytes.
     * @param releaseSpace When `false` data from the buffer will not be removed (peek mode).
     * @param extraBytes Extra bytes to allocate (default 0).
     * @return char*
     */
    char *Buffer::read (int length, char *buff, bool releaseSpace, int extraBytes)
    {
        if (length <= 0) return nullptr;

        if (releaseSpace && bytesAvailable() < length) {
            if (!fillRequest(length-bytesAvailable(), 0, true))
                return nullptr;
        }

        bool allocated = false;
        if (buff == nullptr) {
            buff = (char *)asr::alloc (length + extraBytes);
            if (buff == nullptr) return nullptr;
            allocated = true;
        }

        if (drain(buff, length, releaseSpace) != length) {
            if (allocated) asr::dealloc(buff);
            return nullptr;
        }

        if (extraBytes > 0) buff[length] = '\0';
        return buff;
    }

    /**
     * @brief Writes a byte to the buffer.
     * @return bool
     */
    bool Buffer::writeUInt8 (uint8_t value)
    {
        char tmp[1];
        tmp[0] = value;
        return fill(tmp, 1) == 1;
    }

    /**
     * @brief Writes a 16-bit integer (little endian) to the buffer.
     * @param value
     * @return bool
     */
    bool Buffer::writeUInt16 (uint16_t value)
    {
        char tmp[2];
        tmp[0] = value & 0xFF;
        tmp[1] = (value >> 8) & 0xFF;
        return fill(tmp, 2) == 2;
    }

    /**
     * @brief Writes a 32-bit integer (little endian) to the buffer.
     * @param value 
     * @return bool
     */
    bool Buffer::writeUInt32 (uint32_t value)
    {
        char tmp[4];
        tmp[0] = value & 0xFF;
        tmp[1] = (value >> 8) & 0xFF;
        tmp[2] = (value >> 16) & 0xFF;
        tmp[3] = (value >> 24) & 0xFF;
        return fill(tmp, 4) == 4;
    }

    /**
     * @brief Writes a string to the buffer with a leading-length byte.
     * @param value
     * @param length Number of bytes to write. If -1 `strlen` will be used to determine the length.
     * @return bool
     */
    bool Buffer::writeStr (const char *value, int length)
    {
        if (value == nullptr) return false;
        if (length == -1) length = strlen(value);
        if (length > 255) return false;
        if (!writeUInt8(length)) return false;
        return write(value, length) == length;
    }

    /**
     * @brief Reads a byte from the buffer. Sets EOF.
     * @param peek If true the byte will not be removed from the buffer.
     * @return uint8_t
     */
    uint8_t Buffer::readUInt8 (bool peek)
    {
        char tmp[1];
        if (drain(tmp, 1, !peek) != 1) { return 0; }

        return (unsigned char)tmp[0];
    }

    /**
     * @brief Reads a 16-bit integer (little endian) from the buffer. Sets EOF.
     * @param peek If true the bytes will not be removed from the buffer.
     * @return uint16_t
     */
    uint16_t Buffer::readUInt16 (bool peek)
    {
        char tmp[2];
        if (drain(tmp, 2, !peek) != 2) { return 0; }

        uint16_t i = (unsigned char)tmp[1];
        i <<= 8; i |= (unsigned char)tmp[0];
        return i;
    }

    /**
     * @brief Reads a 32-bit integer (little endian) from the buffer. Sets EOF.
     * @param peek If true the bytes will not be removed from the buffer.
     * @return uint32_t
     */
    uint32_t Buffer::readUInt32 (bool peek)
    {
        char tmp[4];
        if (drain(tmp, 4, !peek) != 4) { return 0; }

        uint32_t i = (unsigned char)tmp[3];
        i <<= 8; i |= (unsigned char)tmp[2];
        i <<= 8; i |= (unsigned char)tmp[1];
        i <<= 8; i |= (unsigned char)tmp[0];
        return i;
    }

    /**
     * @brief Reads a line ending with `nl` into the specified buffer (length should include the trailing zero).
     * 
     * @param buffer Buffer to store the line.
     * @param length Maximum number of bytes to read.
     * @param nl Character that indicates the end of the line.
     * @return Returns nullptr when EOF is reached and no data is read into the buffer.
     */
    char *Buffer::readLine (char *buffer, int length, char nl)
    {
        int n = 0;

        if (buffer == nullptr || length-- <= 0)
            return nullptr;

        while (!_eof && n < length) {
            int ch = readUInt8();
            if (ch == '\n') break;
            buffer[n++] = ch;
        }

        if (_eof && n == 0) {
            return nullptr;
        }

        buffer[n] = '\0';
        return buffer;
    }

}};
