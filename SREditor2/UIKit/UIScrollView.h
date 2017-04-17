#pragma once

#include "UIView.h"

namespace sr {

class UIScrollView;
using UIScrollViewPtr = std::shared_ptr<UIScrollView>;

// The UIScrollView class provides support for displaying content that is larger than the size of the application��s window.
// ��ũ�Ѻ�� ��ũ�ѹٸ� ���� �׷���� �Ѵ�.
class UIScrollView : public UIView {
  SR_MAKE_NONCOPYABLE(UIScrollView);
public:
  SR_DECL_CREATE_FUNC(UIScrollView);

  UIScrollView(const SRRect& frame = SRRect());
  virtual ~UIScrollView() = default;

  // The point at which the origin of the content view is offset from the origin of the scroll view.
  SRPoint contentOffset() const {
    return -getBounds().origin; // �Ʒ��� ��ũ�ѽ� getBounds().origin ���� ������
  }
  // Sets the offset from the content view��s origin that corresponds to the receiver��s origin.
  void setContentOffset(SRPoint contentOffset);

  // Scrolls a specific area of the content so that it is visible in the receiver.
  void scrollRectToVisible(SRRect rect);

private:
  // The point at which the origin of the content view is offset from the origin of the scroll view.
  //SRPoint _contentOffset;

  // The size of the content view.
  // ��ũ�� ������ ��ü ������(defines the size of the scrollable area)
  //SRSize _contentSize;
  
  // The distance that the content view is inset from the enclosing scroll view.
  // ���� content ũ��� �״�� �����ϸ鼭 ���ϴܿ� ������ �� ��� �����
  //UIEdgeInsets _contentInset;
};

} // namespace sr
