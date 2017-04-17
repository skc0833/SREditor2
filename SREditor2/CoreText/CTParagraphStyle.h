#pragma once

#include "SRObject.h"
#include "SRTextBlock.h"
#include <vector>

namespace sr {

// These constants specify text alignment.
typedef uint8_t CTTextAlignment;

// These constants specify what happens when a line is too long for its frame.
typedef uint8_t CTLineBreakMode;

// These constants specify the writing direction.
typedef int8_t CTWritingDirection;

// Constants used to query and modify a paragraph style object.
typedef SRUInt32 CTParagraphStyleSpecifier;

enum {
  kCTLeftTextAlignment = 0,
  kCTRightTextAlignment = 1,
  kCTCenterTextAlignment = 2,
  kCTJustifiedTextAlignment = 3,
  kCTNaturalTextAlignment = 4
};
enum {
  kCTLineBreakByWordWrapping = 0,
  kCTLineBreakByCharWrapping = 1,
  kCTLineBreakByClipping = 2,
  kCTLineBreakByTruncatingHead = 3,
  kCTLineBreakByTruncatingTail = 4,
  kCTLineBreakByTruncatingMiddle = 5
};
enum { kCTWritingDirectionNatural = -1, kCTWritingDirectionLeftToRight = 0, kCTWritingDirectionRightToLeft = 1 };
enum {
  kCTParagraphStyleSpecifierAlignment = 0,
  kCTParagraphStyleSpecifierFirstLineHeadIndent = 1,
  kCTParagraphStyleSpecifierHeadIndent = 2,
  kCTParagraphStyleSpecifierTailIndent = 3,
  kCTParagraphStyleSpecifierTabStops = 4,
  kCTParagraphStyleSpecifierDefaultTabInterval = 5,
  kCTParagraphStyleSpecifierLineBreakMode = 6,
  kCTParagraphStyleSpecifierLineHeightMultiple = 7,
  kCTParagraphStyleSpecifierMaximumLineHeight = 8,
  kCTParagraphStyleSpecifierMinimumLineHeight = 9,
  kCTParagraphStyleSpecifierLineSpacing = 10,
  /* deprecated */ kCTParagraphStyleSpecifierParagraphSpacing = 11,
  kCTParagraphStyleSpecifierParagraphSpacingBefore = 12,
  kCTParagraphStyleSpecifierBaseWritingDirection = 13,
  kCTParagraphStyleSpecifierMaximumLineSpacing = 14,
  kCTParagraphStyleSpecifierMinimumLineSpacing = 15,
  kCTParagraphStyleSpecifierLineSpacingAdjustment = 16,
  kCTParagraphStyleSpecifierCount = 17
};

#if 1 // from CoreTextInternal.h
struct _CTParagraphStyleFloatProperty {
  bool _isDefault = true;
  SRFloat _value = 0.0f;
};

struct _CTParagraphStyleCTTextAlignmentProperty {
  bool _isDefault = true;
  CTWritingDirection _value = kCTNaturalTextAlignment;
};

struct _CTParagraphStyleCTLineBreakModeProperty {
  bool _isDefault = true;
  CTLineBreakMode _value = kCTLineBreakByWordWrapping;
};

struct _CTParagraphStyleCTWritingDirectionProperty {
  bool _isDefault = true;
  CTWritingDirection _value = kCTWritingDirectionNatural;
};

struct _CTParagraphStyleProperties {
  _CTParagraphStyleCTTextAlignmentProperty _alignment;
  _CTParagraphStyleCTLineBreakModeProperty _lineBreakMode;
  _CTParagraphStyleCTWritingDirectionProperty _writingDirection;

  _CTParagraphStyleFloatProperty _firstLineHeadIndent;
  _CTParagraphStyleFloatProperty _headIndent;
  _CTParagraphStyleFloatProperty _tailIndent;
  _CTParagraphStyleFloatProperty _tabInterval;
  _CTParagraphStyleFloatProperty _lineHeightMultiple;
  _CTParagraphStyleFloatProperty _maximumLineHeight;
  _CTParagraphStyleFloatProperty _minimumLineHeight;
  _CTParagraphStyleFloatProperty _lineSpacing;
  _CTParagraphStyleFloatProperty _paragraphSpacing;
  _CTParagraphStyleFloatProperty _paragraphSpacingBefore;
  _CTParagraphStyleFloatProperty _lineSpacingAdjustment;
  _CTParagraphStyleFloatProperty _maximumLineSpacing;
  _CTParagraphStyleFloatProperty _minimumLineSpacing;
};
#endif

// This structure is used to alter the paragraph style.
typedef struct CTParagraphStyleSetting {
  // The specifier of the setting. See CTParagraphStyleSpecifier for possible values.
  CTParagraphStyleSpecifier spec;
  // The size of the value pointed to by the value field. 
  // This value must match the size of the value required by the CTParagraphStyleSpecifier set in the spec field.
  size_t valueSize;
  // A reference to the value of the setting specified by the spec field. 
  // The value must be in the proper range for the spec value and at least as large as the size specified in valueSize.
  const void* value;
} CTParagraphStyleSetting;

class CTParagraphStyle;
using CTParagraphStylePtr = std::shared_ptr<CTParagraphStyle>;
using SRTextBlockList = std::vector<SRTextBlockPtr>;

/*
https://developer.apple.com/reference/coretext/ctparagraphstyle-2ej
The CTParagraphStyle opaque type represents paragraph or ruler attributes in an attributed string.
*/
class CTParagraphStyle : public SRObject {
  SR_MAKE_NONCOPYABLE(CTParagraphStyle);
public:
  SR_DECL_CREATE_FUNC(CTParagraphStyle);

  // Creates an immutable paragraph style.
  CTParagraphStyle(const CTParagraphStyleSetting* settings, size_t settingsCount); // @@todo stl set 등으로 바꾸자!!!
  virtual ~CTParagraphStyle() = default;

  //CTParagraphStyle(const CTParagraphStyle& other);
  //virtual CTParagraphStyle& operator=(const CTParagraphStyle& rhs);
  CTParagraphStylePtr copy();

  // Obtains the current value for a single setting specifier.
  bool getValueForSpecifier(CTParagraphStyleSpecifier spec, size_t valueBufferSize, void* valueBuffer);
  void setValueForSpecifier(CTParagraphStyleSpecifier spec, size_t valueBufferSize, void* valueBuffer);

  // skc added
  virtual SRStringUPtr toString() const;

  void addTextBlock(const SRTextBlockPtr block);

  // You add text blocks to a paragraph style object
  void addTextBlockList(const SRTextBlockList& blocks);
  const SRTextBlockList& getTextBlockList() const { return _textBlocks; }

  static CTParagraphStylePtr defaultParagraphStyle();

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  _CTParagraphStyleProperties _properties;

  // The text blocks containing the paragraph, nested from outermost to innermost to array.
  SRTextBlockList _textBlocks;
};

} // namespace sr
