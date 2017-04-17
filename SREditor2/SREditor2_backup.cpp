// SREditor2.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "SREditor2.h"

#define MAX_LOADSTRING 100

// 전역 변수:
HINSTANCE hInst;								// 현재 인스턴스입니다.
TCHAR szTitle[MAX_LOADSTRING];					// 제목 표시줄 텍스트입니다.
TCHAR szWindowClass[MAX_LOADSTRING];			// 기본 창 클래스 이름입니다.

// 이 코드 모듈에 들어 있는 함수의 정방향 선언입니다.
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
  size += 14; // @@ 임시로 최소 폰트 크기(14) 이상으로 설정함(추후 1개 라인 높이 이상으로 설정???)
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
  //imgAttr->addValue(L"width", 100); // @@todo 현재 기본타입은 SRObjectDictionary이 불가능하다!!!
  CTImageRunInfoPtr imgInfo = CTImageRunInfo::create(imgPath, imgSize.width, imgSize.height);
  imgAttr->addValue(CTImageRunInfo::IMAGE_RUN_KEY, imgInfo);

  CTRunDelegatePtr delegate = CTRunDelegate::create(callbacks, std::dynamic_pointer_cast<SRObject>(imgAttr));
  //SRObjectDictionaryPtr attrDictionaryDelegate = SRObjectDictionary::create();
  //attrDictionaryDelegate->addValue(kCTRunDelegateAttributeName, delegate);
  //attrDictionaryDelegate->addValue(kCTParagraphStyleAttributeName, paragraphStyle); // @@

  //add a space to the text so that it can call the delegate
  const SRObjectDictionaryPtr attribs = attrStr->attributes(range.location, nullptr);
  SRObjectDictionaryPtr newAttribs = SRObjectDictionary::create(*attribs); // 속성을 추가하므로 복사해줘야 한다.
  newAttribs->addValue(kCTRunDelegateAttributeName, delegate);
#if 1
  // range 위치의 속성에 CTRunDelegate 객체만 더 추가하는 방법
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
  //  _srWindow->setBackgroundColor(SRColor(0.643f, 0.643f, 0.643f)); // 회색
  //}
  return _srWindow;
}

#ifdef USE_SRTextTable
static SRAttributedStringPtr tableCellAttributedString(const SRTextBlockList& parentBlocks
  , const SRString& string, SRTextTablePtr table
  , SRColor backgroundColor, SRColor borderColor, SRInt row, SRInt column) {
  SRTextTableBlockPtr tableBlock = SRTextTableBlock::create(table, row, 1, column, 1); // 테이블(블럭)은 셀 블럭에 포함시킴

  tableBlock->setBackgroundColor(backgroundColor);
  tableBlock->setBorderColor(borderColor);
  ////tableBlock->setWidth(4.0, SRTextTableBlock::Margin); // 불필요???
  tableBlock->setWidth(1.0, SRTextTableBlock::Border); // html table 의 경우 테이블의 경계선 두께가 1 이상이면, 셀 경계선은 무조건 1 두께를 갖음
  tableBlock->setWidth(1.0, SRTextTableBlock::Padding); // 셀 경계선으로부터의 패딩

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭부터 추가
  paragraphStyle->addTextBlock(tableBlock); // 셀 블럭 추가(동일한 테이블(블럭)을 포함함)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

// 셀 블럭이 테이블 블럭을 포함함(별도로 테이블 블럭이 존재하지는 않음)
static SRAttributedStringPtr tableAttributedString(const SRTextBlockList& parentBlocks) {
  SRAttributedStringPtr tableString = SRAttributedString::create(L""); // "\n\n"
  SRTextTablePtr table = SRTextTable::create();
  table->setNumberOfColumns(2);
  table->setNumberOfRows(2);
  ////table->setWidth(0.0, SRTextTableBlock::Margin); // 불필요??? html table 속성에는 없는듯함
  table->setWidth(1.0, SRTextTableBlock::Border);
  table->setWidth(1.0, SRTextTableBlock::Padding); // html table cellspacing 에 해당함(기본 2)

  {
    // 테이블 이전에 부모 블럭에 텍스트 추가(텍스트 블럭 추가)
    SRAttributedStringPtr str = SRAttributedString::create(L"@\n"); // "\n\n"
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭만 추가
    str->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
    //tableString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, tableString->getLength()));
  }

  // 테이블 셀들에는 부모 블럭 & 셀 블럭(테이블(블럭)이 포함됨) 속성 추가
  tableString->append(tableCellAttributedString(parentBlocks, L"a\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 0));
  tableString->append(tableCellAttributedString(parentBlocks, L"b\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 1));
  //tableString->appendAttributedString(tableCellAttributedString(parentBlocks, L"b", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 2));
  tableString->append(tableCellAttributedString(parentBlocks, L"c\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 0));
  tableString->append(tableCellAttributedString(parentBlocks, L"d\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1));
  //tableString->appendAttributedString(tableCellAttributedString(parentBlocks, L"d", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 2));

  {
    // 테이블 이후에 부모 블럭에 텍스트 추가
    SRAttributedStringPtr str = SRAttributedString::create(L"#");

    // default TextBlock 속성 추가
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭부터 추가
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
  SRTextListBlockPtr block = SRTextListBlock::create(textList); // 리스트(블럭)은 리스트 블럭에 포함시킴

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭부터 추가
  paragraphStyle->addTextBlock(block); // 셀 블럭 추가(셀 블럭안에 테이블(블럭)이 포함됨)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static SRAttributedStringPtr textListAttributedString(const SRTextBlockList& parentBlocks, SRTextList::MarkerFormat markerFormat) {
  SRAttributedStringPtr attrString = SRAttributedString::create(L"@\n"); // "\n\n"
  //SRTextListPtr textList = SRTextList::create(SRTextList::CIRCLE);
  SRTextListPtr textList = SRTextList::create(markerFormat);

  // default TextBlock 속성 추가
  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭만 추가

  // 테이블 이전에 부모 블럭에 텍스트 추가(텍스트 블럭 추가)
  attrString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrString->getLength()));

  // 테이블 셀들에는 부모 블럭 & 셀 블럭(테이블(블럭)이 포함됨) 속성 추가
  attrString->append(textListItemAttributedString(parentBlocks, L"aa\n", textList)); // aa\nA\nB -> OK!
  attrString->append(textListItemAttributedString(parentBlocks, L"bb\n", textList));
  attrString->append(textListItemAttributedString(parentBlocks, L"cc\n", textList));

  {
    // 테이블 이후에 부모 블럭에 텍스트 추가
    SRAttributedStringPtr str = SRAttributedString::create(L"#");

    // default TextBlock 속성 추가
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭부터 추가
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

// 속성 적용된 문자열을 생성해 리턴해줌
static SRAttributedStringPtr SR_getAttrString() {
  SRAttributedStringPtr attrStr = SRAttributedString::create(
    //L"abcdeghijk.0123456789.우클릭!!!Abcdeghijk.0123456789.ABcdeghijk.0123456789.ABCdeghijk.0123456789."
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
  // 1) 문자열로부터 변경
  SRStringPtr str = SRString::create(L"AAA\naa");
  // 2) AttributedString 로부터 변경
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

#if 0 // 텍스트만 추가
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
  // 부모 문자열 replaceRange.location 위치에 자식 문자열 추가(해당 위치 텍스트 블럭을 부모 블럭으로 설정함)
  if (1) {
    SRRange replaceRange(2, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // 자식 테이블 추가
    SRAttributedStringPtr childTableAttrStr = tableAttributedString(parentTextBlocks);
    parentAttrStr->replaceCharacters(replaceRange, childTableAttrStr);
  }
//#else
  if (1) {
    SRRange replaceRange(5, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // 자식 리스트 추가
    parentAttrStr->replaceCharacters(replaceRange, textListAttributedString(parentTextBlocks, SRTextList::DECIMAL));
  }
  if (1) {
    SRRange replaceRange(7, 1);
    const SRTextBlockList& parentTextBlocks = getTextBlockList(parentAttrStr, replaceRange.location);
    // 자식 테이블 추가
    SRAttributedStringPtr childTableAttrStr = tableAttributedString(parentTextBlocks);
    parentAttrStr->replaceCharacters(replaceRange, childTableAttrStr);
  }
#endif
#endif
#endif
#endif

#ifdef USE_RunDelegate
  if (1) {
    // @@@ 여기서도 생성할 경우, addTextBlockList() 호출을 안하면 에러 발생중!!!
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(rootBlocks);
    ////attrStr->appendAttributedString(SRAttributedString::create(L"ABCDE12345abcde"));
    //attrStr->appendAttributedString(SRAttributedString::create(L"bc")); // @@@ 문자열만 추가시 이전 문자의 속성을 상속받을까???
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
  UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // 고정폭 폰트(28 -> 글자폭 20, 14 -> 20)
  //UIFontPtr font = UIFont::fontWithName(L"Symbol", 14.0); // bullet
  attrStr->addAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  if (1) {
    // 부분 폰트 속성 적용
    SRRange currentRange(2, 1);
    UIFontPtr font = UIFont::fontWithName(L"Arial", 24.0); // 12.0
    attrStr->addAttribute(kCTFontAttributeName, font, currentRange);
    attrStr->addAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(2, 3));
  }
#else
  //UIFontPtr font = UIFont::fontWithName(L"Arial", 36.0); // 12.0
  UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // 고정폭 폰트(28 -> 글자폭 20)
  attrStr->addAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  std::wstring str = attrStr->getString()->data();
  std::wstring::size_type strOffset = 0;

  if (1) {
    // paragraph 속성 적용
    CTTextAlignment alignment = kCTCenterTextAlignment; //kCTRightTextAlignment; //kCTJustifiedTextAlignment;
    CTParagraphStyleSetting _settings[] = { { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment } };
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::create(_settings, sizeof(_settings) / sizeof(_settings[0]));

    // set paragraph style attribute
    //CFAttributedStringSetAttribute(attrStr, CFRangeMake(0, CFAttributedStringGetLength(attrStr)), kCTParagraphStyleAttributeName, paragraphStyle);
    // @@TODO 전체 문단 속성을 적용하고, 부분적으로 수정하자!
    //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, 16/*attrStr->getLength()*/));
    strOffset = str.find('\n', strOffset); // 1st paragraph -> 기본으로 left
    SRRange range;
    range.location = ++strOffset; // 2nd paragraph -> center 로 설정
    strOffset = str.find('\n', strOffset);
    range.length = (strOffset - range.location);
    if (strOffset != std::wstring::npos) attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);
  }
  if (1) {
    // paragraph 속성 적용
    //    create paragraph style and assign text alignment to it
    CTTextAlignment alignment = kCTRightTextAlignment; //kCTJustifiedTextAlignment;
    CTParagraphStyleSetting _settings[] = { { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment } };
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::create(_settings, sizeof(_settings) / sizeof(_settings[0]));

    // set paragraph style attribute
    //strOffset = str.find('\n', strOffset); // 3rd paragraph -> right
    SRRange range;
    range.location = ++strOffset; // 3rd paragraph -> right 로 설정
    strOffset = str.find('\n', strOffset);
    range.length = (strOffset - range.location);
    if (strOffset != std::wstring::npos) attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);
    //attrStr->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(4, 3));
  }

  if (1) {
    // 부분 폰트 속성 적용
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
  docView->setCenter(center); // 부모뷰(윈도우) 내에서의 center 좌표로 설정한다.
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

  // 페이지 크기에서 양옆의 컬럼 마진을 뺀 영역 크기를 구함
  SRRect columnRect = pageRect.insetBy(COLUMN_MARGIN.width, COLUMN_MARGIN.height);
  // 컬럼 사이의 간격을 위해 컬럼 개수 - 1 만큼 폭을 줄임
  columnRect.size.width -= (COLUMN_CNT - 1) * COLUMN_MARGIN.width;
  columnRect.size.width /= COLUMN_CNT;
  columnRect.origin = SRPoint(COLUMN_MARGIN.width, COLUMN_MARGIN.height);

  if (columnViews.empty() || columnViews.size() == lastPageView->_column) {
    // 컬럼이 없거나 꽉찬 경우, 페이지 추가
    SRPageViewPtr pageView = SRPageView::create(pageRect, COLUMN_CNT);
    //pageView->setBackgroundColor(SRColor(0, 0, 1));
    docView->addSubview(pageView);
    //pageView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

    lastPageView = pageView;

    // 이전 페이지의 첫번째 컬럼 영역과 동일하게 설정
    //SRColumnViewPtr firstColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.begin()));
    //columnRect = firstColumnView->getFrame();
  } else {
    // 두 번째 이후 컬럼은 바로 이전 컬럼의 오른쪽 영역으로 설정
    SRColumnViewPtr lastColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.rbegin()));
    columnRect = lastColumnView->getFrame();
    columnRect.origin.x = columnRect.left() + columnRect.width();
    columnRect.origin.x += COLUMN_MARGIN.width; // 컬럼 마진만큼 간격을 띄움
  }

  static SRTextContainerWeakPtr _firstTextContainer;
  //if (columnViews.empty()) {
  if (layoutMgr) {
    // 최초에 한번만 컨테이너를 추가시킴(페이징은 전체를 레이아웃후 페이지 크기로 자르게 함)
    SRSize containerSize(columnRect.size);
    SRTextContainerPtr textContainer = SRTextContainer::initWithSize(containerSize);
    layoutMgr->addTextContainer(textContainer);
    _firstTextContainer = textContainer;
  }

  // 컬럼뷰 추가
  SRColumnViewPtr columnView = SRColumnView::create(columnRect);
  //columnView->init(textContainer);
  columnView->init(_firstTextContainer.lock());
  if (drawRect.size.width == 0) {
    drawRect.size = columnRect.size; // 최초에 컬럼 크기로 설정하기 위함임
  }
  columnView->setDrawRect(drawRect);
  //columnView->setBackgroundColor(SRColor(0, 1, 0));
  //columnView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight
  lastPageView->addSubview(columnView);
  //columnView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

  //// 하단 페이지 여백 추가
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
    // SRLayoutManager::invalidateDisplay()에서 호출됨
    //UITextView::layoutManagerDidInvalidateLayout()
    //  [self _adjustTextLayerSize : FALSE/*setContentPos*/]; // 새로 레이아웃된 크기 획득
    //  [self setNeedsDisplay];
    //UITextView::_adjustTextLayerSize:(BOOL)setContentPos {
    //    SRRect allText = [_layoutManager usedRectForTextContainer:_textContainer]; // 새로 레이아웃된 크기 획득
    //    [self setNeedsDisplay]; // 화면 갱신 예약
    //    [self setContentSize:size]; // 크기 변경이 됐으면, setContentSize()로 크기를 새로 설정함
    //(SRRect)LM::usedRectForTextContainer:(NSTextContainer*)tc
    //  [self layoutIfNeeded]; // 여기서 _layoutAllText()가 호출됨
    //  return 0, 0, _totalSize
    //(void)LM::layoutIfNeeded {
    //  if (_needsLayout) 이면, [self _layoutAllText]; 후, _totalSize 로 SRRect 리턴
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
      // 컬럼뷰(UITextView) 크기에 맞춰지기는 하지만, 페이지뷰 등의 마진은 사라지고 있다!!!
      // 페이지, 문서뷰 크기는 변경되지 않고 있다.
      //columnView->setBounds(bounds);
#endif

      if (filledLength >= textStorage->getLength()) {
        // 레이아웃 완료
        //return;
      }

      //docView->paginate(SRSize(300, 50));
      //docView->paginate(frame.size);
      docView->paginate(frame.size, usedRect.size);
      //columnView->setDrawRect(docView->getPage(0)); // 첫번째 컬럼은 이미 생성된 상태이므로 뷰영역만 수정함

      //// 첫번째 페이지는 크기만 조정함(여기에 전부 레이아웃돼 있다)
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
      // 하단 페이지 여백 추가
      if (docView->getSubviews().size() > 1) {
        SRRect docBounds = docView->getBounds();
        docBounds.size.height += PAGE_MARGIN.height;
        docView->setSize(docBounds.size);
      }
      SR()->updateScrollBar(); // 자식뷰들 추가가 끝나면 스크롤 정보를 갱신해줘야 스크롤바 위치가 맞게 된다.
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
  v1.reserve(2); // 이렇게 안해주면 A(111) 가 한번 더 복사된다(내부 메모리 이동때문???)
  v1.push_back(A(111)); // 생성자1, 복사생성자1, 소멸자1
  v1.push_back(A(222)); // 생성자2, 복사생성자2, 소멸자2
  v1.clear(); // 복사소멸자1, 2
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
  v1.reserve(2); // 이렇게 안해주면 A(111) 가 한번 더 복사된다(내부 메모리 이동때문???)
  APtr p1 = std::make_shared<A>(111);
  APtr p2 = std::make_shared<A>(222);
  v1.push_back(p1);
  v1.push_back(p2);
  v1.clear();
  int b = 0;
  // p1, p2 소멸시 ~A() 호출됨!!!
}

static SRTextStoragePtr _textStorage;

// WM_CREATE 에서만 호출중임
static void SR_create(void* hwnd) {
  RECT rc;
  ::GetClientRect((HWND)hwnd, &rc);
  SRRect windowRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top); // 처음에는 윈도우 크기와 동일한 크기로 CTDocView를 생성함

  //test_vector_ptr();

  if (0) {
    if (0) {
      SRRunArray::test();
      return;
    }
    int ocnt = SRObject::debugInfo()._obj_cnt; // 16
    UIFont::test();
    SRAttributedString::test(hwnd, rc.right - rc.left, rc.bottom - rc.top);
    int ocnt2 = SRObject::debugInfo()._obj_cnt; // 21(static UIFont::_fontCache 때문임)
    int a = 0;
    //return;
  }

  if (_srWindow) {
    //_srWindow->removeSubviews();
    int ocnt = SRObject::debugInfo()._obj_cnt;
    int rcnt = g_render_cnt();
    _srWindow = nullptr; // @@todo 소멸자가 호출되지 않고 있다!!!
    int ocnt2 = SRObject::debugInfo()._obj_cnt;
    int rcnt2 = g_render_cnt();
    int a = 0;
  }

  _srWindow = UIWindow::create(windowRect, WIN32WindowImpl::create(hwnd));
  SR()->setBackgroundColor(SRColor(0.643f, 0.643f, 0.643f)); // 회색

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
  //_textStorage->setAttributedString(SR_getAttrString()); // SRTextStorage::initWithAttributedString() 함수를 추가할까???
  SRLayoutManagerPtr layoutManager = SRLayoutManager::create();
  _textStorage->addLayoutManager(layoutManager);

  // SRDocumentView(:UIScrollView) -> SRPageView -> SRColumnView 순서로 설정
  SRRect docRect = windowRect;
  //docRect = docRect.insetBy(PAGE_MARGIN.width, PAGE_MARGIN.height);
  //docRect.size.height -= 2 * PAGE_MARGIN.height;
  SRDocumentViewPtr docView = SRDocumentView::create(docRect);
  docView->setBackgroundColor(SRColor(1, 1, 1)); // white
  SR()->setContentView(docView); // 윈도우에 컨텐츠뷰를 설정

  //docView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // @@ test!!!

  // SRLayoutManagerDelegate 객체 추가
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
  _textStorage->endEditing(); // 여기서 레이아웃팅됨(텍스트뷰까지 생성 후에 호출되게 하자)
#else
  CTDocViewPtr docView = CTDocView::create(rcWindow);
  SR()->setContentView(docView); // 여기서 SR()이 최초로 호출되면서 내부에서 윈도우 생성???
  //docView->setBackgroundColor(SRColor(1, 1, 0)); // yellow // ###
  docView->setAttributedString(SR_getAttrString());
  docView->buildFrames(2);
#endif
}

// WM_SIZE 에서만 호출중임
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

// WM_PAINT 에서만 호출중임
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
  //ctx = nullptr; // 자동소멸되므로 불필요함
}

// 라인 단위 스크롤 크기
static const int LINE_VSCROLL_SIZE = 20;
static const int LINE_HSCROLL_SIZE = 20;

// 윈도우(UIWindow)의 스크롤바 위치를 갱신해준다.
static void SR_vscroll(void* hwnd, int scrollCode) {
  SRPoint offset = SR()->contentOffset();

  SRScrollInfo info;
  SR()->getScrollInfo(SR_SB_VERT, info);
  if (info.pos != offset.y) {
    //SR_ASSERT(0); // 현재 resize 시에 offset.y 가 훨씬 큰 경우가 생기고 있다(사이즈 줄일 경우)
  }

  SRSize rcSize = SR()->getBounds().size; // 윈도우 크기 저장

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
    //SB_ENDSCROLL로 호출되고 있음
    //SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;

  if (offset.y > maxPos) {
    offset.y = maxPos; // SB_PAGEDOWN 으로 넘어서는 경우
  }
  //if (offset.y <= 0) {
  if (offset.y < 0) {
    offset.y = 0; // SB_PAGEDUP 으로 넘어서는 경우
  }

  if (SR()->contentOffset().y != offset.y) {
    SR()->setContentOffset(offset);
    //SR()->setScrollPos(SR_SB_VERT, offset.y, false);
    // updateWindow() 까지 호출해줘도 스크롤 버튼을 누르고 있으면 스크롤 버튼은 갱신되지 않고 있다.
    // @@todo 스크롤시 화면 깜박임을 없애려면, 갱신 영역을 지정하고, 실제 스크롤 처리를 해줘야 한다???
    // 현재 직접 스크롤 처리를 하고 있으므로, 화면 캐쉬를 써야 깜박임이 없어지려나???
    SR()->invalidateRect(NULL, false/*=erase*/); // 호출안해주면 화면 갱신이 안됨, erase가 true면 WM_ERASEBKGND를 발생시킴.
    //SR()->updateWindow(); // WM_PAINT 메시지를 바로 처리함
  }
}

static void SR_hscroll(void* hwnd, int scrollCode) {
  SRPoint offset = SR()->contentOffset();

  SRScrollInfo info;
  SR()->getScrollInfo(SR_SB_HORZ, info);
  SR_ASSERT(info.pos == offset.x);

  SRSize rcSize = SR()->getBounds().size; // 윈도우 크기 저장

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
    //SB_ENDSCROLL로 호출되고 있음
    //SR_ASSERT(0);
    return;
  }

  int maxPos = info.max - info.page;

  if (offset.x > maxPos) {
    offset.x = maxPos; // SB_PAGEDOWN 으로 넘어서는 경우
  }
  if (offset.x < 0) {
    offset.x = 0; // SB_PAGEDUP 으로 넘어서는 경우
  }

  if (SR()->contentOffset().x != offset.x) {
    SR()->setContentOffset(offset);
    // updateWindow() 까지 호출해줘도 스크롤 버튼을 누르고 있으면 스크롤 버튼은 갱신되지 않고 있다.
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

 	// TODO: 여기에 코드를 입력합니다.
	MSG msg;
	HACCEL hAccelTable;

	// 전역 문자열을 초기화합니다.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SREDITOR2, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 응용 프로그램 초기화를 수행합니다.
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SREDITOR2));

	// 기본 메시지 루프입니다.
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
//  함수: MyRegisterClass()
//
//  목적: 창 클래스를 등록합니다.
//
//  설명:
//
//    Windows 95에서 추가된 'RegisterClassEx' 함수보다 먼저
//    해당 코드가 Win32 시스템과 호환되도록
//    하려는 경우에만 이 함수를 사용합니다. 이 함수를 호출해야
//    해당 응용 프로그램에 연결된
//    '올바른 형식의' 작은 아이콘을 가져올 수 있습니다.
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
//   함수: InitInstance(HINSTANCE, int)
//
//   목적: 인스턴스 핸들을 저장하고 주 창을 만듭니다.
//
//   설명:
//
//        이 함수를 통해 인스턴스 핸들을 전역 변수에 저장하고
//        주 프로그램 창을 만든 다음 표시합니다.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
#if 1
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
     0, 0, 600, 400, NULL, NULL, hInstance, NULL); // 476 -> 2 컬럼시 폭 200
#else
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
     0, 0, 476, 150, NULL, NULL, hInstance, NULL); // 476 -> 2 컬럼시 폭 200
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
//  함수: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  목적: 주 창의 메시지를 처리합니다.
//
//  WM_COMMAND	- 응용 프로그램 메뉴를 처리합니다.
//  WM_PAINT	- 주 창을 그립니다.
//  WM_DESTROY	- 종료 메시지를 게시하고 반환합니다.
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
    // 메뉴 선택을 구문 분석합니다.
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
    // TODO: 여기에 그리기 코드를 추가합니다.
#if 0
    //::Rectangle(hdc, 0, 0, 1, 1); // 안보임
    ::Rectangle(hdc, 0, 0, 2, 2); // (0, 0) 부터 2 pixel 만큼 꽉참(0~1)
    //::Rectangle(hdc, 1, 1, 3, 3); // (1, 1) 부터 2 pixel 만큼 꽉참
    //::Rectangle(hdc, 1, 1, 4, 4); // (1, 1), (3, 3)
    if (0) {
      HPEN hPen = ::CreatePen(PS_SOLID, 2, RGB(255, 0, 0));
      HPEN hOldPen = (HPEN)::SelectObject(hdc, hPen);
      //::Rectangle(hdc, 0, 0, 3, 3); // 두께 2 이면, (0, 0, 3, 3) 이 꽉참
      ::Rectangle(hdc, 1, 1, 4, 4); // 두께 2 -> (0, 0) (3, 3)이 꽉참 -> 0~1, 2~3(좌상단: 두께만큼 밖으로, 우하단: 두께만큼 안으로)
      //::Rectangle(hdc, 2, 2, 5, 5); // 두께 2 이면, (1, 1) 위치부터 꽉참(*) -> 1~2, 3~4
      ::SelectObject(hdc, hOldPen);
      ::DeleteObject(hPen);
    }
#else
    SR_redraw(hWnd, hdc, ps.rcPaint);
#endif
		EndPaint(hWnd, &ps);
		break;
  case WM_ERASEBKGND: // @@todo 스크롤시에 깜박임이 사라지지 않고 있다.
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
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("이미지2 삽입(&F6)"));

    //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("잘라내기(&T)\tCtrl+X"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_COPY, _T("복사(&C)\tCtrl+C"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_PASTE, _T("붙여넣기(&P)\tCtrl+V"));
    //::AppendMenu(hMenu, MF_STRING, IDM_SR_SELECTALL, _T("모두 선택(&A)\tCtrl+A"));

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

// 정보 대화 상자의 메시지 처리기입니다.
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
