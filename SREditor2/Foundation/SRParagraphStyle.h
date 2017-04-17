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

  // SRRunArray Ÿ���� ���� SRObject* �� ���ٽ� ��Ÿ�ӿ� ȣ����� �ʰ� �ǹǷ� ����
  virtual bool operator==(const SRObject& rhs) const;
  // ������ �θ�Ŭ������ operator==() �� ȣ��ǹǷ�, �θ�Ŭ������ ������ �����ص� �����ؾ� �Ѵ�.
  virtual bool operator!=(const SRObject& rhs) const;

// private:
  SRFloat _headIndent;
};

typedef std::shared_ptr<SRParagraphStyle> SRParagraphStylePtr;

} // namespace sr

#endif
#endif
