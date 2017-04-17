#pragma once

#include "SRObject.h"
#include "SRString.h"
#include "SRRunArray.h"
#include "SRDictionary.h"
#include "SRColor.h"
#include "SRTextBlock.h"
#include "../CoreText/CTParagraphStyle.h"

namespace sr {

extern const wchar_t* kCTForegroundColorAttributeName;
extern const wchar_t* kCTBackgroundColorAttributeName;
extern const wchar_t* kCTFontAttributeName;
extern const wchar_t* kCTParagraphStyleAttributeName;

//CORETEXT_EXPORT const CFStringRef kCTCharacterShapeAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTFontAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTKernAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTLigatureAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTForegroundColorAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTForegroundColorFromContextAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTParagraphStyleAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTStrokeWidthAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTStrokeColorAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTSuperscriptAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTUnderlineColorAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTUnderlineStyleAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTVerticalFormsAttributeName;
//CORETEXT_EXPORT const CFStringRef kCTGlyphInfoAttributeName;
extern const wchar_t* kCTRunDelegateAttributeName;

class SRAttributedString;
using SRAttributedStringPtr = std::shared_ptr<SRAttributedString>;
using SRAttributedStringWeakPtr = std::weak_ptr<SRAttributedString>;

class SRAttributedString : public SRObject {
  SR_MAKE_NONCOPYABLE(SRAttributedString);
public:
  SR_DECL_CREATE_FUNC(SRAttributedString);

  SRAttributedString(const SRString& str = L"", const SRObjectDictionaryPtr attributes = nullptr); // 생성자는 항상 문자열, 속성을 같이 전달받자!
  ~SRAttributedString();

  virtual SRStringUPtr toString() const;

  const SRStringPtr getString() const { return _string; }
  SRIndex getLength() const { return _string->length(); }
  const SRRunArrayPtr getAttributeArray() const { return _attributeArray; }

public:

  // Retrieving Attribute Information
  //

  /*! @function CFAttributedStringGetAttributes
  Returns the attributes at the specified location. If effectiveRange is not NULL, upon return *effectiveRange
  contains a range over which the exact same set of attributes apply.
  Note that for performance reasons, the returned effectiveRange is not necessarily the maximal range -
  for that, use CFAttributedStringGetAttributesAndLongestEffectiveRange().
  It's a programming error for loc to specify a location outside the bounds of the attributed string.

  Note that the returned attribute dictionary might change in unpredictable ways from under the caller
  if the attributed string is edited after this call. If you wish to hang on to the dictionary long-term,
  you should make an actual copy of it rather than just retaining it.
  Also, no assumptions should be made about the relationship of the actual CFDictionaryRef returned by
  this call and the dictionary originally used to set the attributes, other than the fact that
  the values stored in the dictionary will be identical (that is, ==) to those originally specified.
  */
  // Returns the attributes for the character at a given index, and by reference the range over which the attributes apply.
  // effectiveRange: Upon return, the range over which the attributes and values are the same as those at index. 
  // loc 위치의 속성들을 리턴. 리턴한 속성들의 범위정보를 effectiveRange 로 리턴
  SRObjectDictionaryPtr attributes(SRUInt loc, SRRange* effectiveRange = nullptr) const;

  // longestEffectiveRange: If non-NULL, upon return contains the maximum range over which the attributes and values 
  // are the same as those at index, clipped to rangeLimit.
  // loc 위치의 속성들을 리턴. 리턴한 속성들과 동일한 최대한의 범위정보를 longestEffectiveRange 로 리턴
  SRObjectDictionaryPtr attributes(SRUInt loc, SRRange* longestEffectiveRange, SRRange rangeLimit) const;

  // Returns the value of a single attribute at the specified location.
  // If the specified attribute doesn't exist at the location, returns NULL.
  // If effectiveRange is not NULL, upon return *effectiveRange contains a range over which the exact same attribute value applies.
  // Returns the value for an attribute with a given name of the character at a given index, and by reference the range over which the attribute applies.
  SRObjectPtr attribute(const SRString& attrName, SRUInt loc, SRRange* effectiveRange = nullptr) const;
  SRObjectPtr attribute(const SRString& attrName, SRUInt loc, SRRange* longestEffectiveRange, SRRange rangeLimit) const;

  // Changing characters
  //

  // The new characters inherit the attributes of the first replaced character from aRange. 
  // Where the length of aRange is 0, the new characters inherit the attributes of the character 
  // preceding aRange if it has any, otherwise of the character following aRange.
  // 교체되는 문자열은 교체범위 첫 글자의 속성을 상속받는다.
  // 교체범위 길이가 0 일 경우에는 교체범위 바로 이전 글자가 존재하면 이 글자의 속성을
  // 아니면 교체범위 바로 이후 글자의 속성을 상속받는다.
  virtual void replaceCharacters(SRRange range, const SRString& str);

  // deletes the characters in the given range along with their associated attributes.
  void deleteCharacters(SRRange range);

  // Changing attributes
  //

  // Sets the attributes for the characters in the specified range to the specified attributes.
  // These new attributes replace any attributes previously associated with the characters in aRange.
  // attrs 속성집합은 range 범위의 모든 속성집합을 교체한다.
  // 유효하게 지정된 범위의 속성집합을 설정한다.
  // clearOtherAttributes 가 false 이면, 설정되지 않는 속성들은 유지되며 그렇지 않으면 삭제됨.
  // dictionary 는 일반적인 방식으로 설정돼야 한다((CFString keys, and arbitrary CFType values)
  // 이 함수 호출 이후에 attrs 전달인자에 변경을 가할 경우, 이는 이 attributed string 에는 영향을 안 미치게 된다.
  virtual void setAttributes(const SRObjectDictionaryPtr attrs, SRRange range, bool clearOtherAttributes = true);

  // Adds an attribute with the given name and value to the characters in the specified range.
  // 해당키가 존재하지 않으면 추가하고, 아니면 수정한다.
  void setAttribute(const SRString& attrName, const SRObjectPtr value, SRRange range);

  // Removes the named attribute from the characters in the specified range.
  void removeAttribute(const SRString& name, SRRange range);

  // Changing characters and attributes
  //

  // Adds the characters and attributes of a given attributed string to the end of the receiver.
  void append(const SRAttributedStringPtr attrStr);

  // Inserts the characters and attributes of the given attributed string into the receiver at the given index.
  void insert(const SRAttributedStringPtr attrStr, SRUInt loc);

  // Replaces the characters and attributes in a given range with the characters and attributes of the given attributed string.
  virtual void replaceCharacters(SRRange range, const SRAttributedStringPtr attrStr);

  // Replaces the receiver’s entire contents with the characters and attributes of the given attributed string.
  void setAttributedString(const SRAttributedStringPtr attrStr);

  // Grouping changes
  //
  // Overridden by subclasses to buffer or optimize a series of changes to the receiver’s characters or attributes, 
  // until it receives a matching endEditing message, upon which it can consolidate changes and notify any observers that it has changed.
  virtual void beginEditing() { SR_ASSERT(0); }

  // Overridden by subclasses to consolidate changes made since a previous beginEditing message and to notify any observers of the changes.
  virtual void endEditing() { SR_ASSERT(0); }

  // Fixing attributes after changes
  //
  // Cleans up font, paragraph style, and attachment attributes within the given range.
  void fixAttributes(SRRange range);

  typedef bool(*cbContainsBlockType)(const SRTextBlockList& textBlocks, SRTextBlockPtr block);
  SRRange rangeOfBlockType(SRTextBlockPtr block, SRUInt location, cbContainsBlockType containsBlockType) const;

  // Returns the range of the individual text block that contains the given location.
  SRRange rangeOfTextBlock(SRTextBlockPtr block, SRUInt location) const;

  // Returns the range of the given text table that contains the given location
  SRRange rangeOfTextTable(SRTextTablePtr table, SRUInt location) const;

  SRRange rangeOfTextList(SRTextBlockPtr list, SRUInt location) const;

  static void test(void* hwnd, int w, int h);

private:
  // deletes the characters in the given range along with their associated attributes.
  //void deleteString(SRRange range); // skc add
  SRObjectDictionaryPtr createAttributesDictionary(const SRObjectDictionaryPtr attrs) const;
  bool isRangeInBounds(const SRRange& range) const {
    // {0, 0} 으로 호출되는 경우도 있다(맨 처음 위치인 경우)
    if (range.location >= 0 && range.length >= 0 && (range.location + range.length) <= getLength()) return true;
    else return false;
  }

  SRStringPtr _string;
  SRRunArrayPtr _attributeArray;
};

} // namespace sr
