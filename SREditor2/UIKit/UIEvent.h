#pragma once

#include <chrono> // for std::chrono::time_point
//#include "UITextInput.h"

namespace sr {

// UIEvent
using UITimeInterval = std::chrono::time_point<std::chrono::system_clock>;

typedef enum {
  KEY_DOWN,
  KEY_UP,
  LEFT_MOUSE_DOWN,
  LEFT_MOUSE_UP,
  RIGHT_MOUSE_DOWN,
  RIGHT_MOUSE_UP,
  MOUSE_MOVE,
  SET_CURSOR,
} UIEventType;
typedef enum {
  NO_KEY,
  LEFT_KEY,
  RIGHT_KEY,
  UP_KEY,
  DOWN_KEY,
  HOME_KEY,
  END_KEY,
  DELETE_KEY,
  A_KEY,
} UIKeyCode;
enum {
  NO_META = 0,
  SHIFT_KEY = 1 << 0,
  CTRL_KEY = 1 << 1,
  ALT_KEY = 1 << 2
};
enum {
  NO_MOUSE_BUTTON = 0,
  LEFT_MOUSE_BUTTON = 1 << 0,
  RIGHT_MOUSE_BUTTON = 1 << 1
};

typedef struct UIEvent {
  UIEventType _type;
  UITimeInterval timestamp;
  // mouse event
  SRPoint _xy;
  SRUInt _pressedMouseButtons;
  // key event
  SRUInt _metaKey;
  UIKeyCode _keyCode;
  // skc added
  SRBool isShiftPressed() {
    return _metaKey & SHIFT_KEY;
  }
  SRBool isCtrlPressed() {
    return _metaKey & CTRL_KEY;
  }

  UIEvent(UIEventType type, SRFloat x, SRFloat y, SRUInt metaKey = NO_META)
    : _type(type), _xy(x, y), _metaKey(metaKey) {
    timestamp = std::chrono::system_clock::now();
    _pressedMouseButtons = NO_MOUSE_BUTTON;
  }
  UIEvent(UIEventType type, UIKeyCode keyCode, SRUInt metaKey = NO_META)
    : _type(type), _keyCode(keyCode), _metaKey(metaKey) {
    timestamp = std::chrono::system_clock::now();
  }
  SRPoint mouseLocation() {
    return _xy;
  }
  SRBool isMouseEvent() {
    if (_type == MOUSE_MOVE || _type == LEFT_MOUSE_DOWN || _type == LEFT_MOUSE_UP)
      return true;
    return false;
  }
  SRUInt pressedMouseButtons() {
    return _pressedMouseButtons;
  }
} UIEvent;

// An abstract interface for responding to and handling events.
class UIResponser { // from NSResponder
public:
  // Responding to Mouse Events
  //
  // Informs the receiver that the user has pressed the left mouse button.
  virtual void mouseDown(UIEvent event) = 0;

  // Informs the receiver that the user has moved the mouse with the left button pressed.
  //virtual void mouseDragged(UIEvent event) = 0;

  // Informs the receiver that the user has released the left mouse button.
  virtual void mouseUp(UIEvent event) = 0;

  // Informs the receiver that the mouse has moved.
  virtual void mouseMoved(UIEvent event) = 0;

  //// Informs the receiver that the cursor has entered a tracking rectangle.
  //virtual void mouseEntered(UIEvent event) = 0; // TODO

  //// Informs the receiver that the cursor has exited a tracking rectangle.
  //virtual void mouseExited(UIEvent event) = 0;

  // Responding to Key Events
  //
  // Informs the receiver that the user has pressed a key.
  virtual void keyDown(UIEvent event) = 0;

  // Informs the receiver that the user has released a key.
  virtual void keyUp(UIEvent event) = 0;

  // Responding to Other Kinds of Events
  //
  // Informs the receiver that the mouse cursor has moved into a cursor rectangle.
  virtual void cursorUpdate(UIEvent event) = 0;

  // Informs the receiver that the mouse's scroll wheel has moved.
  virtual void scrollWheel(UIEvent event) = 0;
};

} // namespace sr
