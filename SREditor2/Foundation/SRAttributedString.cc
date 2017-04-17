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
    // �Ӽ������� �׻� ���ڿ� ���̸�ŭ �����ؾ� �Ѵ�.
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

  // ���� loc ��ġ�� �Ӽ������� ȹ���Ѵ�.
  SRRange effectiveRange;
  SRObjectPtr attrs = attributes(loc, longestEffectiveRange);

  // ������ ���� ������ �Ӽ������̸� rangeLimit ���������� ��� ����Ѵ�.
  loc += longestEffectiveRange->length;
  SRUInt limit = rangeLimit.location + rangeLimit.length; // �˻��� ������ ��ġ(�� ��ġ �ٷ� ���������� �˻��ؾ� ��)
  SRObjectPtr nextAttrs;
  while (loc < limit) {
    nextAttrs = attributes(loc, &effectiveRange);
    if (*attrs != *nextAttrs) break;
    longestEffectiveRange->length += effectiveRange.length;
    loc += effectiveRange.length;
  }

  // loc ��ġ�� �Ӽ������� �����Ѵ�.
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

  // ���� loc ��ġ�� �Ӽ����� ȹ���Ѵ�.
  SRObjectPtr value = attribute(attrName, loc, longestEffectiveRange);
  SR_ASSERT(value);

  // ������ ���� ������ �Ӽ����̸� rangeLimit ���������� ��� ����Ѵ�.
  loc += longestEffectiveRange->length;
  SRUInt limit = rangeLimit.location + rangeLimit.length; // �˻��� ������ ��ġ(�� ��ġ �ٷ� ���������� �˻��ؾ� ��)

  SRRange effectiveRange;
  SRObjectPtr nextValue;
  while (loc < limit) {
    nextValue = attribute(attrName, loc, &effectiveRange);
    if (*value != *nextValue) break;
    longestEffectiveRange->length += effectiveRange.length;
    loc += effectiveRange.length;
  }

  // loc ��ġ�� �Ӽ����� �����Ѵ�.
  return value;
}

// ������ ������ �� �����ϴ� �Ӽ��� ����Ͽ� �߰��Ѵ�.
// replaceCharacters() �� ��쿡�� ������ ���޹��� �Ӽ����� �߰���
void SRAttributedString::replaceCharacters(SRRange range, const SRString& replacement) {
  __checkRangeIsInBoundsVoid(range);

  SRIndex replacementLen = replacement.length();
  SRObjectDictionaryPtr attributesToBeUsed = nullptr;
  if (replacementLen > 0) {
    // By default extend replaced attributes, or take them from the previous character
    if (range.length > 0) {
      // ���� ����: ��� ������ 0 �̻��� ���, ��� ���� ���� ������ �Ӽ��� ȹ��
      attributesToBeUsed = attributes(range.location);
    }
    else if (range.location > 0) {
      // Ư����ġ�� ����: ��� ������ 0 �̰�, ������ ��ġ�� 0 �̻��� ���, ������ ��ġ �ٷ� ���� ������ �Ӽ��� ȹ��
      attributesToBeUsed = attributes(range.location - 1);
    }
    else if (getLength() > 0) {
      // ��� ����, ������ ��ġ�� �Ѵ� 0 �̰�, ���� ���ڿ� ���̰� 0 �̻��� ���, ù ������ �Ӽ��� ȹ��
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
  //__checkRangeIsInBoundsVoid(range); // ���� ���̴� ��쵵 �ִ�.

  SRStringPtr otherStr = replacement->getString();
  SRIndex otherStrLen = otherStr->length();

  if (otherStrLen > 0) {
    SRRange attrRange111 = { 0, 0 }; //error C2552: 'attrRange' : �̴ϼȶ����� ����� ����Ͽ� ������ü�� �ʱ�ȭ�� �� �����ϴ�.
    SRRange attrRange(0, 0);
    while (attrRange.location < otherStrLen) {
      // ���� ���޹��� �Ӽ����� ���� ���� ��ġ�� �߰�
      SRObjectDictionaryPtr otherAttrs = replacement->attributes(attrRange.location, &attrRange);
      SRObjectDictionaryPtr attrs = createAttributesDictionary(otherAttrs);
      // ���� ���ڿ��� range.location ���� ������ ���ڿ�(replacement)�� ó�� �Ӽ����� ������ �Ӽ����� �߰��Ѵ�.
      _attributeArray->insert(SRRange(attrRange.location + range.location, attrRange.length), attrs);
      attrRange.location += attrRange.length;
    }
    SR_ASSERT(attrRange.location == otherStrLen);
  }
  // ���޹��� range ������ ����(���Ե� �Ӽ��� ���� ��ġ����)
  if (range.length > 0) {
    _attributeArray->remove(SRRange(range.location + otherStrLen, range.length));
  }

  // ���޹��� replacement ���ڿ��� ����(���Ե� �Ӽ��� ���� ��ġ��)
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
  // ��Ʈ���� ���� ���, ����Ʈ ��Ʈ�� ä��� ��� �Ӽ��� �����Ѵ�.
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
      // ������ ������ �Ӽ��� ��ü�Ѵ�(���޹��� �Ӽ��� �����ؼ� ��ü�ؾ��Ѵ�)
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
        // range ������ �Ӽ��� replacementAttrs �Ӽ��� ��ģ��. �� �����ϴ� �Ӽ��� ��ü. map.insert() �Լ��� ����?
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
    // range.location ��ġ�� ��ȿ�� effectiveRange�� ���Ѵ�(effectiveRange.location �� range.location ���ϰ�)
    SRObjectPtr orgValue = _attributeArray->getValueAtIndex(range.location, &effectiveRange, NULL);
    SRObjectDictionaryPtr attrs = std::dynamic_pointer_cast<SRObjectDictionary>(orgValue);
    //SR_ASSERT(attrs); // empty �ϼ��� ����!

    // effectiveRange �� range �������� ����(Intersect effectiveRange and range)
    if (effectiveRange.location < range.location) {
      effectiveRange.length -= (range.location - effectiveRange.location);
      effectiveRange.location = range.location;
    }
    if (effectiveRange.length > range.length) {
      effectiveRange.length = range.length;
    }
    // First check to see if the same value already exists; this will avoid a copy
    SRObjectPtr existingValue = (attrs) ? attrs->valueForKey(attrName, nullptr) : nullptr;
    if (!existingValue || (*existingValue != *value)) { // SRObjectPtr�� �ƴ� ���� ��ü�� ������ ���Ѵ�.
      // ���� ��ġ(range.location)�� �Ӽ����� ���޹��� value �� ���� ������ ���� SRObjectDictionary��(attrs) �����Ͽ� �߰��Ѵ�.
      // We need to make a new copy
      attrs = createAttributesDictionary(attrs);
      attrs->setValue(value, attrName); // �ش�Ű�� �������� ������ �߰��ϰ�, �ƴϸ� �����Ѵ�.
      _attributeArray->replace(effectiveRange, attrs, effectiveRange.length);
    } else {
      // effectiveRange �� �Ӽ��� �����Ϸ��� �Ӽ�(value)�� ������ ���, �ƹ�ó���� ����
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

// location ��ġ �������� ��,��� ���� block �� �����ϴ� �ִ� ������ ������
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
        // ����(effRange.location - 1)���� ������ �� �˻�
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
        // ����(effRange.rangeMax())���� ������ �� �˻�
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

// ���̺� ��(block)�� ������ ���̺��� textBlocks ���� ���Եƴ��� ���θ� ����
static inline bool containsTable(const SRTextBlockList& textBlocks, SRTextBlockPtr searchBlock) {
  SRTextTablePtr searchTable = std::dynamic_pointer_cast<SRTextTable>(searchBlock);
  for (auto i = textBlocks.begin(); i != textBlocks.end(); ++i) {
    SRTextBlockPtr block = *i;
    if (block->getBlockType() == SRTextBlock::TableCell) {
      SRTextTableCellBlockPtr tableBlock = std::dynamic_pointer_cast<SRTextTableCellBlock>(block);
      SRTextTablePtr table = tableBlock->getTable();
      if (table == searchTable) {
        // SRTextTable �ּҰ� ������ ��쿡�� ������ ���̺�� ó����
        // ���̺� �Ӽ��� �����ϴٰ� ���� ���̺��� �ƴ�
        return true;
      }
    }
  }
  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr �񱳶� �ȵȴ�.
  return false;
}

static inline bool containsTextBlock(const SRTextBlockList& textBlocks, SRTextBlockPtr searchBlock) {
  if (textBlocks.empty())
    return false;

  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr �񱳶� �ȵȴ�.
  for (auto i = textBlocks.begin(); i != textBlocks.end(); ++i) {
    if (*i == searchBlock) { // �ּҰ� �������� ����
    //if (**i == *searchBlock) { // ���� �������� ����
      // SRTextBlock ��ü�� �������� ���Ѵ� -> **i == *searchBlock
      // SRTextBlock �Ӽ��� �����ϸ� ������ ������ ó����, ���� ���� ���յ��� ����
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
        // SRTextList �ּҰ� ������ ��쿡�� ������ ���̺�� ó����
        return true;
      }
    }
  }
  //std::find(textBlocks.begin(), textBlocks.end(), searchBlock) != textBlocks.end(); // ptr �񱳶� �ȵȴ�.
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

  // Dictionary �� ParagraphStyle ������ �߰�
  SRFloat headIndent = 10.0;
  CTParagraphStyleSetting settings[] = { { kCTParagraphStyleSpecifierHeadIndent, sizeof(SRFloat), &headIndent } };
  CTParagraphStylePtr paraIn = std::make_shared<CTParagraphStyle>(settings, SR_countof(settings));
  dict->addValue(kCTParagraphStyleAttributeName, paraIn);

  // Dictionary �� Font ������ �߰�
  UIFontPtr fontIn = UIFont::fontWithName(L"Arial"); // UIFont::_fontCache �� static �̹Ƿ� �� ����ñ��� ��� �����ְ� �ȴ�.
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
  // AttributedString ���� ������ ���ڿ� ����
  auto str = std::make_shared<SRString>(L"abcde12345");

  // ���� Dictionary �� ���ڿ��� AttributedString ����
  auto astr = SRAttributedString::create(str, dict);

  if (1) {
    // ������ �Ӽ� �߰� �׽�Ʈ
    //auto dict2 = SRObjectDictionary::create(dict);
    //SR_ASSERT(*dict == *dict2);
    headIndent = 11.0;
    CTParagraphStyleSetting settings[] = { { kCTParagraphStyleSpecifierHeadIndent, sizeof(SRFloat), &headIndent } };
    CTParagraphStylePtr paraStyle = std::make_shared<CTParagraphStyle>(settings, SR_countof(settings));
    //astr->addAttribute(kCTParagraphStyleAttributeName, paraStyle, SRRange(0, str->length()));
  }

  // �Ӽ��� Ȯ��
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
    // addAttribute, removeAttribute �׽�Ʈ
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
    // setAttributes �׽�Ʈ
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
