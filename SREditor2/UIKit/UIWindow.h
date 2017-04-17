#pragma once

#include "UIView.h"
#include "UIWindowImpl.h"
#include "UITextInput.h"
#include "../Foundation/SRTextStorage.h"

namespace sr {

class UIWindow;
using UIWindowPtr = std::shared_ptr<UIWindow>;

// 원래 UIKit 의 상속 구조와 동일함
class UIWindow : public UIView {
  SR_MAKE_NONCOPYABLE(UIWindow);
public:
  SR_DECL_CREATE_FUNC(UIWindow);

  UIWindow(const SRRect& frame, UIWindowImplPtr impl);
  virtual ~UIWindow() = default;

  virtual const UIViewPtr hitTest(UIEvent event) const override;
  virtual void draw(CGContextPtr ctx, SRRect rect) const override;
  virtual bool setFrame(const SRRect& frame);
  virtual SRPoint convertTo(SRPoint point, UIViewPtr to = nullptr) const override;
  virtual SRPoint getAlignCenter() const override;

  void updateScrollBar();
  void vscroll(SB_Code scrollCode);
  void hscroll(SB_Code scrollCode);

  // 윈도우의 컨텐츠뷰. window's view hierarchy 에서 최상위 뷰임.
  UIViewPtr contentView() const;
  void setContentView(UIViewPtr view);

  SRPoint contentOffset() const;
  void setContentOffset(SRPoint contentOffset);

  // 스크롤바를 제외한 영역을 리턴
  SRRect getClientRect() const;

  bool invalidateRect(SRRect* rc, bool erase) const {
    return _impl->invalidateRect(rc, erase);
  }
  bool updateWindow() const {
    return _impl->updateWindow();
  }

  // 스크롤바 관련 함수들
  SRSize scrollBarSize(SB_Type type) const {
    return _impl->scrollBarSize(type);
  }
  SRInt scrollPos(SB_Type type) const {
    return _impl->scrollPos(type);
  }
  SRPoint scrollPos() const {
    SRPoint pt(static_cast<SRFloat>(_impl->scrollPos(SR_SB_HORZ))
      , static_cast<SRFloat>(_impl->scrollPos(SR_SB_VERT)));
    return pt;
  }
  void setScrollPos(SB_Type type, int pos, bool redraw) {
    _impl->setScrollPos(type, pos, redraw);
  }
  void getScrollInfo(SB_Type type, SRScrollInfo& info) const {
    return _impl->getScrollInfo(type, info);
  }
  void setScrollInfo(SB_Type type, const SRScrollInfo& info, bool redraw) {
    return _impl->setScrollInfo(type, info, redraw);
  }
  bool hasScrollBar(SB_Type type) const {
    return _impl->hasScrollBar(type);
  }
  void showScrollBar(SB_Type type, bool show) {
    return _impl->showScrollBar(type, show);
  }

  // Scrolls a specific area of the content so that it is visible in the receiver.
  void scrollRectToVisible(SRRect rect);

  // caret
  bool createCaret(SRSize size = { 0, 0 });
  bool destroyCaret();
  bool setCaretPos(SRPoint point);
  void setCaretSize(SRSize size);
  bool showCaret();
  bool hideCaret();
  void updateCaret();
  bool hasSelection();

  // Etc
  void setTextPositions(const UITextPosition* pos1, const UITextPosition* pos2);
  const UITextPosition* textPositions() const {
    return _textPositions;
  }

  SRTextStoragePtr textStorage() const {
    return _textStorage.lock();
  }
  void setTextStorage(SRTextStoragePtr ts) {
    _textStorage = ts;
  }

  // UIResponser
  //
  virtual void mouseDown(UIEvent event) override;
  virtual void mouseUp(UIEvent event) override;
  virtual void mouseMoved(UIEvent event) override;
  virtual void keyDown(UIEvent event) override;
  virtual void keyUp(UIEvent event) override;
  virtual void cursorUpdate(UIEvent event) override;
  virtual void scrollWheel(UIEvent event) override;

  // UITextInput
  //
  virtual SRRect caretRect(UITextPosition position) const override;
  virtual void replace(UITextRange range, const SRString& text) const override;
  virtual UITextPosition position(UITextPosition from, UITextLayoutDirection direction, SRIndex offset) const override;

  SRPoint localCoordinate(SRPoint point) const; //skc: 스크롤 적용된 로컬좌표를 리턴

private:
  //SRPoint localCoordinate(SRPoint point) const; //skc: 스크롤 적용된 로컬좌표를 리턴

  UIViewPtr _contentView;
  UIWindowImplPtr _impl;

  UITextPosition _textPositions[2];
  bool _caretCreated;
  SRSize _caretSize;

  SRTextStorageWeakPtr _textStorage;
};

} // namespace sr
