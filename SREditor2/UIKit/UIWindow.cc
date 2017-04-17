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
  // 첫 번째 자식뷰(contentView)를 리턴함
  // 추후 스크롤바도 별도의 자식뷰로 추가시키자!!!
  SR_ASSERT(getSubviews().size() == 1);
  return getSubviews().at(0);
}

SRPoint UIWindow::localCoordinate(SRPoint point) const {
  point += contentOffset();
  return point;
}

void UIWindow::draw(CGContextPtr ctx, SRRect rect) const {
  // 선택 영역을 순서대로 ctx->_selectedRange 에 저장함
  UITextPosition pos[2] = { _textPositions[0], _textPositions[1] };
  if (pos[0]._offset > pos[1]._offset) {
    std::swap(pos[0], pos[1]);
  }
  ctx->_selectedRange.location = pos[0]._offset;
  ctx->_selectedRange.length = pos[1]._offset - pos[0]._offset;

#if 0 // 윈도우 자체적으로 그릴 경우 배경을 직접 그려줘야 한다.
  // 배경, 경계선 그리기
  drawBackground(ctx, SRRect({ 0, 0 }, getBounds().size));
  //drawBorder(ctx, SRRect({ 0, 0 }, getBounds().size));
  //rect.origin += contentOffset();
  contentView()->draw(ctx, rect); // 내부에서 스크롤 적용하여 드로잉중이므로 윈도우 좌표를 전달함
#else
  UIView::draw(ctx, rect);
#endif
}

void UIWindow::setContentView(UIViewPtr view) {
  removeSubviews(); // 자식뷰들 전부 제거(아니면 리사이징시 뒤에 한번 더 그려지게 된다)
  SR_ASSERT(getSubviews().size() == 0);
  UIWindowPtr thisWindow = getPtr(this); // 자기 자신을 shared_ptr 로 wrapping함
  setWindow(thisWindow); // 윈도우 객체 설정
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
  // 윈도우 좌표를 스크롤뷰 로컬좌표로 변환
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

  // 뷰의 영역을 넘어선 경우, 스크롤바를 표시함
  bool showVScroll = false;
  bool showHScroll = false;

  // 스크롤바를 제외한 영역으로 스크롤바 위치를 지정해야 스크롤바 위치가 맞음
  SRSize clientSize = getClientRect().size;
  //SRSize windowSize = getBounds().size;

  if (contentSize.height > clientSize.height) {
    // 전체 컨텐츠 높이가 윈도우 높이 안에 못들어 가는 경우
    showVScroll = true;
  }
  if (contentSize.width > clientSize.width) {
    // 전체 컨텐츠 폭이 윈도우 폭 안에 못들어 가는 경우
    showHScroll = true;
  }

  if (showVScroll) {
    //	수직 스크롤바 정보 갱신
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
    contentOffset.y = 0; // 수직 스크롤바가 사라질 경우, contentOffset 초기화
  }
  if (showHScroll) {
    //	수평 스크롤바 정보 갱신
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
    contentOffset.x = 0; // 수평 스크롤바가 사라질 경우, contentOffset 초기화
  }

  //SR_LOGD(L"UIScrollView::updateScrollBar() --> {END} offset(%f, %f), showVScroll=%d, showHScroll=%d"
  //  , offset.x, offset.y, showVScroll, showHScroll);

  p->setContentOffset(contentOffset);
  showScrollBar(SR_SB_VERT, showVScroll);
  showScrollBar(SR_SB_HORZ, showHScroll);
}

// 윈도우(UIWindow)의 스크롤바 위치를 갱신해준다.
void UIWindow::vscroll(SB_Code scrollCode) {
  // 라인 단위 스크롤 크기
  static const int LINE_VSCROLL_SIZE = 20;

  UIViewPtr cv = contentView();
  SRRect cvFrame = cv->getFrame();
  //SRRect cvBounds = cv->getBounds();
  const SRSize rcSize = getBounds().size; // 윈도우 크기 저장
  if (cvFrame.size.height <= rcSize.height) {
    return; // 여기서 리턴시키지 않으면 아래 contentOffset()에서 ::GetScrollInfo()가 실패함)
  }

  SRPoint offset = contentOffset();

  SRScrollInfo info;
  getScrollInfo(SR_SB_VERT, info);
  if (info.pos != offset.y) {
    //SR_ASSERT(0); // 현재 resize 시에 offset.y 가 훨씬 큰 경우가 생기고 있다(사이즈 줄일 경우)
    int a = 0;
  }

  //const SRSize rcSize = getBounds().size; // 윈도우 크기 저장

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
    offset.y = maxPos; // SB_PAGEDOWN 으로 넘어서는 경우
  }
  //if (offset.y <= 0) {
  if (offset.y < 0) {
    offset.y = 0; // SB_PAGEDUP 으로 넘어서는 경우
  }

  if (contentOffset().y != offset.y) {
    hideCaret();
    setContentOffset(offset);
    invalidateRect(NULL, false/*=erase*/); // 호출안해주면 화면 갱신이 안됨, erase가 true면 WM_ERASEBKGND를 발생시킴.
    updateCaret();
    showCaret();
  }
}

void UIWindow::hscroll(SB_Code scrollCode) {
  // 라인 단위 스크롤 크기
  static const int LINE_HSCROLL_SIZE = 20;

  SRPoint offset = contentOffset();

  SRScrollInfo info;
  getScrollInfo(SR_SB_HORZ, info);
  SR_ASSERT(info.pos == offset.x);

  SRSize rcSize = getBounds().size; // 윈도우 크기 저장, clientRect 로 처리가 맞을듯???

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
    offset.x = maxPos; // SB_PAGEDOWN 으로 넘어서는 경우
  }
  if (offset.x < 0) {
    offset.x = 0; // SB_PAGEDUP 으로 넘어서는 경우
  }

  if (contentOffset().x != offset.x) {
    hideCaret();
    setContentOffset(offset);
    // updateWindow() 까지 호출해줘도 스크롤 버튼을 누르고 있으면 스크롤 버튼은 갱신되지 않고 있다.
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

  // 현재 윈도우 크기 변경시(resize)에 호출중이며, 윈도우 크기가 변경되므로 여기서 스크롤바를 갱신시킴
  // UIScrollView::setFrame() 에서 ctx->setScrollInfo() 시에도 WM_RESIZE 가 발생됨
  updateScrollBar();

  return false;
}

SRPoint UIWindow::getAlignCenter() const {
  SRPoint center = _center;
  // 스크롤바를 제외시킨 영역의 중앙 좌표를 리턴
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

  // 윈도우 좌표를 컨텐츠뷰(문서뷰) 좌표로 변환(e.g, 문서뷰 사이드 클릭시 x 좌표가 음수값이 됨)
  event._xy = convertTo(localCoordinate(point), contentView()); // window coordinate -> content view's
  if (orgPoint != event._xy) {
    int a = 0;
  }
  // point 좌표는 윈도우 좌표이며, 스크롤뷰에서 스크롤이 적용된 좌표로 변환되서 처리되고 있다.
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

  // @@test 오른쪽 정렬상태에서 라인 왼쪽 클릭시, 윗줄 끝에 캐럿 위치하는 현상 패치용
  //_textPositions[0]._stickToNextLine = _textPositions[1]._stickToNextLine = true;
  SRRect caret = caretRect(textPositions()[1]);
  scrollRectToVisible(caret);
  invalidateRect(NULL, false/*=erase*/);
  updateCaret();
  showCaret();

  SRPoint pt = convertTo(caret.origin, view); // _xyMoving에 컬럼뷰 로컬좌표를 저장
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
  //textPos[1]._stickToNextLine = true; // 디폴트 값 설정함
  if (keyCode == LEFT_KEY || keyCode == HOME_KEY) {
    direction = left;
    if (keyCode == HOME_KEY) {
      UITextPosition movingPos = textPos[1];
      if (event.isCtrlPressed()) {
        textPos[1]._offset = 0;
      } else {
        textPos[1] = contentView()->position(UITextRange(movingPos, movingPos), left);
      }
      // HOME 키로 이동한 경우, 캐럿이 현재 라인의 시작에 위치해야 한다.
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
      //textPos[1]._stickToNextLine = false; // 이게 false 이면, 라인 시작위치에서 이전 라인의 끝으로 인식되어 이동이 안되게 된다???
      UITextPosition movingPos = textPos[1];
      if (event.isCtrlPressed()) {
        textPos[1]._offset = textStorage()->getLength() - 1;
      } else {
        textPos[1] = contentView()->position(UITextRange(movingPos, movingPos), right);
        textPos[1]._offset = std::min(textPos[1]._offset, textStorage()->getLength() - 1);
      }
      if (textStorage()->getString()->data()[textPos[1]._offset] == '\n') {
        // 개행문자로 끝나는 경우, down arrow 키 입력시 다음 라인에 위치시키기 위해 _stickToNextLine를 true로 설정해줘야 한다.
        textPos[1]._stickToNextLine = true;
      } else {
        // END 키로 이동한 경우, 캐럿이 현재 라인의 끝에 위치해야 한다.
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
    // 이전 라인의 끝으로 이동시 필요하지만, 이전 라인의 시작위치일 경우, 1 라인 더 올라가버린다.
    //textPos[1]._stickToNextLine = false;
  } else if (keyCode == DOWN_KEY) {
    direction = down;
    // 이후 라인의 시작으로 이동시 필요하지만, 이전 라인의 끝위치일 경우, 1 라인 더 내려가버린다.
    //textPos[1]._stickToNextLine = true;
  } else if (keyCode == DELETE_KEY) {
    textStorage()->beginEditing();
    if (!hasSelection()) {
      // 블럭이 없는 경우에는 캐럿 이후 문자 하나만 삭제
      textPos[1]._offset = std::min(maxOffset, textPos[1]._offset + 1);
    }
    replace(UITextRange(textPos[0], textPos[1]), L"");
    textPos[1]._offset = textPos[0]._offset;
    textStorage()->endEditing(); // 여기서 레이아웃팅됨
  } else if (keyCode == A_KEY) {
    if (event.isCtrlPressed()) {
      // TODO 전체 선택이 안되고 있다.
      textPos[0]._offset = 0;
      textPos[1]._offset = textStorage()->getLength() - 1;
    } else {
      return;
    }
  }

  if (direction != none && offset != 0) {
    textPos[1] = position(textPos[1], direction, offset);
    if (event.isShiftPressed()) {
      textPos[1]._stickToNextLine = false; // 필요한가???
    } else {
      // 이렇게 하면 END키로 이동 후, down/up 시에 라인의 시작위치로 이동하게 된다.
      //textPos[1]._stickToNextLine = true;
      textPos[0] = textPos[1];
    }
  }

  // TODO: 오른쪽 정렬상태에서 행 처음 위치로 아래 방향키 누를 시, 이전 행 끝에 캐럿이 위치하고 있음
  // 이 위치에서 _stickToNextLine 를 둘다 true 로 설정하면 제대로 표시되고 있다.
  setTextPositions(&textPos[0], &textPos[1]);
  SRRect caret = caretRect(textPos[1]);
  scrollRectToVisible(caret);
  invalidateRect(NULL, false);
  updateCaret();
  showCaret();

  if (direction == left || direction == right) {
    // _xyMoving에 컬럼뷰 로컬좌표를 저장시킨다.
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
  r.origin -= contentOffset(); // 윈도우 좌표로 리턴
  return r;
}

void UIWindow::replace(UITextRange range, const SRString& text) const {
  contentView()->replace(range, text);
  //SRRange textRange(range._range[0]._offset, range._range[1]._offset);
  //SRLayoutManagerPtr layoutManager = textContainer()->layoutManager();
  //SRTextStoragePtr textStorage = layoutManager->textStorage();
  //textStorage->beginEditing();
  //textStorage->replaceCharacters(textRange, text);
  //textStorage->endEditing(); // 여기서 레이아웃팅됨
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
    UIViewPtr hitView = hitTest(UIEvent(MOUSE_MOVE, point.x, point.y)); // 마우스 좌표로부터 뷰를 찾음
    if (hitView) {
      point = convertTo(localCoordinate(point), hitView);
      // root render 좌표로 변환
      SRColumnViewPtr colView = std::dynamic_pointer_cast<SRColumnView>(hitView);
      point.y += colView->_partialRect.origin.y;
      point.x = from._xyMoving.x;

      UITextPosition findPos = colView->textContainer()->rootRender()->closestPosition(point, direction, from);
      findPos._xyMoving.x = from._xyMoving.x;
      // TODO 위 rootRender()->closestPosition() 리턴값은 _offset만 리턴받는게 더 좋을 수 있겠다.
      // findPos._stickToNextLine 값은 이전 라인의 시작(1), 끝(0) 위치에서 달라져야 한다.
      //findPos._stickToNextLine = from._stickToNextLine;

      SRPageViewPtr curPageView = std::dynamic_pointer_cast<SRPageView>(hitView->superview());
      SRRect findCaret = caretRect(findPos);
      SRRect winRect = hitView->getBounds();
      winRect.origin = hitView->convertTo({ 0, 0 });
      winRect.origin -= contentOffset(); // 스크롤 적용

      if (curPageView->_maxColumnCnt > 1 && !winRect.containsPoint(findCaret.origin)) {
        // 멀티 컬럼이고, 찾은 좌표가 현재 페이지가 아니면, 이전 페이지 내의 동일 컬럼에서 찾아야 한다.
        UIViewList pages = contentView()->getSubviews();
        auto it = std::find_if(pages.begin(), pages.end(), [&curPageView](UIViewPtr v) {
          return v == curPageView;
        });
        if (it == pages.begin()) {
          // 이전 페이지가 없는 경우(현재 페이지가 첫 페이지임), 기존 캐럿위치를 유지시킴
          // prevPos._offset 은 멀티 컬럼에서 바로 이후 컬럼 값이므로 전달받았던 from._offset 값과 다를 수 있다.
          return from;
        }
        // 이전 페이지에서 동일 컬럼 획득
        SRIndex colIndex = colView->_colIndex;
        UIViewPtr findPageView = *(--it);
        UIViewList cols = findPageView->getSubviews();
        it = std::find_if(cols.begin(), cols.end(), [&colIndex](UIViewPtr v) {
          SRColumnViewPtr cv = std::dynamic_pointer_cast<SRColumnView>(v);
          return cv->_colIndex == colIndex;
        });
        SRColumnViewPtr findColView = std::dynamic_pointer_cast<SRColumnView>(*it);
        SRPoint pt = { point.x, findColView->_partialRect.bottom() - 1 }; // 이전 페이지 하단 경계선 좌표
        findPos = colView->textContainer()->rootRender()->closestPosition(pt, none, from);

        findCaret = caretRect(findPos);

        SRRect findColRect = findColView->getBounds();
        findColRect.origin = findColView->convertTo({ 0, 0 });
        if (!findColRect.containsPoint(findCaret.origin)) {
          // 찾은 캐럿이 타겟 컬럼이 아닌 경우, 찾은 캐럿의 이전/다음 라인을 찾게함
          // 찾은 위치가 컬럼뷰의 시작 위치의 테이블 마진등에 걸리면 이전 컬럼에 캐럿이 위치하는 경우가 존재함
          // TODO SREditor2.cpp 에서 페이지 크기가 _pageSize(400, 200)이고, 테이블 행에 이미지만 존재하고 2단일 경우,
          // 여기를 타면서 이미지 이전 행에 캐럿이 위치하게 되므로 일단 주석처리함
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
    SRRect caret = caretRect(from); // 윈도우 좌표임
    SRPoint point = caret.origin;
    point.y += caret.size.height;
    SRPoint orgPoint = point;

    UIViewPtr hitView = hitTest(UIEvent(MOUSE_MOVE, point.x, point.y));
    if (hitView) {
      point = convertTo(localCoordinate(point), hitView);
      // root render 좌표로 변환
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
      winRect.origin -= contentOffset(); // 스크롤 적용

      if (curPageView->_maxColumnCnt > 1 && !winRect.containsPoint(findCaret.origin)) {
        // 멀티 컬럼이고, 찾은 좌표가 현재 페이지가 아니면, 이후 페이지 내의 동일 컬럼에서 찾아야 한다.
        UIViewList pages = contentView()->getSubviews();
        auto it = std::find_if(pages.rbegin(), pages.rend(), [&curPageView](UIViewPtr v) {
          return v == curPageView;
        });
        if (it == pages.rbegin()) {
          // 이후 페이지가 없는 경우, 기존 캐럿위치를 유지시킴
          // prevPos._offset 은 멀티 컬럼에서 바로 이후 컬럼 값이므로 전달받았던 from._offset 값과 다를 수 있다.
          //SR_ASSERT(prevPos._offset == from._offset);
          return from;
        }
        // 이후 페이지에서 동일 컬럼 획득
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
        SRPoint pt = { point.x, findColView->_partialRect.top() }; // 이후 페이지 상단 경계선 좌표
        findPos = colView->textContainer()->rootRender()->closestPosition(pt, none, from);

        findCaret = caretRect(findPos);

        SRRect findColRect = findColView->getBounds();
        findColRect.origin = findColView->convertTo({ 0, 0 });
        if (!findColRect.containsPoint(findCaret.origin)) {
          // 찾은 캐럿이 타겟 컬럼이 아닌 경우, 찾은 캐럿의 이전/다음 라인을 찾게함
          // 찾은 위치가 컬럼뷰의 시작 위치의 테이블 마진등에 걸리면 이전 컬럼에 캐럿이 위치하는 경우가 존재함
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
    // 캐럿 크기가 변경될 경우 재성성해줘야 반영된다.
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
  // 아직 캐럿이 생성되지 않는 상태에서도 호출될 수도 있다.
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
      // 이전 페이지에 캐럿이 위치해야 하는 경우, 캐럿 위치를 바꾸지 않는다(like ms-word)
      // 하나의 테이블 컬럼이 여러 페이지에 걸쳐 존재하는 경우, 클릭시 캐럿이 이전 페이지에 위치해야 하는 경우가 존재함
      int a = 0;
      //return;
    }
    createCaret(rect.size); // 매번 새로 생성해줘야 캐럿이 제대로 표시된다!
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
