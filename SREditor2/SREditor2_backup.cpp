// SREditor2.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "SREditor2.h"

#define MAX_LOADSTRING 100

// ���� ����:
HINSTANCE hInst;								// ���� �ν��Ͻ��Դϴ�.
TCHAR szTitle[MAX_LOADSTRING];					// ���� ǥ���� �ؽ�Ʈ�Դϴ�.
TCHAR szWindowClass[MAX_LOADSTRING];			// �⺻ â Ŭ���� �̸��Դϴ�.

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

#include "./UiKit/UIWindow.h"
#include "./CoreText/CTParagraphStyle.h"
#include "./Win32/WIN32GraphicContextImpl.h"
#include "./Win32/WIN32WindowImpl.h"
#include <algorithm>

#define USE_UITextView

#ifdef USE_UITextView
//#include "./UiKit/UITextView.h"
#include "SRTextContainer.h"
#include "SRLayoutManager.h"
#include "SRDocumentView.h"
#endif

#define USE_SRTextTable
#ifdef USE_SRTextTable
#include "SRTextBlock.h"
#endif

#define USE_RunDelegate
#ifdef USE_RunDelegate
#include "./CoreText/CTRunDelegate.h"
#endif

using namespace sr;

const SRSize PAGE_MARGIN(20, 20);
const SRSize COLUMN_MARGIN(10, 10);

SRSize _minWindowSize() {
  SRSize size = PAGE_MARGIN + COLUMN_MARGIN;
  //size += COLUMN_MARGIN;
  size += 14; // @@ �ӽ÷� �ּ� ��Ʈ ũ��(14) �̻����� ������(���� 1�� ���� ���� �̻����� ����???)
  return size;
}

#ifdef USE_RunDelegate
class CTImageRunInfo;
using CTImageRunInfoPtr = std::shared_ptr<CTImageRunInfo>;

class CTImageRunInfo : public SRObject {
  SR_MAKE_NONCOPYABLE(CTImageRunInfo);
public:
  SR_DECL_CREATE_FUNC(CTImageRunInfo);

  CTImageRunInfo(const SRString& path, int w, int h) : _path(path), _width(w), _height(h) {}
  virtual ~CTImageRunInfo() {}

  int width() const { return _width; }
  int height() const { return _height; }
  SRString path() const { return _path; }

  static const SRString IMAGE_RUN_KEY;
  static CTImageRunInfoPtr ptr(SRObjectPtr ref) {
    SRObjectDictionaryPtr imgAttr = std::dynamic_pointer_cast<SRObjectDictionary>(ref);
    return std::dynamic_pointer_cast<CTImageRunInfo>(imgAttr->valueForKey(IMAGE_RUN_KEY));
  }

  static SRFloat imgGetAscentCallback(SRObjectPtr ref) { return ptr(ref)->height(); }
  static SRFloat imgGetDescentCallback(SRObjectPtr ref) { return 0; }
  static SRFloat imgGetWidthCallback(SRObjectPtr ref) { return ptr(ref)->width(); }
  static void imgDeallocCallback(SRObjectPtr ref) { }
  static void imgOnDrawCallback(SRObjectPtr ref, CGContextPtr ctx, SRPoint pt) {
    SRRect rc(pt, SRSize(ptr(ref)->_width, ptr(ref)->_height));
    ctx->drawImage(ptr(ref)->_path, rc);
  }

private:
  int _width;
  int _height;
  SRString _path;
};

//static 
const SRString CTImageRunInfo::IMAGE_RUN_KEY = L"imgInfo";

static void _insertImage(SRAttributedStringPtr attrStr, SRRange range, const SRString& imgPath
  , SRSize imgSize, CTParagraphStylePtr paragraphStyle) {
  CTRunDelegateCallbacks callbacks;
  callbacks.getAscent = CTImageRunInfo::imgGetAscentCallback;
  callbacks.getDescent = CTImageRunInfo::imgGetDescentCallback;
  callbacks.getWidth = CTImageRunInfo::imgGetWidthCallback;
  callbacks.dealloc = CTImageRunInfo::imgDeallocCallback;
  callbacks.onDraw = CTImageRunInfo::imgOnDrawCallback;

  SRObjectDictionaryPtr imgAttr = SRObjectDictionary::create();
  //imgAttr->addValue(L"width", 100); // @@todo ���� �⺻Ÿ���� SRObjectDictionary�� �Ұ����ϴ�!!!
  CTImageRunInfoPtr imgInfo = CTImageRunInfo::create(imgPath, imgSize.width, imgSize.height);
  imgAttr->addValue(CTImageRunInfo::IMAGE_RUN_KEY, imgInfo);

  CTRunDelegatePtr delegate = CTRunDelegate::create(callbacks, std::dynamic_pointer_cast<SRObject>(imgAttr));
  //SRObjectDictionaryPtr attrDictionaryDelegate = SRObjectDictionary::create();
  //attrDictionaryDelegate->addValue(kCTRunDelegateAttributeName, delegate);
  //attrDictionaryDelegate->addValue(kCTParagraphStyleAttributeName, paragraphStyle); // @@

  //add a space to the text so that it can call the delegate
  const SRObjectDictionaryPtr attribs = attrStr->attributes(range.location, nullptr);
  SRObjectDictionaryPtr newAttribs = SRObjectDictionary::create(*attribs); // �Ӽ��� �߰��ϹǷ� ��������� �Ѵ�.
  newAttribs->addValue(kCTRunDelegateAttributeName, delegate);
#if 1
  // range ��ġ�� �Ӽ��� CTRunDelegate ��ü�� �� �߰��ϴ� ���
  attrStr->replaceCharacters(range, SRAttributedString::create(L"x", newAttribs)); // OK!!!
#else
  attrStr->replaceCharactersInRange(range, SRString::create(L"x"));
  attrStr->setAttributes(newAttribs, SRRange(range.location, 1), false); // OK!!!
#endif
  //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));
}
#endif

//static const SRRect _rcClient(0, 0, 400, 300);

static UIWindowPtr _srWindow = nullptr;
static UIWindowPtr& SR() {
  //if (_srWindow == nullptr) {
  //  _srWindow = UIWindow::create(_rcClient);
  //  _srWindow->setBackgroundColor(SRColor(0.643f, 0.643f, 0.643f)); // ȸ��
  //}
  return _srWindow;
}

#ifdef USE_SRTextTable
static SRAttributedStringPtr tableCellAttributedString(const SRTextBlockList& parentBlocks
  , const SRString& string, SRTextTablePtr table
  , SRColor backgroundColor, SRColor borderColor, SRInt row, SRInt column) {
  SRTextTableBlockPtr tableBlock = SRTextTableBlock::create(table, row, 1, column, 1); // ���̺�(��)�� �� ���� ���Խ�Ŵ

  tableBlock->setBackgroundColor(backgroundColor);
  tableBlock->setBorderColor(borderColor);
  ////tableBlock->setWidth(4.0, SRTextTableBlock::Margin); // ���ʿ�???
  tableBlock->setWidth(1.0, SRTextTableBlock::Border); // html table �� ��� ���̺��� ��輱 �β��� 1 �̻��̸�, �� ��輱�� ������ 1 �β��� ����
  tableBlock->setWidth(1.0, SRTextTableBlock::Padding); // �� ��輱���κ����� �е�

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
  paragraphStyle->addTextBlock(tableBlock); // �� �� �߰�(������ ���̺�(��)�� ������)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

// �� ���� ���̺� ���� ������(������ ���̺� ���� ���������� ����)
static SRAttributedStringPtr tableAttributedString(const SRTextBlockList& parentBlocks) {
  SRAttributedStringPtr tableString = SRAttributedString::create(L""); // "\n\n"
  SRTextTablePtr table = SRTextTable::create();
  table->setNumberOfColumns(2);
  table->setNumberOfRows(2);
  ////table->setWidth(0.0, SRTextTableBlock::Margin); // ���ʿ�??? html table �Ӽ����� ���µ���
  table->setWidth(1.0, SRTextTableBlock::Border);
  table->setWidth(1.0, SRTextTableBlock::Padding); // html table cellspacing �� �ش���(�⺻ 2)

  {
    // ���̺� ������ �θ� ���� �ؽ�Ʈ �߰�(�ؽ�Ʈ �� �߰�)
    SRAttributedStringPtr str = SRAttributedString::create(L"@\n"); // "\n\n"
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ���� �߰�
    str->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
    //tableString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, tableString->getLength()));
  }

  // ���̺� ���鿡�� �θ� �� & �� ��(���̺�(��)�� ���Ե�) �Ӽ� �߰�
  tableString->append(tableCellAttributedString(parentBlocks, L"a\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 0));
  tableString->append(tableCellAttributedString(parentBlocks, L"b\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 1));
  //tableString->appendAttributedString(tableCellAttributedString(parentBlocks, L"b", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 2));
  tableString->append(tableCellAttributedString(parentBlocks, L"c\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 0));
  tableString->append(tableCellAttributedString(parentBlocks, L"d\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1));
  //tableString->appendAttributedString(tableCellAttributedString(parentBlocks, L"d", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 2));

  {
    // ���̺� ���Ŀ� �θ� ���� �ؽ�Ʈ �߰�
    SRAttributedStringPtr str = SRAttributedString::create(L"#");

    // default TextBlock �Ӽ� �߰�
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
    str->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
  }

#if 0 // for test
  if (1) {
    SRRange curAttributeRange = { 0 };
    CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
      tableString->attribute(kCTParagraphStyleAttributeName, 8, &curAttributeRange));
    SRTextBlockPtr block = *(paraStyle->getTextBlocks().begin());

    SRRange a = tableString->rangeOfTextBlock(block, 8);
    SRRange b = tableString->rangeOfTextBlock(block, 13);
    SRRange c = tableString->rangeOfTextBlock(block, 14);
    SRRange d = tableString->rangeOfTextBlock(block, 7);

    paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
      tableString->attribute(kCTParagraphStyleAttributeName, 26, &curAttributeRange));
    block = *(paraStyle->getTextBlocks().begin());
    a = tableString->rangeOfTextBlock(block, 26);
    int aaa = 0;
  }
#endif

  return tableString;
}

static SRAttributedStringPtr textListItemAttributedString(const SRTextBlockList& parentBlocks, const SRString& string, SRTextListPtr textList) {
  SRTextListBlockPtr block = SRTextListBlock::create(textList); // ����Ʈ(��)�� ����Ʈ ���� ���Խ�Ŵ

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
  paragraphStyle->addTextBlock(block); // �� �� �߰�(�� ���ȿ� ���̺�(��)�� ���Ե�)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static SRAttributedStringPtr textListAttributedString(const SRTextBlockList& parentBlocks, SRTextList::MarkerFormat markerFormat) {
  SRAttributedStringPtr attrString = SRAttributedString::create(L"@\n"); // "\n\n"
  //SRTextListPtr textList = SRTextList::create(SRTextList::CIRCLE);
  SRTextListPtr textList = SRTextList::create(markerFormat);

  // default TextBlock �Ӽ� �߰�
  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ���� �߰�

  // ���̺� ������ �θ� ���� �ؽ�Ʈ �߰�(�ؽ�Ʈ �� �߰�)
  attrString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrString->getLength()));

  // ���̺� ���鿡�� �θ� �� & �� ��(���̺�(��)�� ���Ե�) �Ӽ� �߰�
  attrString->append(textListItemAttributedString(parentBlocks, L"aa\n", textList)); // aa\nA\nB -> OK!
  attrString->append(textListItemAttributedString(parentBlocks, L"bb\n", textList));
  attrString->append(textListItemAttributedString(parentBlocks, L"cc\n", textList));

  {
    // ���̺� ���Ŀ� �θ� ���� �ؽ�Ʈ �߰�
    SRAttributedStringPtr str = SRAttributedString::create(L"#");

    // default TextBlock �Ӽ� �߰�
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
    str->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    attrString->append(str);
  }

#if 0 // for test
  if (1) {
    SRRange curAttributeRange = { 0 };
    CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
      attrString->attribute(kCTParagraphStyleAttributeName, 1, &curAttributeRange));
    SRTextBlockPtr block = paraStyle->getTextBlockList().at(1);
    SRTextListBlockPtr listBlock = std::dynamic_pointer_cast<SRTextListBlock>(block);
    SRRange a = attrString->rangeOfTextList(listBlock->getList(), 1);
    SRRange b = attrString->rangeOfTextList(listBlock->getList(), 2);
    //SRTextBlockPtr list = *(paraStyle->getTextBlocks().begin());
    //SRRange a = attrString->rangeOfTextBlock(list, 1);
    //SRRange b = attrString->rangeOfTextBlock(list, 2);
    int aaa = 0;
  }
#endif

  return attrString;
}
#endif

static const SRTextBlockList& getTextBlockList(const SRAttributedStringPtr parentAttrStr, SRUInt location) {
  CTParagraphStylePtr parentStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    parentAttrStr->attribute(kCTParagraphStyleAttributeName, location));
  return parentStyle->getTextBlockList();
}

// �Ӽ� ����� ���ڿ��� ������ ��������
static SRAttributedStringPtr SR_getAttrString() {
  SRAttributedStringPtr attrStr = SRAttributedString::create(
    //L"abcdeghijk.0123456789.��Ŭ��!!!Abcdeghijk.0123456789.ABcdeghijk.0123456789.ABCdeghijk.0123456789."
    //L"a\nb\r\ncd\r\ne"
    //L" "
    //L"abcde   ghijk\n123abcdeghijk.0123456789\nAbcdeghijk.0123456789.ABcdeghijk.abcde   ghijk\n123abcdeghijk.0123456789.Abcdeghijk.0123456789.ABcdeghijk" \
    L"abcde   ghijk\n123abcdeghijk.0123456789\nAbcdeghijk.0123456789.ABcdeghijk.abcde   ghijk\n123abcdeghijk.0123456789.Abcdeghijk.0123456789.ABcdeghijk" \
    L"abcde   ghijk\n123abcdeghijk.0123456789\nAbcdeghijk.0123456789.ABcdeghijk.abcde   ghijk\n123abcdeghijk.0123456789.Abcdeghijk.0123456789.ABcdeghijk"
    //L"AAA\n@aabbg@11223344gddBBB"
    L"AabcdB"
    //L"ab\nc"
  ); // @@table

#ifdef USE_SRTextTable
#if 0
  attrStr = tableAttributedString(nullptr);
  SRRange replaceRange(1, 1);
  // 1) ���ڿ��κ��� ����
  SRStringPtr str = SRString::create(L"AAA\naa");
  // 2) AttributedString �κ��� ����
  //SRAttributedStringPtr str = SRAttributedString::create(L"AAA");
  //SRRange curAttributeRange = { 0 };
  //CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
  //  attrStr->attribute(kCTParagraphStyleAttributeName, replaceRange.location, &curAttributeRange));
  //str->addAttribute(kCTParagraphStyleAttributeName, paraStyle, SRRange(0, str->getLength()));
  attrStr->replaceCharactersInRange(replaceRange, str);
#else
  SRTextBlockPtr rootBlock = SRTextBlock::create(SRTextBlock::Text);
  rootBlock->setWidth(0.0, SRTextTableBlock::Margin);
  rootBlock->setWidth(1.0, SRTextTableBlock::Border);
  rootBlock->setWidth(10.0, SRTextBlock::Padding);
  SRTextBlockList rootBlocks;
  rootBlocks.push_back(rootBlock);

#if 0 // �ؽ�Ʈ�� �߰�
  //attrStr = SRAttributedString::create(L"a");
  //attrStr = SRAttributedString::create(L"a\nb\nc\nd\ne\na\nb\nc\nd\ne\na\nb\nc\nd\ne\n");
  attrStr = SRAttributedString::create(L"a\nb\nc\nd\ne\nA\nb\nc\nd\ne\n");
  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(rootBlocks);
  attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));
#else
  attrStr = tableAttributedString(rootBlocks);
  //attrStr = textListAttributedString(rootBlocks, SRTextList::CIRCLE);

  SRAttributedStringPtr parentAttrStr = attrStr;

#if 1
  // �θ� ���ڿ� replaceRange.location ��ġ�� �ڽ� ���ڿ� �߰�(�ش� ��ġ �ؽ�Ʈ ���� �θ� ������ ������)
  if (1) {
    SRRange replaceRange(2, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // �ڽ� ���̺� �߰�
    SRAttributedStringPtr childTableAttrStr = tableAttributedString(parentTextBlocks);
    parentAttrStr->replaceCharacters(replaceRange, childTableAttrStr);
  }
//#else
  if (1) {
    SRRange replaceRange(5, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // �ڽ� ����Ʈ �߰�
    parentAttrStr->replaceCharacters(replaceRange, textListAttributedString(parentTextBlocks, SRTextList::DECIMAL));
  }
  if (1) {
    SRRange replaceRange(7, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // �ڽ� ���̺� �߰�
    SRAttributedStringPtr childTableAttrStr = tableAttributedString(parentTextBlocks);
    parentAttrStr->replaceCharacters(replaceRange, childTableAttrStr);
  }
#endif
#endif
#endif
#endif

#ifdef USE_RunDelegate
  if (1) {
    // @@@ ���⼭�� ������ ���, addTextBlockList() ȣ���� ���ϸ� ���� �߻���!!!
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(rootBlocks);
    ////attrStr->appendAttributedString(SRAttributedString::create(L"ABCDE12345abcde"));
    //attrStr->appendAttributedString(SRAttributedString::create(L"bc")); // @@@ ���ڿ��� �߰��� ���� ������ �Ӽ��� ��ӹ�����???
    //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));
    //_insertImage(attrStr, SRRange(3, 1), L".\\test.bmp", SRSize(100, 100), paragraphStyle);
    //_insertImage(attrStr, SRRange(4, 0), L".\\test1.bmp", SRSize(30, 30), paragraphStyle);
    for (int i = 0; i < 2; ++i) {
      //_insertImage(attrStr, SRRange(4 + i * 2, 0), L".\\test.bmp", SRSize(100, 100), paragraphStyle);
      if (i % 2 == 0) {
        _insertImage(attrStr, SRRange(20 + i * 2, 0), L".\\test.bmp", SRSize(70, 70), paragraphStyle);
      } else {
        _insertImage(attrStr, SRRange(20 + i * 2, 0), L".\\test1.bmp", SRSize(30, 30), paragraphStyle);
      }
    }
    //_insertImage(attrStr, SRRange(2, 1), L".\\test.bmp", SRSize(50, 50), paragraphStyle);
    //_insertImage(attrStr, SRRange(6, 2), L".\\test.bmp", SRSize(100, 150), paragraphStyle);
    //_insertImage(attrStr, SRRange(8, 1), L".\\test.bmp", SRSize(100, 100), paragraphStyle);
  }
#endif

#if 1
  //UIFontPtr font = UIFont::fontWithName(L"Arial", 36.0); // 12.0
  UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // ������ ��Ʈ(28 -> ������ 20, 14 -> 20)
  //UIFontPtr font = UIFont::fontWithName(L"Symbol", 14.0); // bullet
  attrStr->addAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  if (1) {
    // �κ� ��Ʈ �Ӽ� ����
    SRRange currentRange(2, 1);
    UIFontPtr font = UIFont::fontWithName(L"Arial", 24.0); // 12.0
    attrStr->addAttribute(kCTFontAttributeName, font, currentRange);
    attrStr->addAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(2, 3));
  }
#else
  //UIFontPtr font = UIFont::fontWithName(L"Arial", 36.0); // 12.0
  UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // ������ ��Ʈ(28 -> ������ 20)
  attrStr->addAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  std::wstring str = attrStr->getString()->data();
  std::wstring::size_type strOffset = 0;

  if (1) {
    // paragraph �Ӽ� ����
    CTTextAlignment alignment = kCTCenterTextAlignment; //kCTRightTextAlignment; //kCTJustifiedTextAlignment;
    CTParagraphStyleSetting _settings[] = { { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment } };
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::create(_settings, sizeof(_settings) / sizeof(_settings[0]));

    // set paragraph style attribute
    //CFAttributedStringSetAttribute(attrStr, CFRangeMake(0, CFAttributedStringGetLength(attrStr)), kCTParagraphStyleAttributeName, paragraphStyle);
    // @@TODO ��ü ���� �Ӽ��� �����ϰ�, �κ������� ��������!
    //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, 16/*attrStr->getLength()*/));
    strOffset = str.find('\n', strOffset); // 1st paragraph -> �⺻���� left
    SRRange range;
    range.location = ++strOffset; // 2nd paragraph -> center �� ����
    strOffset = str.find('\n', strOffset);
    range.length = (strOffset - range.location);
    if (strOffset != std::wstring::npos) attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);
  }
  if (1) {
    // paragraph �Ӽ� ����
    //    create paragraph style and assign text alignment to it
    CTTextAlignment alignment = kCTRightTextAlignment; //kCTJustifiedTextAlignment;
    CTParagraphStyleSetting _settings[] = { { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment } };
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::create(_settings, sizeof(_settings) / sizeof(_settings[0]));

    // set paragraph style attribute
    //strOffset = str.find('\n', strOffset); // 3rd paragraph -> right
    SRRange range;
    range.location = ++strOffset; // 3rd paragraph -> right �� ����
    strOffset = str.find('\n', strOffset);
    range.length = (strOffset - range.location);
    if (strOffset != std::wstring::npos) attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);
    //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(4, 3));
  }

  if (1) {
    // �κ� ��Ʈ �Ӽ� ����
    SRRange currentRange(1, 2);
    UIFontPtr font = UIFont::fontWithName(L"Arial", 12.0); // 12.0
    attrStr->addAttribute(kCTFontAttributeName, font, currentRange);
  }
  //attrStr->addAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(2, 2));
  attrStr->addAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(1, 2));
#endif

  return attrStr;
}

static void SR_positionCenter() {
  if (SR()->contentView() == nullptr) {
    return;
  }
  UIViewPtr docView = SR()->contentView();
  SRRect rcWindow = SR()->getBounds();
  SRPoint center = SR()->getCenter();
  center.x = rcWindow.origin.x + rcWindow.size.width / 2;
  docView->setCenter(center); // �θ��(������) �������� center ��ǥ�� �����Ѵ�.
}

#ifdef USE_UITextView
static int _columnCnt = 1;
//static SRSize _pageSize(300, 100);
static int __test_col_idx = 0;

static void SR_addColumnView(UIWindowPtr window, SRLayoutManagerPtr layoutMgr, int columnCnt, SRRect drawRect) {
  const int COLUMN_CNT = columnCnt;

  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window->contentView());
  UIViewList pageViews = docView->getSubviews();
  SRPageViewPtr lastPageView;
  UIViewList columnViews;

  static SRRect aaa;
  if (__test_col_idx == 0) {
    aaa = window->getBounds();
  }
  const SRSize bbb = window->getBounds().size;
  if (aaa.size != bbb) {
    int a = 0;
  }

  SRRect pageRect = window->getBounds().insetBy(PAGE_MARGIN.width, PAGE_MARGIN.height);
  //pageRect.size.width = aaa.size.width; // @@test
  SRSize minSize = _minWindowSize();
  if (pageRect.size.width < minSize.width) {
    pageRect.size.width = minSize.width;
  }
  if (pageRect.size.height < minSize.height) {
    pageRect.size.height = minSize.height;
  }

  if (pageViews.size() > 0) {
    lastPageView = std::dynamic_pointer_cast<SRPageView>(*(pageViews.rbegin()));
    SRRect lastPageRect = lastPageView->getFrame();
    pageRect.origin.y = lastPageRect.bottom() + PAGE_MARGIN.height;

    columnViews = lastPageView->getSubviews();
    SR_ASSERT(columnViews.empty() || columnViews.size() <= lastPageView->_column);
  }

  // ������ ũ�⿡�� �翷�� �÷� ������ �� ���� ũ�⸦ ����
  SRRect columnRect = pageRect.insetBy(COLUMN_MARGIN.width, COLUMN_MARGIN.height);
  // �÷� ������ ������ ���� �÷� ���� - 1 ��ŭ ���� ����
  columnRect.size.width -= (COLUMN_CNT - 1) * COLUMN_MARGIN.width;
  columnRect.size.width /= COLUMN_CNT;
  columnRect.origin = SRPoint(COLUMN_MARGIN.width, COLUMN_MARGIN.height);

  if (columnViews.empty() || columnViews.size() == lastPageView->_column) {
    // �÷��� ���ų� ���� ���, ������ �߰�
    SRPageViewPtr pageView = SRPageView::create(pageRect, COLUMN_CNT);
    //pageView->setBackgroundColor(SRColor(0, 0, 1));
    docView->addSubview(pageView);
    //pageView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

    lastPageView = pageView;

    // ���� �������� ù��° �÷� ������ �����ϰ� ����
    //SRColumnViewPtr firstColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.begin()));
    //columnRect = firstColumnView->getFrame();
  } else {
    // �� ��° ���� �÷��� �ٷ� ���� �÷��� ������ �������� ����
    SRColumnViewPtr lastColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.rbegin()));
    columnRect = lastColumnView->getFrame();
    columnRect.origin.x = columnRect.left() + columnRect.width();
    columnRect.origin.x += COLUMN_MARGIN.width; // �÷� ������ŭ ������ ���
  }

  static SRTextContainerWeakPtr _firstTextContainer;
  //if (columnViews.empty()) {
  if (layoutMgr) {
    // ���ʿ� �ѹ��� �����̳ʸ� �߰���Ŵ(����¡�� ��ü�� ���̾ƿ��� ������ ũ��� �ڸ��� ��)
    SRSize containerSize(columnRect.size);
    SRTextContainerPtr textContainer = SRTextContainer::initWithSize(containerSize);
    layoutMgr->addTextContainer(textContainer);
    _firstTextContainer = textContainer;
  }

  // �÷��� �߰�
  SRColumnViewPtr columnView = SRColumnView::create(columnRect);
  //columnView->init(textContainer);
  columnView->init(_firstTextContainer.lock());
  if (drawRect.size.width == 0) {
    drawRect.size = columnRect.size; // ���ʿ� �÷� ũ��� �����ϱ� ������
  }
  columnView->setDrawRect(drawRect);
  //columnView->setBackgroundColor(SRColor(0, 1, 0));
  //columnView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight
  lastPageView->addSubview(columnView);
  //columnView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

  //// �ϴ� ������ ���� �߰�
  //if (false || docView->getSubviews().size() > 1) {
  //  SRRect docBounds = docView->getBounds();
  //  docBounds.size.height += PAGE_MARGIN.height;
  //  docView->setBounds(docBounds);
  //}
}

class LayoutMgrDelegate : public SRLayoutManagerDelegate {
public:
  LayoutMgrDelegate(UIWindowPtr window) : _window(window) {}
  virtual void layoutManagerDidInvalidateLayout(SRLayoutManagerPtr sender) {
    //SR_ASSERT(0);
    // SRLayoutManager::invalidateDisplay()���� ȣ���
    //UITextView::layoutManagerDidInvalidateLayout()
    //  [self _adjustTextLayerSize : FALSE/*setContentPos*/]; // ���� ���̾ƿ��� ũ�� ȹ��
    //  [self setNeedsDisplay];
    //UITextView::_adjustTextLayerSize:(BOOL)setContentPos {
    //    SRRect allText = [_layoutManager usedRectForTextContainer:_textContainer]; // ���� ���̾ƿ��� ũ�� ȹ��
    //    [self setNeedsDisplay]; // ȭ�� ���� ����
    //    [self setContentSize:size]; // ũ�� ������ ������, setContentSize()�� ũ�⸦ ���� ������
    //(SRRect)LM::usedRectForTextContainer:(NSTextContainer*)tc
    //  [self layoutIfNeeded]; // ���⼭ _layoutAllText()�� ȣ���
    //  return 0, 0, _totalSize
    //(void)LM::layoutIfNeeded {
    //  if (_needsLayout) �̸�, [self _layoutAllText]; ��, _totalSize �� SRRect ����
  }
  virtual void didCompleteLayoutFor(SRLayoutManagerPtr sender, SRTextContainerPtr container, bool atEnd) {
    if (atEnd) {
      SRTextStoragePtr textStorage = sender->textStorage();
      int filledLength = container->_textRange.location + container->_textRange.length;
      SR_ASSERT(filledLength <= textStorage->getLength());

#if 1
      SRRect usedRect = sender->usedRectForTextContainer(container);
      SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window()->contentView());
      SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(docView->getSubviews().at(0));
      SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));
      SRRect bounds = columnView->getBounds();
      SRRect frame = columnView->getFrame();
      bounds.size = usedRect.size;
      // �÷���(UITextView) ũ�⿡ ��������� ������, �������� ���� ������ ������� �ִ�!!!
      // ������, ������ ũ��� ������� �ʰ� �ִ�.
      //columnView->setBounds(bounds);
#endif

      if (filledLength >= textStorage->getLength()) {
        // ���̾ƿ� �Ϸ�
        //return;
      }

      //docView->paginate(SRSize(300, 50));
      //docView->paginate(frame.size);
      docView->paginate(frame.size, usedRect.size);
      //columnView->setDrawRect(docView->getPage(0)); // ù��° �÷��� �̹� ������ �����̹Ƿ� �俵���� ������

      //// ù��° �������� ũ�⸸ ������(���⿡ ���� ���̾ƿ��� �ִ�)
      //SRRect pageRect = SR()->getBounds().insetBy(PAGE_MARGIN.width, PAGE_MARGIN.height);
      //SRRect a = docView->getFrame();
      //pageView->setFrame(pageRect);
      //columnView->setDrawRect(docView->getPage(0));

      UIViewList& pageViews = docView->getSubviews();
      pageViews.clear();

      if (SR() != window()) {
        int a = 0;
      }

      for (int i = 0; i < docView->getPageCount(); ++i) {
        SRRect drawRect = docView->getPage(i);
        __test_col_idx = i;
        //SR_addColumnView(window(), sender, _columnCnt, drawRect);
        SR_addColumnView(window(), nullptr, _columnCnt, drawRect);
      }
      // �ϴ� ������ ���� �߰�
      if (docView->getSubviews().size() > 1) {
        SRRect docBounds = docView->getBounds();
        docBounds.size.height += PAGE_MARGIN.height;
        docView->setSize(docBounds.size);
      }
      SR()->updateScrollBar(); // �ڽĺ�� �߰��� ������ ��ũ�� ������ ��������� ��ũ�ѹ� ��ġ�� �°� �ȴ�.
    }
  }
  UIWindowPtr window() const {
    return _window.lock();
  }
  UIWindowWeakPtr _window;
};
using LayoutMgrDelegatePtr = std::shared_ptr<LayoutMgrDelegate>;
#endif

static void test_vector() {
  struct A {
    A(int a) : _a(a) {
      int b = 0;
    }
    ~A() {
      int b = 0;
    }
    A(const A& rhs) {
      _a = rhs._a;
    }
    A& operator=(const A& rhs) {
      _a = rhs._a;
    }
    int _a;
  };
  std::vector<A> v1;
  v1.reserve(2); // �̷��� �����ָ� A(111) �� �ѹ� �� ����ȴ�(���� �޸� �̵�����???)
  v1.push_back(A(111)); // ������1, ���������1, �Ҹ���1
  v1.push_back(A(222)); // ������2, ���������2, �Ҹ���2
  v1.clear(); // ����Ҹ���1, 2
  int b = 0;
}

static void test_vector_ptr() {
  struct A {
    A(int a) : _a(a) {
      int b = 0;
    }
    ~A() {
      int b = 0;
    }
    A(const A& rhs) {
      _a = rhs._a;
    }
    A& operator=(const A& rhs) {
      _a = rhs._a;
    }
    int _a;
  };
  typedef std::shared_ptr<A> APtr;
  std::vector<APtr> v1;
  v1.reserve(2); // �̷��� �����ָ� A(111) �� �ѹ� �� ����ȴ�(���� �޸� �̵�����???)
  APtr p1 = std::make_shared<A>(111);
  APtr p2 = std::make_shared<A>(222);
  v1.push_back(p1);
  v1.push_back(p2);
  v1.clear();
  int b = 0;
  // p1, p2 �Ҹ�� ~A() ȣ���!!!
}

static SRTextStoragePtr _textStorage;

// WM_CREATE ������ ȣ������
static void SR_create(void* hwnd) {
  RECT rc;
  ::GetClientRect((HWND)hwnd, &rc);
  SRRect windowRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top); // ó������ ������ ũ��� ������ ũ��� CTDocView�� ������

  //test_vector_ptr();

  if (0) {
    if (0) {
      SRRunArray::test();
      return;
    }
    int ocnt = SRObject::debugInfo()._obj_cnt; // 16
    UIFont::test();
    SRAttributedString::test(hwnd, rc.right - rc.left, rc.bottom - rc.top);
    int ocnt2 = SRObject::debugInfo()._obj_cnt; // 21(static UIFont::_fontCache ������)
    int a = 0;
    //return;
  }

  if (_srWindow) {
    //_srWindow->removeSubviews();
    int ocnt = SRObject::debugInfo()._obj_cnt;
    int rcnt = g_render_cnt();
    _srWindow = nullptr; // @@todo �Ҹ��ڰ� ȣ����� �ʰ� �ִ�!!!
    int ocnt2 = SRObject::debugInfo()._obj_cnt;
    int rcnt2 = g_render_cnt();
    int a = 0;
  }

  _srWindow = UIWindow::create(windowRect, WIN32WindowImpl::create(hwnd));
  SR()->setBackgroundColor(SRColor(0.643f, 0.643f, 0.643f)); // ȸ��

#ifdef USE_UITextView
  // https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/DrawingStrings.html
  //SRTextStoragePtr *textStorage = SRTextStorage::initWithString();
  //initWithString:@"This is the text string."];
  //NSLayoutManager *layoutManager = [[NSLayoutManager alloc] init];
  //NSTextContainer *textContainer = [[NSTextContainer alloc] init];
  //[layoutManager addTextContainer : textContainer];
  //[textContainer release];
  //[textStorage addLayoutManager : layoutManager];
  //[layoutManager release];

  //SRTextStoragePtr textStorage = SRTextStorage::initWithString(L"");
  _textStorage = SRTextStorage::initWithString(L"");
  //_textStorage->setAttributedString(SR_getAttrString()); // SRTextStorage::initWithAttributedString() �Լ��� �߰��ұ�???
  SRLayoutManagerPtr layoutManager = SRLayoutManager::create();
  _textStorage->addLayoutManager(layoutManager);

  // SRDocumentView(:UIScrollView) -> SRPageView -> SRColumnView ������ ����
  SRRect docRect = windowRect;
  //docRect = docRect.insetBy(PAGE_MARGIN.width, PAGE_MARGIN.height);
  //docRect.size.height -= 2 * PAGE_MARGIN.height;
  SRDocumentViewPtr docView = SRDocumentView::create(docRect);
  docView->setBackgroundColor(SRColor(1, 1, 1)); // white
  SR()->setContentView(docView); // �����쿡 �������並 ����

  //docView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

  // SRLayoutManagerDelegate ��ü �߰�
  LayoutMgrDelegatePtr layoutDelegate = std::make_shared<LayoutMgrDelegate>(SR());
  layoutManager->setDelegate(layoutDelegate);

  //_pageSize = SR()->getBounds().size;
  //_pageSize.height = 50; // @@test
  //SRRect drawRect({ 0, 0 }, _pageSize);
  SRRect drawRect({ 0, 0 }, {0, 0});
  __test_col_idx = 0;
  SR_addColumnView(SR(), layoutManager, _columnCnt, drawRect);

  _textStorage->beginEditing();
  _textStorage->setAttributedString(SR_getAttrString());
  _textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�(�ؽ�Ʈ����� ���� �Ŀ� ȣ��ǰ� ����)
#else
  CTDocViewPtr docView = CTDocView::create(rcWindow);
  SR()->setContentView(docView); // ���⼭ SR()�� ���ʷ� ȣ��Ǹ鼭 ���ο��� ������ ����???
  //docView->setBackgroundColor(SRColor(1, 1, 0)); // yellow // ###
  docView->setAttributedString(SR_getAttrString());
  docView->buildFrames(2);
#endif
}

// WM_SIZE ������ ȣ������
static void SR_resize(void* hwnd, int w, int h) {
  if (!SR() || SR()->contentView() == nullptr) {
    return;
  }
  UIViewPtr docView = SR()->contentView();
  SRRect rcDocView = docView->getFrame();
  static bool isFirst = true;
  if (isFirst || SR()->getFrame().size == SRSize(w, h)) {
    isFirst = false;
    return;
  }

  SRSize size(w, h);
  SRSize minSize = _minWindowSize();
  if (size.width < minSize.width) {
    size.width = minSize.width;
  }
  if (size.height < minSize.height) {
    size.height = minSize.height;
  }
  SR()->setSize(size);
  SR_positionCenter();
}

// WM_PAINT ������ ȣ������
static void SR_redraw(HWND hwnd, HDC hdc, RECT rc) {
  if (!SR() || SR()->contentView() == nullptr) {
    return;
  }
  static int __cnt = 0;
  SR_LOGD(L"SR_redraw (%d) [%03d, %03d, %03d, %03d]"
    , ++__cnt, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  SRRect rcClip = SRRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  CGContextPtr ctx = CGContext::create(WIN32GraphicContextImpl::create(hwnd, rcClip, hdc));
  //CGContext::setCurrentContext(ctx);
  //ctx->clip(rcClip);
  ctx->intersectClipRect(rcClip);

  SR()->draw(ctx);
  //ctx = nullptr; // �ڵ��Ҹ�ǹǷ� ���ʿ���
}

// ���� ���� ��ũ�� ũ��
static const int LINE_VSCROLL_SIZE = 20;
static const int LINE_HSCROLL_SIZE = 20;

// ������(UIWindow)�� ��ũ�ѹ� ��ġ�� �������ش�.
static void SR_vscroll(void* hwnd, int scrollCode) {
  SRPoint offset = SR()->contentOffset();

  SRScrollInfo info;
  SR()->getScrollInfo(SR_SB_VERT, info);
  if (info.pos != offset.y) {
    //SR_ASSERT(0); // ���� resize �ÿ� offset.y �� �ξ� ū ��찡 ����� �ִ�(������ ���� ���)
  }

  SRSize rcSize = SR()->getBounds().size; // ������ ũ�� ����

  switch (scrollCode) {
  case SB_LINEDOWN:
    offset.y += LINE_VSCROLL_SIZE;
    break;
  case SB_LINEUP:
    offset.y -= LINE_VSCROLL_SIZE;
    break;
  case SB_PAGEDOWN:
    offset.y += rcSize.height;
    break;
  case SB_PAGEUP:
    offset.y -= rcSize.height;
    break;
  case SB_THUMBTRACK:
    offset.y = info.trackPos;
    break;
  default:
    //SB_ENDSCROLL�� ȣ��ǰ� ����
    //SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;

  if (offset.y > maxPos) {
    offset.y = maxPos; // SB_PAGEDOWN ���� �Ѿ�� ���
  }
  //if (offset.y <= 0) {
  if (offset.y < 0) {
    offset.y = 0; // SB_PAGEDUP ���� �Ѿ�� ���
  }

  if (SR()->contentOffset().y != offset.y) {
    SR()->setContentOffset(offset);
    //SR()->setScrollPos(SR_SB_VERT, offset.y, false);
    // updateWindow() ���� ȣ�����൵ ��ũ�� ��ư�� ������ ������ ��ũ�� ��ư�� ���ŵ��� �ʰ� �ִ�.
    // @@todo ��ũ�ѽ� ȭ�� �������� ���ַ���, ���� ������ �����ϰ�, ���� ��ũ�� ó���� ����� �Ѵ�???
    // ���� ���� ��ũ�� ó���� �ϰ� �����Ƿ�, ȭ�� ĳ���� ��� �������� ����������???
    SR()->invalidateRect(NULL, false/*=erase*/); // ȣ������ָ� ȭ�� ������ �ȵ�, erase�� true�� WM_ERASEBKGND�� �߻���Ŵ.
    //SR()->updateWindow(); // WM_PAINT �޽����� �ٷ� ó����
  }
}

static void SR_hscroll(void* hwnd, int scrollCode) {
  SRPoint offset = SR()->contentOffset();

  SRScrollInfo info;
  SR()->getScrollInfo(SR_SB_HORZ, info);
  SR_ASSERT(info.pos == offset.x);

  SRSize rcSize = SR()->getBounds().size; // ������ ũ�� ����

  switch (scrollCode) {
  case SB_LINERIGHT:
    offset.x += LINE_HSCROLL_SIZE;
    break;
  case SB_LINELEFT:
    offset.x -= LINE_HSCROLL_SIZE;
    break;
  case SB_PAGEDOWN:
    offset.x += rcSize.width;
    break;
  case SB_PAGEUP:
    offset.x -= rcSize.width;
    break;
  case SB_THUMBTRACK:
    offset.x = info.trackPos;
    break;
  default:
    //SB_ENDSCROLL�� ȣ��ǰ� ����
    //SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;

  if (offset.x > maxPos) {
    offset.x = maxPos; // SB_PAGEDOWN ���� �Ѿ�� ���
  }
  if (offset.x < 0) {
    offset.x = 0; // SB_PAGEDUP ���� �Ѿ�� ���
  }

  if (SR()->contentOffset().x != offset.x) {
    SR()->setContentOffset(offset);
    // updateWindow() ���� ȣ�����൵ ��ũ�� ��ư�� ������ ������ ��ũ�� ��ư�� ���ŵ��� �ʰ� �ִ�.
    SR()->invalidateRect(NULL, false);
    //SR()->updateWindow();
  }
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: ���⿡ �ڵ带 �Է��մϴ�.
	MSG msg;
	HACCEL hAccelTable;

	// ���� ���ڿ��� �ʱ�ȭ�մϴ�.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SREDITOR2, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// ���� ���α׷� �ʱ�ȭ�� �����մϴ�.
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SREDITOR2));

	// �⺻ �޽��� �����Դϴ�.
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}

//
//  �Լ�: MyRegisterClass()
//
//  ����: â Ŭ������ ����մϴ�.
//
//  ����:
//
//    Windows 95���� �߰��� 'RegisterClassEx' �Լ����� ����
//    �ش� �ڵ尡 Win32 �ý��۰� ȣȯ�ǵ���
//    �Ϸ��� ��쿡�� �� �Լ��� ����մϴ�. �� �Լ��� ȣ���ؾ�
//    �ش� ���� ���α׷��� �����
//    '�ùٸ� ������' ���� �������� ������ �� �ֽ��ϴ�.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SREDITOR2));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SREDITOR2);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   �Լ�: InitInstance(HINSTANCE, int)
//
//   ����: �ν��Ͻ� �ڵ��� �����ϰ� �� â�� ����ϴ�.
//
//   ����:
//
//        �� �Լ��� ���� �ν��Ͻ� �ڵ��� ���� ������ �����ϰ�
//        �� ���α׷� â�� ���� ���� ǥ���մϴ�.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
#if 1
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
     0, 0, 600, 400, NULL, NULL, hInstance, NULL); // 476 -> 2 �÷��� �� 200
#else
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
     0, 0, 476, 150, NULL, NULL, hInstance, NULL); // 476 -> 2 �÷��� �� 200
#endif

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  �Լ�: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ����: �� â�� �޽����� ó���մϴ�.
//
//  WM_COMMAND	- ���� ���α׷� �޴��� ó���մϴ�.
//  WM_PAINT	- �� â�� �׸��ϴ�.
//  WM_DESTROY	- ���� �޽����� �Խ��ϰ� ��ȯ�մϴ�.
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

  switch (message)
  {
  case WM_COMMAND:
    wmId = LOWORD(wParam);
    wmEvent = HIWORD(wParam);
    // �޴� ������ ���� �м��մϴ�.
    switch (wmId)
    {
    case IDM_ABOUT:
      DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
      break;
    case IDM_EXIT:
      DestroyWindow(hWnd);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
  case WM_PAINT:
    hdc = BeginPaint(hWnd, &ps);
    // TODO: ���⿡ �׸��� �ڵ带 �߰��մϴ�.
#if 0
    //::Rectangle(hdc, 0, 0, 1, 1); // �Ⱥ���
    ::Rectangle(hdc, 0, 0, 2, 2); // (0, 0) ���� 2 pixel ��ŭ ����(0~1)
    //::Rectangle(hdc, 1, 1, 3, 3); // (1, 1) ���� 2 pixel ��ŭ ����
    //::Rectangle(hdc, 1, 1, 4, 4); // (1, 1), (3, 3)
    if (0) {
      HPEN hPen = ::CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
      HPEN hOldPen = (HPEN)::SelectObject(hdc, hPen);
      //::Rectangle(hdc, 0, 0, 3, 3); // �β� 2 �̸�, (0, 0, 3, 3) �� ����
      ::Rectangle(hdc, 1, 1, 4, 4); // �β� 2 -> (0, 0) (3, 3)�� ���� -> 0~1, 2~3(�»��: �β���ŭ ������, ���ϴ�: �β���ŭ ������)
      //::Rectangle(hdc, 2, 2, 5, 5); // �β� 2 �̸�, (1, 1) ��ġ���� ����(*) -> 1~2, 3~4
      ::SelectObject(hdc, hOldPen);
      ::DeleteObject(hPen);
    }
#else
    SR_redraw(hWnd, hdc, ps.rcPaint);
#endif
		EndPaint(hWnd, &ps);
		break;
  case WM_ERASEBKGND: // @@todo ��ũ�ѽÿ� �������� ������� �ʰ� �ִ�.
  //{
  //  RECT rcClip;
  //  HDC hDC = GetDC(hWnd);
  //  GetClipBox(hDC, &rcClip);
  //  //pWnd->OnDraw(hDC, rcClip);
  //  SR_redraw(hWnd);
  //  ReleaseDC(hWnd, hDC);
  //}
  return TRUE;
  case WM_SIZE:
    hdc = ::GetDC(NULL);
    SR_resize(hWnd, LOWORD(lParam), HIWORD(lParam));
    ::ReleaseDC(NULL, hdc);
    break;
  case WM_CREATE:
    SR_create(hWnd);
    break;
  case WM_VSCROLL:
    SR_vscroll(hWnd, LOWORD(wParam));
    break;
  case WM_HSCROLL:
    SR_hscroll(hWnd, LOWORD(wParam));
    break;
  case WM_LBUTTONUP:
    break;
  case WM_RBUTTONUP:
  {
#ifdef USE_UITextView
    static int __cnt = 0;
    if (++__cnt % 3 == 0) {
      _columnCnt = 1;
    } else if (__cnt % 3 == 1) {
      _columnCnt = 2;
    } else {
     _columnCnt = 3;
    }
    SR_create(hWnd);
    SR()->invalidateRect(NULL, false);
#else
    static int __cnt = 0;
    CTDocViewPtr docView = std::dynamic_pointer_cast<CTDocView>(SR()->contentView());
    if (++__cnt % 2 == 1) {
      //SR()->setAutoresizingMask(UIViewAutoresizingNone);
      docView->buildFrames(1);
    } else {
      //SR()->setAutoresizingMask(UIViewAutoresizingFlexibleWidth);
      docView->buildFrames(2);
    }
    SR()->invalidateRect(NULL, false);
    SR()->updateWindow();
#endif
#if 0
    POINT point = { LOWORD(lParam), HIWORD(lParam) };
    ::ClientToScreen(hWnd, &point);

    HMENU hMenu = ::CreatePopupMenu();
    SR_ASSERT(hMenu);
    if (!hMenu)
      break;

    ::HideCaret((HWND)hWnd);

    ::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("UIViewAutoresizingNone(&F2)"));
    ::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("UIViewAutoresizingFlexibleWidth(&F3)"));
    ::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("UIViewAutoresizingFlexibleHeight(&F5)"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("�̹���2 ����(&F6)"));

    //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("�߶󳻱�(&T)\tCtrl+X"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_COPY, _T("����(&C)\tCtrl+C"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_PASTE, _T("�ٿ��ֱ�(&P)\tCtrl+V"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_SELECTALL, _T("��� ����(&A)\tCtrl+A"));

    ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
    ::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, point.x, point.y, 0, hWnd, NULL);

    ::DestroyMenu(hMenu);

    ::ShowCaret((HWND)hWnd);
#endif
  }
  break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// ���� ��ȭ ������ �޽��� ó�����Դϴ�.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
