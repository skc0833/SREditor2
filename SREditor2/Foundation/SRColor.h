#pragma once

#include "SRObject.h"

namespace sr {

class SRColor;
using SRColorPtr = std::shared_ptr<SRColor>;

class SRColor : public SRObject {
  //SR_MAKE_NONCOPYABLE(SRColor);
public:
  SR_DECL_CREATE_FUNC(SRColor);

  SRColor(SRFloat r = 0, SRFloat g = 0, SRFloat b = 0, SRFloat a = 1);
  SRColor(const SRColor& rhs);
  virtual ~SRColor() = default;

  virtual SRStringUPtr toString() const;

  SRFloat _r;
  SRFloat _g;
  SRFloat _b;
  SRFloat _a;

protected:
  virtual bool isEqual(const SRObject& obj) const override;
};

const SRColor SRColorGray(0.643f, 0.643f, 0.643f);
const SRColor SRColorLightGray(0.75f, 0.75f, 0.75f);
const SRColor SRColorWhite(1, 1, 1);
const SRColor SRColorBlack(0, 0, 0);
const SRColor SRColorRed(1, 0, 0);
const SRColor SRColorGreen(0, 1, 0);
const SRColor SRColorBlue(0, 0, 1);

} // namespace sr
