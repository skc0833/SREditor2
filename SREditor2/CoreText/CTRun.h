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
// ���� �Ӽ��� �ؽ�Ʈ ������ �����ϴ� ���ӵ� �۸��� ����
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
  // �۸��� �� ������ ���� �Ӽ� ����
  // (��Ʈ ��ü, �ʼ� �Ӽ� �߰��� ���� �Ӽ� ���ڿ��� �Ͱ� �ٸ� �� �ִ�)
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
  // �۸��� �� ������ ���ƴ� �Ӽ� ����
  // font substitution �Ǵ� �ʼ� �Ӽ� �߰����� ������ ����(attributed string�� �Ӽ�����)���� ������ �� ����
  SRObjectDictionary _attributes;

  // the range of characters that originally spawned the glyphs in the run.
  // ���� �۸��� �� ���� �۸������� ������ ��ü ���ڿ� ���� ����
  SRRange _range;

  // TODO WinObjC �� ��츸 �����ϴ� ����(�̰� ���ַ��� _range �� ���ڿ��� ȹ���� ����� �ʿ��ϴ�)
  SRString _stringFragment; // ���� ���� ������ ���� ���ڿ�(���๮�ڵ��� ��ȯ�ؼ� _glyphs�� ����ȴ�)

  // �۸��� ������ �۸��� �ε���(����� ���ڸ� ������(���๮�ڵ��� ��ȯ�ؼ� ������))
  std::vector<SRGlyph> _glyphs;

  // ���� ���� ������ �۸��� ������ ��ġ(CTTypesetter::doWrap()���� y �� 0 ���� �����ϰ� ����)
  std::vector<SRPoint> _positions;

  // �۸��� ������ ũ��
  std::vector<SRSize> _advances;

  // �۸��� ������ ��ü ���ڿ� �� �ε���(�۸���, ���ڰ� ���ο� ���ȴ�)
  std::vector<SRIndex> _stringIndices;
};

} // namespace sr

using sr::CTRun;
