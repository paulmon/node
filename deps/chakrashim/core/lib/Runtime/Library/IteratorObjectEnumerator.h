//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class IteratorObjectEnumerator sealed : public JavascriptEnumerator
    {
        typedef SListBase<Js::PropertyRecord const *, Memory::Recycler> PropertyRecordList;
    public:
        static IteratorObjectEnumerator * Create(ScriptContext* scriptContext, Var iteratorObject);
        virtual Var MoveAndGetNext(PropertyId& propertyId, PropertyAttributes* attributes = nullptr);
        virtual void Reset() override;
    protected:
        IteratorObjectEnumerator(ScriptContext* scriptContext, Var iteratorObject);
        DEFINE_VTABLE_CTOR(IteratorObjectEnumerator, JavascriptEnumerator);

    private:
        void EnsureIterator();
        Field(RecyclableObject*) iteratorObject;
        Field(Var) value;
        Field(BOOL) done;
        Field(PropertyRecordList) propertyRecords;
    };

}
