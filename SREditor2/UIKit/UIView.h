#pragma once

#include "../CoreGraphics/CGContext.h"
#include "UILayoutConstraint.h"
#include "UIEvent.h"
#include "UITextInput.h"
#include <vector>
#include <chrono>

namespace sr {

typedef enum {
  UIViewAutoresizingNone = 0,
  UIViewAutoresizingFlexibleLeftMargin = 1 << 0,
  UIViewAutoresizingFlexibleWidth = 1 << 1,
  UIViewAutoresizingFlexibleRightMargin = 1 << 2,
  UIViewAutoresizingFlexibleTopMargin = 1 << 3,
  UIViewAutoresizingFlexibleHeight = 1 << 4,
  UIViewAutoresizingFlexibleBottomMargin = 1 << 5
} UIViewAutoresizing;

typedef struct UIEdgeInsets {
  UIEdgeInsets(SRFloat left, SRFloat top, SRFloat right, SRFloat bottom)
    : left(left), top(top), right(right), bottom(bottom) {}
  SRFloat left, top, right, bottom;
} UIEdgeInsets;

class UIView;
using UIViewPtr = std::shared_ptr<UIView>;
using UIViewWeakPtr = std::weak_ptr<UIView>;
using UIViewList = std::vector<UIViewPtr>;

class UIWindow;
using UIWindowPtr = std::shared_ptr<UIWindow>; // UIWindow.h 를 포함시키면 상호참조로 컴파일 에러 발생중
using UIWindowWeakPtr = std::weak_ptr<UIWindow>;

using NSLayoutConstraintList = std::vector<UILayoutConstraint>;

// UITextInput은 원래 UITextView가 상속받고 있지만, 편의를 위해 여기서 상속받음
// (UIWindow::updateCaret()에서 caretRect(_textPositions[0]) 호출 부분 참고)
class UIView : public SRObject
             , public std::enable_shared_from_this<UIView>
             , public UIResponser
             , public UITextInput {
  SR_MAKE_NONCOPYABLE(UIView);
public:
  SR_DECL_CREATE_FUNC(UIView);

  UIView(const SRRect& frame = SRRect());
  virtual ~UIView();

  // template 대신 UIView 타입을 사용하면 사용처에서 dynamic_pointer_cast 등으로 캐스팅이 필요함
  template<typename T>
  /*virtual*/ std::shared_ptr<T> getPtr(T* ptr) {
    return std::dynamic_pointer_cast<T>(ptr->shared_from_this());
  }

  // Returns the farthest descendant of the receiver in the view hierarchy (including itself)
  // that contains a specified point.
  // point: A point specified in the receiver's local coordinate system (bounds).
  // skc added direction 인자 추가함(제거하자), 원래 UIEvent* 인자가 있어야 한다.
  virtual const UIViewPtr hitTest(UIEvent event) const;
  virtual SRBool pointInside(SRPoint point) const;

  // Lays out the subviews immediately.
  // 하위뷰들을 바로 레이아웃시킨다. 드로잉 전에 하위뷰들을 강제로 레이아웃팅하고 싶을 경우 사용된다.
  virtual void layoutIfNeeded();

  // Invalidates the current layout of the receiver and triggers a layout update during the next update cycle.
  // 현재 뷰의 레이아웃을 무효화시키고, 다음번 update cycle 에 레이아웃을 예약한다(현재는 update cycle 지원 안함!!!)
  // 이 함수를 통해 여러 뷰에 대한 레이아웃이 one update cycle 에 이뤄질수 있다(성능향상)
  void setNeedsLayout();
  //void setNeedsDisplay();

  // 하위뷰들을 레이아웃팅한다. 이 함수를 직접 호출하지 말고 layoutIfNeeded()나 setNeedsLayout()를 호출해 사용해라!
  void layoutSubviews();

  // Draws the receiver's image within the passed-in rectangle.
  // rect: 갱신돼야 할 현재뷰의 로컬좌표 영역(The portion of the view's bounds that needs to be updated)
  virtual void draw(CGContextPtr ctx, SRRect rect) const;
  virtual void drawBackground(CGContextPtr ctx, SRRect rect) const;
  virtual void drawBorder(CGContextPtr ctx, SRRect rect) const;

  virtual bool setFrame(const SRRect& frame);
  virtual void setSize(const SRSize& size);
  virtual void setOrigin(const SRPoint& origin);
  virtual void alignCenter();
  virtual SRPoint getAlignCenter() const; // 중앙 정렬시킬 좌표를 리턴함

  SRRect getBounds() const;
  SRRect getFrame() const;

  SRUInt getAutoresizingMask() const {
    return _autoresizingMask;
  }
  void setAutoresizingMask(SRInt mask) {
    _autoresizingMask = mask;
  }
  SRBool getTranslatesAutoresizingMaskIntoConstraints() const {
    SR_NOT_IMPL();
    return _translatesAutoresizingMaskIntoConstraints;
  }
  void setTranslatesAutoresizingMaskIntoConstraints(SRBool flag) {
    SR_NOT_IMPL();
    _translatesAutoresizingMaskIntoConstraints = flag;
  }

  // Adds a constraint on the layout of the receiving view or its subviews.
  void addConstraint(const UILayoutConstraint& constraint); // TODO

  SRBool getAutoCenterInSuperview() const {
    return _autoCenterInSuperview;
  }
  void setAutoCenterInSuperview(SRBool flag) {
    _autoCenterInSuperview = flag;
  }

  virtual void addSubview(const UIViewPtr view);
  virtual void removeSubviews();
  virtual void removeFromSuperview();

  void setBackgroundColor(const SRColor& color) { _backgroundColor = color; }
  SRColor backgroundColor() const { return _backgroundColor; }

  void setBorderColor(const SRColor& color) { _borderColor = color; }
  SRColor borderColor() const { return _borderColor; }
  void setBorderWidth(const SRFloat& width) { _borderWidth = width; }
  SRFloat borderWidth() const { return _borderWidth; }

  void setWindow(const UIWindowPtr& window) {
    _window = window;
  }
  UIWindowPtr window() const {
    return _window.lock();
  }
  UIViewPtr superview() const {
    return _superview.lock();
  }

  UIViewList getSubviews() const {
    return _subviews;
  }
  //const UIViewList& getSubviews() {
  //  return _subviews;
  //}

  // rect 영역을 to 뷰 좌표공간의 영역을 변환해 리턴한다
  virtual SRRect convertTo(SRRect rect, UIViewPtr to = nullptr) const;
  // point: 현재 뷰의 로컬좌표(A point specified in the local coordinate system (bounds) of the receiver)
  virtual SRPoint convertTo(SRPoint point, UIViewPtr to = nullptr) const;

  UIEdgeInsets getLayoutMargins() const {
    return _layoutMargins;
  }
  void setLayoutMargins(UIEdgeInsets margins) {
    _layoutMargins = margins;
  }

  // Resizes and moves the receiver view so it just encloses its subviews.
  void sizeToFit();

  // Asks the view to calculate and return the size that best fits the specified size.
  SRSize sizeThatFits(SRSize size = {0, 0});

  // skc added
  virtual SRRect getContentsRect() const;
  virtual SRPoint fitToContent(UIEvent event) const;

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
  virtual UITextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) const override;
  virtual SRRect caretRect(UITextPosition position) const override;
  virtual void replace(UITextRange range, const SRString& text) const override;
  virtual UITextPosition position(UITextPosition from, UITextLayoutDirection direction, SRIndex offset) const override;
  virtual UITextPosition position(UITextRange range, UITextLayoutDirection farthest) const override;

protected:
  virtual bool isEqual(const SRObject& obj) const override;
  virtual void beginDraw(CGContextPtr ctx) const;
  virtual void endDraw(CGContextPtr ctx) const;

  void adjustSubviews(SRSize parentSize, SRSize delta);

  // view's location and size using the parent view's coordinate system
  // Important for: placing the view in the parent
  // 부모뷰 내에서의 영역(origin and dimensions)
  // the frame of a view is defined as the smallest bounding box of that view with respect to 
  // it's parents coordinate system, including any transformations applied to that view
  // (프레임은 transformation 이 적용된 이후의 최소 사각형 영역임 -> 이 경우 _frame.size != _bounds.size)
  SRRect _frame; // _center, _bounds 로 계산됨(for debug 디버깅용으로 불필요하지만 추가함)
  
  // The center property can be used to adjust the position of the view without changing its size. 
  // frame.origin = center - bounds.size / 2
  // center = frame.origin + bounds.size / 2
  SRPoint _center;

  // a view's location and size using its own coordinate system(local coordinate system)
  // Important for: placing the view's content or subviews within itself
  // 자기뷰 내에서의 영역(자기뷰 컨텐츠 또는 자식뷰들을 배치)
  // 아래로 스크롤시 _bounds.origin 값은 음수임
  SRRect _bounds;

  // superview 의 bounds 가 변경될때의 현재 뷰의 resize 방식
  SRUInt _autoresizingMask;

  // A Boolean value that determines whether the view’s autoresizing mask is translated into Auto Layout constraints.
  SRBool _translatesAutoresizingMaskIntoConstraints;

  SRBool _autoCenterInSuperview; // TODO UIViewContentMode:center 에 해당???

  // The constraints used by Auto Layout on this UIView.
  // Auto Layout uses the UIView.Constraints of a UIView to lay out its UIView.Subviews.
  NSLayoutConstraintList _constraints;

  UIViewWeakPtr _superview;
  UIViewList _subviews;

  SRColor _backgroundColor;
  SRColor _borderColor;
  SRFloat _borderWidth;

  UIWindowWeakPtr _window;
  bool _needsLayout; // not used

  // The default spacing to use when laying out content in the view.
  // specify the desired amount of space (measured in points) between the edge of the view and any subviews.
  UIEdgeInsets _layoutMargins = {0, 0, 0, 0};
};

} // namespace sr
