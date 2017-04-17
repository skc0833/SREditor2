#include "UITextView.h"
#include "SRTextStorage.h"
#include "SRLayoutManager.h"

namespace sr {

/*
SRTextStorage -> SRLayoutManager[] -> SRTextContainer[]
UITextView 는 SRTextContainer 1개를 소유함. 2단으로 표시하려면 UITextView 2개가 필요함.
*/
UITextView::UITextView(const SRRect& frame)
 : UIScrollView(frame) {
}

UITextView::~UITextView() {
}

// Lays out the subviews immediately.
// 하위뷰들을 바로 레이아웃시킨다. 드로잉 전에 하위뷰들을 강제로 레이아웃팅하고 싶을 경우 사용된다.
void UITextView::layoutIfNeeded() {
  SR_NOT_IMPL();
  if (_needsLayout) {
    // 일단 전체 텍스트로 설정함
    SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
    SRTextStoragePtr textStorage = layoutManager->textStorage();
    textStorage->beginEditing();
    //textStorage->setAttributedString(text); // 이전에 이미 설정됨
    textStorage->endEditing(); // 여기서 레이아웃팅됨
  }
  UIView::layoutIfNeeded();
}

void UITextView::setAttributedText(const SRAttributedStringPtr text) {
  // 일단 전체 텍스트로 설정함
  SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  SRTextStoragePtr textStorage = layoutManager->textStorage();
  textStorage->beginEditing();
  textStorage->setAttributedString(text);
  textStorage->endEditing(); // 여기서 레이아웃팅됨
}

void UITextView::draw(CGContextPtr ctx, SRRect rect) const {
  SR_NOT_IMPL();
  UIView::draw(ctx, rect); // 배경 그리기 -> 두번째 페이지부터는 안 그려지고 있다

  beginDraw(ctx);
  textContainer()->rootRender()->draw(ctx);
  endDraw(ctx);
}

UITextPosition UITextView::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) const {
  UITextPosition pos = textContainer()->rootRender()->closestPosition(point, direction, position);
  return pos;
}

SRRect UITextView::caretRect(UITextPosition position) const {
  SRRect caretRect = textContainer()->rootRender()->caretRect(position);
  return caretRect;
}

static const SRTextBlockList& _getTextBlocks(const SRTextStoragePtr textStorage, SRIndex location
  , SRRange* attributeRange = nullptr) {
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    textStorage->attribute(kCTParagraphStyleAttributeName, location, attributeRange));
  SR_ASSERT(paraStyle);
  if (paraStyle != nullptr) {
    const SRTextBlockList& textBlocks = paraStyle->getTextBlockList();
    SR_ASSERT(!textBlocks.empty()); // 항상 텍스트블럭은 있는 걸로 가정
    return textBlocks;
  }
  SR_ASSERT(0);
  static const SRTextBlockList blist; // @@ & 타입이라서 존재하지 않을 경우, 임시로 처리함
  return blist;
}

void UITextView::replace(UITextRange range, const SRString& text) const {
  SRRange textRange(range._range[0]._offset, range._range[1]._offset - range._range[0]._offset);
  SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  SRTextStoragePtr textStorage = layoutManager->textStorage();
  textStorage->beginEditing();

  if (!text.data().compare(L"")) { // 삭제하는 경우
    // 테이블 셀은 개행문자를 남김(like ms-word)
    // 현재 위치의 텍스트 블럭 리스트를 획득
    SRIndex idx = textRange.location;
    SRIndex rangeMax = textRange.rangeMax();
    while (idx < rangeMax) {
      const SRTextBlockList& textBlocks = _getTextBlocks(textStorage, idx);
      SR_ASSERT(textBlocks.size() > 0);
      SRTextBlockPtr innerBlock = textBlocks.back();
      SRRange blockRange = textStorage->rangeOfTextBlock(innerBlock, idx);
      if (innerBlock->getBlockType() == SRTextBlock::TableCell
        || innerBlock->getBlockType() == SRTextBlock::ListItem) {
        //SRRange blockRange = textStorage->rangeOfTextBlock(innerBlock, idx);
        if (idx <= blockRange.location && blockRange.rangeMax() <= rangeMax) {
          // 셀 전체가 삭제되는 경우, 개행문자를 하나 남김
          // 셀의 일부가 삭제되면서 마지막 개행문자가 삭제되는 경우는 레이아웃시 개행문자를 추가하고 있음
          textStorage->replaceCharacters(blockRange, L"\n");
          idx += 1;
          rangeMax -= (blockRange.length - 1);
          continue;
        }
      }
      if (innerBlock->getBlockType() == SRTextBlock::Text) {
        // 텍스트블럭 일부 삭제시 다음으로 텍스트블럭이 오지 않는 경우, 마지막 개행문자는 남긴다.
        SRIndex nextIdx = blockRange.rangeMax();
        nextIdx = (nextIdx > rangeMax) ? rangeMax : nextIdx;
        SRTextBlockPtr block = _getTextBlocks(textStorage, nextIdx).back();
        if (block->getBlockType() != SRTextBlock::Text) {
          if (idx > blockRange.location) {
            // 텍스트블럭 일부 삭제인 경우
            SRRange r(idx, nextIdx - idx);
            textStorage->replaceCharacters(r, L"\n");
            idx += 1;
            rangeMax -= (r.length - 1);
            continue;
          }
        }
      }
      if (rangeMax > blockRange.rangeMax()) { // 셀 일부가 삭제되는 경우임
        SR_ASSERT(0);
      }
      SRRange r(idx, rangeMax - idx);
      textStorage->replaceCharacters(r, L"");
      idx += r.length;
    }
  } else {
    // TODO: 여러 테이블 셀 선택 후, 문자열 교체시 처리해줘야 한다.
    SRIndex idx = textRange.location;
    SRRange effRange;
    CTParagraphStylePtr paragraphStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
      textStorage->attribute(kCTParagraphStyleAttributeName, idx, &effRange));
    textStorage->replaceCharacters(textRange, text);
    if (effRange.location == idx) {
      // 셀 첫 위치에 문자열 삽입시 이전 셀의 끝에 추가되지 않게 현재 셀의 블럭 속성을 유지시킨다.
      textStorage->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(idx, text.length()));
    }
  }
  textStorage->endEditing(); // 여기서 레이아웃팅됨
}

void UITextView::scrollRangeToVisible(SRRange range) const {
  SR_NOT_IMPL();
}

void UITextView::init(SRTextContainerPtr tc) {
  _textContainer = tc;
  UITextViewPtr thisView = getPtr(this);
  textContainer()->setTextView(thisView);
}

} // namespace sr
