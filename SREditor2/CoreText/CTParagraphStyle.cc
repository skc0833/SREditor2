#include "CTParagraphStyle.h"
#include <sstream> // for wostringstream

static const size_t MAX_SPECIFIER_COUNT = 16;

namespace sr {

// PropertyInfo is the struct containing the expected size of data and an offset to the struct which has the actual data.
struct PropertyInfo {
  size_t expectedSize;
  size_t offset;
};

// this table contains the expected size of the data for any specifier, and its offset in the structure.
static PropertyInfo propertyInfoTable[MAX_SPECIFIER_COUNT];
static void init_propertyInfoTable() {
  static bool __initialized = false;
  if (__initialized) return;
  __initialized = true;

  propertyInfoTable[kCTParagraphStyleSpecifierAlignment] = {
    sizeof(CTTextAlignment), offsetof(_CTParagraphStyleProperties, _alignment)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierFirstLineHeadIndent] = {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _firstLineHeadIndent)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierHeadIndent] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _headIndent)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierTailIndent] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _tailIndent)
  };
  // 현재 kCTParagraphStyleSpecifierTabStops = 4 이 빠져있음
  propertyInfoTable[kCTParagraphStyleSpecifierDefaultTabInterval] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _tabInterval)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierLineBreakMode] =
  {
    sizeof(CTLineBreakMode), offsetof(_CTParagraphStyleProperties, _lineBreakMode)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierLineHeightMultiple] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _lineHeightMultiple)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierMaximumLineHeight] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _maximumLineHeight)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierMinimumLineHeight] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _minimumLineHeight)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierLineSpacing] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _lineSpacing)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierParagraphSpacing] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _paragraphSpacing)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierParagraphSpacingBefore] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _paragraphSpacingBefore)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierBaseWritingDirection] =
  {
    sizeof(CTWritingDirection), offsetof(_CTParagraphStyleProperties, _writingDirection)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierMaximumLineSpacing] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _maximumLineSpacing)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierMinimumLineSpacing] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _minimumLineSpacing)
  };
  propertyInfoTable[kCTParagraphStyleSpecifierLineSpacingAdjustment] =
  {
    sizeof(SRFloat), offsetof(_CTParagraphStyleProperties, _lineSpacingAdjustment)
  };
}

//static
CTParagraphStylePtr CTParagraphStyle::defaultParagraphStyle() {
  CTTextAlignment alignment = kCTLeftTextAlignment; //kCTRightTextAlignment; //kCTJustifiedTextAlignment;
  SRFloat firstLineHeadIndent = 10.0f;
  CTParagraphStyleSetting _settings[] = {
    { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment },
    { kCTParagraphStyleSpecifierFirstLineHeadIndent, sizeof(firstLineHeadIndent), &firstLineHeadIndent },
  };
  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::create(_settings, sizeof(_settings) / sizeof(_settings[0]));
  return paragraphStyle;
}

/**
@Status Caveat
@Notes CTParagraphStyleSpecifier kCTParagraphStyleSpecifierTabStops and kCTParagraphStyleSpecifierCount not supported
*/
CTParagraphStyle::CTParagraphStyle(const CTParagraphStyleSetting* settings, size_t settingsCount) {
  init_propertyInfoTable();

  if (settings) {
    _CTParagraphStyleProperties* paraStyleProperty = &_properties;
    for (size_t i = 0; i < settingsCount; ++i) {
      // validate input specifier and the buffer
      if (settings[i].spec <= MAX_SPECIFIER_COUNT && settings[i].value != nullptr) {
        size_t expectedSize = propertyInfoTable[settings[i].spec].expectedSize;

        SR_ASSERT(settings[i].valueSize == expectedSize);
        if (settings[i].valueSize == expectedSize) {
          // structContainingData is the struct that contains the actual data and a bool varibale _isDefault to store if data has
          // a default value or it has been set by the user.
          // offsetToDefaultStatus is the offset to this struct, which is also in turn the offset to _isDefault bool variable
          // within the struct.
          size_t offsetToDefaultStatus = propertyInfoTable[settings[i].spec].offset;
          auto structContainingData =
            reinterpret_cast<void*>((reinterpret_cast<char*>(paraStyleProperty) + offsetToDefaultStatus));
          *(reinterpret_cast<bool*>(structContainingData)) = false;

          if (expectedSize == sizeof(SRFloat)) {
            auto property = reinterpret_cast<_CTParagraphStyleFloatProperty*>(structContainingData);

            size_t offsetForValue = offsetof(_CTParagraphStyleFloatProperty, _value);
            auto locationToWrite = reinterpret_cast<char*>(property) + offsetForValue;
            *reinterpret_cast<SRFloat*>(locationToWrite) = *reinterpret_cast<const SRFloat*>(settings[i].value);

          }
          else if (settings[i].spec == kCTParagraphStyleSpecifierBaseWritingDirection) {
            auto property = reinterpret_cast<_CTParagraphStyleCTWritingDirectionProperty*>(structContainingData);

            size_t offsetForValue = offsetof(_CTParagraphStyleCTWritingDirectionProperty, _value);
            auto locationToWrite = reinterpret_cast<char*>(property) + offsetForValue;
            *reinterpret_cast<CTWritingDirection*>(locationToWrite) =
              *reinterpret_cast<const CTWritingDirection*>(settings[i].value);
          }
          else if (settings[i].spec == kCTParagraphStyleSpecifierAlignment) {
            auto property = reinterpret_cast<_CTParagraphStyleCTTextAlignmentProperty*>(structContainingData);

            size_t offsetForValue = offsetof(_CTParagraphStyleCTTextAlignmentProperty, _value);
            auto locationToWrite = reinterpret_cast<char*>(property) + offsetForValue;
            *reinterpret_cast<CTTextAlignment*>(locationToWrite) = *reinterpret_cast<const CTTextAlignment*>(settings[i].value);

          }
          else if (settings[i].spec == kCTParagraphStyleSpecifierLineBreakMode) {
            auto property = reinterpret_cast<_CTParagraphStyleCTLineBreakModeProperty*>(structContainingData);

            size_t offsetForValue = offsetof(_CTParagraphStyleCTLineBreakModeProperty, _value);
            auto locationToWrite = reinterpret_cast<char*>(property) + offsetForValue;
            *reinterpret_cast<CTLineBreakMode*>(locationToWrite) = *reinterpret_cast<const CTLineBreakMode*>(settings[i].value);
          }
        }
      }
    }
  }
}

CTParagraphStylePtr CTParagraphStyle::copy() {
  CTParagraphStylePtr obj = CTParagraphStyle::create(nullptr, 0);
  memcpy(&obj->_properties, &(_properties), sizeof(_properties));
  obj->_textBlocks = _textBlocks;
  return obj;
}

//CTParagraphStyle::CTParagraphStyle(const CTParagraphStyle& other) {
//  memcpy(&_properties, &(other._properties), sizeof(other._properties));
//  _textBlocks = other._textBlocks;
//}
//
//CTParagraphStyle& CTParagraphStyle::operator=(const CTParagraphStyle& other) {
//  memcpy(&_properties, &(other._properties), sizeof(other._properties));
//  _textBlocks = other._textBlocks;
//  return *this;
//}

bool CTParagraphStyle::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const CTParagraphStyle&>(obj);
  if (memcmp(&_properties, &(other._properties), sizeof(_properties))) {
    // 문단 속성이 다른 경우, return false;
    return false;
  }
  // 텍스트 블럭 속성 비교
  if (_textBlocks.size() != other._textBlocks.size()) {
    return false;
  }
  for (auto i = _textBlocks.begin(); i < _textBlocks.end(); ++i) {
    const SRTextBlock& lhs = **i;
    const SRTextBlock& rhs = *(other._textBlocks.at(std::distance(_textBlocks.begin(), i)));
    if (lhs != rhs) {
      return false;
    }
  }
  // _textBlocks 까지 동일하므로 동일한 것으로 처리함
  return true;
}

/* e.g,
    NSBackgroundColor = "UIDeviceWhiteColorSpace 0 1";
    NSFont = "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt";
*/
SRStringUPtr CTParagraphStyle::toString() const {
  std::wostringstream ss;
  ss << L"<CTParagraphStyle: " << this << L">";
  ss << L" alignment: " << _properties._alignment._value << L";";
  ss << L" firstLineHeadIndent: " << _properties._firstLineHeadIndent._value << L";";
  ss << L" headIndent: " << _properties._headIndent._value << L";";
  // ...
  return std::make_unique<SRString>(ss.str());
}

void CTParagraphStyle::addTextBlock(const SRTextBlockPtr block) {
  _textBlocks.push_back(block);
}

void CTParagraphStyle::addTextBlockList(const SRTextBlockList& blocks) {
  if (!blocks.empty()) {
    for (auto i = blocks.begin(); i != blocks.end(); ++i) {
      _textBlocks.push_back(*i);
    }
  }
}

/**
@Status Caveat
@Notes CTParagraphStyleSpecifier kCTParagraphStyleSpecifierTabStops and kCTParagraphStyleSpecifierCount not supported
*/
bool CTParagraphStyle::getValueForSpecifier(CTParagraphStyleSpecifier spec, size_t valueBufferSize, void* valueBuffer) {
  if (!valueBuffer) {
    return false;
  }

  bool isDefaultValue = true;

  if (spec <= MAX_SPECIFIER_COUNT) {
    _CTParagraphStyleProperties* paraStyleProperty = &_properties;

    // structContainingData is the struct that contains the actual data and a bool varibale _isDefault to store if data has a default
    // value or it has been set by the user.
    // offsetToDefaultStatus is the offset to this struct, which is also in turn the offset to _isDefault bool variable within the
    // struct.
    size_t offsetToDefaultStatus = propertyInfoTable[spec].offset;
    auto structContainingData = reinterpret_cast<void*>(
      (reinterpret_cast<char*>(paraStyleProperty) + offsetToDefaultStatus)
      );
    isDefaultValue = *(reinterpret_cast<bool*>(structContainingData));

    size_t expectedSize = propertyInfoTable[spec].expectedSize;
    if (expectedSize == sizeof(SRFloat)) {
      auto property = reinterpret_cast<_CTParagraphStyleFloatProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleFloatProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<SRFloat*>(valueBuffer) = *reinterpret_cast<SRFloat*>(locationTocopy);
    }
    else if (spec == kCTParagraphStyleSpecifierBaseWritingDirection) {
      auto property = reinterpret_cast<_CTParagraphStyleCTWritingDirectionProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTWritingDirectionProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTWritingDirection*>(valueBuffer) = *reinterpret_cast<CTWritingDirection*>(locationTocopy);
    }
    else if (spec == kCTParagraphStyleSpecifierAlignment) {
      auto property = reinterpret_cast<_CTParagraphStyleCTTextAlignmentProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTTextAlignmentProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTTextAlignment*>(valueBuffer) = *reinterpret_cast<CTTextAlignment*>(locationTocopy);
    }
    else if (spec == kCTParagraphStyleSpecifierLineBreakMode) {
      auto property = reinterpret_cast<_CTParagraphStyleCTLineBreakModeProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTLineBreakModeProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTLineBreakMode*>(valueBuffer) = *reinterpret_cast<CTLineBreakMode*>(locationTocopy);
    }
  }
  return !isDefaultValue;
}

void CTParagraphStyle::setValueForSpecifier(CTParagraphStyleSpecifier spec, size_t valueBufferSize, void* valueBuffer) {
  if (!valueBuffer) {
    SR_ASSERT(0);
    return;
  }

  //bool isDefaultValue = true;

  if (spec <= MAX_SPECIFIER_COUNT) {
    _CTParagraphStyleProperties* paraStyleProperty = &_properties;

    // structContainingData is the struct that contains the actual data and a bool varibale _isDefault to store if data has a default
    // value or it has been set by the user.
    // offsetToDefaultStatus is the offset to this struct, which is also in turn the offset to _isDefault bool variable within the
    // struct.
    size_t offsetToDefaultStatus = propertyInfoTable[spec].offset;
    auto structContainingData = reinterpret_cast<void*>(
      (reinterpret_cast<char*>(paraStyleProperty) + offsetToDefaultStatus)
      );
    //isDefaultValue = *(reinterpret_cast<bool*>(structContainingData));

    size_t expectedSize = propertyInfoTable[spec].expectedSize;
    if (expectedSize == sizeof(SRFloat)) {
      auto property = reinterpret_cast<_CTParagraphStyleFloatProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleFloatProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<SRFloat*>(locationTocopy) = *reinterpret_cast<SRFloat*>(valueBuffer);
    } else if (spec == kCTParagraphStyleSpecifierBaseWritingDirection) {
      auto property = reinterpret_cast<_CTParagraphStyleCTWritingDirectionProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTWritingDirectionProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTWritingDirection*>(locationTocopy) = *reinterpret_cast<CTWritingDirection*>(valueBuffer);
    } else if (spec == kCTParagraphStyleSpecifierAlignment) {
      auto property = reinterpret_cast<_CTParagraphStyleCTTextAlignmentProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTTextAlignmentProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTTextAlignment*>(locationTocopy) = *reinterpret_cast<CTTextAlignment*>(valueBuffer);
    } else if (spec == kCTParagraphStyleSpecifierLineBreakMode) {
      auto property = reinterpret_cast<_CTParagraphStyleCTLineBreakModeProperty*>(structContainingData);

      size_t offsetForValue = offsetof(_CTParagraphStyleCTLineBreakModeProperty, _value);
      auto locationTocopy = reinterpret_cast<char*>(property) + offsetForValue;
      *reinterpret_cast<CTLineBreakMode*>(locationTocopy) = *reinterpret_cast<CTLineBreakMode*>(valueBuffer);
    }
  }
}

} // namespace sr
