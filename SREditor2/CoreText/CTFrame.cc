#include "CTFrame.h"
#include <algorithm> // std::min_element

namespace sr {

CTFrame::CTFrame() {
}

const CTLinePtr CTFrame::getShortestLIne() const {
  struct comp {
    bool operator()(CTLinePtr l, CTLinePtr r) {
      return l->getGlyphCount() < r->getGlyphCount();
    }
  } compObj;
  CTLinePtr result = *(std::min_element(_lines.begin(), _lines.end(), compObj));
  return result;
}

SRRange CTFrame::getStringRange() const {
  SRRange range(0, 0);
  if (_lines.empty()) {
    //SR_ASSERT(0);
    return range;
  }
  SRRange line = _lines.front()->getStringRange();
  range.location = line.location;

  line = _lines.back()->getStringRange();
  range.length = line.location + line.length; // last index
  range.length -= range.location;
  return range;
}

// TODO 현재 전체 라인의 범위를 리턴하고 있음
SRRange CTFrame::getVisibleStringRange() const {
  return getStringRange();
}

void CTFrame::draw(CGContextPtr ctx) const {
  // 현재 프레임의 좌상단부터 텍스트가 그림
  ctx->setTextPosition(0, 0);

  // 이후부터는 전부 TextPosition 기반으로 출력함!!!
  SRPoint curTextPos = ctx->getTextPosition();
  //curTextPos = { 0, 0 };

  unsigned count = _lines.size();
  for (unsigned i = 0; i < count; i++) {
    CTLinePtr curLine = _lines.at(i);
    SRPoint newPos = curTextPos + _lineOrigins.at(i);

    ctx->setTextPosition(newPos.x, newPos.y); // TextPosition을 다음 라인 위치로 설정
    curLine->draw(ctx);
  }

  ctx->setTextPosition(0, 0);
}

SRTextPosition CTFrame::closestPosition(SRPoint point) const {
  SRTextPosition result;
  result._stickToNextLine = true;

  for (auto it = _lines.begin(); it != _lines.end(); ++it) {
    CTLinePtr line = *it;
    SRFloat lineAscent, lineDescent, lineLeading;
    const SRFloat lineWidth = line->getTypographicBounds(&lineAscent, &lineDescent, &lineLeading);
    const SRFloat lineHeight = lineAscent - lineDescent;
    const SRPoint lineOrigin = _lineOrigins[it - _lines.begin()];
    if (point.y < (lineOrigin.y + lineHeight)) {
      // 찾는 위치가 현재 라인 이내인 경우
      const SRPoint lineOrg = _lineOrigins.at(it - _lines.begin());
      SRPoint caretPos = lineOrg;
      caretPos.y += lineAscent; // baseline 위치

      SRPoint point2 = point;
      point2.x -= caretPos.x; // 가운데, 오른쪽 정렬일 경우, 라인 시작 위치로 보정시킴
      SRIndex gIdx = line->getCaretIndexForPosition(point2);

      for (const auto& run : line->getGlyphRuns()) {
        SRRange runRange = run->getStringRange();
        // glyph의 왼쪽 영역 클릭시에는 이전 글리프 크기를 사용, 오른쪽 영역 클릭시 해당 글리프 크기를 사용
        // (gIdx <= runRange.rangeMax() 조건임. < 로 비교하게 되면 왼쪽 클릭시 현재 글리프가 사용됨(오른쪽으로 하나 쉬프트))
        if (runRange.location <= gIdx && gIdx <= runRange.rangeMax()) {
          SRFloat ascent, descent, leading;
          SRFloat width = run->getTypographicBounds(SRRange(runRange.location, gIdx - runRange.location)
            , &ascent, &descent, &leading);
          SRFloat height = ascent - descent;
          caretPos.x += run->getPositions().front().x;
          caretPos.x += width;
          caretPos.y -= descent;
          caretPos.y -= height; // have to set caret's starting y pos
          result._offset = gIdx;
          if (caretPos.x >= lineOrg.x + lineWidth) {
            result._stickToNextLine = false; // 라인의 끝일 경우임
          }
          return result;
        }
      }
    }
  }
  //SR_ASSERT(0);
  return result;
}

SRRect CTFrame::caretRect(SRTextPosition position) const {
  SRIndex index = position._offset;
  auto it = std::find_if(_lines.begin(), _lines.end(), [&index, &position](CTLinePtr line) {
    if (position._stickToNextLine)
      return index < line->getStringRange().rangeMax(); // 라인 끝의 캐럿을 다음 라인의 시작 위치에 표시
    else
      return index <= line->getStringRange().rangeMax();
  });
  CTLinePtr line = *it; // current line
  SRRect rcCaret = line->caretRect(position);

  SRFloat lineAscent;
  line->getTypographicBounds(&lineAscent, nullptr, nullptr);
  SRIndex lineIndex = it - _lines.begin();
  SRFloat baseline = _lineOrigins[lineIndex].y + lineAscent;
  rcCaret.origin.x += _lineOrigins[lineIndex].x;
  rcCaret.origin.y += baseline; // 캐럿 y = 라인 baseline - 런 높이
  return rcCaret;
}

SRRange CTFrame::lineRange(SRTextPosition position) const {
  SRIndex index = position._offset;
  auto it = std::find_if(_lines.begin(), _lines.end(), [&index, &position](CTLinePtr line) {
    if (position._stickToNextLine)
      return index < line->getStringRange().rangeMax(); // 라인 끝의 캐럿을 다음 라인의 시작 위치에 표시
    else
      return index <= line->getStringRange().rangeMax();
  });
  CTLinePtr line = *it; // current line
  SRRange result = line->getStringRange();
  return result;
}

} // namespace sr
