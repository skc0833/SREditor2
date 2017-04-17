#pragma once

#include "UIScrollView.h"
#include "UITextInput.h"
#include "SRTextContainer.h"
#include "SRAttributedString.h"

namespace sr {

class UITextView;
using UITextViewPtr = std::shared_ptr<UITextView>;
using UITextViewWeakPtr = std::weak_ptr<UITextView>;

// UITextView -> UIScrollView -> UIView
class UITextView : public UIScrollView {
  SR_MAKE_NONCOPYABLE(UITextView);
public:
  SR_DECL_CREATE_FUNC(UITextView);

  UITextView(const SRRect& frame);
  virtual ~UITextView();

  // Lays out the subviews immediately.
  // 하위뷰들을 바로 레이아웃시킨다. 드로잉 전에 하위뷰들을 강제로 레이아웃팅하고 싶을 경우 사용된다.
  virtual void layoutIfNeeded();

  virtual void draw(CGContextPtr ctx, SRRect rect) const override;

  void setAttributedText(const SRAttributedStringPtr text);

  SRTextContainerPtr textContainer() const {
    return _textContainer.lock();
  }

  // Scrolls the receiver until the text in the specified range is visible.
  void scrollRangeToVisible(SRRange range) const;

  void init(SRTextContainerPtr textContainer);

  // UITextInput
  //
  virtual UITextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) const override;
  virtual SRRect caretRect(UITextPosition position) const override;
  virtual void replace(UITextRange range, const SRString& text) const override;

private:
  // The text container object defining the area in which text is displayed in this text view.
  SRTextContainerWeakPtr _textContainer;
};

} // namespace sr
