#ifndef __PAKINTERFACE_HPP__
#define __PAKINTERFACE_HPP__
#ifdef _WIN32
#pragma once
#endif

#include <map>
#include <list>
#include <string>
#include <filesystem>
#include <memory>
#include <cstdio>
#include <fstream>
#include <vector> // how is this not included.

class PakCollection;

using FileTime = std::filesystem::file_time_type;

class PakRecord
{
  public:
	PakCollection *mCollection;
	std::string mFileName;
	FileTime mFileTime;
	std::streamoff mStartPos;
	std::size_t mSize;
};

typedef std::map<std::string, PakRecord> PakRecordMap;

class PakCollection
{
  public:
	explicit PakCollection(std::size_t size)
        : mData(size) {}

    uint8_t* data()
    {
        return mData.data();
    }

    const uint8_t* data() const
    {
        return mData.data();
    }

    std::size_t size() const
    {
        return mData.size();
    }

    std::vector<uint8_t>& vector()
    {
        return mData;
    }

    const std::vector<uint8_t>& vector() const
    {
        return mData;
    }

  private:
	std::vector<uint8_t> mData;
};

typedef std::list<PakCollection> PakCollectionList;

/**
 * @brief file struct that allows to exist both locally and inside a .pak file
 */
struct PFILE
{
	/// @brief non-null => in‐pak file
	PakRecord *mRecord = nullptr;
	/// @brief wrapped FILE* for on‐disk files
	FILE *mFP = nullptr;
	/// @brief current read position
	long mPos = 0;
};

struct PFindData
{
	std::filesystem::directory_iterator it;
	std::filesystem::directory_iterator end;
	std::string pattern;
};

class PakInterfaceBase
{
  public:
	virtual ~PakInterfaceBase() = default;

	virtual PFILE *FOpen(const char *fn, const char *mode)
	{
		return nullptr;
	}
	virtual PFILE *FOpen(const wchar_t *fn, const wchar_t *mode)
	{
		return nullptr;
	}
	virtual int FClose(PFILE *pf)
	{
		return 0;
	}
	virtual int FSeek(PFILE *pf, long offset, int whence)
	{
		return 0;
	}
	virtual int FTell(PFILE *pf)
	{
		return 0;
	}
	virtual std::size_t FRead(void *buf, int size, int count, PFILE *pf)
	{
		return 0;
	}
	virtual int FGetC(PFILE *pf)
	{
		return 0;
	}
	virtual int UnGetC(int c, PFILE *pf)
	{
		return 0;
	}
	virtual char *FGetS(char *str, int size, PFILE *pf)
	{
		return 0;
	}
	virtual wchar_t *FGetS(wchar_t *str, int size, PFILE *pf)
	{
		return str;
	}
	virtual int FEof(PFILE *pf)
	{
		return 0;
	}
	virtual bool AddPakFile(const std::string &fileName)
	{
		return false;
	}

	virtual PFindData FindFirstFile(const std::string &pattern)
	{
		return PFindData{std::filesystem::directory_iterator{}, std::filesystem::directory_iterator{}, pattern};
	}
	virtual bool FindNextFile(PFindData &fd, std::string &outName)
	{
		return false;
	}
	virtual void FindClose(PFindData &fd)
	{
	}
};

class PakInterface : public PakInterfaceBase
{
  public:
	PakCollectionList mPakCollectionList;
	PakRecordMap mPakRecordMap;
	std::string mError;

	PakInterface();
	~PakInterface();

	virtual bool AddPakFile(const std::string &fileName);

	PFILE *FOpen(const char *fn, const char *mode) override;
	int FClose(PFILE *pf) override;
	int FSeek(PFILE *pf, long offset, int whence) override;
	int FTell(PFILE *pf) override;
	std::size_t FRead(void *buf, int size, int count, PFILE *pf) override;
	int FGetC(PFILE *pf) override;
	int UnGetC(int c, PFILE *pf) override;
	char *FGetS(char *str, int size, PFILE *pf) override;
	int FEof(PFILE *pf) override;

	PFindData FindFirstFile(const std::string &pattern) override;
	bool FindNextFile(PFindData &fd, std::string &outName) override;
	void FindClose(PFindData &fd) override;
};

extern PakInterface *gPakInterface;

static PFILE *p_fopen(const char *theFileName, const char *theAccess)
{
	if (gPakInterface)
		return gPakInterface->FOpen(theFileName, theAccess);

	FILE *aFP = fopen(theFileName, theAccess);
	if (!aFP)
		return NULL;

	PFILE *aPFile = new PFILE();
	aPFile->mRecord = NULL;
	aPFile->mPos = 0;
	aPFile->mFP = aFP;

	return aPFile;
}

inline int p_fclose(PFILE *pf)
{
	if (!pf)
		return EOF;
	if (gPakInterface && pf->mRecord)
	{
		return gPakInterface->FClose(pf);
	}
	int r = 0;
	if (pf->mFP)
	{
		r = fclose(pf->mFP);
	}
	delete pf;
	return r;
}

inline int p_fgetc(PFILE *pf)
{
	if (!pf)
		return EOF;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->FGetC(pf);
	return fgetc(pf->mFP);
}

inline int p_ungetc(int c, PFILE *pf)
{
	if (!pf)
		return EOF;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->UnGetC(c, pf);
	return ungetc(c, pf->mFP);
}

inline std::size_t p_fread(void *buf, std::size_t size, std::size_t count, PFILE *pf)
{
	if (!pf)
		return 0;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->FRead(buf, size, count, pf);
	return fread(buf, size, count, pf->mFP);
}

inline int p_fseek(PFILE *pf, long offset, int whence)
{
	if (!pf)
		return -1;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->FSeek(pf, offset, whence);
	return fseek(pf->mFP, offset, whence);
}

inline int p_ftell(PFILE *pf)
{
	if (!pf)
		return -1;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->FTell(pf);
	return ftell(pf->mFP);
}

inline bool p_feof(PFILE *pf)
{
	if (!pf)
		return true;
	if (gPakInterface && pf->mRecord)
		return gPakInterface->FEof(pf);
	return feof(pf->mFP) != 0;
}

inline PFindData p_FindFirstFile(const std::string &pattern)
{
	std::filesystem::path p(pattern);
	PFindData fd;
	if (std::filesystem::exists(p))
	{
		fd.it = std::filesystem::directory_iterator(p);
		fd.end = std::filesystem::directory_iterator();
		fd.pattern = pattern;
	}
	return fd;
}

inline bool p_FindNextFile(PFindData &fd, std::string &outName)
{
	while (fd.it != fd.end)
	{
		auto entry = *fd.it++;
		if (!entry.is_directory())
		{
			outName = entry.path().filename().string();
			return true;
		}
	}
	return false;
}

#endif // __PAKINTERFACE_HPP__
