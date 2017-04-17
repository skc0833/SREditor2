#pragma once

#include "CoreGraphics.h"
#include "SRColor.h"
#include "../UIKit/UIFont.h"

namespace sr {

class CGContextImpl;
using CGContextImplPtr = std::shared_ptr<CGContextImpl>;

class CGContextImpl : public SRObject {
  SR_MAKE_NONCOPYABLE(CGContextImpl);
public:
  CGContextImpl();
  virtual ~CGContextImpl();

public:
  /**
   * Displays glyphs at the specified positions, specified in text-space.
   * The current text position is not updated modified.
   *
   * Note that because the positions are given in text space, they are
   * transformed by the text matrix (i.e. the current text position
   * affects the final glyph positions)
   */
  //void showGlyphsAtPositions(const SRGlyph glyphs[], const SRPoint positions[], size_t count);
  virtual void drawImage(const SRString& path, SRRect rect) = 0;
  virtual void showGlyphsAtPoint(SRFloat x, SRFloat y, const SRColor& color, const SRGlyph *glyphs, size_t count) = 0;
  virtual SRPoint getTextPosition();
  virtual void setTextPosition(SRFloat x, SRFloat y);
  virtual void setFont(UIFontPtr font);
  virtual void setFontSize(SRFloat size);

  virtual void setLineWidth(SRFloat width) = 0;
  virtual SRFloat getLineWidth() = 0;

  virtual void setStrokeColor(SRColor cr) = 0;
  virtual void setStrokePattern(SRPattern pattern) = 0;
  virtual void setFillColor(SRColor cr) = 0;
  virtual void setFillPattern(SRPattern pattern) = 0;

  virtual void drawRect(SRRect rc) = 0;
  virtual void fillRect(SRRect rc) = 0;
  virtual void polyFillOutlined(const std::vector<SRPoint>& points, SRColor crFill, SRColor crOutline) = 0;
  virtual void polyLine(const std::vector<SRPoint>& points, SRColor cr) = 0;

  virtual void setViewportOrg(SRFloat x, SRFloat y, SRPoint* prev = nullptr) = 0;
  virtual SRPoint getViewportOrg() = 0;
  virtual void intersectClipRect(const SRRect& rect) = 0;
  virtual void setClipBox(const SRRect& rect) = 0;
  virtual SRRect getClipBox() = 0;

  virtual void* getSrcDC() = 0;
  virtual void flush(void* destDC = nullptr, SRRect* rcDest = nullptr, SRPoint* ptSrc = nullptr) = 0;

protected:
  SRPoint _curTextPosition;
  UIFontPtr _curFont;
  SRFloat _curFontSize; // 사용되나???
};

} // namespace sr
