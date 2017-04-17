#pragma once

#include "SRObject.h"
#include "SRColor.h"
#include "SRString.h"

namespace sr {

class SRTextBlock;
using SRTextBlockPtr = std::shared_ptr<SRTextBlock>;

// represent a block of text laid out in a subregion of the text container.
// Text blocks appear as attributes on paragraphs, as part of the paragraph style.
class SRTextBlock : public SRObject {
  SR_MAKE_NONCOPYABLE(SRTextBlock);
public:
  SR_DECL_CREATE_FUNC(SRTextBlock);

  typedef enum {
    Margin,   // 뷰 경계선 ~ 컨텐츠 경계선
    Border,   // 컨텐츠 경계선 두께
    Padding,  // 컨텐츠 경계선 ~ 컨텐츠
    LayerMax
  } LayerType;

  //typedef enum {
  //  Absolute,
  //  Percentage,
  //  ValueMax
  //} ValueType;

  typedef enum {
    Text,
    Table,
    TableCell,
    List,
    ListItem
  } BlockType;

  SRTextBlock(BlockType blockType);
  virtual ~SRTextBlock();

  //func rectForLayout(at: NSPoint, in : NSRect, textContainer : NSTextContainer, characterRange : NSRange) -> NSRect
  //func boundsRect(forContentRect: NSRect, in : NSRect, textContainer : NSTextContainer, characterRange : NSRange)
  //func setContentWidth(SRFloat, type: NSTextBlockValueType)
  //virtual SRRect rectForLayout(SRPoint at, SRRect in, SRTextContainerPtr textContainer, SRRange characterRange);

  void setWidth(SRFloat value, LayerType type); // ValueType 은 추후 value 값의 +/- 로 판단하자!
  SRFloat getWidth(LayerType type) const;
  SRFloat getWidthAll() const;

  void setBackgroundColor(SRColor color);
  SRColor getBackgroundColor() const;
  void setBorderColor(SRColor color);
  SRColor getBorderColor() const;

  BlockType getBlockType() const { return _blockType; }

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  SRColor _backgroundColor;
  SRColor _borderColor;
  SRFloat _width[LayerMax];
  BlockType _blockType;
};

class SRTextTable;
using SRTextTablePtr = std::shared_ptr<SRTextTable>;

// represents a text table as a whole. It is responsible for laying out and drawing the text table blocks it contains,
// and it maintains the basic parameters of the table.
class SRTextTable : public SRTextBlock { // NSTextTable 도 NSTextBlock 을 상속받음
  SR_MAKE_NONCOPYABLE(SRTextTable);
public:
  SR_DECL_CREATE_FUNC(SRTextTable);

  SRTextTable();
  virtual ~SRTextTable();

  void setNumberOfColumns(int numberOfColumns) {
    _numberOfColumns = numberOfColumns;
  }
  SRInt getNumberOfColumns() const { return _numberOfColumns; }
  void setNumberOfRows(int numberOfRows) {
    _numberOfRows = numberOfRows;
  }
  SRInt getNumberOfRows() const { return _numberOfRows; }

//private:
  SRInt _numberOfColumns;
  SRInt _numberOfRows; //add

protected:
  virtual bool isEqual(const SRObject& obj) const override;
};

class SRTextTableCellBlock;
using SRTextTableCellBlockPtr = std::shared_ptr<SRTextTableCellBlock>;

// represents a text block that appears as a cell in a text table.
class SRTextTableCellBlock : public SRTextBlock {
  SR_MAKE_NONCOPYABLE(SRTextTableCellBlock);
public:
  //friend SRLayoutManager; //error C2433: 'sr::SRLayoutManager': 데이터 선언에 'friend'을(를) 사용할 수 없습니다.
  SR_DECL_CREATE_FUNC(SRTextTableCellBlock);

  SRTextTableCellBlock(SRTextTablePtr table, int startingRow, int rowSpan, int startingColumn, int columnSpan);
  virtual ~SRTextTableCellBlock();

  SRTextTablePtr getTable() const { return _table; }

//private:
  SRTextTablePtr _table;
  SRInt _startingRow;
  SRInt _rowSpan;
  SRInt _startingColumn;
  SRInt _columnSpan;

protected:
  virtual bool isEqual(const SRObject& obj) const override;
};

class SRTextList;
using SRTextListPtr = std::shared_ptr<SRTextList>;

// represent a block of text laid out in a subregion of the text container.
// Text blocks appear as attributes on paragraphs, as part of the paragraph style.
class SRTextList : public SRTextBlock {
  SR_MAKE_NONCOPYABLE(SRTextList);
public:
  SR_DECL_CREATE_FUNC(SRTextList);

  typedef enum {
    BOX, // NSTextList는 NSString 타입이며, gnustep은 "{box}" 형식을 취하고 있음
    CHECK,
    CIRCLE,
    HYPHEN,
    LOWER_ROMAN,
    UPPER_ROMAN,
    DECIMAL,
  } MarkerFormat;

  SRTextList(MarkerFormat markerFormat);
  virtual ~SRTextList();

  SRString markerForItemNumber(SRInt item) const;

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  MarkerFormat _markerFormat;
  //SRUInt _listOptions;
};

class SRTextListBlock;
using SRTextListBlockPtr = std::shared_ptr<SRTextListBlock>;

class SRTextListBlock : public SRTextBlock {
  SR_MAKE_NONCOPYABLE(SRTextListBlock);
public:
  SR_DECL_CREATE_FUNC(SRTextListBlock);

  SRTextListBlock(SRTextListPtr list);
  virtual ~SRTextListBlock();

  SRTextListPtr getList() const { return _list; }

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  SRTextListPtr _list;
};

} // namespace sr
