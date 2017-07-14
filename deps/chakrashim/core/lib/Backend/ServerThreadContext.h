//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ServerThreadContext : public ThreadContextInfo
{
#if ENABLE_OOP_NATIVE_CODEGEN
public:
    typedef BVSparseNode<JitArenaAllocator> BVSparseNode;

    ServerThreadContext(ThreadContextDataIDL * data);
    ~ServerThreadContext();

    virtual HANDLE GetProcessHandle() const override;

    virtual bool IsThreadBound() const override;

    virtual size_t GetScriptStackLimit() const override;

    virtual intptr_t GetThreadStackLimitAddr() const override;

#if defined(ENABLE_SIMDJS) && (defined(_M_IX86) || defined(_M_X64))
    virtual intptr_t GetSimdTempAreaAddr(uint8 tempIndex) const override;
#endif

    virtual intptr_t GetDisableImplicitFlagsAddr() const override;
    virtual intptr_t GetImplicitCallFlagsAddr() const override;
    virtual intptr_t GetBailOutRegisterSaveSpaceAddr() const override;

    PreReservedSectionAllocWrapper * GetPreReservedSectionAllocator();

    virtual bool IsNumericProperty(Js::PropertyId propId) override;

    ptrdiff_t GetChakraBaseAddressDifference() const;
    ptrdiff_t GetCRTBaseAddressDifference() const;

    OOPCodeGenAllocators * GetCodeGenAllocators();
    CustomHeap::OOPCodePageAllocators * GetThunkPageAllocators();
    CustomHeap::OOPCodePageAllocators  * GetCodePageAllocators();
    SectionAllocWrapper * GetSectionAllocator();
    void UpdateNumericPropertyBV(BVSparseNode * newProps);
    void SetWellKnownHostTypeId(Js::TypeId typeId) { this->wellKnownHostTypeHTMLAllCollectionTypeId = typeId; }
    void AddRef();
    void Release();
    void Close();
    PageAllocator * GetForegroundPageAllocator();
#ifdef STACK_BACK_TRACE
    DWORD GetRuntimePid() { return m_pid; }
#endif

    intptr_t GetRuntimeChakraBaseAddress() const;
    intptr_t GetRuntimeCRTBaseAddress() const;
    intptr_t GetJITCRTBaseAddress() const;

private:

    class AutoCloseHandle
    {
    public:
        AutoCloseHandle(HANDLE handle) : handle(handle) { Assert(handle != GetCurrentProcess()); }
        ~AutoCloseHandle() { CloseHandle(this->handle); }
    private:
        HANDLE handle;
    };

    AutoCloseHandle m_autoProcessHandle;

    BVSparse<HeapAllocator> * m_numericPropertyBV;

    PreReservedSectionAllocWrapper m_preReservedSectionAllocator;
    SectionAllocWrapper m_sectionAllocator;
    CustomHeap::OOPCodePageAllocators m_thunkPageAllocators;
    CustomHeap::OOPCodePageAllocators  m_codePageAllocators;
    OOPCodeGenAllocators m_codeGenAlloc;
    // only allocate with this from foreground calls (never from CodeGen calls)
    PageAllocator m_pageAlloc;

    ThreadContextDataIDL m_threadContextData;

    DWORD m_pid; //save client process id for easier diagnose

    CriticalSection m_cs;
    uint m_refCount;
#endif
};
