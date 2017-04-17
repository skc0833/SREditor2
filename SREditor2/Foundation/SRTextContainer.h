#pragma once

#include "SRObject.h"
#include "SRLayoutManager.h"
#include "../CoreText/CTFrame.h"
#include "../CoreText/RenderObject.h"
#include <functional> // for std::function

namespace sr {

class UITextView;
using UITextViewPtr = std::shared_ptr<UITextView>;
using UITextViewWeakPtr = std::weak_ptr<UITextView>;

class SRTextContainer;
using SRTextContainerPtr = std::shared_ptr<SRTextContainer>;

class SRTextContainer : public SRObject {
  SR_MAKE_NONCOPYABLE(SRTextContainer);
public:
  SR_DECL_CREATE_FUNC(SRTextContainer);

  SRTextContainer(const SRSize& size = SRSize());
  virtual ~SRTextContainer() = default;

  SRSize size() const { return _size; }
  void setSize(SRSize size) { _size = size; }

  const UITextViewPtr textView() const;
  void setTextView(UITextViewPtr textView) {
    _textView = textView;
  }

  SRRect lineFragmentRect(SRRect proposedRect, SRUInt characterIndex
    , SRWritingDirection baseWritingDirection, SRRect* remainingRect);

  SRBool isSimpleRectangularTextContainer() const {
    //return _isSimpleRectangularTextContainer;
    return _exclusionPaths.size() == 0;
  }
  //void setSimpleRectangularTextContainer(SRBool isSimpleRectangularTextContainer) {
  //  _isSimpleRectangularTextContainer = isSimpleRectangularTextContainer;
  //}

  SRLayoutManagerPtr layoutManager() const {
    return _layoutManager.lock();
  }
  void setLayoutManager(SRLayoutManagerPtr layoutManager) {
    _layoutManager = layoutManager;
  }

  const RenderRootPtr rootRender() const {
    return _rootRender;
  }
  void setRootRender(const RenderRootPtr rootRender) {
    _rootRender = rootRender;
  }
  SRRange textRange() const {
    return _textRange;
  }
  void setTextRange(SRRange textRange) {
    _textRange = textRange;
  }

  typedef struct ExclusionPath {
    SRRect _rect;
    SRObjectPtr _obj;
    std::function<void(CGContextPtr ctx, SRObjectPtr obj)> _func;
    explicit ExclusionPath(SRRect rect, SRObjectPtr obj, std::function<void(CGContextPtr ctx, SRObjectPtr obj)> func)
      : _rect(rect), _obj(obj), _func(func) {
    }
  } ExcludePath;

  void addExclusionPath(ExclusionPath path);
  void drawExclusionPath(CGContextPtr ctx) const;
  SRRect getTotalExclusionPath() const;

private:
  SRSize _size; // 컨테이너 크기
  SRRange _textRange; // 포함된 SRTextStorage내의 텍스트 범위
  RenderRootPtr _rootRender;

  SRLayoutManagerWeakPtr _layoutManager;
  UITextViewWeakPtr _textView; // SRTextStorage -> SRLayoutManager -> SRTextContainer -> UITextView

  // A boolean that indicates whether the receiver’s region is a rectangle with no holes or gaps 
  // and whose edges are parallel to the text view's coordinate system axes.
  // 컨테이너가 단순 사각형 영역인지 여부
  //SRBool _isSimpleRectangularTextContainer;

  std::vector<ExclusionPath> _exclusionPaths;
};

} // namespace sr
