#if 0
#ifndef _SRParagraphStyle_H_
#define _SRParagraphStyle_H_

#include "SRBase.h" //#include "SRObject.h"
#include "SRString.h"

namespace sr {

class SRParagraphStyle : public SRObject {
 public:
  SRParagraphStyle(SRFloat headIndent = 0.0);
  SRParagraphStyle(const SRParagraphStyle& rhs);
  ~SRParagraphStyle();
  virtual void dump(const wchar_t* indent = L"");
  virtual SRStringUPtr toString();

  virtual SRParagraphStyle& operator=(const SRParagraphStyle& rhs);

  // SRRunArray 타입을 쓰면 SRObject* 로 접근시 런타임에 호출되지 않게 되므로 조심
  virtual bool operator==(const SRObject& rhs) const;
  // 없으면 부모클래스의 operator==() 가 호출되므로, 부모클래스와 구현이 동일해도 존재해야 한다.
  virtual bool operator!=(const SRObject& rhs) const;

// private:
  SRFloat _headIndent;
};

typedef std::shared_ptr<SRParagraphStyle> SRParagraphStylePtr;

} // namespace sr

#endif
#endif
