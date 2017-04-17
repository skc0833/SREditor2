#include "SRObject.h"
#include "SRString.h"
#include <sstream> // for wostringstream

#undef SR_LOG_FUNC
#define SR_LOG_FUNC()

namespace sr {

static SRUInt32 s_id;
static bool s_debug_on;

//static
SRObject::DebugInfo SRObject::_debugInfo;

#define SR_ALLOC_INC()   _debugInfo._obj_cnt++; if (s_debug_on) SR_LOGD(L"@@@--- alloc(+) %d", _debugInfo._obj_cnt);
#define SR_ALLOC_DEC()   _debugInfo._obj_cnt--; if (s_debug_on) SR_LOGD(L"@@@--- alloc(-) %d", _debugInfo._obj_cnt);
#define SR_ALLOC_INFO()  SR_LOGD(L"@@@--- total alloc %d", _debugInfo._obj_cnt);

SRObject::SRObject()
: _id(s_id++) {
  //const char* name = typeid(*this).name(); // 실제 클래스에 상관없이 항상 "class sr::SRObject"를 리턴
  //const char* raw_name = typeid(*this).raw_name(); // ".?AVSRObject@sr@@"
  //char* func = __FUNCTION__; // "sr::SRObject::SRObject"
  SR_LOG_FUNC();
  SR_ALLOC_INC();
}

SRObject::~SRObject() {
  SR_LOG_FUNC(); // __FUNCTION__ == "sr::SRObject::~SRObject"
  SR_ALLOC_DEC();
}

void SRObject::dump() const {
  SR_LOGD(L"id(%d) %s", id(), toString()->c_str());
}

SRStringUPtr SRObject::toString() const {
  std::wostringstream ss;
  ss << std::hex << this;
  return std::make_unique<SRString>(ss.str()); // 주소를 리턴시킴
}

bool SRObject::isEqual(const SRObject& obj) const {
  SR_ASSERT(0);
  if (this == &obj) return true;
  else return false;
}

SRUInt SRObject::hash() const {
  SR_ASSERT(0);
  return reinterpret_cast<SRUInt>(this);
}

// friend
bool operator==(const SRObject& lhs, const SRObject& rhs) {
  if (&lhs == &rhs) return true;
  // 동일 타입이 아니면 return false, 동일 타입일 경우에만 isEqual()를 호출하므로
  // isEqual() 함수에서는 static_cast 로 캐스팅해도 된다(dynamic_cast 는 type safety check 때문에 더 느림)
  return typeid(lhs) == typeid(rhs) // Allow compare only instances of the same dynamic type
    && lhs.isEqual(rhs);            // If types are the same then do the comparision.
}

// friend
bool operator!=(const SRObject& lhs, const SRObject& rhs) {
  return !(lhs == rhs);
}

} // namespace sr

#if 0 // equality test code
#include <iostream>
#include <string>
#include <typeinfo>
#include <vector>
#include <cassert>

class A {
  int _a;
public:
  A(int v) : _a(v) {}
protected:
  friend bool operator==(const A&, const A&);
  friend bool operator!=(const A&, const A&);
  virtual bool isEqual(const A& obj) const { return obj._a == _a; }
};

bool operator==(const A& lhs, const A& rhs) {
  return typeid(lhs) == typeid(rhs) // Allow compare only instances of the same dynamic type
    && lhs.isEqual(rhs);            // If types are the same then do the comparision.
}
bool operator!=(const A& lhs, const A& rhs) {
  return !(lhs == rhs);
}

class B : public A {
  int _b;
public:
  B(int v1) : A(v1), _b(v1) {}
  B(int v1, int v2) : A(v2), _b(v1) {}
protected:
  virtual bool isEqual(const A& obj) const override {
    // will never throw as isEqual is called only when (typeid(lhs) == typeid(rhs)) is true.
    auto& other = static_cast<const B&>(obj);
    return A::isEqual(other) && other._b == _b;
  }
};

class C : public B {
  int _c;
public:
  C(int v1, int v2, int v3) : B(v1, v2), _c(v3) {}
protected:
  virtual bool isEqual(const A& obj) const override {
    auto& other = static_cast<const C&>(obj);
    return B::isEqual(other) && other._c == _c;
  }
};

static void _test_equality() {
  // Some examples for equality testing
  A* p1 = new B(10);
  A* p2 = new B(10);
  assert(*p1 == *p2); // operator==(B&, B&)

  A* p3 = new B(10, 11);
  assert(!(*p1 == *p3));

  A* p4 = new B(11);
  assert(!(*p1 == *p4));

  A* p5 = new C(10, 11, 12);
  A* p6 = new C(10, 11, 12);
  assert(*p5 == *p6);
  assert(*p4 != *p5); // operator==(B&, C&)
}
#endif
