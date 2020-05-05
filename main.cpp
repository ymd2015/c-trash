/**
@file
@brief ファイルやフォルダをWindowsのゴミ箱へ捨てるコマンド
*/

// shlobj.h のインクルード前に STRICT_TYPED_ITEMIDS を定義することで、
// ITEMIDLIST の種類のミスマッチをエラー扱いする。
#define STRICT_TYPED_ITEMIDS
#include <Shlobj.h>
#include <Shobjidl.h>

// ログ出力
#include <cstdio>
#define LOG_XXX(fmt, ...)  fprintf(stderr, "%-15.15s|%-15.15s|%-5d|" fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_ERR(fmt, ...)  LOG_XXX("ERR|" fmt, ##__VA_ARGS__)
#define LOG_TRC(fmt, ...)  LOG_XXX("TRC|" fmt, ##__VA_ARGS__)

/**
@brief 相対パスから絶対パスを作成する。
*/
char* TR_GetFullPathName(const char* path)
{
	DWORD required_size = GetFullPathNameA(path, 0, NULL, NULL);
	char* buf = (char*)calloc(required_size, sizeof(char));
	if (buf == NULL) {
		LOG_ERR("");
		return NULL;
	}
	DWORD ret = GetFullPathNameA(path, required_size, buf, NULL);
	if (ret == 0 || required_size <= ret) {
		LOG_ERR("");
		free(buf);
		return NULL;
	}
	return buf;
}

/**
@brief CreateFromPathA()のラップ関数

@return NULL:失敗, NULL以外:成功
*/
PIDLIST_ABSOLUTE TR_ILCreateFromPath(const char* path)
{
	try {
		PIDLIST_ABSOLUTE pIdList = ILCreateFromPathA(path);
		return pIdList;
	} catch (...) {
		return nullptr;
	}
}

/**
@brief ファイルやフォルダをWindowsのゴミ箱へ捨てる。

@param path 絶対パス

@return 0:成功, 0以外:失敗
*/
int TR_ToTrash(const char* path)
{
	int rtn = 1;

	PIDLIST_ABSOLUTE pIdList = TR_ILCreateFromPath(path);
	if (pIdList == nullptr) {
		LOG_ERR("path=%s", path);
	} else {
		PCIDLIST_ABSOLUTE_ARRAY pIdlArray = &pIdList;
		UINT itemNum = 1;

		// COMライブラリを初期化する。
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr)) {
			LOG_ERR("hr=%u", hr);
		} else {
			// CLSID_FileOperationに関連付けられたオブジェクトを作成する。
			IFileOperation* pfo;
			hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
			if (FAILED(hr)) {
				LOG_ERR("hr=%u", hr);
			} else {
				// 操作のパラメータをセットする。
				hr = pfo->SetOperationFlags(FOF_ALLOWUNDO);
				if (FAILED(hr)) {
					LOG_ERR("hr=%u", hr);
				} else {
					// ITEMIDLIST構造体のリストからシェルアイテム配列オブジェクトを作成する。
					IShellItemArray* pShellItemArr = NULL;
					hr = SHCreateShellItemArrayFromIDLists(itemNum, pIdlArray, &pShellItemArr);
					if (FAILED(hr)) {
						LOG_ERR("hr=%u", hr);
					} else {
						hr = pfo->DeleteItems(pShellItemArr);
						if (FAILED(hr)) {
							LOG_ERR("hr=%u", hr);
						} else {
							hr = pfo->PerformOperations();
							if (FAILED(hr)) {
								LOG_ERR("hr=%u", hr);
							} else {
								rtn = 0;
							}
						}
						pShellItemArr->Release();
					}
				}
				pfo->Release();
			}
			CoUninitialize();
		}
		ILFree(pIdList);
	}
	return rtn;
}

int main(int argc, char* argv[])
{
	int ret = 0;
	for (int i = 1; i < argc; i++) {
		LOG_TRC("argv[i]=%s", argv[i]);
		char* path = TR_GetFullPathName(argv[i]);
		if (path == NULL) {
			LOG_ERR("");
			ret = 1;
			break;
		}
		LOG_TRC("path=%s", path);
		ret = TR_ToTrash(path);
		free(path);
		if (ret != 0) {
			LOG_ERR("");
			break;
		}
		LOG_TRC("success");
	}
	return ret;
}
