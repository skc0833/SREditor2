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

// setAttributedString() �Լ��� �� �Լ��� ȣ���ϹǷ� override�� �ʿ䰡 ����.
void SRTextStorage::replaceCharactersInRange(SRRange range, const SRAttributedStringPtr attrStr) {
  SRAttributedString::replaceCharacters(range, attrStr);
  //int actions = SRTextStorageEditedAttributes | SRTextStorageEditedCharacters;
  edited(SRTextStorageEditedAttributes | SRTextStorageEditedCharacters, range, attrStr->getLength());
}

void SRTextStorage::setAttributes(const SRObjectDictionaryPtr attrs, SRRange range, bool clearOtherAttributes) {
  SRAttributedString::setAttributes(attrs, range, clearOtherAttributes);
  edited(SRTextStorageEditedAttributes, range, range.length);
}

// ���� ����� ȣ���(from replaceCharactersInRange(), setAttributes())
void SRTextStorage::edited(SRInt editedMask, SRRange editedRange, SRInt delta) {
  // e.g, "The files" -> "Several files", editedRange is {0, 7} and delta is 4
  // TODO beginEditing, endEditing ������ �� �Լ��� ���� ��������� �Ѵ�.
  // invokes processEditing if there are no outstanding beginEditing calls.
  _editedMask |= editedMask;
  _editedRange = editedRange;
  _editedDelta = delta;
  //processEditing(); // ��������� ���̾ƿ���Ű��!
}

void SRTextStorage::beginEditing() {
  //_editCount++;
}

void SRTextStorage::endEditing() {
  //if (_editCount == 0) { // ��ü ������ �Ϸ�� ���?
  processEditing();
}

void SRTextStorage::invalidateAttributes(SRRange range) {
  fixAttributes(range);
}

// edited() or endEditing() ���� ȣ���
void SRTextStorage::processEditing() {
#if 1
  // processEditing() -> textStorage(deprecated!!!) // ����/�Ӽ� ������ �˸�. textStorage(), processEditingForTextStorage�� ���
  //SRRange editedRange(0, getLength());
  //SRInt editedMask = SRTextStorageEditedAttributes | SRTextStorageEditedCharacters;
  //SRInt changeInLength = 0;
  // Represents the range of characters affected after attributes have been fixed.
  // Is either equal to newCharRange or larger. 
  // For example, deleting a paragraph separator character invalidates the layout information 
  // for all characters in the paragraphs that precede and follow the separator.
  // �Ӽ��� ������ ����(newCharRange(editedRange) ũ�� �̻���), ���� �и��� ������ ����, ���� ����ũ�Ⱑ ��
  //SRRange invalidatedRange = editedRange();

  // layout manager �鿡�� ����
  // WinObjC�� �ܼ��� processEditingForTextStorage() �� ȣ������
  // gnustep�� 
  // 1) [nc postNotificationName: NSTextStorageWillProcessEditingNotification object: self];
  // -> NSObject.textStorageWillProcessEditing() ȣ��? �� �Լ� �����ΰ� ����?
  // 2) *** invalidateAttributesInRange() ȣ��. AttrStr�� fixAttributesInRange() ȣ��. ���⼭ font, para, attachment �Ӽ� ����
  // 3) [nc postNotificationName: NSTextStorageDidProcessEditingNotification object: self];
  // -> NSObject.textStorageDidProcessEditing() ȣ��? �� �Լ� �����ΰ� ����?
  // ����, GSLayoutManager.textStorage(deprecated!)���� ȣ������(NSLayoutManager�� GSLayoutManager�� ��ӹ���)
  SRRange oldEditedRange = editedRange();
  //SRInt oldEditedDelta = changeInLength();
  invalidateAttributes(oldEditedRange); // �� �Լ� ���Ŀ� _editedRange �� ����ɼ� �ִ�!

  //for (SRLayoutManagerListIter it = _layoutManagers.begin(); it != _layoutManagers.end(); ++it) {
  for (const auto& item : _layoutManagers) {
    //SRLayoutManagerPtr& item = *it;
    // e.g, "The files" -> "Several files", newCharRange(editedRange) is {0, 7} and delta(changeInLength) is 4
    SRTextStoragePtr ts = this->shared_from_this();
    SRInt editMask = editedMask();
    // ����� ���ڿ� ����(The range in the final string that was explicitly edited)
    SRRange newCharRange = oldEditedRange; // �Ӽ� ���� ���� ����
    SRInt delta = changeInLength(); // The length delta for the editing changes.
    // The range of characters that changed as a result of attribute fixing.
    // This invalidated range is either equal to newCharRange or larger.
    SRRange invalidatedCharRange = editedRange(); // �Ӽ��� ������ ����(newCharRange �̻���)

    // @@ ���� �������ڵ��� ���� ���� �������� �ʰ� ������, LM ������ �ƹ� ���ڵ� ������� �ʰ� ����
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
  // layout manager �鿡�� ����
  for (SRLayoutManagerListIter it = _layoutManagers.begin(); it != _layoutManagers.end(); ++it) {
    SRLayoutManagerPtr& item = *it;
    item->processEditingForTextStorage(SRTextStoragePtr(this), _editedMask, _editedRange, _changeInLength, _editedRange);
  }
#endif
}

void SRTextStorage::addLayoutManager(const SRLayoutManagerPtr aLayoutManage) {
  auto iter = std::find(_layoutManagers.begin(), _layoutManagers.end(), aLayoutManage);
  if (iter == _layoutManagers.end()) {
    // �̹� �߰��� ���� ���� ��쿡�� �߰���
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
