#pragma once

#include "CGContextImpl.h"
#include "SRColor.h"
#include "../UIKit/UIFont.h"
#include <vector>
#include <stack>

namespace sr {

class CGContextImpl;
using CGContextImplPtr = std::shared_ptr<CGContextImpl>;

class CGContext;
using CGContextPtr = std::shared_ptr<CGContext>;

class CGContext : public SRObject {
  typedef struct {
    CGAffineTransform _ctm; // current transformation matrix.
    SRPath _clip;
    SRPoint _textPosition;
  } GState;

  SR_MAKE_NONCOPYABLE(CGContext);
public:
  SR_DECL_CREATE_FUNC(CGContext);

  CGContext(CGContextImplPtr impl);
  virtual ~CGContext();

  /**
   * Displays glyphs at the specified positions, specified in text-space.
   * The current text position is not updated modified.
   *
   * Note that because the positions are given in text space, they are
   * transformed by the text matrix (i.e. the current text position
   * affects the final glyph positions)
   */
  //void showGlyphsAtPositions(const SRGlyph glyphs[], const SRPoint positions[], size_t count);

  // The default line width is 1 unit. When stroked, the line straddles the path, with half of the total width on either side.
  void setLineWidth(SRFloat width);
  SRFloat getLineWidth();
  void setFillColor(SRColor cr);
  void setStrokeColor(SRColor cr);
  void setStrokePattern(SRPattern pattern);
  void setFillPattern(SRPattern pattern);
  void translateBy(SRFloat x, SRFloat y);

  void drawImage(const SRString& path, SRRect rect);
  void showGlyphsAtPoint(SRPoint pos, const SRColor& color, const SRGlyph *glyphs, size_t count);
  void drawRect(SRRect rect);
  void drawRect(SRPattern pattern, SRColor cr, SRRect rect);
  void fillRect(SRRect rect);
  void fillRect(SRColor cr, SRRect rect);
  void polyFillOutlined(const std::vector<SRPoint>& points, SRColor crFill, SRColor crOutline);
  void polyLine(const std::vector<SRPoint>& points, SRColor cr);

  SRPoint getTextPosition() const;
  void setTextPosition(SRFloat x, SRFloat y);
  void setTextPosition(SRPoint pt) {
    setTextPosition(pt.x, pt.y);
  }
  void setFont(UIFontPtr font);
  void setFontSize(SRFloat size);

  CGAffineTransform getTextMatrix() const {
    return _curState._ctm; // 여기서 복사가 이뤄지므로 SR_MAKE_NONCOPYABLE(CGAffineTransform) 를 사용하면 안된다.
  }
  void setTextMatrix(CGAffineTransform tm) {
    _curState._ctm = tm;
  }

  CGContextImplPtr getContextImpl() {
    return _backing;
  }

  void saveGState(); // stack push/pop current CGContext
  void restoreGState();

  void setViewportOrg(SRFloat x, SRFloat y, SRPoint* prev = nullptr);
  SRPoint getViewportOrg();
  
  // Sets the clipping path to the intersection of the current clipping path with the area defined by the specified rectangle.
  void intersectClipRect(const SRRect& rect);
  void setClipBox(const SRRect& rect);
  SRRect getClipBox() const;

  void* getSrcDC() const;
  void flush(void* destDC = nullptr, SRRect* rcDest = nullptr, SRPoint* ptSrc = nullptr) const;

  void setDebugDraw(SRBool on) { _debugDraw = on; }
  SRBool debugDraw() const { return _debugDraw; }

  SRRange _selectedRange; // 텍스트 선택 영역

private:
  SRRect applyCTM(SRRect rect) const;
  SRPoint applyCTM(SRPoint pt) const;

  CGContextImplPtr _backing;
  //CGAffineTransform _ctm; // current transformation matrix.
  std::stack<GState> _savedStates;
  GState _curState;

  SRBool _debugDraw;
};

} // namespace sr
