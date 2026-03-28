#include "pch.h"
#include "ThumbnailProvider.h"
#include "gdiplus.h"
#include "vpkpp/vpkpp.h"
#include <wincodec.h>
#include "VPKReader.hpp"
#include "vtfpp/vtfpp.h"

using namespace Gdiplus;

STDAPI CVPKThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject)
{
	*ppvObject = nullptr;

	CVPKThumbnailProvider* ptp = new CVPKThumbnailProvider();
	if (!ptp)
		return E_OUTOFMEMORY;

	const HRESULT hr = ptp->QueryInterface(riid, ppvObject);
	ptp->Release();
	return hr;
}

CVPKThumbnailProvider::~CVPKThumbnailProvider()
{
	DllRelease();
}

CVPKThumbnailProvider::CVPKThumbnailProvider()
{
	DllAddRef();
	m_cRef = 1;
}

STDMETHODIMP_(HRESULT __stdcall) CVPKThumbnailProvider::QueryInterface(REFIID riid, void** ppvObject)
{
	static const QITAB qit[] =
	{
		QITABENT(CVPKThumbnailProvider, IInitializeWithStream),
		QITABENT(CVPKThumbnailProvider, IThumbnailProvider),
		{ nullptr, 0 }
	};
	return QISearch(this, qit, riid, ppvObject);
}

STDMETHODIMP_(ULONG __stdcall) CVPKThumbnailProvider::AddRef()
{
	const LONG cRef = InterlockedIncrement(&m_cRef);
	return cRef;
}

STDMETHODIMP_(ULONG __stdcall) CVPKThumbnailProvider::Release()
{
	const LONG cRef = InterlockedDecrement(&m_cRef);
	if (cRef == 0)
		delete this;
	return cRef;
}

#define CHECK_RESULT(RESULT) if (!SUCCEEDED(RESULT)) {return S_FALSE;}

STDMETHODIMP_(HRESULT __stdcall) CVPKThumbnailProvider::Initialize(IStream* pstm, DWORD grfMode)
{
	if (grfMode & STGM_READWRITE)
		return STG_E_ACCESSDENIED;

	if (m_imageData.size() > 0)
		return ERROR_ALREADY_INITIALIZED;

	//STATSTG stat;
	//if (pstm->Stat(&stat, STATFLAG_DEFAULT) != S_OK)
	//	return S_FALSE;

	VPKHeader_v1 header;
	if (!SUCCEEDED(header.parse(pstm)))
		return S_FALSE;

	std::map<std::string, VPKEntry> entries;
	if (FAILED(ReadVPKEntries(entries, pstm, true)))
		return S_FALSE;
	
	if (auto it = entries.find("addonimage.jpg"); it != entries.end()) {
		ReadVPKEntryData(it->second, header.getEntryDataOffset(), pstm, m_imageData);
		m_imageDataIsVTF = false;
	}
	else if (auto it = entries.find("addonimage.png"); it != entries.end()) {
		ReadVPKEntryData(it->second, header.getEntryDataOffset(), pstm, m_imageData);
		m_imageDataIsVTF = false;
	}
	else {
		m_imageDataIsVTF = true;
		const char* textures[] = {
			"materials/vgui/select_bill.vtf",
			"materials/vgui/s_panel_lobby_coach.vtf",
			"materials/vgui/s_panel_lobby_mechanic.vtf",
			"materials/vgui/select_francis.vtf",
			"materials/vgui/select_louis.vtf",
			"materials/vgui/s_panel_lobby_gambler.vtf",
			"materials/vgui/s_panel_lobby_producer.vtf",
			"materials/vgui/select_zoey.vtf",

			"materials/vgui/s_panel_namvet.vtf",
			"materials/vgui/s_panel_coach.vtf",
			"materials/vgui/s_panel_mechanic.vtf",
			"materials/vgui/s_panel_biker.vtf",
			"materials/vgui/s_panel_manager.vtf",
			"materials/vgui/s_panel_gambler.vtf",
			"materials/vgui/s_panel_producer.vtf",
			"materials/vgui/s_panel_teenangst.vtf",
			"addonimage.vtf",
		};
		for (uint8_t i = 0; i < ARRAYSIZE(textures); i++) {
			if (auto it = entries.find(textures[i]); it != entries.end()) {
				ReadVPKEntryData(it->second, header.getEntryDataOffset(), pstm, m_imageData);
				break;
			}
		}

		if (m_imageData.empty()) {
			for (auto& it : entries) {
				if (it.first.starts_with("materials/vgui/")) {
					ReadVPKEntryData(it.second, header.getEntryDataOffset(), pstm, m_imageData);
					break;
				}
			}
		}
	}
	return S_OK;
	return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT __stdcall) CVPKThumbnailProvider::GetThumbnail(UINT cx, HBITMAP* phbmp, WTS_ALPHATYPE* pdwAlpha)
{
	*phbmp = nullptr;
	*pdwAlpha = WTSAT_RGB;
	ULONG_PTR token;
	GdiplusStartupInput input;

	if (GdiplusStartup(&token, &input, nullptr) == Ok)
	{
		const Gdiplus::Color bgColor(0x0);

		if (m_imageData.size() > 0 && !m_imageDataIsVTF) {
			IWICImagingFactory* pWICFactory = NULL;
			auto hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
			if (SUCCEEDED(hr)) {
				IWICStream* pWICStream = NULL;
				hr = pWICFactory->CreateStream(&pWICStream);
				if (SUCCEEDED(hr)) {
					hr = pWICStream->InitializeFromMemory(reinterpret_cast<BYTE*>(m_imageData.data()), m_imageData.size());
					if (SUCCEEDED(hr)) {
						auto* pBitmap = Gdiplus::Bitmap::FromStream(pWICStream);
						if (pBitmap != NULL) {
							pBitmap->GetHBITMAP(bgColor, phbmp);
							delete pBitmap;
						}
					}
					pWICStream->Release();
				}
				pWICFactory->Release();
			}
		}
		else if (m_imageDataIsVTF && m_imageData.size() > 0) {
			constexpr auto imageFormat = vtfpp::ImageFormat::BGR888; //TODO: alpha
			auto vtf = vtfpp::VTF(m_imageData);
			const auto width = vtf.getWidth(), height = vtf.getHeight();

			std::vector<std::byte> imageData = vtf.getImageDataAs(imageFormat);
			auto bitmap = Gdiplus::Bitmap(width, height, width * 3, PixelFormat24bppRGB, reinterpret_cast<BYTE*>(imageData.data()));
			bitmap.GetHBITMAP(bgColor, phbmp);
		}

#if 0
		if (*phbmp == nullptr) {
#define CB_COLOR1 0xfe, 0x9d, 0xda, 0xff //0xda9dfe
#define CB_COLOR2 0xff, 0xff, 0xff, 0xff
			const auto checkerBoardSize = 8;
//#define CHECKER_BOARD_COLOR1 0xff, 0x00, 0xff, 0xff
//#define CHECKER_BOARD_COLOR2 0x00, 0x00, 0x00, 0xff
			const BYTE pCheckerboardBitmapData[16] = {
				CB_COLOR1, CB_COLOR2,
				CB_COLOR2, CB_COLOR1,
			};
			//Read Only
			Gdiplus::Bitmap* pCheckerboard = new Gdiplus::Bitmap(2, 2, 8, PixelFormat32bppARGB, (BYTE*)pCheckerboardBitmapData);

			Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(128, 128, PixelFormat24bppRGB);
			if (pBitmap && pCheckerboard) {
				Gdiplus::Graphics graphics(pBitmap);
				graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);

				Gdiplus::ImageAttributes imgAtt;
				imgAtt.SetWrapMode(WrapModeTile);
				graphics.DrawImage(pCheckerboard, RectF(0, 0, 128, 128), RectF(-0.5, -0.5, checkerBoardSize, checkerBoardSize), UnitPixel, &imgAtt);

				pBitmap->GetHBITMAP(bgColor, phbmp);
				delete pBitmap;
				delete pCheckerboard;
			}
		}
#endif
	}

	GdiplusShutdown(token);
	if (*phbmp != nullptr)
		return NOERROR;

	return E_NOTIMPL;
}


CVPKThumbnailProviderFactory::CVPKThumbnailProviderFactory()
{
	m_cRef = 1;
	DllAddRef();
}

CVPKThumbnailProviderFactory::~CVPKThumbnailProviderFactory()
{
	DllRelease();
}

STDMETHODIMP CVPKThumbnailProviderFactory::QueryInterface(REFIID riid, void** ppvObject)
{
	static const QITAB qit[] =
	{
		QITABENT(CVPKThumbnailProviderFactory, IClassFactory),
		{ nullptr, 0 }
	};
	return QISearch(this, qit, riid, ppvObject);
}

STDMETHODIMP_(ULONG) CVPKThumbnailProviderFactory::AddRef()
{
	const LONG cRef = InterlockedIncrement(&m_cRef);
	return cRef;
}

STDMETHODIMP_(ULONG) CVPKThumbnailProviderFactory::Release()
{
	const LONG cRef = InterlockedDecrement(&m_cRef);
	if (0 == cRef)
		delete this;
	return cRef;
}

STDMETHODIMP CVPKThumbnailProviderFactory::CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppvObject)
{
	if (nullptr != punkOuter)
		return CLASS_E_NOAGGREGATION;

	return CVPKThumbnailProvider_CreateInstance(riid, ppvObject);
}

STDMETHODIMP CVPKThumbnailProviderFactory::LockServer(BOOL fLock)
{
	return E_NOTIMPL;
}

