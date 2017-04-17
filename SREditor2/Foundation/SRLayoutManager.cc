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
  // UITextView에게 레이아웃이 변경됐음을 알림
  _delegate->layoutManagerDidInvalidateLayout(this->shared_from_this());

  //UITextView::layoutManagerDidInvalidateLayout()
  //  [self _adjustTextLayerSize : FALSE/*setContentPos*/]; // 새로 레이아웃된 크기 획득
  //  [self setNeedsDisplay];
  //UITextView::_adjustTextLayerSize:(BOOL)setContentPos {
  //    SRRect allText = [_layoutManager usedRectForTextContainer:_textContainer]; // 새로 레이아웃된 크기 획득
  //    [self setNeedsDisplay]; // 화면 갱신 예약
  //    [self setContentSize:size]; // 크기 변경이 됐으면, setContentSize()로 크기를 새로 설정함
  //(SRRect)LM::usedRectForTextContainer:(NSTextContainer*)tc
  //  [self layoutIfNeeded]; // 여기서 _layoutAllText()가 호출됨
  //  return 0, 0, _totalSize
  //(void)LM::layoutIfNeeded {
  //  if (_needsLayout) 이면, [self _layoutAllText]; 후, _totalSize 로 SRRect 리턴

  // @@TODO 여기서 UITextView::layoutManagerDidInvalidateLayout() 를 콜백으로 호출해주자!!!
  //layoutAllText(); // @@TODO 임시로 여기서 호출함
}

void SRLayoutManager::processEditing(SRTextStoragePtr textStorage, 
  SRInt editMask, SRRange newCharRange, SRInt delta, SRRange invalidatedCharRange) {
  // Sent from the NSTextStorage method processEditing to notify the layout manager of an edit action.
  // LayoutManager는 이 메시지를 처리하는 동안 TextStorage 의 컨텐츠를 변경하면 안된다.
  // WinObjC는 아래 함수를 호출중임
  // 결국 TV에게 레이아웃 변경을 알리면, TV는 LM에게 레이아웃된 크기를 획득하여 자신의 크기를 조절함
  //invalidateDisplay(invalidatedCharRange); // invalidateLayout() 이 맞지 않나???
  layoutAllText(); // @@TODO 임시로 여기서 호출함. newCharRange 로 컨테이너를 찾아서 레이아웃팅 필요???
  // setNeedsDisplay()??? Figure 2  The text layout process on 
  // https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Concepts/LayoutManager.html#//apple_ref/doc/uid/20001805-BBCFEBHA
  // gnustep 의 경우,
  //[self invalidateGlyphsForCharacterRange : invalidatedRange
  //  changeInLength : lengthChange
  //  actualCharacterRange : &r];
  //[self _invalidateLayoutFromContainer : 0];
  //[self _didInvalidateLayout];
  return;
}

// 전달받은 영역에 프레임을 생성
CTFramePtr SRLayoutManager::makeFrame(const SRRect layoutRect, const SRRange range,
  const SRTextContainerPtr tc, CTFramesetterPtr _framesetter) {
  const SRSize frameSize = layoutRect.size;
  // TODO 최초 _framesetter->createFrame() 호출시 이 함수 내에서 라인 생성시마다 컨테이너의 hole을 처리해야겠다!
  //CTFramePtr simpleRectFrame = _framesetter->createFrame(range, SRRect(SRPoint(0, 0), frameSize), tc);
  CTFramePtr simpleRectFrame = _framesetter->createFrame(range, layoutRect, tc);
  //if (tc->isSimpleRectangularTextContainer()) {
  //  return simpleRectFrame;
  //}
  return simpleRectFrame;

#if 0
  // 이후, 컨테이너에 hole 등이 존재하는 경우를 처리함
  CTFramePtr outFrame = CTFrame::create(); // lineFragmentRect() 처리된 프레임 저장용
  if (simpleRectFrame->getLines().size() == 0) {
    return outFrame;
  }

  SRPoint layoutPosXY = simpleRectFrame->_frameRect.origin; // 첫 번째 라인의 시작 위치부터 레이아웃함
  //SRSize containerSize = tc->size();
  const SRSize containerSize = frameSize;

  outFrame->_frameRect = simpleRectFrame->_frameRect; // 아래에서 outFrame의 실제 크기를 지정함
  SRSize& outSize = outFrame->_frameRect.size;
  outSize.width = outSize.height = 0;

  // 라인별로 container 의 lineFragmentRect()를 호출하여 레이아웃할 영역을 획득함
  for (auto& line : simpleRectFrame->getLines()) {
    const SRSize szLine = line->getLineSize();
    const float lineHeight = szLine.height;

    SRRange lineRange = line->getStringRange();
    float drawnWidth = 0.0;

    if ((containerSize.height - layoutPosXY.y) < lineHeight) {
      break; // 남은 높이에 라인이 못 들어가는 경우에는 다음 번 컨테이너에 레이아웃하도록 리턴시킴
    }

    // Index of first glyph in line to be drawn
    SRIndex stringIndex = lineRange.location;
    const SRIndex lineEnd = stringIndex + lineRange.length;

    // 현재 라인의 마지막 문자까지 레이아웃
    while (stringIndex < lineEnd) {
      // 1개의 라인에 대해 컨테이너의 실제 영역을 얻어서 다시 처리함(e.g, center hole)
      SRRect proposedRect(layoutPosXY, SRSize(containerSize.width - layoutPosXY.x, lineHeight)); // 현재 컨테이너에 남은 폭으로 생성
      if (proposedRect.width() < 0) {
        //SR_ASSERT(0);
        proposedRect.size.width = 0;
      }
      SRRect remainingRect;
      proposedRect.origin += layoutRect.origin; // lineFragmentRect 에서는 컨테이너상의 좌표가 필요하므로 추가함
      SRRect lineRect = tc->lineFragmentRect(proposedRect, stringIndex, SRWritingDirectionLeftToRight, &remainingRect);
      remainingRect.origin.x -= layoutRect.origin.x; // 컨테이너상의 좌표 -> layoutRect 좌표
      lineRect.origin.x -= layoutRect.origin.x;

      // 컨테이너에서 얻어온 영역에 레이아웃(정렬등도 처리?)
      SRRect fitPath({ 0, 0 }, lineRect.size); // 0, 0 으로 시작해야 한다.
      SRRange fitRange(stringIndex, lineEnd - stringIndex);
      if (fitPath.size.width == 0) {
        // lineRect.size.width 가 0 일 수도 있다(이미지로 시작된 경우)
        int a = 0;
        //fitPath = remainingRect; // 왼쪽에 남은 폭이 없을 경우, remainingRect 영역에 레이아웃시킴
        //layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
      }
      CTFramePtr fitFrame = _framesetter->createFrame(fitRange, fitPath);
      if (fitFrame->getLines().size() != 1) {
        // 생성된 프레임에는 1개 라인만 존재해야함
        SR_ASSERT(0);
        int a = 0;
      }

      CTLinePtr fitLine = fitFrame->getLines().at(0);
      SRSize fitSize = fitLine->getLineSize();
      SRIndex fitLength = fitLine->getStringRange().length;
      SR_ASSERT(fitLength > 0);

      if (fitLength == 1 && fitSize.width > fitPath.width()) {
        // 레이아웃 결과가 1글자도 포함되지 못하는 경우(요청한 폭을 넘는 경우) 다음 위치로 이동
        if (fitPath.height() > lineHeight) {
          SR_ASSERT(0);
          int a = 0;
        }
        // 현재영역에 라인 생성이 실패하면, 다음 위치로 이동
        if (remainingRect.size.width > 0) {
          // 컨테이너의 hole을 건너뜀
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          //layoutPosXY.x -= layoutRect.origin.x; // TODO 임시, 위에서 proposedRect.origin += layoutRect.origin; 하므로
        } else {
          // 개행처리
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
      // fitFrame 에 들어있는 라인(1개)를 outFrame 에 저장.
      if (isLayoutedSameLine) {
        // 마지막 레이아웃된 라인과 동일 라인일 경우, 마지막 라인에 추가시킴
        CTLinePtr lastLine = outFrame->_lines.back(); // 기존 마지막 라인
        CTLinePtr appendLine = fitLine; // 추가되는 라인
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
        const SRFloat appendRunPosX = layoutPosXY.x - lastLineOrg.x; // 추가되는 라인의 런 시작 위치
        for_each(appendLine->_runs.begin(), appendLine->_runs.end(), [&appendRunPosX](CTRunPtr run) {
          for_each(run->_positions.begin(), run->_positions.end(), [&appendRunPosX](SRPoint& pt) {
            pt.x += appendRunPosX;
          });
        });
        lastLine->_runs.insert(lastLine->_runs.end(), appendLine->_runs.begin(), appendLine->_runs.end());
      } else {
        outFrame->_lines.push_back(fitLine);
        SRPoint renderLineOrigin = fitFrame->_lineOrigins[0];
        renderLineOrigin.x += layoutPosXY.x; // fitFrame._frameRect 기준으로 라인 좌표를 처리할까???
        renderLineOrigin.y = layoutPosXY.y;
        outFrame->_lineOrigins.push_back(renderLineOrigin);
      }

      // 다음 레이아웃할 문자 위치로 이동
      stringIndex += fitLength;
      drawnWidth += fitSize.width;

      // outFrame의 레이아웃된 폭 설정
      if (drawnWidth > outSize.width) { outSize.width = drawnWidth; }

      wchar_t lastChar = fitLine->getGlyphRuns().back()->getStringFragment().data().back();
      //if (remainingRect.size.width > 0 /*&& stringIndex < lineEnd*/) {
      if (remainingRect.size.width > 0 && lastChar != '\n') {
        // 개행문자로 끝나지 않고 남은 영역이 있는 경우
        if (stringIndex == lineEnd) {
          // 최초에 생성한 프레임(simpleRectFrame)의 다음 라인을 현재 라인 끝 위치부터 레이아웃시킴
          layoutPosXY.x += fitSize.width; // 레이아웃한 영역(proposedRect)에 공간이 많이 남았을 수 있으므로 fitSize만큼만 추가시킴
        } else {
          // 컨테이너의 hole을 건너뜀
          layoutPosXY = { remainingRect.origin.x, layoutPosXY.y };
          //layoutPosXY.x -= layoutRect.origin.x; // TODO 임시, 위에서 proposedRect.origin += layoutRect.origin; 하므로
        }
      } else {
        SRFloat leftWidth = lineRect.size.width - fitSize.width;
        if (false && leftWidth > 0 && lastChar != '\n') {
          // 현재 남은 영역이 있는 경우, 계속해서 현재 영역에 레이아웃하게 함
          // 이렇게 처리하면, 최초 레이아웃된 라인길이가 유지되면서 짧게 레이아웃되던 현상은 없어지나 워드랩이 안되게 된다.
          // TODO 최초 _framesetter->createFrame() 호출시 이 함수 내에서 라인 생성시마다 컨테이너의 hole을 처리해야겠다!
          layoutPosXY.x += fitSize.width;
        } else {
          // 개행문자로 끝나거나 남은 영역이 없는 경우, 개행처리
          layoutPosXY = { 0.0f, layoutPosXY.y + lineHeight };
          drawnWidth = 0.0;
        }
      }
      outSize.height = layoutPosXY.y; // outFrame의 레이아웃된 높이 설정
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
  // 동일한 kCTParagraphStyleAttributeName 속성 범위를 구한다.
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    textStorage->attribute(kCTParagraphStyleAttributeName, location, attributeRange, rangeLimit));
#endif
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

RenderObjectPtr SRLayoutManager::layoutText(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextBlockPtr textBlock) {
  // layoutRect 사이즈만큼 프레임을 생성
  //CTFramesetterPtr framesetter = CTFramesetter::create(textStorage());
  SRRect rcRoot = parentRender->getRectInRoot();
  rcRoot.origin += layoutRect.origin;
  rcRoot.size = layoutRect.size;
  CTFramePtr outFrame = makeFrame(rcRoot, range, _curTextContainer.lock(), _framesetter);
  SRRange outRange = outFrame->getVisibleStringRange();
  //SR_ASSERT(outRange == range); // outRange < range 경우도 존재함
  if (outRange.length == 0) {
    // layoutRect 크기가 작은 경우 발생하게 된다.
    SR_ASSERT(0);
    return nullptr;
  }
  // 레이아웃 요청 받은 위치에, 프레임 크기로 렌더 객체 생성
  SRRect outRect = layoutRect;
  outRect.size = outFrame->_frameRect.size;
  if (outRect.size.width > layoutRect.size.width) {
    // 끝에 개행문자등을 강제로 포함시켜서 요청한 폭을 넘어갈 수도 있다.
    // 전달한 폭이 작아 1 글자도 안들어갈 경우, 강제로 1글자를 넣은 경우에도 요청한 폭을 넘을 수 있다.
    float www = outFrame->_frameRectWithoutTailingWhiteSpace.width();
    int ccc = outFrame->getShortestLIne()->getGlyphCount();
    if (outFrame->_frameRectWithoutTailingWhiteSpace.width() > layoutRect.size.width
      && outFrame->getShortestLIne()->getGlyphCount() > 1) {
      //SR_ASSERT(0); // 에러 상황임. @@ todo
    }
    outRect.size.width = layoutRect.size.width;
  }
  outRect.size.width = layoutRect.size.width; // TODO 캐럿 표시를 쉽게 처리하기 위함
  RenderTextFramePtr textRender = RenderTextFrame::create(parentRender, outRect);
  textRender->setFrame(outFrame);
  //textRender->setTextBlock(textBlock);
  textRender->setRange(outRange);

  parentRender->addChild(textRender); // 부모 크기도 늘어남

  return textRender;
}

static void _adjustTableRowHeight(RenderTablePtr tableRender, SRIndex rowStartIdx, SRFloat maxPosY) {
  // 셀의 높이를 행의 최대 높이에 맞춰 설정
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

// 전달받은 범위의 블럭이, 부모와 동일한 레벨의 블럭이면 해당 블럭을 리턴
static SRTextBlockPtr _getChildBlock(SRTextStoragePtr textStorage, SRRange range, SRTextBlockPtr parentBlock) {
  const SRTextBlockList& textBlocks = _getTextBlocks(textStorage, range.location);
  for (auto blockIter = textBlocks.begin(); blockIter != textBlocks.end(); ++blockIter) {
    if (SRTextBlock::TableCell == (*blockIter)->getBlockType()) { // SRTextBlockPtr 주소가 동일한 블럭을 찾음(속성 비교 X)
      // 전달받은 범위의 블럭이, 동일한 테이블(parentBlock)에 속한 블럭이면 리턴
      SRTextTableCellBlockPtr resultBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(*blockIter);
      SRTextTablePtr table = std::dynamic_pointer_cast<SRTextTable>(parentBlock);
      if (resultBlock->getTable() == table) {
        return resultBlock;
      }
    } else if (SRTextBlock::ListItem == (*blockIter)->getBlockType()) { // SRTextBlockPtr 주소가 동일한 블럭을 찾음(속성 비교 X)
      // 전달받은 범위의 블럭이, 동일한 리스트(parentBlock)에 속한 블럭이면 리턴
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
  // 테이블 렌더 생성
  const int tableRowCnt = table->getNumberOfRows();
  const int tableColCnt = table->getNumberOfColumns();

  SRRect tableRect(layoutRect.origin, SRSize(layoutRect.width(), 0));
  RenderTablePtr tableRender = RenderTable::create(parentRender, tableRowCnt, tableColCnt, tableRect);
  //tableRender->setTextBlock(table);
  tableRender->setRange(SRRange(range.location, 0)); // 테이블 렌더의 문자열 시작 위치를 지정

  // html spec 에 맞춰서 구현함
  // e.g, <table border="5" cellpadding="0" cellspacing="10" width=100> margin 은 따로 없는듯함
  tableRender->_margin = table->getWidth(SRTextBlock::Margin);
  tableRender->_border = table->getWidth(SRTextBlock::Border);
  tableRender->_padding = table->getWidth(SRTextBlock::Padding);
  const SRFloat tableBorder = tableRender->_border; // 테이블 경계선 두께
  const SRFloat cellSpacing = tableRender->_padding; // 테이블 경계선 ~ 셀 경계선 폭
  const SRSize cellSize = { // cellBorder + cellPadding 포함된 크기임
    (layoutRect.width() - 2 * (tableBorder) - (tableColCnt + 1) * cellSpacing) / tableColCnt,
    layoutRect.height() - 2 * (tableBorder) - 2 * cellSpacing
  };

  // 첫 번째 셀 레이아웃 시작 위치(테이블 시작위치 기준의 셀 경계선 시작위치)
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
      // range.location 위치로 현재 테이블의 셀블럭을 획득
      // 테이블의 끝 부분에 colspan, rowspan 존재시 cellBlock 은 nullptr 이 가능함
      SRTextTableCellBlockPtr cellBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(
        _getChildBlock(textStorage(), range, table));
      //SR_ASSERT(cellBlock);

      // 빈 셀은 존재하면 안된다.
      if (cellBlock->_startingRow > row
        || (cellBlock->_startingRow == row && cellBlock->_startingColumn > col)) {
        //SR_ASSERT(0);
        int a = 0;
        //SRStringPtr text = SRString::create(L"\n");
        //textStorage()->replaceCharacters(SRRange(range.location, 0), text);
        //range.length = 1;
      }

      if (dummyTable[row][col].parentCell) {
        // rowspan, colspan 범위일 경우, 빈 렌더를 생성
        RenderTableCellPtr cellRender = RenderTableCell::create(tableRender, row, 0, col, 0
          , SRRect(layoutXY, SRSize(cellSize.width, 0)));
        cellRender->_parentCell = dummyTable[row][col].parentCell;
        tableRender->addChild(cellRender);
        // 다음 col 로 이동
        layoutXY.x += (cellSize.width + cellSpacing); // 다음 컬럼 위치로 이동
        continue;
      }

      // 셀 영역 획득
      SRRange effRange = textStorage()->rangeOfTextBlock(cellBlock, range.location);
      range.length = effRange.rangeMax() - range.location;

      // 셀 내의 마지막 문자는 개행문자여야 한다(like ms-word)
      SRIndex cellLastIndex = range.location;
      if (range.length > 0) {
        cellLastIndex = range.rangeMax() - 1;
      } else {
        int a = 0;
      }
      wchar_t cellLastChar = textStorage()->getString()->data().at(cellLastIndex);
      if (cellLastChar != '\n') { // 셀 문자열 끝에 개행문자 추가
        SRStringPtr text = SRString::create(L"\n");
        textStorage()->replaceCharacters(SRRange(cellLastIndex + 1, 0), text);
        range.length += 1;
      }

      _ensureMinimunSize(const_cast<SRSize&>(cellSize));
      RenderTableCellPtr cellRender = RenderTableCell::create(tableRender, row, cellBlock->_rowSpan, col, cellBlock->_columnSpan
        , SRRect(layoutXY, cellSize));

      // rowspan, colspan 범위에 해당되는 테이블 내의 위치를 저장
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

      SRRect layoutRect(SRPoint(0, 0), cellSize); // 셀 내의 (0, 0) 위치에 레이아웃

      layoutRect.size.width *= colSpan; // 레이아웃 폭만 colSpan 배수만큼 늘림(rowSpan 라인의 높이는 0이므로 상관없음)

      // 셀 컨텐츠의 경계선, 패딩은 layoutBlock() 함수내에서 처리가 된다.
      layoutBlock(cellRender, layoutRect.size, range, cellBlock);

      // 셀의 폭을 원래 폭으로 설정(레이아웃된 크기는 컨텐츠 크기 만큼이므로 원래 크기 이하여야 함)
      SRRect outRect = cellRender->getRect();
      if (outRect.width() > cellSize.width || outRect.height() > cellSize.height) {
        // 개행문자만큼 폭이 더 클수도 있다.
        //SR_ASSERT(0);
      }
      outRect.size.width = cellSize.width;
      cellRender->setRect(outRect);

      if (cellRender->getRect().height() > maxPosY) {
        maxPosY = cellRender->getRect().height();
      }

      tableRender->addChild(cellRender);

      // 다음 col 로 이동
      layoutXY.x += (cellSize.width + cellSpacing); // 다음 컬럼 위치로 이동
      range.location = tableRender->getRange().rangeMax(); // 레이아웃 완료된 다음 위치로 이동
    }

    // 셀의 높이를 행의 최대 높이에 맞춰 설정
    _adjustTableRowHeight(tableRender, rowStartIdx, maxPosY);
    rowStartIdx += tableColCnt;

    // 다음 row 로 이동
    layoutXY.x = cellStartXY.x;
    layoutXY.y += maxPosY + cellSpacing;
  }

  RenderObjectList cells = tableRender->getChildren();
  SR_ASSERT(tableRender->_numberOfRows * tableRender->_numberOfColumns == cells.size());

  // rowspan, colspan 로 인해 생성된 빈 렌더들 삭제
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

  // tableRender 의 크기는 위에서 셀이 추가되면서 늘어난 상태이므로 따로 설정을 안함
  tableRect.size.height = layoutXY.y + tableBorder; // layoutXY.y는 다음 라인의 시작위치이므로 테이블 경계선 두께만 추가함
  tableRender->setRect(tableRect); // 테이블 크기를 테이블 경계선 두께만큼 더 키운다.

  parentRender->addChild(tableRender); // 테이블 레이아웃이 완료되고 부모에게 추가한다.
  return tableRender;
}

RenderObjectPtr SRLayoutManager::layoutList(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextListPtr list) {
  // 렌더 생성
  //layoutRect.origin.x += 260; // @@test
  //layoutRect.size.width -= 260;
  SRRect listRect(layoutRect.origin, SRSize(layoutRect.width(), 0));
  RenderListPtr listRender = RenderList::create(parentRender, listRect);
  //listRender->setTextBlock(list);
  listRender->setRange(SRRange(range.location, 0)); // 테이블 렌더의 문자열 시작 위치를 지정

  SRPoint layoutXY(0, 0);
  const SRInt rangeMax = range.rangeMax();
  SRInt itemNumber = 1;

  while (range.location < rangeMax) {
    // range.location 위치로 현재 테이블의 셀블럭을 획득
    SRTextListBlockPtr listBlock = std::dynamic_pointer_cast<SRTextListBlock>(
      _getChildBlock(textStorage(), range, list));
    SR_ASSERT(listBlock);

    // 셀 영역 획득
    SRRange effRange = textStorage()->rangeOfTextBlock(listBlock, range.location);
    range.length = effRange.rangeMax() - range.location;

    // list header
    UIFontPtr listHeadFont = std::dynamic_pointer_cast<UIFont>(textStorage()->attribute(kCTFontAttributeName, range.location));
    //SRRect rcListHead(layoutXY, SRSize(25, 25)); // @@todo 블릿 사이즈만큼으로 설정
    SRRect rcListHead(layoutXY, SRSize(listHeadFont->lineHeight() * 2, listHeadFont->lineHeight())); // 현재 폰트 크기로 지정
    SRString marker = list->markerForItemNumber(itemNumber);
    RenderListHeaderPtr listHeadRender = RenderListHeader::create(listRender, rcListHead, marker);
    listRender->addChild(listHeadRender);
    layoutXY.x += rcListHead.size.width;

    // layoutXY 는 테이블 이내의 영역을 차지함((0, 0) 부터 시작)
    SRSize listItemSize(layoutRect.size.width - layoutXY.x, layoutRect.size.height - layoutXY.y);
    _ensureMinimunSize(listItemSize);
    RenderListItemPtr listItemRender = RenderListItem::create(listRender, SRRect(layoutXY, listItemSize));

    // 셀 컨텐츠의 경계선, 패딩은 layoutBlock() 함수내에서 처리가 된다.
    layoutBlock(listItemRender, listItemSize, range, listBlock);
    listRender->addChild(listItemRender);

    range.location = listRender->getRange().rangeMax();

    layoutXY.x = 0;
    layoutXY.y = listItemRender->getRect().bottom();

    ++itemNumber;
  }

  parentRender->addChild(listRender); // 테이블 레이아웃이 완료되고 부모에게 추가한다.
  return listRender;
}

static void _ensure_size(SRSize& size) {
  if (size.width <= 0)
    size.width = 1;
  if (size.height <= 0)
    size.height = 1;
}

// 다음번 개행문자 바로 다음 위치를 리턴함
static SRIndex _next_paragraph(const SRTextStoragePtr& storage, SRIndex pos, SRIndex endPos) {
  const std::wstring str = storage->getString()->data();
  static const int LINE_FEED_CHAR = 10; // '\n'
  static const int CARRIAGE_RETURN_CHAR = 13; // '\r'

  while (pos < endPos) {
    wchar_t ch = str.at(pos++);
    SR_ASSERT(ch != CARRIAGE_RETURN_CHAR);
    if (ch == LINE_FEED_CHAR) { // 현재 \r 만 단독으로 사용되는 경우는 고려하지 않음
      break;
    }
  }
  return pos;
}

// parentRender 가 포함한 targetBlock 레벨의 블럭들을 부모의 layoutRect 영역에 레이아웃함
// 부모의 layoutRect 영역에 부모의 패딩을 적용하여 레이아웃함
// 최초로 루트 블럭부터 호출되며, 테이블 셀, 리스트 아이템에 대해서도 호출됨
// 루트 렌더의 경우, 여러개의 렌더들이 생성되며, 현재 마지막 렌더를 리턴하고 있다.
RenderObjectPtr SRLayoutManager::layoutBlock(RenderObjectPtr parentRender, SRSize requestlayoutSize
  , SRRange layoutRange, const SRTextBlockPtr targetBlock) {
  SR_ASSERT(parentRender);
  parentRender->setRange(SRRange(layoutRange.location, 0)); // 이후 RenderBlock::addChild() 시에 자식의 문자열 범위만큼 늘어남
  //parentRender->setTextBlock(targetBlock);

  // 현재 레이아웃되는 뷰에 대해서만 targetBlock 의 margin, border, padding 을 적용시킨다.
  // 현재 뷰의 자식뷰들은 부모뷰 내에서 margin + border + padding 만큼 안쪽에 위치시킨다.
  parentRender->_margin = targetBlock->getWidth(SRTextBlock::Margin);
  parentRender->_border = targetBlock->getWidth(SRTextBlock::Border);
  parentRender->_padding = targetBlock->getWidth(SRTextBlock::Padding);

  // targetBlock의 좌상단 패딩 적용(컨텐츠가 레이아웃할 위치 지정)
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

  SRRect layoutedRect(SRPoint(0, 0), SRSize(0, 0)); // 레이아웃 완료된 영역 저장용
  RenderObjectPtr lastLayoutedRender = nullptr;

  //const SRIndex layoutRangeMax = layoutRange.rangeMax(); // 레이아웃할 end index
  SRIndex layoutRangeMax = layoutRange.rangeMax(); // 레이아웃할 end index
  while (layoutRange.location < layoutRangeMax) {
    SR_ASSERT(layoutRange.location >= 0);
    if (layoutXY.y >= layoutRect.bottom()) {
      // 레이아웃할 공간이 안남음
      SR_ASSERT(0);
      return lastLayoutedRender;
    }
    // 현재 위치의 텍스트 블럭 리스트를 획득
    const SRTextBlockList& textBlocks = _getTextBlocks(textStorage(), layoutRange.location);
    SR_ASSERT(textBlocks.size() > 0);

    // 부모 텍스트 블럭(targetBlock)과 동일한 레벨의 블럭을 찾아 해당 블럭을 레이아웃함
    bool isLeafBlock = true;
    if (textBlocks.size() > 1 && textBlocks.back() != targetBlock) {
      isLeafBlock = false; // 마지막 블럭이 아닌 경우임
    }

    if (isLeafBlock) {
      // leaf block 인 경우, 개행문자까지의 범위를 그린다.
      SRIndex nextParaIndex = _next_paragraph(textStorage(), layoutRange.location, layoutRangeMax);
      SRRange textRange(layoutRange.location, nextParaIndex - layoutRange.location);
      SRRect textRect(layoutXY, layoutSize);
      // 경계선을 포함한 영역을 지정함
      //const SRFloat border = targetBlock->getWidth(SRTextBlock::Border);
      //textRect = textRect.insetBy(-border, -border);
      lastLayoutedRender = layoutText(parentRender, textRect, textRange, targetBlock);
    } else {
      // leaf block 이 아니면, 현재 블럭의 inner block 으로 진입
      auto iterInnerBlock = std::find(textBlocks.begin(), textBlocks.end(), targetBlock);
      SR_ASSERT(iterInnerBlock != textBlocks.end());

      SRTextBlockPtr innerBlock = *(++iterInnerBlock);
      SRTextBlock::BlockType innerBlockType = innerBlock->getBlockType();

      if (innerBlockType == SRTextBlock::TableCell) { // 현재 Text, TableCell 순서임
                                                      // 현재 블럭의 inner block 이 테이블 셀(첫번째 셀)일 경우, 테이블 레이아웃
        SRTextTableCellBlockPtr child = std::dynamic_pointer_cast<SRTextTableCellBlock>(innerBlock);
        SRTextTablePtr parent = child->getTable();
        SRRange parentRange = textStorage()->rangeOfTextTable(parent, layoutRange.location); // 전체 테이블 영역
        SRRect layoutBlock(layoutXY, layoutSize);

        if (1) {
          // TODO inner table 삽입시 폭이 매우 작게(거의 0?)으로 만들어지고 있어서 일단 비활성화시킴
          // 컨테이너 제외영역 처리
          SRRect proposedRect(layoutBlock);
          SRRect remainingRect;
          SRTextContainerPtr tc = _curTextContainer.lock();
          SRBool parentIsRoot = (typeid(*parentRender) == typeid(RenderRoot));
          if (parentIsRoot && !tc->isSimpleRectangularTextContainer()) {
            // 루트 렌더 바로 하위 자식 렌더인 경우에만 컨테이너 excludePath를 처리한다.
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
          // 셀 마지막 문자 위치에 개행문자를 추가한 경우임
          layoutRangeMax += (lastLayoutedRender->getRange().rangeMax() - parentRange.rangeMax());
        }
        //if (0) {
        //  // 컨테이너 제외영역 처리
        //  layoutXY = layoutRect.origin;
        //  layoutSize = layoutRect.size;
        //}
      } else if (innerBlockType == SRTextBlock::ListItem) {
        // 현재 블럭의 inner block 이 리스트 아이템(첫번째 아이템)일 경우, 리스트 레이아웃
        SRTextListBlockPtr child = std::dynamic_pointer_cast<SRTextListBlock>(innerBlock);
        SRTextListPtr parent = child->getList();
        SRRange parentRange = textStorage()->rangeOfTextList(parent, layoutRange.location); // 전체 테이블 영역
        SRRect layoutBlock(layoutXY, layoutSize);

        if (1) {
          // 컨테이너 제외영역 처리
          SRRect proposedRect(layoutBlock);
          SRRect remainingRect;
          SRTextContainerPtr tc = _curTextContainer.lock();
          SRBool parentIsRoot = (typeid(*parentRender) == typeid(RenderRoot));
          if (parentIsRoot && !tc->isSimpleRectangularTextContainer()) {
            // 루트 렌더 바로 하위 자식 렌더인 경우에만 컨테이너 excludePath를 처리한다.
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
          // 셀 마지막 문자 위치에 개행문자를 추가한 경우임
          layoutRangeMax += (lastLayoutedRender->getRange().rangeMax() - parentRange.rangeMax());
        }
      } else if (innerBlockType == SRTextBlock::Text) {
        // <div> 에 해당??? 이 경우에는 innerBlock 를 갖는 parentRender 를 만들어서 layoutBlock()를 호출함???
        SR_ASSERT(0);
        //SRRect textRect(layoutRect.origin, SRSize(0, 0));
        //RenderTextBlockPtr textRender = RenderTextBlock::create(parentRender, textRect);
        ////textRender->setTextBlock(innerBlock);
        //outRender = layoutBlock(textRender, layoutRect, range, innerBlock);
      } else {
        SR_ASSERT(0);
      }
    }

    // 다음 라인으로 이동
    SR_ASSERT(lastLayoutedRender);
    layoutRange.location = parentRender->getRange().rangeMax();

    SRRect outRect = lastLayoutedRender->getRect();
    //layoutRect.origin.x = 0; // 리스트의 경우, 2번째 라인 이후도 좌측 여백이 있어야 한다.
    //layoutXY.y = outRect.bottom();
    if (layoutXY != outRect.origin) {
      int a = 0;
    }
    layoutXY.y += outRect.height(); // ???
    layoutSize.height = layoutRect.bottom() - layoutXY.y; // 레이아웃된 크기만큼 레이아웃할 높이를 줄인다.

    layoutedRect += outRect;
  }

  // targetBlock의 우하단 패딩 적용
  layoutedRect += totalPadding;

  // rcParent.origin 은 그대로 유지시키고, 레이아웃된 크기만큼으로 설정함(원래 크기보다 줄어들수 있음)
  SRRect rcParent = parentRender->getRect();
  rcParent.size = layoutedRect.size;
  parentRender->setRect(rcParent);

  return lastLayoutedRender; // 마지막 레이아웃한 렌더를 리턴
}

void SRLayoutManager::layoutAllText() {
  // Typesetter.layoutCharactersInRange(range, rect, layoutMgr) ?
  // NSTypesetter:layoutGlyphsInLayoutManager 참고
  // (NSRange)layoutCharactersInRange:(NSRange)characterRange 
  // forLayoutManager:(NSLayoutManager *)layoutManager
  // maximumNumberOfLineFragments : (NSUInteger)maxNumLines;

  SRIndex curTextIdx = 0;
  _framesetter = CTFramesetter::create(textStorage()); // 매번 전체 레이아웃 시작시 초기화
  SR_ASSERT(_textContainers.size() == 1); // 현재는 컨테이너 1개만 지원함(이걸 여러 텍스트뷰에 나눠서 그리고 있음)

  // 아래에서 _delegate 호출시 컨테이너가 추가될 경우, it 가 변경되게 되므로 Range-based for 를 사용하지 않음
  for (auto it = _textContainers.begin(); it != _textContainers.end(); ++it) {
    // 중간 컨테이너 레이아웃이 변경되면, 이후 컨테이너들은 전부 갱신돼야 한다.
    SRTextContainerPtr tc = *it;
    SRIndex tcStartTextIdx = curTextIdx; // 컨테이너에 포함된 문자열의 시작 인덱스

    // 현재 컨테이너에 레이아웃시킬 남은 문자열 범위(tcStartTextIdx ~ 문자열 끝)
    SRRange tcTextRange(tcStartTextIdx, textStorage()->getLength() - tcStartTextIdx);

    const SRTextBlockPtr tcStartTextBlock = _getTextBlocks(textStorage(), tcStartTextIdx).front();

    if (1) {
      // 컨테이너 시작 위치의 텍스트 블럭 범위를 획득하여, tcTextRange 길이를 제한시킴
      SRRange effRange = textStorage()->rangeOfTextBlock(tcStartTextBlock, tcStartTextIdx);
      SR_ASSERT(tcTextRange.length == effRange.rangeMax() - tcTextRange.location); // 항상 동일하므로 현재 블럭은 사실상 불필요함
      tcTextRange.length = effRange.rangeMax() - tcTextRange.location;
    }

    // 현재 레이아웃중인 컨테이너 저장(layoutText() 함수에서만 사용되고 있음)
    _curTextContainer = tc;

    // 컨테이너의 root render 생성
    const RenderRootPtr rootRender = RenderRoot::create();
    tc->setRootRender(rootRender);

    // rootRender 하위에 현재 컨테이너의 폭 안에 레이아웃시킴
    layoutBlock(rootRender, SRSize(tc->size().width, SRFloatMax), tcTextRange, tcStartTextBlock);

    // 레이아웃 요청한 폭 이내로 생성됐는지 확인(높이는 무제한으로 레이아웃하고 있음)
    if (rootRender->getRect().width() > tc->size().width) {
      int a = 0;
      SR_ASSERT(0); // 발생중임. 확인필요!
    }

    // 레이아웃된 문자열 범위를 획득함
    SRRange outRange = rootRender->getRange();
    if (tcTextRange != rootRender->getRange()) {
      //SR_ASSERT(0);
      int a = 0; // 테이블 셀 마지막 문자로 개행문자가 추가된 경우 발생 가능함
    }

    // 레이아웃된 렌더 영역 저장
    _usedRect = rootRender->getRect();

    // 컨테이너의 _exclusionPaths 영역도 포함시킴
    _usedRect += tc->getTotalExclusionPath();

    // 컨테이너에 레이아웃된 문자열 범위 저장
    tc->setTextRange(SRRange(tcStartTextIdx, outRange.rangeMax() - tcStartTextIdx));

    // 다음 레이아웃할 문자 위치로 이동
    curTextIdx = outRange.rangeMax();

    if (_delegate) {
      // 컨테이너 레이아웃이 끝날때마다, delegate 호출
      bool atEnd = false;
      if (tc == _textContainers.back()) {
        atEnd = true; // 마지막 컨테이너이면 atEnd = true;
      }

      // iterator invalidation rules for C++ containers 때문에 _delegate 에서 컨테이너 추가시 it 가 무효화됨
      int tcIdx = it - _textContainers.begin();
      _delegate->didCompleteLayoutFor(this->shared_from_this(), tc, atEnd);
      it = _textContainers.begin() + tcIdx;
    }
  } // for _textContainers
}

void SRLayoutManager::addTextContainer(const SRTextContainerPtr textContainer) {
  _textContainers.push_back(textContainer);
  SR_ASSERT(_textContainers.size() == 1); // 현재는 레이아웃 매니저당 1개의 컨테이너만 지원함
  textContainer->setLayoutManager(SRLayoutManagerPtr(this->shared_from_this()));
}

void SRLayoutManager::drawBackground(const SRRange& glyphsToShow, const SRPoint& origin) {
  SR_NOT_IMPL();
}

void SRLayoutManager::drawGlyphs(const SRRange& glyphsToShow, const SRPoint& origin) {
  SR_NOT_IMPL();
}

} // namespace sr
