#include <image_decoder.h>

#include <combaseapi.h>
#include <wincodec.h>

static IWICImagingFactory* factory = NULL;

void image_decoder_init(void)
{
    HRESULT hr = CoCreateInstance(
        &CLSID_WICImagingFactory1,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory,
        &factory
    );

    if (FAILED(hr)) {
        MessageBox(NULL,
            L"Nie uda\u0142o si\u0119 zainicjalizowa\u0107 komponentu WIC (windowscodecs.dll).",
            NULL, MB_ICONWARNING);
    }
}

void image_decoder_destroy(void)
{
    if (factory) {
        factory->lpVtbl->Release(factory);
    }
}

HBITMAP image_decoder_file_to_hbitmap(LPCWSTR path)
{
    if (!factory) {
        return NULL;
    }

    IWICBitmapDecoder* decoder = NULL;
    IWICBitmapFrameDecode* frame = NULL;
    BYTE* pixels = NULL;
    HRESULT hr;

    hr = factory->lpVtbl->CreateDecoderFromFilename(factory, path, NULL,
        GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);
    if (FAILED(hr)) {
        goto error;
    }

    hr = decoder->lpVtbl->GetFrame(decoder, 0, &frame);
    if (FAILED(hr)) {
        goto error;
    }

    UINT width, height;
    hr = frame->lpVtbl->GetSize(frame, &width, &height);
    if (FAILED(hr)) {
        goto error;
    }

    WICPixelFormatGUID pixel_format;
    hr = frame->lpVtbl->GetPixelFormat(frame, &pixel_format);
    if (FAILED(hr)) {
        goto error;
    }

    if (!IsEqualGUID(&pixel_format, &GUID_WICPixelFormat32bppPBGRA)) {
        IWICBitmapSource* converted = NULL;
        hr = WICConvertBitmapSource(&GUID_WICPixelFormat32bppPBGRA, frame, &converted);

        if (SUCCEEDED(hr)) {
            frame->lpVtbl->Release(frame);
            frame = NULL;
            frame = converted;
        }
    }

    if (FAILED(hr)) {
        goto error;
    }

    pixels = malloc(width * height * 4);
    if (!pixels) {
        goto error;
    }

    hr = frame->lpVtbl->CopyPixels(frame, NULL, width * 4, width * height * 4, pixels);
    if (FAILED(hr)) {
        goto error;
    }

    HBITMAP bitmap = CreateBitmap(width, height, 1, 32, pixels);
    if (!bitmap) {
        goto error;
    }

    if (pixels) {
        free(pixels);
        pixels = NULL;
    }

    if (frame) {
        frame->lpVtbl->Release(frame);
        frame = NULL;
    }

    if (decoder) {
        decoder->lpVtbl->Release(decoder);
        decoder = NULL;
    }

    return bitmap;

error:

    if (pixels) {
        free(pixels);
        pixels = NULL;
    }

    if (frame) {
        frame->lpVtbl->Release(frame);
        frame = NULL;
    }

    if (decoder) {
        decoder->lpVtbl->Release(decoder);
        decoder = NULL;
    }

    return NULL;
}
