
#include <asr/io/FileOutputStream>

namespace asr {
namespace io {

	/**
	**	Executed when at least nMin and at most nMax bytes of free space are required. When nMax is zero, there is no limit of maximum number
	**	of bytes to free. When inquiry is true, no data operation should be made, only return if it is possible.
	*/
	bool FileOutputStream::drainRequest (int nMin, int nMax, bool inquiry)
	{
		char buffer[512];

		if (fp == nullptr) return false;
		if (inquiry) return true;

		if (!nMax) nMax = bytesAvailable();

		while (nMax > 0)
		{
			int i = drain (buffer, nMax > 512 ? 512 : nMax);
			if (!i) break;

			fwrite (buffer, 1, i, fp);

			nMax -= i;
			nMin -= i;
		}

		return nMin <= 0;
	}

	/**
	**	Opens a file for binary writing and sets up the stream.
	*/
	FileOutputStream::FileOutputStream (const char *filepath, int mode, int bufferSize) : Buffer(bufferSize)
	{
		if (mode & O_APPEND)
			fp = fopen (filepath, mode & O_BINARY ? "a+b" : "a+t");
		else
			fp = fopen (filepath, mode & O_BINARY ? "wb" : "wt");

		fpOwned = true;
	}

	/**
	**	Uses the specified FILE as output. Will not close the file upon destruction nor by manual call to close() unless fpOwned is set to true.
	*/
	FileOutputStream::FileOutputStream (FILE *fp, int bufferSize, bool fpOwned) : Buffer(bufferSize)
	{
		this->fp = fp;
		this->fpOwned = fpOwned;
	}

	/**
	**	Destructor. Ensures the file is properly closed and flushed.
	*/
	FileOutputStream::~FileOutputStream()
	{
		close();
	}

	/**
	**	Closes the file stream.
	*/
	void FileOutputStream::close()
	{
		if (fp != nullptr)
		{
			flush();

			if (fpOwned) fclose (fp);
			fp = nullptr;
		}
	}

}};
