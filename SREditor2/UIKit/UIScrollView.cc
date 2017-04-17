#include "UIScrollView.h"
#include "UIWindow.h"

namespace sr {

UIScrollView::UIScrollView(const SRRect& frame)
 : UIView(frame) {
}

void UIScrollView::setContentOffset(SRPoint contentOffset) {
  _bounds.origin = -contentOffset; // ��ũ�ѽÿ� _bounds ��ġ�� ������

  window()->setScrollPos(SR_SB_VERT, contentOffset.y, true/* redraw */);
  window()->setScrollPos(SR_SB_HORZ, contentOffset.x, true);
}

// �������� rect�� UIScrollView ��ǥ���� �Ѵ�.
// A rectangle defining an area of the content view.
// The rectangle should be in the coordinate space of the scroll view.
void UIScrollView::scrollRectToVisible(SRRect rect) {
  SRPoint curCO = contentOffset();
  SRPoint newCO = curCO;
  SRSize windowSize = window()->getFrame().size;
  if (rect.top() < curCO.y) {
    // ���� ��ũ��
    newCO.y = rect.top();
  } else if ((curCO.y + windowSize.height) < rect.bottom()) {
    // �Ʒ��� ��ũ��
    newCO.y = rect.bottom() - windowSize.height;
  }
  if (rect.left() < curCO.x) {
    // �·� ��ũ��
    newCO.x = rect.left();
  } else if ((curCO.x + windowSize.width) < rect.left()) {
    // ��� ��ũ��
    newCO.x = rect.left() - windowSize.width;
  }
  if (curCO != newCO) {
    setContentOffset(newCO);
  }
}

} // namespace sr
