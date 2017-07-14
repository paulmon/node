//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class ThreadContextInfo
{
public:
    ThreadContextInfo();

#if ENABLE_NATIVE_CODEGEN
    intptr_t GetNullFrameDisplayAddr() const;
    intptr_t GetStrictNullFrameDisplayAddr() const;

    intptr_t GetAbsDoubleCstAddr() const;
    intptr_t GetAbsFloatCstAddr() const;
    intptr_t GetSgnDoubleBitCst() const;
    intptr_t GetSgnFloatBitCst() const;
    intptr_t GetMaskNegFloatAddr() const;
    intptr_t GetMaskNegDoubleAddr() const;
    intptr_t GetDoubleOnePointZeroAddr() const;
    intptr_t GetDoublePointFiveAddr() const;
    intptr_t GetFloatPointFiveAddr() const;
    intptr_t GetDoubleNegPointFiveAddr() const;
    intptr_t GetDoubleNegOneAddr() const;
    intptr_t GetFloatNegPointFiveAddr() const;
    intptr_t GetDoubleTwoToFractionAddr() const;
    intptr_t GetFloatTwoToFractionAddr() const;
    intptr_t GetDoubleNegTwoToFractionAddr() const;
    intptr_t GetDoubleNaNAddr() const;
    intptr_t GetDoubleIntMaxPlusOneAddr() const;
    intptr_t GetDoubleUintMaxPlusOneAddr() const;
    intptr_t GetDoubleIntMinMinusOneAddr() const;
    intptr_t GetFloatNaNAddr() const;
    intptr_t GetFloatNegTwoToFractionAddr() const;
    intptr_t GetDoubleZeroAddr() const;
    intptr_t GetFloatZeroAddr() const;
    intptr_t GetDoubleIntMinAddr() const;
    intptr_t GetDoubleTwoTo31Addr() const;

    intptr_t GetUIntConvertConstAddr() const;
    intptr_t GetUInt64ConvertConstAddr() const;
    intptr_t GetUint8ClampedArraySetItemAddr() const;
    intptr_t GetConstructorCacheDefaultInstanceAddr() const;
    intptr_t GetJavascriptObjectNewInstanceAddr() const;
    intptr_t GetJavascriptArrayNewInstanceAddr() const;
    intptr_t GetNativeFloatArrayMissingItemAddr() const;
    intptr_t GetMantissaMaskAddr() const;
    intptr_t GetExponentMaskAddr() const;

#if _M_IX86 || _M_AMD64
    intptr_t GetX86AbsMaskF4Addr() const;
    intptr_t GetX86AbsMaskD2Addr() const;
    intptr_t GetX86NegMaskF4Addr() const;
    intptr_t GetX86NegMaskD2Addr() const;
    intptr_t GetX86AllNegOnesAddr() const;
    intptr_t GetX86AllNegOnesF4Addr() const;
    intptr_t GetX86AllZerosAddr() const;
    intptr_t GetX86AllOnesF4Addr() const;
    intptr_t GetX86LowBytesMaskAddr() const;
    intptr_t GetX86HighBytesMaskAddr() const;
    intptr_t GetX86DoubleWordSignBitsAddr() const;
    intptr_t GetX86WordSignBitsAddr() const;
    intptr_t GetX86ByteSignBitsAddr() const;
    intptr_t GetX86TwoPower32F4Addr() const;
    intptr_t GetX86TwoPower31F4Addr() const;
    intptr_t GetX86TwoPower31I4Addr() const;
    intptr_t GetX86NegTwoPower31F4Addr() const;
    intptr_t GetX86FourLanesMaskAddr(uint8 minorityLane) const;
#endif

    intptr_t GetStringReplaceNameAddr() const;
    intptr_t GetStringMatchNameAddr() const;
#endif

    void SetValidCallTargetForCFG(PVOID callTargetAddress, bool isSetValid = true);
    void ResetIsAllJITCodeInPreReservedRegion();
    bool IsAllJITCodeInPreReservedRegion() const;

    virtual HANDLE GetProcessHandle() const = 0;

    virtual bool IsThreadBound() const = 0;

    virtual size_t GetScriptStackLimit() const = 0;

    virtual intptr_t GetThreadStackLimitAddr() const = 0;

    virtual intptr_t GetDisableImplicitFlagsAddr() const = 0;
    virtual intptr_t GetImplicitCallFlagsAddr() const = 0;

#if ENABLE_NATIVE_CODEGEN
#if defined(ENABLE_SIMDJS) && (defined(_M_IX86) || defined(_M_X64))
    virtual intptr_t GetSimdTempAreaAddr(uint8 tempIndex) const = 0;
#endif
    virtual intptr_t GetBailOutRegisterSaveSpaceAddr() const = 0;
#endif

    virtual bool IsNumericProperty(Js::PropertyId propertyId) = 0;

    bool CanBeFalsy(Js::TypeId typeId) { return typeId == this->wellKnownHostTypeHTMLAllCollectionTypeId; }

    bool IsCFGEnabled();
    bool IsClosed();

#if defined(ENABLE_GLOBALIZATION) && defined(_CONTROL_FLOW_GUARD)
    Js::DelayLoadWinCoreMemory * GetWinCoreMemoryLibrary();
    Js::DelayLoadWinCoreProcessThreads * GetWinCoreProcessThreads();

    Js::DelayLoadWinCoreMemory m_delayLoadWinCoreMemoryLibrary;
    Js::DelayLoadWinCoreProcessThreads m_delayLoadWinCoreProcessThreads;
#endif

    UCrtC99MathApis* GetUCrtC99MathApis() { return &ucrtC99MathApis; }
protected:

    UCrtC99MathApis ucrtC99MathApis;

    Js::TypeId wellKnownHostTypeHTMLAllCollectionTypeId;

    bool m_isAllJITCodeInPreReservedRegion;
    bool m_isClosed;

};

// TODO: OOP JIT, is there any issue when crossing over 2^31/2^63?
template<typename T>
intptr_t SHIFT_ADDR(const ThreadContextInfo*const context, T* address)
{
    return SHIFT_ADDR(context, (intptr_t)address);
}

template<typename T>
intptr_t SHIFT_CRT_ADDR(const ThreadContextInfo*const context, T* address)
{
    return SHIFT_CRT_ADDR(context, (intptr_t)address);
}

intptr_t SHIFT_ADDR(const ThreadContextInfo*const context, intptr_t address);
intptr_t SHIFT_CRT_ADDR(const ThreadContextInfo*const context, intptr_t address);

