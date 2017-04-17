#include "CTLine.h"
#include <algorithm> // for std::lower_bound

namespace sr {

CTLine::CTLine() {
}

SRIndex CTLine::getGlyphCount() const {
  SRIndex cnt = 0;
  for (const auto& run : _runs) {
    cnt += run->getGlyphCount();
  }
  return cnt;
}

SRFloat CTLine::getTypographicBounds(SRFloat* ascent, SRFloat* descent, SRFloat* leading) {
  if (ascent) {
    *ascent = this->_ascent;
  }
  if (descent) {
    *descent = this->_descent;
  }
  if (leading) {
    *leading = this->_leading;
  }
  return this->_width;
}

SRFloat CTLine::getLineHeight() const {
  SRFloat lineHeight = _ascent - _descent + _leading;
  return lineHeight;
}

SRSize CTLine::getLineSize() const {
  SR_ASSERT(_runs.size() > 0);
  CTRunPtr headRun = _runs.at(0);
  SR_ASSERT(headRun->_positions.size() > 0);
  SR_ASSERT(headRun->_positions[0] == SRPoint(0, 0));

  // ���� ������ ��ġ�� �θ� CTFrame._lineOrigins �� ������.
  return SRSize(_width, getLineHeight());
}

/*
���޹��� ��ġ�� �ش��ϴ� ĳ�� �ε����� ������
�۸����� [origin.x, advance.width) ������ �����ϰ� ��
e.g, �� 2 �̸�, ù �۸����� [0, 1] �ι�° �۸����� [2, 3] ������ ������
MS���� �յ���ҽ� �������� ���� �ؽ�Ʈ �ٷ� �������� ������
ĳ���� �׻� �ؽ�Ʈ �ٷ� ������ ��ġ�ϸ�, ĳ�� �ٷ� ���� �ؽ�Ʈ ũ��� ������
 a   A   b\n
        | |
|   |   |(�� ��ġ ĳ���� A �۸��� ũ����)

Hit Test�� �۸��� �������� ��,�� ������ ���� ���� ó����
(�� �۸����� �߰� ������ �������� ó��)

01234567890123456789012345
A____B____C____x____y____z
| ||   ||   ||   ||   || |
<Glyph ����(Hit test, ĳ�� ��ġ��)>
0: [0,2]
1: [3,7]
2: [8,2]
*/
SRIndex CTLine::getCaretIndexForPosition(SRPoint point) {
  // ���� ������ ��쿡�� ������ ���๮�ڵ� ���Եž߰ڴ�???
  if (_runs.size() == 0) {
    SR_ASSERT(0);
    return SRNotFound;
  }
  if (_runs.front()->_positions.size() == 0) {
    // �۸����� �ϳ��� ���� ���� �����ϸ� �ȵ�
    SR_ASSERT(0);
    return SRNotFound;
  }
  SR_ASSERT(/*point.x >= 0 && */point.y >= 0);

  // ���� ���γ��� ��� �۸������� origin �� ����
  SRIndex charIndex = _strRange.location;
  SRFloat prevX = -1;
  for (auto& run : _runs) {
    for (auto& origin : run->_positions) {
      if (point.x <= origin.x) {
        if (prevX >= 0) {
          // ���� �۸��� ����� ����, ���� �۸��� origin �� �߰� ������ �������� �۸��� �ε����� ����
          SRFloat mid = (prevX + origin.x) / 2;
          if (point.x < mid) {
            return charIndex - 1;
          } else {
            return charIndex;
          }
        } else {
          // ���� �۸����� ���� ���(ù��° �۸��� ���� ���� Ŭ��), ���� �۸��� �ε����� ����
          return charIndex;
        }
      }
      ++charIndex;
      prevX = origin.x;
    }
  }

  // ������ �۸��� origin ���� Ŭ���� ó��
  SR_ASSERT(charIndex > 0); // 1�� ���� �̻� �����ؾ߰ڴ�.
  wchar_t lastChar = _runs.back()->_stringFragment.data().back();
  wchar_t lastGlyph = _runs.back()->getGlyphs().back();
  //SR_ASSERT(lastGlyph == lastGlyph);
  if (lastChar == '\n') {
    // ������ ���ڰ� ���๮���� ���, ���๮�� ���� ��ġ�� ����
    return charIndex - 1;
  } else {
    // ������ ���ڰ� ���๮�ڰ� �ƴ� ���, ������ ������ �߰� ������ �������� ����
    SRFloat lastOrigin = _runs.back()->_positions.back().x;
    SRFloat lastWidth = _runs.back()->_advances.back().width;
    if (prevX != lastOrigin) {
      SR_ASSERT(0);
    }
    SRFloat mid = lastOrigin + (lastWidth / 2);
    if (point.x < mid) {
      return charIndex - 1;
    } else {
      return charIndex;
    }
  }
}

static CTRunPtr _findRun(const CTRunArray& runs, SRIndex strIndex) {
  auto it = std::find_if(runs.begin(), runs.end(), [&strIndex](CTRunPtr run) {
    SRRange range = run->getStringRange();
    return strIndex < range.rangeMax();
  });
  if (it == runs.end()) {
    if (strIndex > runs.back()->getStringRange().rangeMax()) {
      // ������ �� ��ġ�� �Ѿ�� ���
      SR_ASSERT(0);
    }
    return nullptr;
    //return runs.back(); // TODO Ȯ���� �ʿ���!!!
  }
  return *it;
}

SRRect CTLine::glyphRect(SRRange range) const {
  // ĳ�� ��ġ�� ĳ�� �ε��� ��ġ�� �۸��� ���� ��ġ
  // ĳ�� ���̴� ĳ�� �ε��� ���� �۸����� ����(���� ���� ��ġ�� ù �۸��� ����)
  // ���� ������ ĳ�� ��ġ(�۸��� ���� ��ġ)���� ���� ��ŭ ��, ���� ����
  SR_ASSERT(range.location >= 0 && range.length >= 0);
  if (range.length > 0 && (range.rangeMax() <= _strRange.location || _strRange.rangeMax() <= range.location)) {
    // ���� ������ ���� ���ΰ� ��ġ�� ������, �� ���� ����
    return SRRect();
  }
  //else if ((range.location < _strRange.location || _strRange.rangeMax() < range.location)) {
  //  // ĳ���� ���� ���� ������ �ƴ� ���, ����ó��
  //  SR_ASSERT(0);
  //  return SRRect();
  //}
  // ���� ���� ���� �̳��� ����
  if (range.location < _strRange.location) {
    range.length -= (_strRange.location - range.location);
    range.location = _strRange.location;
  }
  if (range.rangeMax() > _strRange.rangeMax()) {
    range.length = _strRange.rangeMax() - range.location;
  }
  SR_ASSERT(range.location >= _strRange.location && range.rangeMax() <= _strRange.rangeMax());

  // ���� ĳ�� ��ġ ȹ��(�׻� ���๮�� ���� ��ġ�θ� �����ϳ�???)
  const CTRunArray& runs = getGlyphRuns();
  CTRunPtr run = _findRun(runs, range.location);
  SRBool eol = false;
  if (!run && range.location == _strRange.rangeMax()) {
    // ������ �� ��ġ�� ���, ������ �� ��ü�� ������Ŵ
    run = runs.back();
    --range.location;
    eol = true;
  }
  SRIndex offsetInRun = range.location - run->getStringRange().location;
  SRRect result;
  result.origin = run->getPositions().at(offsetInRun); // run->getPositions()�� y ���� ���� �׻� 0��
  if (eol) {
    // ������ �� ��ġ�� ���, ĳ�� ��ġ�� ������ �۸��� ���� ��ġ�� ����
    result.origin.x += run->getAdvances().at(offsetInRun).width;
  }
  if (range.length == 0) {
    // ĳ�� ũ�� ����
    if (offsetInRun == 0 && run != runs.front()) {
      // ���� ���� ��ġ�� ���, ���� ���� ������
      auto it = std::find_if(runs.begin(), runs.end(), [&run](CTRunPtr item) {
        return item == run;
      });
      run = *(--it);
    }
    // ã�� ���� ���̸� ���Ѵ�.
    SRFloat ascent, descent, leading;
    SRFloat width = run->getTypographicBounds(SRRange(range.location, 0), &ascent, &descent, &leading);
    SRFloat height = ascent - descent;
    result.origin.y -= ascent; // ĳ���� baseline�κ��� ascent��ŭ ���� ǥ����
    result.size = { 2, height };
  } else {
    // �� ĳ�� ��ġ ȹ��
    /*
    0   1 2(3) <- (3) ��ġ�� ���ÿ������� ���� ������
    |a  |b|$|
    |c  |d|$
    3   4 5
    */
#if 1
    if (range.rangeMax() == _strRange.rangeMax()) {
      // EOL �� ���, ������ �۸����� ������ġ + ������ ����
      CTRunPtr run = _findRun(runs, range.rangeMax() - 1);
      SRIndex offsetInRun = range.rangeMax() - 1 - run->getStringRange().location;
      result.size.width = run->getPositions().at(offsetInRun).x - result.origin.x + run->getAdvances().at(offsetInRun).width;
    } else {
      // EOL ���� ��ġ�� ���, range.rangeMax() ��ġ�� �۸��� ���� ��ġ���� ����
      CTRunPtr run = _findRun(runs, range.rangeMax());
      SRIndex offsetInRun = range.rangeMax() - run->getStringRange().location;
      result.size.width = run->getPositions().at(offsetInRun).x - result.origin.x;
    }
    result.size.height = getLineHeight();
#else
    CTRunPtr run = _findRun(runs, range.rangeMax() - 1);
    SRIndex offsetInRun = range.rangeMax() - run->getStringRange().location;
    if (offsetInRun >= run->getStringRange().rangeMax()) {
      offsetInRun = run->getStringRange().rangeMax() - 1; // TODO �̷��� ó���ϴ� �� �´��� Ȯ���� �ʿ���!!!
    }
    result.size.width = run->getPositions().at(offsetInRun).x - result.origin.x;
    result.size.height = getLineHeight();
#endif
  }
  return result;
}

SRRect CTLine::caretRect(SRTextPosition position) const {
  SRIndex index = position._offset;
  if (!(getStringRange().location <= index && index <= getStringRange().rangeMax())) {
    // ���� ���� ������ �ƴ� ���
    SR_ASSERT(0);
    return SRRect();
  }

  SRRect caretRect = glyphRect(SRRange(index, 0));
  SRRect rcCaret;
  for (const auto& run : getGlyphRuns()) {
    SRRange runRange = run->getStringRange();
    // glyph�� ���� ���� Ŭ���ÿ��� ���� �۸��� ũ�⸦ ���, ������ ���� Ŭ���� �ش� �۸��� ũ�⸦ ���
    // (index <= runRange.rangeMax() ������. < �� ���ϰ� �Ǹ� ���� Ŭ���� ���� �۸����� ����(���������� �ϳ� ����Ʈ))
    if (runRange.location <= index && index <= runRange.rangeMax()) {
      SRFloat ascent, descent, leading;
      SRFloat width = run->getTypographicBounds(SRRange(runRange.location, index - runRange.location)
        , &ascent, &descent, &leading);
      SRFloat height = ascent - descent;
      rcCaret.origin.x += run->getPositions().front().x;
      rcCaret.origin.x += width;
      rcCaret.origin.y -= descent;
      rcCaret.origin.y -= height; // have to set caret's starting y pos
      rcCaret.size = SRSize(2, height);
      if (caretRect != rcCaret) {
        int a = 0;
        //SR_ASSERT(0);
      }
      return caretRect;
    }
  }
  SR_ASSERT(0);
  return rcCaret;
}

void CTLine::draw(CGContextPtr ctx) const {
  SRPoint lineOrigin = ctx->getTextPosition(); // ���� ���� ��ġ
  // baseline ��ġ�� �ؽ�Ʈ�� ����׵ǰ� ������
  ctx->setTextPosition(ctx->getTextPosition() + SRPoint(0, _ascent));

  if (ctx->_selectedRange.length > 0) {
    SRRect selectedRect = glyphRect(ctx->_selectedRange);
    selectedRect.origin += lineOrigin;
    ctx->fillRect(SRColorLightGray, selectedRect);
  }

  for (auto& run : _runs) {
    // ���γ��� ��� ������ _glyphOrigins ��ġ�� ���� ������ġ ������
    run->draw(ctx);
  }
}

} // namespace sr
