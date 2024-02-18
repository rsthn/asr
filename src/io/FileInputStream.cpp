
#include <asr/io/FileInputStream>
#include <cstdio>

namespace asr {
namespace io {

	/**
	**	Executed when at least nMin and at most nMax bytes are required to be available in the buffer. When nMax is zero, there is no limit
	**	of maximum number bytes to be filled. When inquiry is true, no data operation should be made, only return if it is possible.
	*/
	bool FileInputStream::fillRequest (int nMin, int nMax, bool inquiry)
	{
		char buffer[512];

		if (fp == nullptr || fsize-ftell(fp) < nMin) return false;
		if (inquiry) return true;

		if (nMax == 0) nMax = spaceAvailable();

		while (nMax > 0 && !feof(fp))
		{
			int i = fread (buffer, 1, nMax > 512 ? 512 : nMax, fp);
			if (!i) break;

			fill (buffer, i);

			nMax -= i;
			nMin -= i;
		}

		return nMin <= 0;
	}

	/**
	**	Opens a file for reading and sets up the stream.
	*/
	FileInputStream::FileInputStream (const char *filepath, int mode, int bufferSize) : Buffer(bufferSize)
	{
		fp = fopen (filepath, mode == O_BINARY ? "rb" : "rt");
		if (fp == nullptr) return;

		fseek (fp, 0, SEEK_END);
		fsize = ftell(fp);
		fseek (fp, 0, SEEK_SET);

		fillRequest(0);

		fpOwned = true;
	}

	/**
	**	Uses the specified FILE as input. Will not close the file upon destruction nor by manual call to close() unless fpOwned is set to true,
	**	and the available data will be calculated from the current file position to its end.
	*/
	FileInputStream::FileInputStream (FILE *fp, int bufferSize, bool fpOwned) : Buffer(bufferSize)
	{
		this->fp = fp;
		this->fpOwned = fpOwned;

		long pos = ftell(fp);
		fseek (fp, 0, SEEK_END);
		fsize = ftell(fp) - pos;
		fseek (fp, pos, SEEK_SET);

		fillRequest(0);
	}

	/**
	**	Destructor. Ensures the file is properly closed and flushed.
	*/
	FileInputStream::~FileInputStream()
	{
		close();
	}

	/**
	**	Closes the file stream.
	*/
	void FileInputStream::close()
	{
		if (fp != nullptr)
		{
			if (fpOwned) fclose (fp);
			fp = nullptr;
		}
	}

}};
