#pragma once

#include "SRObject.h"
#include "SRTextBlock.h"
#include "CTFrame.h"
#include "../CoreGraphics/CGContext.h"
#include "../UIKit/UIEvent.h"
#include "../UIKit/UITextInput.h"
#include <vector>

namespace sr {

int __sr_debug_render_cnt();

class RenderRoot;
using RenderRootPtr = std::shared_ptr<RenderRoot>;

class RenderObject;
using RenderObjectPtr = std::shared_ptr<RenderObject>;
using RenderObjectWeakPtr = std::weak_ptr<RenderObject>;

using RenderObjectList = std::vector<RenderObjectPtr>;

class RenderObject : public SRObject, public std::enable_shared_from_this<RenderObject> {
  SR_MAKE_NONCOPYABLE(RenderObject);
public:
  SR_DECL_CREATE_FUNC(RenderObject);

  RenderObject(RenderObjectPtr parent, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderObject();
  virtual void draw(CGContextPtr ctx) const;
  SRPoint convertTo(SRPoint point, RenderObjectPtr to = nullptr) const;
  // 클릭된 좌표를 컨텐츠 영역 이내로 맞춤
  virtual SRPoint fitToContent(SRPoint point, UITextLayoutDirection direction = none) const;

  void beginDraw(CGContextPtr ctx) const;
  void endDraw(CGContextPtr ctx) const;

  virtual const RenderObjectPtr hitTest(SRPoint point, UITextLayoutDirection direction = none)/* const*/;
  virtual SRBool pointInside(SRPoint point) const;

  virtual SRTextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition())/* const*/ {
    SR_ASSERT(0);
    return SRTextPosition();
  }
  virtual RenderObjectPtr getRenderByIndex(SRTextPosition position);
  virtual SRRect caretRect(SRTextPosition position) const;
  virtual SRRange lineRange(SRTextPosition position) const;

  RenderRootPtr root() const {
    RenderObjectPtr v = _parent.lock();
    while (v && v->parent()) {
      v = v->parent();
    }
    return std::dynamic_pointer_cast<RenderRoot>(v);
  }

  RenderObjectPtr parent() const { return _parent.lock(); }
  void removeFromParent();

  void addChild(RenderObjectPtr child);
  RenderObjectList& getChildren() { return _children; }
  const RenderObjectList& getChildren() const { return _children; }

  void setRect(SRRect rect);
  SRRect getRect() const { return _rect; }

  SRRect getRectInRoot() const;
  void setSize(SRFloat w, SRFloat h) {
    if (w >= 0.0) { _rect.size.width = w; }
    if (h >= 0.0) { _rect.size.height = h; }
  }
  void insetBy(SRFloat dx, SRFloat dy) { _rect = _rect.insetBy(dx, dy); }
  void setRange(SRRange range);
  SRRange getRange() const { return _astrRange; }

  virtual RenderObjectPtr findRender(UITextLayoutDirection direction, SRTextPosition position);

  // SRTextBlock 의 값을 복사해옴
  SRFloat _margin;
  SRFloat _border;
  SRFloat _padding;

  RenderObjectWeakPtr _prev, _next;

private:
  RenderObjectWeakPtr _parent;
  RenderObjectList _children;
  SRRect _rect; // 부모 뷰 내에서의 절대좌표 위치
  SRRange _astrRange; // 문자열 범위. 레이아웃된 범위 저장.
};

// 루트 렌더
//
class RenderRoot : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderRoot);
public:
  SR_DECL_CREATE_FUNC(RenderRoot);

  RenderRoot(SRRect rect = { 0, 0, 0, 0 }) : RenderObject(nullptr, rect) {}
  virtual ~RenderRoot() = default;
  virtual void draw(CGContextPtr ctx) const override {
    RenderObject::draw(ctx); // for debug
  }
  virtual SRTextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) override;

  std::vector<std::pair<SRFloat, SRRange>> _pageBreakPos;
};

class RenderTextFrame;
using RenderTextFramePtr = std::shared_ptr<RenderTextFrame>;

// 한 문단을 나타내는 Text Frame 렌더
// CTFrame 을 포함함
//
class RenderTextFrame : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderTextFrame);
public:
  SR_DECL_CREATE_FUNC(RenderTextFrame);

  RenderTextFrame(RenderObjectPtr parent, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderTextFrame() = default;
  virtual void draw(CGContextPtr ctx) const override;

  virtual SRTextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) override;
  virtual SRRect caretRect(SRTextPosition position) const override;
  virtual SRRange lineRange(SRTextPosition position) const override;

  void setFrame(CTFramePtr frame);

protected:
  CTFramePtr _frame;
};

// Table
//
class RenderTable;
using RenderTablePtr = std::shared_ptr<RenderTable>;

class RenderTable : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderTable);
public:
  SR_DECL_CREATE_FUNC(RenderTable);

  RenderTable(RenderObjectPtr parent, SRInt row, SRInt col, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderTable() = default;
  virtual void draw(CGContextPtr ctx) const override;
  virtual SRPoint fitToContent(SRPoint point, UITextLayoutDirection direction = none) const override;

  SRInt _numberOfColumns;
  SRInt _numberOfRows;
};

class RenderTableCell;
using RenderTableCellPtr = std::shared_ptr<RenderTableCell>;

class RenderTableCell : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderTableCell);
public:
  SR_DECL_CREATE_FUNC(RenderTableCell);

  RenderTableCell(RenderObjectPtr parent, SRInt startingRow, SRInt rowSpan
    , SRInt startingColumn, SRInt columnSpan, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderTableCell() = default;
  virtual void draw(CGContextPtr ctx) const override;
  virtual RenderObjectPtr findRender(UITextLayoutDirection direction, SRTextPosition position) override;

  RenderTableCellPtr _parentCell; // used temporary while layout rowspan, colspan
  SRInt _row;
  SRInt _rowSpan;
  SRInt _column;
  SRInt _columnSpan;
};

// Text List
//
class RenderList;
using RenderListPtr = std::shared_ptr<RenderList>;

class RenderList : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderList);
public:
  SR_DECL_CREATE_FUNC(RenderList);

  RenderList(RenderObjectPtr parent, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderList() = default;
  virtual SRPoint fitToContent(SRPoint point, UITextLayoutDirection direction = none) const override;
};

// 리스트 아이템의 헤더
//
class RenderListHeader;
using RenderListHeaderPtr = std::shared_ptr<RenderListHeader>;

class RenderListHeader : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderListHeader);
public:
  SR_DECL_CREATE_FUNC(RenderListHeader);

  RenderListHeader(RenderObjectPtr parent, SRRect rect, SRString marker);
  virtual ~RenderListHeader() = default;
  virtual void draw(CGContextPtr ctx) const override;

private:
  SRString _marker;
};

class RenderListItem;
using RenderListItemPtr = std::shared_ptr<RenderListItem>;

class RenderListItem : public RenderObject {
  SR_MAKE_NONCOPYABLE(RenderListItem);
public:
  SR_DECL_CREATE_FUNC(RenderListItem);

  RenderListItem(RenderObjectPtr parent, SRRect rect = { 0, 0, 0, 0 });
  virtual ~RenderListItem() = default;
  virtual RenderObjectPtr findRender(UITextLayoutDirection direction, SRTextPosition position) override;
};

} // namespace sr
