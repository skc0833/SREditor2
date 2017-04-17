#pragma once

#include "SRBase.h"
#include "SRAttributedString.h"
//#include "SRLayoutManager.h"

// https://developer.apple.com/library/prerelease/content/documentation/Cocoa/Conceptual/TextStorageLayer/Tasks/ChangingTextStorage.html#//apple_ref/doc/uid/20000848-CJBBIAAF

namespace sr {

class SRLayoutManager;
using SRLayoutManagerPtr = std::shared_ptr<SRLayoutManager>;
using SRLayoutManagerWeakPtr = std::weak_ptr<SRLayoutManager>;

class SRTextStorage;
using SRTextStoragePtr = std::shared_ptr<SRTextStorage>;
using SRTextStorageWeakPtr = std::weak_ptr<SRTextStorage>;

class SRTextStorage : public SRAttributedString, public std::enable_shared_from_this<SRTextStorage> {
  SR_MAKE_NONCOPYABLE(SRTextStorage);
public:
  SR_DECL_CREATE_FUNC(SRTextStorage);

  typedef enum EditActions {
    SRTextStorageEditedAttributes = (1 << 0), // Attributes were added, removed, or changed.
    SRTextStorageEditedCharacters = (1 << 1)  // Characters were added, removed, or replaced.
  } EditActions;

  typedef std::vector<SRLayoutManagerPtr>   SRLayoutManagerList;
  typedef SRLayoutManagerList::iterator     SRLayoutManagerListIter;

public:
  SRTextStorage(const SRString& str = SRString(L""));
  virtual ~SRTextStorage();

  //SRTextStorage(const SRTextStorage& rhs);
  //virtual SRTextStorage& operator=(const SRTextStorage& rhs);

  // override
  virtual void replaceCharactersInRange(SRRange range, const SRAttributedStringPtr attrStr);
  virtual void setAttributes(const SRObjectDictionaryPtr attrs, SRRange range, bool clearOtherAttributes = true);

  // Accessing Layout Managers
  //
  // Adds a layout manager to the receiver¡¯s set of layout managers.
  void addLayoutManager(const SRLayoutManagerPtr aLayoutManage);

  // Removes a layout manager from the receiver¡¯s set of layout managers.
  void removeLayoutManager(const SRLayoutManagerPtr aLayoutManage);

  // Handling Text Editing Messages
  //
  // Tracks changes made to the receiver, allowing the text storage to record the full extent of changes made.
  void edited(SRInt editedMask, SRRange editedRange, SRInt delta);

  virtual void beginEditing();

  // Cleans up changes made to the receiver and notifies its delegate and layout managers of changes.
  void processEditing();

  virtual void endEditing();

  // Called from processEditing() to invalidate attributes when the text storage changes. 
  // If the receiver is not lazy, this method simply calls fixAttributes(in:).
  // If lazy attribute fixing is in effect, this method instead records the range needing fixing.
  void invalidateAttributes(SRRange range);

  SRInt editedMask() {
    return _editedMask;
  }
  SRRange editedRange() {
    return _editedRange;
  }
  SRInt changeInLength() {
    return _editedDelta;
  }

  static SRTextStoragePtr initWithString(const SRString& string);

 private:
  // Determining the Nature of Changes
  //
  // A mask describing the kinds of edits pending for the text storage object. (read-only)
  //@property(readonly, nonatomic) SRTextStorageEditActions editedMask
  SRInt _editedMask;

  // The range of text containing changes. (read-only)
  SRRange _editedRange;

  // The difference between the current length of the edited range and its length before editing began. (read-only)
  SRInt _editedDelta;

  SRLayoutManagerList _layoutManagers;
};

} // namespace sr
