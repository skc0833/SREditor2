#include "SRRunArray.h"
#include <vector>
#include "SRLog.h"
#include "SRString.h"
#include <sstream> // for wostringstream

// from Commits on Mar 31, 2016
// https://github.com/Microsoft/WinObjC/blob/develop/Frameworks/CoreFoundation/String.subproj/CFRunArray.c

#undef SR_LOG_FUNC
#define SR_LOG_FUNC()

namespace sr {

typedef struct SRRunArray::RunArrayItem SRRunArrayItem;

static bool ISSAME(const SRObjectPtr& obj1, const SRObjectPtr& obj2) {
  if (*obj1 == *obj2) {
    return true;
  } else {
    return false;
  }
}

SRRunArray::SRRunArray() {
  SR_LOG_FUNC();
  init();
}

void SRRunArray::init() {
  _numBlocks = 0; // 1 ???
  _length = 0;
  _list = std::make_shared<SRRunArrayList>(_numBlocks); // 0 �� ���ǹ���?
  //__list = *_list; // @@
}

SRRunArray::~SRRunArray() {
  SR_LOG_FUNC();
  _list = nullptr;
}

SRStringUPtr SRRunArray::toString() const {
  SRIndex idx = 0;
  std::wostringstream ss;
  ss << L"<SRRunArray: " << this << L"> _numBlocks: " << _numBlocks << L", _length: " << _length << std::endl;
  for (const auto& item : *_list) {
    ss << L"[" << idx++ << L"] length: " << item.length << L", obj=" << item.obj->toString()->c_str() << std::endl;
  }
  return std::make_unique<SRString>(ss.str());
}

SRRunArray& SRRunArray::operator+=(const SRRunArrayPtr rhs) {
  for (SRIndex idx = 0; idx < rhs->getCount(); ++idx) {
    SRIndex length;
    SRObjectPtr obj = getValueAtRunArrayIndex(idx, &length);
    this->append(obj, length);
    SR_LOGD(L"[%d] length=%d, obj=%p", idx++, length, obj);
  }
  return *this;
}

void SRRunArray::checkSanity(SRIndex loc, SRIndex len) {
  if (loc > 0) {
    SR_ASSERT(loc <= this->_length);
    if (len > 0 && loc + len > _length) {
      // ������ ����Ʈ ������ �Ѿ�� �����
      SR_ASSERT(0);
    }
  }
  if (_list->size() == 0) { // ���� insert �ÿ� �߻���
    //SR_ASSERT(0);
  }
  SR_ASSERT(_list->size() == this->_numBlocks);
}

// index ��ġ�� ���� ����
SRRunArrayItem& SRRunArray::getRunArrayItem(SRIndex index) {
  SR_ASSERT(_list->size() > 0 && index < (SRIndex)_list->size());
  SRRunArrayItem& item = _list->at(index);
  return item;
}

SRIndex SRRunArray::blockForLocation(SRIndex location, SRRange *effectiveRange) {
  checkSanity(location);

  SRIndex loc = 0;
  SRIndex block = 0;
  SRIndex itemLength = 0;

  //for (SRRunArrayListIter it = _list->begin(); it != _list->end(); ++it) {
  for (const auto& item : *_list) {
    //SRRunArrayItem& item = *it;
    itemLength = item.length;
    if (location > 0 && loc + item.length <= location) {
      // ���� ������ �̵�
      loc += itemLength;
      ++block;
    } else {
      // 0 ��ġ or ����� ���� ���ԵǴ� ��ġ�� ��� ����
      break;
    }
  }
  if (effectiveRange) {
    // ã�� �� ������ ����
    effectiveRange->location = loc;
    effectiveRange->length = itemLength;
  }
  return block;
}

SRObjectPtr SRRunArray::getValueAtIndex(SRIndex loc, SRRange* effectiveRange, SRIndex* blockIndexPtr) {
  SRIndex blockIndex = blockForLocation(loc, effectiveRange);
  if (blockIndexPtr) *blockIndexPtr = blockIndex;
  if (_list->empty()) {
    if (loc > 0) {
      SR_ASSERT(0);
    }
    return nullptr;
  }
  return _list->at(blockIndex).obj;
}

SRObjectPtr SRRunArray::getValueAtRunArrayIndex(SRIndex blockIndex, SRIndex* lengthPtr) {
  if (blockIndex >= _numBlocks) return nullptr;
  if (lengthPtr) *lengthPtr = _list->at(blockIndex).length;
  return _list->at(blockIndex).obj;
}

void SRRunArray::append(SRObjectPtr obj, SRIndex count) {
  SRRange range(_length, count);
  insert(range, obj);
}

// range.location ��ġ�� range.length ���� ��ŭ �߰���
void SRRunArray::insert(SRRange range, SRObjectPtr obj) {
  if (range.length == 0) {
    SR_ASSERT(0);
    return; // insert �� ��ü�� �����Ƿ� ����ó����
  }
  checkSanity(range.location);

  if (range.location == this->_length) {	// Append
    // ����Ʈ ���� �߰��Ǵ� ���
    if (this->_length > 0) {	// The list isn't empty
      // �� ����Ʈ�� �ƴ� ���
      if (ISSAME(obj, getRunArrayItem(this->_numBlocks - 1).obj)) {
        // ����Ʈ ���� ���� ���� ���� ���, ������ ���� ���̸� �÷���
        SRRunArrayItem& item = getRunArrayItem(this->_numBlocks - 1);
        item.length += range.length;
      } else {
        // ����Ʈ ���� ���� �ٸ� ���̸�, ����Ʈ ���� ���ο� ���� �߰���
        _list->push_back(RunArrayItem(range.length, obj));
        this->_numBlocks++;
      }
    } else {	// The list is empty...
      // �� ����Ʈ�� ���, ����Ʈ�� �߰���
      _list->push_back(RunArrayItem(range.length, obj));
      this->_numBlocks++;
    }
  } else {	// At this stage we are inserting, and the length of the list is > 0.
    // ����Ʈ �߰��� ���ԵǴ� ���
    SRRange blockRange;
    SRIndex block = blockForLocation(range.location, &blockRange);
    if (ISSAME(obj, getRunArrayItem(block).obj)) {
      // ���� ��ġ ��(�����)�� �����ϸ� �ش� ���� ���̸� ����
      getRunArrayItem(block).length += range.length;
    } else if ((block > 0) && (blockRange.location == range.location) && (ISSAME(obj, getRunArrayItem(block - 1).obj))) {
      // �� ���� ��ġ�� ���Եǰ�, �ٷ� ���� ���� �����ϸ� ���� ���� ���̸� ����
      getRunArrayItem(block - 1).length += range.length;
    } else if (blockRange.location == range.location) {
      // �� ���� ��ġ�� ���Ե����� ����, ���� ���� �ٸ� ��ü�̸�, ���� ��ġ ���� ������ 1��ŭ ���������� shift
      _list->insert(_list->begin() + block, RunArrayItem(range.length, obj));
      _numBlocks++;
    } else {
      // �� �߰��� �ٸ� ��ü�� ���Ե� ���, ���� ���� �յڷ� ������ �� ���̸� ����
      SRIndex headLen = range.location - blockRange.location; // head -> ũ��: range ���� - blockRange ����
      SRIndex tailLen = blockRange.length - headLen; // tail -> ũ��: ���� blockRange ũ�⿡�� head ��ŭ ����
      getRunArrayItem(block).length = headLen; // head
      _list->insert(_list->begin() + block + 1, RunArrayItem(range.length, obj)); // ���� ��ü(obj)
      _list->insert(_list->begin() + block + 2, RunArrayItem(tailLen, getRunArrayItem(block).obj)); // tail
      _numBlocks += 2;
    }
  }
  _length += range.length;
   checkSanity();
}

void SRRunArray::remove(SRRange range) {
  replace(range, nullptr, 0);
}

void SRRunArray::replace(SRRange range, SRObjectPtr newObject, SRIndex newLength) {
  checkSanity(range.location, range.length);
  if (range.length == 0) {
    // ������ insert()�� ó��. �� �Լ����� ����ó���ص� �ɵ�??? -> insert(SRRange(range.location, newLength), newObject);
    //SR_ASSERT(0);
    return;
  }
  SRRange blockRange;
  SRIndex block, toBeDeleted, firstEmptyBlock, lastEmptyBlock;
  SRRunArrayListIter firstEmptyIter, lastEmptyIter;

  if (newLength == 0) {
    newObject = nullptr; // �����Ǵ� �����
  }

  // ��ü�� ��ġ�� �� ȹ��
  block = blockForLocation(range.location, &blockRange);
  _length -= range.length;

  // ���� �������� ������ ũ�� ȹ��(��ũ�� �̳��� ����)
  // e.g, "aaa" -> blockRange(0, 3), range.location=1 -> toBeDeleted=2(��������� �ִ� �������� ����)
  // toBeDeleted(2) > range.length(1) -> toBeDeleted=1
  toBeDeleted = blockRange.length - (range.location - blockRange.location);
  if (toBeDeleted > range.length) {
    // ��������� �ִ� �������� ���� �̳��̸�, �ش� ���̸�ŭ�� �����ϰ� ��
    toBeDeleted = range.length;
  }

  // ���� �� ���̸� ���̰�, 0�� �Ǹ� ���� ���Ե� ��ü���� ������Ŵ
  if ((getRunArrayItem(block).length -= toBeDeleted) == 0) {
    getRunArrayItem(block).obj = nullptr;
  }
  range.length -= toBeDeleted; // �پ�� ���� �ݿ�

  // ó���� ���� �� ���� ��ġ�� ���� ����(�߰��� ���ԵǴ� ���� ������ ���)
  // ���� �� ���̰� 0�� �Ǹ� ���� ��, �ƴϸ� ���� ���� ����Ŵ
  // e.g, a[abbc]c -> firstEmptyBlock=1, lastEmptyBlock=1 -> aAc
  firstEmptyBlock = (getRunArrayItem(block).length == 0) ? block : block + 1;

  while (range.length) {
    // ���޹��� �������� ��� ���� ���� ����, �� ���� ��ü ����
    block++;
    SRRunArrayItem& item = getRunArrayItem(block);
    toBeDeleted = range.length;
    if (toBeDeleted >= item.length) toBeDeleted = item.length;
    if ((item.length -= toBeDeleted) == 0) {
      getRunArrayItem(block).obj = nullptr;
    }
    range.length -= toBeDeleted;
    SR_ASSERT(range.length >= 0);
  }
  SR_ASSERT(range.length == 0);

  // ó���� �� �� ���� ��ġ�� ���� ����(�߰��� ���ԵǴ� ���� ������ ���)
  // e.g, a[abbc]c -> firstEmptyBlock=1, lastEmptyBlock=1 -> aAc
  lastEmptyBlock = (block == 0 || getRunArrayItem(block).length == 0) ? block : block - 1;

  // �߰��� ������ ���ŵ� ������ �����ϴ� ���, ���Ե� ��ġ�� ���� ������ �õ�
  // firstEmptyBlock: ���� ���� ��(����� ���� ����)
  // lastEmptyBlock: ���� �� ��(����� ���� ��)
  if (firstEmptyBlock <= lastEmptyBlock) {
    if (newObject) {	/* See if the new object can be merged with one of the end blocks */
      // newObject �� ����, ���� ���� ������ ��� ���� �õ�
      /*
      [a]abbcc -> first=1, last=0 -> Aabbcc <-- N/A(0, 1, 2 �� ������ -> ������ �� ����)
      a[a]bbcc -> first=1, last=0 -> aAbbcc <-- N/A
      a[ab]bcc -> first=1, last=0 -> aAbcc  <-- N/A
      a[abb]cc -> first=1, last=1 -> aAcc   <-- first (O), last (O) �Ѵ� ���ɽ� first�� ����!(1 �� ����)
      a[abbc]c -> first=1, last=1 -> aAc    <-- first (O), last (O) �Ѵ� ���ɽ� first�� ����!
      a[abbcc] -> first=1, last=2 -> aA     <-- first (O), last (X) last�� �����Ƿ� last���� ���� �Ұ���(1, 2 �� ����)

      [aa]bbcc -> first=0, last=0 -> Bbbcc  <-- first (X), last (O)
      [aab]bcc -> first=0, last=0 -> Bbcc   <-- first (X), last (O)
      [aabb]cc -> first=0, last=1 -> Ccc    <-- first (X), last (O)
      [aabbc]c -> first=0, last=1 -> Cc     <-- first (X), last (O)
      [aabbcc] -> first=0, last=2 -> A      <-- first (X), last (X)

      a[abbc]cdd -> first=1, last=1 -> aAcdd     <-- first (O), last (O)

      first �� 0 �̸�, ������ �ƹ����� ���� �����!
      last �� ������ �������̸� ������ �� �������� ����
      */
      if ((firstEmptyBlock > 0) && ISSAME(getRunArrayItem(firstEmptyBlock - 1).obj, newObject)) {
        // ������ ���� �������� ���� �ְ�, ���� ���� ���� ��ü�̸� �������� ����
        // e.g, a[abb]cc -> first=1, last=1 -> aAcc (a, A �� ���յ�)
        getRunArrayItem(firstEmptyBlock - 1).length += newLength;
        _length += newLength;
    		newObject = nullptr; // ���յ����� ����
      } else if ((lastEmptyBlock + 1 < _numBlocks) && ISSAME(getRunArrayItem(lastEmptyBlock + 1).obj, newObject)) {
        // ������ ���� ���ĺ��� ���� �ְ�, ���� ���� ���� ��ü�̸� ���ĺ��� ����
        // e.g, [aab]bcc -> first=0, last=0, _numBlocks(3) -> Bbcc (B, b �� ���յ�)
        // e.g, a[abb]cc -> first=1, last=1 -> aCcc (C, c �� ���յ�)
        getRunArrayItem(lastEmptyBlock + 1).length += newLength;
        _length += newLength;
        newObject = nullptr; // ���յ����� ����
      }
    } else { // newObject == nullptr
      //SR_ASSERT(0); // SRDocumentView ������ ȣ��ǰ� ����
    }
    if (!newObject && (firstEmptyBlock > 0) && (lastEmptyBlock + 1 < _numBlocks)
	      && ISSAME(getRunArrayItem(firstEmptyBlock - 1).obj, getRunArrayItem(lastEmptyBlock + 1).obj)) {
      // newObject �� ������ �ư�(nullptr), ������ �� ����, ���ĺ��� �����ϸ� ����
      // e.g, a[abb]cc -> first=1, last=1 -> |aA|cc| (a, A �� ���յ�) -> |aAcc| (aA, cc �� ���յ�) first=1, last=2
		  lastEmptyBlock++;
		  getRunArrayItem(firstEmptyBlock - 1).length += getRunArrayItem(lastEmptyBlock).length;
      getRunArrayItem(lastEmptyBlock).obj = nullptr;
    }
    if (newObject && (firstEmptyBlock < _numBlocks)) {
      // newObject�� ���� ���Ե��� �ʾ�����, ù��° �� ���� �߰�
      // e.g, a[abbcc] -> first=1, last=2 -> aX, first=2, last=2
      getRunArrayItem(firstEmptyBlock).obj = newObject;
      getRunArrayItem(firstEmptyBlock).length = newLength;
      _length += newLength;
      firstEmptyBlock++;
      newObject = nullptr;
    }

    // �߰��� �� ������ ������Ų��.
    if (firstEmptyBlock <= lastEmptyBlock) {
      // e.g, aa[bb]cc -> first=1, last=1 -> aacc (�߰��� ���� ������ �ȵ� �����)
      SRRunArrayListIter delBegin = _list->begin() + firstEmptyBlock;
      SRRunArrayListIter delEnd = _list->begin() + lastEmptyBlock + 1;
      _list->erase(delBegin, delEnd);
      _numBlocks -= (lastEmptyBlock - firstEmptyBlock + 1);
    }
  }

  if (newObject) {
    // e.g, a[a]bbcc -> first=1, last=0 -> aXbbcc (������ ���� ���� ���)
    // e.g, aa[bb]cc -> first=1, last=1 -> aaXcc (�߰��� ���� ������ �ȵ� ���)
    insert(SRRange(range.location, newLength), newObject);
  }
}

void SRRunArray::test() {
  using PSTR = std::shared_ptr<std::wstring>;
  struct Node : public SRObject {
    Node(std::wstring str) {
      _pstr = std::make_shared<std::wstring>(str);
      SR_LOGD(L"[%p] Node(%s)", this, _pstr->c_str());
    }
    ~Node() {
      SR_LOGD(L"[%p] ~Node(%s)", this, _pstr->c_str());
    }
    bool isEqual(const SRObject& obj) const override {
      auto& other = static_cast<const Node&>(obj);
      //const Node* other = static_cast<const Node*>(&rhs);
      bool rtn = *_pstr == *(other._pstr);
      return rtn;
    }
    SRStringUPtr Node::toString() const {
      std::wostringstream ss;
      ss << std::hex << this << L" " << _pstr->c_str();
      return std::make_unique<SRString>(ss.str());
    }
    //std::wstring _str;
    PSTR _pstr;
  };
  SRRunArrayPtr arr = SRRunArray::create();
  arr->insert(SRRange(0, 2), std::make_shared<Node>(L"a"));
  arr->insert(SRRange(2, 2), std::make_shared<Node>(L"b"));
  arr->insert(SRRange(4, 2), std::make_shared<Node>(L"c"));
  arr->dump(); // aabbcc

  arr->replace(SRRange(2, 2), std::make_shared<Node>(L"c"), 1);
  arr->dump(); // aaccc
}

} // namespace sr
