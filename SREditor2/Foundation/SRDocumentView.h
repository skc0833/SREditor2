#pragma once

#include "../UIKit/UITextView.h"
#include "../CoreText/CTFrame.h"
#include <vector>

namespace sr {

class SRDocumentView;
using SRDocumentViewPtr = std::shared_ptr<SRDocumentView>;
using SRDocumentViewWeakPtr = std::weak_ptr<SRDocumentView>;

class SRColumnView;
using SRColumnViewPtr = std::shared_ptr<SRColumnView>;

class SRDocumentView : public UIScrollView {
  SR_MAKE_NONCOPYABLE(SRDocumentView);
public:
  SR_DECL_CREATE_FUNC(SRDocumentView);

  SRDocumentView(const SRRect& frame) : UIScrollView(frame) {}
  virtual ~SRDocumentView() = default;

  virtual void draw(CGContextPtr ctx, SRRect rect) const override;
  virtual const UIViewPtr hitTest(UIEvent event) const override;

  void paginate(const SRSize& pageSize, const SRSize& totalSize);
  SRUInt getPageCount() const { return _pageRects.size(); }
  SRRect getPage(SRUInt page) const { return _pageRects.at(page); }
  SRSize getTotalPageSize() const { return _totalPageSize; }

  // 첫번째 컬럼뷰를 리턴
  SRColumnViewPtr firstColumnView() const;

  // UITextInput
  //
  virtual SRRect caretRect(UITextPosition position) const override;
  virtual void replace(UITextRange range, const SRString& text) const override;
  virtual UITextPosition position(UITextRange range, UITextLayoutDirection farthest) const override;

  //virtual SRRange lineRange(SRTextPosition position) const;

private:
  // 페이지당 문자열 범위는 저장이 불가능하므로 페이지 영역만 저장시킴
  // (테이블 등으로 인해 문자열 범위가 연속적이지 않을 수 있다)
  std::vector<SRRect> _pageRects;
  SRSize _totalPageSize; // 전체 페이지들을 합친 크기
};

class SRPageView;
using SRPageViewPtr = std::shared_ptr<SRPageView>;
using SRPageViewWeakPtr = std::weak_ptr<SRPageView>;

class SRPageView : public UIView {
  SR_MAKE_NONCOPYABLE(SRPageView);
public:
  SR_DECL_CREATE_FUNC(SRPageView);

  SRPageView(const SRRect& frame, int column = 1)
    : UIView(frame), _maxColumnCnt(column) {}
  virtual ~SRPageView() = default;

  virtual void draw(CGContextPtr ctx, SRRect rect) const override;
  virtual const UIViewPtr hitTest(UIEvent event) const override;
  virtual SRRect getContentsRect() const; // skc added

  int _maxColumnCnt; // 페이지 내에 포함 가능한 최대 컬럼 개수
};

class SRColumnView : public UITextView {
  SR_MAKE_NONCOPYABLE(SRColumnView);
public:
  SR_DECL_CREATE_FUNC(SRColumnView);

  SRColumnView(const SRRect& frame, SRIndex colIndex) : UITextView(frame) {
    _colIndex = colIndex;
  }
  virtual ~SRColumnView() = default;
  
  virtual void draw(CGContextPtr ctx, SRRect rect) const override;

  // UIResponser
  //
  virtual void mouseDown(UIEvent event) override;
  virtual void mouseUp(UIEvent event) override;
  virtual void mouseMoved(UIEvent event) override;
  virtual void cursorUpdate(UIEvent event) override;

  // UITextInput
  //
  virtual UITextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) const override;
  virtual SRRect caretRect(UITextPosition position) const override;
  // Return the text position that is at the farthest extent in a given layout direction within a range of text.
  virtual UITextPosition position(UITextRange range, UITextLayoutDirection farthest) const override;

  void setPartialRect(SRRect drawRect) {
    _partialRect = drawRect;
  }

  SRDocumentViewPtr getDocumentView() const;

  // 전체 이미지(문서뷰 이미지)중에서 현재 뷰에(좌상단부터) 그려질 영역
  SRRect _partialRect;
  SRRange _strRange;
  SRIndex _colIndex;
};

} // namespace sr
