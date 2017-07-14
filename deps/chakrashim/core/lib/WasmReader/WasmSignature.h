//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
class WasmSignature
{
public:
    WasmSignature();

    void AllocateParams(uint32 count, Recycler * recycler);
    void SetParam(WasmTypes::WasmType type, uint32 index);
    void SetResultType(WasmTypes::WasmType type);
    void SetSignatureId(uint32 id);

    Local GetParam(uint index) const;
    WasmTypes::WasmType GetResultType() const;
    uint32 GetParamCount() const;
    uint32 GetParamSize(uint index) const;
    uint32 GetParamsSize() const;
    void FinalizeSignature();
    uint32 GetSignatureId() const;
    size_t GetShortSig() const;

    bool IsEquivalent(const WasmSignature* sig) const;
    static WasmSignature * FromIDL(WasmSignatureIDL* sig);

    static uint GetOffsetOfShortSig() { return offsetof(WasmSignature, m_shortSig); }

    void Dump();
private:
    Field(WasmTypes::WasmType) m_resultType;
    Field(uint32) m_id;
    Field(uint32) m_paramSize;
    Field(uint32) m_paramsCount;
    Field(size_t) m_shortSig;
    Field(Local*) m_params;
};

} // namespace Wasm
