#pragma once

//#define OP_InitializeWithFile 1
 
class CVPKThumbnailProviderFactory : public IClassFactory
{
private:
	LONG m_cRef;

	~CVPKThumbnailProviderFactory();

public:
	CVPKThumbnailProviderFactory();

	//  IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, void**);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	//  IClassFactory methods
	STDMETHOD(CreateInstance)(IUnknown*, REFIID, void**);
	STDMETHOD(LockServer)(BOOL);
};

class CVPKThumbnailProvider : public IThumbnailProvider, IInitializeWithStream

{
	~CVPKThumbnailProvider();

public:
	CVPKThumbnailProvider();

	//  IUnknown methods
	STDMETHOD(QueryInterface)(REFIID, void**) override;
	STDMETHOD_(ULONG, AddRef)() override;
	STDMETHOD_(ULONG, Release)() override;

	//  IInitializeWithSteam methods
	STDMETHOD(Initialize)(IStream*, DWORD) override;

	//  IThumbnailProvider methods
	STDMETHOD(GetThumbnail)(UINT, HBITMAP*, WTS_ALPHATYPE*) override;

private:
	volatile LONG m_cRef;

	std::vector<std::byte> m_imageData;
	bool m_imageDataIsVTF = false;
};

STDAPI CVPKThumbnailProvider_CreateInstance(REFIID riid, void** ppvObject);