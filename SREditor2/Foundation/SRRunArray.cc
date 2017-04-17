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
  _list = std::make_shared<SRRunArrayList>(_numBlocks); // 0 은 무의미함?
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
      // 범위가 리스트 범위를 넘어서는 경우임
      SR_ASSERT(0);
    }
  }
  if (_list->size() == 0) { // 최초 insert 시에 발생함
    //SR_ASSERT(0);
  }
  SR_ASSERT(_list->size() == this->_numBlocks);
}

// index 위치의 블럭을 리턴
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
      // 다음 블럭으로 이동
      loc += itemLength;
      ++block;
    } else {
      // 0 위치 or 현재블럭 내에 포함되는 위치인 경우 리턴
      break;
    }
  }
  if (effectiveRange) {
    // 찾은 블럭 정보를 저장
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

// range.location 위치에 range.length 길이 만큼 추가함
void SRRunArray::insert(SRRange range, SRObjectPtr obj) {
  if (range.length == 0) {
    SR_ASSERT(0);
    return; // insert 할 객체가 없으므로 예외처리함
  }
  checkSanity(range.location);

  if (range.location == this->_length) {	// Append
    // 리스트 끝에 추가되는 경우
    if (this->_length > 0) {	// The list isn't empty
      // 빈 리스트가 아닌 경우
      if (ISSAME(obj, getRunArrayItem(this->_numBlocks - 1).obj)) {
        // 리스트 끝의 블럭과 같은 블럭인 경우, 마지막 블럭의 길이만 늘려줌
        SRRunArrayItem& item = getRunArrayItem(this->_numBlocks - 1);
        item.length += range.length;
      } else {
        // 리스트 끝의 블럭과 다른 블럭이면, 리스트 끝에 새로운 블럭을 추가함
        _list->push_back(RunArrayItem(range.length, obj));
        this->_numBlocks++;
      }
    } else {	// The list is empty...
      // 빈 리스트인 경우, 리스트에 추가함
      _list->push_back(RunArrayItem(range.length, obj));
      this->_numBlocks++;
    }
  } else {	// At this stage we are inserting, and the length of the list is > 0.
    // 리스트 중간에 삽입되는 경우
    SRRange blockRange;
    SRIndex block = blockForLocation(range.location, &blockRange);
    if (ISSAME(obj, getRunArrayItem(block).obj)) {
      // 삽입 위치 블럭(현재블럭)과 동일하면 해당 블럭의 길이만 증가
      getRunArrayItem(block).length += range.length;
    } else if ((block > 0) && (blockRange.location == range.location) && (ISSAME(obj, getRunArrayItem(block - 1).obj))) {
      // 블럭 시작 위치에 삽입되고, 바로 이전 블럭과 동일하면 이전 블럭의 길이만 증가
      getRunArrayItem(block - 1).length += range.length;
    } else if (blockRange.location == range.location) {
      // 블럭 시작 위치에 삽입되지만 이전, 현재 블럭과 다른 객체이면, 삽입 위치 이후 블럭들을 1만큼 오른쪽으로 shift
      _list->insert(_list->begin() + block, RunArrayItem(range.length, obj));
      _numBlocks++;
    } else {
      // 블럭 중간에 다른 객체가 삽입될 경우, 현재 블럭을 앞뒤로 나누고 블럭 길이를 조정
      SRIndex headLen = range.location - blockRange.location; // head -> 크기: range 시작 - blockRange 시작
      SRIndex tailLen = blockRange.length - headLen; // tail -> 크기: 원래 blockRange 크기에서 head 만큼 제외
      getRunArrayItem(block).length = headLen; // head
      _list->insert(_list->begin() + block + 1, RunArrayItem(range.length, obj)); // 삽입 객체(obj)
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
    // 삽입은 insert()로 처리. 이 함수에서 전부처리해도 될듯??? -> insert(SRRange(range.location, newLength), newObject);
    //SR_ASSERT(0);
    return;
  }
  SRRange blockRange;
  SRIndex block, toBeDeleted, firstEmptyBlock, lastEmptyBlock;
  SRRunArrayListIter firstEmptyIter, lastEmptyIter;

  if (newLength == 0) {
    newObject = nullptr; // 삭제되는 경우임
  }

  // 교체할 위치의 블럭 획득
  block = blockForLocation(range.location, &blockRange);
  _length -= range.length;

  // 현재 블럭내에서 삭제할 크기 획득(블럭크기 이내로 한정)
  // e.g, "aaa" -> blockRange(0, 3), range.location=1 -> toBeDeleted=2(현재블럭에서 최대 삭제가능 길이)
  // toBeDeleted(2) > range.length(1) -> toBeDeleted=1
  toBeDeleted = blockRange.length - (range.location - blockRange.location);
  if (toBeDeleted > range.length) {
    // 현재블럭에서 최대 삭제가능 길이 이내이면, 해당 길이만큼만 삭제하게 함
    toBeDeleted = range.length;
  }

  // 현재 블럭 길이를 줄이고, 0이 되면 블럭에 포함된 객체까지 삭제시킴
  if ((getRunArrayItem(block).length -= toBeDeleted) == 0) {
    getRunArrayItem(block).obj = nullptr;
  }
  range.length -= toBeDeleted; // 줄어든 길이 반영

  // 처리된 시작 블럭 다음 위치의 블럭을 저장(중간에 포함되는 블럭들 삭제시 사용)
  // 현재 블럭 길이가 0가 되면 현재 블럭, 아니면 다음 블럭을 가리킴
  // e.g, a[abbc]c -> firstEmptyBlock=1, lastEmptyBlock=1 -> aAc
  firstEmptyBlock = (getRunArrayItem(block).length == 0) ? block : block + 1;

  while (range.length) {
    // 전달받은 범위내의 모든 블럭들 길이 조절, 블럭 내의 객체 삭제
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

  // 처리된 끝 블럭 이전 위치의 블럭을 저장(중간에 포함되는 블럭들 삭제시 사용)
  // e.g, a[abbc]c -> firstEmptyBlock=1, lastEmptyBlock=1 -> aAc
  lastEmptyBlock = (block == 0 || getRunArrayItem(block).length == 0) ? block : block - 1;

  // 중간에 온전히 제거될 블럭들이 존재하는 경우, 삽입될 위치의 블럭과 병합을 시도
  // firstEmptyBlock: 삭제 시작 블럭(비워진 블럭의 시작)
  // lastEmptyBlock: 삭제 끝 블럭(비워진 블럭의 끝)
  if (firstEmptyBlock <= lastEmptyBlock) {
    if (newObject) {	/* See if the new object can be merged with one of the end blocks */
      // newObject 가 이전, 이후 블럭과 동일할 경우 병합 시도
      /*
      [a]abbcc -> first=1, last=0 -> Aabbcc <-- N/A(0, 1, 2 블럭 건재함 -> 삭제할 블럭 없음)
      a[a]bbcc -> first=1, last=0 -> aAbbcc <-- N/A
      a[ab]bcc -> first=1, last=0 -> aAbcc  <-- N/A
      a[abb]cc -> first=1, last=1 -> aAcc   <-- first (O), last (O) 둘다 가능시 first에 병합!(1 블럭 삭제)
      a[abbc]c -> first=1, last=1 -> aAc    <-- first (O), last (O) 둘다 가능시 first에 병합!
      a[abbcc] -> first=1, last=2 -> aA     <-- first (O), last (X) last가 없으므로 last에는 병합 불가능(1, 2 블럭 삭제)

      [aa]bbcc -> first=0, last=0 -> Bbbcc  <-- first (X), last (O)
      [aab]bcc -> first=0, last=0 -> Bbcc   <-- first (X), last (O)
      [aabb]cc -> first=0, last=1 -> Ccc    <-- first (X), last (O)
      [aabbc]c -> first=0, last=1 -> Cc     <-- first (X), last (O)
      [aabbcc] -> first=0, last=2 -> A      <-- first (X), last (X)

      a[abbc]cdd -> first=1, last=1 -> aAcdd     <-- first (O), last (O)

      first 가 0 이면, 이전에 아무블럭도 없는 경우임!
      last 가 마지막 블럭이전이면 마지막 블럭 이전블럭과 병합
      */
      if ((firstEmptyBlock > 0) && ISSAME(getRunArrayItem(firstEmptyBlock - 1).obj, newObject)) {
        // 삭제된 블럭의 이전블럭이 남아 있고, 이전 블럭과 같은 객체이면 이전블럭에 병합
        // e.g, a[abb]cc -> first=1, last=1 -> aAcc (a, A 가 병합됨)
        getRunArrayItem(firstEmptyBlock - 1).length += newLength;
        _length += newLength;
    		newObject = nullptr; // 병합됐음을 설정
      } else if ((lastEmptyBlock + 1 < _numBlocks) && ISSAME(getRunArrayItem(lastEmptyBlock + 1).obj, newObject)) {
        // 삭제된 블럭의 이후블럭이 남아 있고, 이후 블럭과 같은 객체이면 이후블럭에 병합
        // e.g, [aab]bcc -> first=0, last=0, _numBlocks(3) -> Bbcc (B, b 가 병합됨)
        // e.g, a[abb]cc -> first=1, last=1 -> aCcc (C, c 가 병합됨)
        getRunArrayItem(lastEmptyBlock + 1).length += newLength;
        _length += newLength;
        newObject = nullptr; // 병합됐음을 설정
      }
    } else { // newObject == nullptr
      //SR_ASSERT(0); // SRDocumentView 삭제시 호출되고 있음
    }
    if (!newObject && (firstEmptyBlock > 0) && (lastEmptyBlock + 1 < _numBlocks)
	      && ISSAME(getRunArrayItem(firstEmptyBlock - 1).obj, getRunArrayItem(lastEmptyBlock + 1).obj)) {
      // newObject 가 병합이 됐고(nullptr), 삭제된 블럭 이전, 이후블럭이 동일하면 병합
      // e.g, a[abb]cc -> first=1, last=1 -> |aA|cc| (a, A 가 병합됨) -> |aAcc| (aA, cc 가 병합됨) first=1, last=2
		  lastEmptyBlock++;
		  getRunArrayItem(firstEmptyBlock - 1).length += getRunArrayItem(lastEmptyBlock).length;
      getRunArrayItem(lastEmptyBlock).obj = nullptr;
    }
    if (newObject && (firstEmptyBlock < _numBlocks)) {
      // newObject가 아직 삽입되지 않았으면, 첫번째 빈 블럭에 추가
      // e.g, a[abbcc] -> first=1, last=2 -> aX, first=2, last=2
      getRunArrayItem(firstEmptyBlock).obj = newObject;
      getRunArrayItem(firstEmptyBlock).length = newLength;
      _length += newLength;
      firstEmptyBlock++;
      newObject = nullptr;
    }

    // 중간에 빈 블럭들을 삭제시킨다.
    if (firstEmptyBlock <= lastEmptyBlock) {
      // e.g, aa[bb]cc -> first=1, last=1 -> aacc (추가할 블럭이 병합이 안된 경우임)
      SRRunArrayListIter delBegin = _list->begin() + firstEmptyBlock;
      SRRunArrayListIter delEnd = _list->begin() + lastEmptyBlock + 1;
      _list->erase(delBegin, delEnd);
      _numBlocks -= (lastEmptyBlock - firstEmptyBlock + 1);
    }
  }

  if (newObject) {
    // e.g, a[a]bbcc -> first=1, last=0 -> aXbbcc (삭제할 블럭이 없는 경우)
    // e.g, aa[bb]cc -> first=1, last=1 -> aaXcc (추가할 블럭이 병합이 안될 경우)
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
