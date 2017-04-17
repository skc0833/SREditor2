#pragma once

#include "../CoreGraphics/CGContextImpl.h"
#include <unordered_map>
#include <Windows.h> // for HDC

namespace sr {

struct SRColorHash {
  std::size_t operator()(const sr::SRColor& k) const {
    std::size_t result = std::hash<float>()(k._r) ^
      (std::hash<float>()(k._g) << 1) ^ (std::hash<float>()(k._b) << 2) ^ (std::hash<float>()(k._a) << 3);
    return result;
  }
};
struct SRColorEqual { // struct std::equal_to<sr::SRColor> 를 정의해도 된다(UIFont::_fontCache 참고)
  bool operator()(const sr::SRColor& lhs, const sr::SRColor& rhs) const {
    return lhs._r == rhs._r && lhs._g == rhs._g && lhs._b == rhs._b && lhs._a == rhs._a;
  }
};

struct Win32PenInfo;

struct Win32PenInfoHash {
  std::size_t operator()(const sr::Win32PenInfo& k) const;
};
struct Win32PenInfoEqual {
  bool operator()(const sr::Win32PenInfo& lhs, const sr::Win32PenInfo& rhs) const;
};

typedef struct Win32PenInfo {
  Win32PenInfo(SRPattern style, SRInt width, SRColor color)
    : style(style), width(width), color(color) {}
  virtual ~Win32PenInfo() = default;
  bool operator==(const Win32PenInfo& rhs) const {
    return Win32PenInfoEqual()(*this, rhs);
  }
  bool operator!=(const Win32PenInfo& rhs) const {
    return !operator==(rhs);
  }

  SRPattern style;
  SRInt width;
  SRColor color;
} Win32PenInfo;

class WIN32GraphicContextImpl;
using WIN32GraphicContextImplPtr = std::shared_ptr<WIN32GraphicContextImpl>;

using PenMap = std::unordered_map<Win32PenInfo, void*, Win32PenInfoHash, Win32PenInfoEqual>;
using BrushMap = std::unordered_map<SRColor, void*, SRColorHash, SRColorEqual>;

class WIN32GraphicContextImpl : public CGContextImpl {
  SR_MAKE_NONCOPYABLE(WIN32GraphicContextImpl);
public:
  SR_DECL_CREATE_FUNC(WIN32GraphicContextImpl);

  WIN32GraphicContextImpl(void* hwnd, SRRect rcSrc, SRRect rcDest, void* hdc = nullptr);
  virtual ~WIN32GraphicContextImpl();

public:
  //void setTextPosition(SRFloat x, SRFloat y);

  /**
   * Displays glyphs at the specified positions, specified in text-space.
   * The current text position is not updated modified.
   *
   * Note that because the positions are given in text space, they are
   * transformed by the text matrix (i.e. the current text position
   * affects the final glyph positions)
   */
  //void showGlyphsAtPositions(const SRGlyph glyphs[], const SRPoint positions[], size_t count);

  virtual void drawImage(const SRString& path, SRRect rect);
  virtual void showGlyphsAtPoint(SRFloat x, SRFloat y, const SRColor& color, const SRGlyph *glyphs, size_t count);
  virtual void setLineWidth(SRFloat width);
  virtual SRFloat getLineWidth();
  virtual void setFillColor(SRColor cr);
  virtual void setStrokeColor(SRColor cr);
  virtual void setStrokePattern(SRPattern pattern);
  virtual void setFillPattern(SRPattern pattern);
  //virtual void translateBy(SRFloat x, SRFloat y);

  virtual void drawRect(SRRect rc);
  virtual void fillRect(SRRect rc);
  virtual void polyFillOutlined(const std::vector<SRPoint>& points, SRColor crFill, SRColor crOutline);
  virtual void polyLine(const std::vector<SRPoint>& points, SRColor cr);

  virtual void setViewportOrg(SRFloat x, SRFloat y, SRPoint* prev = nullptr);
  virtual SRPoint getViewportOrg();
  virtual void intersectClipRect(const SRRect& rect);
  virtual void setClipBox(const SRRect& rect);
  virtual SRRect getClipBox();

  virtual void* getSrcDC() {
    return (void*)_srcDC;
  }
  virtual void flush(void* destDC = nullptr, SRRect* rcDest = nullptr, SRPoint* ptSrc = nullptr);

private:
  HPEN getPen(Win32PenInfo penInfo);
  Win32PenInfo selectPen(Win32PenInfo penInfo);
  HBRUSH getBrush(SRColor cr);
  SRColor selectBrush(SRColor cr);

private:
  bool _isFlushed; // for debug
  HDC _srcDC, _destDC;
  SRRect _rcSrc, _rcDest;
  HBITMAP _oldBitmap;

  SRFloat _lineWidth;
  SRColor _fillColor;
  SRColor _strokeColor;
  SRPattern _strokePattern;
  SRPattern _fillPattern;
  SRFloat _x;
  SRFloat _y;

  HWND _hwnd;
  bool _isCreatedDC;
  PenMap _mapPens;
  Win32PenInfo _currentPen;
  bool _isPenSelected;
  BrushMap _mapBrushes;
  SRColor _currentBrush;
  bool _isBrushSelected;
};

} // namespace sr
