#include "SRTextStorage.h"
#include "SRLayoutManager.h"
//#include "SRLog.h"
//#include "SRString.h"
#include <algorithm>

namespace sr {


SRTextStorage::SRTextStorage(const SRString& str)
 : SRAttributedString(str) {
  _editedMask = 0;
  _editedRange = SRRange(0, 0);
  _editedDelta = 0;
}

SRTextStorage::~SRTextStorage() {
}

// setAttributedString() 함수도 이 함수를 호출하므로 override할 필요가 없다.
void SRTextStorage::replaceCharactersInRange(SRRange range, const SRAttributedStringPtr attrStr) {
  SRAttributedString::replaceCharacters(range, attrStr);
  //int actions = SRTextStorageEditedAttributes | SRTextStorageEditedCharacters;
  edited(SRTextStorageEditedAttributes | SRTextStorageEditedCharacters, range, attrStr->getLength());
}

void SRTextStorage::setAttributes(const SRObjectDictionaryPtr attrs, SRRange range, bool clearOtherAttributes) {
  SRAttributedString::setAttributes(attrs, range, clearOtherAttributes);
  edited(SRTextStorageEditedAttributes, range, range.length);
}

// 내용 변경시 호출됨(from replaceCharactersInRange(), setAttributes())
void SRTextStorage::edited(SRInt editedMask, SRRange editedRange, SRInt delta) {
  // e.g, "The files" -> "Several files", editedRange is {0, 7} and delta is 4
  // TODO beginEditing, endEditing 구현시 이 함수도 같이 수정해줘야 한다.
  // invokes processEditing if there are no outstanding beginEditing calls.
  _editedMask |= editedMask;
  _editedRange = editedRange;
  _editedDelta = delta;
  //processEditing(); // 명시적으로 레이아웃시키자!
}

void SRTextStorage::beginEditing() {
  //_editCount++;
}

void SRTextStorage::endEditing() {
  //if (_editCount == 0) { // 전체 편집이 완료된 경우?
  processEditing();
}

void SRTextStorage::invalidateAttributes(SRRange range) {
  fixAttributes(range);
}

// edited() or endEditing() 에서 호출됨
void SRTextStorage::processEditing() {
#if 1
  // processEditing() -> textStorage(deprecated!!!) // 문자/속성 변경을 알림. textStorage(), processEditingForTextStorage와 비슷
  //SRRange editedRange(0, getLength());
  //SRInt editedMask = SRTextStorageEditedAttributes | SRTextStorageEditedCharacters;
  //SRInt changeInLength = 0;
  // Represents the range of characters affected after attributes have been fixed.
  // Is either equal to newCharRange or larger. 
  // For example, deleting a paragraph separator character invalidates the layout information 
  // for all characters in the paragraphs that precede and follow the separator.
  // 속성이 보정된 영역(newCharRange(editedRange) 크기 이상임), 문단 분리자 삭제시 이전, 이후 문단크기가 됨
  //SRRange invalidatedRange = editedRange();

  // layout manager 들에게 통지
  // WinObjC는 단순히 processEditingForTextStorage() 만 호출중임
  // gnustep은 
  // 1) [nc postNotificationName: NSTextStorageWillProcessEditingNotification object: self];
  // -> NSObject.textStorageWillProcessEditing() 호출? 이 함수 구현부가 없다?
  // 2) *** invalidateAttributesInRange() 호출. AttrStr의 fixAttributesInRange() 호출. 여기서 font, para, attachment 속성 수정
  // 3) [nc postNotificationName: NSTextStorageDidProcessEditingNotification object: self];
  // -> NSObject.textStorageDidProcessEditing() 호출? 이 함수 구현부가 없다?
  // 이후, GSLayoutManager.textStorage(deprecated!)들을 호출중임(NSLayoutManager는 GSLayoutManager를 상속받음)
  SRRange oldEditedRange = editedRange();
  //SRInt oldEditedDelta = changeInLength();
  invalidateAttributes(oldEditedRange); // 이 함수 이후에 _editedRange 가 변경될수 있다!

  //for (SRLayoutManagerListIter it = _layoutManagers.begin(); it != _layoutManagers.end(); ++it) {
  for (const auto& item : _layoutManagers) {
    //SRLayoutManagerPtr& item = *it;
    // e.g, "The files" -> "Several files", newCharRange(editedRange) is {0, 7} and delta(changeInLength) is 4
    SRTextStoragePtr ts = this->shared_from_this();
    SRInt editMask = editedMask();
    // 변경된 문자열 범위(The range in the final string that was explicitly edited)
    SRRange newCharRange = oldEditedRange; // 속성 보정 이전 영역
    SRInt delta = changeInLength(); // The length delta for the editing changes.
    // The range of characters that changed as a result of attribute fixing.
    // This invalidated range is either equal to newCharRange or larger.
    SRRange invalidatedCharRange = editedRange(); // 속성이 보정된 영역(newCharRange 이상임)

    // @@ 현재 전달인자들이 전부 값이 설정되지 않고 있으며, LM 에서도 아무 인자도 사용하지 않고 있음
    item->processEditing(ts, editMask, newCharRange, delta, invalidatedCharRange);
  }

  // edited values reset to be used again in the next pass.
  _editedRange = SRRange(0, 0);
  _editedDelta = 0;
  _editedMask = 0;
#else
  // 1) SRTextStorageWillProcessEditingNotification
  // posting an SRTextStorageWillProcessEditingNotification to the default notification center
  // Posted before a text storage finishes processing edits in processEditing.

  // 2) fixes attributes
  fixAttributesInRange(_editedRange);

  // 3) SRTextStorageDidProcessEditingNotification
  // Posted after a text storage finishes processing edits in processEditing.

  // 4) processEditingForTextStorage
  // layout manager 들에게 통지
  for (SRLayoutManagerListIter it = _layoutManagers.begin(); it != _layoutManagers.end(); ++it) {
    SRLayoutManagerPtr& item = *it;
    item->processEditingForTextStorage(SRTextStoragePtr(this), _editedMask, _editedRange, _changeInLength, _editedRange);
  }
#endif
}

void SRTextStorage::addLayoutManager(const SRLayoutManagerPtr aLayoutManage) {
  auto iter = std::find(_layoutManagers.begin(), _layoutManagers.end(), aLayoutManage);
  if (iter == _layoutManagers.end()) {
    // 이미 추가돼 있지 않은 경우에만 추가함
    _layoutManagers.push_back(aLayoutManage);
    aLayoutManage->setTextStorage(this->shared_from_this());
  }
}

void SRTextStorage::removeLayoutManager(const SRLayoutManagerPtr aLayoutManage) {
  SR_NOT_IMPL();
  sr::remove_erase(_layoutManagers, aLayoutManage);
}

//static 
SRTextStoragePtr SRTextStorage::initWithString(const SRString& string) {
  return std::make_shared<SRTextStorage>(string);
}

} // namespace sr
