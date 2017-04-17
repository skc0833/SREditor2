#pragma once

namespace sr {

typedef enum {
  NSLayoutRelationLessThanOrEqual = -1,
  NSLayoutRelationEqual = 0,
  NSLayoutRelationGreaterThanOrEqual = 1,
} UILayoutRelation;

typedef enum {
  NSLayoutAttributeLeft = 1,
  NSLayoutAttributeRight,
  NSLayoutAttributeTop,
  NSLayoutAttributeBottom,
  NSLayoutAttributeLeading,
  NSLayoutAttributeTrailing,
  NSLayoutAttributeWidth,
  NSLayoutAttributeHeight,
  NSLayoutAttributeCenterX,
  NSLayoutAttributeCenterY,
  NSLayoutAttributeBaseline,
  NSLayoutAttributeNotAnAttribute = 0
} UILayoutAttribute;

typedef SRFloat NSLayoutPriority;
const NSLayoutPriority NSLayoutPriorityRequired = 1000;

class UIView;
using UIViewPtr = std::shared_ptr<UIView>;

// A constraint defines a relationship between two user interface objects that must be satisfied
// by the constraint-based layout system.
// Each constraint is a linear equation with the following format:
// item1.attribute1 = multiplier ¡¿ item2.attribute2 + constant
typedef struct UILayoutConstraint {
  // The priority of the constraint.
  // By default, all constraints are required(NSLayoutPriorityRequired)
  NSLayoutPriority priority;
  
  // The first object participating in the constraint.
  UIViewPtr firstItem; // needs UIViewWeakPtr ???

  // The attribute of the first object participating in the constraint.
  UILayoutAttribute firstAttribute;

  // The relation between the two attributes in the constraint.
  UILayoutRelation relation;

  // The second object participating in the constraint.
  UIViewPtr secondItem;

  // The attribute of the second object participating in the constraint.
  UILayoutAttribute secondAttribute;

  // The multiplier applied to the second attribute participating in the constraint.
  SRFloat multiplier;

  // The constant added to the multiplied second attribute participating in the constraint.
  SRFloat constant;

  UILayoutConstraint(UIViewPtr view1, UILayoutAttribute attr1, UILayoutRelation relatedBy
    , UIViewPtr view2/*toItem*/, UILayoutAttribute attr2, SRFloat multiplier, SRFloat constant) {
    firstItem = view1;
    firstAttribute = attr1;
    relation = relatedBy;
    secondItem = view2;
    secondAttribute = attr2;
    multiplier = multiplier;
    constant = constant;
  }
} UILayoutConstraint;

} // namespace sr
