#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN

#include <Windows.h>
#include <comdef.h>

#pragma warning(disable : 26812)
#include "BMDSwitcherAPI_h.h"

int main()
{
    HRESULT hr = S_OK;
    IBMDSwitcherDiscovery* pSwitcherDiscovery = NULL;

    if (FAILED(hr = ::CoInitializeEx(NULL, 0))) {
        fprintf(stderr, "CoInitializeEx failed: 0x%08X\n", hr);
        goto exit;
    }

    if (FAILED(hr = ::CoCreateInstance(CLSID_CBMDSwitcherDiscovery, NULL, CLSCTX_ALL, IID_IBMDSwitcherDiscovery, (LPVOID*)&pSwitcherDiscovery)))
    {
        fprintf(stderr, "CoCreateInstance on CLSID_CBMDSwitcherDiscovery::IID_IBMDSwitcherDiscovery failed: 0x%08X\n", hr);
        goto exit;
    }

    {
        BSTR bstrIpAddress = ::SysAllocString(L"");
        IBMDSwitcher* pSwitcher = NULL;
        BMDSwitcherConnectToFailure eSwitcherConnectFailure;
        if (FAILED(hr = pSwitcherDiscovery->ConnectTo(bstrIpAddress, &pSwitcher, &eSwitcherConnectFailure)))
        {
            fprintf(stderr, "BMDSwitcherDiscovery::ConnectTo failed: 0x%08X\n", hr);
            goto exit;
        }
        ::SysFreeString(bstrIpAddress);
        pSwitcher->AddRef();

        IBMDSwitcherMultiViewIterator* pSwitcherMultiViewIterator = NULL;
        if (FAILED(hr = pSwitcher->CreateIterator(__uuidof(IBMDSwitcherMultiViewIterator), (LPVOID*)&pSwitcherMultiViewIterator)))
        {
            fprintf(stderr, "BMDSwitcher::CreateIterator failed: 0x%08X\n", hr);
            goto exit;
        }
        pSwitcherMultiViewIterator->AddRef();

        IBMDSwitcherMultiView* pMultiView = NULL;
        pSwitcherMultiViewIterator->Next(&pMultiView);
        while (NULL != pMultiView)
        {
            pMultiView->AddRef();

            /// For each multiview window that supports such, flips the VU Meter enabledness
            /// BUG: Only flips the first (program window, window #1) unless a pause is introduced between calls to
            /// SetVuMeterEnabled.  Even 1ms was enough.  However, It took almost a full second for GetVuMeterEnabled to reflect
            /// the new value.
            unsigned int uiWindowCount = 0;
            pMultiView->GetWindowCount(&uiWindowCount);
            for (unsigned int i = 0; i < uiWindowCount; i++)
            {
                BOOL supportsVuMeters = FALSE;
                BOOL enabled = FALSE;
                BOOL newEnabled = FALSE;

                pMultiView->CurrentInputSupportsVuMeter(i, &supportsVuMeters);

                if (TRUE == supportsVuMeters)
                {
                    pMultiView->GetVuMeterEnabled(i, &enabled);
                    newEnabled = 1 - enabled;
                    pMultiView->SetVuMeterEnabled(i, newEnabled);

                    fprintf(stdout, "Setting VuMeterEnabled for window %i to %i\n", i, 1 - enabled);

                    // "Fixes" the problem
                    ::Sleep(1000);

                    // BUG - does not return the new value
                    pMultiView->GetVuMeterEnabled(i, &enabled);
                    if (enabled != newEnabled)
                    {
                        fprintf(stderr, " Failed to flip VU meter state for window %i to %i\n", i, newEnabled);
                    }
                    else
                    {
                        fprintf(stdout, " Flipped VU meter state for window %i to %i\n", i, newEnabled);
                    }
                }
            }
            pMultiView->Release();
            pMultiView = NULL;

            if (S_FALSE == pSwitcherMultiViewIterator->Next(&pMultiView)) break;
        }

        pSwitcher->Release();
    }
exit:
    if (NULL != pSwitcherDiscovery)
    {
        pSwitcherDiscovery->Release();
    }

    ::CoUninitialize();
    return 0;
}
