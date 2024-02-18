
#include <cstring>
#include <asr/crypto/CRC32>

namespace asr {
namespace crypto {

    /**
     * @brief Returns the CRC32 hash of the given data.
     * 
     * @param data Data buffer to hash.
     * @param length Length of the data buffer. If -1, the length is calculated using strlen.
     * @param value Initial value for CRC32. Default is 0xFFFFFFFFUL.
     * @return uint32_t 
     */
    uint32_t crc32 (const char *data, int length, uint32_t value)
    {
        static uint32_t table[256] = { 0 };
        static bool table_ready = false;

        if (!table_ready)
        {
            table_ready = true;

            for (int i = 0; i < 256; i++)
            {
                uint32_t rem = i;

                for (int j = 0; j < 8; j++)
                    rem = (rem & 1) ? (rem >> 1) ^ 0xEDB88320UL : (rem >> 1);

                table[i] = rem;
            }
        }

        if (length == -1) length = strlen(data);

        for (int i = 0; i < length; i++)
            value = (value >> 8) ^ table[(uint8_t)(value ^ data[i])];

        return ~value;
    }

}};
