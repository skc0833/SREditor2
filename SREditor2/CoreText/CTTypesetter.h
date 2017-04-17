#pragma once

#include "SRObject.h"
#include "SRAttributedString.h"
#include "CTLine.h"

namespace sr {

class CTTypesetter;
using CTTypesetterPtr = std::shared_ptr<CTTypesetter>;

typedef SRFloat(*WidthFinderFunc)(void* opaque, SRIndex idx, SRFloat offset, SRFloat height);

/*
https://developer.apple.com/reference/coretext/cttypesetter-61p
The CTTypesetter opaque type represents a typesetter, which performs line layout.
*/
// The typesetter creates glyph runs as it produces lines from character strings, attributes, and font objects. 
class CTTypesetter : public SRObject {
  SR_MAKE_NONCOPYABLE(CTTypesetter);
public:
  SR_DECL_CREATE_FUNC(CTTypesetter);

  // Creates an immutable typesetter object using an attributed string.
  CTTypesetter(const SRAttributedStringPtr string);
  virtual ~CTTypesetter() = default;

  // Creates an immutable line from the typesetter.
  CTLinePtr createLine(SRRange stringRange);

  // Suggests a contextual line breakpoint based on the width provided.
  CTLinePtr suggestLineBreak(SRRange range, double width);

  // skc added
  SRAttributedStringPtr attributedString() const;

private:
  SRFloat fillLine(const SRRange lineRange, std::vector<SRPoint>& glyphOrigins,
    std::vector<SRSize>& glyphAdvances, std::vector<wchar_t>& glyphCharacters,
    std::vector<SRIndex>& stringIndices, CTLinePtr outLine);
  SRIndex doWrap(SRRange range, SRFloat requestedLineWidth, CTLinePtr outLine = nullptr);
  SRIndex charactersLen();

  SRAttributedStringWeakPtr _attributedString;
};

} // namespace sr
