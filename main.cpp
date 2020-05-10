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

#define LOGW_XXX(fmt, ...)  fwprintf(stderr, L"%-15.15s|%-15.15s|%-5d|" fmt L"\n", __FILEW__, __FUNCTIONW__, __LINE__, ##__VA_ARGS__);
#define LOGW_ERR(fmt, ...)  LOGW_XXX("ERR|" fmt, ##__VA_ARGS__)
#define LOGW_TRC(fmt, ...)  LOGW_XXX("TRC|" fmt, ##__VA_ARGS__)

/**
@brief 相対パスから絶対パスを作成する。
*/
wchar_t* TR_GetFullPathName(const wchar_t* path)
{
	// 絶対パス格納に必要なサイズを取得する。
	DWORD required_size = GetFullPathNameW(path, 0, NULL, NULL);
	if (required_size == 0) {
		LOG_ERR("GetLastError()=%lu", GetLastError());
		return NULL;
	}
	wchar_t* buf = (wchar_t*)calloc(required_size, sizeof(wchar_t));
	if (buf == NULL) {
		LOG_ERR("");
		return NULL;
	}
	// 絶対パスを格納する。
	DWORD ret = GetFullPathNameW(path, required_size, buf, NULL);
	if (ret == 0 || required_size <= ret) {
		LOG_ERR("");
		free(buf);
		return NULL;
	}
	return buf;
}

/**
@brief ファイルやフォルダをWindowsのゴミ箱へ捨てる。

@param path 絶対パス

@return 0:成功, 0以外:失敗
*/
int TR_ToTrash(const wchar_t* path)
{
	int rtn = 1;

	PIDLIST_ABSOLUTE pIdList = ILCreateFromPathW(path);
	if (pIdList == nullptr) {
		LOGW_ERR(L"path=%ls", path);
	} else {
		PCIDLIST_ABSOLUTE_ARRAY pIdlArray = &pIdList;
		UINT itemNum = 1;

		// COMライブラリを初期化する。
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		if (FAILED(hr)) {
			LOG_ERR("hr=%ld", hr);
		} else {
			// CLSID_FileOperationに関連付けられたオブジェクトを作成する。
			IFileOperation* pfo;
			hr = CoCreateInstance(CLSID_FileOperation, NULL, CLSCTX_ALL, IID_PPV_ARGS(&pfo));
			if (FAILED(hr)) {
				LOG_ERR("hr=%ld", hr);
			} else {
				// 操作のパラメータをセットする。
				hr = pfo->SetOperationFlags(
					FOF_NOCONFIRMATION | // すべてのダイアログボックスで YES を選択する。
					FOF_SILENT         | // 進捗ダイアログを表示しない。
					FOFX_RECYCLEONDELETE // 削除時の代わりにゴミ箱へ移動する。
				);
				if (FAILED(hr)) {
					LOG_ERR("hr=%ld", hr);
				} else {
					// ITEMIDLIST構造体のリストからシェルアイテム配列オブジェクトを作成する。
					IShellItemArray* pShellItemArr = NULL;
					hr = SHCreateShellItemArrayFromIDLists(itemNum, pIdlArray, &pShellItemArr);
					if (FAILED(hr)) {
						LOG_ERR("hr=%ld", hr);
					} else {
						// 削除操作を登録。
						hr = pfo->DeleteItems(pShellItemArr);
						if (FAILED(hr)) {
							LOG_ERR("hr=%ld", hr);
						} else {
							// 登録した操作を実行する。
							hr = pfo->PerformOperations();
							if (FAILED(hr)) {
								LOG_ERR("hr=%ld", hr);
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

int wmain(int argc, wchar_t* argv[])
{
	int ret = 0;
	for (int i = 1; i < argc; i++) {
		LOGW_TRC(L"argv[i]=%ls", argv[i]);
		wchar_t* path = TR_GetFullPathName(argv[i]);
		if (path == NULL) {
			LOG_ERR("");
			ret = 1;
			break;
		}
		LOGW_TRC(L"path=%ls", path);
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
