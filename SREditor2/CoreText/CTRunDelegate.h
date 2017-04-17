#pragma once

#include "SRObject.h"
#include "../CoreGraphics/CGContext.h"

namespace sr {

// Defines a pointer to a function that determines typographic ascent of glyphs in the run.
typedef SRFloat(*CTRunDelegateGetAscentCallback)(SRObjectPtr refCon);
typedef SRFloat(*CTRunDelegateGetDescentCallback)(SRObjectPtr refCon);
typedef SRFloat(*CTRunDelegateGetWidthCallback)(SRObjectPtr refCon);
// Defines a pointer to a function that is invoked when a CTRunDelegate object is deallocated.
typedef void(*CTRunDelegateDeallocateCallback)(SRObjectPtr refCon);
typedef void(*CTRunDelegateOnDrawCallback)(SRObjectPtr refCon, CGContextPtr ctx, SRPoint pt); // skc added

// A structure holding pointers to callbacks implemented by the run delegate.
typedef struct CTRunDelegateCallbacks {
  //SRIndex version;
  CTRunDelegateDeallocateCallback dealloc;
  CTRunDelegateGetAscentCallback getAscent;
  CTRunDelegateGetDescentCallback getDescent;
  CTRunDelegateGetWidthCallback getWidth;
  CTRunDelegateOnDrawCallback onDraw;

  CTRunDelegateCallbacks();
} CTRunDelegateCallbacks;

class CTRunDelegate;
using CTRunDelegatePtr = std::shared_ptr<CTRunDelegate>;

/*
https://developer.apple.com/reference/coretext/ctrundelegate-q5q
The CTRunDelegate opaque type represents a run delegate, which is assigned to a run (attribute range)
to control typographic traits such glyph ascent, glyph descent, and glyph width.
*/
class CTRunDelegate : public SRObject {
  SR_MAKE_NONCOPYABLE(CTRunDelegate);
public:
  SR_DECL_CREATE_FUNC(CTRunDelegate);

  CTRunDelegate(const CTRunDelegateCallbacks& cb, SRObjectPtr ref);
  virtual ~CTRunDelegate() = default;

  CTRunDelegateCallbacks cb() const { return _cb; }
  SRObjectPtr ref() const { return _ref; }

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  CTRunDelegateCallbacks _cb;
  SRObjectPtr _ref;
};

} // namespace sr
