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

CHAKRA_API JsTTDNotifyLongLivedReferenceAdd(_In_ JsValueRef value)  { return JsNoError; }
CHAKRA_API JsTTDCheckAndAssertIfTTDRunning(_In_ const char* msg)    { return JsNoError; }
CHAKRA_API JsTTDHostExit(_In_ int statusCode)                       { return JsNoError; }
CHAKRA_API JsTTDStart()                                             { return JsNoError; }
CHAKRA_API JsTTDStop()                                              { return JsNoError; }
CHAKRA_API JsTTDNotifyYield()                                       { return JsNoError; }

CHAKRA_API JsCreateWeakReference(
        _In_ JsValueRef value,
        _Out_ JsWeakRef* weakRef)
{
    return JsNoError;
}

CHAKRA_API
    JsCopyString(
        _In_ JsValueRef value,
        _Out_opt_ char* buffer,
        _In_ size_t bufferSize,
        _Out_opt_ size_t* written)
{
    return JsNoError;
}

CHAKRA_API
    JsCopyStringUtf16(
        _In_ JsValueRef value,
        _In_ int start,
        _In_ int length,
        _Out_opt_ uint16_t* buffer,
        _Out_opt_ size_t* written)
{
    return JsNoError;
}


CHAKRA_API
    JsCreateString(
        _In_ const char *content,
        _In_ size_t length,
        _Out_ JsValueRef *value)
{
    return JsNoError;
}

CHAKRA_API
    JsCreateStringUtf16(
        _In_ const uint16_t *content,
        _In_ size_t length,
        _Out_ JsValueRef *value)
{
    return JsNoError;
}

CHAKRA_API
    JsCreatePropertyId(
        _In_z_ const char *name,
        _In_ size_t length,
        _Out_ JsPropertyIdRef *propertyId)
{
    return JsNoError;
}

CHAKRA_API
    JsGetWeakReferenceValue(
        _In_ JsWeakRef weakRef,
        _Out_ JsValueRef* value)
{
    return JsNoError;
}

CHAKRA_API
    JsParse(
        _In_ JsValueRef script,
        _In_ JsSourceContext sourceContext,
        _In_ JsValueRef sourceUrl,
        _In_ JsParseScriptAttributes parseAttributes,
        _Out_ JsValueRef *result)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetBreakpoints(
            _Out_ JsValueRef *breakpoints)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetFunctionPosition(
            _In_ JsValueRef function,
            _Out_ JsValueRef *functionPosition)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagStartDebugging(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsDiagDebugEventCallback debugEventCallback,
            _In_opt_ void* callbackState)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagRemoveBreakpoint(
            _In_ unsigned int breakpointId)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDPauseTimeTravelBeforeRuntimeOperation()
{
    return JsNoError;
}

CHAKRA_API
        JsTTDReStartTimeTravelAfterRuntimeOperation()
{
    return JsNoError;
}

CHAKRA_API
        JsDiagRequestAsyncBreak(
            _In_ JsRuntimeHandle runtimeHandle)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetObjectFromHandle(
            _In_ unsigned int objectHandle,
            _Out_ JsValueRef *handleObject)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetSource(
            _In_ unsigned int scriptId,
            _Out_ JsValueRef *source)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetStackProperties(
            _In_ unsigned int stackFrameIndex,
            _Out_ JsValueRef *properties)
{
    return JsNoError;
}

CHAKRA_API
        JsDiagGetProperties(
            _In_ unsigned int objectHandle,
            _In_ unsigned int fromCount,
            _In_ unsigned int totalCount,
            _Out_ JsValueRef *propertiesObject)
{
    return JsNoError;
}

CHAKRA_API JsTTDGetSnapShotBoundInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t targetEventTime,
        _Out_ int64_t* startSnapTime,
        _Out_ int64_t* endSnapTime)
{
    return JsNoError;
}

    CHAKRA_API JsTTDGetPreviousSnapshotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t currentSnapStartTime,
        _Out_ int64_t* previousSnapTime)
{
    return JsNoError;
}

CHAKRA_API JsTTDNotifyContextDestroy(
        _In_ JsContextRef context)
{
    return JsNoError;
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
    return JsNoError;
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
    return JsNoError;
}

CHAKRA_API JsTTDGetSnapTimeTopLevelEventMove(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ JsTTDMoveMode moveMode,
        _In_opt_ uint32_t kthEvent,
        _Inout_ int64_t* targetEventTime,
        _Out_ int64_t* targetStartSnapTime,
        _Out_opt_ int64_t* targetEndSnapTime)
{
    return JsNoError;
}

CHAKRA_API JsTTDPreExecuteSnapShotInterval(
        _In_ JsRuntimeHandle runtimeHandle,
        _In_ int64_t startSnapTime,
        _In_ int64_t endSnapTime,
        _In_ JsTTDMoveMode moveMode,
        _Out_ int64_t* newTargetEventTime)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDReplayExecution(
            _Inout_ JsTTDMoveMode* moveMode,
            _Out_ int64_t* rootEventTime)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDRawBufferCopySyncIndirect(
            _In_ JsValueRef dst,
            _In_ size_t dstIndex,
            _In_ JsValueRef src,
            _In_ size_t srcIndex,
            _In_ size_t count)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDRawBufferModifySyncIndirect(
            _In_ JsValueRef buffer,
            _In_ size_t index,
            _In_ size_t count)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDRawBufferAsyncModifyComplete(
            _In_ byte* finalModPos)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDRawBufferAsyncModificationRegister(
            _In_ JsValueRef instance,
            _In_ byte* initialModPos)
{
    return JsNoError;
}

CHAKRA_API
JsGetAndClearExceptionWithMetadata(
    _Out_ JsValueRef *metadata)
{
    return JsNoError;
}

CHAKRA_API
        JsTTDMoveToTopLevelEvent(
            _In_ JsRuntimeHandle runtimeHandle,
            _In_ JsTTDMoveMode moveMode,
            _In_ int64_t snapshotTime,
            _In_ int64_t eventTime)
{
    return JsNoError;
}

#endif // NODE_ENGINE_CHAKRA