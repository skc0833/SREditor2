#pragma once

#include "SRObject.h"
#include "SRAttributedString.h"
#include "CTTypesetter.h"

namespace sr {

class SRTextContainer;
using SRTextContainerPtr = std::shared_ptr<SRTextContainer>;

class CTFrame;
using CTFramePtr = std::shared_ptr<CTFrame>;

class CTFramesetter;
using CTFramesetterPtr = std::shared_ptr<CTFramesetter>;

/*
https://developer.apple.com/reference/coretext/ctframesetter-2eg
The CTFramesetter opaque type is used to generate text frames. 
That is, CTFramesetter is an object factory for CTFrame objects.

The framesetter takes an attributed string object and a shape descriptor object 
and calls into the typesetter to create line objects that fill that shape.
The output is a frame object containing an array of lines.
The frame can then draw itself directly into the current graphic context.
*/
class CTFramesetter : public SRObject, public std::enable_shared_from_this<CTFramesetter> {
  SR_MAKE_NONCOPYABLE(CTFramesetter);
public:
  SR_DECL_CREATE_FUNC(CTFramesetter);

  // Creates an immutable framesetter object from an attributed string.
  CTFramesetter(const SRAttributedStringPtr string);
  virtual ~CTFramesetter() = default;

  // Creates an immutable frame using a framesetter.
  // SRObjectDictionaryPtr attributes 인자는 생략했음
  CTFramePtr createFrame(SRRange stringRange, const SRPath frameRect, const SRTextContainerPtr tc = nullptr);

  // Returns the typesetter object being used by the framesetter.
  //CTTypesetterRef CTFramesetterGetTypesetter();

  // Determines the frame size needed for a string range.
  SRSize suggestFrameSizeWithConstraints(SRRange stringRange, SRObjectDictionaryPtr frameAttributes
    , SRSize constraints, SRRange* fitRange);

private:
   CTTypesetterPtr _typesetter;
};

} // namespace sr
