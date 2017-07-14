//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "stdafx.h"

#include <sys/stat.h>

#if defined(__APPLE__)
#include <mach-o/dyld.h> // _NSGetExecutablePath
#elif defined(__linux__)
#include <unistd.h> // readlink
#elif !defined(_WIN32)
#error "How to get the executable path for this platform?"
#endif // _WIN32 ?

//TODO: x-plat definitions
#ifdef _WIN32
#define MAX_URI_LENGTH 512
#define TTD_HOST_PATH_SEP "\\"

void TTDHostBuildCurrentExeDirectory(char* path, size_t* pathLength, size_t bufferLength)
{
    wchar exePath[MAX_PATH];
    GetModuleFileName(NULL, exePath, MAX_PATH);

    size_t i = wcslen(exePath) - 1;
    while(exePath[i] != _u('\\'))
    {
        --i;
    }

    if(i * 3 > bufferLength)
    {
        printf("Don't overflow path buffer during conversion");
        exit(1);
    }
    *pathLength = utf8::EncodeInto((LPUTF8)path, exePath, (charcount_t)(i + 1));
    path[*pathLength] = '\0';
}

int TTDHostMKDir(const char* path, size_t pathLength)
{
    char16 cpath[MAX_PATH];
    LPCUTF8 pathbase = (LPCUTF8)path;

    if(MAX_PATH <= pathLength) //<= to account for null terminator
    {
        printf("Don't overflow path buffer during conversion");
        exit(1);
    }
    utf8::DecodeUnitsIntoAndNullTerminate(cpath, pathbase, pathbase + pathLength);

    return _wmkdir(cpath);
}

JsTTDStreamHandle TTDHostOpen(size_t pathLength, const char* path, bool isWrite)
{
    char16 wpath[MAX_PATH];
    LPCUTF8 pathbase = (LPCUTF8)path;

    if(MAX_PATH <= pathLength) //<= to account for null terminator
    {
        printf("Don't overflow path buffer during conversion");
        exit(1);
    }
    utf8::DecodeUnitsIntoAndNullTerminate(wpath, pathbase, pathbase + pathLength);

    FILE* res = nullptr;
    _wfopen_s(&res, wpath, isWrite ? _u("w+b") : _u("r+b"));

    return (JsTTDStreamHandle)res;
}

#define TTDHostRead(buff, size, handle) fread_s(buff, size, 1, size, (FILE*)handle);
#define TTDHostWrite(buff, size, handle) fwrite(buff, 1, size, (FILE*)handle)
#else

#ifdef __APPLE__
#include <mach-o/dyld.h>
#else
#include <unistd.h>
#endif
#define MAX_URI_LENGTH 512
#define TTD_HOST_PATH_SEP "/"

void TTDHostBuildCurrentExeDirectory(char* path, size_t* pathLength, size_t bufferLength)
{
    char exePath[MAX_URI_LENGTH];
    //TODO: xplattodo move this logic to PAL
    #ifdef __APPLE__
    uint32_t tmpPathSize = sizeof(exePath);
    _NSGetExecutablePath(exePath, &tmpPathSize);
    size_t i = strlen(exePath) - 1;
    #else
    size_t i = readlink("/proc/self/exe", exePath, MAX_URI_LENGTH) - 1;
    #endif

    while(exePath[i] != '/')
    {
        --i;
    }
    *pathLength = i + 1;

    if(*pathLength > bufferLength)
    {
        printf("Don't overflow path buffer during copy.");
        exit(1);
    }

    memcpy_s(path, bufferLength, exePath, *pathLength);
}

int TTDHostMKDir(const char* path, size_t pathLength)
{
    return mkdir(path, 0700);
}

JsTTDStreamHandle TTDHostOpen(size_t pathLength, const char* path, bool isWrite)
{
    return (JsTTDStreamHandle)fopen(path, isWrite ? "w+b" : "r+b");
}

#define TTDHostRead(buff, size, handle) fread(buff, 1, size, (FILE*)handle)
#define TTDHostWrite(buff, size, handle) fwrite(buff, 1, size, (FILE*)handle)
#endif

HRESULT Helpers::LoadScriptFromFile(LPCSTR filename, LPCSTR& contents, UINT* lengthBytesOut /*= nullptr*/)
{
    HRESULT hr = S_OK;
    BYTE * pRawBytes = nullptr;
    UINT lengthBytes = 0;
    contents = nullptr;
    FILE * file = NULL;

    //
    // Open the file as a binary file to prevent CRT from handling encoding, line-break conversions,
    // etc.
    //
    if (fopen_s(&file, filename, "rb") != 0)
    {
#ifdef _WIN32
        DWORD lastError = GetLastError();
        char16 wszBuff[512];
        fprintf(stderr, "Error in opening file '%s' ", filename);
        wszBuff[0] = 0;
        if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
            nullptr,
            lastError,
            0,
            wszBuff,
            _countof(wszBuff),
            nullptr))
        {
            fwprintf(stderr, _u(": %s"), wszBuff);
        }
        fwprintf(stderr, _u("\n"));
#elif defined(_POSIX_VERSION)
        fprintf(stderr, "Error in opening file: ");
        perror(filename);
#endif
        IfFailGo(E_FAIL);
    }

    if (file != NULL)
    {
        // Determine the file length, in bytes.
        fseek(file, 0, SEEK_END);
        lengthBytes = ftell(file);
        fseek(file, 0, SEEK_SET);
        const size_t bufferLength = lengthBytes + sizeof(BYTE);
        pRawBytes = (LPBYTE)malloc(bufferLength);
        if (nullptr == pRawBytes)
        {
            fwprintf(stderr, _u("out of memory"));
            IfFailGo(E_OUTOFMEMORY);
        }

        //
        // Read the entire content as a binary block.
        //
        size_t readBytes = fread(pRawBytes, sizeof(BYTE), lengthBytes, file);
        if (readBytes < lengthBytes * sizeof(BYTE))
        {
            IfFailGo(E_FAIL);
        }

        pRawBytes[lengthBytes] = 0; // Null terminate it. Could be UTF16

        //
        // Read encoding to make sure it's supported
        //
        // Warning: The UNICODE buffer for parsing is supposed to be provided by the host.
        // This is not a complete read of the encoding. Some encodings like UTF7, UTF1, EBCDIC, SCSU, BOCU could be
        // wrongly classified as ANSI
        //
        {
            C_ASSERT(sizeof(WCHAR) == 2);
            if (bufferLength > 2)
            {
                if ((pRawBytes[0] == 0xFE && pRawBytes[1] == 0xFF) ||
                    (pRawBytes[0] == 0xFF && pRawBytes[1] == 0xFE) ||
                    (bufferLength > 4 && pRawBytes[0] == 0x00 && pRawBytes[1] == 0x00 &&
                        ((pRawBytes[2] == 0xFE && pRawBytes[3] == 0xFF) ||
                         (pRawBytes[2] == 0xFF && pRawBytes[3] == 0xFE))))

                {
                    // unicode unsupported
                    fwprintf(stderr, _u("unsupported file encoding. Only ANSI and UTF8 supported"));
                    IfFailGo(E_UNEXPECTED);
                }
            }
        }
    }

    contents = reinterpret_cast<LPCSTR>(pRawBytes);

Error:
    if (SUCCEEDED(hr))
    {
        if (lengthBytesOut)
        {
            *lengthBytesOut = lengthBytes;
        }
    }

    if (file != NULL)
    {
        fclose(file);
    }

    if (pRawBytes && reinterpret_cast<LPCSTR>(pRawBytes) != contents)
    {
        free(pRawBytes);
    }

    return hr;
}

LPCWSTR Helpers::JsErrorCodeToString(JsErrorCode jsErrorCode)
{
    bool hasException = false;
    ChakraRTInterface::JsHasException(&hasException);
    if (hasException)
    {
        WScriptJsrt::PrintException("", JsErrorScriptException);
    }

    switch (jsErrorCode)
    {
    case JsNoError:
        return _u("JsNoError");
        break;

    case JsErrorInvalidArgument:
        return _u("JsErrorInvalidArgument");
        break;

    case JsErrorNullArgument:
        return _u("JsErrorNullArgument");
        break;

    case JsErrorNoCurrentContext:
        return _u("JsErrorNoCurrentContext");
        break;

    case JsErrorInExceptionState:
        return _u("JsErrorInExceptionState");
        break;

    case JsErrorNotImplemented:
        return _u("JsErrorNotImplemented");
        break;

    case JsErrorWrongThread:
        return _u("JsErrorWrongThread");
        break;

    case JsErrorRuntimeInUse:
        return _u("JsErrorRuntimeInUse");
        break;

    case JsErrorBadSerializedScript:
        return _u("JsErrorBadSerializedScript");
        break;

    case JsErrorInDisabledState:
        return _u("JsErrorInDisabledState");
        break;

    case JsErrorCannotDisableExecution:
        return _u("JsErrorCannotDisableExecution");
        break;

    case JsErrorHeapEnumInProgress:
        return _u("JsErrorHeapEnumInProgress");
        break;

    case JsErrorOutOfMemory:
        return _u("JsErrorOutOfMemory");
        break;

    case JsErrorScriptException:
        return _u("JsErrorScriptException");
        break;

    case JsErrorScriptCompile:
        return _u("JsErrorScriptCompile");
        break;

    case JsErrorScriptTerminated:
        return _u("JsErrorScriptTerminated");
        break;

    case JsErrorFatal:
        return _u("JsErrorFatal");
        break;

    default:
        return _u("<unknown>");
        break;
    }
}

void Helpers::LogError(__in __nullterminated const char16 *msg, ...)
{
    va_list args;
    va_start(args, msg);
    wprintf(_u("ERROR: "));
    vfwprintf(stderr, msg, args);
    wprintf(_u("\n"));
    fflush(stdout);
    va_end(args);
}

HRESULT Helpers::LoadBinaryFile(LPCSTR filename, LPCSTR& contents, UINT& lengthBytes, bool printFileOpenError)
{
    HRESULT hr = S_OK;
    contents = nullptr;
    lengthBytes = 0;
    size_t result;
    FILE * file;

    //
    // Open the file as a binary file to prevent CRT from handling encoding, line-break conversions,
    // etc.
    //
    if (fopen_s(&file, filename, "rb") != 0)
    {
        if (printFileOpenError)
        {
#ifdef _WIN32
            DWORD lastError = GetLastError();
            char16 wszBuff[512];
            fprintf(stderr, "Error in opening file '%s' ", filename);
            wszBuff[0] = 0;
            if (FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                nullptr,
                lastError,
                0,
                wszBuff,
                _countof(wszBuff),
                nullptr))
            {
                fwprintf(stderr, _u(": %s"), wszBuff);
            }
#endif
            fprintf(stderr, "\n");
            IfFailGo(E_FAIL);
        }
        else
        {
            return E_FAIL;
        }
    }
    // file will not be nullptr if _wfopen_s succeeds
    __analysis_assume(file != nullptr);

    //
    // Determine the file length, in bytes.
    //
    fseek(file, 0, SEEK_END);
    lengthBytes = ftell(file);
    fseek(file, 0, SEEK_SET);
    contents = (LPCSTR)HeapAlloc(GetProcessHeap(), 0, lengthBytes);
    if (nullptr == contents)
    {
        fwprintf(stderr, _u("out of memory"));
        IfFailGo(E_OUTOFMEMORY);
    }
    //
    // Read the entire content as a binary block.
    //
    result = fread((void*)contents, sizeof(char), lengthBytes, file);
    if (result != lengthBytes)
    {
        fwprintf(stderr, _u("Read error"));
        IfFailGo(E_FAIL);
    }
    fclose(file);

Error:
    if (contents && FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, (void*)contents);
        contents = nullptr;
    }

    return hr;
}

void Helpers::TTReportLastIOErrorAsNeeded(BOOL ok, const char* msg)
{
    if(!ok)
    {
#ifdef _WIN32
        DWORD lastError = GetLastError();
        LPTSTR pTemp = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY, NULL, lastError, 0, (LPTSTR)&pTemp, 0, NULL);
        fwprintf(stderr, _u("Error is: %s\n"), pTemp);
#else
        fprintf(stderr, "Error is: %i %s\n", errno, strerror(errno));
#endif
        fprintf(stderr, "Message is: %s\n", msg);

        AssertMsg(false, "IO Error!!!");
        exit(1);
    }
}

//We assume bounded ascii path length for simplicity
#define MAX_TTD_ASCII_PATH_EXT_LENGTH 64

void Helpers::CreateTTDDirectoryAsNeeded(size_t* uriLength, char* uri, const char* asciiDir1, const wchar* asciiDir2)
{
    if(*uriLength + strlen(asciiDir1) + wcslen(asciiDir2) + 2 > MAX_URI_LENGTH || strlen(asciiDir1) >= MAX_TTD_ASCII_PATH_EXT_LENGTH || wcslen(asciiDir2) >= MAX_TTD_ASCII_PATH_EXT_LENGTH)
    {
        printf("We assume bounded MAX_URI_LENGTH for simplicity.\n");
        printf("%s, %s, %ls\n", uri, asciiDir1, asciiDir2);
        exit(1);
    }

    int success = 0;
    int extLength = 0;

    extLength = sprintf_s(uri + *uriLength, MAX_TTD_ASCII_PATH_EXT_LENGTH, "%s%s", asciiDir1, TTD_HOST_PATH_SEP);
    if(extLength == -1 || MAX_URI_LENGTH < (*uriLength) + extLength)
    {
        printf("Failed directory extension 1.\n");
        printf("%s, %s, %ls\n", uri, asciiDir1, asciiDir2);
        exit(1);
    }
    *uriLength += extLength;

    success = TTDHostMKDir(uri, *uriLength);
    if(success != 0)
    {
        //we may fail because someone else created the directory -- that is ok
        Helpers::TTReportLastIOErrorAsNeeded(errno != ENOENT, "Failed to create directory");
    }

    char realAsciiDir2[MAX_TTD_ASCII_PATH_EXT_LENGTH];
    size_t asciiDir2Length = wcslen(asciiDir2) + 1;
    for(size_t i = 0; i < asciiDir2Length; ++i)
    {
        if(asciiDir2[i] > CHAR_MAX)
        {
            printf("Test directory names can only include ascii chars.\n");
            exit(1);
        }
        realAsciiDir2[i] = (char)asciiDir2[i];
    }

    extLength = sprintf_s(uri + *uriLength, MAX_TTD_ASCII_PATH_EXT_LENGTH, "%s%s", realAsciiDir2, TTD_HOST_PATH_SEP);
    if(extLength == -1 || MAX_URI_LENGTH < *uriLength + extLength)
    {
        printf("Failed directory create 2.\n");
        printf("%s, %s, %ls\n", uri, asciiDir1, asciiDir2);
        exit(1);
    }
    *uriLength += extLength;

    success = TTDHostMKDir(uri, *uriLength);
    if(success != 0)
    {
        //we may fail because someone else created the directory -- that is ok
        Helpers::TTReportLastIOErrorAsNeeded(errno != ENOENT, "Failed to create directory");
    }
}

void Helpers::GetTTDDirectory(const wchar* curi, size_t* uriLength, char* uri, size_t bufferLength)
{
    TTDHostBuildCurrentExeDirectory(uri, uriLength, bufferLength);

    Helpers::CreateTTDDirectoryAsNeeded(uriLength, uri, "_ttdlog", curi);
}

JsTTDStreamHandle CALLBACK Helpers::TTCreateStreamCallback(size_t uriLength, const char* uri, size_t asciiNameLength, const char* asciiName, bool read, bool write)
{
    AssertMsg((read | write) & (!read | !write), "Read/Write streams not supported yet -- defaulting to read only");

    if(uriLength + asciiNameLength + 1 > MAX_URI_LENGTH)
    {
        printf("We assume bounded MAX_URI_LENGTH for simplicity.");
        exit(1);
    }

    char path[MAX_URI_LENGTH];
    memset(path, 0, MAX_URI_LENGTH);

    memcpy_s(path, MAX_URI_LENGTH, uri, uriLength);
    memcpy_s(path + uriLength, MAX_URI_LENGTH - uriLength, asciiName, asciiNameLength);

    JsTTDStreamHandle res = TTDHostOpen(uriLength + asciiNameLength, path, write);
    if(res == nullptr)
    {
        fprintf(stderr, "Failed to open file: %s\n", path);
    }

    Helpers::TTReportLastIOErrorAsNeeded(res != nullptr, "Failed File Open");
    return res;
}

bool CALLBACK Helpers::TTReadBytesFromStreamCallback(JsTTDStreamHandle handle, byte* buff, size_t size, size_t* readCount)
{
    AssertMsg(handle != nullptr, "Bad file handle.");

    if(size > MAXDWORD)
    {
        *readCount = 0;
        return false;
    }

    BOOL ok = FALSE;
    *readCount = TTDHostRead(buff, size, (FILE*)handle);
    ok = (*readCount != 0);

    Helpers::TTReportLastIOErrorAsNeeded(ok, "Failed Read!!!");

    return ok ? true : false;
}

bool CALLBACK Helpers::TTWriteBytesToStreamCallback(JsTTDStreamHandle handle, const byte* buff, size_t size, size_t* writtenCount)
{
    AssertMsg(handle != nullptr, "Bad file handle.");

    if(size > MAXDWORD)
    {
        *writtenCount = 0;
        return false;
    }

    BOOL ok = FALSE;
    *writtenCount = TTDHostWrite(buff, size, (FILE*)handle);
    ok = (*writtenCount == size);

    Helpers::TTReportLastIOErrorAsNeeded(ok, "Failed Read!!!");

    return ok ? true : false;
}

void CALLBACK Helpers::TTFlushAndCloseStreamCallback(JsTTDStreamHandle handle, bool read, bool write)
{
    fflush((FILE*)handle);
    fclose((FILE*)handle);
}

#define SET_BINARY_PATH_ERROR_MESSAGE(path, msg) \
    str_len = (int) strlen(msg);                 \
    memcpy(path, msg, (size_t)str_len);          \
    path[str_len] = char(0)

void GetBinaryLocation(char *path, const unsigned size)
{
    AssertMsg(path != nullptr, "Path can not be nullptr");
    AssertMsg(size < INT_MAX, "Isn't it too big for a path buffer?");
#ifdef _WIN32
    LPWSTR wpath = (WCHAR*)malloc(sizeof(WCHAR) * size);
    int str_len;
    if (!wpath)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed. OutOfMemory!");
        return;
    }
    str_len = GetModuleFileNameW(NULL, wpath, size - 1);
    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName has failed.");
        free(wpath);
        return;
    }

    str_len = WideCharToMultiByte(CP_UTF8, 0, wpath, str_len, path, size, NULL, NULL);
    free(wpath);

    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: GetModuleFileName (WideCharToMultiByte) has failed.");
        return;
    }

    if ((unsigned)str_len > size - 1)
    {
        str_len = (int) size - 1;
    }
    path[str_len] = char(0);
#elif defined(__APPLE__)
    uint32_t path_size = (uint32_t)size;
    char *tmp = nullptr;
    int str_len;
    if (_NSGetExecutablePath(path, &path_size))
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: _NSGetExecutablePath has failed.");
        return;
    }

    tmp = (char*)malloc(size);
    char *result = realpath(path, tmp);
    str_len = strlen(result);
    memcpy(path, result, str_len);
    free(tmp);
    path[str_len] = char(0);
#elif defined(__linux__)
    int str_len = readlink("/proc/self/exe", path, size - 1);
    if (str_len <= 0)
    {
        SET_BINARY_PATH_ERROR_MESSAGE(path, "GetBinaryLocation: /proc/self/exe has failed.");
        return;
    }
    path[str_len] = char(0);
#else
#warning "Implement GetBinaryLocation for this platform"
#endif
}

// xplat-todo: Implement a corresponding solution for GetModuleFileNameW
// and cleanup PAL. [ https://github.com/Microsoft/ChakraCore/pull/2288 should be merged first ]
// GetModuleFileName* PAL is not reliable and forces us to explicitly double initialize PAL
// with argc / argv....
void GetBinaryPathWithFileNameA(char *path, const size_t buffer_size, const char* filename)
{
    char fullpath[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];

    char modulename[_MAX_PATH];
    GetBinaryLocation(modulename, _MAX_PATH);
    _splitpath_s(modulename, drive, _MAX_DRIVE, dir, _MAX_DIR, nullptr, 0, nullptr, 0);
    _makepath_s(fullpath, drive, dir, filename, nullptr);

    size_t len = strlen(fullpath);
    if (len < buffer_size)
    {
        memcpy(path, fullpath, len * sizeof(char));
    }
    else
    {
        len = 0;
    }
    path[len] = char(0);
}
