//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "WasmReaderPch.h"

#ifdef ENABLE_WASM

#if DBG_DUMP
#define DebugPrintOp(op) if (PHASE_TRACE(Js::WasmReaderPhase, GetFunctionBody())) { PrintOpName(op); }
#else
#define DebugPrintOp(op)
#endif

namespace Wasm
{
#define WASM_SIGNATURE(id, nTypes, ...) const WasmTypes::WasmType WasmOpCodeSignatures::id[] = {__VA_ARGS__};
#include "WasmBinaryOpCodes.h"

#if DBG_DUMP
void
WasmBytecodeGenerator::PrintOpName(WasmOp op) const
{
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
case wb##opname: \
    Output::Print(_u("%s\r\n"), _u(#opname)); \
    break;
#include "WasmBinaryOpCodes.h"
    }
}
#endif

/* static */
Js::AsmJsRetType
WasmToAsmJs::GetAsmJsReturnType(WasmTypes::WasmType wasmType)
{
    switch (wasmType)
    {
    case WasmTypes::I32: return Js::AsmJsRetType::Signed;
    case WasmTypes::I64: return Js::AsmJsRetType::Int64;
    case WasmTypes::F32: return Js::AsmJsRetType::Float;
    case WasmTypes::F64: return Js::AsmJsRetType::Double;
    case WasmTypes::Void: return Js::AsmJsRetType::Void;
    default:
        throw WasmCompilationException(_u("Unknown return type %u"), wasmType);
    }
}

/* static */
Js::AsmJsVarType
WasmToAsmJs::GetAsmJsVarType(WasmTypes::WasmType wasmType)
{
    Js::AsmJsVarType asmType = Js::AsmJsVarType::Int;
    switch (wasmType)
    {
    case WasmTypes::I32: return Js::AsmJsVarType::Int;
    case WasmTypes::I64: return Js::AsmJsVarType::Int64;
    case WasmTypes::F32: return Js::AsmJsVarType::Float;
    case WasmTypes::F64: return Js::AsmJsVarType::Double;
    default:
        throw WasmCompilationException(_u("Unknown var type %u"), wasmType);
    }
}

typedef bool(*SectionProcessFunc)(WasmModuleGenerator*);
typedef void(*AfterSectionCallback)(WasmModuleGenerator*);

WasmModuleGenerator::WasmModuleGenerator(Js::ScriptContext* scriptContext, Js::Utf8SourceInfo* sourceInfo, const byte* binaryBuffer, uint binaryBufferLength) :
    m_sourceInfo(sourceInfo),
    m_scriptContext(scriptContext),
    m_recycler(scriptContext->GetRecycler())
{
    m_module = RecyclerNewFinalized(m_recycler, Js::WebAssemblyModule, scriptContext, binaryBuffer, binaryBufferLength, scriptContext->GetLibrary()->GetWebAssemblyModuleType());

    m_sourceInfo->EnsureInitialized(0);
    m_sourceInfo->GetSrcInfo()->sourceContextInfo->EnsureInitialized();
}

Js::WebAssemblyModule*
WasmModuleGenerator::GenerateModule()
{
    m_module->GetReader()->InitializeReader();

    BVStatic<bSectLimit + 1> visitedSections;

    for (uint8 sectionCode = bSectCustom + 1; sectionCode < bSectLimit ; ++sectionCode)
    {
        SectionCode precedent = SectionInfo::All[sectionCode].precedent;
        if (GetReader()->ReadNextSection((SectionCode)sectionCode))
        {
            if (precedent != bSectLimit && !visitedSections.Test(precedent))
            {
                throw WasmCompilationException(_u("%s section missing before %s"),
                                               SectionInfo::All[precedent].name,
                                               SectionInfo::All[sectionCode].name);
            }
            visitedSections.Set(sectionCode);

            if (!GetReader()->ProcessCurrentSection())
            {
                throw WasmCompilationException(_u("Error while reading section %s"), SectionInfo::All[sectionCode].name);
            }
        }
    }

    uint32 funcCount = m_module->GetWasmFunctionCount();
    for (uint32 i = 0; i < funcCount; ++i)
    {
        GenerateFunctionHeader(i);
    }

#if DBG_DUMP
    if (PHASE_TRACE1(Js::WasmReaderPhase))
    {
        GetReader()->PrintOps();
    }
#endif
    // If we see a FunctionSignatures section we need to see a FunctionBodies section
    if (visitedSections.Test(bSectFunctionSignatures) && !visitedSections.Test(bSectFunctionBodies))
    {
        throw WasmCompilationException(_u("Missing required section: %s"), SectionInfo::All[bSectFunctionBodies].name);
    }

    return m_module;
}

WasmBinaryReader*
WasmModuleGenerator::GetReader() const
{
    return m_module->GetReader();
}

void
WasmModuleGenerator::GenerateFunctionHeader(uint32 index)
{
    TRACE_WASM_DECODER(_u("GenerateFunction Header %u \n"), index);
    WasmFunctionInfo* wasmInfo = m_module->GetWasmFunctionInfo(index);
    if (!wasmInfo)
    {
        throw WasmCompilationException(_u("Invalid function index %u"), index);
    }

    const char16* functionName = nullptr;
    int nameLength = 0;

    if (wasmInfo->GetNameLength() > 0)
    {
        functionName = wasmInfo->GetName();
        nameLength = wasmInfo->GetNameLength();
    }
    else
    {
        for (uint32 iExport = 0; iExport < m_module->GetExportCount(); ++iExport)
        {
            Wasm::WasmExport* wasmExport = m_module->GetExport(iExport);
            if (wasmExport  &&
                wasmExport->nameLength > 0 &&
                m_module->GetFunctionIndexType(wasmExport->index) == FunctionIndexTypes::Function &&
                wasmExport->index == wasmInfo->GetNumber())
            {
                nameLength = wasmExport->nameLength + 16;
                char16 * autoName = RecyclerNewArrayLeafZ(m_recycler, char16, nameLength);
                nameLength = swprintf_s(autoName, nameLength, _u("%s[%u]"), wasmExport->name, wasmInfo->GetNumber());
                functionName = autoName;
                break;
            }
        }
    }

    if (!functionName)
    {
        char16 * autoName = RecyclerNewArrayLeafZ(m_recycler, char16, 32);
        nameLength = swprintf_s(autoName, 32, _u("wasm-function[%u]"), wasmInfo->GetNumber());
        functionName = autoName;
    }

    Js::FunctionBody* body = Js::FunctionBody::NewFromRecycler(
        m_scriptContext,
        functionName,
        nameLength,
        0,
        0,
        m_sourceInfo,
        m_sourceInfo->GetSrcInfo()->sourceContextInfo->sourceContextId,
        wasmInfo->GetNumber(),
        nullptr,
        Js::FunctionInfo::Attributes::None,
        Js::FunctionBody::Flags_None
#ifdef PERF_COUNTERS
        , false /* is function from deferred deserialized proxy */
#endif
    );
    wasmInfo->SetBody(body);
    // TODO (michhol): numbering
    body->SetSourceInfo(0);
    body->AllocateAsmJsFunctionInfo();
    body->SetIsAsmJsFunction(true);
    body->SetIsAsmjsMode(true);
    body->SetIsWasmFunction(true);
    body->GetAsmJsFunctionInfo()->SetIsHeapBufferConst(false);

    WasmReaderInfo* readerInfo = RecyclerNew(m_recycler, WasmReaderInfo);
    readerInfo->m_funcInfo = wasmInfo;
    readerInfo->m_module = m_module;

    Js::AsmJsFunctionInfo* info = body->GetAsmJsFunctionInfo();
    info->SetWasmReaderInfo(readerInfo);
    info->SetWebAssemblyModule(m_module);

    if (wasmInfo->GetParamCount() >= Js::Constants::InvalidArgSlot)
    {
        Js::Throw::OutOfMemory();
    }
    Js::ArgSlot paramCount = (Js::ArgSlot)wasmInfo->GetParamCount();
    info->SetArgCount(paramCount);
    info->SetWasmSignature(wasmInfo->GetSignature());
    Js::ArgSlot argSizeLength = max(paramCount, 3ui16);
    info->SetArgSizeArrayLength(argSizeLength);
    uint* argSizeArray = RecyclerNewArrayLeafZ(m_recycler, uint, argSizeLength);
    info->SetArgsSizesArray(argSizeArray);

    if (paramCount > 0)
    {
        // +1 here because asm.js includes the this pointer
        body->SetInParamsCount(paramCount + 1);
        body->SetReportedInParamsCount(paramCount + 1);
        info->SetArgTypeArray(RecyclerNewArrayLeaf(m_recycler, Js::AsmJsVarType::Which, paramCount));
    }
    else
    {
        // overwrite default value in this case
        body->SetHasImplicitArgIns(false);
    }
    Js::ArgSlot paramSize = 0;
    for (Js::ArgSlot i = 0; i < paramCount; ++i)
    {
        WasmTypes::WasmType type = wasmInfo->GetParam(i);
        info->SetArgType(WasmToAsmJs::GetAsmJsVarType(type), i);
        uint16 size = 0;
        switch (type)
        {
        case WasmTypes::F32:
        case WasmTypes::I32:
            CompileAssert(sizeof(float) == sizeof(int32));
#ifdef _M_X64
            // on x64, we always alloc (at least) 8 bytes per arguments
            size = sizeof(void*);
#elif _M_IX86
            size = sizeof(int32);
#else
            Assert(UNREACHED);
#endif
            break;
        case WasmTypes::F64:
        case WasmTypes::I64:
            CompileAssert(sizeof(double) == sizeof(int64));
            size = sizeof(int64);
            break;
        default:
            Assume(UNREACHED);
        }
        argSizeArray[i] = size;
        // REVIEW: reduce number of checked adds
        paramSize = UInt16Math::Add(paramSize, size);
    }
    info->SetArgByteSize(paramSize);
    info->SetReturnType(WasmToAsmJs::GetAsmJsReturnType(wasmInfo->GetResultType()));
}

WAsmJs::RegisterSpace*
AllocateRegisterSpace(ArenaAllocator* alloc, WAsmJs::Types)
{
    return Anew(alloc, WAsmJs::RegisterSpace, 1);
}

void
WasmBytecodeGenerator::GenerateFunctionBytecode(Js::ScriptContext* scriptContext, WasmReaderInfo* readerinfo)
{
    WasmBytecodeGenerator generator(scriptContext, readerinfo);
    generator.GenerateFunction();
    if (!generator.GetReader()->IsCurrentFunctionCompleted())
    {
        throw WasmCompilationException(_u("Invalid function format"));
    }
}

WasmBytecodeGenerator::WasmBytecodeGenerator(Js::ScriptContext* scriptContext, WasmReaderInfo* readerInfo) :
    m_scriptContext(scriptContext),
    m_alloc(_u("WasmBytecodeGen"), scriptContext->GetThreadContext()->GetPageAllocator(), Js::Throw::OutOfMemory),
    m_evalStack(&m_alloc),
    mTypedRegisterAllocator(&m_alloc, AllocateRegisterSpace, 1 << WAsmJs::SIMD),
    m_blockInfos(&m_alloc),
    isUnreachable(false)
{
    m_writer.Create();
    m_funcInfo = readerInfo->m_funcInfo;
    m_module = readerInfo->m_module;
    // Init reader to current func offset
    GetReader()->SeekToFunctionBody(m_funcInfo->m_readerInfo);

    // Use binary size to estimate bytecode size
    const long astSize = readerInfo->m_funcInfo->m_readerInfo.size;
    m_writer.InitData(&m_alloc, astSize);
}

void
WasmBytecodeGenerator::GenerateFunction()
{
    TRACE_WASM_DECODER(_u("GenerateFunction %u \n"), m_funcInfo->GetNumber());
    if (PHASE_OFF(Js::WasmBytecodePhase, GetFunctionBody()))
    {
        throw WasmCompilationException(_u("Compilation skipped"));
    }
    Js::AutoProfilingPhase functionProfiler(m_scriptContext, Js::WasmFunctionBodyPhase);
    Unused(functionProfiler);

    m_maxArgOutDepth = 0;

    // TODO: fix these bools
    m_writer.Begin(GetFunctionBody(), &m_alloc, true, true, false);
    try
    {
        Js::ByteCodeLabel exitLabel = m_writer.DefineLabel();
        m_funcInfo->SetExitLabel(exitLabel);
        EnregisterLocals();

        EnterEvalStackScope();
        // The function's yield type is the return type
        GetReader()->m_currentNode.block.sig = m_funcInfo->GetResultType();
        EmitInfo lastInfo = EmitBlock();
        if (lastInfo.type != WasmTypes::Void || m_funcInfo->GetResultType() == WasmTypes::Void)
        {
            EmitReturnExpr(&lastInfo);
        }
        ExitEvalStackScope();
        m_writer.MarkAsmJsLabel(exitLabel);
        m_writer.EmptyAsm(Js::OpCodeAsmJs::Ret);
        m_writer.End();
        GetReader()->FunctionEnd();
    }
    catch (...)
    {
        GetReader()->FunctionEnd();
        m_writer.Reset();
        throw;
    }


#if DBG_DUMP
    if (PHASE_DUMP(Js::ByteCodePhase, GetFunctionBody()))
    {
        Js::AsmJsByteCodeDumper::Dump(GetFunctionBody(), &mTypedRegisterAllocator, nullptr);
    }
#endif

    Js::AsmJsFunctionInfo * info = GetFunctionBody()->GetAsmJsFunctionInfo();
    mTypedRegisterAllocator.CommitToFunctionBody(GetFunctionBody());
    mTypedRegisterAllocator.CommitToFunctionInfo(info, GetFunctionBody());

    GetFunctionBody()->CheckAndSetOutParamMaxDepth(m_maxArgOutDepth);
}

void
WasmBytecodeGenerator::EnregisterLocals()
{
    uint32 nLocals = m_funcInfo->GetLocalCount();
    m_locals = AnewArray(&m_alloc, WasmLocal, nLocals);

    m_funcInfo->GetBody()->SetFirstTmpReg(nLocals);
    for (uint i = 0; i < nLocals; ++i)
    {
        WasmTypes::WasmType type = m_funcInfo->GetLocal(i);
        WasmRegisterSpace * regSpace = GetRegisterSpace(type);
        if (regSpace == nullptr)
        {
            throw WasmCompilationException(_u("Unable to find local register space"));
        }
        m_locals[i] = WasmLocal(regSpace->AcquireRegister(), type);

        // Zero only the locals not corresponding to formal parameters.
        if (i >= m_funcInfo->GetParamCount()) {
            switch (type)
            {
            case WasmTypes::F32:
                m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, m_locals[i].location, 0.0f);
                break;
            case WasmTypes::F64:
                m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, m_locals[i].location, 0.0);
                break;
            case WasmTypes::I32:
                m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, m_locals[i].location, 0);
                break;
            case WasmTypes::I64:
                m_writer.AsmLong1Const1(Js::OpCodeAsmJs::Ld_LongConst, m_locals[i].location, 0);
                break;
            default:
                Assume(UNREACHED);
            }
        }
    }
}

void
WasmBytecodeGenerator::EmitExpr(WasmOp op)
{
    DebugPrintOp(op);
    switch (op)
    {
#define WASM_OPCODE(opname, opcode, sig, nyi) \
    case opcode: \
        if (nyi) throw WasmCompilationException(_u("Operator %s NYI"), _u(#opname)); break;
#include "WasmBinaryOpCodes.h"
    default:
        break;
    }

    EmitInfo info;

    if (IsUnreachable() && !IsBlockOpCode(op))
    {
        return;
    }

    switch (op)
    {
    case wbGetGlobal:
        info = EmitGetGlobal();
        break;
    case wbSetGlobal:
        info = EmitSetGlobal();
        break;
    case wbGetLocal:
        info = EmitGetLocal();
        break;
    case wbSetLocal:
        info = EmitSetLocal(false);
        break;
    case wbTeeLocal:
        info = EmitSetLocal(true);
        break;
    case wbReturn:
        EmitReturnExpr();
        break;
    case wbF32Const:
        info = EmitConst<WasmTypes::F32>();
        break;
    case wbF64Const:
        info = EmitConst<WasmTypes::F64>();
        break;
    case wbI32Const:
        info = EmitConst<WasmTypes::I32>();
        break;
    case wbI64Const:
        info = EmitConst<WasmTypes::I64>();
        break;
    case wbBlock:
        if (IsUnreachable())
        {
            EmitBlockCommon(nullptr);
            return;
        }
        info = EmitBlock();
        break;
    case wbLoop:
        if (IsUnreachable())
        {
            EmitBlockCommon(nullptr);
            return;
        }
        info = EmitLoop();
        break;
    case wbCall:
        info = EmitCall<wbCall>();
        break;
    case wbCallIndirect:
        info = EmitCall<wbCallIndirect>();
        break;
    case wbIf:
        if (IsUnreachable())
        {
            bool endOnElse = false;
            EmitBlockCommon(nullptr, &endOnElse);
            if (endOnElse)
            {
                EmitBlockCommon(nullptr);
            }
            return;
        }
        info = EmitIfElseExpr();
        break;
    case wbElse:
        throw WasmCompilationException(_u("Unexpected else opcode"));
    case wbEnd:
        throw WasmCompilationException(_u("Unexpected end opcode"));
    case wbBr:
        EmitBr();
        break;
    case wbBrIf:
        info = EmitBrIf();
        break;
    case wbSelect:
        info = EmitSelect();
        break;
    case wbBrTable:
        EmitBrTable();
        break;
    case wbDrop:
        info = EmitDrop();
        break;
    case wbNop:
        return;
    case wbCurrentMemory:
    {
        GetFunctionBody()->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);
        Js::RegSlot tempReg = GetRegisterSpace(WasmTypes::I32)->AcquireTmpRegister();
        info = EmitInfo(tempReg, WasmTypes::I32);
        m_writer.AsmReg1(Js::OpCodeAsmJs::CurrentMemory_Int, tempReg);
        break;
    }
    case wbGrowMemory:
    {
        info = EmitGrowMemory();
        break;
    }
    case wbUnreachable:
        m_writer.EmptyAsm(Js::OpCodeAsmJs::Unreachable_Void);
        SetUnreachableState(true);
        break;
#define WASM_MEMREAD_OPCODE(opname, opcode, sig, nyi, viewtype) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess(wb##opname, WasmOpCodeSignatures::sig, viewtype, false); \
        break;
#define WASM_MEMSTORE_OPCODE(opname, opcode, sig, nyi, viewtype) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig > 0);\
        info = EmitMemAccess(wb##opname, WasmOpCodeSignatures::sig, viewtype, true); \
        break;
#define WASM_BINARY_OPCODE(opname, opcode, sig, asmjsop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 3);\
        info = EmitBinExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#define WASM_UNARY__OPCODE(opname, opcode, sig, asmjsop, nyi) \
    case wb##opname: \
        Assert(WasmOpCodeSignatures::n##sig == 2);\
        info = EmitUnaryExpr(Js::OpCodeAsmJs::##asmjsop, WasmOpCodeSignatures::sig); \
        break;
#include "WasmBinaryOpCodes.h"
    default:
        throw WasmCompilationException(_u("Unknown expression's op 0x%X"), op);
    }

    if (info.type != WasmTypes::Void && !IsUnreachable())
    {
        PushEvalStack(info);
    }
}


EmitInfo
WasmBytecodeGenerator::EmitGetGlobal()
{
    uint globalIndex = GetReader()->m_currentNode.var.num;
    WasmGlobal* global = m_module->GetGlobal(globalIndex);

    WasmTypes::WasmType type = global->GetType();

    Js::RegSlot slot = m_module->GetOffsetForGlobal(global);

    CompileAssert(WasmTypes::I32 == 1);
    CompileAssert(WasmTypes::I64 == 2);
    CompileAssert(WasmTypes::F32 == 3);
    CompileAssert(WasmTypes::F64 == 4);
    static const Js::OpCodeAsmJs globalOpcodes[] = {
        Js::OpCodeAsmJs::LdSlot_Int,
        Js::OpCodeAsmJs::LdSlot_Long,
        Js::OpCodeAsmJs::LdSlot_Flt,
        Js::OpCodeAsmJs::LdSlot_Db
    };

    WasmRegisterSpace * regSpace = GetRegisterSpace(type);
    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();
    EmitInfo info(tmpReg, type);

    m_writer.AsmSlot(globalOpcodes[type - 1], tmpReg, WasmBytecodeGenerator::ModuleEnvRegister, slot);

    return info;
}

EmitInfo
WasmBytecodeGenerator::EmitSetGlobal()
{
    uint globalIndex = GetReader()->m_currentNode.var.num;
    WasmGlobal* global = m_module->GetGlobal(globalIndex);
    Js::RegSlot slot = m_module->GetOffsetForGlobal(global);

    WasmTypes::WasmType type = global->GetType();
    EmitInfo info = PopEvalStack();

    if (info.type != type)
    {
        throw WasmCompilationException(_u("TypeError in setglobal for %u"), globalIndex);
    }

    CompileAssert(WasmTypes::I32 == 1);
    CompileAssert(WasmTypes::I64 == 2);
    CompileAssert(WasmTypes::F32 == 3);
    CompileAssert(WasmTypes::F64 == 4);
    static const Js::OpCodeAsmJs globalOpcodes[] = {
        Js::OpCodeAsmJs::StSlot_Int,
        Js::OpCodeAsmJs::StSlot_Long,
        Js::OpCodeAsmJs::StSlot_Flt,
        Js::OpCodeAsmJs::StSlot_Db
    };

    m_writer.AsmSlot(globalOpcodes[type - 1], info.location, WasmBytecodeGenerator::ModuleEnvRegister, slot);
    ReleaseLocation(&info);

    return EmitInfo();
}



EmitInfo
WasmBytecodeGenerator::EmitGetLocal()
{
    if (m_funcInfo->GetLocalCount() < GetReader()->m_currentNode.var.num)
    {
        throw WasmCompilationException(_u("%u is not a valid local"), GetReader()->m_currentNode.var.num);
    }

    WasmLocal local = m_locals[GetReader()->m_currentNode.var.num];

    Js::OpCodeAsmJs op = GetLoadOp(local.type);
    WasmRegisterSpace * regSpace = GetRegisterSpace(local.type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();

    m_writer.AsmReg2(op, tmpReg, local.location);

    return EmitInfo(tmpReg, local.type);
}

EmitInfo
WasmBytecodeGenerator::EmitSetLocal(bool tee)
{
    uint localNum = GetReader()->m_currentNode.var.num;
    if (localNum >= m_funcInfo->GetLocalCount())
    {
        throw WasmCompilationException(_u("%u is not a valid local"), localNum);
    }

    WasmLocal local = m_locals[localNum];

    EmitInfo info = PopEvalStack();
    if (info.type != local.type)
    {
        throw WasmCompilationException(_u("TypeError in setlocal for %u"), localNum);
    }

    m_writer.AsmReg2(GetLoadOp(local.type), local.location, info.location);

    if (tee)
    {
        return info;
    }
    else
    {
        ReleaseLocation(&info);
        return EmitInfo();
    }
}

template<WasmTypes::WasmType type>
EmitInfo
WasmBytecodeGenerator::EmitConst()
{
    WasmRegisterSpace * regSpace = GetRegisterSpace(type);

    Js::RegSlot tmpReg = regSpace->AcquireTmpRegister();
    EmitInfo dst(tmpReg, type);
    EmitLoadConst(dst, GetReader()->m_currentNode.cnst);
    return dst;
}

void WasmBytecodeGenerator::EmitLoadConst(EmitInfo dst, WasmConstLitNode cnst)
{
    switch (dst.type)
    {
    case WasmTypes::F32:
        m_writer.AsmFloat1Const1(Js::OpCodeAsmJs::Ld_FltConst, dst.location, cnst.f32);
        break;
    case WasmTypes::F64:
        m_writer.AsmDouble1Const1(Js::OpCodeAsmJs::Ld_DbConst, dst.location, cnst.f64);
        break;
    case WasmTypes::I32:
        m_writer.AsmInt1Const1(Js::OpCodeAsmJs::Ld_IntConst, dst.location, cnst.i32);
        break;
    case WasmTypes::I64:
        m_writer.AsmLong1Const1(Js::OpCodeAsmJs::Ld_LongConst, dst.location, cnst.i64);
        break;
    default:
        throw WasmCompilationException(_u("Unknown type %u"), dst.type);
    }
}

void
WasmBytecodeGenerator::EmitBlockCommon(BlockInfo* blockInfo, bool* endOnElse /*= nullptr*/)
{
    WasmOp op;
    EnterEvalStackScope();
    if(endOnElse) *endOnElse = false;
    do {
        op = GetReader()->ReadExpr();
        if (op == wbEnd)
        {
            break;
        }
        if (endOnElse && op == wbElse)
        {
            *endOnElse = true;
            break;
        }
        EmitExpr(op);
    } while (true);
    DebugPrintOp(op);
    if (blockInfo && blockInfo->HasYield() && !IsUnreachable())
    {
        EmitInfo info = PopEvalStack();
        YieldToBlock(*blockInfo, info);
        ReleaseLocation(&info);
    }
    ExitEvalStackScope();
}

EmitInfo
WasmBytecodeGenerator::EmitBlock()
{
    Js::ByteCodeLabel blockLabel = m_writer.DefineLabel();

    BlockInfo blockInfo = PushLabel(blockLabel);
    EmitBlockCommon(&blockInfo);
    m_writer.MarkAsmJsLabel(blockLabel);
    EmitInfo yieldInfo = PopLabel(blockLabel);

    // Reset unreachable state
    SetUnreachableState(false);

    // block yields last value
    return yieldInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitLoop()
{
    Js::ByteCodeLabel loopTailLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel loopHeadLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel loopLandingPadLabel = m_writer.DefineLabel();

    uint loopId = m_writer.EnterLoop(loopHeadLabel);

    // Internally we create a block for loop to exit, but semantically, they don't exist so pop it
    BlockInfo implicitBlockInfo = PushLabel(loopTailLabel);
    m_blockInfos.Pop();

    // We don't want nested block to jump directly to the loop header
    // instead, jump to the landing pad and let it jump back to the loop header
    PushLabel(loopLandingPadLabel, false);
    EmitBlockCommon(&implicitBlockInfo);
    PopLabel(loopLandingPadLabel);

    // By default we don't loop, jump over the landing pad
    m_writer.AsmBr(loopTailLabel);
    m_writer.MarkAsmJsLabel(loopLandingPadLabel);
    m_writer.AsmBr(loopHeadLabel);

    // Put the implicit block back on the stack and yield the last expression to it
    m_blockInfos.Push(implicitBlockInfo);
    m_writer.MarkAsmJsLabel(loopTailLabel);
    // Pop the implicit block to resolve the yield correctly
    EmitInfo loopInfo = PopLabel(loopTailLabel);
    m_writer.ExitLoop(loopId);

    // Reset unreachable state
    SetUnreachableState(false);

    return loopInfo;
}

template<WasmOp wasmOp>
EmitInfo
WasmBytecodeGenerator::EmitCall()
{
    uint funcNum = Js::Constants::UninitializedValue;
    uint signatureId = Js::Constants::UninitializedValue;
    WasmSignature * calleeSignature = nullptr;
    EmitInfo indirectIndexInfo;
    const bool isImportCall = GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Import;
    Assert(isImportCall || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::Function || GetReader()->m_currentNode.call.funcType == FunctionIndexTypes::ImportThunk);
    switch (wasmOp)
    {
    case wbCall:
        funcNum = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetWasmFunctionInfo(funcNum)->GetSignature();
        break;
    case wbCallIndirect:
        indirectIndexInfo = PopEvalStack();
        signatureId = GetReader()->m_currentNode.call.num;
        calleeSignature = m_module->GetSignature(signatureId);
        ReleaseLocation(&indirectIndexInfo);
        break;
    default:
        Assume(UNREACHED);
    }

    // emit start call
    Js::ArgSlot argSize;
    Js::OpCodeAsmJs startCallOp;
    if (isImportCall)
    {
        argSize = (Js::ArgSlot)(calleeSignature->GetParamCount() * sizeof(Js::Var));
        startCallOp = Js::OpCodeAsmJs::StartCall;
    }
    else
    {
        startCallOp = Js::OpCodeAsmJs::I_StartCall;
        argSize = (Js::ArgSlot)calleeSignature->GetParamsSize();
    }
    // Add return value
    argSize += sizeof(Js::Var);

    if (argSize >= UINT16_MAX)
    {
        throw WasmCompilationException(_u("Argument size too big"));
    }

    m_writer.AsmStartCall(startCallOp, argSize);

    uint32 nArgs = calleeSignature->GetParamCount();
    //copy args into a list so they could be generated in the right order (FIFO)
    JsUtil::List<EmitInfo, ArenaAllocator> argsList(&m_alloc);
    for (uint32 i = 0; i < nArgs; i++)
    {
        argsList.Add(PopEvalStack());
    }
    Assert((uint32)argsList.Count() == nArgs);

    // Size of the this pointer (aka undefined)
    int32 argsBytesLeft = sizeof(Js::Var);
    for (uint32 i = 0; i < nArgs; ++i)
    {
        EmitInfo info = argsList.Item(nArgs - i - 1);
        if (calleeSignature->GetParam(i) != info.type)
        {
            throw WasmCompilationException(_u("Call argument does not match formal type"));
        }

        Js::OpCodeAsmJs argOp = Js::OpCodeAsmJs::Nop;
        switch (info.type)
        {
        case WasmTypes::F32:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Flt : Js::OpCodeAsmJs::I_ArgOut_Flt;
            break;
        case WasmTypes::F64:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Db : Js::OpCodeAsmJs::I_ArgOut_Db;
            break;
        case WasmTypes::I32:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Int : Js::OpCodeAsmJs::I_ArgOut_Int;
            break;
        case WasmTypes::I64:
            argOp = isImportCall ? Js::OpCodeAsmJs::ArgOut_Long : Js::OpCodeAsmJs::I_ArgOut_Long;
            break;
        default:
            throw WasmCompilationException(_u("Unknown argument type %u"), info.type);
        }
        //argSize

        if (argsBytesLeft < 0 || (argsBytesLeft % sizeof(Js::Var)) != 0)
        {
            throw WasmCompilationException(_u("Error while emitting call arguments"));
        }
        Js::RegSlot argLoc = argsBytesLeft / sizeof(Js::Var);
        argsBytesLeft += isImportCall ? sizeof(Js::Var) : calleeSignature->GetParamSize(i);

        m_writer.AsmReg2(argOp, argLoc, info.location);
    }

    //registers need to be released from higher ordinals to lower
    for (uint32 i = 0; i < nArgs; i++)
    {
        ReleaseLocation(&(argsList.Item(i)));
    }

    // emit call
    switch (wasmOp)
    {
    case wbCall:
    {
        uint32 offset = isImportCall ? m_module->GetImportFuncOffset() : m_module->GetFuncOffset();
        uint32 index = UInt32Math::Add(offset, funcNum);
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlot, 0, 1, index);
        break;
    }
    case wbCallIndirect:
        if (indirectIndexInfo.type != WasmTypes::I32)
        {
            throw WasmCompilationException(_u("Indirect call index must be int type"));
        }
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdSlotArr, 0, 1, m_module->GetTableEnvironmentOffset());
        m_writer.AsmSlot(Js::OpCodeAsmJs::LdArr_WasmFunc, 0, 0, indirectIndexInfo.location);
        m_writer.AsmReg1IntConst1(Js::OpCodeAsmJs::CheckSignature, 0, calleeSignature->GetSignatureId());
        break;
    default:
        Assume(UNREACHED);
    }

    // calculate number of RegSlots the arguments consume
    Js::ArgSlot args;
    Js::OpCodeAsmJs callOp = Js::OpCodeAsmJs::Nop;
    if (isImportCall)
    {
        args = (Js::ArgSlot)(calleeSignature->GetParamCount() + 1);
        callOp = Js::OpCodeAsmJs::Call;
    }
    else
    {
        args = (Js::ArgSlot)(::ceil((double)(argSize / sizeof(Js::Var))));
        callOp = Js::OpCodeAsmJs::I_Call;
    }

    m_writer.AsmCall(callOp, 0, 0, args, WasmToAsmJs::GetAsmJsReturnType(calleeSignature->GetResultType()));

    // emit result coercion
    EmitInfo retInfo;
    retInfo.type = calleeSignature->GetResultType();
    if (retInfo.type != WasmTypes::Void)
    {
        Js::OpCodeAsmJs convertOp = Js::OpCodeAsmJs::Nop;
        retInfo.location = GetRegisterSpace(retInfo.type)->AcquireTmpRegister();
        switch (retInfo.type)
        {
        case WasmTypes::F32:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTF : Js::OpCodeAsmJs::I_Conv_VTF;
            break;
        case WasmTypes::F64:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTD : Js::OpCodeAsmJs::I_Conv_VTD;
            break;
        case WasmTypes::I32:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTI : Js::OpCodeAsmJs::I_Conv_VTI;
            break;
        case WasmTypes::I64:
            convertOp = isImportCall ? Js::OpCodeAsmJs::Conv_VTL : Js::OpCodeAsmJs::Ld_Long;
            break;
        default:
            throw WasmCompilationException(_u("Unknown call return type %u"), retInfo.type);
        }
        m_writer.AsmReg2(convertOp, retInfo.location, 0);
    }

    // track stack requirements for out params

    // + 1 for return address
    uint maxDepthForLevel = args + 1;
    if (maxDepthForLevel > m_maxArgOutDepth)
    {
        m_maxArgOutDepth = maxDepthForLevel;
    }

    return retInfo;
}

EmitInfo
WasmBytecodeGenerator::EmitIfElseExpr()
{
    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel endLabel = m_writer.DefineLabel();

    EmitInfo checkExpr = PopEvalStack();
    ReleaseLocation(&checkExpr);
    if (checkExpr.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("If expression must have type i32"));
    }

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, checkExpr.location);

    BlockInfo blockInfo = PushLabel(endLabel);
    bool endOnElse = false;
    EmitBlockCommon(&blockInfo, &endOnElse);
    EnsureYield(blockInfo);

    m_writer.AsmBr(endLabel);
    m_writer.MarkAsmJsLabel(falseLabel);

    EmitInfo retInfo;
    EmitInfo falseExpr;
    if (endOnElse)
    {
        // In case the true block sets the unreachable state, we still have to emit the else block
        SetUnreachableState(false);
        if (blockInfo.yieldInfo)
        {
            blockInfo.yieldInfo->didYield = false;
        }
        EmitBlockCommon(&blockInfo);
        EnsureYield(blockInfo);
    }
    m_writer.MarkAsmJsLabel(endLabel);

    // Reset unreachable state
    SetUnreachableState(false);

    return PopLabel(endLabel);
}

void
WasmBytecodeGenerator::EmitBrTable()
{
    const uint numTargets = GetReader()->m_currentNode.brTable.numTargets;
    const UINT* targetTable = GetReader()->m_currentNode.brTable.targetTable;
    const UINT defaultEntry = GetReader()->m_currentNode.brTable.defaultTarget;

    // Compile scrutinee
    EmitInfo scrutineeInfo = PopEvalStack();

    if (scrutineeInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("br_table expression must be of type I32"));
    }

    m_writer.AsmReg2(Js::OpCodeAsmJs::BeginSwitch_Int, scrutineeInfo.location, scrutineeInfo.location);
    EmitInfo yieldInfo;
    if (ShouldYieldToBlock(defaultEntry))
    {
        yieldInfo = PopEvalStack();
    }
    // Compile cases
    for (uint i = 0; i < numTargets; i++)
    {
        uint target = targetTable[i];
        YieldToBlock(target, yieldInfo);
        Js::ByteCodeLabel targetLabel = GetLabel(target);
        m_writer.AsmBrReg1Const1(Js::OpCodeAsmJs::Case_IntConst, targetLabel, scrutineeInfo.location, i);
    }

    YieldToBlock(defaultEntry, yieldInfo);
    m_writer.AsmBr(GetLabel(defaultEntry), Js::OpCodeAsmJs::EndSwitch_Int);
    ReleaseLocation(&scrutineeInfo);
    ReleaseLocation(&yieldInfo);

    SetUnreachableState(true);
}


EmitInfo
WasmBytecodeGenerator::EmitGrowMemory()
{
    GetFunctionBody()->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);

    EmitInfo info = PopEvalStack();
    if (info.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("Invalid type for GrowMemory"));
    }

    m_writer.AsmReg2(Js::OpCodeAsmJs::GrowMemory, info.location, info.location);
    return info;
}

EmitInfo
WasmBytecodeGenerator::EmitDrop()
{
    EmitInfo info = PopEvalStack();
    ReleaseLocation(&info);
    return EmitInfo();
}

EmitInfo
WasmBytecodeGenerator::EmitBinExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType lhsType = signature[1];
    WasmTypes::WasmType rhsType = signature[2];

    EmitInfo rhs = PopEvalStack();
    EmitInfo lhs = PopEvalStack();

    if (lhsType != lhs.type)
    {
        throw WasmCompilationException(_u("Invalid type for LHS"));
    }
    if (rhsType != rhs.type)
    {
        throw WasmCompilationException(_u("Invalid type for RHS"));
    }

    GetRegisterSpace(rhsType)->ReleaseLocation(&rhs);
    GetRegisterSpace(lhsType)->ReleaseLocation(&lhs);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer.AsmReg3(op, resultReg, lhs.location, rhs.location);

    return EmitInfo(resultReg, resultType);
}

EmitInfo
WasmBytecodeGenerator::EmitUnaryExpr(Js::OpCodeAsmJs op, const WasmTypes::WasmType* signature)
{
    WasmTypes::WasmType resultType = signature[0];
    WasmTypes::WasmType inputType = signature[1];

    EmitInfo info = PopEvalStack();

    if (inputType != info.type)
    {
        throw WasmCompilationException(_u("Invalid input type"));
    }

    GetRegisterSpace(inputType)->ReleaseLocation(&info);

    Js::RegSlot resultReg = GetRegisterSpace(resultType)->AcquireTmpRegister();

    m_writer.AsmReg2(op, resultReg, info.location);

    return EmitInfo(resultReg, resultType);
}

EmitInfo
WasmBytecodeGenerator::EmitMemAccess(WasmOp wasmOp, const WasmTypes::WasmType* signature, Js::ArrayBufferView::ViewType viewType, bool isStore)
{
    WasmTypes::WasmType type = signature[0];
    const uint offset = GetReader()->m_currentNode.mem.offset;
    GetFunctionBody()->GetAsmJsFunctionInfo()->SetUsesHeapBuffer(true);

    EmitInfo rhsInfo;
    if (isStore)
    {
        rhsInfo = PopEvalStack();
    }
    EmitInfo exprInfo = PopEvalStack();

    if (exprInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("Index expression must be of type I32"));
    }

    Js::RegSlot addrReg = GetRegisterSpace(WasmTypes::I64)->AcquireTmpRegister();

    if (offset != 0)
    {
        Js::RegSlot offsetReg = GetRegisterSpace(WasmTypes::I64)->AcquireTmpRegister();
        m_writer.AsmLong1Const1(Js::OpCodeAsmJs::Ld_LongConst, offsetReg, offset);

        Js::RegSlot indexReg = GetRegisterSpace(WasmTypes::I64)->AcquireTmpRegister();
        m_writer.AsmReg2(Js::OpCodeAsmJs::Conv_UTL, indexReg, exprInfo.location);

        GetRegisterSpace(WasmTypes::I64)->ReleaseTmpRegister(indexReg);
        GetRegisterSpace(WasmTypes::I64)->ReleaseTmpRegister(offsetReg);

        m_writer.AsmReg3(Js::OpCodeAsmJs::Add_Long, addrReg, indexReg, offsetReg);
    }
    else
    {
        m_writer.AsmReg2(Js::OpCodeAsmJs::Conv_UTL, addrReg, exprInfo.location);
    }

    GetRegisterSpace(WasmTypes::I64)->ReleaseTmpRegister(addrReg);

    if (isStore) // Stores
    {
        if (rhsInfo.type != type)
        {
            throw WasmCompilationException(_u("Invalid type for store op"));
        }
        m_writer.AsmTypedArr(Js::OpCodeAsmJs::StArrWasm, rhsInfo.location, addrReg, viewType);
        ReleaseLocation(&rhsInfo);
        ReleaseLocation(&exprInfo);

        return EmitInfo();
    }

    ReleaseLocation(&exprInfo);
    Js::RegSlot resultReg = GetRegisterSpace(type)->AcquireTmpRegister();   
    m_writer.AsmTypedArr(Js::OpCodeAsmJs::LdArrWasm, resultReg, addrReg, viewType);

    EmitInfo yieldInfo;
    if (!isStore)
    {
        // Yield only on load
        yieldInfo = EmitInfo(resultReg, type);
    }
    return yieldInfo;
}

void
WasmBytecodeGenerator::EmitReturnExpr(EmitInfo* explicitRetInfo)
{
    if (m_funcInfo->GetResultType() == WasmTypes::Void)
    {
        // TODO (michhol): consider moving off explicit 0 for return reg
        m_writer.AsmReg1(Js::OpCodeAsmJs::LdUndef, 0);
    }
    else
    {
        EmitInfo retExprInfo = explicitRetInfo ? *explicitRetInfo : PopEvalStack();

        if (m_funcInfo->GetResultType() != retExprInfo.type)
        {
            throw WasmCompilationException(_u("Result type must match return type"));
        }

        Js::OpCodeAsmJs retOp = GetReturnOp(retExprInfo.type);
        m_writer.Conv(retOp, 0, retExprInfo.location);
        ReleaseLocation(&retExprInfo);
    }
    m_writer.AsmBr(m_funcInfo->GetExitLabel());

    SetUnreachableState(true);
}

EmitInfo
WasmBytecodeGenerator::EmitSelect()
{
    EmitInfo conditionInfo = PopEvalStack();
    if (conditionInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("select condition must have I32 type"));
    }

    Js::ByteCodeLabel falseLabel = m_writer.DefineLabel();
    Js::ByteCodeLabel doneLabel = m_writer.DefineLabel();

    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrFalse_Int, falseLabel, conditionInfo.location);
    ReleaseLocation(&conditionInfo);

    EmitInfo falseInfo = PopEvalStack();
    EmitInfo trueInfo = PopEvalStack();

    // Refresh the lifetime of the true location
    m_writer.AsmReg2(GetLoadOp(trueInfo.type), trueInfo.location, trueInfo.location);

    m_writer.AsmBr(doneLabel);
    m_writer.MarkAsmJsLabel(falseLabel);

    if (trueInfo.type != falseInfo.type)
    {
        throw WasmCompilationException(_u("select operands must both have same type"));
    }

    Js::OpCodeAsmJs op = GetLoadOp(trueInfo.type);

    m_writer.AsmReg2(op, trueInfo.location, falseInfo.location);
    ReleaseLocation(&falseInfo);

    m_writer.MarkAsmJsLabel(doneLabel);

    return trueInfo;
}

void
WasmBytecodeGenerator::EmitBr()
{
    UINT depth = GetReader()->m_currentNode.br.depth;

    if (ShouldYieldToBlock(depth))
    {
        EmitInfo info = PopEvalStack();
        YieldToBlock(depth, info);
        ReleaseLocation(&info);
    }

    Js::ByteCodeLabel target = GetLabel(depth);
    m_writer.AsmBr(target);

    SetUnreachableState(true);
}

EmitInfo
WasmBytecodeGenerator::EmitBrIf()
{
    UINT depth = GetReader()->m_currentNode.br.depth;

    EmitInfo conditionInfo = PopEvalStack();
    ReleaseLocation(&conditionInfo);

    if (conditionInfo.type != WasmTypes::I32)
    {
        throw WasmCompilationException(_u("br_if condition must have I32 type"));
    }

    EmitInfo info;
    if (ShouldYieldToBlock(depth))
    {
        info = PopEvalStack();
        YieldToBlock(depth, info);
    }

    Js::ByteCodeLabel target = GetLabel(depth);
    m_writer.AsmBrReg1(Js::OpCodeAsmJs::BrTrue_Int, target, conditionInfo.location);
    return info;
}

/* static */
Js::OpCodeAsmJs
WasmBytecodeGenerator::GetLoadOp(WasmTypes::WasmType wasmType)
{
    switch (wasmType)
    {
    case WasmTypes::F32:
        return Js::OpCodeAsmJs::Ld_Flt;
    case WasmTypes::F64:
        return Js::OpCodeAsmJs::Ld_Db;
    case WasmTypes::I32:
        return Js::OpCodeAsmJs::Ld_Int;
    case WasmTypes::I64:
        return Js::OpCodeAsmJs::Ld_Long;
    default:
        throw WasmCompilationException(_u("Unknown load operator %u"), wasmType);
    }
}

Js::OpCodeAsmJs
WasmBytecodeGenerator::GetReturnOp(WasmTypes::WasmType type)
{
    Js::OpCodeAsmJs retOp = Js::OpCodeAsmJs::Nop;
    switch (type)
    {
    case WasmTypes::F32:
        retOp = Js::OpCodeAsmJs::Return_Flt;
        break;
    case WasmTypes::F64:
        retOp = Js::OpCodeAsmJs::Return_Db;
        break;
    case WasmTypes::I32:
        retOp = Js::OpCodeAsmJs::Return_Int;
        break;
    case WasmTypes::I64:
        retOp = Js::OpCodeAsmJs::Return_Long;
        break;
    default:
        throw WasmCompilationException(_u("Unknown return type %u"), type);
    }
    return retOp;
}

void
WasmBytecodeGenerator::ReleaseLocation(EmitInfo * info)
{
    if (WasmTypes::IsLocalType(info->type))
    {
        GetRegisterSpace(info->type)->ReleaseLocation(info);
    }
}

EmitInfo
WasmBytecodeGenerator::EnsureYield(BlockInfo info)
{
    EmitInfo yieldEmitInfo;
    if (info.HasYield())
    {
        yieldEmitInfo = info.yieldInfo->info;
        if (!info.DidYield())
        {
            // Emit a load to the yield location to make sure we have a dest there
            // Most likely we can't reach this code so the value doesn't matter
            WasmConstLitNode cnst;
            cnst.i64 = 0;
            info.yieldInfo->didYield = true;
            EmitLoadConst(yieldEmitInfo, cnst);
        }
    }
    return yieldEmitInfo;
}

EmitInfo
WasmBytecodeGenerator::PopLabel(Js::ByteCodeLabel labelValidation)
{
    Assert(m_blockInfos.Count() > 0);
    BlockInfo info = m_blockInfos.Pop();
    UNREFERENCED_PARAMETER(labelValidation);
    Assert(info.label == labelValidation);
    return EnsureYield(info);
}

BlockInfo
WasmBytecodeGenerator::PushLabel(Js::ByteCodeLabel label, bool addBlockYieldInfo /*= true*/)
{
    BlockInfo info;
    info.label = label;
    if (addBlockYieldInfo)
    {
        WasmTypes::WasmType type = GetReader()->m_currentNode.block.sig;
        if (type != WasmTypes::Void)
        {
            info.yieldInfo = Anew(&m_alloc, BlockInfo::YieldInfo);
            info.yieldInfo->info = EmitInfo(GetRegisterSpace(type)->AcquireTmpRegister(), type);
            info.yieldInfo->didYield = false;
        }
    }
    m_blockInfos.Push(info);
    return info;
}

void
WasmBytecodeGenerator::YieldToBlock(uint relativeDepth, EmitInfo expr)
{
    BlockInfo blockInfo = GetBlockInfo(relativeDepth);
    YieldToBlock(blockInfo, expr);
}

void WasmBytecodeGenerator::YieldToBlock(BlockInfo blockInfo, EmitInfo expr)
{
    if (blockInfo.HasYield())
    {
        EmitInfo yieldInfo = blockInfo.yieldInfo->info;

        // Do not yield unrechable expressions
        if (IsUnreachable())
        {
            return;
        }

        if (yieldInfo.type != expr.type)
        {
            throw WasmCompilationException(_u("Invalid yield type"));
        }

        blockInfo.yieldInfo->didYield = true;
        m_writer.AsmReg2(GetLoadOp(expr.type), yieldInfo.location, expr.location);
    }
}

bool
WasmBytecodeGenerator::ShouldYieldToBlock(uint relativeDepth) const
{
    return GetBlockInfo(relativeDepth).HasYield();
}

Wasm::BlockInfo
WasmBytecodeGenerator::GetBlockInfo(uint relativeDepth) const
{
    if (relativeDepth >= (uint)m_blockInfos.Count())
    {
        throw WasmCompilationException(_u("Invalid branch target"));
    }
    return m_blockInfos.Peek(relativeDepth);
}

Js::ByteCodeLabel
WasmBytecodeGenerator::GetLabel(uint relativeDepth)
{
    return GetBlockInfo(relativeDepth).label;
}

bool WasmBytecodeGenerator::IsBlockOpCode(WasmOp op)
{
    return op == wbBlock || op == wbIf || op == wbLoop;
}

WasmRegisterSpace *
WasmBytecodeGenerator::GetRegisterSpace(WasmTypes::WasmType type)
{
    switch (type)
    {
    case WasmTypes::I32: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT32);
    case WasmTypes::I64: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::INT64);
    case WasmTypes::F32: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT32);
    case WasmTypes::F64: return mTypedRegisterAllocator.GetRegisterSpace(WAsmJs::FLOAT64);
    default:
        return nullptr;
    }
}

EmitInfo
WasmBytecodeGenerator::PopEvalStack()
{
    // The scope marker should at least be there
    Assert(!m_evalStack.Empty());
    EmitInfo info = m_evalStack.Pop();
    if (info.type == WasmTypes::Limit)
    {
        throw WasmCompilationException(_u("Reached end of stack"));
    }
    return info;
}

void
WasmBytecodeGenerator::PushEvalStack(EmitInfo info)
{
    Assert(!m_evalStack.Empty());
    m_evalStack.Push(info);
}

void
WasmBytecodeGenerator::EnterEvalStackScope()
{
    m_evalStack.Push(EmitInfo(WasmTypes::Limit));
}

void
WasmBytecodeGenerator::ExitEvalStackScope()
{
    Assert(!m_evalStack.Empty());
    EmitInfo info = m_evalStack.Pop();
    if (info.type != WasmTypes::Limit)
    {
        uint32 nElemLeftOnStack = 1;
        while(m_evalStack.Pop().type != WasmTypes::Limit) { ++nElemLeftOnStack; }
        throw WasmCompilationException(_u("Expected stack to be empty, but has %d"), nElemLeftOnStack);
    }
}

void WasmBytecodeGenerator::SetUnreachableState(bool isUnreachable)
{
    if (isUnreachable)
    {
        // Remove whatever is left on the stack when we become unreachable
        Assert(!m_evalStack.Empty());
        while(m_evalStack.Top().type != WasmTypes::Limit)
        {
            EmitInfo info = m_evalStack.Pop();
            ReleaseLocation(&info);
        }
    }

    this->isUnreachable = isUnreachable;
}

Wasm::WasmReaderBase*
WasmBytecodeGenerator::GetReader() const
{
    if (m_funcInfo->GetCustomReader())
    {
        return m_funcInfo->GetCustomReader();
    }
    return m_module->GetReader();
}

void
WasmCompilationException::FormatError(const char16* _msg, va_list arglist)
{
    char16 buf[2048];

    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, _msg, arglist);
    errorMsg = SysAllocString(buf);
}

WasmCompilationException::WasmCompilationException(const char16* _msg, ...)
{
    va_list arglist;
    va_start(arglist, _msg);
    FormatError(_msg, arglist);
}

WasmCompilationException::WasmCompilationException(const char16* _msg, va_list arglist)
{
    FormatError(_msg, arglist);
}

} // namespace Wasm

#endif // ENABLE_WASM
