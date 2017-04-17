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

// TODO ���� ��ü ������ ������ �����ϰ� ����
SRRange CTFrame::getVisibleStringRange() const {
  return getStringRange();
}

void CTFrame::draw(CGContextPtr ctx) const {
  // ���� �������� �»�ܺ��� �ؽ�Ʈ�� �׸�
  ctx->setTextPosition(0, 0);

  // ���ĺ��ʹ� ���� TextPosition ������� �����!!!
  SRPoint curTextPos = ctx->getTextPosition();
  //curTextPos = { 0, 0 };

  unsigned count = _lines.size();
  for (unsigned i = 0; i < count; i++) {
    CTLinePtr curLine = _lines.at(i);
    SRPoint newPos = curTextPos + _lineOrigins.at(i);

    ctx->setTextPosition(newPos.x, newPos.y); // TextPosition�� ���� ���� ��ġ�� ����
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
      // ã�� ��ġ�� ���� ���� �̳��� ���
      const SRPoint lineOrg = _lineOrigins.at(it - _lines.begin());
      SRPoint caretPos = lineOrg;
      caretPos.y += lineAscent; // baseline ��ġ

      SRPoint point2 = point;
      point2.x -= caretPos.x; // ���, ������ ������ ���, ���� ���� ��ġ�� ������Ŵ
      SRIndex gIdx = line->getCaretIndexForPosition(point2);

      for (const auto& run : line->getGlyphRuns()) {
        SRRange runRange = run->getStringRange();
        // glyph�� ���� ���� Ŭ���ÿ��� ���� �۸��� ũ�⸦ ���, ������ ���� Ŭ���� �ش� �۸��� ũ�⸦ ���
        // (gIdx <= runRange.rangeMax() ������. < �� ���ϰ� �Ǹ� ���� Ŭ���� ���� �۸����� ����(���������� �ϳ� ����Ʈ))
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
            result._stickToNextLine = false; // ������ ���� �����
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
      return index < line->getStringRange().rangeMax(); // ���� ���� ĳ���� ���� ������ ���� ��ġ�� ǥ��
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
  rcCaret.origin.y += baseline; // ĳ�� y = ���� baseline - �� ����
  return rcCaret;
}

SRRange CTFrame::lineRange(SRTextPosition position) const {
  SRIndex index = position._offset;
  auto it = std::find_if(_lines.begin(), _lines.end(), [&index, &position](CTLinePtr line) {
    if (position._stickToNextLine)
      return index < line->getStringRange().rangeMax(); // ���� ���� ĳ���� ���� ������ ���� ��ġ�� ǥ��
    else
      return index <= line->getStringRange().rangeMax();
  });
  CTLinePtr line = *it; // current line
  SRRange result = line->getStringRange();
  return result;
}

} // namespace sr
