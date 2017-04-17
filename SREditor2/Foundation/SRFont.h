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

  // SRRunArray Ÿ���� ���� SRObject* �� ���ٽ� ��Ÿ�ӿ� ȣ����� �ʰ� �ǹǷ� ����
  virtual bool operator==(const SRObject& rhs) const;
  // ������ �θ�Ŭ������ operator==() �� ȣ��ǹǷ�, �θ�Ŭ������ ������ �����ص� �����ؾ� �Ѵ�.
  virtual bool operator!=(const SRObject& rhs) const;

// private:
  SRString _fontName;
};

typedef std::shared_ptr<SRFont> SRFontPtr;

} // namespace sr

#endif