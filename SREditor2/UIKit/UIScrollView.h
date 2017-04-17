#pragma once

#include "UIView.h"

namespace sr {

class UIScrollView;
using UIScrollViewPtr = std::shared_ptr<UIScrollView>;

// The UIScrollView class provides support for displaying content that is larger than the size of the application’s window.
// 스크롤뷰는 스크롤바만 직접 그려줘야 한다.
class UIScrollView : public UIView {
  SR_MAKE_NONCOPYABLE(UIScrollView);
public:
  SR_DECL_CREATE_FUNC(UIScrollView);

  UIScrollView(const SRRect& frame = SRRect());
  virtual ~UIScrollView() = default;

  // The point at which the origin of the content view is offset from the origin of the scroll view.
  SRPoint contentOffset() const {
    return -getBounds().origin; // 아래로 스크롤시 getBounds().origin 값은 음수임
  }
  // Sets the offset from the content view’s origin that corresponds to the receiver’s origin.
  void setContentOffset(SRPoint contentOffset);

  // Scrolls a specific area of the content so that it is visible in the receiver.
  void scrollRectToVisible(SRRect rect);

private:
  // The point at which the origin of the content view is offset from the origin of the scroll view.
  //SRPoint _contentOffset;

  // The size of the content view.
  // 스크롤 가능한 전체 영역임(defines the size of the scrollable area)
  //SRSize _contentSize;
  
  // The distance that the content view is inset from the enclosing scroll view.
  // 보통 content 크기는 그대로 유지하면서 상하단에 여백을 줄 경우 사용함
  //UIEdgeInsets _contentInset;
};

} // namespace sr
