#pragma once

#include "SRObject.h"

/*
|a  b  0|
|c  d  0|
|tx ty 1|

x' = ax + cy + tx
y' = bx + dy + ty

* scale
|sx 0  0|
|0  sy 0|
|0  0  1|

x' = x * sx
y' = y * sy

* rotate
|cosa  sina  0|
|-sina cosa  0|
|0     0     1|

x' = x*cosa - ysina
y' = x*sina + ycosa

* translate
|1  0  0|
|0  1  0|
|tx ty 1|

x' = x + tx
y' = y + ty
*/

namespace sr {

//class CGAffineTransform;

class CGAffineTransform : public SRObject {
public:
  CGAffineTransform();
  virtual ~CGAffineTransform() = default;
  virtual SRStringUPtr toString() const;

  // tx, ty 만큼 이동
  void translatedBy(SRFloat tx, SRFloat ty);

  //SRFloat a; // reserved
  //SRFloat b; // reserved
  //SRFloat c; // reserved
  //SRFloat d; // reserved
  SRFloat tx;
  SRFloat ty;
};

} // namespace sr
