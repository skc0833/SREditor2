#pragma once

#include "SRObject.h"
#include "SRDictionary.h"
#include "CoreGraphics/CoreGraphics.h"
#include <vector>

namespace sr {

class CTRun;
using CTRunPtr = std::shared_ptr<CTRun>;

/*
https://developer.apple.com/reference/coretext/ctrun-61n
The CTRun opaque type represents a glyph run, which is a set of consecutive glyphs
sharing the same attributes and direction.

The typesetter creates glyph runs as it produces lines from character strings, attributes, and font objects. 
That is, a line is constructed of one or more glyphs runs. Glyph runs can draw themselves into a graphic context, 
if desired, although most users have no need to interact directly with glyph runs.
*/
// 동일 속성과 텍스트 방향을 공유하는 연속된 글리프 집합
class CTRun : public SRObject {
  friend class CTTypesetter;
  friend class CTLine;
  friend class CTFramesetter;
  SR_MAKE_NONCOPYABLE(CTRun);
public:
  SR_DECL_CREATE_FUNC(CTRun);

  CTRun();
  virtual ~CTRun() = default;

  // Gets the glyph count for the run.
  SRIndex getGlyphCount() const { return _positions.size(); }

  // Returns the attribute dictionary that was used to create the glyph run.
  // The dictionary returned is either the same one that was set as an attribute dictionary 
  // on the original attributed string or a dictionary that has been manufactured by the layout engine.
  // Attribute dictionaries can be manufactured in the case of font substitution or if the run is missing critical attributes.
  // 글리프 런 생성에 사용된 속성 집합
  // (폰트 대체, 필수 속성 추가로 원본 속성 문자열의 것과 다를 수 있다)
  SRObjectDictionary& getAttributes() { return _attributes; }

  const std::vector<SRGlyph>& getGlyphs() const {
    return _glyphs;
  }
  const std::vector<SRPoint>& getPositions() const {
    return _positions;
  }
  const std::vector<SRSize>& getAdvances() const {
    return _advances;
  }
  const std::vector<SRIndex>& getStringIndices() const {
    return _stringIndices;
  }

  // Gets the range of characters that originally spawned the glyphs in the run.
  SRRange getStringRange() const { return _range; }

  // Gets the typographic bounds of the run.
  SRFloat getTypographicBounds(SRRange range, SRFloat* ascent, SRFloat* descent, SRFloat* leading) const;

  void draw(CGContextPtr ctx, SRRange range = { 0, 0 });

  const SRString& getStringFragment() const {
    return _stringFragment;
  }

  // skc added
  SRRect caretRect(SRTextPosition position) const;

private:
  // 글리프 런 생성시 사용됐던 속성 사전
  // font substitution 또는 필수 속성 추가등의 이유로 원본(attributed string의 속성사전)에서 수정될 수 있음
  SRObjectDictionary _attributes;

  // the range of characters that originally spawned the glyphs in the run.
  // 현재 글리프 런 내의 글리프들을 생성한 전체 문자열 내의 범위
  SRRange _range;

  // TODO WinObjC 의 경우만 존재하는 듯함(이걸 없애려면 _range 로 문자열을 획득할 방법이 필요하다)
  SRString _stringFragment; // 현재 런을 생성한 원본 문자열(개행문자등은 변환해서 _glyphs에 저장된다)

  // 글리프 각각의 글리프 인덱스(현재는 문자를 저장중(개행문자등은 변환해서 저장중))
  std::vector<SRGlyph> _glyphs;

  // 현재 라인 내에서 글리프 각각의 위치(CTTypesetter::doWrap()에서 y 는 0 으로 고정하고 있음)
  std::vector<SRPoint> _positions;

  // 글리프 각각의 크기
  std::vector<SRSize> _advances;

  // 글리프 각각의 전체 문자열 내 인덱스(글리프, 문자간 매핑에 사용된다)
  std::vector<SRIndex> _stringIndices;
};

} // namespace sr

using sr::CTRun;
