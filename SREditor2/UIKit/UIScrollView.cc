#include "UIScrollView.h"
#include "UIWindow.h"

namespace sr {

UIScrollView::UIScrollView(const SRRect& frame)
 : UIView(frame) {
}

void UIScrollView::setContentOffset(SRPoint contentOffset) {
  _bounds.origin = -contentOffset; // 스크롤시에 _bounds 위치를 변경함

  window()->setScrollPos(SR_SB_VERT, contentOffset.y, true/* redraw */);
  window()->setScrollPos(SR_SB_HORZ, contentOffset.x, true);
}

// 전달인자 rect는 UIScrollView 좌표여야 한다.
// A rectangle defining an area of the content view.
// The rectangle should be in the coordinate space of the scroll view.
void UIScrollView::scrollRectToVisible(SRRect rect) {
  SRPoint curCO = contentOffset();
  SRPoint newCO = curCO;
  SRSize windowSize = window()->getFrame().size;
  if (rect.top() < curCO.y) {
    // 위로 스크롤
    newCO.y = rect.top();
  } else if ((curCO.y + windowSize.height) < rect.bottom()) {
    // 아래로 스크롤
    newCO.y = rect.bottom() - windowSize.height;
  }
  if (rect.left() < curCO.x) {
    // 좌로 스크롤
    newCO.x = rect.left();
  } else if ((curCO.x + windowSize.width) < rect.left()) {
    // 우로 스크롤
    newCO.x = rect.left() - windowSize.width;
  }
  if (curCO != newCO) {
    setContentOffset(newCO);
  }
}

} // namespace sr
