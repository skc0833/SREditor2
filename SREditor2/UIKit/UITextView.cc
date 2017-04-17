#include "UITextView.h"
#include "SRTextStorage.h"
#include "SRLayoutManager.h"

namespace sr {

/*
SRTextStorage -> SRLayoutManager[] -> SRTextContainer[]
UITextView �� SRTextContainer 1���� ������. 2������ ǥ���Ϸ��� UITextView 2���� �ʿ���.
*/
UITextView::UITextView(const SRRect& frame)
 : UIScrollView(frame) {
}

UITextView::~UITextView() {
}

// Lays out the subviews immediately.
// ��������� �ٷ� ���̾ƿ���Ų��. ����� ���� ��������� ������ ���̾ƿ����ϰ� ���� ��� ���ȴ�.
void UITextView::layoutIfNeeded() {
  SR_NOT_IMPL();
  if (_needsLayout) {
    // �ϴ� ��ü �ؽ�Ʈ�� ������
    SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
    SRTextStoragePtr textStorage = layoutManager->textStorage();
    textStorage->beginEditing();
    //textStorage->setAttributedString(text); // ������ �̹� ������
    textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
  }
  UIView::layoutIfNeeded();
}

void UITextView::setAttributedText(const SRAttributedStringPtr text) {
  // �ϴ� ��ü �ؽ�Ʈ�� ������
  SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  SRTextStoragePtr textStorage = layoutManager->textStorage();
  textStorage->beginEditing();
  textStorage->setAttributedString(text);
  textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
}

void UITextView::draw(CGContextPtr ctx, SRRect rect) const {
  SR_NOT_IMPL();
  UIView::draw(ctx, rect); // ��� �׸��� -> �ι�° ���������ʹ� �� �׷����� �ִ�

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
    SR_ASSERT(!textBlocks.empty()); // �׻� �ؽ�Ʈ���� �ִ� �ɷ� ����
    return textBlocks;
  }
  SR_ASSERT(0);
  static const SRTextBlockList blist; // @@ & Ÿ���̶� �������� ���� ���, �ӽ÷� ó����
  return blist;
}

void UITextView::replace(UITextRange range, const SRString& text) const {
  SRRange textRange(range._range[0]._offset, range._range[1]._offset - range._range[0]._offset);
  SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  SRTextStoragePtr textStorage = layoutManager->textStorage();
  textStorage->beginEditing();

  if (!text.data().compare(L"")) { // �����ϴ� ���
    // ���̺� ���� ���๮�ڸ� ����(like ms-word)
    // ���� ��ġ�� �ؽ�Ʈ �� ����Ʈ�� ȹ��
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
          // �� ��ü�� �����Ǵ� ���, ���๮�ڸ� �ϳ� ����
          // ���� �Ϻΰ� �����Ǹ鼭 ������ ���๮�ڰ� �����Ǵ� ���� ���̾ƿ��� ���๮�ڸ� �߰��ϰ� ����
          textStorage->replaceCharacters(blockRange, L"\n");
          idx += 1;
          rangeMax -= (blockRange.length - 1);
          continue;
        }
      }
      if (innerBlock->getBlockType() == SRTextBlock::Text) {
        // �ؽ�Ʈ�� �Ϻ� ������ �������� �ؽ�Ʈ���� ���� �ʴ� ���, ������ ���๮�ڴ� �����.
        SRIndex nextIdx = blockRange.rangeMax();
        nextIdx = (nextIdx > rangeMax) ? rangeMax : nextIdx;
        SRTextBlockPtr block = _getTextBlocks(textStorage, nextIdx).back();
        if (block->getBlockType() != SRTextBlock::Text) {
          if (idx > blockRange.location) {
            // �ؽ�Ʈ�� �Ϻ� ������ ���
            SRRange r(idx, nextIdx - idx);
            textStorage->replaceCharacters(r, L"\n");
            idx += 1;
            rangeMax -= (r.length - 1);
            continue;
          }
        }
      }
      if (rangeMax > blockRange.rangeMax()) { // �� �Ϻΰ� �����Ǵ� �����
        SR_ASSERT(0);
      }
      SRRange r(idx, rangeMax - idx);
      textStorage->replaceCharacters(r, L"");
      idx += r.length;
    }
  } else {
    // TODO: ���� ���̺� �� ���� ��, ���ڿ� ��ü�� ó������� �Ѵ�.
    SRIndex idx = textRange.location;
    SRRange effRange;
    CTParagraphStylePtr paragraphStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
      textStorage->attribute(kCTParagraphStyleAttributeName, idx, &effRange));
    textStorage->replaceCharacters(textRange, text);
    if (effRange.location == idx) {
      // �� ù ��ġ�� ���ڿ� ���Խ� ���� ���� ���� �߰����� �ʰ� ���� ���� �� �Ӽ��� ������Ų��.
      textStorage->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(idx, text.length()));
    }
  }
  textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
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
