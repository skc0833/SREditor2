#pragma once

#include "SRObject.h"
#include "SRAttributedString.h"
#include "CTFrameSetter.h"
#include "CTLine.h"

namespace sr {

class CTFrame;
using CTFramePtr = std::shared_ptr<CTFrame>;

using CTLineArray = std::vector<CTLinePtr>;

/*
https://developer.apple.com/reference/coretext/ctframe-cn8
The CTFrame opaque type represents a frame containing multiple lines of text.
The frame object is the output resulting from the text-framing process performed by a framesetter object.
*/
class CTFrame : public SRObject {
  SR_MAKE_NONCOPYABLE(CTFrame);
public:
  SR_DECL_CREATE_FUNC(CTFrame);

  CTFrame();
  virtual ~CTFrame() = default;

  // Returns the range of characters originally requested to fill the frame.
  SRRange getStringRange() const;

  // Returns the range of characters that actually fit in the frame.
  SRRange getVisibleStringRange() const;

  //Returns the frame attributes used to create the frame.
  //CFDictionary CTFrameGetFrameAttributes();

  // Returns an array of lines stored in the frame.
  const CTLineArray& getLines() const {
    return _lines;
  }

  // Draws an entire frame into a context.
  void draw(CGContextPtr ctx) const;

  // skc added
  const CTLinePtr getShortestLIne() const;
  SRTextPosition closestPosition(SRPoint point) const;
  SRRect caretRect(SRTextPosition position) const;
  SRRect glyphRect(SRRange range) const;
  SRRange lineRange(SRTextPosition position) const;

// private:
  SRRect _frameRect; // 실제 레이아웃된 영역임(전체라인 영역 포함)
  SRRect _frameRectWithoutTailingWhiteSpace; // for debug
  CTLineArray _lines;
  std::vector<SRPoint> _lineOrigins; // 라인들의 시작 위치(baseline은 여기에 ascent를 더한 위치임)
};

} // namespace sr
