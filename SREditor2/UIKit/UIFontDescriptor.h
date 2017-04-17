#pragma once

#error "not used!!!"

#include "SRBase.h" //#include "SRObject.h"
#include "SRString.h"
#include "SRDictionary.h"

namespace sr {

  class UIFontDescriptor;
  typedef std::shared_ptr<UIFontDescriptor> UIFontDescriptorPtr;

  //extern const wchar_t* UIFontDescriptorNameAttribute;
  //extern const wchar_t* UIFontDescriptorSizeAttribute;
  //extern const wchar_t* UIFontDescriptorTraitsAttribute;
  extern const SRString UIFontDescriptorNameAttribute;
  extern const SRString UIFontDescriptorSizeAttribute;
  extern const SRString UIFontDescriptorTraitsAttribute;

  enum {
    /* Typeface info (lower 16 bits of UIFontDescriptorSymbolicTraits ) */
    UIFontDescriptorTraitItalic = 1u << 0,
    UIFontDescriptorTraitBold = 1u << 1,
    UIFontDescriptorTraitExpanded = 1u << 5,
    UIFontDescriptorTraitCondensed = 1u << 6,
    UIFontDescriptorTraitMonoSpace = 1u << 10,
    UIFontDescriptorTraitVertical = 1u << 11,
    UIFontDescriptorTraitUIOptimized = 1u << 12,
    UIFontDescriptorTraitTightLeading = 1u << 15,
    UIFontDescriptorTraitLooseLeading = 1u << 16,

    /* Font appearance info (upper 16 bits of UIFontDescriptorSymbolicTraits */
    //UIFontDescriptorClassMask = 0xF0000000,
    //UIFontDescriptorClassUnknown = 0u << 28,
    //UIFontDescriptorClassOldStyleSerifs = 1u << 28,
    //UIFontDescriptorClassTransitionalSerifs = 2u << 28,
    //UIFontDescriptorClassModernSerifs = 3u << 28,
    //UIFontDescriptorClassClarendonSerifs = 4u << 28,
    //UIFontDescriptorClassSlabSerifs = 5u << 28,
    //UIFontDescriptorClassFreeformSerifs = 7u << 28,
    //UIFontDescriptorClassSansSerif = 8u << 28,
    //UIFontDescriptorClassOrnamentals = 9u << 28,
    //UIFontDescriptorClassScripts = 10u << 28,
    //UIFontDescriptorClassSymbolic = 12u << 28,
  };
  typedef SRUInt32 UIFontDescriptorSymbolicTraits;

  struct VoidPtrEqual : public std::binary_function<std::shared_ptr<void>, std::shared_ptr<void>, bool> {
    bool operator()(std::shared_ptr<void> p1, std::shared_ptr<void> p2) const {
      if (p1 == p2) return true; // @@TODO 검증안됨!!!
      else return false;
    }
  };
  typedef SRDictionary<const SRString, SRStringEqual, std::shared_ptr<void>, VoidPtrEqual> SRFontAttrDictionary;
  //typedef SRDictionaryPtr<const SRStringPtr, SRObjectPtr>  SRFontAttrDictionaryPtr;

  // TODO 사전에 폰트속성들을 갖고 있어야 한다!!! UIFontDescriptorNameAttribute, UIFontDescriptorSizeAttribute 등등
  class UIFontDescriptor : public SRObject {
  public:
    UIFontDescriptor();
    //UIFontDescriptor(const SRFontAttrDictionary& attributes);
    UIFontDescriptor(const UIFontDescriptor& rhs);
    virtual ~UIFontDescriptor();

    virtual UIFontDescriptor& operator=(const UIFontDescriptor& rhs);
    //virtual bool operator==(const UIFontDescriptor& rhs) const;
    //virtual bool operator!=(const UIFontDescriptor& rhs) const;
    virtual SRUInt hash() const;

    SRFloat pointSize();
    UIFontDescriptorSymbolicTraits symbolicTraits();
    SRFontAttrDictionary& fontAttributes();

    static UIFontDescriptorPtr create();
    //static SRFloat getSystemFontSize();

    // Creating a Font Descriptor
    static UIFontDescriptorPtr fontDescriptorWithName(const SRString& fontName, SRFloat size);
    static UIFontDescriptorPtr fontDescriptorWithFontAttributes(const SRFontAttrDictionary& attributes);

    // Initializing a Font Descriptor
    static UIFontDescriptorPtr initWithFontAttributes(const SRFontAttrDictionary& attributes);

    // Querying a Font Descriptor
    SRObjectPtr objectForKey(const SRString& attribute);

  private:
    //SRString _name;
    //SRFloat _size;
    //UIFontDescriptorSymbolicTraits _traits;
    SRFontAttrDictionary _fontAttributes;
  };

} // namespace sr
