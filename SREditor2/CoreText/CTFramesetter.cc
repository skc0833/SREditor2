#include "CTFramesetter.h"
#include "CTFrame.h"
#include "CTParagraphStyle.h"
#include "../Foundation/SRTextContainer.h"
#include <algorithm> // for_each

namespace sr {

CTFramesetter::CTFramesetter(const SRAttributedStringPtr string) {
  _typesetter = CTTypesetter::create(string);
}

// frameRect는 tc 영역내에서의 좌표임(e.g, 루트블럭의 마진 존재시 마진부터 시작된다(e.g, (10,10 320-무한)))
CTFramePtr CTFramesetter::createFrame(SRRange stringRange, const SRPath frameRect, const SRTextContainerPtr tc) {
  CTFramePtr ret = CTFrame::create();
  ret->_frameRect = { {0, 0}, frameRect.size };

  // 하나의 문단씩 호출됨을 가정함
  const SRAttributedStringPtr astr = _typesetter->attributedString();
  {
    std::wstring str = astr->getString()->data();
    SRIndex lastIndex = stringRange.location + stringRange.length - 1;
    if (str.find(L"\n", stringRange.location) != lastIndex) {
      // \n 문자 위치가 전달받은 범위의 마지막 문자가 아닌 경우, 에러 상황
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
    SR_ASSERT(style); // 문단속성은 필수!
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
    SRPoint layoutPosXY; // = frameRect.origin; // (0, 0) 위치부터 레이아웃

    for (;;) {
      if (curIdx >= endIdx) {
        SR_ASSERT(curIdx == endIdx);
        break; // 전체범위 레이아웃 완료(curIdx는 다음 번 레이아웃할 위치이므로 endIdx와 같아질수 있다)
      }

      // 전달받은 범위 전체로부터 1개 라인 범위를 획득
      CTLinePtr line = _typesetter->suggestLineBreak(SRRange(curIdx, endIdx - curIdx), frameRect.width() - layoutPosXY.x);
      if (!line) {
        SR_ASSERT(0); // 한 문자도 레이아웃되지 못한 경우는 없어야겠다(폭이 좁아도 1 문자는 레이아웃시키고 있음)
        break;
      }
      SRIndex pos = curIdx + line->getStringRange().length;
      SRFloat lineHeight = line->getLineSize().height;
      // 현재 컨테이너에 남은 폭으로 생성
      //SRRect proposedRect(layoutPosXY, SRSize(frameRect.right() - layoutPosXY.x, lineHeight));
      SRRect proposedRect(layoutPosXY, SRSize(frameRect.width() - layoutPosXY.x, lineHeight));
      if (proposedRect.width() < 0) {
        //SR_ASSERT(0);
        proposedRect.size.width = 0;
      }
      SRRect remainingRect;
      proposedRect.origin += frameRect.origin; // lineFragmentRect 에서는 컨테이너상의 좌표가 필요하므로 추가함
      SRRect lineRect = tc->lineFragmentRect(proposedRect, 0, SRWritingDirectionLeftToRight, &remainingRect);
      if (lineRect != proposedRect) {
        remainingRect.origin.x -= frameRect.origin.x; // 컨테이너상의 좌표 -> layoutRect 좌표
        lineRect.origin.x -= frameRect.origin.x;

        line = _typesetter->suggestLineBreak(SRRange(curIdx, endIdx - curIdx), lineRect.size.width);
        if (!line) {
          SR_ASSERT(0); // 한 문자도 레이아웃되지 못한 경우는 없어야겠다(폭이 좁아도 1 문자는 레이아웃시키고 있음)
          break;
        }
        pos = curIdx + line->getStringRange().length;

        if (line->getStringRange().length == 1 && line->getLineSize().width > lineRect.size.width) {
          // 레이아웃 결과가 1글자도 포함되지 못하는 경우(요청한 폭을 넘는 경우) 다음 위치로 이동
          if (remainingRect.size.width > 0) {
            // 컨테이너의 hole을 건너뜀
            layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          } else {
            // 개행처리
            if (ret->getLines().size() == 0) {
              // 첫 라인인 경우, 현재 라인의 높이만큼 이동하면, 첫 라인의 y 좌표가 너무 커질수 있으므로
              // 적절한 값 만큼만 y 좌표를 늘리게 함(e.g, wrapping image 존재시 이미지 하단과 다음 텍스트라인과 간격이 너무 넓게됨)
              layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight / 2 };
              //layoutPosXY = { 0.0f, layoutPosXY.y + 5 }; // 5 pixel 씩 라인 높이를 늘려가면서 1 글자이상 포함되는지 체크함
            } else {
              // 이전 라인이 존재하면, 이전 라인 높이만큼 이동해야 정확하다.
              layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
            }
          }
          continue;
        }
      }

      const SRFloat lineWidth = line->getLineSize().width;
      lineHeight = line->getLineSize().height;

      if ((sizeOut.height + lineHeight) > frameRect.height()) {
        // 전달받은 영역의 높이를 초과하는 경우, 레이아웃 중단
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
          lineOrigin.x = 0; // TODO 2단 중앙 정렬시 발생되고 있으며, 원인분석 필요함
        }
#if 1
        SRBool isLayoutedSameLine = false;
        if (ret->_lineOrigins.size() > 0) {
          const SRPoint lastLineOrg = ret->_lineOrigins.back();
          if (lastLineOrg.y == layoutPosXY.y)
            isLayoutedSameLine = true;
        }
        // fitFrame 에 들어있는 라인(1개)를 ret 에 저장.
        if (isLayoutedSameLine) {
          // 마지막 레이아웃된 라인과 동일 라인일 경우, 마지막 라인에 추가시킴
          CTLinePtr lastLine = ret->_lines.back(); // 기존 마지막 라인
          CTLinePtr appendLine = line; // 추가되는 라인
          lastLine->_width = (layoutPosXY.x + lineWidth);
          if (lastLine->_ascent != appendLine->_ascent || lastLine->_descent != appendLine->_descent
            || lastLine->_leading != appendLine->_leading) {
            //SR_ASSERT(0);
            int a = 0;
          }
          //lastLine->_trailingWhitespaceWidth += appendLine->_trailingWhitespaceWidth;
          lastLine->_trailingWhitespaceWidth = appendLine->_trailingWhitespaceWidth; // 마지막 공백문자 길이만 저장
          lastLine->_strRange.length += appendLine->_strRange.length;

          const SRPoint lastLineOrg = ret->_lineOrigins.back();
          const SRFloat appendRunPosX = layoutPosXY.x - lastLineOrg.x; // 추가되는 라인의 런 시작 위치
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
        // center hole이 없었을 경우의 원본 소스
        ret->_lines.push_back(line);
        ret->_lineOrigins.push_back(lineOrigin);
#endif
      }

      curIdx = pos;
      if ((layoutPosXY.x + lineWidth) > sizeOut.width) {
        sizeOut.width = layoutPosXY.x + lineWidth;
      }
      if ((lineWidth - line->_trailingWhitespaceWidth) > sizeWithoutTailWhiteSpace.width) {
        // 라인 끝의 공백문자를 제외한 폭도 저장시킴
        sizeWithoutTailWhiteSpace.width = (lineWidth - line->_trailingWhitespaceWidth);
      }

      wchar_t lastChar = line->getGlyphRuns().back()->getStringFragment().data().back();
      if (remainingRect.size.width > 0 && lastChar != '\n') {
        // 개행문자로 끝나지 않고 남은 영역이 있는 경우
        if (curIdx == endIdx) {
          // 최초에 생성한 프레임(simpleRectFrame)의 다음 라인을 현재 라인 끝 위치부터 레이아웃시킴
          layoutPosXY.x += lineWidth; // 레이아웃한 영역(proposedRect)에 공간이 많이 남았을 수 있으므로 fitSize만큼만 추가시킴
        } else {
          // 컨테이너의 hole을 건너뜀
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
        }
      } else {
        SRFloat leftWidth = lineRect.size.width - lineWidth;
        if (false && leftWidth > 0 && lastChar != '\n') {
          // 현재 남은 영역이 있는 경우, 계속해서 현재 영역에 레이아웃하게 함
          // 이렇게 처리하면, 최초 레이아웃된 라인길이가 유지되면서 짧게 레이아웃되던 현상은 없어지나 워드랩이 안되게 된다.
          layoutPosXY.x += lineWidth;
        } else {
          // 개행문자로 끝나거나 남은 영역이 없는 경우, 개행처리
          layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
        }
      }
      sizeOut.height = layoutPosXY.y; // 레이아웃된 높이 설정
    }
  }

  if (ret) {
    ret->_frameRect.size = sizeOut; // 실제 차지하는 영역으로 설정함
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
