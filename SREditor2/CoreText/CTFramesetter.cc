#include "CTFramesetter.h"
#include "CTFrame.h"
#include "CTParagraphStyle.h"
#include "../Foundation/SRTextContainer.h"
#include <algorithm> // for_each

namespace sr {

CTFramesetter::CTFramesetter(const SRAttributedStringPtr string) {
  _typesetter = CTTypesetter::create(string);
}

// frameRect�� tc ������������ ��ǥ��(e.g, ��Ʈ���� ���� ����� �������� ���۵ȴ�(e.g, (10,10 320-����)))
CTFramePtr CTFramesetter::createFrame(SRRange stringRange, const SRPath frameRect, const SRTextContainerPtr tc) {
  CTFramePtr ret = CTFrame::create();
  ret->_frameRect = { {0, 0}, frameRect.size };

  // �ϳ��� ���ܾ� ȣ����� ������
  const SRAttributedStringPtr astr = _typesetter->attributedString();
  {
    std::wstring str = astr->getString()->data();
    SRIndex lastIndex = stringRange.location + stringRange.length - 1;
    if (str.find(L"\n", stringRange.location) != lastIndex) {
      // \n ���� ��ġ�� ���޹��� ������ ������ ���ڰ� �ƴ� ���, ���� ��Ȳ
      SR_ASSERT(0);
    }
  }

  // Only fill in frame if there is text
  SRSize sizeOut(0, 0);
  SRSize sizeWithoutTailWhiteSpace(0, 0);

  if (_typesetter->attributedString()->getLength() > 0) {
    // Paragraph settings are expected at effective range 0
    SRObjectDictionaryPtr attributes = std::dynamic_pointer_cast<SRObjectDictionary>(
      _typesetter->attributedString()->attributes(stringRange.location, nullptr));
    CTParagraphStylePtr style = std::dynamic_pointer_cast<CTParagraphStyle>(
      attributes->valueForKey(kCTParagraphStyleAttributeName));
    CTTextAlignment alignment = kCTLeftTextAlignment;
    SR_ASSERT(style); // ���ܼӼ��� �ʼ�!
    if (style != nullptr) {
      if (!style->getValueForSpecifier(kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &alignment)) {
        // No alignment found, use default of left alignment
        alignment = kCTLeftTextAlignment;
      }
    }

    SRIndex curIdx = stringRange.location;
    if (stringRange.length <= 0) {
      const SRStringPtr string = _typesetter->attributedString()->getString();
      stringRange.length = string->length() - stringRange.location;
    }

    const SRIndex endIdx = stringRange.rangeMax();
    SRPoint layoutPosXY; // = frameRect.origin; // (0, 0) ��ġ���� ���̾ƿ�

    for (;;) {
      if (curIdx >= endIdx) {
        SR_ASSERT(curIdx == endIdx);
        break; // ��ü���� ���̾ƿ� �Ϸ�(curIdx�� ���� �� ���̾ƿ��� ��ġ�̹Ƿ� endIdx�� �������� �ִ�)
      }

      // ���޹��� ���� ��ü�κ��� 1�� ���� ������ ȹ��
      CTLinePtr line = _typesetter->suggestLineBreak(SRRange(curIdx, endIdx - curIdx), frameRect.width() - layoutPosXY.x);
      if (!line) {
        SR_ASSERT(0); // �� ���ڵ� ���̾ƿ����� ���� ���� ����߰ڴ�(���� ���Ƶ� 1 ���ڴ� ���̾ƿ���Ű�� ����)
        break;
      }
      SRIndex pos = curIdx + line->getStringRange().length;
      SRFloat lineHeight = line->getLineSize().height;
      // ���� �����̳ʿ� ���� ������ ����
      //SRRect proposedRect(layoutPosXY, SRSize(frameRect.right() - layoutPosXY.x, lineHeight));
      SRRect proposedRect(layoutPosXY, SRSize(frameRect.width() - layoutPosXY.x, lineHeight));
      if (proposedRect.width() < 0) {
        //SR_ASSERT(0);
        proposedRect.size.width = 0;
      }
      SRRect remainingRect;
      proposedRect.origin += frameRect.origin; // lineFragmentRect ������ �����̳ʻ��� ��ǥ�� �ʿ��ϹǷ� �߰���
      SRRect lineRect = tc->lineFragmentRect(proposedRect, 0, SRWritingDirectionLeftToRight, &remainingRect);
      if (lineRect != proposedRect) {
        remainingRect.origin.x -= frameRect.origin.x; // �����̳ʻ��� ��ǥ -> layoutRect ��ǥ
        lineRect.origin.x -= frameRect.origin.x;

        line = _typesetter->suggestLineBreak(SRRange(curIdx, endIdx - curIdx), lineRect.size.width);
        if (!line) {
          SR_ASSERT(0); // �� ���ڵ� ���̾ƿ����� ���� ���� ����߰ڴ�(���� ���Ƶ� 1 ���ڴ� ���̾ƿ���Ű�� ����)
          break;
        }
        pos = curIdx + line->getStringRange().length;

        if (line->getStringRange().length == 1 && line->getLineSize().width > lineRect.size.width) {
          // ���̾ƿ� ����� 1���ڵ� ���Ե��� ���ϴ� ���(��û�� ���� �Ѵ� ���) ���� ��ġ�� �̵�
          if (remainingRect.size.width > 0) {
            // �����̳��� hole�� �ǳʶ�
            layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          } else {
            // ����ó��
            if (ret->getLines().size() == 0) {
              // ù ������ ���, ���� ������ ���̸�ŭ �̵��ϸ�, ù ������ y ��ǥ�� �ʹ� Ŀ���� �����Ƿ�
              // ������ �� ��ŭ�� y ��ǥ�� �ø��� ��(e.g, wrapping image ����� �̹��� �ϴܰ� ���� �ؽ�Ʈ���ΰ� ������ �ʹ� �аԵ�)
              layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight / 2 };
              //layoutPosXY = { 0.0f, layoutPosXY.y + 5 }; // 5 pixel �� ���� ���̸� �÷����鼭 1 �����̻� ���ԵǴ��� üũ��
            } else {
              // ���� ������ �����ϸ�, ���� ���� ���̸�ŭ �̵��ؾ� ��Ȯ�ϴ�.
              layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
            }
          }
          continue;
        }
      }

      const SRFloat lineWidth = line->getLineSize().width;
      lineHeight = line->getLineSize().height;

      if ((sizeOut.height + lineHeight) > frameRect.height()) {
        // ���޹��� ������ ���̸� �ʰ��ϴ� ���, ���̾ƿ� �ߴ�
        SR_ASSERT(0); // just for check
        break;
      }

      if (ret) {
        SRPoint lineOrigin = layoutPosXY;
        switch (alignment) {
        case kCTRightTextAlignment:
          lineOrigin.x += lineRect.size.width - lineWidth;
          break;
        case kCTCenterTextAlignment:
          lineOrigin.x += (lineRect.size.width - lineWidth) / 2;
          break;
        default: // kCTLeftTextAlignment
          lineOrigin.x += 0.0f;
          break;
        }
        if (lineOrigin.x < 0) {
          //SR_ASSERT(0);
          lineOrigin.x = 0; // TODO 2�� �߾� ���Ľ� �߻��ǰ� ������, ���κм� �ʿ���
        }
#if 1
        SRBool isLayoutedSameLine = false;
        if (ret->_lineOrigins.size() > 0) {
          const SRPoint lastLineOrg = ret->_lineOrigins.back();
          if (lastLineOrg.y == layoutPosXY.y)
            isLayoutedSameLine = true;
        }
        // fitFrame �� ����ִ� ����(1��)�� ret �� ����.
        if (isLayoutedSameLine) {
          // ������ ���̾ƿ��� ���ΰ� ���� ������ ���, ������ ���ο� �߰���Ŵ
          CTLinePtr lastLine = ret->_lines.back(); // ���� ������ ����
          CTLinePtr appendLine = line; // �߰��Ǵ� ����
          lastLine->_width = (layoutPosXY.x + lineWidth);
          if (lastLine->_ascent != appendLine->_ascent || lastLine->_descent != appendLine->_descent
            || lastLine->_leading != appendLine->_leading) {
            //SR_ASSERT(0);
            int a = 0;
          }
          //lastLine->_trailingWhitespaceWidth += appendLine->_trailingWhitespaceWidth;
          lastLine->_trailingWhitespaceWidth = appendLine->_trailingWhitespaceWidth; // ������ ���鹮�� ���̸� ����
          lastLine->_strRange.length += appendLine->_strRange.length;

          const SRPoint lastLineOrg = ret->_lineOrigins.back();
          const SRFloat appendRunPosX = layoutPosXY.x - lastLineOrg.x; // �߰��Ǵ� ������ �� ���� ��ġ
          for_each(appendLine->_runs.begin(), appendLine->_runs.end(), [&appendRunPosX](CTRunPtr run) {
            for_each(run->_positions.begin(), run->_positions.end(), [&appendRunPosX](SRPoint& pt) {
              pt.x += appendRunPosX;
            });
          });
          lastLine->_runs.insert(lastLine->_runs.end(), appendLine->_runs.begin(), appendLine->_runs.end());
        } else {
          ret->_lines.push_back(line);
          ret->_lineOrigins.push_back(lineOrigin);
        }
#else
        // center hole�� ������ ����� ���� �ҽ�
        ret->_lines.push_back(line);
        ret->_lineOrigins.push_back(lineOrigin);
#endif
      }

      curIdx = pos;
      if ((layoutPosXY.x + lineWidth) > sizeOut.width) {
        sizeOut.width = layoutPosXY.x + lineWidth;
      }
      if ((lineWidth - line->_trailingWhitespaceWidth) > sizeWithoutTailWhiteSpace.width) {
        // ���� ���� ���鹮�ڸ� ������ ���� �����Ŵ
        sizeWithoutTailWhiteSpace.width = (lineWidth - line->_trailingWhitespaceWidth);
      }

      wchar_t lastChar = line->getGlyphRuns().back()->getStringFragment().data().back();
      if (remainingRect.size.width > 0 && lastChar != '\n') {
        // ���๮�ڷ� ������ �ʰ� ���� ������ �ִ� ���
        if (curIdx == endIdx) {
          // ���ʿ� ������ ������(simpleRectFrame)�� ���� ������ ���� ���� �� ��ġ���� ���̾ƿ���Ŵ
          layoutPosXY.x += lineWidth; // ���̾ƿ��� ����(proposedRect)�� ������ ���� ������ �� �����Ƿ� fitSize��ŭ�� �߰���Ŵ
        } else {
          // �����̳��� hole�� �ǳʶ�
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
        }
      } else {
        SRFloat leftWidth = lineRect.size.width - lineWidth;
        if (false && leftWidth > 0 && lastChar != '\n') {
          // ���� ���� ������ �ִ� ���, ����ؼ� ���� ������ ���̾ƿ��ϰ� ��
          // �̷��� ó���ϸ�, ���� ���̾ƿ��� ���α��̰� �����Ǹ鼭 ª�� ���̾ƿ��Ǵ� ������ �������� ���左�� �ȵǰ� �ȴ�.
          layoutPosXY.x += lineWidth;
        } else {
          // ���๮�ڷ� �����ų� ���� ������ ���� ���, ����ó��
          layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
        }
      }
      sizeOut.height = layoutPosXY.y; // ���̾ƿ��� ���� ����
    }
  }

  if (ret) {
    ret->_frameRect.size = sizeOut; // ���� �����ϴ� �������� ������
    sizeWithoutTailWhiteSpace.height = sizeOut.height;
    ret->_frameRectWithoutTailingWhiteSpace.size = sizeWithoutTailWhiteSpace;
  }

  return ret;
}

SRSize CTFramesetter::suggestFrameSizeWithConstraints(SRRange stringRange, SRObjectDictionaryPtr frameAttributes
  , SRSize constraints, SRRange* fitRange) {
  SR_NOT_IMPL();
  SRRect frameSize;
  frameSize.size = constraints;

  CTFramePtr frame = createFrame(stringRange, frameSize);
  SRSize ret = frame != nullptr ? frame->_frameRect.size : SRSize(0, 0);

  if (fitRange) {
    *fitRange = frame->getVisibleStringRange();
  }
  return ret;
}

} // namespace sr
