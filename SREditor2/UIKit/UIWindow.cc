#include "UIWindow.h"
#include "UIScrollView.h"
#include "../Foundation/SRDocumentView.h"
#include <algorithm>

#define SR_LOG_TAG    L"UIWindow"
#include "SRLog.h"

namespace sr {

UIWindow::UIWindow(const SRRect& frame, UIWindowImplPtr impl)
 : UIView(frame) {
  _impl = impl;
  _caretCreated = false;
}

UIViewPtr UIWindow::contentView() const {
  // ù ��° �ڽĺ�(contentView)�� ������
  // ���� ��ũ�ѹٵ� ������ �ڽĺ�� �߰���Ű��!!!
  SR_ASSERT(getSubviews().size() == 1);
  return getSubviews().at(0);
}

SRPoint UIWindow::localCoordinate(SRPoint point) const {
  point += contentOffset();
  return point;
}

void UIWindow::draw(CGContextPtr ctx, SRRect rect) const {
  // ���� ������ ������� ctx->_selectedRange �� ������
  UITextPosition pos[2] = { _textPositions[0], _textPositions[1] };
  if (pos[0]._offset > pos[1]._offset) {
    std::swap(pos[0], pos[1]);
  }
  ctx->_selectedRange.location = pos[0]._offset;
  ctx->_selectedRange.length = pos[1]._offset - pos[0]._offset;

#if 0 // ������ ��ü������ �׸� ��� ����� ���� �׷���� �Ѵ�.
  // ���, ��輱 �׸���
  drawBackground(ctx, SRRect({ 0, 0 }, getBounds().size));
  //drawBorder(ctx, SRRect({ 0, 0 }, getBounds().size));
  //rect.origin += contentOffset();
  contentView()->draw(ctx, rect); // ���ο��� ��ũ�� �����Ͽ� ��������̹Ƿ� ������ ��ǥ�� ������
#else
  UIView::draw(ctx, rect);
#endif
}

void UIWindow::setContentView(UIViewPtr view) {
  removeSubviews(); // �ڽĺ�� ���� ����(�ƴϸ� ������¡�� �ڿ� �ѹ� �� �׷����� �ȴ�)
  SR_ASSERT(getSubviews().size() == 0);
  UIWindowPtr thisWindow = getPtr(this); // �ڱ� �ڽ��� shared_ptr �� wrapping��
  setWindow(thisWindow); // ������ ��ü ����
  addSubview(view);
}

SRPoint UIWindow::contentOffset() const {
  UIScrollViewPtr p = std::dynamic_pointer_cast<UIScrollView>(contentView());
  return p->contentOffset();
}

void UIWindow::setContentOffset(SRPoint contentOffset) {
  UIScrollViewPtr p = std::dynamic_pointer_cast<UIScrollView>(contentView());
  p->setContentOffset(contentOffset);
}

void UIWindow::scrollRectToVisible(SRRect rect) {
  // ������ ��ǥ�� ��ũ�Ѻ� ������ǥ�� ��ȯ
  rect.origin += contentOffset();
  UIScrollViewPtr p = std::dynamic_pointer_cast<UIScrollView>(contentView());
  p->scrollRectToVisible(rect);
}

SRRect UIWindow::getClientRect() const {
  SRRect frame = getFrame();
  if (hasScrollBar(SR_SB_VERT)) {
    frame.size.width -= scrollBarSize(SR_SB_VERT).width;
}
  if (hasScrollBar(SR_SB_HORZ)) {
    frame.size.height -= scrollBarSize(SR_SB_HORZ).height;
  }
  return frame;
}

void UIWindow::updateScrollBar() {
  UIScrollViewPtr p = std::dynamic_pointer_cast<UIScrollView>(contentView());
  SRSize contentSize = p->getBounds().size;
  SRPoint contentOffset = p->contentOffset();

  // ���� ������ �Ѿ ���, ��ũ�ѹٸ� ǥ����
  bool showVScroll = false;
  bool showHScroll = false;

  // ��ũ�ѹٸ� ������ �������� ��ũ�ѹ� ��ġ�� �����ؾ� ��ũ�ѹ� ��ġ�� ����
  SRSize clientSize = getClientRect().size;
  //SRSize windowSize = getBounds().size;

  if (contentSize.height > clientSize.height) {
    // ��ü ������ ���̰� ������ ���� �ȿ� ����� ���� ���
    showVScroll = true;
  }
  if (contentSize.width > clientSize.width) {
    // ��ü ������ ���� ������ �� �ȿ� ����� ���� ���
    showHScroll = true;
  }

  if (showVScroll) {
    //	���� ��ũ�ѹ� ���� ����
    int pos = scrollPos(SR_SB_VERT);
    SRScrollInfo info;
    info.max = SR_MAX(contentSize.height, clientSize.height);
    info.page = clientSize.height;
    info.pos = pos;
    setScrollInfo(SR_SB_VERT, info, true);
    if (info.pos != contentOffset.y) {
      contentOffset.y = info.pos;
    }
  } else {
    contentOffset.y = 0; // ���� ��ũ�ѹٰ� ����� ���, contentOffset �ʱ�ȭ
  }
  if (showHScroll) {
    //	���� ��ũ�ѹ� ���� ����
    int pos = scrollPos(SR_SB_HORZ);
    SRScrollInfo info;
    info.max = SR_MAX(contentSize.width, clientSize.width);
    info.page = clientSize.width;
    info.pos = pos;
    setScrollInfo(SR_SB_HORZ, info, true);
    if (info.pos != contentOffset.x) {
      contentOffset.x = info.pos;
    }
  } else {
    contentOffset.x = 0; // ���� ��ũ�ѹٰ� ����� ���, contentOffset �ʱ�ȭ
  }

  //SR_LOGD(L"UIScrollView::updateScrollBar() --> {END} offset(%f, %f), showVScroll=%d, showHScroll=%d"
  //  , offset.x, offset.y, showVScroll, showHScroll);

  p->setContentOffset(contentOffset);
  showScrollBar(SR_SB_VERT, showVScroll);
  showScrollBar(SR_SB_HORZ, showHScroll);
}

// ������(UIWindow)�� ��ũ�ѹ� ��ġ�� �������ش�.
void UIWindow::vscroll(SB_Code scrollCode) {
  // ���� ���� ��ũ�� ũ��
  static const int LINE_VSCROLL_SIZE = 20;

  UIViewPtr cv = contentView();
  SRRect cvFrame = cv->getFrame();
  //SRRect cvBounds = cv->getBounds();
  const SRSize rcSize = getBounds().size; // ������ ũ�� ����
  if (cvFrame.size.height <= rcSize.height) {
    return; // ���⼭ ���Ͻ�Ű�� ������ �Ʒ� contentOffset()���� ::GetScrollInfo()�� ������)
  }

  SRPoint offset = contentOffset();

  SRScrollInfo info;
  getScrollInfo(SR_SB_VERT, info);
  if (info.pos != offset.y) {
    //SR_ASSERT(0); // ���� resize �ÿ� offset.y �� �ξ� ū ��찡 ����� �ִ�(������ ���� ���)
    int a = 0;
  }

  //const SRSize rcSize = getBounds().size; // ������ ũ�� ����

  switch (scrollCode) {
  case SR_SB_LINEDOWN:
    offset.y += LINE_VSCROLL_SIZE;
    break;
  case SR_SB_LINEUP:
    offset.y -= LINE_VSCROLL_SIZE;
    break;
  case SR_SB_PAGEDOWN:
    offset.y += rcSize.height;
    break;
  case SR_SB_PAGEUP:
    offset.y -= rcSize.height;
    break;
  case SR_SB_THUMBTRACK:
    offset.y = info.trackPos;
    break;
  default:
    SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;
  if (offset.y > maxPos) {
    offset.y = maxPos; // SB_PAGEDOWN ���� �Ѿ�� ���
  }
  //if (offset.y <= 0) {
  if (offset.y < 0) {
    offset.y = 0; // SB_PAGEDUP ���� �Ѿ�� ���
  }

  if (contentOffset().y != offset.y) {
    hideCaret();
    setContentOffset(offset);
    invalidateRect(NULL, false/*=erase*/); // ȣ������ָ� ȭ�� ������ �ȵ�, erase�� true�� WM_ERASEBKGND�� �߻���Ŵ.
    updateCaret();
    showCaret();
  }
}

void UIWindow::hscroll(SB_Code scrollCode) {
  // ���� ���� ��ũ�� ũ��
  static const int LINE_HSCROLL_SIZE = 20;

  SRPoint offset = contentOffset();

  SRScrollInfo info;
  getScrollInfo(SR_SB_HORZ, info);
  SR_ASSERT(info.pos == offset.x);

  SRSize rcSize = getBounds().size; // ������ ũ�� ����, clientRect �� ó���� ������???

  switch (scrollCode) {
  case SR_SB_LINERIGHT:
    offset.x += LINE_HSCROLL_SIZE;
    break;
  case SR_SB_LINELEFT:
    offset.x -= LINE_HSCROLL_SIZE;
    break;
  case SR_SB_PAGERIGHT:
    offset.x += rcSize.width;
    break;
  case SR_SB_PAGELEFT:
    offset.x -= rcSize.width;
    break;
  case SR_SB_THUMBTRACK:
    offset.x = info.trackPos;
    break;
  default:
    SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;
  if (offset.x > maxPos) {
    offset.x = maxPos; // SB_PAGEDOWN ���� �Ѿ�� ���
  }
  if (offset.x < 0) {
    offset.x = 0; // SB_PAGEDUP ���� �Ѿ�� ���
  }

  if (contentOffset().x != offset.x) {
    hideCaret();
    setContentOffset(offset);
    // updateWindow() ���� ȣ�����൵ ��ũ�� ��ư�� ������ ������ ��ũ�� ��ư�� ���ŵ��� �ʰ� �ִ�.
    invalidateRect(NULL, false);
    updateCaret();
    showCaret();
  }
}


bool UIWindow::setFrame(const SRRect& frame) {
  //SR_LOGD(L">>> UIWindow::setFrame(%.1f,%.1f %.1f-%.1f)", frame.left(), frame.top(), frame.width(), frame.height());
  if (UIView::setFrame(frame)) {
    return true;
  }

  // ���� ������ ũ�� �����(resize)�� ȣ�����̸�, ������ ũ�Ⱑ ����ǹǷ� ���⼭ ��ũ�ѹٸ� ���Ž�Ŵ
  // UIScrollView::setFrame() ���� ctx->setScrollInfo() �ÿ��� WM_RESIZE �� �߻���
  updateScrollBar();

  return false;
}

SRPoint UIWindow::getAlignCenter() const {
  SRPoint center = _center;
  // ��ũ�ѹٸ� ���ܽ�Ų ������ �߾� ��ǥ�� ����
  if (hasScrollBar(SR_SB_VERT)) {
    center.x -= scrollBarSize(SR_SB_VERT).width / 2;
  }
  if (hasScrollBar(SR_SB_HORZ)) {
    center.y -= scrollBarSize(SR_SB_HORZ).height / 2;
  }
  return center;
}

SRPoint UIWindow::convertTo(SRPoint point, UIViewPtr to) const {
  return UIView::convertTo(point, to); // for debug
}

const UIViewPtr UIWindow::hitTest(UIEvent event) const {
  SRPoint point = event._xy;
  SRPoint orgPoint = point;

  // ������ ��ǥ�� ��������(������) ��ǥ�� ��ȯ(e.g, ������ ���̵� Ŭ���� x ��ǥ�� �������� ��)
  event._xy = convertTo(localCoordinate(point), contentView()); // window coordinate -> content view's
  if (orgPoint != event._xy) {
    int a = 0;
  }
  // point ��ǥ�� ������ ��ǥ�̸�, ��ũ�Ѻ信�� ��ũ���� ����� ��ǥ�� ��ȯ�Ǽ� ó���ǰ� �ִ�.
  return contentView()->hitTest(event);
}

// UIResponser
//
void UIWindow::mouseDown(UIEvent event) {
  hideCaret();
  const UIViewPtr view = hitTest(event);
  if (view) {
    SRPoint point = event.mouseLocation();
    event._xy = convertTo(localCoordinate(point), view);
    view->mouseDown(event);
  }

  // @@test ������ ���Ļ��¿��� ���� ���� Ŭ����, ���� ���� ĳ�� ��ġ�ϴ� ���� ��ġ��
  //_textPositions[0]._stickToNextLine = _textPositions[1]._stickToNextLine = true;
  SRRect caret = caretRect(textPositions()[1]);
  scrollRectToVisible(caret);
  invalidateRect(NULL, false/*=erase*/);
  updateCaret();
  showCaret();

  SRPoint pt = convertTo(caret.origin, view); // _xyMoving�� �÷��� ������ǥ�� ����
  pt += contentOffset();
  _textPositions[1]._xyMoving.x = pt.x;
}

void UIWindow::mouseUp(UIEvent event) {
  UIViewPtr view = hitTest(event);
  if (view) {
    SRPoint point = event.mouseLocation();
    event._xy = convertTo(localCoordinate(point), view);
    view->mouseUp(event);
  }
}

void UIWindow::mouseMoved(UIEvent event) {
#if 1
  UIViewPtr view = hitTest(event);
  if (view) {
    SRPoint point = event.mouseLocation();
    event._xy = convertTo(localCoordinate(point), view);
    view->mouseMoved(event);

    SRTextPosition textPos = textPositions()[1];
    if (event.pressedMouseButtons() == LEFT_MOUSE_BUTTON && textPos._offset != SRNotFound) {
      SRRect caret = caretRect(textPos);
      SR_LOGD(L"mouseMoved() textPos=%d, point=[%.1f, %.1f], caret=[%.1f, %.1f %.1f-%.1f]"
        , textPos._offset, point.x, point.y, caret.left(), caret.top(), caret.width(), caret.height());
      scrollRectToVisible(caret);
      invalidateRect(NULL, false/*=erase*/);
    }
  }
#endif
}

void UIWindow::cursorUpdate(UIEvent event) {
#if 1
  UIViewPtr view = hitTest(event);
  if (view) {
    SRPoint point = event.mouseLocation();
    event._xy = convertTo(localCoordinate(point), view);
    view->cursorUpdate(event);
  }
#endif
}

void UIWindow::scrollWheel(UIEvent event) {
  SR_ASSERT(0);
}

void UIWindow::keyDown(UIEvent event) {
  hideCaret();
  UITextPosition textPos[2] = { textPositions()[0], textPositions()[1] };
  SRIndex maxOffset = textStorage()->getLength() - 1;
  UITextLayoutDirection direction = none;
  SRIndex offset = 1;
  UIKeyCode keyCode = event._keyCode;
  //textPos[1]._stickToNextLine = true; // ����Ʈ �� ������
  if (keyCode == LEFT_KEY || keyCode == HOME_KEY) {
    direction = left;
    if (keyCode == HOME_KEY) {
      UITextPosition movingPos = textPos[1];
      if (event.isCtrlPressed()) {
        textPos[1]._offset = 0;
      } else {
        textPos[1] = contentView()->position(UITextRange(movingPos, movingPos), left);
      }
      // HOME Ű�� �̵��� ���, ĳ���� ���� ������ ���ۿ� ��ġ�ؾ� �Ѵ�.
      SR_ASSERT(textPos[1]._stickToNextLine == true);
      //textPos[1]._stickToNextLine = true;
      if (!event.isShiftPressed()) {
        textPos[0] = textPos[1];
      }
      offset = 0;
    } else {
      textPos[1]._stickToNextLine = true;
    }
  } else if (keyCode == RIGHT_KEY || keyCode == END_KEY) {
    direction = right;
    //textPos[1]._stickToNextLine = true;
    if (keyCode == END_KEY) {
      //textPos[1]._stickToNextLine = false; // �̰� false �̸�, ���� ������ġ���� ���� ������ ������ �νĵǾ� �̵��� �ȵǰ� �ȴ�???
      UITextPosition movingPos = textPos[1];
      if (event.isCtrlPressed()) {
        textPos[1]._offset = textStorage()->getLength() - 1;
      } else {
        textPos[1] = contentView()->position(UITextRange(movingPos, movingPos), right);
        textPos[1]._offset = std::min(textPos[1]._offset, textStorage()->getLength() - 1);
      }
      if (textStorage()->getString()->data()[textPos[1]._offset] == '\n') {
        // ���๮�ڷ� ������ ���, down arrow Ű �Է½� ���� ���ο� ��ġ��Ű�� ���� _stickToNextLine�� true�� ��������� �Ѵ�.
        textPos[1]._stickToNextLine = true;
      } else {
        // END Ű�� �̵��� ���, ĳ���� ���� ������ ���� ��ġ�ؾ� �Ѵ�.
        SR_ASSERT(textPos[1]._stickToNextLine == false);
        //textPos[1]._stickToNextLine = false;
      }
      if (!event.isShiftPressed()) {
        textPos[0] = textPos[1];
      }
      offset = 0;
    }
  } else if (keyCode == UP_KEY) {
    direction = up;
    // ���� ������ ������ �̵��� �ʿ�������, ���� ������ ������ġ�� ���, 1 ���� �� �ö󰡹�����.
    //textPos[1]._stickToNextLine = false;
  } else if (keyCode == DOWN_KEY) {
    direction = down;
    // ���� ������ �������� �̵��� �ʿ�������, ���� ������ ����ġ�� ���, 1 ���� �� ������������.
    //textPos[1]._stickToNextLine = true;
  } else if (keyCode == DELETE_KEY) {
    textStorage()->beginEditing();
    if (!hasSelection()) {
      // ���� ���� ��쿡�� ĳ�� ���� ���� �ϳ��� ����
      textPos[1]._offset = std::min(maxOffset, textPos[1]._offset + 1);
    }
    replace(UITextRange(textPos[0], textPos[1]), L"");
    textPos[1]._offset = textPos[0]._offset;
    textStorage()->endEditing(); // ���⼭ ���̾ƿ��õ�
  } else if (keyCode == A_KEY) {
    if (event.isCtrlPressed()) {
      // TODO ��ü ������ �ȵǰ� �ִ�.
      textPos[0]._offset = 0;
      textPos[1]._offset = textStorage()->getLength() - 1;
    } else {
      return;
    }
  }

  if (direction != none && offset != 0) {
    textPos[1] = position(textPos[1], direction, offset);
    if (event.isShiftPressed()) {
      textPos[1]._stickToNextLine = false; // �ʿ��Ѱ�???
    } else {
      // �̷��� �ϸ� ENDŰ�� �̵� ��, down/up �ÿ� ������ ������ġ�� �̵��ϰ� �ȴ�.
      //textPos[1]._stickToNextLine = true;
      textPos[0] = textPos[1];
    }
  }

  // TODO: ������ ���Ļ��¿��� �� ó�� ��ġ�� �Ʒ� ����Ű ���� ��, ���� �� ���� ĳ���� ��ġ�ϰ� ����
  // �� ��ġ���� _stickToNextLine �� �Ѵ� true �� �����ϸ� ����� ǥ�õǰ� �ִ�.
  setTextPositions(&textPos[0], &textPos[1]);
  SRRect caret = caretRect(textPos[1]);
  scrollRectToVisible(caret);
  invalidateRect(NULL, false);
  updateCaret();
  showCaret();

  if (direction == left || direction == right) {
    // _xyMoving�� �÷��� ������ǥ�� �����Ų��.
    event._xy = caret.origin;
    UIViewPtr v = hitTest(event);
    SRPoint pt = convertTo(caret.origin, v);
    _textPositions[1]._xyMoving.x = localCoordinate(pt).x;
  } else if (direction == up || direction == down) {
    _textPositions[1]._xyMoving.y = caret.origin.y;
  }
}

void UIWindow::keyUp(UIEvent event) {
}

// UITextInput
//
SRRect UIWindow::caretRect(UITextPosition position) const {
  SRRect r = contentView()->caretRect(position);
  r.origin -= contentOffset(); // ������ ��ǥ�� ����
  return r;
}

void UIWindow::replace(UITextRange range, const SRString& text) const {
  contentView()->replace(range, text);
  //SRRange textRange(range._range[0]._offset, range._range[1]._offset);
  //SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  //SRTextStoragePtr textStorage = layoutManager->textStorage();
  //textStorage->beginEditing();
  //textStorage->replaceCharacters(textRange, text);
  //textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
}

UITextPosition UIWindow::position(UITextPosition from, UITextLayoutDirection direction, SRIndex offset) const {
  UITextPosition textPos = from;
  SRIndex maxOffset = textStorage()->getLength() - 1;
  if (direction == left) {
    textPos._offset = std::max(0, from._offset - offset);
  } else if (direction == right) {
    textPos._offset = std::min(maxOffset, from._offset + offset);
  } else if (direction == up) {
    SRRect caret = caretRect(from);
    SRPoint point = caret.origin;
    point.y -= 1;
    SRPoint orgPoint = point;
    UIViewPtr hitView = hitTest(UIEvent(MOUSE_MOVE, point.x, point.y)); // ���콺 ��ǥ�κ��� �並 ã��
    if (hitView) {
      point = convertTo(localCoordinate(point), hitView);
      // root render ��ǥ�� ��ȯ
      SRColumnViewPtr colView = std::dynamic_pointer_cast<SRColumnView>(hitView);
      point.y += colView->_partialRect.origin.y;
      point.x = from._xyMoving.x;

      UITextPosition findPos = colView->textContainer()->rootRender()->closestPosition(point, direction, from);
      findPos._xyMoving.x = from._xyMoving.x;
      // TODO �� rootRender()->closestPosition() ���ϰ��� _offset�� ���Ϲ޴°� �� ���� �� �ְڴ�.
      // findPos._stickToNextLine ���� ���� ������ ����(1), ��(0) ��ġ���� �޶����� �Ѵ�.
      //findPos._stickToNextLine = from._stickToNextLine;

      SRPageViewPtr curPageView = std::dynamic_pointer_cast<SRPageView>(hitView->superview());
      SRRect findCaret = caretRect(findPos);
      SRRect winRect = hitView->getBounds();
      winRect.origin = hitView->convertTo({ 0, 0 });
      winRect.origin -= contentOffset(); // ��ũ�� ����

      if (curPageView->_maxColumnCnt > 1 && !winRect.containsPoint(findCaret.origin)) {
        // ��Ƽ �÷��̰�, ã�� ��ǥ�� ���� �������� �ƴϸ�, ���� ������ ���� ���� �÷����� ã�ƾ� �Ѵ�.
        UIViewList pages = contentView()->getSubviews();
        auto it = std::find_if(pages.begin(), pages.end(), [&curPageView](UIViewPtr v) {
          return v == curPageView;
        });
        if (it == pages.begin()) {
          // ���� �������� ���� ���(���� �������� ù ��������), ���� ĳ����ġ�� ������Ŵ
          // prevPos._offset �� ��Ƽ �÷����� �ٷ� ���� �÷� ���̹Ƿ� ���޹޾Ҵ� from._offset ���� �ٸ� �� �ִ�.
          return from;
        }
        // ���� ���������� ���� �÷� ȹ��
        SRIndex colIndex = colView->_colIndex;
        UIViewPtr findPageView = *(--it);
        UIViewList cols = findPageView->getSubviews();
        it = std::find_if(cols.begin(), cols.end(), [&colIndex](UIViewPtr v) {
          SRColumnViewPtr cv = std::dynamic_pointer_cast<SRColumnView>(v);
          return cv->_colIndex == colIndex;
        });
        SRColumnViewPtr findColView = std::dynamic_pointer_cast<SRColumnView>(*it);
        SRPoint pt = { point.x, findColView->_partialRect.bottom() - 1 }; // ���� ������ �ϴ� ��輱 ��ǥ
        findPos = colView->textContainer()->rootRender()->closestPosition(pt, none, from);

        findCaret = caretRect(findPos);

        SRRect findColRect = findColView->getBounds();
        findColRect.origin = findColView->convertTo({ 0, 0 });
        if (!findColRect.containsPoint(findCaret.origin)) {
          // ã�� ĳ���� Ÿ�� �÷��� �ƴ� ���, ã�� ĳ���� ����/���� ������ ã����
          // ã�� ��ġ�� �÷����� ���� ��ġ�� ���̺� ����� �ɸ��� ���� �÷��� ĳ���� ��ġ�ϴ� ��찡 ������
          // TODO SREditor2.cpp ���� ������ ũ�Ⱑ _pageSize(400, 200)�̰�, ���̺� �࿡ �̹����� �����ϰ� 2���� ���,
          // ���⸦ Ÿ�鼭 �̹��� ���� �࿡ ĳ���� ��ġ�ϰ� �ǹǷ� �ϴ� �ּ�ó����
          //findPos = colView->textContainer()->rootRender()->closestPosition(pt, direction, findPos);
          //findCaret = caretRect(findPos);
        }
        findPos._xyMoving.x = from._xyMoving.x;
        return findPos;
      } else {
        return findPos;
      }
    }
  } else if (direction == down) {
    SRRect caret = caretRect(from); // ������ ��ǥ��
    SRPoint point = caret.origin;
    point.y += caret.size.height;
    SRPoint orgPoint = point;

    UIViewPtr hitView = hitTest(UIEvent(MOUSE_MOVE, point.x, point.y));
    if (hitView) {
      point = convertTo(localCoordinate(point), hitView);
      // root render ��ǥ�� ��ȯ
      SRColumnViewPtr colView = std::dynamic_pointer_cast<SRColumnView>(hitView);
      point.y += colView->_partialRect.origin.y;
      point.x = from._xyMoving.x;

      UITextPosition findPos = colView->textContainer()->rootRender()->closestPosition(point, direction, from);
      findPos._xyMoving.x = from._xyMoving.x;
      //findPos._stickToNextLine = from._stickToNextLine;

      SRPageViewPtr curPageView = std::dynamic_pointer_cast<SRPageView>(hitView->superview());
      SRRect findCaret = caretRect(findPos);
      SRRect winRect = hitView->getBounds();
      winRect.origin = hitView->convertTo({ 0, 0 });
      winRect.origin -= contentOffset(); // ��ũ�� ����

      if (curPageView->_maxColumnCnt > 1 && !winRect.containsPoint(findCaret.origin)) {
        // ��Ƽ �÷��̰�, ã�� ��ǥ�� ���� �������� �ƴϸ�, ���� ������ ���� ���� �÷����� ã�ƾ� �Ѵ�.
        UIViewList pages = contentView()->getSubviews();
        auto it = std::find_if(pages.rbegin(), pages.rend(), [&curPageView](UIViewPtr v) {
          return v == curPageView;
        });
        if (it == pages.rbegin()) {
          // ���� �������� ���� ���, ���� ĳ����ġ�� ������Ŵ
          // prevPos._offset �� ��Ƽ �÷����� �ٷ� ���� �÷� ���̹Ƿ� ���޹޾Ҵ� from._offset ���� �ٸ� �� �ִ�.
          //SR_ASSERT(prevPos._offset == from._offset);
          return from;
        }
        // ���� ���������� ���� �÷� ȹ��
        SRIndex colIndex = colView->_colIndex;
        UIViewPtr findPageView = *(--it);
        UIViewList cols = findPageView->getSubviews();
        it = std::find_if(cols.rbegin(), cols.rend(), [&colIndex](UIViewPtr v) {
          SRColumnViewPtr cv = std::dynamic_pointer_cast<SRColumnView>(v);
          return cv->_colIndex == colIndex;
        });
        if (it == cols.rend()) {
          return from;
        }
        SRColumnViewPtr findColView = std::dynamic_pointer_cast<SRColumnView>(*it);
        SRPoint pt = { point.x, findColView->_partialRect.top() }; // ���� ������ ��� ��輱 ��ǥ
        findPos = colView->textContainer()->rootRender()->closestPosition(pt, none, from);

        findCaret = caretRect(findPos);

        SRRect findColRect = findColView->getBounds();
        findColRect.origin = findColView->convertTo({ 0, 0 });
        if (!findColRect.containsPoint(findCaret.origin)) {
          // ã�� ĳ���� Ÿ�� �÷��� �ƴ� ���, ã�� ĳ���� ����/���� ������ ã����
          // ã�� ��ġ�� �÷����� ���� ��ġ�� ���̺� ����� �ɸ��� ���� �÷��� ĳ���� ��ġ�ϴ� ��찡 ������
          findPos = colView->textContainer()->rootRender()->closestPosition(pt, direction, findPos);
          findCaret = caretRect(findPos);
        }

        findPos._xyMoving.x = from._xyMoving.x;
        return findPos;
      } else {
        return findPos;
      }
    }
  }
  return textPos;
}

// caret
bool UIWindow::createCaret(SRSize size) {
  //SR_LOGD(L"createCaret() _textPositions[0]._offset=%d, size=[%.1f, %.1f]", _textPositions[0]._offset, size.width, size.height);
  if (_caretCreated) {
    //SR_LOGD(L"createCaret() destroyCaret");
    destroyCaret();
  }
  if (size != SRSize(0, 0)) {
    _caretSize = size;
  }
  _caretCreated = _impl->createCaret(_caretSize);
  SR_ASSERT(_caretCreated);
  return _caretCreated;
}

bool UIWindow::destroyCaret() {
  //SR_LOGD(L"destroyCaret()");
  if (!_caretCreated) {
    return true;
  }
  _caretCreated = false;
  bool result = _impl->destroyCaret();
  SR_ASSERT(result);
  return result;
}

bool UIWindow::setCaretPos(SRPoint point) {
  //SR_LOGD(L"setCaretPos() point=[%.1f, %.1f]", point.x, point.y);
  return _impl->setCaretPos(point);
}

void UIWindow::setCaretSize(SRSize size) {
  //SR_LOGD(L"setCaretSize() _caretCreated=%d, size=[%.1f, %.1f]", _caretCreated, size.width, size.height);
  if (_caretSize != size) {
    // ĳ�� ũ�Ⱑ ����� ��� �缺������� �ݿ��ȴ�.
    destroyCaret();
    createCaret(size);
  }
  _caretSize = size;
}

bool UIWindow::showCaret() {
  if (!_caretCreated)
    return false;
  //SR_LOGD(L"showCaret()");
  if (_caretCreated && !hasSelection()) {
    bool result = _impl->showCaret();
    SR_ASSERT(result);
    return result;
  }
  return true;
}

bool UIWindow::hideCaret() {
  // ���� ĳ���� �������� �ʴ� ���¿����� ȣ��� ���� �ִ�.
  if (!_caretCreated)
    return false;
  //SR_LOGD(L"hideCaret()");
  return _impl->hideCaret();
}

void UIWindow::updateCaret() {
  if (!hasSelection() && _textPositions[0]._offset != SRNotFound) {
    //SRRect rect = _textInput->caretRect(_textPositions[0]);
    SRRect rect = caretRect(_textPositions[0]);
    //SR_LOGD(L"updateCaret() _textPositions[%d, %d] -> rect=[%.1f, %.1f, %.1f, %.1f]"
    //  , _textPositions[0]._offset, _textPositions[1]._offset, rect.left(), rect.top(), rect.width(), rect.height());
    if (rect.origin.y < 0) {
      // ���� �������� ĳ���� ��ġ�ؾ� �ϴ� ���, ĳ�� ��ġ�� �ٲ��� �ʴ´�(like ms-word)
      // �ϳ��� ���̺� �÷��� ���� �������� ���� �����ϴ� ���, Ŭ���� ĳ���� ���� �������� ��ġ�ؾ� �ϴ� ��찡 ������
      int a = 0;
      //return;
    }
    createCaret(rect.size); // �Ź� ���� ��������� ĳ���� ����� ǥ�õȴ�!
    setCaretPos(rect.origin);
  }
}

void UIWindow::setTextPositions(const UITextPosition* pos1, const UITextPosition* pos2) {
  if (pos1) _textPositions[0] = *pos1;
  if (pos2) _textPositions[1] = *pos2;
}

bool UIWindow::hasSelection() {
  return _textPositions[0]._offset != _textPositions[1]._offset;
}

} // namespace sr
