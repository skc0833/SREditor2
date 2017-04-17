#include "SRAttributedString.h"
#include <string>
#include <sstream> // for wostringstream
#include "../UIKit/UIFont.h" // for test
#include "../CoreText/CTParagraphStyle.h"

namespace sr {

const wchar_t* kCTForegroundColorAttributeName = L"SRForegroundColor";
const wchar_t* kCTBackgroundColorAttributeName = L"SRBackgroundColorAttributeName";
const wchar_t* kCTFontAttributeName = L"SRFontAttributeName";
const wchar_t* kCTParagraphStyleAttributeName = L"SRParagraphStyleAttributeName";

//const CFStringRef kCTCharacterShapeAttributeName = static_cast<CFStringRef>(@"kCTCharacterShapeAttributeName");
//const CFStringRef kCTFontAttributeName = static_cast<CFStringRef>(@"NSFont");
//const CFStringRef kCTKernAttributeName = static_cast<CFStringRef>(@"kCTKernAttributeName");
//const CFStringRef kCTLigatureAttributeName = static_cast<CFStringRef>(@"kCTLigatureAttributeName");
//const CFStringRef kCTForegroundColorAttributeName = static_cast<CFStringRef>(@"NSForegroundColor");
//const CFStringRef kCTForegroundColorFromContextAttributeName = static_cast<CFStringRef>(@"kCTForegroundColorFromContextAttributeName");
//const CFStringRef kCTParagraphStyleAttributeName = static_cast<CFStringRef>(@"kCTParagraphStyleAttributeName");
//const CFStringRef kCTStrokeWidthAttributeName = static_cast<CFStringRef>(@"kCTStrokeWidthAttributeName");
//const CFStringRef kCTStrokeColorAttributeName = static_cast<CFStringRef>(@"kCTStrokeColorAttributeName");
//const CFStringRef kCTSuperscriptAttributeName = static_cast<CFStringRef>(@"kCTSuperscriptAttributeName");
//const CFStringRef kCTUnderlineColorAttributeName = static_cast<CFStringRef>(@"kCTUnderlineColorAttributeName");
//const CFStringRef kCTUnderlineStyleAttributeName = static_cast<CFStringRef>(@"kCTUnderlineStyleAttributeName");
//const CFStringRef kCTVerticalFormsAttributeName = static_cast<CFStringRef>(@"kCTVerticalFormsAttributeName");
//const CFStringRef kCTGlyphInfoAttributeName = static_cast<CFStringRef>(@"kCTGlyphInfoAttributeName");
const wchar_t* kCTRunDelegateAttributeName = L"kCTRunDelegateAttributeName";

#define __AssertIndexIsInBounds(idx) SR_LOG_ASSERT((idx) >= 0 && (idx) < getLength() \
    , L"%s(): index %d out of bounds (length %d)", __WFUNCTION__, idx, getLength())

#define __AssertRangeIsInBounds(range) SR_LOG_ASSERT(isRangeInBounds(range) \
    , L"%s(): range %d,%d out of bounds (length %d)", __WFUNCTION__, range.location, range.length, getLength())
#define __checkRangeIsInBounds(range, result) \
  __AssertRangeIsInBounds(range); \
  if (!isRangeInBounds(range)) return result;
#define __checkRangeIsInBoundsVoid(range) \
  __AssertRangeIsInBounds(range); \
  if (!isRangeInBounds(range)) return;
  
SRAttributedString::SRAttributedString(const SRString& str, const SRObjectDictionaryPtr attributes) {
  _string = SRString::create(str);
  _attributeArray = SRRunArray::create();
  if (attributes == nullptr) {
    // 속성사전은 항상 문자열 길이만큼 존재해야 한다.
    if (_string->length() > 0) {
      auto dict = SRObjectDictionary::create();
      _attributeArray->insert(SRRange(0, _string->length()), dict);
    }
  } else {
    _attributeArray->insert(SRRange(0, _string->length()), attributes);
  }
}

SRAttributedString::~SRAttributedString() {
}

/* e.g, SRAttributedString
Bo{
    NSBackgroundColor = "UIDeviceWhiteColorSpace 0 1";
    NSFont = "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt";
}Bi{
    NSBackgroundColor = "UIDeviceWhiteColorSpace 0 1";
    NSFont = "<UICTFont: 0x7fd3e2f83310> font-family: \".HelveticaNeueInterface-Italic\"; font-weight: normal; font-style: italic; font-size: 12.00pt";
}Bu{
}
*/
SRStringUPtr SRAttributedString::toString() const {
  std::wostringstream ss;
  std::wstring ws = _string->c_str();

  SRIndex cnt = _attributeArray->getCount();
  SRIndex idx;
  SRRange effectiveRange;
  ss << std::endl;
  for (idx = 0; idx < cnt; ) {
    SRObjectPtr obj = _attributeArray->getValueAtIndex(idx, &effectiveRange);
    SR_ASSERT(idx == effectiveRange.location);
    ss << ws.substr(idx, effectiveRange.length) << L"{" << std::endl;
    ss << obj->toString()->c_str();
    ss << L"}";
    idx += effectiveRange.length;
  }
  SR_ASSERT(idx == cnt);
  return std::make_unique<SRString>(ss.str());
}

SRObjectDictionaryPtr SRAttributedString::attributes(SRUInt loc, SRRange* effectiveRange) const {
  SRObjectPtr attrs = _attributeArray->getValueAtIndex(loc, effectiveRange, NULL);
  SRObjectDictionaryPtr ptr = std::dynamic_pointer_cast<SRObjectDictionary>(attrs);
  SR_ASSERT(ptr);
  return ptr;
}

SRObjectDictionaryPtr SRAttributedString::attributes(SRUInt loc, SRRange* longestEffectiveRange, SRRange rangeLimit) const {
  SR_NOT_IMPL();
  __AssertIndexIsInBounds(loc);
  __checkRangeIsInBounds(rangeLimit, nullptr);

  // 먼저 loc 위치의 속성집합을 획득한다.
  SRRange effectiveRange;
  SRObjectPtr attrs = attributes(loc, longestEffectiveRange);

  // 다음번 블럭이 동일한 속성집합이면 rangeLimit 범위내에서 계속 계산한다.
  loc += longestEffectiveRange->length;
  SRUInt limit = rangeLimit.location + rangeLimit.length; // 검사할 마지막 위치(이 위치 바로 이전까지만 검사해야 함)
  SRObjectPtr nextAttrs;
  while (loc < limit) {
    nextAttrs = attributes(loc, &effectiveRange);
    if (*attrs != *nextAttrs) break;
    longestEffectiveRange->length += effectiveRange.length;
    loc += effectiveRange.length;
  }

  // loc 위치의 속성집합을 리턴한다.
  SRObjectDictionaryPtr ptr = std::dynamic_pointer_cast<SRObjectDictionary>(attrs);
  SR_ASSERT(ptr);
  return ptr;
}

SRObjectPtr SRAttributedString::attribute(const SRString& attrName, SRUInt loc, SRRange* effectiveRange) const {
  __AssertIndexIsInBounds(loc);
  SRObjectPtr attrs = _attributeArray->getValueAtIndex(loc, effectiveRange);
  SRObjectDictionaryPtr ptr = std::dynamic_pointer_cast<SRObjectDictionary>(attrs);
  SR_ASSERT(ptr);
  SRObjectPtr value = ptr->valueForKey(attrName, nullptr);
  return value;
}

SRObjectPtr SRAttributedString::attribute(const SRString& attrName, SRUInt loc, SRRange* longestEffectiveRange, SRRange rangeLimit) const {
  __AssertIndexIsInBounds(loc);
  __checkRangeIsInBounds(rangeLimit, nullptr);

  // 먼저 loc 위치의 속성값을 획득한다.
  SRObjectPtr value = attribute(attrName, loc, longestEffectiveRange);
  SR_ASSERT(value);

  // 다음번 블럭이 동일한 속성값이면 rangeLimit 범위내에서 계속 계산한다.
  loc += longestEffectiveRange->length;
  SRUInt limit = rangeLimit.location + rangeLimit.length; // 검사할 마지막 위치(이 위치 바로 이전까지만 검사해야 함)

  SRRange effectiveRange;
  SRObjectPtr nextValue;
  while (loc < limit) {
    nextValue = attribute(attrName, loc, &effectiveRange);
    if (*value != *nextValue) break;
    longestEffectiveRange->length += effectiveRange.length;
    loc += effectiveRange.length;
  }

  // loc 위치의 속성값을 리턴한다.
  return value;
}

// 지정한 범위에 기 존재하는 속성을 사용하여 추가한다.
// replaceCharacters() 의 경우에는 무조건 전달받은 속성으로 추가함
void SRAttributedString::replaceCharacters(SRRange range, const SRString& replacement) {
  __checkRangeIsInBoundsVoid(range);

  SRIndex replacementLen = replacement.length();
  SRObjectDictionaryPtr attributesToBeUsed = nullptr;
  if (replacementLen > 0) {
    // By default extend replaced attributes, or take them from the previous character
    if (range.length > 0) {
      // 범위 변경: 대상 범위가 0 이상인 경우, 대상 범위 시작 문자의 속성을 획득
      attributesToBeUsed = attributes(range.location);
    }
    else if (range.location > 0) {
      // 특정위치에 삽입: 대상 범위가 0 이고, 삽입할 위치가 0 이상인 경우, 삽입할 위치 바로 이전 문자의 속성을 획득
      attributesToBeUsed = attributes(range.location - 1);
    }
    else if (getLength() > 0) {
      // 대상 범위, 삽입할 위치가 둘다 0 이고, 현재 문자열 길이가 0 이상인 경우, 첫 문자의 속성을 획득
      attributesToBeUsed = attributes(0);
    }
    else {
      attributesToBeUsed = SRObjectDictionary::create();
    }
  }
  if (range.length > 0) {
    _attributeArray->replace(range, attributesToBeUsed, replacementLen);
  }
  else if (replacementLen) {
    _attributeArray->insert(SRRange(range.location, replacementLen), attributesToBeUsed);
  }
  _string->replaceCharactersInRange(range, replacement);
}


void SRAttributedString::replaceCharacters(SRRange range, const SRAttributedStringPtr replacement) {
  //__checkRangeIsInBoundsVoid(range); // 끝에 붙이는 경우도 있다.

  SRStringPtr otherStr = replacement->getString();
  SRIndex otherStrLen = otherStr->length();

  if (otherStrLen > 0) {
    SRRange attrRange111 = { 0, 0 }; //error C2552: 'attrRange' : 이니셜라이저 목록을 사용하여 비집합체를 초기화할 수 없습니다.
    SRRange attrRange(0, 0);
    while (attrRange.location < otherStrLen) {
      // 먼저 전달받은 속성들을 변경 시작 위치에 추가
      SRObjectDictionaryPtr otherAttrs = replacement->attributes(attrRange.location, &attrRange);
      SRObjectDictionaryPtr attrs = createAttributesDictionary(otherAttrs);
      // 원본 문자열의 range.location 부터 변경할 문자열(replacement)의 처음 속성부터 마지막 속성까지 추가한다.
      _attributeArray->insert(SRRange(attrRange.location + range.location, attrRange.length), attrs);
      attrRange.location += attrRange.length;
    }
    SR_ASSERT(attrRange.location == otherStrLen);
  }
  // 전달받은 range 영역을 삭제(삽입된 속성들 다음 위치부터)
  if (range.length > 0) {
    _attributeArray->remove(SRRange(range.location + otherStrLen, range.length));
  }

  // 전달받은 replacement 문자열을 삽입(삽입된 속성들 시작 위치에)
  _string->replaceCharactersInRange(range, *otherStr);
}


// deletes the characters in the given range along with their associated attributes.
void SRAttributedString::deleteCharacters(SRRange range) {
  SRAttributedStringPtr attrStr = SRAttributedString::create();
  replaceCharacters(range, attrStr);
}

void SRAttributedString::append(const SRAttributedStringPtr attrStr) {
  insert(attrStr, getLength());
}

void SRAttributedString::insert(const SRAttributedStringPtr attrStr, SRUInt loc) {
  replaceCharacters(SRRange(loc, 0), attrStr);
}

void SRAttributedString::setAttributedString(const SRAttributedStringPtr attrStr) {
  replaceCharacters(SRRange(0, this->getLength()), attrStr);
}

void SRAttributedString::fixAttributes(SRRange range) {
  __checkRangeIsInBoundsVoid(range);
  // TODO
  //SR_ASSERT(0);
  // 폰트등이 없을 경우, 디폴트 폰트로 채우는 등등 속성을 보정한다.
  //[self fixFontAttributeInRange : range]; gunstep
  //[self fixParagraphStyleAttributeInRange : range];
  //[self fixAttachmentAttributeInRange : range];
}

SRObjectDictionaryPtr SRAttributedString::createAttributesDictionary(const SRObjectDictionaryPtr attrs) const {
  SRObjectDictionaryPtr newAttrs;
  if (attrs) {
    //TODO have to deep copy???
    newAttrs = SRObjectDictionary::create(*attrs);
  } else {
    newAttrs = SRObjectDictionary::create();
  }
  return newAttrs;
}

//SRObjectDictionaryPtr SRAttributedString::getAttributes(SRIndex loc, SRRange* effectiveRange) const {
//  SRObjectPtr attrs = _attributeArray->getValueAtIndex(loc, effectiveRange, NULL);
//  SRObjectDictionaryPtr ptr = std::dynamic_pointer_cast<SRObjectDictionary>(attrs);
//  SR_ASSERT(ptr);
//  return ptr;
//}

//SRObjectPtr SRAttributedString::getAttribute(SRIndex loc, const SRString& attrName, SRRange* effectiveRange) const {
//  __AssertIndexIsInBounds(loc);
//  SRObjectPtr attrs = _attributeArray->getValueAtIndex(loc, effectiveRange);
//  SRObjectDictionaryPtr ptr = std::dynamic_pointer_cast<SRObjectDictionary>(attrs);
//  SR_ASSERT(ptr);
//  SRObjectPtr value = ptr->valueForKey(attrName, nullptr);
//  return value;
//}

void SRAttributedString::setAttributes(const SRObjectDictionaryPtr replacementAttrs, SRRange range, bool clearOtherAttributes) {
  __checkRangeIsInBoundsVoid(range);

  if (clearOtherAttributes) { // Just blast all attribute dictionaries in the specified range
    if (range.length) {
      // 지정한 범위의 속성을 교체한다(전달받은 속성을 복사해서 교체해야한다)
      SRObjectDictionaryPtr attrs = createAttributesDictionary(replacementAttrs);
      _attributeArray->replace(range, attrs, range.length);
      //_attributeArray->replace(range, replacementAttrs, range.length);
      // !!! [self edited:CFAttributedStringEditedAttributes range:range changeInLength:0];
    }
  } else { // More difficult --- set specified keys and values on the existing dictionaries in the specified range
    SRIndex numAdditionalItems = replacementAttrs->count();
    if (numAdditionalItems) {
      // Extract the new keys and values so we don't do it over and over for each range
      //createLocalArray(additionalKeys, numAdditionalItems);
      //createLocalArray(additionalValues, numAdditionalItems);
      //CFDictionaryGetKeysAndValues(replacementAttrs, additionalKeys, additionalValues);

      // CFAttributedStringBeginEditing(attrStr);
      while (range.length) {
        // range 범위의 속성에 replacementAttrs 속성을 합친다. 기 존재하는 속성은 교체. map.insert() 함수로 가능?
        SRRange effectiveRange;
        SRObjectPtr obj = _attributeArray->getValueAtIndex(range.location, &effectiveRange, NULL);
        SRObjectDictionaryPtr attrs = std::dynamic_pointer_cast<SRObjectDictionary>(obj);
        SR_ASSERT(attrs);

        // Intersect effectiveRange and range
        if (effectiveRange.location < range.location) {
            effectiveRange.length -= (range.location - effectiveRange.location);
            effectiveRange.location = range.location;
        }
        if (effectiveRange.length > range.length) effectiveRange.length = range.length;
        // We need to make a new copy
        attrs = createAttributesDictionary(attrs);
        //__CFDictionaryAddMultiple(attrs, additionalKeys, additionalValues, numAdditionalItems);
        attrs->addMulitiple(replacementAttrs);
        _attributeArray->replace(effectiveRange, attrs, effectiveRange.length);
        //CFRelease(attrs);
        range.length -= effectiveRange.length;
        range.location += effectiveRange.length;
        SR_ASSERT(range.length >= 0);
      }
      // CFAttributedStringEndEditing(attrStr);

      //freeLocalArray(additionalKeys);
      //freeLocalArray(additionalValues);
    }
  }
}

void SRAttributedString::setAttribute(const SRString& attrName, const SRObjectPtr value, SRRange range) {
  __checkRangeIsInBoundsVoid(range);

  // CFAttributedStringBeginEditing(attrStr);
  while (range.length) {
    SRRange effectiveRange;
    // range.location 위치의 유효한 effectiveRange를 구한다(effectiveRange.location 는 range.location 이하값)
    SRObjectPtr orgValue = _attributeArray->getValueAtIndex(range.location, &effectiveRange, NULL);
    SRObjectDictionaryPtr attrs = std::dynamic_pointer_cast<SRObjectDictionary>(orgValue);
    //SR_ASSERT(attrs); // empty 일수도 있음!

    // effectiveRange 를 range 영역내로 설정(Intersect effectiveRange and range)
    if (effectiveRange.location < range.location) {
      effectiveRange.length -= (range.location - effectiveRange.location);
      effectiveRange.location = range.location;
    }
    if (effectiveRange.length > range.length) {
      effectiveRange.length = range.length;
    }
    // First check to see if the same value already exists; this will avoid a copy
    SRObjectPtr existingValue = (attrs) ? attrs->valueForKey(attrName, nullptr) : nullptr;
    if (!existingValue || (*existingValue != *value)) { // SRObjectPtr가 아닌 실제 객체가 같은지 비교한다.
      // 현재 위치(range.location)의 속성값이 전달받은 value 와 같지 않으면 기존 SRObjectDictionary를(attrs) 복사하여 추가한다.
      // We need to make a new copy
      attrs = createAttributesDictionary(attrs);
      attrs->setValue(value, attrName); // 해당키가 존재하지 않으면 추가하고, 아니면 수정한다.
      _attributeArray->replace(effectiveRange, attrs, effectiveRange.length);
    } else {
      // effectiveRange 의 속성이 변경하려는 속성(value)과 동일한 경우, 아무처리도 안함
      //SR_ASSERT(0);
      int a = 0;
    }
    range.length -= effectiveRange.length;
    range.location += effectiveRange.length;
    SR_ASSERT(range.length >= 0);
  }
  // CFAttributedStringEndEditing(attrStr);
}

void SRAttributedString::removeAttribute(const SRString& attrName, SRRange range) {
  __checkRangeIsInBoundsVoid(range);

  // CFAttributedStringBeginEditing(attrStr);
  while (range.length) {
    SRRange effectiveRange;
    SRObjectPtr obj = _attributeArray->getValueAtIndex(range.location, &effectiveRange, NULL);
    SRObjectDictionaryPtr attrs = std::dynamic_pointer_cast<SRObjectDictionary>(obj);
    SR_ASSERT(attrs);
    // Intersect effectiveRange and range
    if (effectiveRange.location < range.location) {
        effectiveRange.length -= (range.location - effectiveRange.location);
        effectiveRange.location = range.location;
    }
    if (effectiveRange.length > range.length) effectiveRange.length = range.length;
    // First check to see if the value is not there; this will avoid a copy
    if (attrs->containsKey(attrName)) {
        // We need to make a new copy
        attrs = createAttributesDictionary(attrs);
        attrs->removeValueForKey(attrName);
        _attributeArray->replace(effectiveRange, attrs, effectiveRange.length);
    }
    range.length -= effectiveRange.length;
    range.location += effectiveRange.length;
  }
}

//// deletes the characters in the given range along with their associated attributes.
//void SRAttributedString::deleteString(SRRange range) {
//  SRAttributedStringPtr attrStr = SRAttributedString::create();
//  replaceCharacters(range, attrStr);
//}

// location 위치 기준으로 좌,우로 동일 block 을 소유하는 최대 범위를 리턴함
SRRange SRAttributedString::rangeOfBlockType(SRTextBlockPtr block, SRUInt location, cbContainsBlockType containsBlockType) const {
  SRRange effRange;
  CTParagraphStylePtr style = std::dynamic_pointer_cast<CTParagraphStyle>(
    attribute(kCTParagraphStyleAttributeName, location, &effRange));
  SR_ASSERT(style);

  if (style) {
    const SRTextBlockList& textBlocks = style->getTextBlockList();

    if (containsBlockType(textBlocks, block)) {
      SRRange newEffRange;

      while ((effRange.location > 0)) {
        // 좌측(effRange.location - 1)으로 동일한 블럭 검색
        style = std::dynamic_pointer_cast<CTParagraphStyle>(
          attribute(kCTParagraphStyleAttributeName, effRange.location - 1, &newEffRange));

        if (style) {
          if (containsBlockType(style->getTextBlockList(), block)) {
            effRange.location = newEffRange.location;
            effRange.length += newEffRange.length;
            continue;
          }
        }
        break;
      }

      SRUInt len = getLength();
      while (effRange.rangeMax() < len) {
        // 우측(effRange.rangeMax())으로 동일한 블럭 검색
        style = std::dynamic_pointer_cast<CTParagraphStyle>(
          attribute(kCTParagraphStyleAttributeName, effRange.rangeMax(), &newEffRange));
        if (style) {
          if (containsBlockType(style->getTextBlockList(), block)) {
            effRange.length += newEffRange.length;
            continue;
          }
        }
        break;
      }
      return effRange;
    }
  }
  return SRRange(SRNotFound, 0);
}

// 테이블 셀(block)과 동일한 테이블이 textBlocks 내에 포함됐는지 여부를 리턴
static inline bool containsTable(const SRTextBlockList& textBlocks, SRTextBlockPtr searchBlock) {
  SRTextTablePtr searchTable = std::dynamic_pointer_cast<SRTextTable>(searchBlock);
  for (auto i = textBlocks.begin(); i != textBlocks.end(); ++i) {
    SRTextBlockPtr block = *i;
    if (block->getBlockType() == SRTextBlock::TableCell) {
      SRTextTableCellBlockPtr tableBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(block);
      SRTextTablePtr table = tableBlock->getTable();
      if (table == searchTable) {
        // SRTextTable 주소가 동일할 경우에만 동일한 테이블로 처리함
        // 테이블 속성이 동일하다고 동일 테이블은 아님
        return true;
      }
    }
  }
  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr 비교라서 안된다.
  return false;
}

static inline bool containsTextBlock(const SRTextBlockList& textBlocks, SRTextBlockPtr searchBlock) {
  if (textBlocks.empty())
    return false;

  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr 비교라서 안된다.
  for (auto i = textBlocks.begin(); i != textBlocks.end(); ++i) {
    if (*i == searchBlock) { // 주소가 동일한지 비교함
    //if (**i == *searchBlock) { // 값이 동일한지 비교함
      // SRTextBlock 객체가 동일한지 비교한다 -> **i == *searchBlock
      // SRTextBlock 속성이 동일하면 동일한 블럭으로 처리함, 추후 문단 병합등을 위함
      return true;
    }
  }
  return false;
}

static inline bool containsList(const SRTextBlockList& textBlocks, SRTextBlockPtr searchBlock) {
  SRTextListPtr searchList = std::dynamic_pointer_cast<SRTextList>(searchBlock);
  for (auto i = textBlocks.begin(); i != textBlocks.end(); ++i) {
    SRTextBlockPtr block = *i;
    if (block->getBlockType() == SRTextBlock::ListItem) {
      SRTextListPtr list = std::dynamic_pointer_cast<SRTextListBlock>(block)->getList();
      SR_ASSERT(list);
      if (list == searchList) {
        // SRTextList 주소가 동일할 경우에만 동일한 테이블로 처리함
        return true;
      }
    }
  }
  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr 비교라서 안된다.
  return false;
}

SRRange SRAttributedString::rangeOfTextBlock(SRTextBlockPtr block, SRUInt location) const {
  return rangeOfBlockType(block, location, containsTextBlock);
}

SRRange SRAttributedString::rangeOfTextTable(SRTextTablePtr table, SRUInt location) const {
  return rangeOfBlockType(table, location, containsTable);
}

SRRange SRAttributedString::rangeOfTextList(SRTextBlockPtr list, SRUInt location) const {
  return rangeOfBlockType(list, location, containsList);
}


static void test_dictionary0() {
  auto dict = SRObjectDictionary::create();
  //SRObjectDictionaryPtr dict = SRObjectDictionary::create();

  // Dictionary 에 ParagraphStyle 아이템 추가
  SRFloat headIndent = 10.0;
  CTParagraphStyleSetting settings[] = { { kCTParagraphStyleSpecifierHeadIndent, sizeof(SRFloat), &headIndent } };
  CTParagraphStylePtr paraIn = std::make_shared<CTParagraphStyle>(settings, SR_countof(settings));
  dict->addValue(kCTParagraphStyleAttributeName, paraIn);

  // Dictionary 에 Font 아이템 추가
  UIFontPtr fontIn = UIFont::fontWithName(L"Arial"); // UIFont::_fontCache 가 static 이므로 앱 종료시까지 계속 남아있게 된다.
  dict->addValue(kCTFontAttributeName, fontIn);
  //bool a = *fontIn != *fontIn;

  auto paraOut = dict->valueForKey(kCTParagraphStyleAttributeName);
  SR_ASSERT(*paraIn == *paraOut);
  auto fontOut = dict->valueForKey(kCTFontAttributeName);
  SR_ASSERT(*fontIn == *fontOut);

/*
id(146)   <SRDictionary: 0091CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00917B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 008EFEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
*/
  dict->dump();

#if 1
  // AttributedString 으로 생성할 문자열 생성
  auto str = std::make_shared<SRString>(L"abcde12345");

  // 위의 Dictionary 와 문자열로 AttributedString 생성
  auto astr = SRAttributedString::create(str, dict);

  if (1) {
    // 동일한 속성 추가 테스트
    //auto dict2 = SRObjectDictionary::create(dict);
    //SR_ASSERT(*dict == *dict2);
    headIndent = 11.0;
    CTParagraphStyleSetting settings[] = { { kCTParagraphStyleSpecifierHeadIndent, sizeof(SRFloat), &headIndent } };
    CTParagraphStylePtr paraStyle = std::make_shared<CTParagraphStyle>(settings, SR_countof(settings));
    //astr->addAttribute(kCTParagraphStyleAttributeName, paraStyle, SRRange(0, str->length()));
  }

  // 속성값 확인
  SRRange effectiveRange;
  SRObjectPtr obj = astr->attribute(kCTParagraphStyleAttributeName, 0, &effectiveRange);
  paraOut = std::dynamic_pointer_cast<CTParagraphStyle>(obj);
  SR_ASSERT(*paraIn == *paraOut);

  obj = astr->attribute(kCTFontAttributeName, 1, &effectiveRange);
  fontOut = std::dynamic_pointer_cast<UIFont>(obj);
  SR_ASSERT(*fontIn == *fontOut);

/*
id(176) 
abcde12345{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}
*/
  astr->dump();

  if (1) {
    // addAttribute, removeAttribute 테스트
    astr->setAttribute(kCTBackgroundColorAttributeName, std::make_shared<SRColor>(0.5, 1, 0.25), SRRange(1, 2));
    obj = astr->attribute(kCTBackgroundColorAttributeName, 0, &effectiveRange);
    obj = astr->attribute(kCTBackgroundColorAttributeName, 1, &effectiveRange);
    auto colorOut = std::dynamic_pointer_cast<SRColor>(obj);

    obj = astr->attribute(kCTBackgroundColorAttributeName, 2, &effectiveRange);
    obj = astr->attribute(kCTBackgroundColorAttributeName, 3, &effectiveRange);

/*
id(176) 
a{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}bc{
  <SRDictionary: 0059DEF0>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRBackgroundColorAttributeName = "<SRColor: 005767FC> 0.5, 1, 0.25, 1;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}de12345{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}
*/
    astr->dump();

    astr->removeAttribute(kCTBackgroundColorAttributeName, SRRange(1, 2));
/*
id(176) 
abcde12345{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}
*/
    astr->dump();
  }

  if (1) {
    // setAttributes 테스트
    //SRObjectDictionaryPtr dict2(new SRObjectDictionary());
    auto dict2 = SRObjectDictionary::create();

    //*dict2 = *dict;
    dict2->addValue(kCTBackgroundColorAttributeName, SRColorPtr(new SRColor(0.5, 1, 0.25)));

    astr->setAttributes(dict2, SRRange(1, 2));
/*
id(176) 
a{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}bc{
  <SRDictionary: 0059E0D0>
  SRBackgroundColorAttributeName = "<SRColor: 0056FB40> 0.5, 1, 0.25, 1;";
}de12345{
  <SRDictionary: 0059CB44>
  SRParagraphStyleAttributeName = "<CTParagraphStyle: 00597B2C> alignment: 4; firstLineHeadIndent: 0; headIndent: 10;";
  SRFontAttributeName = "<UIFont: 0056FEF4> font-family: Arial; font-weight: 5; font-size: 12pt; traits: 16777220;";
}
*/
    astr->dump();
  }
#endif
}

//static
void SRAttributedString::test(void* hwnd, int w, int h) {
  test_dictionary0();
  return;
}

} // namespace sr
