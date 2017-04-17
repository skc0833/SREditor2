#include "WIN32WindowImpl.h"
#include <Windows.h>

namespace sr {

static SRInt getNativeSBType(SB_Type type) {
  if (type == SR_SB_HORZ) {
    return SB_HORZ;
  } else if (type == SR_SB_VERT) {
    return SB_VERT;
  }
  SR_ASSERT(0);
}
static const wchar_t* SB_STR(SB_Type type) {
  switch (type) {
  case SR_SB_HORZ :
    return L"SR_SB_HORZ";
  case SR_SB_VERT :
    return L"SR_SB_VERT";
  }
  SR_ASSERT(0);
  return L"";
}

WIN32WindowImpl::WIN32WindowImpl(void* hwnd) {
  _hwnd = hwnd;
}

WIN32WindowImpl::~WIN32WindowImpl() {
}

bool WIN32WindowImpl::invalidateRect(SRRect* rc, bool erase) {
  RECT* lpRect = NULL;
  RECT r;
  if (rc) {
    r.left = rc->left();
    r.top = rc->top();
    r.right = rc->right();
    r.bottom = rc->bottom();
    lpRect = &r;
  }
  // The InvalidateRect function adds a rectangle to the specified window's update region. 
  // The update region represents the portion of the window's client area that must be redrawn.
  int result = ::InvalidateRect((HWND)_hwnd, lpRect, erase);
  SR_ASSERT(result);
  return result;
}

bool WIN32WindowImpl::updateWindow() {
  // UpdateWindow 함수란 생성된 윈도우의 일부가 다른 윈도우 등에 가려졌거나 리사이즈 되었을 경우,
  // 즉 무효화 영역(Invalid Region)에 대해 WM_PAINT 메시지를 해당 윈도우에 보냄으로써 무효화 영역을
  // 갱신시켜 주는 함수임.
  // UpdateWindow 함수 호출을 통해 발생되는 WM_PAINT 메시지는 해당 윈도우의
  // 메시지 큐를 거치지 않고 해당 윈도우에 바로 전달되는 특징을 가짐
  // The UpdateWindow function updates the client area of the specified window by sending a WM_PAINT message to the window 
  // if the window's update region is not empty. 
  // The function sends a WM_PAINT message directly to the window procedure of the specified window, bypassing the application queue. 
  // If the update region is empty, no message is sent.
  int result = ::UpdateWindow((HWND)_hwnd);
  SR_ASSERT(result);
  return result;
}

SRRect WIN32WindowImpl::getClientRect() {
  SR_ASSERT(0); // not used!
  RECT rc;
  ::GetClientRect((HWND)_hwnd, &rc);
  SRRect rect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  return rect;
}

// ScrollBar Stuffes
SRSize WIN32WindowImpl::scrollBarSize(SB_Type type) {
  SRSize ret;
  if (type == SR_SB_HORZ) {
    ret.width = ::GetSystemMetrics(SM_CXHSCROLL);
    ret.height = ::GetSystemMetrics(SM_CYHSCROLL);
    return ret;
  } else if (type == SR_SB_VERT) {
    ret.width = ::GetSystemMetrics(SM_CXVSCROLL);
    ret.height = ::GetSystemMetrics(SM_CYVSCROLL);
    return ret;
  }
  SR_ASSERT(0);
  return ret;
}

SRInt WIN32WindowImpl::scrollPos(SB_Type type) {
  return ::GetScrollPos((HWND)_hwnd, getNativeSBType(type));
}

void WIN32WindowImpl::setScrollPos(SB_Type type, int pos, bool redraw) {
  ::SetScrollPos((HWND)_hwnd, getNativeSBType(type), pos, redraw);
}

bool WIN32WindowImpl::hasScrollBar(SB_Type type) {
  int wndStyle = ::GetWindowLong((HWND)_hwnd, GWL_STYLE);
  if (type == SR_SB_HORZ) {
    return (wndStyle & WS_HSCROLL);
  } else if (type == SR_SB_VERT) {
    return (wndStyle & WS_VSCROLL);
  }
  SR_ASSERT(0);
  return false;
}

void WIN32WindowImpl::setScrollInfo(SB_Type type, const SRScrollInfo& info, bool redraw) {
  SCROLLINFO siNew = { 0 };
  siNew.cbSize = sizeof(siNew);
  siNew.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
  siNew.nMax = info.max;
  siNew.nPos = SR_MIN(siNew.nMax, info.pos);
  siNew.nPage = info.page;
  ::SetScrollInfo((HWND)_hwnd, getNativeSBType(type), &siNew, redraw);
}

void WIN32WindowImpl::getScrollInfo(SB_Type type, SRScrollInfo& info) {
  SCROLLINFO siNew = { 0 };
  siNew.cbSize = sizeof(siNew);
  siNew.fMask = SIF_PAGE | SIF_POS | SIF_RANGE | SIF_TRACKPOS;
  SR_VERIFY(::GetScrollInfo((HWND)_hwnd, getNativeSBType(type), &siNew));
  info.pos = siNew.nPos;
  info.max = siNew.nMax;
  info.page = siNew.nPage;
  info.trackPos = siNew.nTrackPos;
}

void WIN32WindowImpl::showScrollBar(SB_Type type, bool show) {
  //SR_LOGD(L"WIN32WindowImpl::showScrollBar(%s, %d)", SB_STR(type), show);
  SR_VERIFY(::ShowScrollBar((HWND)_hwnd, getNativeSBType(type), show));
}

bool WIN32WindowImpl::createCaret(SRSize caretSize) {
  bool result = ::CreateCaret((HWND)_hwnd, (HBITMAP)NULL, caretSize.width, caretSize.height);
  SR_ASSERT(result);
  return result;
}

bool WIN32WindowImpl::destroyCaret() {
  bool result = ::DestroyCaret();
  SR_ASSERT(result);
  return result;
}

bool WIN32WindowImpl::setCaretPos(SRPoint point) {
  bool result = ::SetCaretPos(point.x, point.y);
  SR_ASSERT(result);
  return result;
}

bool WIN32WindowImpl::showCaret() {
  bool result = ::ShowCaret((HWND)_hwnd);
  SR_ASSERT(result);
  return result;
  //if (!hasSelection()) {
  //  //VERIFY(::SetCaretPos(_pos_xy[0].x_ + _scrollXY.x_, _pos_xy[0].y_ + _scrollXY.y_));
  //  setSize(_pos[0].size().cx_, _pos[0].size().cy_);
  //  setCaretPos(_scrollXY.x_ + _pos[0].xy().x_, _scrollXY.y_ + _pos[0].xy().y_);
  //  _hidden = false;
  //  bool rtn = ::ShowCaret((HWND)_hwnd);
  //  ASSERT(rtn);
  //  return rtn;
  //}
  //return true;
}

bool WIN32WindowImpl::hideCaret() {
  //if (_hidden) {
  //  return true;
  //}
  //_hidden = true;
  return ::HideCaret((HWND)_hwnd);
}

} // namespace sr
