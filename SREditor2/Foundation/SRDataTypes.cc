#include "SRBase.h"
#include "SRDataTypes.h"

namespace sr {

SRRange::SRRange(SRIndex loc, SRIndex len) {
  if (loc < 0) { loc = 0; }
  if (len < 0) { len = 0; }
  location = loc;
  length = len;

#if 0 // for test
  bool is = std::numeric_limits<int>::is_signed; // true
  int id = std::numeric_limits<int>::digits; // 31
  bool is2 = std::numeric_limits<unsigned int>::is_signed; // false
  int id2 = std::numeric_limits<unsigned int>::digits; // 32

  int imin = std::numeric_limits<int>::min();
  int imax = std::numeric_limits<int>::max();
  int imax2 = INT_MAX; // 2147483647
  SR_ASSERT(imax == imax2);
  SR_ASSERT(sizeof(int) == 4);
  unsigned int uimin = std::numeric_limits<unsigned int>::min();
  unsigned int uimax = std::numeric_limits<unsigned int>::max();
  unsigned int uimax2 = UINT_MAX; // 0xffffffff ==	4294967295
  SR_ASSERT(uimax == uimax2);
  SR_ASSERT(sizeof(unsigned int) == 4);
  long lmin = std::numeric_limits<long>::min();
  long lmax = std::numeric_limits<long>::max();
  long lmax2 = LONG_MAX; // 2147483647L
  SR_ASSERT(lmax == lmax2);
  SR_ASSERT(sizeof(long) == 4);
  long long llmin = std::numeric_limits<long long>::min();
  long long llmax = std::numeric_limits<long long>::max();
  long long llmax2 = LLONG_MAX; // 9223372036854775807i64
  SR_ASSERT(llmax == llmax2);
  SR_ASSERT(sizeof(long long) == 8);
  //int mip = INTPTR_MAX; // not defined!
  //int mi64 = INT64_MAX;
  //int mi32 = INT32_MAX;
#endif
}

SRRect& SRRect::operator+=(const SRRect& rhs) {
  *this = *this + rhs;
  return *this;
}

SRRect& SRRect::operator+=(const SRFloat v) {
  size.width += v;
  size.height += v;
  return *this;
}

SRRect SRRect::operator+(const SRRect& rhs) const {
  SRRect ret = *this;
  if (rhs.left() < left()) {
    ret.origin.x = rhs.left();
  }
  if (rhs.right() > right()) {
    ret.size.width = rhs.right() - left();
  }
  if (rhs.top() < top()) {
    ret.origin.y = rhs.top();
  }
  if (rhs.bottom() > bottom()) {
    ret.size.height = rhs.bottom() - top();
  }
  return ret;
}

SRRect SRRect::offsetBy(SRFloat dx, SRFloat dy) const {
  SRRect rect = *this;
  rect.origin.x += dx;
  rect.origin.y += dy;
  return rect;
}

SRRect SRRect::offsetBy(SRPoint pt) const {
  return offsetBy(pt.x, pt.y);
}

SRRect SRRect::insetBy(SRFloat dx, SRFloat dy) const {
  SRRect rect = *this;
  rect = rect.offsetBy(dx, dy);
  rect.size.width -= (2 * dx);
  rect.size.height -= (2 * dy);
  return rect;
}

SRRect SRRect::insetBy(SRRect rc) const {
  SRRect rect = *this;
  rect.origin.x += rc.left();
  rect.origin.y += rc.top();
  rect.size.width -= (rc.left() + rc.right());
  rect.size.height -= (rc.top() + rc.bottom());
  return rect;
}

//SRRect SRRect::standardize(SRRect r) {
//  SRRect out;
//
//  if (r.size.width < 0.0f) {
//    out.origin.x = r.origin.x + r.size.width;
//    out.size.width = -r.size.width;
//  } else {
//    out.origin.x = r.origin.x;
//    out.size.width = r.size.width;
//  }
//
//  if (r.size.height < 0.0f) {
//    out.origin.y = r.origin.y + r.size.height;
//    out.size.height = -r.size.height;
//  } else {
//    out.origin.y = r.origin.y;
//    out.size.height = r.size.height;
//  }
//  return out;
//}

// 겹치는지 여부를 리턴해준다.
// right, bottom 경계선은 포함시키지 않게 처리함
// the right and bottom edges of the rectangle are considered exclusive.
// e.g, {0, 0, 1, 1}, { 1, 1, 2, 2 } -> {0, 0, 0, 0}
bool SRRect::intersects(SRRect a, SRRect b) {
  //return !((b.origin.x > a.origin.x + a.size.width) || (b.origin.y > a.origin.y + a.size.height) ||
  //  (a.origin.x > b.origin.x + b.size.width) || (a.origin.y > b.origin.y + b.size.height));

  // origin 좌표는 음수일 수 있다(e.g, Hit Test시에 테이블 셀의 로컬 좌표로 변환시)
  if (!(/*a.left() >= 0 && a.top() >= 0 && */a.width() >= 0 && a.height() >= 0)) {
    SR_ASSERT(0);
  }
  if (!(/*b.left() >= 0 && b.top() >= 0 && */b.width() >= 0 && b.height() >= 0)) {
    SR_ASSERT(0);
  }
  return a.left() < b.right() && a.right() > b.left() && a.top() < b.bottom() && a.bottom() > b.top();
}

// 겹치는 영역을 리턴해준다.
SRRect SRRect::intersection(SRRect r) const {
  if (intersects(r)) {
    SRRect result = *this;
    if (result.left() < r.left()) result.origin.x = r.left();
    if (result.top() < r.top()) result.origin.y = r.top();
    //if (right() > r.right()) result.size.width = r.right() - left(); // error!!!
    //if (bottom() > r.bottom()) result.size.height = r.bottom() - top();
    if (r.right() < result.right()) result.size.width = r.right() - result.left();
    if (r.bottom() < result.bottom()) result.size.height = r.bottom() - result.top();
    SR_ASSERT(result.size.width >= 0 && result.size.height > 0);
    return result;
  } else {
    return SRRect();
  }
}

} // namespace sr
