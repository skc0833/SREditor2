#include "CTRunDelegate.h"

namespace sr {

CTRunDelegateCallbacks::CTRunDelegateCallbacks() {
  memset(this, 0, sizeof(*this));
}

CTRunDelegate::CTRunDelegate(const CTRunDelegateCallbacks& cb, SRObjectPtr ref)
  : _cb(cb), _ref(ref) {
  SR_ASSERT(_cb.dealloc);
  SR_ASSERT(_cb.getAscent);
  SR_ASSERT(_cb.getDescent);
  SR_ASSERT(_cb.getWidth);
  SR_ASSERT(_cb.onDraw);
}

bool CTRunDelegate::isEqual(const SRObject& other) const {
  if (this == &other) return true;
  else return false;
}

} // namespace sr
