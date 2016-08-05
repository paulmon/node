#include <windows.storage.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

using namespace Microsoft::WRL;
using namespace ABI::Windows::Storage;
using namespace Microsoft::WRL::Wrappers;
using namespace Windows::Foundation;
using namespace ABI::Windows::ApplicationModel;

// gets the path to local storage of a UWP application.
extern "C" HRESULT GetLocalStoragePath(WCHAR* path) {
  HRESULT hr;
  ComPtr<IApplicationDataStatics> applicationDataStatics;

#ifdef _DEBUG
  RoInitialize(RO_INIT_MULTITHREADED);
#endif

  hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), 
	                         &applicationDataStatics);
  if (FAILED(hr)) {
      return hr;
  }

  ComPtr<IApplicationData> applicationData;
  hr = applicationDataStatics->get_Current(&applicationData);
  if (FAILED(hr)) {
      return hr;
  }

  ComPtr<IStorageFolder> storageFolder;
  hr = applicationData->get_LocalFolder(&storageFolder);
  if (FAILED(hr)) {
      return hr;
  }
  
  ComPtr<IStorageItem> storageItem;
  hr = storageFolder.As(&storageItem);
  if (FAILED(hr)) {
      return hr;
  }

  HSTRING folderName;
  hr = storageItem->get_Path(&folderName);
  if (FAILED(hr)) {
      return hr;
  }

  UINT32 length;
  PCWSTR value = WindowsGetStringRawBuffer(folderName, &length);
  wcsncpy_s(path, MAX_PATH, value, _TRUNCATE);
  WindowsDeleteString(folderName);
  return S_OK;
}

// gets the path to the installed location of a UWP application.
extern "C" HRESULT GetInstalledLocationPath(WCHAR* path) {
	HRESULT hr;
	ComPtr<IActivationFactory> activationFactory;

#ifdef _DEBUG
	RoInitialize(RO_INIT_MULTITHREADED);
#endif

	hr = GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Package).Get(),
		&activationFactory);
	if (FAILED(hr)) {
		return hr;
	}

	ComPtr<IPackageStatics> pPackage;
	hr = activationFactory.As(&pPackage);
	if (FAILED(hr)) {
		return hr;
	}

	ComPtr<IPackage> current;
	hr = pPackage->get_Current(&current);
	if (FAILED(hr)) {
		return hr;
	}

	ComPtr<IStorageFolder> storageFolder;
	hr = current->get_InstalledLocation(&storageFolder);
	if (FAILED(hr)) {
		return hr;
	}

	ComPtr<IStorageItem> storageItem;
	hr = storageFolder.As(&storageItem);
	if (FAILED(hr)) {
		return hr;
	}

	HSTRING folderName;
	hr = storageItem->get_Path(&folderName);
	if (FAILED(hr)) {
		return hr;
	}

	UINT32 length;
	PCWSTR value = WindowsGetStringRawBuffer(folderName, &length);
	wcsncpy_s(path, MAX_PATH, value, _TRUNCATE);
	WindowsDeleteString(folderName);
	return S_OK;
}
