#include "CTTypesetter.h"
#include "CoreGraphics/CoreGraphics.h"
#include "UIKit/UIFont.h"
#include "CTRun.h"
#include "CTRunDelegate.h"
#include "CTParagraphStyle.h"
#include <algorithm> // for std::remove

namespace sr {

// https://unicode-table.com/en/search/?q=arrow
const static wchar_t _new_line_glyph = 0x21B2; // Downwards Arrow with Tip Leftwards U+21B2, Downwards Arrow with Corner Leftwards U+21B5

static const SRInt LINE_FEED_CHAR = 10; // '\n'
static const SRInt CARRIAGE_RETURN_CHAR = 13; // '\r'

static bool _isWhiteSpace(SRInt ch) {
	return (ch== ' ' || ch == '\t' || ch == _new_line_glyph || ch == '`'/*for debug*/);
}

CTTypesetter::CTTypesetter(const SRAttributedStringPtr string) {
  _attributedString = string;
}

SRAttributedStringPtr CTTypesetter::attributedString() const {
  try {
    auto ptr = _attributedString.lock();
    SR_ASSERT(ptr);
    return ptr;
  }
  catch (std::bad_weak_ptr b) {
    // 이미 delete 된 포인터였을 경우 진입한다.
    // std::weak_ptr refers to an already deleted object
    SR_ASSERT(0);
  }
  return nullptr;
}

SRIndex CTTypesetter::charactersLen() {
  return attributedString()->getLength();
}

SRFloat CTTypesetter::fillLine(const SRRange lineRange, std::vector<SRPoint>& glyphOrigins,
  std::vector<SRSize>& glyphAdvances, std::vector<wchar_t>& glyphCharacters,
  std::vector<SRIndex>& stringIndices, CTLinePtr outLine) {
  SRFloat maxAscender = 0.0f;
  SRFloat maxDescender = 0.0f;
  SRFloat maxLeading = 0.0f; // not used!

  SRIndex curIdx = lineRange.location;
  const SRIndex endIdx = lineRange.rangeMax();
  SRIndex glyphIdx = 0;
  SRRange curRange;
  outLine->_width = 0.0f;
  outLine->_trailingWhitespaceWidth = 0.0f;

  while (curIdx < endIdx) {
    SRObjectDictionaryPtr attribs = attributedString()->attributes(curIdx, &curRange);

    SRRange runRange;
    runRange.location = curIdx;
    runRange.length = (curRange.location + curRange.length) - curIdx;
    if (runRange.location + runRange.length > lineRange.location + lineRange.length) {
      // 현재 속성 블럭이 현재 라인을 넘어서는 경우, 현재 라인 끝까지로 제한시킴
      // 동일 속성이 여러 라인에 걸쳐 길 경우 발생됨
      runRange.length = lineRange.location + lineRange.length - runRange.location;
    }

    CTRunPtr run = CTRun::create();
    UIFontPtr runFont = std::dynamic_pointer_cast<UIFont>(attribs->valueForKey(kCTFontAttributeName));
    if (runFont == nullptr) {
      SR_ASSERT(0); // 폰트가 없는 경우는 없어야겠다.
      runFont = UIFont::systemFont();
      run->getAttributes().setValue(runFont, kCTFontAttributeName); // 폰트가 없을 경우, 디폴트 폰트로 추가함
    }

    // kCTRunDelegateAttributeName 확인
    CTRunDelegatePtr runDelegate = std::dynamic_pointer_cast<CTRunDelegate>(attribs->valueForKey(kCTRunDelegateAttributeName));
    SRFloat curAscender = runFont->ascender();
    SRFloat curDescender = runFont->descender();

    if (runDelegate != nullptr) {
      curAscender = runDelegate->cb().getAscent(runDelegate->ref());
      curDescender = runDelegate->cb().getDescent(runDelegate->ref());
    }

    if (curAscender > maxAscender) {
      // 현재 라인에서 폰트가 바뀌거나, runDelegate 가 존재시 최대값을 저장시킴(추후 baseline 계산에 사용됨)
      maxAscender = curAscender;
    }
    if (curDescender < maxDescender) {
      maxDescender = curDescender;
    }

    // 런에 _attributedString의 모든 속성을 추가함
    // 위에서 font substitution 또는 필수 속성 추가등의 이유로 원본(_attributedString) 속성에서 수정될 수 있으므로
    // 런에 이미 동일한 키가 존재하면 해당 키는 추가하지 않음(@@이런 경우는 테스트 필요)
    run->getAttributes().addMulitiple(attribs, false);

    run->_range.location = runRange.location;
    run->_range.length = runRange.length; // 뒤에 개행문자는 빼고 계산되고 있다???
    // _stringIndices 에 문자 인덱스를 저장중인데 _stringFragment 가 필요한가???
    run->_stringFragment = attributedString()->getString()->substring(runRange);

    // 현재 런내의 글리프들의 위치
    run->_positions.assign(glyphOrigins.begin() + glyphIdx, glyphOrigins.begin() + glyphIdx + runRange.length);
    // 현재 런내의 글리프들의 크기
    run->_advances.assign(glyphAdvances.begin() + glyphIdx, glyphAdvances.begin() + glyphIdx + runRange.length);
    // 현재 런내의 문자들. characters는 run->_stringFragment와 달리 trailing whitespace가 제거돼 있고, 개행문자도 0으로 치환돼 있다.
    run->_glyphs.assign(glyphCharacters.begin() + glyphIdx, glyphCharacters.begin() + glyphIdx + runRange.length);
    // 문자 인덱스 저장(@@todo 문자2개가 1개의 글리프로 변환된 경우등도 처리해야함)
    //run->_stringIndices.assign(_characters.begin() + glyphIdx, _characters.begin() + glyphIdx + runRange.length);
    run->_stringIndices.assign(stringIndices.begin() + glyphIdx, stringIndices.begin() + glyphIdx + runRange.length);

    // 개행문자 대신 삽입된 문자들 삭제(현재는 $로 삽입중이며, 굳이 삭제하지 않게함)
    // 개행문자의 경우 위에서 0 으로 넣고 있지만(characters.push_back((wchar_t)0);) 이후 trailing whitespace가 제거되면서 같이 삭제중이다.
    //std::vector<wchar_t>::iterator testCharIter = std::find(run->_glyphCharacters.begin(), run->_glyphCharacters.end(), 0);
    //if (testCharIter != run->_glyphCharacters.end()) {
    //  SR_ASSERT(0); // 개행문자 1개인 경우 걸림
    //}
    //run->_glyphCharacters.erase(std::remove(run->_glyphCharacters.begin(), run->_glyphCharacters.end(), 0), run->_glyphCharacters.end());
    //if (!(run->_glyphOrigins.size() == run->_glyphAdvances.size() == run->_glyphCharacters.size())) {
    //  SR_ASSERT(0); // 개행문자 1개인 경우 발생되고 있음
    //  int a = 0;
    //}

    glyphIdx += runRange.length;
    outLine->_runs.push_back(run); // 생성한 런 객체를 라인에 추가

    // 각 런의 폭을 outLine->_width 에 더한다.
    for_each(run->_advances.begin(), run->_advances.end(), [&outLine](SRSize sz) {
      outLine->_width += sz.width;
    });
    curIdx = curRange.location + curRange.length; // 다음번 속성 위치로 이동
  }

  // tailing white space 폭을 저장함(디버깅용)
  for (SRIndex i = glyphCharacters.size() - 1; i > 0; --i) {
    wchar_t ch = glyphCharacters[i];
    if (_isWhiteSpace(ch)) {
      outLine->_trailingWhitespaceWidth += glyphAdvances[i].width;
    }
  }

  // 현재 라인내의 최대 ascender 등을 저장한다(라인 높이 계산에 사용됨)
  outLine->_ascent = maxAscender;
  outLine->_descent = maxDescender;
  outLine->_leading = maxLeading;
  outLine->_strRange = lineRange; // 라인 문자열 범위(전달인자 그대로 임)
  return outLine->_width;
}

// 현재 라인 폭에 포함된 마지막 문자 다음 문자 인덱스를 리턴시킴
SRIndex CTTypesetter::doWrap(SRRange range, SRFloat requestedLineWidth, CTLinePtr outLine) {
  if (range.length == SRNotFound) {
		range.length = charactersLen() - range.location;
	}

	// Measure the string
  const wchar_t* chars = attributedString()->getString()->c_str();
	SRIndex curIndex = range.location;
  SRIndex endIndex = range.rangeMax();
	bool hitLinebreak = false; // 개행이 됐는지 여부

  std::vector<SRPoint> glyphOrigins;
  std::vector<SRSize> glyphAdvances;
  std::vector<wchar_t> glyphCharacters;
  std::vector<SRIndex> stringIndices;

  SRInt penX = 0;
  const SRIndex lineStartIndex = curIndex;
  SRIndex lastWhiteSpaceIndex = SRNotFound; // for wordwrap

	SRRange curAttributeRange = { 0 };
	SRFloat curFontHeight = 0.0f;
  const SRFloat maxWidth = requestedLineWidth;

  UIFontPtr curFont = nullptr; // 현재 파싱중인 위치의 폰트
  SRFloat fontHeight = 0.0f;

	// Lookup each glyph
  while (curIndex < endIndex) {
    glyphOrigins.push_back(SRPoint(penX, 0.0f)); // 글리프 위치
    glyphAdvances.push_back(SRSize(0, 0)); // 글리프 크기

    SRInt curChar = chars[curIndex];

		if (curChar == '\t') {
			curChar = ' '; // tab 출력이 안될 경우에만 필요함
		}
    if (curChar == ' ') {
      lastWhiteSpaceIndex = curIndex;
    }

    if (curChar == LINE_FEED_CHAR || curChar == CARRIAGE_RETURN_CHAR) {
      glyphCharacters.push_back((wchar_t)_new_line_glyph);
    } else {
      if (curChar == ' ') {
        glyphCharacters.push_back((wchar_t)' '); // '`'
      } else {
        glyphCharacters.push_back((wchar_t)curChar);
      }
    }
    stringIndices.push_back(curIndex);

		auto& glyphAdvance = glyphAdvances.back(); // 현재 문자의 글리프 폭을 저장

    // 개행문자 처리, 현재 문자가 LINE_FEED_CHAR(\n) 이거나 바로 이전문자가 CARRIAGE_RETURN_CHAR(\r) 인 경우, 개행처리함
    if ((chars[curIndex] == LINE_FEED_CHAR) || (curIndex > lineStartIndex && chars[curIndex - 1] == CARRIAGE_RETURN_CHAR)) {
			if ((curIndex > 0 && chars[curIndex - 1] == CARRIAGE_RETURN_CHAR && chars[curIndex] != LINE_FEED_CHAR)) {
        // \r\n 이 아니라, \rx 같은 경우 이전 \r 위치로 이동
        SR_ASSERT(0); // 이런 경우가 존재하나??? 테스트 필요!!!
				--curIndex;
			}
			// We have hit a hard linebreak, consume it
			//++curIndex;
			// Do a hard line break
			hitLinebreak = true;
			//break; // 개행문자도 폭을 저장시키기 위해 계속 진행시킴
		}

    SRInt advanceX = 0;

    // 새로운 속성을 획득한 경우임. curIndex < curAttributeRange.location 조건은 RTL 일 경우를 위해 필요할까???
    SR_ASSERT(curIndex >= curAttributeRange.location);

    // Have we reached a new attribute range?
		if (curIndex < curAttributeRange.location || curIndex >= (curAttributeRange.location + curAttributeRange.length)) {
      // 새로운 attribute range 일 경우, 해당 폰트 적용
			// Grab and set the new font
      SRObjectDictionaryPtr attribs = attributedString()->attributes(curIndex, &curAttributeRange);
      
      // kCTRunDelegateAttributeName 확인
      const CTRunDelegatePtr runDelegate = std::dynamic_pointer_cast<CTRunDelegate>(attribs->valueForKey(kCTRunDelegateAttributeName));
      if (runDelegate != nullptr) {
        // kCTRunDelegateAttributeName 속성 존재시 크기를 얻어옴
        fontHeight = runDelegate->cb().getAscent(runDelegate->ref());
        fontHeight += runDelegate->cb().getDescent(runDelegate->ref());
        curFont = nullptr;
        advanceX = runDelegate->cb().getWidth(runDelegate->ref()); // 현재 문자 폭
      } else {
        // kCTRunDelegateAttributeName 속성 부재시 폰트 크기를 얻어옴
        UIFontPtr font = std::dynamic_pointer_cast<UIFont>(attribs->valueForKey(kCTFontAttributeName));
        if (font == nullptr) {
          font = UIFont::systemFont();
        }
        // 폰트 높이를 획득한다.
        fontHeight = font->lineHeight();
        curFont = font;
      }
      //SRFloat curX = penX;
      // 현재 x 위치와 폰트높이로 레이아웃 가능한 남은 폭을 얻어온다.
      //SRFloat width = widthFunc(widthParam, curIndex, curX, fontHeight); // 속성이 변경될 경우에만 남은 폭을 얻어오고 있음
      //maxWidth = curX + width;
    }

    // 현재 문자의 폭을 얻어온다.
    if (curFont) {
      advanceX = curFont->advancementForGlyph(curChar); // 개행문자(\n)는 0 임
    } else {
      // advance_x는 이미 설정돼 있어야 한다.
      SR_ASSERT(advanceX > 0);
    }

    penX += advanceX;
		glyphAdvance.width = advanceX;
    glyphAdvance.height = fontHeight; // 글자 높이는 폰트 높이로 추가함

    if (hitLinebreak) {
      // 개행문자가 있었던 경우
      //penX -= advance_x; // 개행문자 폭은 제외시킴
      ++curIndex; // 개행문자까지 현재 라인에 포함시킨다. 개행문자는 폭을 넘어서도 포함시킴.
      break;
    }

    if (penX > maxWidth) {
      if (lastWhiteSpaceIndex != SRNotFound) { // unsigned int
        // 공백문자가 있었으면 마지막 공백문자까지 현재 라인에 위치시킴(라인의 폭은 아래 fillLine() 함수에서 설정함)
        curIndex = ++lastWhiteSpaceIndex;
        while (curIndex < endIndex) {
          if (_isWhiteSpace(chars[curIndex])) ++curIndex; // consume white spaces
          else break;
        }
      } else if (curIndex > lineStartIndex) {
        // 현재 라인에 1 문자 이상 존재하고, 현재문자 폭이 전체 너비를 넘어설 경우 이전문자 위치로 이동
        penX -= advanceX;
      } else {
        // 현재 라인에 1 문자도 못들어가는 경우, 강제로 1문자를 넣게함
        ++curIndex;
      }
      hitLinebreak = true;
      break;
    } else {
      // 레이아웃할 폭이 남은 경우, 다음 문자 위치로 이동
      ++curIndex;
    }
	} // end of while (curIndex < endIndex)

  // 개행없이 현재 라인의 폭에 다 들어가는 경우
	if (curIndex >= endIndex && !hitLinebreak) {
    //SR_ASSERT(0);
	}

  // outLine 인자가 NULL 이 아닌 경우, outLine 에 라인정보와 Run을 추가해서 리턴해준다.
  if (outLine) {
    // 1 개 라인 설정
    SRRange lineRange(lineStartIndex, curIndex - lineStartIndex);
    SRFloat lineWidth = fillLine(lineRange, glyphOrigins, glyphAdvances, glyphCharacters, stringIndices, outLine);
    if (lineWidth != penX) {
      //SR_ASSERT(0);
      int a = 0;
    }
	}

	return curIndex; // 현재 라인 폭에 포함된 마지막 문자 다음 문자 인덱스를 리턴
}

// width 폭 안에 레이아웃 가능한 최대 문자열 인덱스를 리턴
CTLinePtr CTTypesetter::suggestLineBreak(SRRange range, double width) {
  CTLinePtr ret = CTLine::create();
  doWrap(range, width, ret);
  return ret;
}

// stringRange 범위만큼 레이아웃된 라인 객체를 리턴
// 이미 suggestLineBreak()을 통해 문자열 범위를 획득 후 호출되고 있음
CTLinePtr CTTypesetter::createLine(SRRange stringRange) {
  CTLinePtr ret = CTLine::create();
  doWrap(stringRange, FLT_MAX, ret);
  return ret;
}

} // namespace sr
