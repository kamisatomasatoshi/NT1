#include <windows.h>
#include <process.h>
#include <mmsystem.h>

#pragma comment(lib, "wsock32.lib")
#pragma comment(lib, "winmm.lib")


HWND hwMain;

// 送受信する座標データ
struct POS
{
	int x;
	int y;
};
POS pos1P, pos2P, old_pos2P;
RECT rect;

// プロトタイプ宣言
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI Threadfunc(void*);

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_  HINSTANCE hPrevInstance, _In_ LPSTR szCmdLine, _In_ int iCmdShow) {

	MSG  msg;
	WNDCLASS wndclass;
	WSADATA wdData;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = "CWindow";

	RegisterClass(&wndclass);

	// winsock初期化
	WSAStartup(MAKEWORD(2, 0), &wdData);

	hwMain = CreateWindow("CWindow", "Server",
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
		800, 600, NULL, NULL, hInstance, NULL);

	// ウインドウ表示
	ShowWindow(hwMain, iCmdShow);

	// ウィンドウ領域更新(WM_PAINTメッセージを発行)
	UpdateWindow(hwMain);

	// メッセージループ
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// winsock終了
	WSACleanup();

	return 0;
}

// ウインドウプロシージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {

	static HDC hdc, mdc, mdc2P;
	static PAINTSTRUCT ps;
	static HBITMAP hBitmap;
	static HBITMAP hBitmap2P;
	static char str[256];

	static HANDLE hThread;
	static DWORD dwID;

	// WINDOWSから飛んで来るメッセージに対応する処理の記述
	switch (iMsg) {
	case WM_CREATE:
		// リソースからビットマップを読み込む（1P）
		hBitmap = LoadBitmap(
			((LPCREATESTRUCT)lParam)->hInstance,
			MAKEINTRESOURCE(hBitmap));

		// ディスプレイと互換性のある論理デバイス（デバイスコンテキスト）を取得（1P）
		mdc = CreateCompatibleDC(NULL);

		// 論理デバイスに読み込んだビットマップを展開（1P）
		SelectObject(mdc, hBitmap);

		// リソースからビットマップを読み込む（2P）
		hBitmap2P = LoadBitmap(
			((LPCREATESTRUCT)lParam)->hInstance,
			MAKEINTRESOURCE(hBitmap2P));

		// （2P）
		mdc2P = CreateCompatibleDC(NULL);
		// （2P）
		SelectObject(mdc2P, hBitmap2P);

		// 位置情報を初期化
		pos1P.x = pos1P.y = 0;
		pos2P.x = pos2P.y = 100;

		// データを送受信処理をスレッド（WinMainの流れに関係なく動作する処理の流れ）として生成。
		// データ送受信をスレッドにしないと何かデータを受信するまでRECV関数で止まってしまう。
		hThread = (HANDLE)CreateThread(NULL, 0, &Threadfunc, (LPVOID)&pos1P, 0, &dwID);
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_ESCAPE:
			SendMessage(hwnd, WM_CLOSE, NULL, NULL);
			break;
		case VK_RIGHT:
			//☆　→キー押されたらサーバ側キャラのX座標を更新
			pos1P.x += 5;
			break;
		case VK_LEFT:
			//☆　←キー押されたらサーバ側キャラのX座標を更新
			pos1P.x -= 5;
			break;
		case VK_DOWN:
			//☆　↓キー押されたらサーバ側キャラのY座標を更新
			pos1P.y += 5;
			break;
		case VK_UP:
			//☆　↑キー押されたらサーバ側キャラのY座標を更新
			pos1P.y -= 5;
			break;
		}

		// 指定ウィンドウの指定矩形領域を更新領域に追加
		// hWnd	 ：	ウインドウのハンドル
		// lprec ：RECTのポインタ．NULLなら全体
		// bErase：TRUEなら更新領域を背景色で初期化， FALSEなら現在の状態から上書き描画
		// 返り値：成功すればTRUE，それ以外はFALSE
		InvalidateRect(hwnd, NULL, TRUE);

		// WM_PAINTメッセージがウィンドウに送信される
		UpdateWindow(hwnd);
		break;
	case WM_PAINT:
		// 更新領域に描画する為に必要な描画ツール（デバイスコンテキスト）を取得
		hdc = BeginPaint(hwnd, &ps);

		// 転送元デバイスコンテキストから転送先デバイスコンテキストへ
		// 長方形カラーデータのビットブロックを転送
		// サーバ側キャラ描画
		BitBlt(hdc, pos1P.x, pos1P.y, 32, 32, mdc, 0, 0, SRCCOPY);
		// クライアント側キャラ描画
		BitBlt(hdc, pos2P.x, pos2P.y, 32, 32, mdc2P, 0, 0, SRCCOPY);

		wsprintf(str, L"サーバ側：X:%d Y:%d　　クライアント側：X:%d Y:%d", pos1P.x, pos1P.y, pos2P.x, pos2P.y);
		SetWindowText(hwMain, str);

		// 更新領域を空にする
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY: /* ウインドウ破棄時 */
		DeleteObject(hBitmap);
		DeleteDC(mdc);
		DeleteObject(hBitmap2P);
		DeleteDC(mdc2P);

		PostQuitMessage(0);

		return 0;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

/* 通信スレッド関数 */
DWORD WINAPI Threadfunc(void* px) 
{

	SOCKET sWait, sConnect;
	WORD wPort = 8000;
	SOCKADDR_IN saConnect, saLocal;
	int iLen, iRecv;

	// リスンソケット
	sWait = socket(AF_INET, SOCK_STREAM, 0);

	ZeroMemory(&saLocal, sizeof(saLocal));

	// 8000番に接続待機用ソケット作成
	saLocal.sin_family = AF_INET;
	saLocal.sin_addr.s_addr = htonl(INADDR_ANY);
	saLocal.sin_port = htons(wPort);

	if (bind(sWait,(SOCKADDR*)&saConnect, sizeof(saConnect) == SOCKET_ERROR))
	{

		closesocket(sWait);
		SetWindowText(hwMain, L"接続待機ソケット失敗");
		return 1;
	}

	if (listen(sWait, 2) == SOCKET_ERROR) {

		closesocket(sWait);
		SetWindowText(hwMain, L"接続待機ソケット失敗");
		return 1;
	}

	SetWindowText(hwMain, L"接続待機ソケット成功");

	iLen = sizeof(saConnect);

	// 接続受け入れて送受信用ソケット作成
	sConnect = accept(sConnect, (SOCKADDR*)&sConnect, &iLen);

	// 接続待ち用ソケット解放
	closesocket(sWait);

	if (sConnect == INVALID_SOCKET) 
	{

		shutdown(sConnect, 2);
		closesocket(sConnect);
		shutdown(sWait, 2);
		closesocket(sWait);

		SetWindowText(hwMain, L"ソケット接続失敗");

		return 1;
	}

	SetWindowText(hwMain, L"ソケット接続成功");

	iRecv = 0;

	while (1)
	{
		int     nRcv;

		old_pos2P = pos2P;

		// クライアント側キャラの位置情報を受け取り
		nRcv = recv(sConnect, (char*)&pos2P, sizeof(POS), 0);

		if (nRcv == SOCKET_ERROR)break;

		// サーバ側キャラの位置情報を送信
		send(sConnect, (const char*)&pos1P, sizeof(POS), 0);

		// 受信したクライアントが操作するキャラの座標が更新されていたら、更新領域を作って
		// InvalidateRect関数でWM_PAINTメッセージを発行、キャラを再描画する
		if (old_pos2P.x != pos2P.x || old_pos2P.y != pos2P.y)
		{
			rect.left = old_pos2P.x - 10;
			rect.top = old_pos2P.y - 10;
			rect.right = old_pos2P.x + 42;
			rect.bottom = old_pos2P.y + 42;
			InvalidateRect(hwMain, &rect, TRUE);
		}
	}

	shutdown(sConnect, 2);
	closesocket(sConnect);

	return 0;
}