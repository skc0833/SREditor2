#include "SRLayoutManager.h"
#include "SRTextContainer.h"
#include "SRTextBlock.h"
#include "../CoreText/CTFramesetter.h"
#include "../CoreText/CTParagraphStyle.h"
#include "../UIKit/UITextView.h"
#include <algorithm>

namespace sr {

static void _ensureMinimunSize(SRSize& size) {
  if (size.width <= 0) {
    size.width = 1;
  }
  if (size.height <= 0) {
    size.height = 1;
  }
}

SRLayoutManager::SRLayoutManager() : _needsLayout(false) {
}

SRLayoutManager::~SRLayoutManager() {
}

SRRect SRLayoutManager::usedRectForTextContainer(SRTextContainerPtr tc) {
  // Returns the text container's currently used area, which determines the size 
  // that the view would need to be in order to display all the glyphs that are currently 
  // laid out in the container. This causes neither glyph generation nor layout.
  //SR_ASSERT(0);
  return _usedRect;
}

void SRLayoutManager::invalidateDisplay(const SRRange& charRange) {
  SR_NOT_IMPL();
  _needsLayout = true;
  // UITextView���� ���̾ƿ��� ��������� �˸�
  _delegate->layoutManagerDidInvalidateLayout(this->shared_from_this());

  //UITextView::layoutManagerDidInvalidateLayout()
  //  [self _adjustTextLayerSize : FALSE/*setContentPos*/]; // ���� ���̾ƿ��� ũ�� ȹ��
  //  [self setNeedsDisplay];
  //UITextView::_adjustTextLayerSize:(BOOL)setContentPos {
  //    SRRect allText = [_layoutManager usedRectForTextContainer:_textContainer]; // ���� ���̾ƿ��� ũ�� ȹ��
  //    [self setNeedsDisplay]; // ȭ�� ���� ����
  //    [self setContentSize:size]; // ũ�� ������ ������, setContentSize()�� ũ�⸦ ���� ������
  //(SRRect)LM::usedRectForTextContainer:(NSTextContainer*)tc
  //  [self layoutIfNeeded]; // ���⼭ _layoutAllText()�� ȣ���
  //  return 0, 0, _totalSize
  //(void)LM::layoutIfNeeded {
  //  if (_needsLayout) �̸�, [self _layoutAllText]; ��, _totalSize �� SRRect ����

  // @@TODO ���⼭ UITextView::layoutManagerDidInvalidateLayout() �� �ݹ����� ȣ��������!!!
  //layoutAllText(); // @@TODO �ӽ÷� ���⼭ ȣ����
}

void SRLayoutManager::processEditing(SRTextStoragePtr textStorage, 
  SRInt editMask, SRRange newCharRange, SRInt delta, SRRange invalidatedCharRange) {
  // Sent from the NSTextStorage method processEditing to notify the layout manager of an edit action.
  // LayoutManager�� �� �޽����� ó���ϴ� ���� TextStorage �� �������� �����ϸ� �ȵȴ�.
  // WinObjC�� �Ʒ� �Լ��� ȣ������
  // �ᱹ TV���� ���̾ƿ� ������ �˸���, TV�� LM���� ���̾ƿ��� ũ�⸦ ȹ���Ͽ� �ڽ��� ũ�⸦ ������
  //invalidateDisplay(invalidatedCharRange); // invalidateLayout() �� ���� �ʳ�???
  layoutAllText(); // @@TODO �ӽ÷� ���⼭ ȣ����. newCharRange �� �����̳ʸ� ã�Ƽ� ���̾ƿ��� �ʿ�???
  // setNeedsDisplay()??? Figure 2  The text layout process on 
  // https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Concepts/LayoutManager.html#//apple_ref/doc/uid/20001805-BBCFEBHA
  // gnustep �� ���,
  //[self invalidateGlyphsForCharacterRange : invalidatedRange
  //  changeInLength : lengthChange
  //  actualCharacterRange : &r];
  //[self _invalidateLayoutFromContainer : 0];
  //[self _didInvalidateLayout];
  return;
}

// ���޹��� ������ �������� ����
CTFramePtr SRLayoutManager::makeFrame(const SRRect layoutRect, const SRRange range,
  const SRTextContainerPtr tc, CTFramesetterPtr _framesetter) {
  const SRSize frameSize = layoutRect.size;
  // TODO ���� _framesetter->createFrame() ȣ��� �� �Լ� ������ ���� �����ø��� �����̳��� hole�� ó���ؾ߰ڴ�!
  //CTFramePtr simpleRectFrame = _framesetter->createFrame(range, SRRect(SRPoint(0, 0), frameSize), tc);
  CTFramePtr simpleRectFrame = _framesetter->createFrame(range, layoutRect, tc);
  //if (tc->isSimpleRectangularTextContainer()) {
  //  return simpleRectFrame;
  //}
  return simpleRectFrame;

#if 0
  // ����, �����̳ʿ� hole ���� �����ϴ� ��츦 ó����
  CTFramePtr outFrame = CTFrame::create(); // lineFragmentRect() ó���� ������ �����
  if (simpleRectFrame->getLines().size() == 0) {
    return outFrame;
  }

  SRPoint layoutPosXY = simpleRectFrame->_frameRect.origin; // ù ��° ������ ���� ��ġ���� ���̾ƿ���
  //SRSize containerSize = tc->size();
  const SRSize containerSize = frameSize;

  outFrame->_frameRect = simpleRectFrame->_frameRect; // �Ʒ����� outFrame�� ���� ũ�⸦ ������
  SRSize& outSize = outFrame->_frameRect.size;
  outSize.width = outSize.height = 0;

  // ���κ��� container �� lineFragmentRect()�� ȣ���Ͽ� ���̾ƿ��� ������ ȹ����
  for (auto& line : simpleRectFrame->getLines()) {
    const SRSize szLine = line->getLineSize();
    const float lineHeight = szLine.height;

    SRRange lineRange = line->getStringRange();
    float drawnWidth = 0.0;

    if ((containerSize.height - layoutPosXY.y) < lineHeight) {
      break; // ���� ���̿� ������ �� ���� ��쿡�� ���� �� �����̳ʿ� ���̾ƿ��ϵ��� ���Ͻ�Ŵ
    }

    // Index of first glyph in line to be drawn
    SRIndex stringIndex = lineRange.location;
    const SRIndex lineEnd = stringIndex + lineRange.length;

    // ���� ������ ������ ���ڱ��� ���̾ƿ�
    while (stringIndex < lineEnd) {
      // 1���� ���ο� ���� �����̳��� ���� ������ �� �ٽ� ó����(e.g, center hole)
      SRRect proposedRect(layoutPosXY, SRSize(containerSize.width - layoutPosXY.x, lineHeight)); // ���� �����̳ʿ� ���� ������ ����
      if (proposedRect.width() < 0) {
        //SR_ASSERT(0);
        proposedRect.size.width = 0;
      }
      SRRect remainingRect;
      proposedRect.origin += layoutRect.origin; // lineFragmentRect ������ �����̳ʻ��� ��ǥ�� �ʿ��ϹǷ� �߰���
      SRRect lineRect = tc->lineFragmentRect(proposedRect, stringIndex, SRWritingDirectionLeftToRight, &remainingRect);
      remainingRect.origin.x -= layoutRect.origin.x; // �����̳ʻ��� ��ǥ -> layoutRect ��ǥ
      lineRect.origin.x -= layoutRect.origin.x;

      // �����̳ʿ��� ���� ������ ���̾ƿ�(���ĵ ó��?)
      SRRect fitPath({ 0, 0 }, lineRect.size); // 0, 0 ���� �����ؾ� �Ѵ�.
      SRRange fitRange(stringIndex, lineEnd - stringIndex);
      if (fitPath.size.width == 0) {
        // lineRect.size.width �� 0 �� ���� �ִ�(�̹����� ���۵� ���)
        int a = 0;
        //fitPath = remainingRect; // ���ʿ� ���� ���� ���� ���, remainingRect ������ ���̾ƿ���Ŵ
        //layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
      }
      CTFramePtr fitFrame = _framesetter->createFrame(fitRange, fitPath);
      if (fitFrame->getLines().size() != 1) {
        // ������ �����ӿ��� 1�� ���θ� �����ؾ���
        SR_ASSERT(0);
        int a = 0;
      }

      CTLinePtr fitLine = fitFrame->getLines().at(0);
      SRSize fitSize = fitLine->getLineSize();
      SRIndex fitLength = fitLine->getStringRange().length;
      SR_ASSERT(fitLength > 0);

      if (fitLength == 1 && fitSize.width > fitPath.width()) {
        // ���̾ƿ� ����� 1���ڵ� ���Ե��� ���ϴ� ���(��û�� ���� �Ѵ� ���) ���� ��ġ�� �̵�
        if (fitPath.height() > lineHeight) {
          SR_ASSERT(0);
          int a = 0;
        }
        // ���翵���� ���� ������ �����ϸ�, ���� ��ġ�� �̵�
        if (remainingRect.size.width > 0) {
          // �����̳��� hole�� �ǳʶ�
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          //layoutPosXY.x -= layoutRect.origin.x; // TODO �ӽ�, ������ proposedRect.origin += layoutRect.origin; �ϹǷ�
        } else {
          // ����ó��
          layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
          drawnWidth = 0.0;
        }
        continue;
      }

      SRBool isLayoutedSameLine = false;
      if (outFrame->_lineOrigins.size() > 0) {
        const SRPoint lastLineOrg = outFrame->_lineOrigins.back();
        if (lastLineOrg.y == layoutPosXY.y)
          isLayoutedSameLine = true;
      }
      // fitFrame �� ����ִ� ����(1��)�� outFrame �� ����.
      if (isLayoutedSameLine) {
        // ������ ���̾ƿ��� ���ΰ� ���� ������ ���, ������ ���ο� �߰���Ŵ
        CTLinePtr lastLine = outFrame->_lines.back(); // ���� ������ ����
        CTLinePtr appendLine = fitLine; // �߰��Ǵ� ����
        lastLine->_width = (layoutPosXY.x + fitSize.width);
        //SR_ASSERT(lastLine->_trailingWhitespaceWidth == 0);
        if (lastLine->_ascent != appendLine->_ascent || lastLine->_descent != appendLine->_descent
          || lastLine->_leading != appendLine->_leading) {
          //SR_ASSERT(0);
          int a = 0;
        }
        lastLine->_trailingWhitespaceWidth += appendLine->_trailingWhitespaceWidth;
        lastLine->_strRange.length += appendLine->_strRange.length;

        const SRPoint lastLineOrg = outFrame->_lineOrigins.back();
        const SRFloat appendRunPosX = layoutPosXY.x - lastLineOrg.x; // �߰��Ǵ� ������ �� ���� ��ġ
        for_each(appendLine->_runs.begin(), appendLine->_runs.end(), [&appendRunPosX](CTRunPtr run) {
          for_each(run->_positions.begin(), run->_positions.end(), [&appendRunPosX](SRPoint& pt) {
            pt.x += appendRunPosX;
          });
        });
        lastLine->_runs.insert(lastLine->_runs.end(), appendLine->_runs.begin(), appendLine->_runs.end());
      } else {
        outFrame->_lines.push_back(fitLine);
        SRPoint renderLineOrigin = fitFrame->_lineOrigins[0];
        renderLineOrigin.x += layoutPosXY.x; // fitFrame._frameRect �������� ���� ��ǥ�� ó���ұ�???
        renderLineOrigin.y = layoutPosXY.y;
        outFrame->_lineOrigins.push_back(renderLineOrigin);
      }

      // ���� ���̾ƿ��� ���� ��ġ�� �̵�
      stringIndex += fitLength;
      drawnWidth += fitSize.width;

      // outFrame�� ���̾ƿ��� �� ����
      if (drawnWidth > outSize.width) { outSize.width = drawnWidth; }

      wchar_t lastChar = fitLine->getGlyphRuns().back()->getStringFragment().data().back();
      //if (remainingRect.size.width > 0 /*&& stringIndex < lineEnd*/) {
      if (remainingRect.size.width > 0 && lastChar != '\n') {
        // ���๮�ڷ� ������ �ʰ� ���� ������ �ִ� ���
        if (stringIndex == lineEnd) {
          // ���ʿ� ������ ������(simpleRectFrame)�� ���� ������ ���� ���� �� ��ġ���� ���̾ƿ���Ŵ
          layoutPosXY.x += fitSize.width; // ���̾ƿ��� ����(proposedRect)�� ������ ���� ������ �� �����Ƿ� fitSize��ŭ�� �߰���Ŵ
        } else {
          // �����̳��� hole�� �ǳʶ�
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          //layoutPosXY.x -= layoutRect.origin.x; // TODO �ӽ�, ������ proposedRect.origin += layoutRect.origin; �ϹǷ�
        }
      } else {
        SRFloat leftWidth = lineRect.size.width - fitSize.width;
        if (false && leftWidth > 0 && lastChar != '\n') {
          // ���� ���� ������ �ִ� ���, ����ؼ� ���� ������ ���̾ƿ��ϰ� ��
          // �̷��� ó���ϸ�, ���� ���̾ƿ��� ���α��̰� �����Ǹ鼭 ª�� ���̾ƿ��Ǵ� ������ �������� ���左�� �ȵǰ� �ȴ�.
          // TODO ���� _framesetter->createFrame() ȣ��� �� �Լ� ������ ���� �����ø��� �����̳��� hole�� ó���ؾ߰ڴ�!
          layoutPosXY.x += fitSize.width;
        } else {
          // ���๮�ڷ� �����ų� ���� ������ ���� ���, ����ó��
          layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
          drawnWidth = 0.0;
        }
      }
      outSize.height = layoutPosXY.y; // outFrame�� ���̾ƿ��� ���� ����
    } // while (stringIndex < lineEnd)
  } // for (auto& line : lines)

  return outFrame;
#endif
}

static const SRTextBlockList& _getTextBlocks(const SRTextStoragePtr textStorage, SRIndex location, SRRange* attributeRange = nullptr) {
  //SRIndex location = rangeLimit.location;
#if 1
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    textStorage->attribute(kCTParagraphStyleAttributeName, location, attributeRange));
#else
  // ������ kCTParagraphStyleAttributeName �Ӽ� ������ ���Ѵ�.
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    textStorage->attribute(kCTParagraphStyleAttributeName, location, attributeRange, rangeLimit));
#endif
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

RenderObjectPtr SRLayoutManager::layoutText(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextBlockPtr textBlock) {
  // layoutRect �����ŭ �������� ����
  //CTFramesetterPtr framesetter = CTFramesetter::create(textStorage());
  SRRect rcRoot = parentRender->getRectInRoot();
  rcRoot.origin += layoutRect.origin;
  rcRoot.size = layoutRect.size;
  CTFramePtr outFrame = makeFrame(rcRoot, range, _curTextContainer.lock(), _framesetter);
  SRRange outRange = outFrame->getVisibleStringRange();
  //SR_ASSERT(outRange == range); // outRange < range ��쵵 ������
  if (outRange.length == 0) {
    // layoutRect ũ�Ⱑ ���� ��� �߻��ϰ� �ȴ�.
    SR_ASSERT(0);
    return nullptr;
  }
  // ���̾ƿ� ��û ���� ��ġ��, ������ ũ��� ���� ��ü ����
  SRRect outRect = layoutRect;
  outRect.size = outFrame->_frameRect.size;
  if (outRect.size.width > layoutRect.size.width) {
    // ���� ���๮�ڵ��� ������ ���Խ��Ѽ� ��û�� ���� �Ѿ ���� �ִ�.
    // ������ ���� �۾� 1 ���ڵ� �ȵ� ���, ������ 1���ڸ� ���� ��쿡�� ��û�� ���� ���� �� �ִ�.
    float www = outFrame->_frameRectWithoutTailingWhiteSpace.width();
    int ccc = outFrame->getShortestLIne()->getGlyphCount();
    if (outFrame->_frameRectWithoutTailingWhiteSpace.width() > layoutRect.size.width
      && outFrame->getShortestLIne()->getGlyphCount() > 1) {
      //SR_ASSERT(0); // ���� ��Ȳ��. @@ todo
    }
    outRect.size.width = layoutRect.size.width;
  }
  outRect.size.width = layoutRect.size.width; // TODO ĳ�� ǥ�ø� ���� ó���ϱ� ����
  RenderTextFramePtr textRender = RenderTextFrame::create(parentRender, outRect);
  textRender->setFrame(outFrame);
  //textRender->setTextBlock(textBlock);
  textRender->setRange(outRange);

  parentRender->addChild(textRender); // �θ� ũ�⵵ �þ

  return textRender;
}

static void _adjustTableRowHeight(RenderTablePtr tableRender, SRIndex rowStartIdx, SRFloat maxPosY) {
  // ���� ���̸� ���� �ִ� ���̿� ���� ����
  RenderObjectList renders = tableRender->getChildren();
  for (int i = rowStartIdx; i < renders.size(); ++i) {
    SRRect outRect = renders.at(i)->getRect();
    if (outRect.height() > maxPosY) {
      SR_ASSERT(0);
    }
    outRect.size.height = maxPosY;
    renders.at(i)->setRect(outRect);
  }
}

// ���޹��� ������ ����, �θ�� ������ ������ ���̸� �ش� ���� ����
static SRTextBlockPtr _getChildBlock(SRTextStoragePtr textStorage, SRRange range, SRTextBlockPtr parentBlock) {
  const SRTextBlockList& textBlocks = _getTextBlocks(textStorage, range.location);
  for (auto blockIter = textBlocks.begin(); blockIter != textBlocks.end(); ++blockIter) {
    if (SRTextBlock::TableCell == (*blockIter)->getBlockType()) { // SRTextBlockPtr �ּҰ� ������ ���� ã��(�Ӽ� �� X)
      // ���޹��� ������ ����, ������ ���̺�(parentBlock)�� ���� ���̸� ����
      SRTextTableCellBlockPtr resultBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(*blockIter);
      SRTextTablePtr table = std::dynamic_pointer_cast<SRTextTable>(parentBlock);
      if (resultBlock->getTable() == table) {
        return resultBlock;
      }
    } else if (SRTextBlock::ListItem == (*blockIter)->getBlockType()) { // SRTextBlockPtr �ּҰ� ������ ���� ã��(�Ӽ� �� X)
      // ���޹��� ������ ����, ������ ����Ʈ(parentBlock)�� ���� ���̸� ����
      SRTextListBlockPtr resultBlock = std::dynamic_pointer_cast<SRTextListBlock>(*blockIter);
      SRTextListPtr list = std::dynamic_pointer_cast<SRTextList>(parentBlock);
      if (resultBlock->getList() == list) {
        return resultBlock;
      }
    }
  }
  //SR_ASSERT(0);
  return nullptr;
}

RenderObjectPtr SRLayoutManager::layoutTable(RenderObjectPtr parentRender, SRRect layoutRect
  , SRRange range, SRTextTablePtr table) {
  // ���̺� ���� ����
  const int tableRowCnt = table->getNumberOfRows();
  const int tableColCnt = table->getNumberOfColumns();

  SRRect tableRect(layoutRect.origin, SRSize(layoutRect.width(), 0));
  RenderTablePtr tableRender = RenderTable::create(parentRender, tableRowCnt, tableColCnt, tableRect);
  //tableRender->setTextBlock(table);
  tableRender->setRange(SRRange(range.location, 0)); // ���̺� ������ ���ڿ� ���� ��ġ�� ����

  // html spec �� ���缭 ������
  // e.g, <table border="5" cellpadding="0" cellspacing="10" width=100> margin �� ���� ���µ���
  tableRender->_margin = table->getWidth(SRTextBlock::Margin);
  tableRender->_border = table->getWidth(SRTextBlock::Border);
  tableRender->_padding = table->getWidth(SRTextBlock::Padding);
  const SRFloat tableBorder = tableRender->_border; // ���̺� ��輱 �β�
  const SRFloat cellSpacing = tableRender->_padding; // ���̺� ��輱 ~ �� ��輱 ��
  const SRSize cellSize = { // cellBorder + cellPadding ���Ե� ũ����
    (layoutRect.width() - 2 * (tableBorder) - (tableColCnt + 1) * cellSpacing) / tableColCnt,
    layoutRect.height() - 2 * (tableBorder) - 2 * cellSpacing
  };

  // ù ��° �� ���̾ƿ� ���� ��ġ(���̺� ������ġ ������ �� ��輱 ������ġ)
  const SRPoint cellStartXY(tableBorder + cellSpacing, tableBorder + cellSpacing);
  SRPoint layoutXY = cellStartXY;

  typedef struct DummyCell {
    RenderTableCellPtr parentCell;
    DummyCell(RenderTableCellPtr parent = nullptr) : parentCell(parent) {}
  } DummyCell;
  std::vector<DummyCell> dummyRow(tableColCnt, DummyCell());
  std::vector<std::vector<DummyCell>> dummyTable(tableRowCnt, dummyRow);
  //dummyCellList.assign(tableRowCnt * tableColCnt, DummyCell());

  SRIndex rowStartIdx = 0;
  for (int row = 0; row < tableRowCnt; ++row) {
    SRFloat maxPosY = 0;
    for (int col = 0; col < tableColCnt; ++col) {
      // range.location ��ġ�� ���� ���̺��� ������ ȹ��
      // ���̺��� �� �κп� colspan, rowspan ����� cellBlock �� nullptr �� ������
      SRTextTableCellBlockPtr cellBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(
        _getChildBlock(textStorage(), range, table));
      //SR_ASSERT(cellBlock);

      // �� ���� �����ϸ� �ȵȴ�.
      if (cellBlock->_startingRow > row
        || (cellBlock->_startingRow == row && cellBlock->_startingColumn > col)) {
        //SR_ASSERT(0);
        int a = 0;
        //SRStringPtr text = SRString::create(L"\n");
        //textStorage()->replaceCharacters(SRRange(range.location, 0), text);
        //range.length = 1;
      }

      if (dummyTable[row][col].parentCell) {
        // rowspan, colspan ������ ���, �� ������ ����
        RenderTableCellPtr cellRender = RenderTableCell::create(tableRender, row, 0, col, 0
          , SRRect(layoutXY, SRSize(cellSize.width, 0)));
        cellRender->_parentCell = dummyTable[row][col].parentCell;
        tableRender->addChild(cellRender);
        // ���� col �� �̵�
        layoutXY.x += (cellSize.width + cellSpacing); // ���� �÷� ��ġ�� �̵�
        continue;
      }

      // �� ���� ȹ��
      SRRange effRange = textStorage()->rangeOfTextBlock(cellBlock, range.location);
      range.length = effRange.rangeMax() - range.location;

      // �� ���� ������ ���ڴ� ���๮�ڿ��� �Ѵ�(like ms-word)
      SRIndex cellLastIndex = range.location;
      if (range.length > 0) {
        cellLastIndex = range.rangeMax() - 1;
      } else {
        int a = 0;
      }
      wchar_t cellLastChar = textStorage()->getString()->data().at(cellLastIndex);
      if (cellLastChar != '\n') { // �� ���ڿ� ���� ���๮�� �߰�
        SRStringPtr text = SRString::create(L"\n");
        textStorage()->replaceCharacters(SRRange(cellLastIndex + 1, 0), text);
        range.length += 1;
      }

      _ensureMinimunSize(const_cast<SRSize&>(cellSize));
      RenderTableCellPtr cellRender = RenderTableCell::create(tableRender, row, cellBlock->_rowSpan, col, cellBlock->_columnSpan
        , SRRect(layoutXY, cellSize));

      // rowspan, colspan ������ �ش�Ǵ� ���̺� ���� ��ġ�� ����
      SRInt rowSpan = cellBlock->_rowSpan;
      SRInt colSpan = cellBlock->_columnSpan;
      for (int c = col + 1; c < col + colSpan; ++c) {
        dummyTable[row][c] = DummyCell(cellRender); // 1) colspan apply
        for (int r = row + 1; r < row + rowSpan; ++r) {
          dummyTable[r][c] = DummyCell(cellRender); // 2) colspan & rowspan apply(from 2nd col)
        }
      }
      for (int r = row + 1; r < row + rowSpan; ++r) {
        dummyTable[r][col] = DummyCell(cellRender); // 3) rowspan apply(from 1st col)
      }

      SRRect layoutRect(SRPoint(0, 0), cellSize); // �� ���� (0, 0) ��ġ�� ���̾ƿ�

      layoutRect.size.width *= colSpan; // ���̾ƿ� ���� colSpan �����ŭ �ø�(rowSpan ������ ���̴� 0�̹Ƿ� �������)

      // �� �������� ��輱, �е��� layoutBlock() �Լ������� ó���� �ȴ�.
      layoutBlock(cellRender, layoutRect.size, range, cellBlock);

      // ���� ���� ���� ������ ����(���̾ƿ��� ũ��� ������ ũ�� ��ŭ�̹Ƿ� ���� ũ�� ���Ͽ��� ��)
      SRRect outRect = cellRender->getRect();
      if (outRect.width() > cellSize.width || outRect.height() > cellSize.height) {
        // ���๮�ڸ�ŭ ���� �� Ŭ���� �ִ�.
        //SR_ASSERT(0);
      }
      outRect.size.width = cellSize.width;
      cellRender->setRect(outRect);

      if (cellRender->getRect().height() > maxPosY) {
        maxPosY = cellRender->getRect().height();
      }

      tableRender->addChild(cellRender);

      // ���� col �� �̵�
      layoutXY.x += (cellSize.width + cellSpacing); // ���� �÷� ��ġ�� �̵�
      range.location = tableRender->getRange().rangeMax(); // ���̾ƿ� �Ϸ�� ���� ��ġ�� �̵�
    }

    // ���� ���̸� ���� �ִ� ���̿� ���� ����
    _adjustTableRowHeight(tableRender, rowStartIdx, maxPosY);
    rowStartIdx += tableColCnt;

    // ���� row �� �̵�
    layoutXY.x = cellStartXY.x;
    layoutXY.y += maxPosY + cellSpacing;
  }

  RenderObjectList cells = tableRender->getChildren();
  SR_ASSERT(tableRender->_numberOfRows * tableRender->_numberOfColumns == cells.size());

  // rowspan, colspan �� ���� ������ �� ������ ����
  for (auto& item : cells) {
    RenderTableCellPtr cell = std::dynamic_pointer_cast<RenderTableCell>(item);
    if (cell->getRange().length == 0) {
      SR_ASSERT(cell->_columnSpan == 0 || cell->_rowSpan == 0);
      SRRect rc = cell->_parentCell->getRect();
      rc += cell->getRect(); // increase rowspan, colspan's parent cell size
      cell->_parentCell->setRect(rc);
      cell->removeFromParent();
    }
  }

  // tableRender �� ũ��� ������ ���� �߰��Ǹ鼭 �þ �����̹Ƿ� ���� ������ ����
  tableRect.size.height = layoutXY.y + tableBorder; // layoutXY.y�� ���� ������ ������ġ�̹Ƿ� ���̺� ��輱 �β��� �߰���
  tableRender->setRect(tableRect); // ���̺� ũ�⸦ ���̺� ��輱 �β���ŭ �� Ű���.

  parentRender->addChild(tableRender); // ���̺� ���̾ƿ��� �Ϸ�ǰ� �θ𿡰� �߰��Ѵ�.
  return tableRender;
}

RenderObjectPtr SRLayoutManager::layoutList(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextListPtr list) {
  // ���� ����
  //layoutRect.origin.x += 260; // @@test
  //layoutRect.size.width -= 260;
  SRRect listRect(layoutRect.origin, SRSize(layoutRect.width(), 0));
  RenderListPtr listRender = RenderList::create(parentRender, listRect);
  //listRender->setTextBlock(list);
  listRender->setRange(SRRange(range.location, 0)); // ���̺� ������ ���ڿ� ���� ��ġ�� ����

  SRPoint layoutXY(0, 0);
  const SRInt rangeMax = range.rangeMax();
  SRInt itemNumber = 1;

  while (range.location < rangeMax) {
    // range.location ��ġ�� ���� ���̺��� ������ ȹ��
    SRTextListBlockPtr listBlock = std::dynamic_pointer_cast<SRTextListBlock>(
      _getChildBlock(textStorage(), range, list));
    SR_ASSERT(listBlock);

    // �� ���� ȹ��
    SRRange effRange = textStorage()->rangeOfTextBlock(listBlock, range.location);
    range.length = effRange.rangeMax() - range.location;

    // list header
    UIFontPtr listHeadFont = std::dynamic_pointer_cast<UIFont>(textStorage()->attribute(kCTFontAttributeName, range.location));
    //SRRect rcListHead(layoutXY, SRSize(25, 25)); // @@todo �� �����ŭ���� ����
    SRRect rcListHead(layoutXY, SRSize(listHeadFont->lineHeight() * 2, listHeadFont->lineHeight())); // ���� ��Ʈ ũ��� ����
    SRString marker = list->markerForItemNumber(itemNumber);
    RenderListHeaderPtr listHeadRender = RenderListHeader::create(listRender, rcListHead, marker);
    listRender->addChild(listHeadRender);
    layoutXY.x += rcListHead.size.width;

    // layoutXY �� ���̺� �̳��� ������ ������((0, 0) ���� ����)
    SRSize listItemSize(layoutRect.size.width - layoutXY.x, layoutRect.size.height - layoutXY.y);
    _ensureMinimunSize(listItemSize);
    RenderListItemPtr listItemRender = RenderListItem::create(listRender, SRRect(layoutXY, listItemSize));

    // �� �������� ��輱, �е��� layoutBlock() �Լ������� ó���� �ȴ�.
    layoutBlock(listItemRender, listItemSize, range, listBlock);
    listRender->addChild(listItemRender);

    range.location = listRender->getRange().rangeMax();

    layoutXY.x = 0;
    layoutXY.y = listItemRender->getRect().bottom();

    ++itemNumber;
  }

  parentRender->addChild(listRender); // ���̺� ���̾ƿ��� �Ϸ�ǰ� �θ𿡰� �߰��Ѵ�.
  return listRender;
}

static void _ensure_size(SRSize& size) {
  if (size.width <= 0)
    size.width = 1;
  if (size.height <= 0)
    size.height = 1;
}

// ������ ���๮�� �ٷ� ���� ��ġ�� ������
static SRIndex _next_paragraph(const SRTextStoragePtr& storage, SRIndex pos, SRIndex endPos) {
  const std::wstring str = storage->getString()->data();
  static const int LINE_FEED_CHAR = 10; // '\n'
  static const int CARRIAGE_RETURN_CHAR = 13; // '\r'

  while (pos < endPos) {
    wchar_t ch = str.at(pos++);
    SR_ASSERT(ch != CARRIAGE_RETURN_CHAR);
    if (ch == LINE_FEED_CHAR) { // ���� \r �� �ܵ����� ���Ǵ� ���� ������� ����
      break;
    }
  }
  return pos;
}

// parentRender �� ������ targetBlock ������ ������ �θ��� layoutRect ������ ���̾ƿ���
// �θ��� layoutRect ������ �θ��� �е��� �����Ͽ� ���̾ƿ���
// ���ʷ� ��Ʈ ������ ȣ��Ǹ�, ���̺� ��, ����Ʈ �����ۿ� ���ؼ��� ȣ���
// ��Ʈ ������ ���, �������� �������� �����Ǹ�, ���� ������ ������ �����ϰ� �ִ�.
RenderObjectPtr SRLayoutManager::layoutBlock(RenderObjectPtr parentRender, SRSize requestlayoutSize
  , SRRange layoutRange, const SRTextBlockPtr targetBlock) {
  SR_ASSERT(parentRender);
  parentRender->setRange(SRRange(layoutRange.location, 0)); // ���� RenderBlock::addChild() �ÿ� �ڽ��� ���ڿ� ������ŭ �þ
  //parentRender->setTextBlock(targetBlock);

  // ���� ���̾ƿ��Ǵ� �信 ���ؼ��� targetBlock �� margin, border, padding �� �����Ų��.
  // ���� ���� �ڽĺ���� �θ�� ������ margin + border + padding ��ŭ ���ʿ� ��ġ��Ų��.
  parentRender->_margin = targetBlock->getWidth(SRTextBlock::Margin);
  parentRender->_border = targetBlock->getWidth(SRTextBlock::Border);
  parentRender->_padding = targetBlock->getWidth(SRTextBlock::Padding);

  // targetBlock�� �»�� �е� ����(�������� ���̾ƿ��� ��ġ ����)
  //const SRFloat totalPadding = targetBlock->getWidthAll(); // Text or TableCell
  const SRFloat totalPadding = parentRender->_margin + parentRender->_border + parentRender->_padding;
  SRRect layoutRect = { SRPoint(0, 0), requestlayoutSize };
  layoutRect = layoutRect.insetBy(totalPadding, totalPadding);

  SRPoint layoutXY = layoutRect.origin;
  SRSize layoutSize = layoutRect.size;
  if (layoutSize.width <= 0 || layoutSize.height <= 0) {
    //SR_ASSERT(0);
    _ensure_size(layoutSize);
    //return nullptr;
  }

  SRRect layoutedRect(SRPoint(0, 0), SRSize(0, 0)); // ���̾ƿ� �Ϸ�� ���� �����
  RenderObjectPtr lastLayoutedRender = nullptr;

  //const SRIndex layoutRangeMax = layoutRange.rangeMax(); // ���̾ƿ��� end index
  SRIndex layoutRangeMax = layoutRange.rangeMax(); // ���̾ƿ��� end index
  while (layoutRange.location < layoutRangeMax) {
    SR_ASSERT(layoutRange.location >= 0);
    if (layoutXY.y >= layoutRect.bottom()) {
      // ���̾ƿ��� ������ �ȳ���
      SR_ASSERT(0);
      return lastLayoutedRender;
    }
    // ���� ��ġ�� �ؽ�Ʈ �� ����Ʈ�� ȹ��
    const SRTextBlockList& textBlocks = _getTextBlocks(textStorage(), layoutRange.location);
    SR_ASSERT(textBlocks.size() > 0);

    // �θ� �ؽ�Ʈ ��(targetBlock)�� ������ ������ ���� ã�� �ش� ���� ���̾ƿ���
    bool isLeafBlock = true;
    if (textBlocks.size() > 1 && textBlocks.back() != targetBlock) {
      isLeafBlock = false; // ������ ���� �ƴ� �����
    }

    if (isLeafBlock) {
      // leaf block �� ���, ���๮�ڱ����� ������ �׸���.
      SRIndex nextParaIndex = _next_paragraph(textStorage(), layoutRange.location, layoutRangeMax);
      SRRange textRange(layoutRange.location, nextParaIndex - layoutRange.location);
      SRRect textRect(layoutXY, layoutSize);
      // ��輱�� ������ ������ ������
      //const SRFloat border = targetBlock->getWidth(SRTextBlock::Border);
      //textRect = textRect.insetBy(-border, -border);
      lastLayoutedRender = layoutText(parentRender, textRect, textRange, targetBlock);
    } else {
      // leaf block �� �ƴϸ�, ���� ���� inner block ���� ����
      auto iterInnerBlock = std::find(textBlocks.begin(), textBlocks.end(), targetBlock);
      SR_ASSERT(iterInnerBlock != textBlocks.end());

      SRTextBlockPtr innerBlock = *(++iterInnerBlock);
      SRTextBlock::BlockType innerBlockType = innerBlock->getBlockType();

      if (innerBlockType == SRTextBlock::TableCell) { // ���� Text, TableCell ������
                                                      // ���� ���� inner block �� ���̺� ��(ù��° ��)�� ���, ���̺� ���̾ƿ�
        SRTextTableCellBlockPtr child = std::dynamic_pointer_cast<SRTextTableCellBlock>(innerBlock);
        SRTextTablePtr parent = child->getTable();
        SRRange parentRange = textStorage()->rangeOfTextTable(parent, layoutRange.location); // ��ü ���̺� ����
        SRRect layoutBlock(layoutXY, layoutSize);

        if (1) {
          // TODO inner table ���Խ� ���� �ſ� �۰�(���� 0?)���� ��������� �־ �ϴ� ��Ȱ��ȭ��Ŵ
          // �����̳� ���ܿ��� ó��
          SRRect proposedRect(layoutBlock);
          SRRect remainingRect;
          SRTextContainerPtr tc = _curTextContainer.lock();
          SRBool parentIsRoot = (typeid(*parentRender) == typeid(RenderRoot));
          if (parentIsRoot && !tc->isSimpleRectangularTextContainer()) {
            // ��Ʈ ���� �ٷ� ���� �ڽ� ������ ��쿡�� �����̳� excludePath�� ó���Ѵ�.
            SRRect lineRect = tc->lineFragmentRect(proposedRect, 0, SRWritingDirectionLeftToRight, &remainingRect);
            //if (lineRect != proposedRect) {
            if (lineRect.size.width == 0) {
              layoutBlock = remainingRect;
            } else {
              layoutBlock = lineRect;
            }
            //}
          }
        }
        lastLayoutedRender = std::dynamic_pointer_cast<RenderTable>(
          layoutTable(parentRender, layoutBlock, parentRange, parent));
        SR_ASSERT(lastLayoutedRender->getRange().location == parentRange.location);
        if (lastLayoutedRender->getRange().rangeMax() != parentRange.rangeMax()) {
          // �� ������ ���� ��ġ�� ���๮�ڸ� �߰��� �����
          layoutRangeMax += (lastLayoutedRender->getRange().rangeMax() - parentRange.rangeMax());
        }
        //if (0) {
        //  // �����̳� ���ܿ��� ó��
        //  layoutXY = layoutRect.origin;
        //  layoutSize = layoutRect.size;
        //}
      } else if (innerBlockType == SRTextBlock::ListItem) {
        // ���� ���� inner block �� ����Ʈ ������(ù��° ������)�� ���, ����Ʈ ���̾ƿ�
        SRTextListBlockPtr child = std::dynamic_pointer_cast<SRTextListBlock>(innerBlock);
        SRTextListPtr parent = child->getList();
        SRRange parentRange = textStorage()->rangeOfTextList(parent, layoutRange.location); // ��ü ���̺� ����
        SRRect layoutBlock(layoutXY, layoutSize);

        if (1) {
          // �����̳� ���ܿ��� ó��
          SRRect proposedRect(layoutBlock);
          SRRect remainingRect;
          SRTextContainerPtr tc = _curTextContainer.lock();
          SRBool parentIsRoot = (typeid(*parentRender) == typeid(RenderRoot));
          if (parentIsRoot && !tc->isSimpleRectangularTextContainer()) {
            // ��Ʈ ���� �ٷ� ���� �ڽ� ������ ��쿡�� �����̳� excludePath�� ó���Ѵ�.
            SRRect lineRect = tc->lineFragmentRect(proposedRect, 0, SRWritingDirectionLeftToRight, &remainingRect);
            //if (lineRect != proposedRect) {
            if (lineRect.size.width == 0) {
              layoutBlock = remainingRect;
            } else {
              layoutBlock = lineRect;
            }
            //}
          }
        }
        lastLayoutedRender = std::dynamic_pointer_cast<RenderList>(
          layoutList(parentRender, layoutBlock, parentRange, parent));
        SR_ASSERT(lastLayoutedRender->getRange().location == parentRange.location);
        if (lastLayoutedRender->getRange().rangeMax() != parentRange.rangeMax()) {
          // �� ������ ���� ��ġ�� ���๮�ڸ� �߰��� �����
          layoutRangeMax += (lastLayoutedRender->getRange().rangeMax() - parentRange.rangeMax());
        }
      } else if (innerBlockType == SRTextBlock::Text) {
        // <div> �� �ش�??? �� ��쿡�� innerBlock �� ���� parentRender �� ���� layoutBlock()�� ȣ����???
        SR_ASSERT(0);
        //SRRect textRect(layoutRect.origin, SRSize(0, 0));
        //RenderTextBlockPtr textRender = RenderTextBlock::create(parentRender, textRect);
        ////textRender->setTextBlock(innerBlock);
        //outRender = layoutBlock(textRender, layoutRect, range, innerBlock);
      } else {
        SR_ASSERT(0);
      }
    }

    // ���� �������� �̵�
    SR_ASSERT(lastLayoutedRender);
    layoutRange.location = parentRender->getRange().rangeMax();

    SRRect outRect = lastLayoutedRender->getRect();
    //layoutRect.origin.x = 0; // ����Ʈ�� ���, 2��° ���� ���ĵ� ���� ������ �־�� �Ѵ�.
    //layoutXY.y = outRect.bottom();
    if (layoutXY != outRect.origin) {
      int a = 0;
    }
    layoutXY.y += outRect.height(); // ???
    layoutSize.height = layoutRect.bottom() - layoutXY.y; // ���̾ƿ��� ũ�⸸ŭ ���̾ƿ��� ���̸� ���δ�.

    layoutedRect += outRect;
  }

  // targetBlock�� ���ϴ� �е� ����
  layoutedRect += totalPadding;

  // rcParent.origin �� �״�� ������Ű��, ���̾ƿ��� ũ�⸸ŭ���� ������(���� ũ�⺸�� �پ��� ����)
  SRRect rcParent = parentRender->getRect();
  rcParent.size = layoutedRect.size;
  parentRender->setRect(rcParent);

  return lastLayoutedRender; // ������ ���̾ƿ��� ������ ����
}

void SRLayoutManager::layoutAllText() {
  // Typesetter.layoutCharactersInRange(range, rect, layoutMgr) ?
  // NSTypesetter:layoutGlyphsInLayoutManager ����
  // (NSRange)layoutCharactersInRange:(NSRange)characterRange 
  // forLayoutManager:(NSLayoutManager *)layoutManager
  // maximumNumberOfLineFragments : (NSUInteger)maxNumLines;

  SRIndex curTextIdx = 0;
  _framesetter = CTFramesetter::create(textStorage()); // �Ź� ��ü ���̾ƿ� ���۽� �ʱ�ȭ
  SR_ASSERT(_textContainers.size() == 1); // ����� �����̳� 1���� ������(�̰� ���� �ؽ�Ʈ�信 ������ �׸��� ����)

  // �Ʒ����� _delegate ȣ��� �����̳ʰ� �߰��� ���, it �� ����ǰ� �ǹǷ� Range-based for �� ������� ����
  for (auto it = _textContainers.begin(); it != _textContainers.end(); ++it) {
    // �߰� �����̳� ���̾ƿ��� ����Ǹ�, ���� �����̳ʵ��� ���� ���ŵž� �Ѵ�.
    SRTextContainerPtr tc = *it;
    SRIndex tcStartTextIdx = curTextIdx; // �����̳ʿ� ���Ե� ���ڿ��� ���� �ε���

    // ���� �����̳ʿ� ���̾ƿ���ų ���� ���ڿ� ����(tcStartTextIdx ~ ���ڿ� ��)
    SRRange tcTextRange(tcStartTextIdx, textStorage()->getLength() - tcStartTextIdx);

    const SRTextBlockPtr tcStartTextBlock = _getTextBlocks(textStorage(), tcStartTextIdx).front();

    if (1) {
      // �����̳� ���� ��ġ�� �ؽ�Ʈ �� ������ ȹ���Ͽ�, tcTextRange ���̸� ���ѽ�Ŵ
      SRRange effRange = textStorage()->rangeOfTextBlock(tcStartTextBlock, tcStartTextIdx);
      SR_ASSERT(tcTextRange.length == effRange.rangeMax() - tcTextRange.location); // �׻� �����ϹǷ� ���� ���� ��ǻ� ���ʿ���
      tcTextRange.length = effRange.rangeMax() - tcTextRange.location;
    }

    // ���� ���̾ƿ����� �����̳� ����(layoutText() �Լ������� ���ǰ� ����)
    _curTextContainer = tc;

    // �����̳��� root render ����
    const RenderRootPtr rootRender = RenderRoot::create();
    tc->setRootRender(rootRender);

    // rootRender ������ ���� �����̳��� �� �ȿ� ���̾ƿ���Ŵ
    layoutBlock(rootRender, SRSize(tc->size().width, SRFloatMax), tcTextRange, tcStartTextBlock);

    // ���̾ƿ� ��û�� �� �̳��� �����ƴ��� Ȯ��(���̴� ���������� ���̾ƿ��ϰ� ����)
    if (rootRender->getRect().width() > tc->size().width) {
      int a = 0;
      SR_ASSERT(0); // �߻�����. Ȯ���ʿ�!
    }

    // ���̾ƿ��� ���ڿ� ������ ȹ����
    SRRange outRange = rootRender->getRange();
    if (tcTextRange != rootRender->getRange()) {
      //SR_ASSERT(0);
      int a = 0; // ���̺� �� ������ ���ڷ� ���๮�ڰ� �߰��� ��� �߻� ������
    }

    // ���̾ƿ��� ���� ���� ����
    _usedRect = rootRender->getRect();

    // �����̳��� _exclusionPaths ������ ���Խ�Ŵ
    _usedRect += tc->getTotalExclusionPath();

    // �����̳ʿ� ���̾ƿ��� ���ڿ� ���� ����
    tc->setTextRange(SRRange(tcStartTextIdx, outRange.rangeMax() - tcStartTextIdx));

    // ���� ���̾ƿ��� ���� ��ġ�� �̵�
    curTextIdx = outRange.rangeMax();

    if (_delegate) {
      // �����̳� ���̾ƿ��� ����������, delegate ȣ��
      bool atEnd = false;
      if (tc == _textContainers.back()) {
        atEnd = true; // ������ �����̳��̸� atEnd = true;
      }

      // iterator invalidation rules for C++ containers ������ _delegate ���� �����̳� �߰��� it �� ��ȿȭ��
      int tcIdx = it - _textContainers.begin();
      _delegate->didCompleteLayoutFor(this->shared_from_this(), tc, atEnd);
      it = _textContainers.begin() + tcIdx;
    }
  } // for _textContainers
}

void SRLayoutManager::addTextContainer(const SRTextContainerPtr textContainer) {
  _textContainers.push_back(textContainer);
  SR_ASSERT(_textContainers.size() == 1); // ����� ���̾ƿ� �Ŵ����� 1���� �����̳ʸ� ������
  textContainer->setLayoutManager(SRLayoutManagerPtr(this->shared_from_this()));
}

void SRLayoutManager::drawBackground(const SRRange& glyphsToShow, const SRPoint& origin) {
  SR_NOT_IMPL();
}

void SRLayoutManager::drawGlyphs(const SRRange& glyphsToShow, const SRPoint& origin) {
  SR_NOT_IMPL();
}

} // namespace sr
