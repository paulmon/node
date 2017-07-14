//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Wasm
{

    enum SectionFlag
    {
        fSectNone,
        fSectIgnore,
    };

#define WASM_SECTION(name, id, flag, precendent) bSect ## name,
    enum SectionCode : uint8
    {
#include "WasmSections.h"
        bSectLimit,
        bsectLastKnownSection = bSectDataSegments
    };

    struct SectionInfo
    {
        SectionInfo(SectionFlag, SectionCode, const char16*, const char*, const uint32);
        SectionFlag flag;
        SectionCode precedent;
        const char16* name;
        const char* id;
        const uint32 nameLength;
        static SectionInfo All[bSectLimit];
    };
}
#endif
