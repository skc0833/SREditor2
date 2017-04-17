#ifndef _SRFont_H_
#define _SRFont_H_

#include "SRObject.h"
#include "SRString.h"

namespace sr {

class SRFont : public SRObject {
 public:
  SRFont(SRString fontName = L"");
  SRFont(const SRFont& rhs);
  ~SRFont();
  virtual void dump(const wchar_t* indent = L"");
  virtual SRStringUPtr toString();

  virtual SRFont& operator=(const SRFont& rhs);

  // SRRunArray 타입을 쓰면 SRObject* 로 접근시 런타임에 호출되지 않게 되므로 조심
  virtual bool operator==(const SRObject& rhs) const;
  // 없으면 부모클래스의 operator==() 가 호출되므로, 부모클래스와 구현이 동일해도 존재해야 한다.
  virtual bool operator!=(const SRObject& rhs) const;

// private:
  SRString _fontName;
};

typedef std::shared_ptr<SRFont> SRFontPtr;

} // namespace sr

#endif