
#include <asr/ofilebuffer>

namespace asr {

    bool OFileBuffer::drain_request(int n_min, int n_max, bool inquiry)
    {
        char buffer[512];

        if (fp == nullptr)
            return false;

        if (inquiry)
            return true;

        if (!n_max) n_max = bytes_available();

        while (n_max > 0)
        {
            int i = drain(buffer, n_max > 512 ? 512 : n_max);
            if (!i) break;

            fwrite(buffer, 1, i, fp);

            n_max -= i;
            n_min -= i;
        }

        return n_min <= 0;
    }

    OFileBuffer::OFileBuffer(const char *filepath, int mode, int buffer_size) : Buffer(buffer_size)
    {
        if (mode & O_APPEND)
            fp = fopen(filepath, mode & O_BINARY ? "a+b" : "a+t");
        else
            fp = fopen(filepath, mode & O_BINARY ? "wb" : "wt");

        fp_owned = true;
    }

    OFileBuffer::OFileBuffer(FILE *fp, int buffer_size, bool fp_owned) : Buffer(buffer_size)
    {
        this->fp = fp;
        this->fp_owned = fp_owned;
    }

    OFileBuffer::~OFileBuffer() {
        close();
    }

    void OFileBuffer::close()
    {
        if (fp != nullptr)
        {
            flush();
            if (fp_owned)
                fclose(fp);
            fp = nullptr;
        }
    }

};
