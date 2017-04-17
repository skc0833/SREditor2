#include "SRDocumentView.h"
#include "../UIKit/UIWindow.h"
#include "../Win32/WIN32GraphicContextImpl.h"
#undef max // ������ ������� ��𿡼��� ���ǵǰ� �־ std::max() ȣ��ο��� ������ ���� �߻�����
#undef min
#include <algorithm>

namespace sr {

static const std::vector<std::pair<SRFloat, SRRange>>& _lineList(const SRDocumentView* docView) {
  //SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(docView->getSubviews().at(0));
  //SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));
  SRColumnViewPtr columnView = docView->firstColumnView();
  SRTextContainerPtr tv = columnView->textContainer();
  return tv->rootRender()->_pageBreakPos;
}

SRColumnViewPtr SRDocumentView::firstColumnView() const {
  SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(getSubviews().at(0));
  SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));
  return columnView;
}

SRDocumentViewPtr SRColumnView::getDocumentView() const {
  SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(superview());
  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(pageView->superview());
  return docView;
}

void SRDocumentView::paginate(const SRSize& pageSize, const SRSize& totalSize) {
  _pageRects.clear();
  _totalPageSize = totalSize;
  const std::vector<std::pair<SRFloat, SRRange>>& lines = _lineList(this);
  SRFloat prevY = 0;
  SRFloat curY = 0;
  SRRect pageRect;
  pageRect.size.width = pageSize.width;
  SRFloat& nextPageStart = pageRect.origin.y;
  SRFloat nextPageEnd = pageSize.height;
  for (int i = 0; i < lines.size(); ++i) {
    auto lineInfo = lines.at(i);
    curY = lineInfo.first;
    if ((i > 0 && (curY) > nextPageEnd)/* || (i == lines.size() - 1)*/) {
      if (i == lines.size() - 1) {
        int a = 0;
      }
      // ���� ������ �ϴ��� �Ѿ�� ���, �������� �߰���
      pageRect.size.height = prevY - nextPageStart;
      if (pageRect.size.height > pageSize.height) {
        //SR_ASSERT(0);
        int a = 0;
      }
      _pageRects.push_back(pageRect);
      // ���� ������ ���� ����(����, �� ��ġ)
      nextPageStart = prevY;
      nextPageEnd = (prevY + pageSize.height);
    }
    prevY = curY; // ���� �������� ������ ��ġ�� ������
  }

  if (curY > nextPageStart) {
    // last page
    //rect.size.height = curY - rect.origin.y;
    pageRect.size.height = totalSize.height - nextPageStart; // ���� ���̸� ���� �Ҵ��Ѵ�.
    if (pageRect.size.height > pageSize.height) {
      // ���� ���̰� ������ ũ�⸦ �Ѵ� ��쵵 �߻��ϰ� �ִ�.
      // (������ ������ ������ ũ�⺸�� �۰�, ������ ���� ���Ŀ��� �������� ���� �����)
      // �ϴ� �� ��쿡�� �׳� �� �������� �׷����� ��(�ణ�� ���� �߻��ϰ� ��)
      //SR_ASSERT(0); // @@ �߻��ǰ� ����. Ȯ���ʿ�!!!
    }
    _pageRects.push_back(pageRect);
  }
}

// declared for debug
void SRDocumentView::draw(CGContextPtr ctx, SRRect rect) const {
  // �ڽĺ�� �׸���
  SR_PROF_TIME_START(L"SRDocumentView::draw");
  UIView::draw(ctx, rect);
  SR_PROF_TIME_END(L"SRDocumentView::draw");
}

const UIViewPtr SRDocumentView::hitTest(UIEvent event) const {
  // UIWindow::hitTest()���� �ٷ� ȣ��ǰ� ����
  if (event.isMouseEvent()) {
    // ���콺 �̺�Ʈ�� ���, SRDocumentView ���� �ܸ̿� Ŭ���ص� SRDocumentView �̳��� ĳ���� ������
    SRPoint& point = event._xy;
    SRPoint orgPoint = point;

    // ���� ������ �̳��� ��ǥ�� ������Ŵ
    SRRect rect({ 0, 0 }, getBounds().size); // local area
    point.x = std::max(point.x, rect.left());
    point.x = std::min(point.x, rect.right() - 1);
    point.y = std::max(point.y, rect.top());
    point.y = std::min(point.y, rect.bottom() - 1);
    if (orgPoint != point) {
      //SR_ASSERT(0);
      int a = 0;
    }

    // Ŭ���� ��ġ���� �ڿ� �ִ� ���� ������ �並 ã�´�.
    auto it = std::find_if(_subviews.begin(), _subviews.end(), [&point](UIViewPtr v) {
      return point.y < v->getFrame().top();
    });
    if (it == _subviews.begin()) {
      // ù ������ ���� ��ġ�� Ŭ���� ���, ù ������ top ��ġ�� ����
      point.y = _subviews.front()->getFrame().top();
    } else {
      UIViewPtr prevView = *(--it);
      if (point.y >= prevView->getFrame().bottom()) {
        // ������ ���̸� Ŭ����, ���� ������ bottom - 1 ��ġ�� ����(like ms-word)
        point.y = prevView->getFrame().bottom() - 1;
      }
    }
  }

  return UIView::hitTest(event);
}

UITextPosition SRDocumentView::position(UITextRange range, UITextLayoutDirection farthest) const {
  SRColumnViewPtr firstColView = firstColumnView();
  UITextPosition result = firstColView->position(range, farthest);
  return result;
}

// ĳ�� ��ġ�κ��� ������Ʈ���� ĳ�� ��ǥ�� ȹ�� ��, ������ ��ǥ�� ����
SRRect SRDocumentView::caretRect(UITextPosition position) const {
  SRColumnViewPtr firstColView = firstColumnView();

  // ������Ʈ���� caretRect ��ġ ȹ��(ù��° �÷��並 �����)
  SRRect caretRect = firstColView->caretRect(position);

  // ������Ʈ���� caretRect ��ġ�� ������ ��ǥ�� ��ȯ�ϱ� ���� ĳ���� ��ġ�� �÷��並 ��ȸ
  const auto it = std::find_if(_pageRects.begin(), _pageRects.end(), [&caretRect](SRRect item) {
    // ĳ���� ���� ������(�÷�) ���� �̳��� �����ϴ��� üũ
    return caretRect.top() < item.bottom();
  });

  // ������Ʈ ��ǥ�� ������ ��ǥ�� ��ȯ�ϱ� ����, ã�� �÷��� ���� ��ǥ�� ���Ѵ�.
  SRRect targetColRect = *it;
  caretRect.origin -= targetColRect.origin; // ������Ʈ ��ǥ -> ���� �÷��� ���� ��ǥ

  if (caretRect.origin.y < 0) {
    // ���� ������(�÷�)�� ĳ���� ��ġ�ؾ� �ϴ� ���, ĳ�� ��ġ�� �ٲ��� �ʴ´�(like ms-word)
    // �ϳ��� ���̺� �÷��� ���� �������� ���� �����ϴ� ���, Ŭ���� ĳ���� ���� �������� ��ġ�ؾ� �ϴ� ��찡 ������
    return caretRect;
  }

  // ���� �÷��� ���� ��ǥ -> ������ ��ǥ
  int colIndex = it - _pageRects.begin(); // ĳ���� ��ġ�� �÷��� �ε���

  // ĳ���� ��ġ�� �������� �˻�(���������� �÷��� ������ �����)
  const auto pageViews = getSubviews();
  auto pageIter = std::find_if(pageViews.begin(), pageViews.end(), [&colIndex](UIViewPtr pageView) {
    if (colIndex < pageView->getSubviews().size()) {
      return true;
    }
    colIndex -= pageView->getSubviews().size();
    return false;
  });
  const auto pageView = std::dynamic_pointer_cast<SRPageView>(*pageIter);
  const auto columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(colIndex));
  SRPoint orgPoint = caretRect.origin;
  caretRect.origin = columnView->convertTo(caretRect.origin); // ������ ��ǥ�� ����
  if (orgPoint != caretRect.origin) {
    int a = 0;
  }
  return caretRect;
}

void SRDocumentView::replace(UITextRange range, const SRString& text) const {
  SRColumnViewPtr firstColView = firstColumnView();
  firstColView->replace(range, text);
}

// declared for debug
void SRPageView::draw(CGContextPtr ctx, SRRect rect) const {
  // �ڽĺ�� �׸���
  UIView::draw(ctx, rect);
}

SRRect SRPageView::getContentsRect() const {
  return _bounds;
}

const UIViewPtr SRPageView::hitTest(UIEvent event) const {
  SR_ASSERT(_subviews.size() > 0);
  SR_ASSERT(getBounds().origin.x == 0 && getBounds().origin.y == 0);
  SRPoint point = event.mouseLocation();

  if (pointInside(point)) {
    // �÷� ���� Ŭ���� ���� �÷� ��ġ�� ������
    auto it = std::find_if(_subviews.begin(), _subviews.end(), [&point](UIViewPtr v) {
      return point.x < v->getFrame().left();
    });
    if (it == _subviews.begin()) {
      // ù �÷� ���� ��ġ�� Ŭ���� ���, ù �÷� left ��ġ�� ����
      if (event.isMouseEvent()) {
        event._xy.x = _subviews.front()->getFrame().left();
      }
    } else {
      UIViewPtr prevView = *(--it);
      if (point.x >= prevView->getFrame().right()) {
        // �÷� ���̸� Ŭ����, ���� �÷� right - 1 ��ġ�� ����(righ, bottom is exclusive!)
        if (event.isMouseEvent()) {
          event._xy.x = prevView->getFrame().right() - 1;
        }
      }
    }
  } else {
    // ���� ������ ������ �ƴ� ���(mouseMove()���� ȣ��� ���, �����ϰ� ����)
    //SR_ASSERT(0);
    return nullptr;
  }
  return UIView::hitTest(event);
}

void SRColumnView::draw(CGContextPtr ctx, SRRect rect) const {
  beginDraw(ctx);

  // ���� ���� ��ũ�� ����
  SR_ASSERT(getBounds().origin.x == 0 && getBounds().origin.y == 0); // ����� UIScrollView�� ��쿡�� _bounds.origin�� ������
  ctx->translateBy(-getBounds().origin.x, -getBounds().origin.y);

  // �ؽ�Ʈ�䰡 ������ ��Ʈ ������ �ؽ�Ʈ���� frame ��ġ�� ������Ѵ�.
  static CGContextPtr ctxRender = nullptr;
  CGAffineTransform ctm = ctx->getTextMatrix();
//#define __forceFullRedrawForSelection
#ifndef __forceFullRedrawForSelection
  if (_partialRect.top() == 0) {
#endif
    // ù �������� ��� ��ü ���� ������ ����� ��, ctxRender �� ������
    SRRect rcSrc({ 0, 0 }, getDocumentView()->getTotalPageSize()); // ��Ʈ ���� ��ü ������
    SRRect rcDest({ ctm.tx, ctm.ty }, _partialRect.size);
    void* destDC = ctx->getSrcDC(); // ctxRender�� ctx(������ dc)�� _srcDC(�޸� dc)�� ������ϰ� ����
    ctxRender = CGContext::create(WIN32GraphicContextImpl::create(nullptr, rcSrc, rcDest, destDC));
    ctxRender->_selectedRange = ctx->_selectedRange;
    ctxRender->setDebugDraw(ctx->debugDraw());

    // draw background, border
    UIView::drawBackground(ctxRender, rcSrc);
    UIView::drawBorder(ctxRender, rcSrc);
    textContainer()->rootRender()->draw(ctxRender);
    //textContainer()->rootRender()->beginDraw(ctxRender);
    textContainer()->drawExclusionPath(ctxRender);
    //textContainer()->rootRender()->endDraw(ctxRender);
    ctxRender->flush(); // ctx(������ dc)�� _srcDC(�޸� dc)�� ����� ����
#ifndef __forceFullRedrawForSelection
  } else {
#else
  if (_partialRect.top() != 0) {
#endif
    // �� ��° ���������ʹ� ù ������ ����׽� �����ߴ� ctxRender ���� ������ ������ �����ؼ� �����
    SRRect rcSrc = _partialRect; // ���� �÷��� ������� ������ ����
    SRRect rcDest({ ctm.tx, ctm.ty }, _partialRect.size);
    void* destDC = ctx->getSrcDC(); // ctxRender�� ctx(������ dc)�� _srcDC(�޸� dc)�� ������ϰ� ����
    ctxRender->flush(destDC, &rcDest, &rcSrc.origin);
  }
  endDraw(ctx);
  //return true;
}

SRRect SRColumnView::caretRect(UITextPosition position) const {
  // ������Ʈ �������� ĳ�� ��ǥ�� ���Ͻ�Ű��, SRDocumentView���� ������ ��ǥ�� ��ȯ�ؼ� ó���ϰ���
  SRRect caretRect = textContainer()->rootRender()->caretRect(position);
  return caretRect;
}

UITextPosition SRColumnView::position(UITextRange range, UITextLayoutDirection farthest) const {
  UITextPosition position = range._range[0];
  SRRange lineRange = textContainer()->rootRender()->lineRange(position);
  UITextPosition result;
  switch (farthest) {
  case left:
    result._offset = lineRange.location;
    break;
  case right:
  {
    std::wstring str = textContainer()->layoutManager()->textStorage()->getString()->data();
    result._offset = lineRange.rangeMax();
    if (result._offset > 0 && str.at(result._offset - 1) == '\n') {
      //result._stickToNextLine = false; // ���� ������ ���� ĳ���� ǥ�õǰ� ��
      result._offset -= 1;
    } else {
      result._stickToNextLine = false; // ���� ������ ���� ĳ���� ǥ�õǰ� ��
    }
  }
  break;
  default:
    SR_ASSERT(0);
  }
  return result;
}

UITextPosition SRColumnView::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) const {
  // ���� �� ��ǥ -> ������Ʈ ��ǥ
  point += _partialRect.origin;
  SRPoint orgPoint = point;

  // ���� ���� _partialRect �̳��� ��ǥ�� ������Ų��(������ ���� Ŭ���� ó���� ����)
  point.x = std::max(point.x, _partialRect.left());
  point.x = std::min(point.x, _partialRect.right() - 1);
  point.y = std::max(point.y, _partialRect.top());
  point.y = std::min(point.y, _partialRect.bottom() - 1);
  if (orgPoint != point) {
    int a = 0;
  }

  return UITextView::closestPosition(point, direction, position);
}

void SRColumnView::mouseDown(UIEvent event) {
  SRPoint point = event.mouseLocation();
  //SR_LOGD(L"SRColumnView(%p)::mouseDown() point=[%.1f, %.1f] scroll=[%.1f, %.1f]"
  //  , this, point.x, point.y, window()->contentOffset().x, window()->contentOffset().y);

  SRTextPosition textPos = closestPosition(point);
  if (textPos._offset == SRNotFound) {
    //SR_LOGD(L"return invalid caret position!");
    return;
  }
  SRRect rcCaret = caretRect(textPos); // ������Ʈ �������� ĳ�� ��ǥ ����
  if (rcCaret.origin.y < _partialRect.top()) {
    // ���� �������� ĳ���� ��ġ�ؾ� �ϴ� ���, ĳ�� ��ġ�� �ٲ��� �ʴ´�(like ms-word)
    // �ϳ��� ���̺� �÷��� ���� �������� ���� �����ϴ� ���, Ŭ���� ĳ���� ���� �������� ��ġ�ؾ� �ϴ� ��찡 ������
    return;
  }

  window()->hideCaret();
  if (event.isShiftPressed()) {
    // ���� ���� ����
    window()->setTextPositions(nullptr, &textPos);
    window()->invalidateRect(NULL, false);
  } else {
    // ĳ�� ǥ��
    if (window()->hasSelection()) {
      // ���� ������ �־����� ���� ������ ����� ���� �ٽ� �������
      window()->invalidateRect(NULL, false);
    }
    window()->setTextPositions(&textPos, &textPos);
    window()->updateCaret();
    window()->showCaret();
  }
}

void SRColumnView::mouseUp(UIEvent event) {
}

void SRColumnView::mouseMoved(UIEvent event) {
  SRPoint point = event.mouseLocation();
  //SR_LOGD(L"SRColumnView(%p)::mouseMoved() point=[%.1f, %.1f] scroll=[%.1f, %.1f]"
  //  , this, point.x, point.y, window()->contentOffset().x, window()->contentOffset().y);

  SRTextPosition textPos = closestPosition(point);
  if (textPos._offset == SRNotFound) {
    SR_LOGD(L"return invalid caret position!");
    return;
  }

  if (event.pressedMouseButtons() == LEFT_MOUSE_BUTTON) {
    const UITextPosition* prePos = window()->textPositions();
    if (prePos[1]._offset != textPos._offset) {
      // ���� ������ �߻��� ���, ĳ���� ����� ���� ���� ǥ�ø� ���� ȭ���� ���Ž�Ų��.
      window()->hideCaret();
      window()->setTextPositions(nullptr, &textPos);
      window()->invalidateRect(NULL, false);
    }
  }
}

void SRColumnView::cursorUpdate(UIEvent event) {
  //SR_LOGD(L"[cursor] SRColumnView::cursorUpdate() IDC_IBEAM");
  ::SetCursor(::LoadCursor(NULL, IDC_IBEAM));
}

} // namespace sr
