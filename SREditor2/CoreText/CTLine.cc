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

  // 현재 라인의 위치는 부모 CTFrame._lineOrigins 에 존재함.
  return SRSize(_width, getLineHeight());
}

/*
전달받은 위치에 해당하는 캐럿 인덱스를 리턴함
글리프는 [origin.x, advance.width) 영역을 차지하게 됨
e.g, 폭 2 이면, 첫 글리프는 [0, 1] 두번째 글리프는 [2, 3] 영역을 차지함
MS워드 균등분할시 블럭설정은 다음 텍스트 바로 이전까지 반전됨
캐럿은 항상 텍스트 바로 이전에 위치하며, 캐럿 바로 왼쪽 텍스트 크기로 설정됨
 a   A   b\n
        | |
|   |   |(이 위치 캐럿이 A 글리프 크기임)

Hit Test시 글리프 기준으로 좌,우 동일한 폭을 갖게 처리됨
(각 글리프의 중간 지점을 기준으로 처리)

01234567890123456789012345
A____B____C____x____y____z
| ||   ||   ||   ||   || |
<Glyph 영역(Hit test, 캐럿 위치용)>
0: [0,2]
1: [3,7]
2: [8,2]
*/
SRIndex CTLine::getCaretIndexForPosition(SRPoint point) {
  // 선택 영역일 경우에는 마지막 개행문자도 포함돼야겠다???
  if (_runs.size() == 0) {
    SR_ASSERT(0);
    return SRNotFound;
  }
  if (_runs.front()->_positions.size() == 0) {
    // 글리프가 하나도 없는 경우는 존재하면 안됨
    SR_ASSERT(0);
    return SRNotFound;
  }
  SR_ASSERT(/*point.x >= 0 && */point.y >= 0);

  // 현재 라인내의 모든 글리프들의 origin 을 저장
  SRIndex charIndex = _strRange.location;
  SRFloat prevX = -1;
  for (auto& run : _runs) {
    for (auto& origin : run->_positions) {
      if (point.x <= origin.x) {
        if (prevX >= 0) {
          // 이전 글리프 존재시 이전, 현재 글리프 origin 의 중간 지점을 기준으로 글리프 인덱스를 리턴
          SRFloat mid = (prevX + origin.x) / 2;
          if (point.x < mid) {
            return charIndex - 1;
          } else {
            return charIndex;
          }
        } else {
          // 이전 글리프가 없는 경우(첫번째 글리프 왼쪽 영역 클릭), 현재 글리프 인덱스를 리턴
          return charIndex;
        }
      }
      ++charIndex;
      prevX = origin.x;
    }
  }

  // 마지막 글리프 origin 이후 클릭시 처리
  SR_ASSERT(charIndex > 0); // 1개 문자 이상 존재해야겠다.
  wchar_t lastChar = _runs.back()->_stringFragment.data().back();
  wchar_t lastGlyph = _runs.back()->getGlyphs().back();
  //SR_ASSERT(lastGlyph == lastGlyph);
  if (lastChar == '\n') {
    // 마지막 문자가 개행문자인 경우, 개행문자 왼쪽 위치를 리턴
    return charIndex - 1;
  } else {
    // 마지막 문자가 개행문자가 아닌 경우, 마지막 문자의 중간 지점을 기준으로 리턴
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
      // 라인의 끝 위치를 넘어서는 경우
      SR_ASSERT(0);
    }
    return nullptr;
    //return runs.back(); // TODO 확인이 필요함!!!
  }
  return *it;
}

SRRect CTLine::glyphRect(SRRange range) const {
  // 캐럿 위치는 캐럿 인덱스 위치의 글리프 시작 위치
  // 캐럿 높이는 캐럿 인덱스 왼쪽 글리프의 높이(라인 시작 위치는 첫 글리프 높이)
  // 선택 영역은 캐럿 위치(글리프 시작 위치)부터 길이 만큼 폭, 라인 높이
  SR_ASSERT(range.location >= 0 && range.length >= 0);
  if (range.length > 0 && (range.rangeMax() <= _strRange.location || _strRange.rangeMax() <= range.location)) {
    // 선택 영역이 현재 라인과 겹치지 않으면, 빈 영역 리턴
    return SRRect();
  }
  //else if ((range.location < _strRange.location || _strRange.rangeMax() < range.location)) {
  //  // 캐럿이 현재 라인 범위가 아닌 경우, 에러처리
  //  SR_ASSERT(0);
  //  return SRRect();
  //}
  // 현재 라인 범위 이내로 제한
  if (range.location < _strRange.location) {
    range.length -= (_strRange.location - range.location);
    range.location = _strRange.location;
  }
  if (range.rangeMax() > _strRange.rangeMax()) {
    range.length = _strRange.rangeMax() - range.location;
  }
  SR_ASSERT(range.location >= _strRange.location && range.rangeMax() <= _strRange.rangeMax());

  // 시작 캐럿 위치 획득(항상 개행문자 이전 위치로만 진입하나???)
  const CTRunArray& runs = getGlyphRuns();
  CTRunPtr run = _findRun(runs, range.location);
  SRBool eol = false;
  if (!run && range.location == _strRange.rangeMax()) {
    // 라인의 끝 위치일 경우, 마지막 런 객체로 설정시킴
    run = runs.back();
    --range.location;
    eol = true;
  }
  SRIndex offsetInRun = range.location - run->getStringRange().location;
  SRRect result;
  result.origin = run->getPositions().at(offsetInRun); // run->getPositions()의 y 값은 현재 항상 0임
  if (eol) {
    // 라인의 끝 위치일 경우, 캐럿 위치를 마지막 글리프 다음 위치로 설정
    result.origin.x += run->getAdvances().at(offsetInRun).width;
  }
  if (range.length == 0) {
    // 캐럿 크기 지정
    if (offsetInRun == 0 && run != runs.front()) {
      // 런의 시작 위치인 경우, 이전 런을 리턴함
      auto it = std::find_if(runs.begin(), runs.end(), [&run](CTRunPtr item) {
        return item == run;
      });
      run = *(--it);
    }
    // 찾은 런의 높이를 구한다.
    SRFloat ascent, descent, leading;
    SRFloat width = run->getTypographicBounds(SRRange(range.location, 0), &ascent, &descent, &leading);
    SRFloat height = ascent - descent;
    result.origin.y -= ascent; // 캐럿은 baseline로부터 ascent만큼 위에 표시함
    result.size = { 2, height };
  } else {
    // 끝 캐럿 위치 획득
    /*
    0   1 2(3) <- (3) 위치는 선택영역에만 포함 가능함
    |a  |b|$|
    |c  |d|$
    3   4 5
    */
#if 1
    if (range.rangeMax() == _strRange.rangeMax()) {
      // EOL 인 경우, 마지막 글리프의 시작위치 + 폭까지 선택
      CTRunPtr run = _findRun(runs, range.rangeMax() - 1);
      SRIndex offsetInRun = range.rangeMax() - 1 - run->getStringRange().location;
      result.size.width = run->getPositions().at(offsetInRun).x - result.origin.x + run->getAdvances().at(offsetInRun).width;
    } else {
      // EOL 이전 위치인 경우, range.rangeMax() 위치의 글리프 시작 위치까지 선택
      CTRunPtr run = _findRun(runs, range.rangeMax());
      SRIndex offsetInRun = range.rangeMax() - run->getStringRange().location;
      result.size.width = run->getPositions().at(offsetInRun).x - result.origin.x;
    }
    result.size.height = getLineHeight();
#else
    CTRunPtr run = _findRun(runs, range.rangeMax() - 1);
    SRIndex offsetInRun = range.rangeMax() - run->getStringRange().location;
    if (offsetInRun >= run->getStringRange().rangeMax()) {
      offsetInRun = run->getStringRange().rangeMax() - 1; // TODO 이렇게 처리하는 게 맞는지 확인이 필요함!!!
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
    // 현재 라인 범위가 아닌 경우
    SR_ASSERT(0);
    return SRRect();
  }

  SRRect caretRect = glyphRect(SRRange(index, 0));
  SRRect rcCaret;
  for (const auto& run : getGlyphRuns()) {
    SRRange runRange = run->getStringRange();
    // glyph의 왼쪽 영역 클릭시에는 이전 글리프 크기를 사용, 오른쪽 영역 클릭시 해당 글리프 크기를 사용
    // (index <= runRange.rangeMax() 조건임. < 로 비교하게 되면 왼쪽 클릭시 현재 글리프가 사용됨(오른쪽으로 하나 쉬프트))
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
  SRPoint lineOrigin = ctx->getTextPosition(); // 라인 시작 위치
  // baseline 위치에 텍스트가 드로잉되게 설정함
  ctx->setTextPosition(ctx->getTextPosition() + SRPoint(0, _ascent));

  if (ctx->_selectedRange.length > 0) {
    SRRect selectedRect = glyphRect(ctx->_selectedRange);
    selectedRect.origin += lineOrigin;
    ctx->fillRect(SRColorLightGray, selectedRect);
  }

  for (auto& run : _runs) {
    // 라인내의 모든 런들의 _glyphOrigins 위치는 라인 시작위치 기준임
    run->draw(ctx);
  }
}

} // namespace sr
