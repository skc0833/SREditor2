#pragma once

#include "SRBase.h"
#include "SRTextStorage.h"
#include "../CoreText/RenderObject.h"
#include <vector>

// https://developer.apple.com/library/prerelease/content/documentation/TextFonts/Conceptual/CocoaTextArchitecture/TextSystemArchitecture/ArchitectureOverview.html#//apple_ref/doc/uid/TP40009459-CH7-SW4

namespace sr {

class SRTextContainer;
using SRTextContainerPtr = std::shared_ptr<SRTextContainer>;
using SRTextContainerWeakPtr = std::weak_ptr<SRTextContainer>;

class SRLayoutManager;
using SRLayoutManagerPtr = std::shared_ptr<SRLayoutManager>;
using SRLayoutManagerWeakPtr = std::weak_ptr<SRLayoutManager>;

using SRTextContainerList = std::vector<SRTextContainerPtr>;

class SRLayoutManagerDelegate {
public:
  // Informs the delegate that the given layout manager has invalidated layout information (not glyph information).
  // This method is invoked only when layout was complete and then became invalidated for some reason.
  // Delegates can use this information to show an indicator of background layout or to enable a button that forces immediate layout of text.
  // 레이아웃이 완성되고 나서 다른 이유로 무효화됐을 때 호출된다.
  virtual void layoutManagerDidInvalidateLayout(SRLayoutManagerPtr sender) = 0;

  // Informs the delegate that the given layout manager has finished laying out text in the given text container.
  // paginating 시에 유용함
  virtual void didCompleteLayoutFor(SRLayoutManagerPtr sender, SRTextContainerPtr container, bool atEnd) = 0;
};
using SRLayoutManagerDelegatePtr = std::shared_ptr<SRLayoutManagerDelegate>;

class SRLayoutManager : public SRObject, public std::enable_shared_from_this<SRLayoutManager> {
  SR_MAKE_NONCOPYABLE(SRLayoutManager);
public:
  SR_DECL_CREATE_FUNC(SRLayoutManager);

  SRLayoutManager();
  virtual ~SRLayoutManager();

  void setDelegate(SRLayoutManagerDelegatePtr delegate) {
    _delegate = delegate;
  }

  // Managing the Text Containers
  //
  const SRTextContainerList& textContainers() const {
    return _textContainers;
  }
  void addTextContainer(const SRTextContainerPtr textContainer);

  // Accessing the Text Storage
  //
  SRTextStoragePtr textStorage() {
    return _textStorage.lock();
  }
  void setTextStorage(SRTextStoragePtr storage) {
    _textStorage = storage;
  }

  // SRTextStorage.processEditing() 로부터 LM에게 에디팅을 알릴 경우 호출된다.
  // e.g, "The files" -> "Several files", newCharRange(editedRange) is {0, 7} and delta(changeInLength) is 4
  // textStorage(edited, ...) 함수는 deprecated 함수임
  void processEditing(SRTextStoragePtr textStorage, 
    SRInt editMask, // NSTextStorageEditActions: NSTextStorageEditedAttributes, NSTextStorageEditedCharacters
    SRRange newCharRange, // 변경된 문자열 범위(The range in the final string that was explicitly edited)
    SRInt delta,     // The length delta for the editing changes.
    SRRange invalidatedCharRange); // 속성이 보정된 영역(newCharRange 이상임)
                                   // The range of characters that changed as a result of attribute fixing.
                                   // This invalidated range is either equal to newCharRange or larger.

  // Draws background marks for the given glyph range, which must lie completely within a single text container.
  // Background marks are such things as selection highlighting, text background color, 
  // and any background for marked text, along with block decoration such as table backgrounds and borders.
  // 드로잉을 위해 NSTextView 로부터 호출된다.
  // Background marks 는 선택영역 반전, 배경색, 테이블 경계선등을 포함한다.
  // lockFocus() ???
  // origin: The position of the text container in the coordinate system of the currently focused view.
  void drawBackground(const SRRange& glyphsToShow, const SRPoint& origin);

  // draws the actual glyphs, including attachments, as well as any underlines or strikethoughs.
  void drawGlyphs(const SRRange& glyphsToShow, const SRPoint& origin);

  // Returns the bounding rectangle for the glyphs laid out in the given text container.
  SRRect usedRectForTextContainer(SRTextContainerPtr tc);

private:
  void layoutAllText();
  CTFramePtr makeFrame(const SRRect layoutRect, const SRRange range, const SRTextContainerPtr tc, CTFramesetterPtr _framesetter);

  RenderObjectPtr layoutText(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextBlockPtr textBlock);
  RenderObjectPtr layoutTable(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextTablePtr table);
  RenderObjectPtr layoutList(RenderObjectPtr parentRender, SRRect layoutRect, SRRange range, SRTextListPtr list);
  RenderObjectPtr layoutBlock(RenderObjectPtr parentRender, SRSize requestlayoutSize, SRRange range, const SRTextBlockPtr targetBlock);

  void invalidateDisplay(const SRRange& charRange);

  SRTextStorageWeakPtr _textStorage;
  SRLayoutManagerDelegatePtr _delegate;
  bool _needsLayout; // not used

  SRTextContainerWeakPtr _curTextContainer;
  SRRect _usedRect;
  CTFramesetterPtr _framesetter;

//public:
  SRTextContainerList _textContainers;
};

} // namespace sr
