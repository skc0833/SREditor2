#include "WIN32GraphicContextImpl.h"
#include <Windows.h>

#define SR_RGB(r, g, b)  RGB((r) * 255, (g) * 255, (b) * 255)

namespace sr {

static RECT getRECT(const SRRect& rc) {
  RECT r;
  r.left = (LONG)rc.left();
  r.top = (LONG)rc.top();
  r.right = (LONG)rc.right();
  r.bottom = (LONG)rc.bottom();
  return r;
}

static int Win32PenStyle(SRPattern pattern) {
  // PS_INSIDEFRAME
  switch (pattern) {
  case SR_SOLID:
    return PS_SOLID;
  case SR_DASH:
    return PS_DASH;
  case SR_DOT:
    return PS_DOT;
  // The pen is solid. When this pen is used in any GDI drawing function that takes a bounding rectangle,
  // the dimensions of the figure are shrunk so that it fits entirely in the bounding rectangle, 
  // taking into account the width of the pen. This applies only to geometric pens.
  case SR_PS_INSIDEFRAME:
    return PS_INSIDEFRAME;
  }
  SR_ASSERT(0);
  return PS_SOLID;
}

// 헤더에 정의되면 Win32PenInfo 가 순환참조로 컴파일 에러 발생중
std::size_t Win32PenInfoHash::operator()(const sr::Win32PenInfo& k) const {
  std::size_t result = std::hash<float>()(k.color._r) ^
    (std::hash<float>()(k.color._g) << 1) ^
    (std::hash<float>()(k.color._b) << 2) ^
    (std::hash<float>()(k.color._a) << 3) ^
    (std::hash<int>()(k.style) << 4) ^
    (std::hash<int>()(k.width) << 5);
  return result;
}

bool Win32PenInfoEqual::operator()(const sr::Win32PenInfo& lhs, const sr::Win32PenInfo& rhs) const {
  return lhs.color._r == rhs.color._r && lhs.color._g == rhs.color._g
    && lhs.color._b == rhs.color._b && lhs.color._a == rhs.color._a
    && lhs.style == rhs.style && lhs.width == rhs.width;
}

WIN32GraphicContextImpl::WIN32GraphicContextImpl(void* hwnd, SRRect rcSrc, SRRect rcDest, void* destDC)
: _hwnd((HWND)hwnd), _destDC((HDC)destDC), _isCreatedDC(false), _rcSrc(rcSrc), _rcDest(rcDest)
, _strokePattern(SR_SOLID), _lineWidth(1), _strokeColor(0, 0, 0), _fillPattern(SR_SOLID)
, _currentPen(_strokePattern, _lineWidth, _strokeColor) {
  _isPenSelected = _isBrushSelected = false;
  _isFlushed = false;
  if (!_destDC) {
    SR_ASSERT(0);
    _isCreatedDC = true;
    _destDC = ::GetDC((HWND)_hwnd);
  }

  // memory dc 에 그린 후, 소멸자에서 _hDestDC(e.g, WM_PAINT 시의 윈도우 dc)에 그려지게 처리함
  _srcDC = ::CreateCompatibleDC(_destDC);
  HBITMAP memBitmap = ::CreateCompatibleBitmap(_destDC, _rcSrc.width(), _rcSrc.height());
  _oldBitmap = (HBITMAP)::SelectObject(_srcDC, memBitmap);
  //const RECT rtClip = { _rcSrc.left(), _rcSrc.top(), _rcSrc.right(), _rcSrc.bottom() };
  const RECT rtClip = { 0, 0, _rcSrc.right(), _rcSrc.bottom() }; // 전체 메모리 dc 영역 배경색을 채운다(default는 black임)
  ::FillRect(_srcDC, &rtClip, (HBRUSH)::GetStockObject(WHITE_BRUSH));

  // The SetBkMode function sets the background mix mode of the specified device context.
  // The background mix mode is used with text, hatched brushes, and pen styles that are not solid lines.
  // OPAQUE: Background is filled with the current background color before the text, hatched brush, or pen is drawn.
  // TRANSPARENT: Background remains untouched.
  ::SetBkMode(_srcDC, TRANSPARENT); // 필요한가???
}

WIN32GraphicContextImpl::~WIN32GraphicContextImpl() {
  for (const auto& item : _mapPens) {
    HPEN obj = (HPEN)item.second;
    bool rtn = ::DeleteObject(obj);
    SR_ASSERT(rtn);
  }
  for (const auto& item : _mapBrushes) {
    HBRUSH obj = (HBRUSH)item.second;
    bool rtn = ::DeleteObject(obj);
    SR_ASSERT(rtn);
  }

  if (!_isFlushed) {
    SR_ASSERT(0);
  }
  // _hdc 의 _rcSrc.left, top 부터 _rcSrc 의 크기만큼, _hOrgDC 의 _rcDest.left, top 위치에 드로잉함
  //::BitBlt(_destDC, _rcDest.left(), _rcDest.top(), _rcSrc.width(), _rcSrc.height(), _srcDC, _rcSrc.left(), _rcSrc.top(), SRCCOPY); // ok!
  //::BitBlt(_destDC, _rcDest.left(), _rcDest.top(), _rcDest.width(), _rcDest.height(), _srcDC, _rcSrc.left(), _rcSrc.top(), SRCCOPY);
  ::DeleteObject(::SelectObject(_srcDC, _oldBitmap));
  ::DeleteObject(_srcDC);

  if (_isCreatedDC) {
    _isCreatedDC = true;
    ::ReleaseDC(_hwnd, _srcDC);
  }
}

void WIN32GraphicContextImpl::flush(void* destDC, SRRect* rcDest, SRPoint* ptSrc) {
  // _hdc 의 _rcSrc.left, top 부터 _rcSrc 의 크기만큼, _hOrgDC 의 _rcDest.left, top 위치에 드로잉함
  if (!destDC) destDC = _destDC;
  if (!rcDest) rcDest = &_rcDest;
  if (!ptSrc) ptSrc = &_rcSrc.origin;

  ::BitBlt((HDC)destDC, rcDest->left(), rcDest->top(), rcDest->width(), rcDest->height(), _srcDC, ptSrc->x, ptSrc->y, SRCCOPY);
  _isFlushed = true;
}

void WIN32GraphicContextImpl::setLineWidth(SRFloat width) {
  _lineWidth = width;
}

SRFloat WIN32GraphicContextImpl::getLineWidth() {
  return _lineWidth;
}

void WIN32GraphicContextImpl::setFillColor(SRColor cr) {
  _fillColor = cr;
}

void WIN32GraphicContextImpl::setStrokeColor(SRColor cr) {
  _strokeColor = cr;
}

void WIN32GraphicContextImpl::setStrokePattern(SRPattern pattern) {
  _strokePattern = pattern;
}

void WIN32GraphicContextImpl::setFillPattern(SRPattern pattern) {
  _fillPattern = pattern;
}

//void WIN32GraphicContextImpl::translateBy(SRFloat x, SRFloat y) {
//  _x = x;
//  _y = y;
//}

void WIN32GraphicContextImpl::drawRect(SRRect rc) {
  selectPen(Win32PenInfo(_strokePattern, _lineWidth, _strokeColor));
  ::SelectObject(_srcDC, ::GetStockObject(NULL_BRUSH));
  ::Rectangle(_srcDC, rc.left(), rc.top(), rc.right(), rc.bottom());
}

void WIN32GraphicContextImpl::fillRect(SRRect rc) {
  HBRUSH hBrush = getBrush(_fillColor);
  RECT rect = getRECT(rc);
  ::FillRect(_srcDC, &rect, hBrush);
}

HPEN WIN32GraphicContextImpl::getPen(Win32PenInfo penInfo) {
  PenMap::const_iterator it = _mapPens.find(penInfo);
  if (it != _mapPens.end()) {
    void* val = _mapPens.at(penInfo);
    return (HPEN)val;
  }
  //HPEN hPen = ::CreatePen(PS_SOLID, 1, 0x02000000 | cr);
  HPEN hPen = ::CreatePen(Win32PenStyle(penInfo.style), penInfo.width, SR_RGB(penInfo.color._r, penInfo.color._g, penInfo.color._b));
  std::pair<PenMap::iterator, bool> ret = _mapPens.insert(std::pair<Win32PenInfo, void*>(penInfo, hPen));
  if (!ret.second) {
    // already exist
    SR_ASSERT(0);
  }
  return hPen;
}

Win32PenInfo WIN32GraphicContextImpl::selectPen(Win32PenInfo penInfo) {
  Win32PenInfo old = _currentPen;
  if (!_isPenSelected || _currentPen != penInfo) {
    ::SelectObject(_srcDC, getPen(penInfo));
    _isPenSelected = true;
    _currentPen = penInfo;
  }
  return old;
}

HBRUSH WIN32GraphicContextImpl::getBrush(SRColor cr) {
  BrushMap::const_iterator it = _mapBrushes.find(cr);
  if (it != _mapBrushes.end()) {
    void* val = _mapBrushes.at(cr);
    return (HBRUSH)val;
  }
  HBRUSH hBrush = ::CreateSolidBrush(SR_RGB(cr._r, cr._g, cr._b));
  std::pair<BrushMap::iterator, bool> ret = _mapBrushes.insert(std::pair<SRColor, void*>(cr, hBrush));
  if (!ret.second) {
    // already exist
    SR_ASSERT(0);
  }
  return hBrush;
}

SRColor WIN32GraphicContextImpl::selectBrush(SRColor cr) {
  SRColor old = _currentBrush;
  if (!_isBrushSelected || _currentBrush != cr) {
    ::SelectObject(_srcDC, getBrush(cr));
    _isBrushSelected = true;
    _currentBrush = cr;
  }
  return old;
}

static POINT* _convertPOINT(const std::vector<SRPoint>& points) {
  POINT* pt = new POINT[points.size()];
  std::vector<SRPoint>::const_iterator it = points.begin();
  for (int i = 0; it != points.end(); ++it, ++i) {
    SRPoint p = *it;
    pt[i].x = p.x;
    pt[i].y = p.y;
  }
  return pt;
}

void WIN32GraphicContextImpl::polyFillOutlined(const std::vector<SRPoint>& points, SRColor crFill, SRColor crOutline) {
  SRColor oldBrush = selectBrush(crFill);
  //SRColor oldPen = selectPen(crOutline);
  POINT* pt = _convertPOINT(points);
  // The Polygon function draws a polygon consisting of two or more vertices connected by straight lines.
  // The polygon is outlined by using the current pen and filled by using the current brush and polygon fill mode.
  ::Polygon(_srcDC, pt, points.size());
  delete[] pt;
  selectBrush(oldBrush);
  //selectPen(oldPen);
}

void WIN32GraphicContextImpl::polyLine(const std::vector<SRPoint>& points, SRColor cr) {
  //SRColor oldPen = selectPen(cr);
  POINT* pt = _convertPOINT(points);
  ::Polyline(_srcDC, pt, points.size());
  delete[] pt;
  //selectPen(oldPen);
}

void WIN32GraphicContextImpl::drawImage(const SRString& path, SRRect rect) {
  //HBITMAP _hBitmap = (HBITMAP)::LoadImage(g_hInst(), path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
  HBITMAP _hBitmap = (HBITMAP)::LoadImage(NULL, path.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
  HDC hdcMem = ::CreateCompatibleDC((HDC)_srcDC);
  HBITMAP oldBitmap = (HBITMAP)::SelectObject(hdcMem, (HBITMAP)_hBitmap);

  // bad: SRCPAINT, SRCERASE, SRCINVERT, good: SRCCOPY, SRCAND(블럭선택 반영됨)
  //::BitBlt((HDC)_hdc, 100, 100, 50, 50, hdcMem, 0, 0, SRCAND); // SRCCOPY
  ::BitBlt((HDC)_srcDC, rect.origin.x, rect.origin.y - rect.height(), rect.width(), rect.height()
    , hdcMem, 0, 0, SRCCOPY); // SRCCOPY

  ::SelectObject(hdcMem, oldBitmap);
  ::DeleteObject(_hBitmap);
  ::DeleteDC(hdcMem);
}

void WIN32GraphicContextImpl::showGlyphsAtPoint(SRFloat x, SRFloat y, const SRColor& color, const SRGlyph* glyphs, size_t count) {
  UINT err = ::SetTextAlign(_srcDC, TA_BASELINE); // baseline 을 기준으로 그려야한다.
  SR_ASSERT(err != GDI_ERROR);
  ::SetTextColor(_srcDC, SR_RGB(color._r, color._g, color._b));
  HFONT old = (HFONT)::SelectObject(_srcDC, _curFont->fontInfo()->getDeviceFontInfo());
  wchar_t* text = (wchar_t*)glyphs;
  ::TextOutW(_srcDC, x, y, text, count);
  //wchar_t newline = 0x21B5;
  //::TextOutW(_srcDC, x, y, &newline, 1); // ok!
  ::SelectObject(_srcDC, old);
}

void WIN32GraphicContextImpl::setViewportOrg(SRFloat x, SRFloat y, SRPoint* prev) {
  POINT pt;
  SR_VERIFY(::SetViewportOrgEx(_srcDC, x, y, &pt));
  if (prev) {
    prev->x = pt.x;
    prev->y = pt.y;
  }
}

SRPoint WIN32GraphicContextImpl::getViewportOrg() {
  POINT pt;
  ::GetViewportOrgEx(_srcDC, &pt);
  return SRPoint(pt.x, pt.y);
}

void WIN32GraphicContextImpl::intersectClipRect(const SRRect& rect) {
  RECT rcClip;
  ::GetClipBox(_srcDC, &rcClip);
  // The IntersectClipRect function creates a new Clip Region from the intersection of the current Clip Region
  // and the specified rectangle.
  ::IntersectClipRect(_srcDC, rect.left(), rect.top(), rect.right(), rect.bottom());
  ::GetClipBox(_srcDC, &rcClip);
  if (rect != SRRect(rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top)) {
    // rect.top() 이 음수인 경우, rcClip 은 0 으로 설정되어 값이 다를 수 있다.
    //SR_ASSERT(0);
    int a = 0;
  }
}

void WIN32GraphicContextImpl::setClipBox(const SRRect& rect) {
  RECT rcClip;
  ::GetClipBox(_srcDC, &rcClip);
  HRGN hrgn = ::CreateRectRgn(rect.left(), rect.top(), rect.right(), rect.bottom());
  SR_ASSERT(hrgn != NULL);
  ::SelectClipRgn(_srcDC, hrgn);
  ::DeleteObject(hrgn);
  ::GetClipBox(_srcDC, &rcClip);
  if (rect != SRRect(rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top)) {
    //SR_ASSERT(0); // 이런 경우가 생기고 있음(rcClip.top 이 불일치)
    int a = 0;
  }
}

SRRect WIN32GraphicContextImpl::getClipBox() {
  RECT rcClip;
  ::GetClipBox(_srcDC, &rcClip);
  SRRect clip(rcClip.left, rcClip.top, rcClip.right - rcClip.left, rcClip.bottom - rcClip.top);
  return clip;
}

} // namespace sr
