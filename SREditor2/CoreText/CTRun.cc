#include "CTRun.h"
#include "SRAttributedString.h"
#include "../UIKit/UIFont.h"
#include "CTRunDelegate.h"
#include <algorithm> // for_each

namespace sr {

CTRun::CTRun() {
}

SRFloat CTRun::getTypographicBounds(SRRange range, SRFloat* ascent, SRFloat* descent, SRFloat* leading) const {
  //if (range.location == -1) range.location = _range.location;
  if (range.length == 0) {
    // If the length of the range is set to 0, then the measure operation continues 
    // from the range's start index to the end of the run.
    //range.location = _range.location; // �̷��� �ϸ� �� ���� Ŭ���� ������ ���� ĳ���� ��ġ�ϰ� �ִ�.
    //range.length = _range.length;
  }
  SR_ASSERT(range.location >= _range.location && range.length <= _range.length);

  UIFontPtr font = std::dynamic_pointer_cast<UIFont>(_attributes.valueForKey(kCTFontAttributeName));
  SR_ASSERT(font); // ��Ʈ�� ���� ���� ����߰ڴ�.
  if (ascent) *ascent = font->ascender();
  if (descent) *descent = font->descender();
  if (leading) *leading = 0.0f;
#if 1
  if (range.length == 0) {
    return 0.0f;
  }
  // ���� ������ _glyphOrigins ������ �ε����� ã�´�.
  SRIndex startIdx = range.location - _range.location;
  SRIndex endIdx = (range.rangeMax() - 1) - _range.location;
  if (startIdx < 0 || endIdx >= _positions.size()) {
    SR_ASSERT(0);
  }
  SRFloat startX = _positions[startIdx].x;
  SRFloat endX = _positions[endIdx].x + _advances[endIdx].width;
  // ���� �۸����� �����ϸ� ���� �۸��� origin ���� ��ġ���� ����
  if (endIdx < _positions.size() - 1) {
    endX = _positions[endIdx + 1].x;
  }
  return endX - startX;
#else
  SRSize size;
  //if (range.length == 0) { range.length = _stringFragment.length() - range.location; }
  for (SRIndex idx = range.location; idx < range.rangeMax(); ++idx) {
    SRSize sz = _glyphAdvances[idx - range.location];
    size.width += sz.width;
    // for debug, check all glyph's height is same.
    if (size.height == 0) size.height = sz.height;
    else SR_ASSERT(size.height == sz.height);
  }
  return size.width;
#endif
}

void CTRun::draw(CGContextPtr ctx, SRRange textRange) {
  if (textRange.length == 0) {
    textRange.length = _stringFragment.length() - textRange.location;
    //textRange.length = _range.length - textRange.location;
  }
  UIFontPtr font = std::dynamic_pointer_cast<UIFont>(_attributes.valueForKey(kCTFontAttributeName));
  UIFontPtr systemFont = UIFont::systemFont();
  ctx->setFont(font);

  SRColorPtr foregroundColor = std::dynamic_pointer_cast<SRColor>(_attributes.valueForKey(kCTForegroundColorAttributeName));
  SRColor crText(0, 0, 0); // default color
  if (foregroundColor) {
    crText = *foregroundColor;
  }

  int idx = 0;
  for (auto& glyph = _positions.begin(); glyph != _positions.end(); ++glyph, ++idx) {
    SRPoint glyphPos = (*glyph);
    if (_stringFragment.data()[idx] != _glyphs[idx]) {
      // _glyphCharacters �� ���� ���ڿ����� ��ȯ�� ���ڸ� �����ϹǷ� �ٸ� �� �ִ�.
      // (e.g, \n -> Downwards Arrow with Tip Leftwards U+21B2)
      // TODO _stringFragment �� �ʿ��Ѱ�?
      //SR_ASSERT(0);
    }
    const CTRunDelegatePtr runDelegate = std::dynamic_pointer_cast<CTRunDelegate>(_attributes.valueForKey(kCTRunDelegateAttributeName));
    if (runDelegate != nullptr) { // �̹��� ���� ǥ���ϱ� ����
      runDelegate->cb().onDraw(runDelegate->ref(), ctx, glyphPos);
    } else {
      // _stringIndices�� ��ü ���ڿ����� �ε����̹Ƿ�, _stringFragment[idx]�� ���ؾ���
      wchar_t c = _stringFragment.data()[idx];
      if (c == '\n') { // ���๮�ڴ� �ý��� ��Ʈ�� ����ϰ� ��
        ctx->setFont(systemFont);
      } else {
        ctx->setFont(font);
      }
      ctx->showGlyphsAtPoint(glyphPos, crText, (const SRGlyph *)&_glyphs[idx], 1);
    }
  }
}

SRRect CTRun::caretRect(SRTextPosition position) const {
  SRIndex index = position._offset;
  if (!(getStringRange().location <= index && index <= getStringRange().rangeMax())) {
    SR_ASSERT(0);
    return SRRect();
  }
  SRIndex localIndex = index - getStringRange().location;

  // e.g, �۸��� 2���� �ؽ�Ʈ 1���� ���
  auto lower = std::lower_bound(_stringIndices.begin(), _stringIndices.end(), index);
  SRIndex glyphIndex = lower - _stringIndices.begin();
  SR_ASSERT(glyphIndex == localIndex);

  SRRect rcCaret;
  rcCaret.origin.x = _positions[glyphIndex].x;
  rcCaret.size.width = 2;

  SRFloat ascent, descent;
  SRFloat width = getTypographicBounds(SRRange(_range.location, 1), &ascent, &descent, nullptr);
  SRFloat height = ascent - descent;
  rcCaret.size.height = height;
  rcCaret.origin.y -= descent; // ĳ���� y ��ġ�� ������ bottom���� ���� ���̸�ŭ ����� �Ѵ�.
  rcCaret.origin.y -= height; // have to set caret's starting y pos
  return rcCaret;
}

} // namespace sr
