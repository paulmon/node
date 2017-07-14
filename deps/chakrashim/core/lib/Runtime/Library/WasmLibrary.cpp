//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

#ifdef ENABLE_WASM
#include "../WasmReader/WasmReaderPch.h"
// Included for AsmJsDefaultEntryThunk
#include "Language/InterpreterStackFrame.h"

namespace Js
{

    Var WasmLibrary::WasmLazyTrapCallback(RecyclableObject *callee, CallInfo, ...)
    {
        AsmJsScriptFunction* asmFunction = static_cast<AsmJsScriptFunction*>(callee);
        Assert(asmFunction);
        ScriptContext * scriptContext = asmFunction->GetScriptContext();
        Assert(scriptContext);
        auto error = asmFunction->GetFunctionBody()->GetAsmJsFunctionInfo()->GetLazyError();
        JavascriptExceptionOperators::Throw(error, scriptContext);
    }

    void WasmLibrary::SetWasmEntryPointToInterpreter(Js::ScriptFunction* func, bool deferParse)
    {
        Assert(AsmJsScriptFunction::Is(func));
        FunctionEntryPointInfo* entrypointInfo = (FunctionEntryPointInfo*)func->GetEntryPointInfo();
        entrypointInfo->SetIsAsmJSFunction(true);

        if (deferParse)
        {
            func->SetEntryPoint(WasmLibrary::WasmDeferredParseExternalThunk);
            entrypointInfo->jsMethod = WasmLibrary::WasmDeferredParseInternalThunk;
        }
        else
        {
            func->SetEntryPoint(Js::AsmJsExternalEntryPoint);
            entrypointInfo->jsMethod = AsmJsDefaultEntryThunk;
        }
    }

#if _M_IX86
    __declspec(naked)
        Var WasmLibrary::WasmDeferredParseExternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Register functions
        __asm
        {
            push ebp
            mov ebp, esp
            lea eax, [esp + 8]
            push 0
            push eax
            call WasmLibrary::WasmDeferredParseEntryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
            jmp eax
        }
    }

    __declspec(naked)
        Var WasmLibrary::WasmDeferredParseInternalThunk(RecyclableObject* function, CallInfo callInfo, ...)
    {
        // Register functions
        __asm
        {
            push ebp
            mov ebp, esp
            lea eax, [esp + 8]
            push 1
            push eax
            call WasmLibrary::WasmDeferredParseEntryPoint
#ifdef _CONTROL_FLOW_GUARD
            // verify that the call target is valid
            mov  ecx, eax
            call[__guard_check_icall_fptr]
            mov eax, ecx
#endif
            pop ebp
            // Although we don't restore ESP here on WinCE, this is fine because script profiler is not shipped for WinCE.
            jmp eax
        }
    }
#elif defined(_M_X64)
    // Do nothing: the implementation of WasmLibrary::WasmDeferredParseExternalThunk is declared (appropriately decorated) in
    // Language\amd64\amd64_Thunks.asm.
#endif // _M_IX86

}

#endif // ENABLE_WASM

Js::JavascriptMethod Js::WasmLibrary::WasmDeferredParseEntryPoint(Js::AsmJsScriptFunction** funcPtr, int internalCall)
{
#ifdef ENABLE_WASM
    AsmJsScriptFunction* func = *funcPtr;

    FunctionBody* body = func->GetFunctionBody();
    AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
    ScriptContext* scriptContext = func->GetScriptContext();

    Js::FunctionEntryPointInfo * entrypointInfo = (Js::FunctionEntryPointInfo*)func->GetEntryPointInfo();
    Wasm::WasmReaderInfo* readerInfo = info->GetWasmReaderInfo();
    if (readerInfo)
    {
        info->SetWasmReaderInfo(nullptr);
        try
        {
            Wasm::WasmBytecodeGenerator::GenerateFunctionBytecode(scriptContext, readerInfo);
            func->GetDynamicType()->SetEntryPoint(Js::AsmJsExternalEntryPoint);
            entrypointInfo->jsMethod = AsmJsDefaultEntryThunk;
            // Do MTJRC/MAIC:0 check
#if ENABLE_DEBUG_CONFIG_OPTIONS
            if (CONFIG_FLAG(ForceNative) || CONFIG_FLAG(MaxAsmJsInterpreterRunCount) == 0)
            {
                GenerateFunction(scriptContext->GetNativeCodeGenerator(), body, func);
                body->SetIsAsmJsFullJitScheduled(true);
            }
#endif
        }
        catch (Wasm::WasmCompilationException& ex)
        {
            char16* originalMessage = ex.ReleaseErrorMessage();
            intptr_t offset = readerInfo->m_module->GetReader()->GetCurrentOffset();
            intptr_t start = readerInfo->m_funcInfo->m_readerInfo.startOffset;
            uint32 size = readerInfo->m_funcInfo->m_readerInfo.size;

            Wasm::WasmCompilationException newEx = Wasm::WasmCompilationException(
                _u("function %s at offset %d/%d: %s"),
                body->GetDisplayName(),
                offset - start,
                size,
                originalMessage
            );
            SysFreeString(originalMessage);
            char16* msg = newEx.ReleaseErrorMessage();
            JavascriptLibrary *library = scriptContext->GetLibrary();
            JavascriptError *pError = library->CreateWebAssemblyCompileError();
            JavascriptError::SetErrorMessage(pError, WASMERR_WasmCompileError, msg, scriptContext);

            func->GetDynamicType()->SetEntryPoint(WasmLazyTrapCallback);
            entrypointInfo->jsMethod = WasmLazyTrapCallback;
            info->SetLazyError(pError);
        }
    }
    else
    {
        // This can happen if another function had its type changed and then was parsed
        // They still share the function body, so just change the entry point
        Assert(body->GetByteCodeCount() > 0);
        Js::JavascriptMethod externalEntryPoint = info->GetLazyError() ? WasmLazyTrapCallback : Js::AsmJsExternalEntryPoint;
        func->GetDynamicType()->SetEntryPoint(externalEntryPoint);
        if (body->GetIsAsmJsFullJitScheduled())
        {
            Js::FunctionEntryPointInfo* defaultEntryPoint = (Js::FunctionEntryPointInfo*)body->GetDefaultEntryPointInfo();
            func->ChangeEntryPoint(defaultEntryPoint, defaultEntryPoint->jsMethod);
        }
        else if (entrypointInfo->jsMethod == WasmLibrary::WasmDeferredParseInternalThunk)
        {
            // The entrypointInfo is still shared even if the type has been changed
            // However, no sibling functions changed this entry point yet, so fix it
            entrypointInfo->jsMethod = info->GetLazyError() ? WasmLazyTrapCallback : AsmJsDefaultEntryThunk;
        }
    }

    Assert(body->HasValidEntryPoint());
    Js::JavascriptMethod entryPoint = internalCall ? entrypointInfo->jsMethod : func->GetDynamicType()->GetEntryPoint();
    return entryPoint;
#else
    Js::Throw::InternalError();
#endif
}
