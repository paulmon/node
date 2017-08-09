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

// stubs.cc
// Included in addon.gypi to enable building an Addon built with onecore.lib 
// (workaround for linker errors).

#include <Windows.h>
#include <assert.h>

BOOL WINAPI GetProcessAffinityMask(
	_In_  HANDLE     hProcess,
	_Out_ PDWORD_PTR lpProcessAffinityMask,
	_Out_ PDWORD_PTR lpSystemAffinityMask
	)
{
	assert(FALSE);
	return FALSE;
}

DWORD_PTR WINAPI SetThreadAffinityMask(
	_In_ HANDLE    hThread,
	_In_ DWORD_PTR dwThreadAffinityMask
	)
{
	assert(FALSE);
	return 0;
}

BOOL WINAPI RegisterWaitForSingleObject(
	_Out_    PHANDLE             phNewWaitObject,
	_In_     HANDLE              hObject,
	_In_     WAITORTIMERCALLBACK Callback,
	_In_opt_ PVOID               Context,
	_In_     ULONG               dwMilliseconds,
	_In_     ULONG               dwFlags
	)
{
	assert(FALSE);
	return FALSE;
}

BOOL WINAPI UnregisterWait(
	_In_ HANDLE WaitHandle
	)
{
	assert(FALSE);
	return FALSE;
}

BOOL WINAPI SetProcessAffinityMask(
	_In_ HANDLE    hProcess,
	_In_ DWORD_PTR dwProcessAffinityMask
	)
{
	assert(FALSE);
	return false;
}