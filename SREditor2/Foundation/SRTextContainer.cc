#include "SRTextContainer.h"
#include "../UIKit/UITextView.h"
#include <algorithm>

namespace sr {

SRTextContainer::SRTextContainer(const SRSize& size)
 : _size(size) {
}

static void __validate_rect(SRRect rect) {
  if (!(rect.left() >= 0 && rect.top() >= 0 && rect.width() >= 0 && rect.height() >= 0)) {
    SR_ASSERT(0);
  }
}

void SRTextContainer::addExclusionPath(ExclusionPath path) {
  _exclusionPaths.push_back(path);
}

void SRTextContainer::drawExclusionPath(CGContextPtr ctx) const {
  //SRPoint textViewOrgInWindow = textView()->convertTo(SRPoint(0, 0));
  for_each(_exclusionPaths.begin(), _exclusionPaths.end(), [&ctx](ExclusionPath path) {
    path._func(ctx, path._obj);
  });
}

SRRect SRTextContainer::getTotalExclusionPath() const {
  SRRect result;
  for (auto it = _exclusionPaths.begin(); it != _exclusionPaths.end(); ++it) {
    result += it->_rect;
  }
  return result;
}

SRRect SRTextContainer::lineFragmentRect(SRRect proposedRect, SRUInt characterIndex
  , SRWritingDirection baseWritingDirection, SRRect* remainingRect) {
  const SRRect totalRect(0.0f, 0.0f, _size.width, _size.height);
  // ��û�� ������ �����̳��� ����� ��쵵 �����Ƿ� �����̳� ���� �̳��� ������Ų��.
  SRRect ret = totalRect.intersection(proposedRect);
  if (ret != proposedRect) {
    SR_ASSERT(0);
  }

#if 1 // exclude hole
  if (_exclusionPaths.size() == 0) {
    return ret;
  }

#if 1
  const SRRect orgRet = ret;
  //*remainingRect = proposedRect;
  *remainingRect = ret;
  remainingRect->size.width = 0;
  for (auto it = _exclusionPaths.begin(); it != _exclusionPaths.end(); ++it) {
    SRRect hole = it->_rect;
    // ���ϰ����� hole ������ ������ �� ������ ����
    if (orgRet.intersects(hole)) {
      if (hole.right() < orgRet.right()) {
        // remainingRect�� ������ ���� ������ ����
        remainingRect->origin.x = hole.right();
        remainingRect->size.width = orgRet.right() - hole.right();
        //remainingRect->size.width = orgRet.right() - hole.right() - remainingRect->left();
      }
      // hole�� ��ġ�� ���� ������ ����
      ret.size.width = std::max(0.0f, hole.left() - orgRet.left());
      break;
    }
  }
#else
  // TODO _exclusionPaths�� �������� ���, ó���� �ʿ���
  SRRect hole = _exclusionPaths[0]._rect;

  //for_each(_exclusionPaths.begin(), _exclusionPaths.end(), [&hole/*, &textViewOrgInWindow*/](ExclusionPath path) {
  //  hole += path._rect;
  //});
  const SRRect orgRet = ret;
  //*remainingRect = proposedRect;
  *remainingRect = ret;
  remainingRect->size.width = 0;
  // ���ϰ����� hole ������ ������ �� ������ ����
  if (orgRet.intersects(hole)) {
    if (hole.right() < orgRet.right()) {
      // remainingRect�� ������ ���� ������ ����
      remainingRect->origin.x = hole.right();
      remainingRect->size.width = orgRet.right() - hole.right();
      //remainingRect->size.width = orgRet.right() - hole.right() - remainingRect->left();
    }
    // hole�� ��ġ�� ���� ������ ����
    ret.size.width = std::max(0.0f, hole.left() - orgRet.left());
  }
#endif
#endif

  __validate_rect(ret);
  __validate_rect(*remainingRect);

#if 0
  if (_exclusionPaths != nil) {
    for (UIBezierPath* path in(NSArray*)_exclusionPaths) {
      SRRect shapeRect = _CGPathFitRect(path.SRPath, ret, _size, self.lineFragmentPadding);
      ret = CGRectIntersection(shapeRect, ret);
    }

    if (remainingRect) {
      *remainingRect = CGRectMake(ret.origin.x + ret.size.width + self.lineFragmentPadding,
        ret.origin.y,
        _size.width - ret.origin.x - ret.size.width,
        ret.size.height);
      for (UIBezierPath* path in(NSArray*)_exclusionPaths) {
        SRRect shapeRect = _CGPathFitRect(path.SRPath, *remainingRect, _size, self.lineFragmentPadding);
        *remainingRect = CGRectIntersection(shapeRect, *remainingRect);
      }
    }
  }
#endif

  return ret;
}

const UITextViewPtr SRTextContainer::textView() const {
  try {
    //SR_ASSERT(0); // ���� ������ �ʰ� ����
    auto ptr = _textView.lock();
    SR_ASSERT(ptr);
    return ptr;
  } catch (std::bad_weak_ptr b) {
    // �̹� delete �� �����Ϳ��� ��� �����Ѵ�.
    // std::weak_ptr refers to an already deleted object
    SR_ASSERT(0);
  }
  return nullptr;
}

} // namespace sr
