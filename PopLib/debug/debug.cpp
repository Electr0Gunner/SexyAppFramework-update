#include "./debug.hpp"

#include "misc/autocrit.hpp"
#include "misc/critsect.hpp"

#include <time.h>
#include <stdarg.h>
#include <SDL3/SDL.h>

#include "memmgr.hpp"

bool gInAssert = false;
bool gDumpLeakedMem = false;

static FILE *gTraceFile = nullptr;
static int gTraceFileLen = 0;
static int gTraceFileNum = 1;

using namespace PopLib;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
struct ALLOC_INFO
{
	int size;
	char file[512];
	int line;
};
static bool gShowLeaks = false;
static bool gAllocMapValid = false;
class AllocMap : public std::map<void *, ALLOC_INFO>
{
  public:
	CritSect mCrit;

  public:
	AllocMap()
	{
		gAllocMapValid = true;
	}
	~AllocMap()
	{
		if (gShowLeaks)
			DumpUnfreed();

		gAllocMapValid = false;
	}
};
static AllocMap gAllocMap;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Trace(const char *theStr)
{
	if (gTraceFile == nullptr)
	{
		gTraceFileNum = (gTraceFileNum + 1) % 2;
		char aBuf[50];
		sprintf(aBuf, "trace%d.txt", gTraceFileNum + 1);
		gTraceFile = fopen(aBuf, "w");
		if (gTraceFile == nullptr)
			return;
	}

	fprintf(gTraceFile, "%s\n", theStr);
	fflush(gTraceFile);

	gTraceFileLen += strlen(theStr);
	if (gTraceFileLen > 100000)
	{
		fclose(gTraceFile);
		gTraceFile = nullptr;
		gTraceFileLen = 0;
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void TraceFmt(const PopChar *fmt, ...)
{
	// Does not append a newline by default, also takes vararg parameters

	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);

	if (gTraceFile == nullptr)
	{
		gTraceFileNum = (gTraceFileNum + 1) % 2;
		char aBuf[50];
		sprintf(aBuf, "trace%d.txt", gTraceFileNum + 1);
		gTraceFile = fopen(aBuf, "w");
		if (gTraceFile == nullptr)
			return;
	}

	fprintf(gTraceFile, "%s", result.c_str());
	fflush(gTraceFile);

	gTraceFileLen += result.length();
	if (gTraceFileLen > 100000)
	{
		fclose(gTraceFile);
		gTraceFile = nullptr;
		gTraceFileLen = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MemAddTrack(void *addr, int asize, const char *fname, int lnum)
{
	if (!gAllocMapValid)
		return;

	AutoCrit aCrit(gAllocMap.mCrit);
	gShowLeaks = true;

	ALLOC_INFO &info = gAllocMap[addr];
	strncpy(info.file, fname, sizeof(info.file) - 1);
	info.line = lnum;
	info.size = asize;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void MemRemoveTrack(void *addr)
{
	if (!gAllocMapValid)
		return;

	AutoCrit aCrit(gAllocMap.mCrit);
	AllocMap::iterator anItr = gAllocMap.find(addr);
	if (anItr != gAllocMap.end())
		gAllocMap.erase(anItr);
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void DumpUnfreed()
{
	if (!gAllocMapValid)
		return;

	AutoCrit aCrit(gAllocMap.mCrit);
	AllocMap::iterator i;
	int totalSize = 0;
	char buf[8192];

#ifdef DUMP_LEAKED_MEM
	char hex_dump[1024];
	char ascii_dump[1024];
	int count = 0;
	int index = 0;
#endif

	FILE *f = fopen("mem_leaks.txt", "wt");
	if (!f)
		return;

	time_t aTime = time(nullptr);
	sprintf(buf, "Memory Leak Report for %s\n", asctime(localtime(&aTime)));
	fprintf(f, "%s", buf);
	SDL_Log("\n%s", buf);
	for (i = gAllocMap.begin(); i != gAllocMap.end(); i++)
	{
		sprintf(buf, "%s(%d) : Leak %d byte%s\n", i->second.file, i->second.line, i->second.size,
				i->second.size > 1 ? "s" : "");
		SDL_Log("%s", buf);
		fprintf(f, "%s", buf);

#ifdef DUMP_LEAKED_MEM
		unsigned char *data = (unsigned char *)i->first;

		for (index = 0; index < i->second.size; index++)
		{
			unsigned char _c = *data;

			if (count == 0)
				sprintf(hex_dump, "\t%02X ", _c);
			else
				sprintf(hex_dump, "%s%02X ", hex_dump, _c);

			if ((_c < 32) || (_c > 126))
				_c = '.';

			if (count == 7)
				sprintf(ascii_dump, "%s%c ", ascii_dump, _c);
			else
				sprintf(ascii_dump, "%s%c", count == 0 ? "\t" : ascii_dump, _c);

			if (++count == 16)
			{
				count = 0;
				sprintf(buf, "%s\t%s\n", hex_dump, ascii_dump);
				fprintf(f, buf);

				memset((void *)hex_dump, 0, 1024);
				memset((void *)ascii_dump, 0, 1024);
			}

			data++;
		}

		if (count != 0)
		{
			fprintf(f, hex_dump);
			for (index = 0; index < 16 - count; index++)
				fprintf(f, "\t");

			fprintf(f, ascii_dump);

			for (index = 0; index < 16 - count; index++)
				fprintf(f, ".");
		}

		count = 0;
		fprintf(f, "\n\n");
		memset((void *)hex_dump, 0, 1024);
		memset((void *)ascii_dump, 0, 1024);

#endif // DUMP_LEAKED_MEM

		totalSize += i->second.size;
	}

	sprintf(buf, "-----------------------------------------------------------\n");
	fprintf(f, "%s", buf);
	SDL_Log("%s", buf);
	sprintf(buf, "Total Unfreed: %d bytes (%dKB)\n\n", totalSize, totalSize / 1024);
	SDL_Log("%s", buf);
	fprintf(f, "%s", buf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void OutputDebug(const PopChar *fmt, ...)
{
	va_list argList;
	va_start(argList, fmt);
	std::string result = vformat(fmt, argList);
	va_end(argList);

	SDL_Log("%s", result.c_str());
}
