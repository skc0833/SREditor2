#pragma once

#include "SRObject.h"

namespace sr {

class UIWindowImpl;
using UIWindowImplPtr = std::shared_ptr<UIWindowImpl>;

class UIWindowImpl : public SRObject {
  SR_MAKE_NONCOPYABLE(UIWindowImpl);
public:
  SR_DECL_CREATE_FUNC(UIWindowImpl);

  UIWindowImpl() {}
  virtual ~UIWindowImpl() {}

public:
  virtual bool invalidateRect(SRRect* rc, bool erase) = 0;
  virtual bool updateWindow() = 0;
  virtual SRRect getClientRect() = 0;

  // scrollbar
  virtual SRSize scrollBarSize(SB_Type type) = 0;
  virtual SRInt scrollPos(SB_Type type) = 0;
  virtual void setScrollPos(SB_Type type, int pos, bool redraw) = 0;
  virtual bool hasScrollBar(SB_Type type) = 0;
  virtual void setScrollInfo(SB_Type type, const SRScrollInfo& info, bool redraw) = 0;
  virtual void getScrollInfo(SB_Type type, SRScrollInfo& info) = 0;
  virtual void showScrollBar(SB_Type type, bool show) = 0;

  // caret
  virtual bool createCaret(SRSize caretSize) = 0;
  virtual bool destroyCaret() = 0;
  virtual bool setCaretPos(SRPoint point) = 0;
  virtual bool showCaret() = 0;
  virtual bool hideCaret() = 0;
};

} // namespace sr
