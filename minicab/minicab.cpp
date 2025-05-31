#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <fci.h>

#undef HUGE

#include <io.h>
#include <fcntl.h>
#include <strsafe.h>

#include <cstdint>
#include <new>

#include <algorithm>
#include <limits>

#include "wimlib.h"

wchar_t *ConvertToUTF16(const char *path)
{
	int requiredSize = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
	if (requiredSize == 0)
		return nullptr;

	wchar_t *convertedPath = static_cast<wchar_t *>(malloc(static_cast<size_t>(requiredSize) * sizeof(wchar_t)));
	if (!convertedPath)
		return nullptr;

	::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, convertedPath, requiredSize);
	return convertedPath;
}

char *ConvertToUTF8(const wchar_t *path)
{
	int requiredSize = ::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, path, -1, nullptr, 0, nullptr, nullptr);
	if (requiredSize == 0)
	{
		DWORD err = GetLastError();
		return nullptr;
	}

	char *convertedPath = static_cast<char *>(malloc(requiredSize));
	if (!convertedPath)
		return nullptr;

	::WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, path, -1, convertedPath, requiredSize, nullptr, nullptr);
	return convertedPath;
}

class ContextFile
{
public:
	explicit ContextFile(HANDLE hfile)
		: m_hfile(hfile)
	{
	}

	UINT Read(void *memory, UINT cb, int *err)
	{
		DWORD dwBytesRead = 0;

		if (ReadFile(m_hfile, memory, cb, &dwBytesRead, NULL) == FALSE)
		{
			*err = GetLastError();
			return static_cast<DWORD>(-1);
		}

		return dwBytesRead;
	}

	UINT Write(const void *memory, UINT cb, int *err)
	{
		DWORD dwBytesWritten = 0;

		if (WriteFile(m_hfile, memory, cb, &dwBytesWritten, nullptr) == FALSE)
		{
			*err = GetLastError();
			return static_cast<DWORD>(-1);
		}

		return dwBytesWritten;
	}

	int Close(int *err)
	{
		int result = 0;
		if (CloseHandle(m_hfile) == FALSE)
		{
			*err = GetLastError();
			result = -1;
		}

		void *mem = this;
		this->~ContextFile();
		free(mem);

		return result;
	}

	long Seek(long dist, int seektype, int *err)
	{
		LONG result = SetFilePointer(m_hfile, dist, nullptr, seektype);
		if (result == -1)
			*err = GetLastError();

		return result;
	}

private:
	HANDLE m_hfile;
};

struct Context
{
	const wchar_t *m_basePath;

	int FilePlaced(PCCAB pccab, LPSTR pszFile, long cbFile, BOOL fContinuation);
	ContextFile *Open(LPSTR pszFile, int oflag, int pmode, int *err);
	UINT Read(ContextFile *hf, void *memory, UINT cb, int *err);
	UINT Write(ContextFile *hf, const void *memory, UINT cb, int *err);
	int Close(ContextFile *hf, int *err);
	long Seek(ContextFile *hf, long dist, int seektype, int *err);
	int Delete(LPSTR pszFile, int *err);
	BOOL GetTempFile(char *pszTempName, int cbTempName);
	BOOL GetNextCabinet(PCCAB pccab, ULONG  cbPrevCab);
	ContextFile *GetOpenInfo(LPSTR pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err);
};

int Context::FilePlaced(PCCAB pccab, LPSTR pszFile, long cbFile, BOOL fContinuation)
{
	// -1 to abort, anything else to succeed
	return 0;
}

ContextFile *Context::Open(LPSTR pszFile, int oflag, int pmode, int *err)
{
	HANDLE hFile = NULL;
	DWORD dwDesiredAccess = 0;
	DWORD dwCreationDisposition = 0;

	if (oflag & _O_RDWR)
	{
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
	}
	else if (oflag & _O_WRONLY)
	{
		dwDesiredAccess = GENERIC_WRITE;
	}
	else
	{
		dwDesiredAccess = GENERIC_READ;
	}

	if (oflag & _O_CREAT)
	{
		dwCreationDisposition = CREATE_ALWAYS;
	}
	else
	{
		dwCreationDisposition = OPEN_EXISTING;
	}

	wchar_t *convertedName = ConvertToUTF16(pszFile);
	if (!convertedName)
	{
		*err = E_OUTOFMEMORY;
		return nullptr;
	}

	hFile = CreateFileW(convertedName,
		dwDesiredAccess,
		FILE_SHARE_READ,
		NULL,
		dwCreationDisposition,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	free(convertedName);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		*err = GetLastError();
		return nullptr;
	}

	void *mem = malloc(sizeof(ContextFile));
	if (!mem)
	{
		CloseHandle(hFile);
		*err = E_OUTOFMEMORY;
		return nullptr;
	}

	return new (mem) ContextFile(hFile);
}

UINT Context::Read(ContextFile *hf, void *memory, UINT cb, int *err)
{
	return hf->Read(memory, cb, err);
}

UINT Context::Write(ContextFile *hf, const void *memory, UINT cb, int *err)
{
	return hf->Write(memory, cb, err);
}

int Context::Close(ContextFile *hf, int *err)
{
	return hf->Close(err);
}

long Context::Seek(ContextFile *hf, long dist, int seektype, int *err)
{
	return hf->Seek(dist, seektype, err);
}

int Context::Delete(LPSTR pszFile, int *err)
{
	if (!DeleteFileA(pszFile))
	{
		*err = GetLastError();
		return -1;
	}

	return 0;
}

BOOL Context::GetTempFile(char *pszTempName, int cbTempName)
{
	BOOL bSucceeded = FALSE;
	wchar_t pszTempPath[MAX_PATH];
	wchar_t pszTempFile[MAX_PATH];

	if (GetTempPathW(MAX_PATH, pszTempPath) != 0)
	{
		if (GetTempFileNameW(pszTempPath, L"CABINET", 0, pszTempFile) != 0)
		{
			DeleteFileW(pszTempFile);

			char *converted = ConvertToUTF8(pszTempFile);
			if (!converted)
			{
				return FALSE;
			}

			size_t convertedLen = strlen(converted);
			if (convertedLen < static_cast<size_t>(cbTempName))
			{
				memcpy(pszTempName, converted, convertedLen);
				pszTempName[convertedLen] = '\0';

				bSucceeded = TRUE;
			}

			free(converted);

			bSucceeded = true;
		}
	}

	return bSucceeded;
}

BOOL Context::GetNextCabinet(PCCAB pccab, ULONG cbPrevCab)
{
	return FALSE;
}

ContextFile *Context::GetOpenInfo(LPSTR pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err)
{
	wchar_t *wcName = ConvertToUTF16(pszName);
	if (!wcName)
		return nullptr;

	ContextFile *result = nullptr;

	HANDLE hfile = ::CreateFileW(wcName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hfile == INVALID_HANDLE_VALUE)
		*err = GetLastError();
	else
	{
		BY_HANDLE_FILE_INFORMATION fileInfo;
		FILETIME fileTime;
		if (GetFileInformationByHandle(hfile, &fileInfo)
			&& FileTimeToLocalFileTime(&fileInfo.ftCreationTime, &fileTime)
			&& FileTimeToDosDateTime(&fileTime, pdate, ptime))
		{
			*pattribs = static_cast<USHORT>(fileInfo.dwFileAttributes);
			*pattribs &= (_A_RDONLY | _A_HIDDEN | _A_SYSTEM | _A_ARCH);
			*pattribs |= _A_NAME_IS_UTF;

			void *fileMem = malloc(sizeof(ContextFile));
			result = new (fileMem) ContextFile(hfile);
		}
	}

	if (!result)
		CloseHandle(hfile);

	if (wcName)
		free(wcName);

	return result;
}

static int CtxFilePlaced(PCCAB pccab, LPSTR pszFile, long cbFile, BOOL  fContinuation, void *pv)
{
	return static_cast<Context *>(pv)->FilePlaced(pccab, pszFile, cbFile, fContinuation);
}

static void *CtxAlloc(ULONG cb)
{
	if (cb == 0)
		return nullptr;

	return malloc(cb);
}

static void CtxFree(void *memory)
{
	if (memory != nullptr)
		free(memory);
}

static INT_PTR CtxOpen(LPSTR pszFile, int oflag, int pmode, int *err, void *pv)
{
	ContextFile *f = static_cast<Context *>(pv)->Open(pszFile, oflag, pmode, err);
	if (f)
		return reinterpret_cast<INT_PTR>(f);
	else
		return -1;
}

static UINT CtxRead(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
	return static_cast<Context *>(pv)->Read(reinterpret_cast<ContextFile *>(hf), memory, cb, err);
}

static UINT CtxWrite(INT_PTR hf, void *memory, UINT cb, int *err, void *pv)
{
	return static_cast<Context *>(pv)->Write(reinterpret_cast<ContextFile *>(hf), memory, cb, err);
}

static int CtxClose(INT_PTR hf, int *err, void *pv)
{
	return static_cast<Context *>(pv)->Close(reinterpret_cast<ContextFile *>(hf), err);
}

static long CtxSeek(INT_PTR hf, long dist, int seektype, int *err, void *pv)
{
	return static_cast<Context *>(pv)->Seek(reinterpret_cast<ContextFile *>(hf), dist, seektype, err);
}

static int CtxDelete(LPSTR pszFile, int *err, void *pv)
{
	return static_cast<Context *>(pv)->Delete(pszFile, err);
}

BOOL CtxGetTempFile(char *pszTempName, int cbTempName, void *pv)
{
	return static_cast<Context *>(pv)->GetTempFile(pszTempName, cbTempName);
}

BOOL CtxGetNextCabinet(PCCAB pccab, ULONG  cbPrevCab, void *pv)
{
	return static_cast<Context *>(pv)->GetNextCabinet(pccab, cbPrevCab);
}

long CtxStatus(UINT typeStatus, ULONG cb1, ULONG cb2, void *pv)
{
	return 0;
}

INT_PTR CtxGetOpenInfo(LPSTR pszName, USHORT *pdate, USHORT *ptime, USHORT *pattribs, int *err, void *pv)
{
	ContextFile *f = static_cast<Context *>(pv)->GetOpenInfo(pszName, pdate, ptime, pattribs, err);
	if (f)
		return reinterpret_cast<INT_PTR>(f);
	else
		return -1;
}

int AddFile(HFCI fci, const wchar_t *path, size_t pathLength, size_t truncateSize)
{
	char *convertedPath = ConvertToUTF8(path);
	char *convertedName = ConvertToUTF8(path + truncateSize);

	BOOL succeeded = false;

	if (convertedPath && convertedName)
	{
		TCOMP tcomp = (tcompTYPE_LZX | tcompLZX_WINDOW_HI);
		//TCOMP tcomp = tcompTYPE_MSZIP;
		succeeded = FCIAddFile(fci, convertedPath, convertedName, FALSE, CtxGetNextCabinet, CtxStatus, CtxGetOpenInfo, tcomp);
	}

	if (convertedPath)
		free(convertedPath);
	if (convertedName)
		free(convertedName);

	return succeeded ? 0 : -1;
}

int RecursiveAddDir(HFCI fci, const wchar_t *path, size_t pathLength, size_t truncateSize)
{
	int result = 0;

	wchar_t *expanded = static_cast<wchar_t *>(malloc((pathLength + 3) * sizeof(wchar_t)));
	if (!expanded)
		return -1;

	memcpy(expanded, path, pathLength * sizeof(wchar_t));
	expanded[pathLength] = L'\\';
	expanded[pathLength + 1] = L'*';
	expanded[pathLength + 2] = L'\0';

	WIN32_FIND_DATAW findData;
	HANDLE hfind = FindFirstFileW(expanded, &findData);

	free(expanded);

	if (hfind == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
			return 0;
		else
			return -1;
	}

	for (;;)
	{
		if (wcscmp(findData.cFileName, L".") && wcscmp(findData.cFileName, L".."))
		{
			wchar_t *extendedPath = nullptr;

			size_t fileNameLength = wcslen(findData.cFileName);
			size_t extPathLen = pathLength + 1 + fileNameLength;

			extendedPath = static_cast<wchar_t *>(malloc(sizeof(wchar_t) * (extPathLen + 1)));
			if (!extendedPath)
			{
				result = -1;
				break;
			}

			memcpy(extendedPath, path, pathLength * sizeof(wchar_t));
			extendedPath[pathLength] = L'\\';
			memcpy(extendedPath + pathLength + 1, findData.cFileName, fileNameLength * sizeof(wchar_t));
			extendedPath[extPathLen] = L'\0';

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				result = RecursiveAddDir(fci, extendedPath, extPathLen, truncateSize);
			else
				result = AddFile(fci, extendedPath, extPathLen, truncateSize);

			free(extendedPath);
		}

		if (result != 0)
			break;

		if (!FindNextFileW(hfind, &findData))
			break;
	}

	FindClose(hfind);
	return result;
}


int RecursiveAdd(HFCI fci, const wchar_t *path)
{
	DWORD attribs = GetFileAttributesW(path);

	if (attribs == INVALID_FILE_ATTRIBUTES)
		return -1;

	const size_t pathLength = wcslen(path);

	if (attribs & FILE_ATTRIBUTE_DIRECTORY)
	{
		size_t truncateSize = wcslen(path) + 1;
		return RecursiveAddDir(fci, path, pathLength, truncateSize);
	}
	else
	{
		size_t truncateSize = wcslen(path);

		while (truncateSize > 0)
		{
			truncateSize--;
			if (path[truncateSize] == '\\' || path[truncateSize] == '/')
			{
				truncateSize++;
				break;
			}
		}

		return AddFile(fci, path, pathLength, truncateSize);
	}
}

char myData[32768];
char myCompressedData[1024];

int wmain(int argc, const wchar_t **argv)
{
	if (argc < 4)
	{
		fprintf(stderr, "minicab <output cab folder> <output cab name> <input folder>");
		return -1;
	}

	ERF erf = {};
	Context ctx = {};
	CCAB cabParams = {};

	const wchar_t *outFolder = argv[1];
	const wchar_t *outCabName = argv[2];
	const wchar_t *inFolder = argv[3];

	const int CAB_MAX_SIZE = 0x7FFFFFFF;

	cabParams.cb = CAB_MAX_SIZE;
	cabParams.cbFolderThresh = CAB_MAX_SIZE - 16;
	ctx.m_basePath = outCabName;

	if (!::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, outCabName, -1, cabParams.szCab, sizeof(cabParams.szCab), nullptr, nullptr))
	{
		fprintf(stderr, "Failed to convert cab name to multi-byte");
		return -1;
	}

	if (!::WideCharToMultiByte(CP_ACP, WC_NO_BEST_FIT_CHARS, outFolder, -1, cabParams.szCabPath, sizeof(cabParams.szCab), nullptr, nullptr))
	{
		fprintf(stderr, "Failed to convert cab name to multi-byte");
		return -1;
	}

	Sleep(10000);

	HFCI fci = FCICreate(&erf, CtxFilePlaced, CtxAlloc, CtxFree, CtxOpen, CtxRead, CtxWrite, CtxClose, CtxSeek, CtxDelete, CtxGetTempFile, &cabParams, &ctx);
	
	int result = RecursiveAdd(fci, inFolder);

	if (result == 0)
	{
		if (!FCIFlushCabinet(fci, FALSE, CtxGetNextCabinet, CtxStatus))
			result = -1;
	}

	FCIDestroy(fci);

	Sleep(10000);

    return result;
}
