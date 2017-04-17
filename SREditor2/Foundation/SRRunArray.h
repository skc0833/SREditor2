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

  // ����� �����Ƿ� ���Ǹ� ���� �����ͷ� ���� ���ϰ� ��
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

  // RunArrayItem �� ����� ��ü ���̸� �����Ѵ�(block ������ �ƴ�)
  // Total count of values stored by the RunArrayItems in list
  SRIndex getCount() const { return _length; }
  //SRIndex getBlockCount() const { return _numBlocks; }

  // loc ��ġ�� ��ü�� ����. loc ��ġ�� ���Ե� ��(run)�� ũ�� ������ effectiveRange �� ������.
  // runArrayIndexPtr ���� �ش� ���� �ε����� ������.
  SRObjectPtr getValueAtIndex(SRIndex loc, SRRange* effectiveRange, SRIndex* blockIndexPtr = nullptr);
  
  // blockIndex ��ġ�� ��ü�� ����. lengthPtr ���� �ش� ���� ũ�⸦ ������.
  SRObjectPtr getValueAtRunArrayIndex(SRIndex blockIndex, SRIndex* lengthPtr = nullptr);

  void append(SRObjectPtr obj, SRIndex count); // skc added, insert �� ������. TODO ���ʿ�???
  
  // Inserts range.length instances of obj at location range.location
  void insert(SRRange range, SRObjectPtr obj);
  
  // Delete values specified by the range
  void remove(SRRange range);
  
  // Replaces values in the specified range with count instances of obj
  void replace(SRRange range, SRObjectPtr obj, SRIndex count);

  void clear() {
    remove(SRRange(0, getCount()));
  }

  void checkSanity(SRIndex loc = 0, SRIndex len = 0); // SRIndex == SRUInt �Ƿ� 0

  static void test();

private:
  SRRunArray::RunArrayItem& getRunArrayItem(SRIndex index);
  SRIndex blockForLocation(SRIndex location, SRRange *effectiveRange = nullptr);

  void init();
  SRRunArrayListPtr _list;
  SRIndex   _numBlocks; // SRRunArrayItem ����(_list.size()�� ����������, ������ ���� ���ܵ�)
  SRIndex   _length;    // SRRunArrayItem�� ����� ��ü ����
};

} // namespace sr
