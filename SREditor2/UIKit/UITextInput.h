#pragma once

#include <memory> // for std::shared_ptr
#include "../Foundation/SRDataTypes.h"
#include "../Foundation/SRString.h"

namespace sr {

// represents a position in a text container;
// in other words, it is an index into the backing string in a text-displaying view.
// 절대 위치(absolute position)와 비주얼 텍스트상의 위치(position in the visible text) 추적에 필요함
typedef SRTextPosition UITextPosition;

typedef struct UITextRange {
  UITextRange(UITextPosition p0, UITextPosition p1) {
    _range[0] = p0;
    _range[1] = p1;
  }
  UITextPosition _range[2];
} UITextRange;

typedef enum UITextLayoutDirection {
  none, left, right, up, down
} UITextLayoutDirection;

// A subclass of UIResponder can adopt this protocol to implement simple text entry.
// When instances of this subclass are the first responder, the system keyboard is displayed.
class UIKeyInput {
public:
  // Add the character text to your class'ss backing store at the index 
  // corresponding to the cursor and redisplay the text.
  //virtual void insertText(SRString text) = 0;
  //// Remove the character just before the cursor from your class's backing store and redisplay the text.
  //virtual void deleteBackward() = 0;
  //// true if the backing store has textual content, false otherwise.
  //virtual bool hasText() = 0;
};

class UITextInput;
using UITextInputPtr = std::shared_ptr<UITextInput>;

// The protocol you implement to interact with the text input system and enable features 
// such as autocorrection and multistage text input in documents.
class UITextInput : public UIKeyInput {
public:
  // Return the position in a document that is closest to a specified point.
  virtual UITextPosition closestPosition(SRPoint point, UITextLayoutDirection direction = none
    , SRTextPosition position = SRTextPosition()) const = 0;
  
  // Return a rectangle used to draw the caret at a given insertion point.
  // 윈도우 좌표로 리턴함
  virtual SRRect caretRect(UITextPosition position) const = 0;

  // Replace the text in a document that is in the specified range.
  virtual void replace(UITextRange range, const SRString& text) const = 0;

  // Returns the text position at a given offset in a specified direction from another text position.
  virtual UITextPosition position(UITextPosition from, UITextLayoutDirection direction, SRIndex offset) const = 0;

  // Return the text position that is at the farthest extent in a given layout direction within a range of text.
  virtual UITextPosition position(UITextRange range, UITextLayoutDirection farthest) const = 0;

  // Return a text range from a given text position to its farthest extent in a certain direction of layout.
  //virtual UITextRange characterRange(UITextPosition position, UITextLayoutDirection direction) const = 0;

  //selectedTextRange: UITextRange // UIWindow::_textPositions[2] 가 대신하고 있음, 원래 UITextView::selectedRange도 있음
  //void setTextInputView(UIViewPtr view) {
  //  _textInputView = view;
  //}
  //UIViewPtr textInputView() const {
  //  return _textInputView;
  //}

private:
  // An affiliated view that provides a coordinate system for all geometric values in this protocol.
  //UIViewPtr _textInputView;
};

} // namespace sr
