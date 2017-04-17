#pragma once

// 기본 데이터 타입들 정의
//#include <math.h> // for sqrt
#include <limits> // std::numeric_limits<int>::max()
//#include <climits>

#ifdef min
#error "min defined!!!"
#undef min
#endif
#ifdef max
#error "max defined!!!"
#undef max
#endif

namespace sr {

// 기본타입(swift 타입이름 사용)
typedef unsigned char           SRBool;
//typedef unsigned char           UInt8;
//typedef signed char             Int8;
//typedef unsigned short          UInt16;
//typedef signed short            Int16;
typedef unsigned int            SRUInt32;
typedef signed int              SRInt32;
//typedef uint64_t		            UInt64;
//typedef int64_t		              Int64; // MSVC2010 compile error
typedef unsigned __int64		    SRUInt64;
typedef __int64		              SRInt64;

typedef float                   SRFloat32;
typedef double                  SRFloat64;

// SRInt == NSInteger: TODO When building 32-bit applications, NSInteger is a 32-bit integer. A 64-bit application treats NSInteger as a 64-bit integer.
#if (defined(__LP64__) && __LP64__) || defined(_WIN64)
// __LP64__: LP64 machine, OS X or Linux
// _WIN64: LLP64 machine, Windows
#error 64bit needs test!!!
typedef SRUInt64                SRUInt;
typedef SRInt64                 SRInt;
typedef SRFloat64               SRFloat;
#define SRFloatMin              DBL_MIN
#define SRFLOAT_MAX             DBL_MAX
#define SRIntMax                LONG_MAX
#define SRIntMin                LONG_MIN
#define SRUIntMax               ULONG_MAX
#else
// 32-bit machine, Windows or Linux or OS X
typedef SRUInt32                SRUInt;
typedef SRInt32                 SRInt;
typedef SRFloat32               SRFloat;
static const SRFloat  SRFloatMin  = std::numeric_limits<SRFloat>::min();  //FLT_MIN
static const SRFloat  SRFloatMax  = std::numeric_limits<SRFloat>::max();  //FLT_MAX
static const SRInt    SRIntMin    = std::numeric_limits<SRInt>::min();    //INT_MIN
static const SRInt    SRIntMax    = std::numeric_limits<SRInt>::max();    //INT_MAX
static const SRUInt   SRUIntMin   = std::numeric_limits<SRUInt>::min();   //UINT_MIN
static const SRUInt   SRUIntMax   = std::numeric_limits<SRUInt>::max();   //UINT_MAX
#endif

typedef SRInt                   SRIndex; // typedef long CFIndex;

//typedef unsigned short          unichar;
typedef wchar_t                 unichar;

//#define NSNotFound NSIntegerMax
static const SRInt SRNotFound = SRIntMax;

typedef struct SRRange {
  SRRange(SRIndex loc = 0, SRIndex len = 0);
  bool operator==(const SRRange& rhs) const {
    return location == rhs.location && length == rhs.length;
  }
  bool operator!=(const SRRange& rhs) const {
    return !operator==(rhs);
  }
  SRIndex rangeMax() const { // using max() makes name conflict!
    return location + length;
  }
  SRIndex location; // wstring::length() 등은 unsigned 타입이라서 unsigned 타입으로 변경함(-1 과 비교시 필요)
  SRIndex length;
} SRRange;

#ifdef __cplusplus
  struct SRPoint {
    SRFloat x, y;

    SRPoint() : x(0), y(0) {}
    SRPoint(SRFloat x, SRFloat y) : x(x), y(y) {}

    SRPoint operator+(const SRPoint& v) const {
      SRPoint ret = { x + v.x, y + v.y };
      return ret;
    }
    SRPoint operator+(SRFloat scalar) const {
      SRPoint ret = { x + scalar, y + scalar };
      return ret;
    }
    SRPoint& operator+=(const SRPoint& v) {
      x += v.x;
      y += v.y;
      return *this;
    }
    SRPoint& operator+=(SRFloat scalar) {
      x += scalar;
      y += scalar;
      return *this;
    }
    SRPoint operator-() const {
      SRPoint ret = { -x, -y };
      return ret;
    }
    SRPoint operator-(const SRPoint& v) const {
      SRPoint ret = { x - v.x, y - v.y };
      return ret;
    }
    SRPoint operator-(SRFloat scalar) const {
      SRPoint ret = { x - scalar, y - scalar };
      return ret;
    }
    SRPoint& operator-=(const SRPoint& v) {
      x -= v.x;
      y -= v.y;
      return *this;
    }
    SRPoint& operator-=(SRFloat scalar) {
      x -= scalar;
      y -= scalar;
      return *this;
    }
    bool operator==(const SRPoint& v) const {
      return x == v.x && y == v.y;
    }
    bool operator!=(const SRPoint& v) const {
      return x != v.x || y != v.y;
    }
    SRPoint operator*(SRFloat scalar) const {
      SRPoint ret = { x * scalar, y * scalar };
      return ret;
    }
    SRPoint& operator*=(SRFloat scalar) {
      x *= scalar;
      y *= scalar;
      return *this;
    }
    SRPoint operator/(SRFloat scalar) const {
      SRPoint ret = { x / scalar, y / scalar };
      return ret;
    }
    SRPoint& operator/=(SRFloat scalar) {
      x /= scalar;
      y /= scalar;
      return *this;
    }
    static inline SRPoint point(SRFloat x, SRFloat y) {
      SRPoint ret;
      ret.x = x;
      ret.y = y;
      return ret;
    }
  };
#else
#error "supported only __cplusplus"
  typedef struct SRPoint {
    SRFloat x;
    SRFloat y;
  } SRPoint;
#endif

  typedef struct SRSize {
    SRSize() : width(0), height(0) {}
    SRSize(SRFloat width, SRFloat height) : width(width), height(height) {}
    bool operator==(const SRSize& v) const {
      return width == v.width && height == v.height;
    }
    bool operator!=(const SRSize& rhs) const {
      return !operator==(rhs);
    }
    SRSize operator+(const SRSize& rhs) const {
      SRSize ret = *this;
      ret.width += rhs.width;
      ret.height += rhs.height;
      return ret;
    }
    SRSize operator+(SRFloat rhs) const {
      SRSize ret = *this;;
      ret += rhs;
      return ret;
    }
    SRSize& operator+=(const SRSize& rhs) {
      width += rhs.width;
      height += rhs.height;
      return *this;
    }
    SRSize& operator+=(SRFloat rhs) {
      width += rhs;
      height += rhs;
      return *this;
    }
    SRSize operator-(SRFloat rhs) const {
      SRSize ret = *this;;
      ret -= rhs;
      return ret;
    }
    SRSize& operator-=(SRFloat rhs) {
      width -= rhs;
      height -= rhs;
      return *this;
    }
    SRFloat width;
    SRFloat height;
  } SRSize;

  typedef struct SRRect {
    SRRect() {}
    SRRect(SRPoint origin, SRSize size) : origin(origin), size(size) {}
    SRRect(SRFloat x, SRFloat y, SRFloat width, SRFloat height) : origin(x, y), size(width, height) {}
    bool operator==(const SRRect& v) const {
      return origin == v.origin && size == v.size;
    }
    bool operator!=(const SRRect& v) const {
      return !operator==(v);
    }
    SRRect& operator+=(const SRRect& rhs);
    SRRect& operator+=(const SRFloat v);
    SRRect operator+(const SRRect& rhs) const;
    SRFloat left() const { return origin.x; }
    SRFloat top() const { return origin.y; }
    SRFloat right() const { return left() + width(); }
    SRFloat bottom() const { return top() + height(); }
    SRFloat width() const { return size.width; }
    SRFloat height() const { return size.height; }
    bool isEmpty() const { return (width() == 0 || height() == 0); }

    // translating horizontally by dx and vertically by dy.
    SRRect offsetBy(SRFloat dx, SRFloat dy) const;
    SRRect offsetBy(SRPoint pt) const;
    // moving each of horizontal sides inward by dy and each of vertical sides inward by dx.
    SRRect insetBy(SRFloat dx, SRFloat dy) const;
    SRRect insetBy(SRRect rc) const;
    // 겹치는 영역을 리턴해준다.
    SRRect intersection(SRRect r) const;
    bool intersects(SRRect r) const { return SRRect::intersects(*this, r); }
    bool containsPoint(SRPoint pt) const { return SRRect::containsPoint(*this, pt); }
    static bool intersects(SRRect a, SRRect b);
    static bool containsPoint(SRRect a, SRPoint b) {
      return intersects(a, SRRect(b, SRSize(1, 1)));
    }
    //static SRRect standardize(SRRect r);

    SRPoint origin;
    SRSize size;
  } SRRect;

  typedef unsigned short SRFontIndex; // An index into a font table.
  typedef SRFontIndex SRGlyph;        // An index into the internal glyph table of a font.

  typedef SRRect         SRPath; // @@TODO 임시로 SRRect 와 동일하게 처리함

  typedef struct SRTextPosition {
    SRTextPosition() {
      _offset = SRNotFound;
      _stickToNextLine = true;
    }
    SRPoint _xyMoving;        // 상하 라인 이동시 라인 길이에 상관없이 최초 좌표를 유지하기 위함
    SRIndex _offset;          // index into the backing string in a text-displaying view
    SRBool _stickToNextLine;  // TODO UITextInput.selectionAffinity 에 대응됨(UITextStorageDirection.forward, backward)
  } SRTextPosition;

  enum SB_Type {
    SR_SB_HORZ,
    SR_SB_VERT
  };
  enum SB_Code {
    SR_SB_NONE,
    SR_SB_LINEUP,
    SR_SB_LINELEFT,
    SR_SB_LINEDOWN,
    SR_SB_LINERIGHT,
    SR_SB_PAGEUP,
    SR_SB_PAGELEFT,
    SR_SB_PAGEDOWN,
    SR_SB_PAGERIGHT,
    SR_SB_THUMBTRACK,
    //SB_ENDSCROLL
  };
  typedef struct {
    SRInt pos;
    SRInt page;
    SRInt max;
    SRInt trackPos;
  } SRScrollInfo;

  typedef enum {
    SRWritingDirectionNatural = -1,
    SRWritingDirectionLeftToRight = 0,
    SRWritingDirectionRightToLeft = 1
  } SRWritingDirection;

  // @@todo Core Graphics CGPattern 로 적당한 헤더파일로 옮기자!
  typedef enum {
    SR_SOLID = 0,
    SR_DASH,
    SR_DOT,
    SR_PS_INSIDEFRAME,
  } SRPattern;

} // namespace sr
