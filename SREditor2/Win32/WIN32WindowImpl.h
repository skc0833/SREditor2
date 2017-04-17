#pragma once

#include "../UIKit/UIWindowImpl.h"

namespace sr {

class WIN32WindowImpl;
using WIN32WindowImplPtr = std::shared_ptr<WIN32WindowImpl>;

class WIN32WindowImpl : public UIWindowImpl {
  SR_MAKE_NONCOPYABLE(WIN32WindowImpl);
public:
  SR_DECL_CREATE_FUNC(WIN32WindowImpl);

  WIN32WindowImpl(void* hwnd);
  virtual ~WIN32WindowImpl();

public:
  virtual bool invalidateRect(SRRect* rc, bool erase);
  virtual bool updateWindow();
  virtual SRRect getClientRect();

  virtual SRSize scrollBarSize(SB_Type type);
  virtual SRInt scrollPos(SB_Type type);
  virtual void setScrollPos(SB_Type type, int pos, bool redraw);
  virtual void setScrollInfo(SB_Type type, const SRScrollInfo& info, bool redraw);
  virtual void getScrollInfo(SB_Type type, SRScrollInfo& info);
  virtual bool hasScrollBar(SB_Type type);
  virtual void showScrollBar(SB_Type type, bool show);

  // caret
  virtual bool createCaret(SRSize caretSize);
  virtual bool destroyCaret();
  virtual bool setCaretPos(SRPoint point);
  virtual bool showCaret();
  virtual bool hideCaret();

private:
  void* _hwnd;
};

} // namespace sr
