#pragma once

#include "SRObject.h"
#include "SRAttributedString.h"
#include "CTRun.h"

namespace sr {

class CTLine;
using CTLinePtr = std::shared_ptr<CTLine>;
using CTRunArray = std::vector<CTRunPtr>;

/*
https://developer.apple.com/reference/coretext/ctline-61l
A CTLine object contains an array of glyph runs. Line objects are created by the typesetter
during a framesetting operation and can draw themselves directly into a graphics context.
*/
// 1 개의 라인을 Glyph Run의 배열로 관리함
class CTLine : public SRObject {
  friend class CTFramesetter;
  friend class CTTypesetter;
  friend class SRLayoutManager;
  SR_MAKE_NONCOPYABLE(CTLine);
public:
  SR_DECL_CREATE_FUNC(CTLine);

  CTLine();
  virtual ~CTLine() = default;

  // Draws a complete line.
  void draw(CGContextPtr ctx) const;

  // Returns the total glyph count for the line object.
  SRIndex getGlyphCount() const;

  // Returns the array of glyph runs that make up the line object.
  const CTRunArray& getGlyphRuns() const {
    return _runs;
  }

  // Gets the range of characters that originally spawned the glyphs in the line.
  SRRange getStringRange() const {
    return _strRange;
  }

  // Calculates the typographic bounds of a line.
  SRFloat getTypographicBounds(SRFloat* ascent, SRFloat* descent, SRFloat* leading);

  // Returns the trailing whitespace width for a line.
  SRFloat getTrailingWhitespaceWidth() const {
    return _trailingWhitespaceWidth;
  }

  // Performs hit testing.
  SRIndex getCaretIndexForPosition(SRPoint point);

  // Determines the graphical offset or offsets for a string index.
  //SRFloat CTLineGetOffsetForStringIndex(SRIndex charIndex, SRFloat* secondaryOffset);

  // skc added
  //SRIndex getGlyphIndexForPosition(SRPoint point);
  SRRect caretRect(SRTextPosition position) const;
  SRRect glyphRect(SRRange range) const;

  // 라인의 크기를 리턴
  SRSize getLineSize() const;

  // 라인의 높이를 리턴
  SRFloat getLineHeight() const;
  
private:
  SRRange _strRange;
  SRFloat _width;
  // 라인 끝의 공백 문자열 폭(레이아웃시 요청받은 폭을 넘어가도 현재 라인에 추가되게 처리함. 디버깅용으로 추가함)
  SRFloat _trailingWhitespaceWidth;
  SRFloat _ascent, _descent, _leading;
  CTRunArray _runs;
};

} // namespace sr
