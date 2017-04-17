#error "not used!!!"

#include "UIFontDescriptor.h"
//#define SR_LOG_TAG  "TEST"
#include "SRLog.h"
#include "SRString.h"

namespace sr {

//const wchar_t* UIFontDescriptorNameAttribute = L"UIFontDescriptorNameAttribute";
//const wchar_t* UIFontDescriptorSizeAttribute = L"UIFontDescriptorSizeAttribute";
//const wchar_t* UIFontDescriptorTraitsAttribute = L"UIFontDescriptorTraitsAttribute";
const SRString UIFontDescriptorNameAttribute = L"UIFontDescriptorNameAttribute";
const SRString UIFontDescriptorSizeAttribute = L"UIFontDescriptorSizeAttribute";
const SRString UIFontDescriptorTraitsAttribute = L"UIFontDescriptorTraitsAttribute";

/**
All these values are fixed number in IOS no matter on iphone* or ipad*.
*/
static const SRFloat c_smallSystemFontSize = 12.0f;
static const SRFloat c_systemFontSize = 14.0f;
static const SRFloat c_labelFontSize = 17.0f;
static const SRFloat c_buttonFontSize = 14.0f;

UIFontDescriptor::UIFontDescriptor() {
}

//UIFontDescriptor::UIFontDescriptor(const SRFontAttrDictionary& attributes) {
//}

UIFontDescriptor::UIFontDescriptor(const UIFontDescriptor& rhs) {
  *this = rhs;
}

UIFontDescriptor::~UIFontDescriptor() {
}

UIFontDescriptor& UIFontDescriptor::operator=(const UIFontDescriptor& rhs) {
  SR_ASSERT(0);
  if (this == &rhs) {
    return *this;
  }
  return *this;
}

SRUInt UIFontDescriptor::hash() const {
  auto name = std::static_pointer_cast<SRString>(_fontAttributes.objectForKey(UIFontDescriptorNameAttribute, nullptr));
  auto size = std::static_pointer_cast<SRFloat>(_fontAttributes.objectForKey(UIFontDescriptorSizeAttribute, nullptr));
  auto traits = std::static_pointer_cast<UIFontDescriptorSymbolicTraits>(_fontAttributes.objectForKey(UIFontDescriptorTraitsAttribute, nullptr));
  size_t hashValue = std::hash<std::wstring>()(name->data());
  hashValue += std::hash<SRFloat>()(*size);
  hashValue += std::hash<UIFontDescriptorSymbolicTraits>()(*traits);
  return hashValue;
}

//static 
UIFontDescriptorPtr UIFontDescriptor::create() {
  auto ptr = std::make_shared<UIFontDescriptor>();
  return ptr;
}

//static
//SRFloat UIFontDescriptor::getSystemFontSize() {
//  return c_systemFontSize;
//}

//static
UIFontDescriptorPtr UIFontDescriptor::fontDescriptorWithName(const SRString& fontName, SRFloat size) {
  auto attributes = SRFontAttrDictionary::create();
  attributes->addValue(UIFontDescriptorNameAttribute, std::make_unique<SRString>(fontName));
  attributes->addValue(UIFontDescriptorSizeAttribute, std::make_unique<SRFloat>(size));
  return initWithFontAttributes(*attributes);
}

//static 
UIFontDescriptorPtr UIFontDescriptor::fontDescriptorWithFontAttributes(const SRFontAttrDictionary& attributes) {
  return initWithFontAttributes(attributes);
}

//static 
UIFontDescriptorPtr UIFontDescriptor::initWithFontAttributes(const SRFontAttrDictionary& attributes) {
  auto ptr = UIFontDescriptor::create();
  ptr->_fontAttributes = attributes;
  return ptr;
}

SRObjectPtr UIFontDescriptor::objectForKey(const SRString& attribute) {
  return std::static_pointer_cast<SRObject>(_fontAttributes.objectForKey(attribute, nullptr));
}

} // namespace sr
