#include "SRTextBlock.h"

namespace sr {

SRTextBlock::SRTextBlock(BlockType blockType)
: _blockType(blockType) {
  memset(_width, 0, sizeof(_width));
}

SRTextBlock::~SRTextBlock() {
}

bool SRTextBlock::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const SRTextBlock&>(obj);
  if (getBackgroundColor() != other.getBackgroundColor() ||
    getBorderColor() != other.getBorderColor() ||
    getWidth(Padding) != other.getWidth(Padding) ||
    getWidth(Border) != other.getWidth(Border) ||
    getWidth(Margin) != other.getWidth(Margin)) {
    return false;
  }
  return true;
}

//SRRect SRTextBlock::rectForLayout(SRPoint at, SRRect in, SRTextContainerPtr textContainer, SRRange characterRange) {
//  // SRTextContainer::lineFragmentRect()�� ������ ����ϱ� ������ �ణ �ٸ�?
//  // ���⼭�� �����̳��� hole ���� �Ű澲�� ����, Frame �����ÿ��� �����Ű��!
//  // SRRect lineRect = tc->lineFragmentRect(proposedRect, stringIndex, SRWritingDirectionLeftToRight, &remainingRect);
//  return in;
//}

void SRTextBlock::setWidth(SRFloat value, LayerType type) {
  _width[type] = value;
}

SRFloat SRTextBlock::getWidth(LayerType type) const {
  return _width[type];
}

SRFloat SRTextBlock::getWidthAll() const {
  SRFloat total = getWidth(Margin);
  total += getWidth(Border);
  total += getWidth(Padding);
  return total;
}

void SRTextBlock::setBackgroundColor(SRColor color) {
  _backgroundColor = color;
}

SRColor SRTextBlock::getBackgroundColor() const {
  return _backgroundColor;
}

void SRTextBlock::setBorderColor(SRColor color) {
  _borderColor = color;
}

SRColor SRTextBlock::getBorderColor() const {
  return _borderColor;
}


SRTextTable::SRTextTable()
: SRTextBlock(BlockType::Table), _numberOfColumns(0), _numberOfRows(0) {
}

SRTextTable::~SRTextTable() {
}

bool SRTextTable::isEqual(const SRObject& obj) const {
  SR_ASSERT(0);
  // ���̺� ��ü���� ������ ���� �ٸ� ��ü�� ������(�Ǵ� id �� �Ǻ�???)
  return SRObject::isEqual(obj);
}

SRTextTableCellBlock::SRTextTableCellBlock(SRTextTablePtr table, int startingRow, int rowSpan, int startingColumn, int columnSpan)
: SRTextBlock(BlockType::TableCell), _table(table), _startingRow(startingRow), _rowSpan(rowSpan)
, _startingColumn(startingColumn), _columnSpan(columnSpan) {
}

SRTextTableCellBlock::~SRTextTableCellBlock() {
}

bool SRTextTableCellBlock::isEqual(const SRObject& obj) const {
  //SR_ASSERT(0);
  const auto& other = static_cast<const SRTextTableCellBlock&>(obj);
  if (!SRTextBlock::isEqual(obj)) return false; // �θ� �ٸ��� �ڽĵ� �ٸ��� ó����
  if (_table != other._table ||
    _startingRow != other._startingRow ||
    _rowSpan != other._rowSpan ||
    _startingColumn != other._startingColumn ||
    _columnSpan != other._columnSpan) {
    return false;
  }
  return true; // ���� ���̺��� ����Ű��, row, col ���� ������ ���� ��ü�� ���???
}

//SRRect SRTextTableCellBlock::rectForLayout(SRPoint at, SRRect in, SRTextContainerPtr textContainer, SRRange characterRange) {
//  SRInt cols = _table->getNumberOfColumns();
//  SRSize colSize(in.width() / cols, in.height());
//  return SRRect(in.origin, colSize);
//}

SRTextList::SRTextList(MarkerFormat markerFormat)
  : SRTextBlock(BlockType::List), _markerFormat(markerFormat) {
}

SRTextList::~SRTextList() {
}

bool SRTextList::isEqual(const SRObject& obj) const {
  SR_ASSERT(0);
  // ���̺� ��ü���� ������ ���� �ٸ� ��ü�� ������(�Ǵ� id �� �Ǻ�???)
  return SRObject::isEqual(obj);
}

static std::wstring _replace_string(std::wstring& subject, const std::wstring& search, const std::wstring& replace) {
  size_t pos = 0;
  while ((pos = subject.find(search, pos)) != std::wstring::npos) {
    subject.replace(pos, search.length(), replace);
    pos += replace.length();
  }
  return subject;
}

SRString SRTextList::markerForItemNumber(SRInt item) const {
  SRString marker;
  switch (_markerFormat) {
  case BOX:
    marker = L"\x25A1";
    break;
  case CIRCLE:
    marker = L"\x25CF"; // 25CF(Black Circle), 25E6(White Bullet)
    break;
  case DECIMAL:
    marker += item;
    marker += L")";
    break;
  }
  SR_ASSERT(marker.length() > 0);
  return marker;
}

SRTextListBlock::SRTextListBlock(SRTextListPtr list)
  : SRTextBlock(BlockType::ListItem), _list(list) {
}

SRTextListBlock::~SRTextListBlock() {
}

bool SRTextListBlock::isEqual(const SRObject& obj) const {
  //SR_ASSERT(0);
  const auto& other = static_cast<const SRTextListBlock&>(obj);
  if (!SRTextBlock::isEqual(obj)) {
    // �θ� �ٸ��� �ڽĵ� �ٸ��� ó����
    return false;
  }
  if (_list != other._list) {
    return false;
  }
  //return true; // �Ӽ��� ������ ������ ������ ó���� --> bullet �� 1���� ǥ�õǰ� �ȴ�.
  return false; // ���� �ּҰ� �ƴ� �̻� ���� ���� ����??? ������ ���� id ���� �ʿ�???
}

} // namespace sr
