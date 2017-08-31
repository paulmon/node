// Copyright Microsoft. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and / or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifdef NODE_ENGINE_CHAKRA

#include <v8.h>
#include "jsrtutils.h"

//#include "..\core\lib\jsrt\JsrtInternal.h"
#include "..\core\lib\common\core\Assertions.h"
#include "..\core\lib\common\codex\Utf8Helper.h"

#define PARAM_NOT_NULL(p) \
    if (p == nullptr) \
    { \
        return JsErrorNullArgument; \
    }

#define VALIDATE_JSREF(p) \
    if (p == JS_INVALID_REFERENCE) \
    { \
        return JsErrorInvalidArgument; \
    } \

inline JsErrorCode __NotImplemented(char* method)
{
    fprintf(stderr, "ERROR: Not Implemented: %s\r\n", method);
#ifdef DEBUG
    __debugbreak();
#endif
    return JsErrorNotImplemented;
}
CHAKRA_API JsTTDNotifyLongLivedReferenceAdd(_In_ JsValueRef value)
{
    return __NotImplemented("JsTTDNotifyLongLivedReferenceAdd");
}

CHAKRA_API JsTTDCheckAndAssertIfTTDRunning(_In_ const char* msg)
{
    return __NotImplemented("JsTTDCheckAndAssertIfTTDRunning");
}

CHAKRA_API JsTTDHostExit(_In_ int statusCode)
{
    return __NotImplemented("JsTTDHostExit");
}

CHAKRA_API JsTTDStart()
{
    return __NotImplemented("JsTTDStart");
}

CHAKRA_API JsTTDStop()
{
    return __NotImplemented("JsTTDStop");
}

CHAKRA_API JsTTDNotifyYield()
{
    return __NotImplemented("JsTTDNotifyYield");
}

CHAKRA_API JsCreateWeakReference(
        _In_ JsValueRef value,
        _Out_ JsWeakRef* weakRef)
{
    return __NotImplemented("JsCreateWeakReference");
}

CHAKRA_API JsCopyString(
    _In_ JsValueRef value,
    _Out_opt_ char* buffer,
    _In_ size_t bufferSize,
    _Out_opt_ size_t* writtenLength,
    _Out_opt_ size_t* actualLength)
{
    PARAM_NOT_NULL(value);
    VALIDATE_JSREF(value);

    const char16* str = nullptr;
    size_t strLength = 0;
    JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    utf8::WideToNarrow utf8Str(str, strLength);
    if (!buffer)
    {
        if (actualLength)
        {
            *actualLength = utf8Str.Length();
        }
    }
    else
    {
        size_t count = min(bufferSize, utf8Str.Length());
        // Try to copy whole characters if buffer size insufficient
        auto maxFitChars = utf8::ByteIndexIntoCharacterIndex(
            (LPCUTF8)(const char*)utf8Str, count,
            utf8::DecodeOptions::doChunkedEncoding);
        count = utf8::CharacterIndexToByteIndex(
            (LPCUTF8)(const char*)utf8Str, utf8Str.Length(), maxFitChars);

        memmove(buffer, utf8Str, sizeof(char) * count);
        if (writtenLength)
        {
            *writtenLength = count;
        }
    }

    return JsNoError;
}

template <class CopyFunc>
JsErrorCode WriteStringCopy(
    JsValueRef value,
    int start,
    int length,
    _Out_opt_ size_t* written,
    const CopyFunc& copyFunc)
{
    if (written)
    {
        *written = 0;  // init to 0 for default
    }

    const char16* str = nullptr;
    size_t strLength = 0;
    JsErrorCode errorCode = JsStringToPointer(value, &str, &strLength);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    if (start < 0 || (size_t)start > strLength)
    {
        return JsErrorInvalidArgument;  // start out of range, no chars written
    }

    size_t count = min(static_cast<size_t>(length), strLength - start);
    if (count == 0)
    {
        return JsNoError;  // no chars written
    }

    errorCode = copyFunc(str + start, count, written);
    if (errorCode != JsNoError)
    {
        return errorCode;
    }

    if (written)
    {
        *written = count;
    }

    return JsNoError;
}

CHAKRA_API JsCopyStringUtf16(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ uint16_t* buffer,
    _Out_opt_ size_t* written)
{
    PARAM_NOT_NULL(value);
    VALIDATE_JSREF(value);

    return WriteStringCopy(value, start, length, written,
        [buffer](const char16* src, size_t count, size_t *needed)
    {
        if (buffer)
        {
            memmove(buffer, src, sizeof(char16) * count);
        }
        else
        {
            *needed = count;
        }
        return JsNoError;
    });
}

CHAKRA_API JsCreateString(
    _In_ const char *content,
    _In_ size_t length,
    _Out_ JsValueRef *value)
{
    PARAM_NOT_NULL(content);

    utf8::NarrowToWide wstr(content, length);
    if (!wstr)
    {
        return JsErrorOutOfMemory;
    }

    return JsPointerToString(wstr, wstr.Length(), value);
}

CHAKRA_API JsCreateStringUtf16(
        _In_ const uint16_t *content,
        _In_ size_t length,
        _Out_ JsValueRef *value)
{
    return JsPointerToString(reinterpret_cast<const WCHAR*>(content), length, value);
}

CHAKRA_API JsCreatePropertyId(
    _In_z_ const char *name,
    _In_ size_t length,
    _Out_ JsPropertyIdRef *propertyId)
{
    PARAM_NOT_NULL(name);
    utf8::NarrowToWide wname(name, length);
    if (!wname)
    {
        return JsErrorOutOfMemory;
    }

    return JsGetPropertyIdFromName(wname, propertyId);
}

CHAKRA_API JsGetWeakReferenceValue(
        _In_ JsWeakRef weakRef,
        _Out_ JsValueRef* value)
{
    return __NotImplemented("JsGetWeakReferenceValue");
}

CHAKRA_API JsParse(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _Out_ JsValueRef *result)
{
    size_t len = 0;
    size_t copied = 0;
    size_t written = 0;
    size_t actualLength = 0;

    IfJsErrorRet(JsCopyString(script, nullptr, 0, nullptr, &actualLength));

    char* strScript = reinterpret_cast<char*>(malloc(len + 1));
    CHAKRA_VERIFY(strScript != nullptr);

    JsErrorCode errorCode = JsCopyString(script, strScript, len, &written, nullptr);

    wchar_t* wstrScript = new wchar_t[len + 1];
    CHAKRA_VERIFY(wstrScript != nullptr);

    errno_t err = mbstowcs_s(&copied, wstrScript, len + 1, strScript, len);

    IfJsErrorRet(JsCopyString(sourceUrl, nullptr, 0, nullptr, &actualLength));

    char* strUrl = reinterpret_cast<char*>(malloc(len + 1));
    CHAKRA_VERIFY(strUrl != nullptr);

    errorCode = JsCopyString(sourceUrl, strUrl, len, &written, nullptr);

    wchar_t* wstrUrl = new wchar_t[len + 1];
    CHAKRA_VERIFY(wstrUrl != nullptr);

    err = mbstowcs_s(&copied, wstrUrl, len + 1, strUrl, len);

    if (errorCode == JsNoError) {
        errorCode = JsParseScript(wstrScript, sourceContext, wstrUrl, result);
    }

    return errorCode;
}

CHAKRA_API JsDiagGetBreakpoints(
            _Out_ JsValueRef *breakpoints)
{
    return __NotImplemented("JsDiagGetBreakpoints");
}

CHAKRA_API JsDiagGetFunctionPosition(
            _In_ JsValueRef function,
            _Out_ JsValueRef *functionPosition)
{
    return __NotImplemented("JsDiagGetFunctionPosition");
}

CHAKRA_API JsDiagStartDebugging(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsDiagDebugEventCallback debugEventCallback,
            _In_opt_ void* callbackState)
{
    return __NotImplemented("JsDiagStartDebugging");
}

CHAKRA_API JsDiagRemoveBreakpoint(
            _In_ unsigned int breakpointId)
{
    return __NotImplemented("JsDiagRemoveBreakpoint");
}

CHAKRA_API JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
    return __NotImplemented("JsTTDPauseTimeTravelBeforeRuntimeOperation");
}

CHAKRA_API JsTTDReStartTimeTravelAfterRuntimeOperation()
{
    return __NotImplemented("JsTTDReStartTimeTravelAfterRuntimeOperation");
}

CHAKRA_API JsDiagRequestAsyncBreak(
            _In_ JsRuntimeHandle runtimeHandle)
{
    return __NotImplemented("JsDiagRequestAsyncBreak");
}

CHAKRA_API JsDiagGetObjectFromHandle(
            _In_ unsigned int objectHandle,
            _Out_ JsValueRef *handleObject)
{
    return __NotImplemented("JsDiagGetObjectFromHandle");
}

CHAKRA_API JsDiagGetSource(
            _In_ unsigned int scriptId,
            _Out_ JsValueRef *source)
{
    return __NotImplemented("JsDiagGetSource");
}

CHAKRA_API JsDiagGetStackProperties(
            _In_ unsigned int stackFrameIndex,
            _Out_ JsValueRef *properties)
{
    return __NotImplemented("JsDiagGetStackProperties");
}

CHAKRA_API JsDiagGetProperties(
            _In_ unsigned int objectHandle,
            _In_ unsigned int fromCount,
            _In_ unsigned int totalCount,
            _Out_ JsValueRef *propertiesObject)
{
    return __NotImplemented("JsDiagGetProperties");
}

CHAKRA_API JsTTDGetSnapShotBoundInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t targetEventTime,
        _Out_ int64_t* startSnapTime,
        _Out_ int64_t* endSnapTime)
{
    return __NotImplemented("JsTTDGetSnapShotBoundInterval");
}

CHAKRA_API JsTTDGetPreviousSnapshotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t currentSnapStartTime,
        _Out_ int64_t* previousSnapTime)
{
    return __NotImplemented("JsTTDGetPreviousSnapshotInterval");
}

CHAKRA_API JsTTDNotifyContextDestroy(
        _In_ JsContextRef context)
{
    return __NotImplemented("JsTTDNotifyContextDestroy");
}

CHAKRA_API JsTTDCreateRecordRuntime(
            _In_ JsRuntimeAttributes attributes,
            _In_ size_t snapInterval,
            _In_ size_t snapHistoryLength,
            _In_ TTDOpenResourceStreamCallback openResourceStream,
            _In_ JsTTDWriteBytesToStreamCallback writeBytesToStream,
            _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
            _In_opt_ JsThreadServiceCallback threadService,
            _Out_ JsRuntimeHandle *runtime)
{
    return __NotImplemented("JsTTDCreateRecordRuntime");
}


CHAKRA_API JsTTDCreateReplayRuntime(
            _In_ JsRuntimeAttributes attributes,
            _In_reads_(infoUriCount) const char* infoUri,
            _In_ size_t infoUriCount,
            _In_ bool enableDebugging,
            _In_ TTDOpenResourceStreamCallback openResourceStream,
            _In_ JsTTDReadBytesFromStreamCallback readBytesFromStream,
            _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
            _In_opt_ JsThreadServiceCallback threadService,
            _Out_ JsRuntimeHandle *runtime)
{
    return __NotImplemented("JsTTDCreateReplayRuntime");
}

CHAKRA_API JsTTDGetSnapTimeTopLevelEventMove(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ JsTTDMoveMode moveMode,
        _In_opt_ uint32_t kthEvent,
        _Inout_ int64_t* targetEventTime,
        _Out_ int64_t* targetStartSnapTime,
        _Out_opt_ int64_t* targetEndSnapTime)
{
    return __NotImplemented("JsTTDGetSnapTimeTopLevelEventMove");
}

CHAKRA_API JsTTDPreExecuteSnapShotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t startSnapTime,
        _In_ int64_t endSnapTime,
        _In_ JsTTDMoveMode moveMode,
        _Out_ int64_t* newTargetEventTime)
{
    return __NotImplemented("JsTTDPreExecuteSnapShotInterval");
}

CHAKRA_API JsTTDReplayExecution(
            _Inout_ JsTTDMoveMode* moveMode,
            _Out_ int64_t* rootEventTime)
{
    return __NotImplemented("JsTTDReplayExecution");
}

CHAKRA_API JsTTDRawBufferCopySyncIndirect(
            _In_ JsValueRef dst,
            _In_ size_t dstIndex,
            _In_ JsValueRef src,
            _In_ size_t srcIndex,
            _In_ size_t count)
{
    return __NotImplemented("JsTTDRawBufferCopySyncIndirect");
}

CHAKRA_API JsTTDRawBufferModifySyncIndirect(
            _In_ JsValueRef buffer,
            _In_ size_t index,
            _In_ size_t count)
{
    return __NotImplemented("JsTTDRawBufferModifySyncIndirect");
}

CHAKRA_API JsTTDRawBufferAsyncModifyComplete(
            _In_ byte* finalModPos)
{
    return __NotImplemented("JsTTDRawBufferAsyncModifyComplete");
}

CHAKRA_API JsTTDRawBufferAsyncModificationRegister(
            _In_ JsValueRef instance,
            _In_ byte* initialModPos)
{
    return __NotImplemented("JsTTDRawBufferAsyncModificationRegister");
}

CHAKRA_API JsGetAndClearExceptionWithMetadata(
    _Out_ JsValueRef *metadata)
{
    return __NotImplemented("JsGetAndClearExceptionWithMetadata");
}

CHAKRA_API JsTTDMoveToTopLevelEvent(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsTTDMoveMode moveMode,
            _In_ int64_t snapshotTime,
            _In_ int64_t eventTime)
{
    return __NotImplemented("JsTTDMoveToTopLevelEvent");
}

CHAKRA_API JsHasOwnProperty(
    _In_ JsValueRef object,
    _In_ JsPropertyIdRef propertyId,
    _Out_ bool *hasOwnProperty)
{
    return __NotImplemented("JsHasOwnProperty");
}

CHAKRA_API JsGetDataViewInfo(
    _In_ JsValueRef dataView,
    _Out_opt_ JsValueRef *arrayBuffer,
    _Out_opt_ unsigned int *byteOffset,
    _Out_opt_ unsigned int *byteLength)
{
    return __NotImplemented("JsGetDataViewInfo");
}

CHAKRA_API JsCopyStringOneByte(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ char* buffer,
    _Out_opt_ size_t* written)
{
    return __NotImplemented("JsCopyStringOneByte");
}

#endif // NODE_ENGINE_CHAKRA