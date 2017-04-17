#pragma once

#include "SRObject.h"
#include <vector>

namespace sr {

class SRRunArray;
using SRRunArrayPtr = std::shared_ptr<SRRunArray>;

class SRRunArray : public SRObject {
  SR_MAKE_NONCOPYABLE(SRRunArray);
public:
  SR_DECL_CREATE_FUNC(SRRunArray);

  // 사이즈가 작으므로 편의를 위해 포인터로 관리 안하게 함
  typedef struct RunArrayItem {
    RunArrayItem(SRIndex len_ = 0, SRObjectPtr obj_ = NULL) {
      length = len_;
      obj = obj_;
    }
    ~RunArrayItem() {
    }
    SRIndex     length;
    SRObjectPtr obj;
  } RunArrayItem;

  typedef std::vector<SRRunArray::RunArrayItem>   SRRunArrayList;
  typedef SRRunArrayList::iterator                SRRunArrayListIter;
  typedef std::shared_ptr<SRRunArrayList>         SRRunArrayListPtr;

  SRRunArray();
  ~SRRunArray();

  virtual SRStringUPtr toString() const override;
  virtual SRRunArray& operator+=(const SRRunArrayPtr rhs);

  // RunArrayItem 에 저장된 전체 길이를 리턴한다(block 개수가 아님)
  // Total count of values stored by the RunArrayItems in list
  SRIndex getCount() const { return _length; }
  //SRIndex getBlockCount() const { return _numBlocks; }

  // loc 위치의 객체를 리턴. loc 위치가 포함된 블럭(run)의 크기 정보를 effectiveRange 에 리턴함.
  // runArrayIndexPtr 에는 해당 블럭의 인덱스를 리턴함.
  SRObjectPtr getValueAtIndex(SRIndex loc, SRRange* effectiveRange, SRIndex* blockIndexPtr = nullptr);
  
  // blockIndex 위치의 객체를 리턴. lengthPtr 에는 해당 블럭의 크기를 리턴함.
  SRObjectPtr getValueAtRunArrayIndex(SRIndex blockIndex, SRIndex* lengthPtr = nullptr);

  void append(SRObjectPtr obj, SRIndex count); // skc added, insert 로 구현함. TODO 불필요???
  
  // Inserts range.length instances of obj at location range.location
  void insert(SRRange range, SRObjectPtr obj);
  
  // Delete values specified by the range
  void remove(SRRange range);
  
  // Replaces values in the specified range with count instances of obj
  void replace(SRRange range, SRObjectPtr obj, SRIndex count);

  void clear() {
    remove(SRRange(0, getCount()));
  }

  void checkSanity(SRIndex loc = 0, SRIndex len = 0); // SRIndex == SRUInt 므로 0

  static void test();

private:
  SRRunArray::RunArrayItem& getRunArrayItem(SRIndex index);
  SRIndex blockForLocation(SRIndex location, SRRange *effectiveRange = nullptr);

  void init();
  SRRunArrayListPtr _list;
  SRIndex   _numBlocks; // SRRunArrayItem 개수(_list.size()와 동일하지만, 검증을 위해 남겨둠)
  SRIndex   _length;    // SRRunArrayItem에 저장된 전체 길이
};

} // namespace sr
