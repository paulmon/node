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

CHAKRA_API JsTTDNotifyLongLivedReferenceAdd(_In_ JsValueRef value)  
{ 
    fprintf(stderr, "ERROR: Not Implemented: JsTTDNotifyLongLivedReferenceAdd\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDCheckAndAssertIfTTDRunning(_In_ const char* msg)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDCheckAndAssertIfTTDRunning\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDHostExit(_In_ int statusCode)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDHostExit\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDStart()
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDStart\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDStop()
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDStop\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDNotifyYield()
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDNotifyYield\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsCreateWeakReference(
        _In_ JsValueRef value,
        _Out_ JsWeakRef* weakRef)
{
    fprintf(stderr, "ERROR: Not Implemented: JsCreateWeakReference\r\n");
    return JsErrorNotImplemented;
}

#if 0
CHAKRA_API
    JsCopyString(
        _In_ JsValueRef value,
        _Out_opt_ char* buffer,
        _In_ size_t bufferSize,
        _Out_opt_ size_t* written)
{
    //if (bufferSize > 26000)
    //    __debugbreak();

    size_t wBufferSize = bufferSize;
    size_t converted = 0;
    const wchar_t* wbuffer = nullptr;

    auto result = JsStringToPointer(value, &wbuffer, &wBufferSize);
    if (buffer != nullptr)
    {
        wcstombs_s(&converted, buffer, bufferSize, wbuffer, wBufferSize);
        *written = converted;
    }
    else
    {
        if (wBufferSize == 26495) __debugbreak();

        wcstombs_s(&converted, nullptr, 0, wbuffer, wBufferSize);
        *written = converted;
    }
    return result;
}
#else
CHAKRA_API
JsCopyString(
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
#endif

#if 0
CHAKRA_API
    JsCopyStringUtf16(
        _In_ JsValueRef value,
        _In_ int start,
        _In_ int length,
        _Out_opt_ uint16_t* buffer,
        _Out_opt_ size_t* written)
{
    fprintf(stderr, "ERROR: Not Implemented: JsCopyStringUtf16\r\n");
    return JsErrorNotImplemented;
}
#else
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
#endif

#if 0
CHAKRA_API
    JsCreateString(
        _In_ const char *content,
        _In_ size_t length,
        _Out_ JsValueRef *value)
{
    if (length > 260000)
        __debugbreak();

    wchar_t* wcontent = new wchar_t[length + 1];
    if (wcontent)
    {
        size_t newLen;
        mbstowcs_s(&newLen, wcontent, length + 1, content, length);
        JsErrorCode result = JsPointerToString(wcontent, length, value);
        delete[] wcontent;
        return result;
    }
    return JsErrorOutOfMemory;
}
#else
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
#endif

CHAKRA_API
    JsCreateStringUtf16(
        _In_ const uint16_t *content,
        _In_ size_t length,
        _Out_ JsValueRef *value)
{
    return JsPointerToString(reinterpret_cast<const WCHAR*>(content), length, value);
}

#if 0
CHAKRA_API
    JsCreatePropertyId(
        _In_z_ const char *name,
        _In_ size_t length,
        _Out_ JsPropertyIdRef *propertyId)
{
    fprintf(stderr, "ERROR: Not Implemented: JsCreatePropertyId\r\n");
    return JsErrorNotImplemented;
    //wchar_t* wname = new wchar_t[length+1];
    //if (wname)
    //{
    //    size_t newLen = 0;
    //    mbstowcs_s(&newLen, wname, length + 1, name, length);
    //    JsErrorCode result = JsGetPropertyIdFromName(wname, propertyId);
    //    delete[] wname;
    //    return result;
    //}
    //return JsErrorOutOfMemory;
}
#else
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
#endif

CHAKRA_API
    JsGetWeakReferenceValue(
        _In_ JsWeakRef weakRef,
        _Out_ JsValueRef* value)
{
    fprintf(stderr, "ERROR: Not Implemented: JsGetWeakReferenceValue\r\n");
    return JsErrorNotImplemented;
}

#if 1
CHAKRA_API
    JsParse(
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
#else
#define _ALWAYSINLINE __forceinline
_ALWAYSINLINE JsErrorCode CompileRun(
    JsValueRef scriptVal,
    JsSourceContext sourceContext,
    JsValueRef sourceUrl,
    JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result,
    bool parseOnly)
{
    PARAM_NOT_NULL(scriptVal);
    VALIDATE_JSREF(scriptVal);
    PARAM_NOT_NULL(sourceUrl);

    bool isExternalArray = Js::ExternalArrayBuffer::Is(scriptVal),
        isString = false;
    bool isUtf8 = !(parseAttributes & JsParseScriptAttributeArrayBufferIsUtf16Encoded);

    LoadScriptFlag scriptFlag = LoadScriptFlag_None;
    const byte* script;
    size_t cb;
    const WCHAR *url;

    if (isExternalArray)
    {
        script = ((Js::ExternalArrayBuffer*)(scriptVal))->GetBuffer();

        cb = ((Js::ExternalArrayBuffer*)(scriptVal))->GetByteLength();

        scriptFlag = (LoadScriptFlag)(isUtf8 ?
            LoadScriptFlag_ExternalArrayBuffer | LoadScriptFlag_Utf8Source :
            LoadScriptFlag_ExternalArrayBuffer);
    }
    else
    {
        isString = Js::JavascriptString::Is(scriptVal);
        if (!isString)
        {
            return JsErrorInvalidArgument;
        }
    }

    JsErrorCode error = GlobalAPIWrapper_NoRecord([&]() -> JsErrorCode {
        if (isString)
        {
            Js::JavascriptString* jsString = Js::JavascriptString::FromVar(scriptVal);
            script = (const byte*)jsString->GetSz();

            // JavascriptString is 2 bytes (WCHAR/char16)
            cb = jsString->GetLength() * sizeof(WCHAR);
        }

        if (!Js::JavascriptString::Is(sourceUrl))
        {
            return JsErrorInvalidArgument;
        }

        url = Js::JavascriptString::FromVar(sourceUrl)->GetSz();

        return JsNoError;

    });

    if (error != JsNoError)
    {
        return error;
    }

    return RunScriptCore(scriptVal, script, cb, scriptFlag,
        sourceContext, url, parseOnly, parseAttributes, false, result);
}

CHAKRA_API JsParse(
    _In_ JsValueRef scriptVal,
    _In_ JsSourceContext sourceContext,
    _In_ JsValueRef sourceUrl,
    _In_ JsParseScriptAttributes parseAttributes,
    _Out_ JsValueRef *result)
{
    return CompileRun(scriptVal, sourceContext, sourceUrl, parseAttributes,
        result, true);
}
#endif

CHAKRA_API
        JsDiagGetBreakpoints(
            _Out_ JsValueRef *breakpoints)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetBreakpoints\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagGetFunctionPosition(
            _In_ JsValueRef function,
            _Out_ JsValueRef *functionPosition)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetFunctionPosition\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagStartDebugging(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsDiagDebugEventCallback debugEventCallback,
            _In_opt_ void* callbackState)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagStartDebugging\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagRemoveBreakpoint(
            _In_ unsigned int breakpointId)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagRemoveBreakpoint\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDPauseTimeTravelBeforeRuntimeOperation\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDReStartTimeTravelAfterRuntimeOperation()
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDReStartTimeTravelAfterRuntimeOperation\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagRequestAsyncBreak(
            _In_ JsRuntimeHandle runtimeHandle)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagRequestAsyncBreak\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagGetObjectFromHandle(
            _In_ unsigned int objectHandle,
            _Out_ JsValueRef *handleObject)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetObjectFromHandle\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagGetSource(
            _In_ unsigned int scriptId,
            _Out_ JsValueRef *source)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetSource\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagGetStackProperties(
            _In_ unsigned int stackFrameIndex,
            _Out_ JsValueRef *properties)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetStackProperties\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsDiagGetProperties(
            _In_ unsigned int objectHandle,
            _In_ unsigned int fromCount,
            _In_ unsigned int totalCount,
            _Out_ JsValueRef *propertiesObject)
{
    fprintf(stderr, "ERROR: Not Implemented: JsDiagGetProperties\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDGetSnapShotBoundInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t targetEventTime,
        _Out_ int64_t* startSnapTime,
        _Out_ int64_t* endSnapTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDGetSnapShotBoundInterval\r\n");
    return JsErrorNotImplemented;
}

    CHAKRA_API JsTTDGetPreviousSnapshotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t currentSnapStartTime,
        _Out_ int64_t* previousSnapTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDGetPreviousSnapshotInterval\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDNotifyContextDestroy(
        _In_ JsContextRef context)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDNotifyContextDestroy\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDCreateRecordRuntime(
            _In_ JsRuntimeAttributes attributes,
            _In_ size_t snapInterval,
            _In_ size_t snapHistoryLength,
            _In_ TTDOpenResourceStreamCallback openResourceStream,
            _In_ JsTTDWriteBytesToStreamCallback writeBytesToStream,
            _In_ JsTTDFlushAndCloseStreamCallback flushAndCloseStream,
            _In_opt_ JsThreadServiceCallback threadService,
            _Out_ JsRuntimeHandle *runtime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDCreateRecordRuntime\r\n");
    return JsErrorNotImplemented;
}


CHAKRA_API
        JsTTDCreateReplayRuntime(
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
    fprintf(stderr, "ERROR: Not Implemented: JsTTDCreateReplayRuntime\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDGetSnapTimeTopLevelEventMove(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ JsTTDMoveMode moveMode,
        _In_opt_ uint32_t kthEvent,
        _Inout_ int64_t* targetEventTime,
        _Out_ int64_t* targetStartSnapTime,
        _Out_opt_ int64_t* targetEndSnapTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDGetSnapTimeTopLevelEventMove\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API JsTTDPreExecuteSnapShotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t startSnapTime,
        _In_ int64_t endSnapTime,
        _In_ JsTTDMoveMode moveMode,
        _Out_ int64_t* newTargetEventTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDPreExecuteSnapShotInterval\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDReplayExecution(
            _Inout_ JsTTDMoveMode* moveMode,
            _Out_ int64_t* rootEventTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDReplayExecution\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDRawBufferCopySyncIndirect(
            _In_ JsValueRef dst,
            _In_ size_t dstIndex,
            _In_ JsValueRef src,
            _In_ size_t srcIndex,
            _In_ size_t count)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDRawBufferCopySyncIndirect\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDRawBufferModifySyncIndirect(
            _In_ JsValueRef buffer,
            _In_ size_t index,
            _In_ size_t count)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDRawBufferModifySyncIndirect\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDRawBufferAsyncModifyComplete(
            _In_ byte* finalModPos)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDRawBufferAsyncModifyComplete\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
        JsTTDRawBufferAsyncModificationRegister(
            _In_ JsValueRef instance,
            _In_ byte* initialModPos)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDRawBufferAsyncModificationRegister\r\n");
    return JsErrorNotImplemented;
}

#if 1
CHAKRA_API
JsGetAndClearExceptionWithMetadata(
    _Out_ JsValueRef *metadata)
{
    fprintf(stderr, "ERROR: Not Implemented: JsGetAndClearExceptionWithMetadata\r\n");
    return JsErrorNotImplemented;
}
#else
CHAKRA_API JsGetAndClearExceptionWithMetadata(_Out_ JsValueRef *metadata)
{
    PARAM_NOT_NULL(metadata);
    *metadata = nullptr;

    JsrtContext *currentContext = JsrtContext::GetCurrent();

    if (currentContext == nullptr)
    {
        return JsErrorNoCurrentContext;
    }

    Js::ScriptContext *scriptContext = currentContext->GetScriptContext();
    Assert(scriptContext != nullptr);

    if (scriptContext->GetRecycler() && scriptContext->GetRecycler()->IsHeapEnumInProgress())
    {
        return JsErrorHeapEnumInProgress;
    }
    else if (scriptContext->GetThreadContext()->IsInThreadServiceCallback())
    {
        return JsErrorInThreadServiceCallback;
    }

    if (scriptContext->GetThreadContext()->IsExecutionDisabled())
    {
        return JsErrorInDisabledState;
    }

    HRESULT hr = S_OK;
    Js::JavascriptExceptionObject *recordedException = nullptr;

    BEGIN_TRANSLATE_OOM_TO_HRESULT
        if (scriptContext->HasRecordedException())
        {
            recordedException = scriptContext->GetAndClearRecordedException();
        }
    END_TRANSLATE_OOM_TO_HRESULT(hr)

        if (hr == E_OUTOFMEMORY)
        {
            recordedException = scriptContext->GetThreadContext()->GetRecordedException();
        }
    if (recordedException == nullptr)
    {
        return JsErrorInvalidArgument;
    }

    Js::Var exception = recordedException->GetThrownObject(nullptr);

    if (exception == nullptr)
    {
        // TODO: How does this early bailout impact TTD?
        return JsErrorInvalidArgument;
    }

    return ContextAPIWrapper<false>([&](Js::ScriptContext* scriptContext, TTDRecorder& _actionEntryPopper) -> JsErrorCode {
        Js::Var exceptionMetadata = Js::JavascriptExceptionMetadata::CreateMetadataVar(scriptContext);
        Js::JavascriptOperators::OP_SetProperty(exceptionMetadata, Js::PropertyIds::exception, exception, scriptContext);

        Js::FunctionBody *functionBody = recordedException->GetFunctionBody();
        if (functionBody == nullptr)
        {
            // This is probably a parse error. We can get the error location metadata from the thrown object.
            Js::JavascriptExceptionMetadata::PopulateMetadataFromCompileException(exceptionMetadata, exception, scriptContext);
        }
        else
        {
            if (!Js::JavascriptExceptionMetadata::PopulateMetadataFromException(exceptionMetadata, recordedException, scriptContext))
            {
                return JsErrorInvalidArgument;
            }
        }

        *metadata = exceptionMetadata;

#if ENABLE_TTD
        if (hr != E_OUTOFMEMORY)
        {
            PERFORM_JSRT_TTD_RECORD_ACTION(scriptContext, RecordJsRTGetAndClearExceptionWithMetadata);
            PERFORM_JSRT_TTD_RECORD_ACTION_RESULT(scriptContext, metadata);
        }
#endif


        return JsNoError;
    });
}
#endif

CHAKRA_API
        JsTTDMoveToTopLevelEvent(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsTTDMoveMode moveMode,
            _In_ int64_t snapshotTime,
            _In_ int64_t eventTime)
{
    fprintf(stderr, "ERROR: Not Implemented: JsTTDMoveToTopLevelEvent\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
JsHasOwnProperty(
    _In_ JsValueRef object,
    _In_ JsPropertyIdRef propertyId,
    _Out_ bool *hasOwnProperty)
{
    fprintf(stderr, "ERROR: Not Implemented: JsHasOwnProperty\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
JsGetDataViewInfo(
    _In_ JsValueRef dataView,
    _Out_opt_ JsValueRef *arrayBuffer,
    _Out_opt_ unsigned int *byteOffset,
    _Out_opt_ unsigned int *byteLength)
{
    fprintf(stderr, "ERROR: Not Implemented: JsGetDataViewInfo\r\n");
    return JsErrorNotImplemented;
}

CHAKRA_API
JsCopyStringOneByte(
    _In_ JsValueRef value,
    _In_ int start,
    _In_ int length,
    _Out_opt_ char* buffer,
    _Out_opt_ size_t* written)
{
    fprintf(stderr, "ERROR: Not Implemented: JsCopyStringOneByte\r\n");
    return JsErrorNotImplemented;
}

#endif // NODE_ENGINE_CHAKRA