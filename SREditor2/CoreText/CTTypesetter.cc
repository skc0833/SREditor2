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
    // �̹� delete �� �����Ϳ��� ��� �����Ѵ�.
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
      // ���� �Ӽ� ���� ���� ������ �Ѿ�� ���, ���� ���� �������� ���ѽ�Ŵ
      // ���� �Ӽ��� ���� ���ο� ���� �� ��� �߻���
      runRange.length = lineRange.location + lineRange.length - runRange.location;
    }

    CTRunPtr run = CTRun::create();
    UIFontPtr runFont = std::dynamic_pointer_cast<UIFont>(attribs->valueForKey(kCTFontAttributeName));
    if (runFont == nullptr) {
      SR_ASSERT(0); // ��Ʈ�� ���� ���� ����߰ڴ�.
      runFont = UIFont::systemFont();
      run->getAttributes().setValue(runFont, kCTFontAttributeName); // ��Ʈ�� ���� ���, ����Ʈ ��Ʈ�� �߰���
    }

    // kCTRunDelegateAttributeName Ȯ��
    CTRunDelegatePtr runDelegate = std::dynamic_pointer_cast<CTRunDelegate>(attribs->valueForKey(kCTRunDelegateAttributeName));
    SRFloat curAscender = runFont->ascender();
    SRFloat curDescender = runFont->descender();

    if (runDelegate != nullptr) {
      curAscender = runDelegate->cb().getAscent(runDelegate->ref());
      curDescender = runDelegate->cb().getDescent(runDelegate->ref());
    }

    if (curAscender > maxAscender) {
      // ���� ���ο��� ��Ʈ�� �ٲ�ų�, runDelegate �� ����� �ִ밪�� �����Ŵ(���� baseline ��꿡 ����)
      maxAscender = curAscender;
    }
    if (curDescender < maxDescender) {
      maxDescender = curDescender;
    }

    // ���� _attributedString�� ��� �Ӽ��� �߰���
    // ������ font substitution �Ǵ� �ʼ� �Ӽ� �߰����� ������ ����(_attributedString) �Ӽ����� ������ �� �����Ƿ�
    // ���� �̹� ������ Ű�� �����ϸ� �ش� Ű�� �߰����� ����(@@�̷� ���� �׽�Ʈ �ʿ�)
    run->getAttributes().addMulitiple(attribs, false);

    run->_range.location = runRange.location;
    run->_range.length = runRange.length; // �ڿ� ���๮�ڴ� ���� ���ǰ� �ִ�???
    // _stringIndices �� ���� �ε����� �������ε� _stringFragment �� �ʿ��Ѱ�???
    run->_stringFragment = attributedString()->getString()->substring(runRange);

    // ���� ������ �۸������� ��ġ
    run->_positions.assign(glyphOrigins.begin() + glyphIdx, glyphOrigins.begin() + glyphIdx + runRange.length);
    // ���� ������ �۸������� ũ��
    run->_advances.assign(glyphAdvances.begin() + glyphIdx, glyphAdvances.begin() + glyphIdx + runRange.length);
    // ���� ������ ���ڵ�. characters�� run->_stringFragment�� �޸� trailing whitespace�� ���ŵ� �ְ�, ���๮�ڵ� 0���� ġȯ�� �ִ�.
    run->_glyphs.assign(glyphCharacters.begin() + glyphIdx, glyphCharacters.begin() + glyphIdx + runRange.length);
    // ���� �ε��� ����(@@todo ����2���� 1���� �۸����� ��ȯ�� ��� ó���ؾ���)
    //run->_stringIndices.assign(_characters.begin() + glyphIdx, _characters.begin() + glyphIdx + runRange.length);
    run->_stringIndices.assign(stringIndices.begin() + glyphIdx, stringIndices.begin() + glyphIdx + runRange.length);

    // ���๮�� ��� ���Ե� ���ڵ� ����(����� $�� �������̸�, ���� �������� �ʰ���)
    // ���๮���� ��� ������ 0 ���� �ְ� ������(characters.push_back((wchar_t)0);) ���� trailing whitespace�� ���ŵǸ鼭 ���� �������̴�.
    //std::vector<wchar_t>::iterator testCharIter = std::find(run->_glyphCharacters.begin(), run->_glyphCharacters.end(), 0);
    //if (testCharIter != run->_glyphCharacters.end()) {
    //  SR_ASSERT(0); // ���๮�� 1���� ��� �ɸ�
    //}
    //run->_glyphCharacters.erase(std::remove(run->_glyphCharacters.begin(), run->_glyphCharacters.end(), 0), run->_glyphCharacters.end());
    //if (!(run->_glyphOrigins.size() == run->_glyphAdvances.size() == run->_glyphCharacters.size())) {
    //  SR_ASSERT(0); // ���๮�� 1���� ��� �߻��ǰ� ����
    //  int a = 0;
    //}

    glyphIdx += runRange.length;
    outLine->_runs.push_back(run); // ������ �� ��ü�� ���ο� �߰�

    // �� ���� ���� outLine->_width �� ���Ѵ�.
    for_each(run->_advances.begin(), run->_advances.end(), [&outLine](SRSize sz) {
      outLine->_width += sz.width;
    });
    curIdx = curRange.location + curRange.length; // ������ �Ӽ� ��ġ�� �̵�
  }

  // tailing white space ���� ������(������)
  for (SRIndex i = glyphCharacters.size() - 1; i > 0; --i) {
    wchar_t ch = glyphCharacters[i];
    if (_isWhiteSpace(ch)) {
      outLine->_trailingWhitespaceWidth += glyphAdvances[i].width;
    }
  }

  // ���� ���γ��� �ִ� ascender ���� �����Ѵ�(���� ���� ��꿡 ����)
  outLine->_ascent = maxAscender;
  outLine->_descent = maxDescender;
  outLine->_leading = maxLeading;
  outLine->_strRange = lineRange; // ���� ���ڿ� ����(�������� �״�� ��)
  return outLine->_width;
}

// ���� ���� ���� ���Ե� ������ ���� ���� ���� �ε����� ���Ͻ�Ŵ
SRIndex CTTypesetter::doWrap(SRRange range, SRFloat requestedLineWidth, CTLinePtr outLine) {
  if (range.length == SRNotFound) {
		range.length = charactersLen() - range.location;
	}

	// Measure the string
  const wchar_t* chars = attributedString()->getString()->c_str();
	SRIndex curIndex = range.location;
  SRIndex endIndex = range.rangeMax();
	bool hitLinebreak = false; // ������ �ƴ��� ����

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

  UIFontPtr curFont = nullptr; // ���� �Ľ����� ��ġ�� ��Ʈ
  SRFloat fontHeight = 0.0f;

	// Lookup each glyph
  while (curIndex < endIndex) {
    glyphOrigins.push_back(SRPoint(penX, 0.0f)); // �۸��� ��ġ
    glyphAdvances.push_back(SRSize(0, 0)); // �۸��� ũ��

    SRInt curChar = chars[curIndex];

		if (curChar == '\t') {
			curChar = ' '; // tab ����� �ȵ� ��쿡�� �ʿ���
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

		auto& glyphAdvance = glyphAdvances.back(); // ���� ������ �۸��� ���� ����

    // ���๮�� ó��, ���� ���ڰ� LINE_FEED_CHAR(\n) �̰ų� �ٷ� �������ڰ� CARRIAGE_RETURN_CHAR(\r) �� ���, ����ó����
    if ((chars[curIndex] == LINE_FEED_CHAR) || (curIndex > lineStartIndex && chars[curIndex - 1] == CARRIAGE_RETURN_CHAR)) {
			if ((curIndex > 0 && chars[curIndex - 1] == CARRIAGE_RETURN_CHAR && chars[curIndex] != LINE_FEED_CHAR)) {
        // \r\n �� �ƴ϶�, \rx ���� ��� ���� \r ��ġ�� �̵�
        SR_ASSERT(0); // �̷� ��찡 �����ϳ�??? �׽�Ʈ �ʿ�!!!
				--curIndex;
			}
			// We have hit a hard linebreak, consume it
			//++curIndex;
			// Do a hard line break
			hitLinebreak = true;
			//break; // ���๮�ڵ� ���� �����Ű�� ���� ��� �����Ŵ
		}

    SRInt advanceX = 0;

    // ���ο� �Ӽ��� ȹ���� �����. curIndex < curAttributeRange.location ������ RTL �� ��츦 ���� �ʿ��ұ�???
    SR_ASSERT(curIndex >= curAttributeRange.location);

    // Have we reached a new attribute range?
		if (curIndex < curAttributeRange.location || curIndex >= (curAttributeRange.location + curAttributeRange.length)) {
      // ���ο� attribute range �� ���, �ش� ��Ʈ ����
			// Grab and set the new font
      SRObjectDictionaryPtr attribs = attributedString()->attributes(curIndex, &curAttributeRange);
      
      // kCTRunDelegateAttributeName Ȯ��
      const CTRunDelegatePtr runDelegate = std::dynamic_pointer_cast<CTRunDelegate>(attribs->valueForKey(kCTRunDelegateAttributeName));
      if (runDelegate != nullptr) {
        // kCTRunDelegateAttributeName �Ӽ� ����� ũ�⸦ ����
        fontHeight = runDelegate->cb().getAscent(runDelegate->ref());
        fontHeight += runDelegate->cb().getDescent(runDelegate->ref());
        curFont = nullptr;
        advanceX = runDelegate->cb().getWidth(runDelegate->ref()); // ���� ���� ��
      } else {
        // kCTRunDelegateAttributeName �Ӽ� ����� ��Ʈ ũ�⸦ ����
        UIFontPtr font = std::dynamic_pointer_cast<UIFont>(attribs->valueForKey(kCTFontAttributeName));
        if (font == nullptr) {
          font = UIFont::systemFont();
        }
        // ��Ʈ ���̸� ȹ���Ѵ�.
        fontHeight = font->lineHeight();
        curFont = font;
      }
      //SRFloat curX = penX;
      // ���� x ��ġ�� ��Ʈ���̷� ���̾ƿ� ������ ���� ���� ���´�.
      //SRFloat width = widthFunc(widthParam, curIndex, curX, fontHeight); // �Ӽ��� ����� ��쿡�� ���� ���� ������ ����
      //maxWidth = curX + width;
    }

    // ���� ������ ���� ���´�.
    if (curFont) {
      advanceX = curFont->advancementForGlyph(curChar); // ���๮��(\n)�� 0 ��
    } else {
      // advance_x�� �̹� ������ �־�� �Ѵ�.
      SR_ASSERT(advanceX > 0);
    }

    penX += advanceX;
		glyphAdvance.width = advanceX;
    glyphAdvance.height = fontHeight; // ���� ���̴� ��Ʈ ���̷� �߰���

    if (hitLinebreak) {
      // ���๮�ڰ� �־��� ���
      //penX -= advance_x; // ���๮�� ���� ���ܽ�Ŵ
      ++curIndex; // ���๮�ڱ��� ���� ���ο� ���Խ�Ų��. ���๮�ڴ� ���� �Ѿ�� ���Խ�Ŵ.
      break;
    }

    if (penX > maxWidth) {
      if (lastWhiteSpaceIndex != SRNotFound) { // unsigned int
        // ���鹮�ڰ� �־����� ������ ���鹮�ڱ��� ���� ���ο� ��ġ��Ŵ(������ ���� �Ʒ� fillLine() �Լ����� ������)
        curIndex = ++lastWhiteSpaceIndex;
        while (curIndex < endIndex) {
          if (_isWhiteSpace(chars[curIndex])) ++curIndex; // consume white spaces
          else break;
        }
      } else if (curIndex > lineStartIndex) {
        // ���� ���ο� 1 ���� �̻� �����ϰ�, ���繮�� ���� ��ü �ʺ� �Ѿ ��� �������� ��ġ�� �̵�
        penX -= advanceX;
      } else {
        // ���� ���ο� 1 ���ڵ� ������ ���, ������ 1���ڸ� �ְ���
        ++curIndex;
      }
      hitLinebreak = true;
      break;
    } else {
      // ���̾ƿ��� ���� ���� ���, ���� ���� ��ġ�� �̵�
      ++curIndex;
    }
	} // end of while (curIndex < endIndex)

  // ������� ���� ������ ���� �� ���� ���
	if (curIndex >= endIndex && !hitLinebreak) {
    //SR_ASSERT(0);
	}

  // outLine ���ڰ� NULL �� �ƴ� ���, outLine �� ���������� Run�� �߰��ؼ� �������ش�.
  if (outLine) {
    // 1 �� ���� ����
    SRRange lineRange(lineStartIndex, curIndex - lineStartIndex);
    SRFloat lineWidth = fillLine(lineRange, glyphOrigins, glyphAdvances, glyphCharacters, stringIndices, outLine);
    if (lineWidth != penX) {
      //SR_ASSERT(0);
      int a = 0;
    }
	}

	return curIndex; // ���� ���� ���� ���Ե� ������ ���� ���� ���� �ε����� ����
}

// width �� �ȿ� ���̾ƿ� ������ �ִ� ���ڿ� �ε����� ����
CTLinePtr CTTypesetter::suggestLineBreak(SRRange range, double width) {
  CTLinePtr ret = CTLine::create();
  doWrap(range, width, ret);
  return ret;
}

// stringRange ������ŭ ���̾ƿ��� ���� ��ü�� ����
// �̹� suggestLineBreak()�� ���� ���ڿ� ������ ȹ�� �� ȣ��ǰ� ����
CTLinePtr CTTypesetter::createLine(SRRange stringRange) {
  CTLinePtr ret = CTLine::create();
  doWrap(stringRange, FLT_MAX, ret);
  return ret;
}

} // namespace sr
