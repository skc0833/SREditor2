// SREditor2.cpp : 응용 프로그램에 대한 진입점을 정의합니다.
//

#define NOMINMAX  // for minwindef.h
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

#include "SRTextContainer.h"
#include "SRLayoutManager.h"
#include "SRDocumentView.h"
#include "SRTextBlock.h"
#include "./CoreText/CTRunDelegate.h"

using namespace sr;

static SRIndex _demoWrappingImagePos = 0;

#include "SREditor2-inl.h"

const static SRSize _defaultPageSize(600, 400);
static SRSize _pageSize = _defaultPageSize;
//static const SRSize _pageSize(230, 200);
static const UIEdgeInsets _docLayoutMargin(10, 10, 10, 10);
static const UIEdgeInsets _pageLayoutMargin(10, 10, 10, 10);
static const SRFloat _spaceBetweenColumn = 10; // 컬럼 사이의 간격

static void _insertDemoImage(SRBool isWrap);

// 최소 윈도우 크기를 리턴시킴(윈도우 크기가 최소 폰트 이하일 경우 crash 발생하므로)
SRSize _minWindowSize() {
  SRSize size(14, 14); // @@ 임시로 최소 폰트 크기(14) 이상으로 설정함(추후 1개 라인 높이 이상으로 설정???)
  return size;
}

static UIWindowPtr _srWindow = nullptr;
static SRTextStoragePtr _textStorage = nullptr;
static SRInt _columnCnt = 1;

static UIWindowPtr& SR() {
  return _srWindow;
}

typedef enum {
  SR_DEMO_NONE,
  SR_DEMO_TABLE_LIST,
  SR_DEMO_TEXT,
} SR_DEMO_TYPE;

static SR_DEMO_TYPE _demoType = SR_DEMO_TEXT;
static SRBool _demoChanged = false;
static SRBool _debugDrawBorder = false;

// 속성 적용된 문자열을 생성해 리턴해줌
static SRAttributedStringPtr SR_getTestAttrString() {
  if (!_demoChanged && _textStorage) {
    return _textStorage;
  }

  SRAttributedStringPtr attrStr = SRAttributedString::create(L"\n");

  SRTextBlockPtr rootBlock = SRTextBlock::create(SRTextBlock::Text);
  rootBlock->setWidth(0.0, SRTextBlock::Margin); // 컬럼뷰 경계선 ~ 컨텐츠 경계선
  //rootBlock->setWidth(1.0, SRTextBlock::Border); // 컨텐츠 경계선 두께
  rootBlock->setWidth(10.0, SRTextBlock::Padding); // 컨텐츠 경계선 ~ 컨텐츠
  SRTextBlockList rootBlocks;
  rootBlocks.push_back(rootBlock);

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(rootBlocks);
  attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));

  UIFontPtr font = UIFont::fontWithName(L"굴림체", 24.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
  attrStr->setAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  switch (_demoType) {
  case SR_DEMO_TEXT:
    attrStr = _makeTestTextString(rootBlocks);
    break;
  case SR_DEMO_TABLE_LIST:
    attrStr = _makeTestTableString(rootBlocks);
    break;
  }
  return attrStr;
}

// 페이지 크기 획득
static SRRect _getPageRect(SRRect docViewRect) {
  SRRect pageRect = docViewRect;
  if (_pageSize.height > 0) {
    pageRect.size = _pageSize; // 페이지 크기를 고정 크기로 설정
  }
  pageRect.size.width -= _docLayoutMargin.left + _docLayoutMargin.right;
  pageRect.size.height -= _docLayoutMargin.top + _docLayoutMargin.bottom;
  SRSize minSize = _minWindowSize();
  if (pageRect.size.width < minSize.width) {
    SR_ASSERT(0);
    pageRect.size.width = minSize.width;
  }
  if (pageRect.size.height < minSize.height) {
    SR_ASSERT(0);
    pageRect.size.height = minSize.height;
  }
  pageRect.origin = { 0, 0 };
  return pageRect;
}

static SRRect _getColumnRect(SRRect docViewRect, int columnCnt) {
  SRRect pageRect = _getPageRect(docViewRect);
  // 페이지 크기에서 페이지 마진을 뺀 영역 크기를 구함
  SRRect columnRect = pageRect;
  columnRect.size.width -= _pageLayoutMargin.left + _pageLayoutMargin.right;
  columnRect.size.height -= _pageLayoutMargin.top + _pageLayoutMargin.bottom;
  // 컬럼 사이의 간격을 위해 컬럼 개수 - 1 만큼 폭을 줄임
  columnRect.size.width -= (columnCnt - 1) * _spaceBetweenColumn;
  columnRect.size.width /= columnCnt;
  SR_ASSERT(columnRect.origin.x == 0 && columnRect.origin.y == 0);
  return columnRect;
}

// 마지막 페이지에 컬럼을 추가한다.
static void SR_addColumnView(UIWindowPtr window, SRLayoutManagerPtr layoutMgr, int columnCnt, SRRect partialRect) {
  const int COLUMN_CNT = columnCnt;

  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window->contentView());
  docView->setLayoutMargins(_docLayoutMargin);
  UIViewList pageViews = docView->getSubviews();
  SRPageViewPtr lastPageView;
  UIViewList columnViews;

  // 페이지 크기 획득
  SRRect pageRect = _getPageRect(docView->getFrame());

  if (pageViews.size() > 0) {
    // 마지막 페이지뷰 정보 획득
    lastPageView = std::dynamic_pointer_cast<SRPageView>(*(pageViews.rbegin()));
    SRRect lastPageRect = lastPageView->getFrame();
    pageRect.origin.y = lastPageRect.bottom();
    pageRect.origin.y += _docLayoutMargin.bottom;

    columnViews = lastPageView->getSubviews();
    SR_ASSERT(columnViews.empty() || columnViews.size() <= lastPageView->_maxColumnCnt);
  }

  SRRect columnRect = _getColumnRect(docView->getFrame(), COLUMN_CNT);
  SRIndex colIndex = 0;

  if (columnViews.empty() || columnViews.size() == lastPageView->_maxColumnCnt) {
    // 컬럼이 없거나 꽉찬 경우, 페이지 추가
    SRPageViewPtr pageView = SRPageView::create(pageRect, COLUMN_CNT);
    //pageView->setBackgroundColor(SRColor(0, 0.5, 0.5)); // 청록색
    pageView->setBackgroundColor(SRColorWhite);
    pageView->setBorderColor(SRColorBlue);
    pageView->setBorderWidth(1.0f); // 페이지 경계선
    pageView->setLayoutMargins(_pageLayoutMargin); // 페이지뷰 안에 자식뷰(컬럼뷰)가 위치할 마진을 설정
    // 현재, 다음 페이지 사이를 클릭시 현재 페이지가 클릭되도록 설정(like ms-word)
    SRRect hitTestRect = pageRect;
    hitTestRect.size.height += _docLayoutMargin.bottom;
    docView->addSubview(pageView);
    docView->sizeToFit(); // 자식뷰 크기에 맞춘다.
    lastPageView = pageView;
  } else {
    // 두 번째 이후 컬럼은 바로 이전 컬럼의 오른쪽 영역으로 설정
    SRColumnViewPtr lastColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.rbegin()));
    columnRect = lastColumnView->getFrame();
    columnRect.origin.x = columnRect.left() + columnRect.width();
    columnRect.origin.x += _spaceBetweenColumn; // 컬럼 마진만큼 간격을 띄움
    colIndex = lastColumnView->_colIndex + 1;
  }

  // 컬럼뷰 추가
  SRColumnViewPtr columnView = SRColumnView::create(columnRect, colIndex);
  const SRTextContainerList& tcs = layoutMgr->textContainers();
  SR_ASSERT(tcs.size() == 1);
  SRTextContainerPtr textContainer = tcs.at(0);
  columnView->init(textContainer);
  if (partialRect.size.width == 0) {
    partialRect.size = columnRect.size; // 최초에 컬럼 크기로 설정하기 위함임
  }
  columnView->setPartialRect(partialRect); // 현재 컬럼뷰가 표시할 첫번째 컬럼뷰 내의 영역(페이징)
  columnView->setBackgroundColor(SRColorWhite);
  columnView->setBorderColor(SRColorGreen);
  //columnView->setBorderWidth(1.0f); // 컬럼 경계선
  //columnView->setAutoresizingMask(UIViewAutoresizingFlexibleWidth); // UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight
  lastPageView->addSubview(columnView);
}

class LayoutMgrDelegate : public SRLayoutManagerDelegate {
public:
  LayoutMgrDelegate(UIWindowPtr window) : _window(window) {}
  virtual void layoutManagerDidInvalidateLayout(SRLayoutManagerPtr sender) {}
  virtual void didCompleteLayoutFor(SRLayoutManagerPtr sender, SRTextContainerPtr container, bool atEnd) {
    SR_ASSERT(atEnd);
    if (atEnd) {
      SRTextStoragePtr textStorage = sender->textStorage();
      // for debug
      const int filledLength = container->textRange().location + container->textRange().length;
      if (filledLength != textStorage->getLength()) {
        // 현재 한번에 전부 레이아웃하고 있음
        SR_ASSERT(0);
      }

      SRRect usedRect = sender->usedRectForTextContainer(container);
      SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window()->contentView());
      SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(docView->getSubviews().at(0));
      SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));

      // 컬럼뷰 크기로 페이징
      SRRect frame = columnView->getFrame();
      docView->paginate(frame.size, usedRect.size);

      docView->removeSubviews();

      for (int i = 0; i < docView->getPageCount(); ++i) {
        SRRect columnRect = docView->getPage(i);
        SR_addColumnView(window(), sender, _columnCnt, columnRect);
      }
      
      docView->alignCenter();
      SR()->updateScrollBar(); // 자식뷰들 추가가 끝나면 스크롤 정보를 갱신해줘야 스크롤바 위치가 맞게 된다.
    }
  }
  UIWindowPtr window() const {
    return _window.lock();
  }
  UIWindowWeakPtr _window;
};
using LayoutMgrDelegatePtr = std::shared_ptr<LayoutMgrDelegate>;

static SRRect _getWindowRect(HWND hwnd) {
  // 스크롤바 크기만큼 추가
  RECT rc;
  ::GetClientRect((HWND)hwnd, &rc);
  SRRect windowRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);

  if (SR() && SR()->hasScrollBar(SR_SB_VERT)) {
    windowRect.size.width += SR()->scrollBarSize(SR_SB_VERT).width;
  }
  if (SR() && SR()->hasScrollBar(SR_SB_HORZ)) {
    windowRect.size.height += SR()->scrollBarSize(SR_SB_HORZ).height;
  }
  return windowRect;
}

static SRDocumentViewPtr _layoutDocument(SRLayoutManagerPtr layoutManager, SRRect docRect, SRAttributedStringPtr attrString) {
  SRDocumentViewPtr docView = SRDocumentView::create(docRect);
  //docView->setBackgroundColor(SRColor(0.5, 0.5, 0));
  docView->setBackgroundColor(SRColorGray);
  docView->setBorderColor(SRColorBlack);
  //docView->setBorderWidth(1.0f);
  // 가운데 정렬시킨다.
  //docView->setAutoresizingMask(UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin);
  docView->setAutoCenterInSuperview(true);

  SR()->setContentView(docView); // 윈도우에 컨텐츠뷰를 설정

  SRRect columnRect;
  SR_addColumnView(SR(), layoutManager, _columnCnt, columnRect); // 최초에 필요함

  _textStorage->beginEditing();
  _textStorage->setAttributedString(attrString);
  _textStorage->endEditing(); // 여기서 레이아웃팅됨

  return docView;
}

#if 0
#include <functional>
#include <iostream>
struct Foo {
  Foo(int num) : num_(num) {
  }
  void print_add(int i) const {
    std::cout << num_ + i << '\n';
  }
  int num_;
};

void print_num(int i) {
  std::cout << i << '\n';
}

struct PrintNum {
  void operator()(int i) const {
    std::cout << i << '\n';
  }
};
#endif

class ExclusionPathItem : public SRObject {
public:
  SRString _path;
  SRRect _rect; // 첫번째 컬럼뷰내의 좌표
};

using ExclusionPathItemPtr = std::shared_ptr<ExclusionPathItem>;
static std::vector<ExclusionPathItemPtr> _exclusionPathItems;

static void _drawExclusionPath(CGContextPtr ctx, SRObjectPtr obj) {
  ExclusionPathItemPtr exObj = std::dynamic_pointer_cast<ExclusionPathItem>(obj);
  SRRect rc = exObj->_rect;
  ctx->setClipBox(rc); // needs set clipbox for drawing!
  ctx->setTextPosition(SRPoint(0, rc.height())); // 이미지 높이만큼 그릴 위치를 낮춘다.
  ctx->drawImage(exObj->_path, rc);
  //ctx->drawRect(rc);
}

static void _addExclusionPath(SRAttributedStringPtr astr, SRTextContainerPtr tc, SRString path, SRRect rect) {
  ExclusionPathItemPtr item = std::make_shared<ExclusionPathItem>();
  item->_path = path;
  item->_rect = rect;
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(astr->attribute(kCTParagraphStyleAttributeName, 0, nullptr));

  SRTextBlockPtr rootBlock = *(paraStyle->getTextBlockList().begin());
  //item->_rect.origin += rootBlock->getWidthAll(); // 루트블럭 마진만큼 이동
  tc->addExclusionPath(SRTextContainer::ExclusionPath(item->_rect, item, _drawExclusionPath));
}

// WM_CREATE 에서만 호출중임
static void SR_create(void* hwnd) {
  // 가끔씩 스크롤바가 남아있는 윈도우 버그가 존재하며 해결책을 못찾음
  // SetWindowPos(), MoveWindow() 등등 구글링으로 찾은 방법이 전부 무용함
  ::ShowScrollBar((HWND)hwnd, SB_BOTH, FALSE); // 먼저 기존에 추가됐었던 스크롤바를 없앰
  ::SetScrollPos((HWND)hwnd, SB_BOTH, 0, TRUE); // 스크롤 위치 초기화(스크롤바를 감춰도 위치가 남아 있으므로)

  SRRect windowRect = _getWindowRect((HWND)hwnd); // 스크롤바 크기도 포함됨
  SR_LOGD(L"SR_create() _columnCnt=%d, windowRect=[%.1f, %.1f, %.1f, %.1f]", _columnCnt
    , windowRect.left(), windowRect.top(), windowRect.width(), windowRect.height());

  if (false && _srWindow) { // for debug
    int ocnt = SRObject::debugInfo()._obj_cnt;
    int rcnt = __sr_debug_render_cnt();
    _srWindow = nullptr; // ~UIWindow() 호출됨
    int ocnt2 = SRObject::debugInfo()._obj_cnt;
    int rcnt2 = __sr_debug_render_cnt();
  }

  _srWindow = UIWindow::create(windowRect, WIN32WindowImpl::create(hwnd)); // _srWindow 존재시 여기서 ~UIWindow() 호출됨
  SR()->setBackgroundColor(SRColorGray);

  SRAttributedStringPtr attrString = SR_getTestAttrString();

  // SRTextStorage -> SRLayoutManager -> SRTextContainer -> UITextView
  // https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/DrawingStrings.html
  _textStorage = SRTextStorage::initWithString(L"");
  SRLayoutManagerPtr layoutManager = SRLayoutManager::create();
  _textStorage->addLayoutManager(layoutManager);

  SR()->setTextStorage(_textStorage);

  SRRect docRect = windowRect; // 먼저 전체 윈도우 영역에 레이아웃을 시도함

  // 최초에 한번만 컨테이너를 추가시킴(페이징은 전체를 레이아웃후 페이지 크기로 자르게 함)
  SRSize containerSize = _getColumnRect(docRect, _columnCnt).size;
  containerSize.height = SRFloatMax;
  SRTextContainerPtr textContainer = SRTextContainer::create(containerSize);
  layoutManager->addTextContainer(textContainer);

  // SRLayoutManagerDelegate 객체 추가
  LayoutMgrDelegatePtr layoutDelegate = std::make_shared<LayoutMgrDelegate>(SR());
  layoutManager->setDelegate(layoutDelegate);

  SRDocumentViewPtr docView = _layoutDocument(layoutManager, docRect, attrString);

  // 스크롤바가 추가돼야 하는 경우, 문서는 스크롤바를 제외한 영역에 다시 한번 레이아웃시킴
  bool needsRelayout = false;
  if (docView->getBounds().size.height > windowRect.size.height) {
    docRect.size.width -= SR()->scrollBarSize(SR_SB_VERT).width;
    needsRelayout = true;
  }
  if (docView->getBounds().size.width > windowRect.size.width) {
    docRect.size.height -= SR()->scrollBarSize(SR_SB_HORZ).height;
    needsRelayout = true;
  }
  if (needsRelayout) {
    // 변경된 크기(스크롤바만큼 줄어듬)를 layoutManager내의 textContainer에 적용
    containerSize = _getColumnRect(docRect, _columnCnt).size;
    containerSize.height = SRFloatMax;
    textContainer->setSize(containerSize);
    docView = _layoutDocument(layoutManager, docRect, attrString);
  } else {
    //::ShowScrollBar((HWND)hwnd, SB_BOTH, FALSE); // @@ 간혹 스크롤바가 남아 있는 경우를 workaround -> 마찬가지임
  }

  // 데모용
  if (_demoType == SR_DEMO_TEXT) {
    UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
    textPos[0]._offset = textPos[1]._offset = _demoWrappingImagePos;
    SR()->setTextPositions(&textPos[0], &textPos[1]);
    _insertDemoImage(true); // @@
  }

  // 최초 캐럿 위치 설정
  SR()->hideCaret();
  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
  textPos[0]._offset = textPos[1]._offset = 0;
  SR()->setTextPositions(&textPos[0], &textPos[1]);
  SR()->updateCaret();
  SR()->showCaret();
}

// WM_SIZE 에서만 호출중임
static void SR_resize(void* hwnd, int w, int h) {
  //SR_LOGD(L"SR_resize() [%03d, %03d]", w, h);
  if (!SR() || SR()->contentView() == nullptr) {
    SR_ASSERT(0);
    return;
  }
  if (w == 0 && h == 0) {
    // 윈도우 최소화시 아무 처리를 하지 않게함
    return;
  }
  SR()->hideCaret();
  UIViewPtr docView = SR()->contentView();

  SRSize size = _getWindowRect((HWND)hwnd).size;
  //SR_ASSERT(size.width == w && size.height == h);
  SRSize minSize = _minWindowSize();
  if (size.width < minSize.width) {
    size.width = minSize.width;
  }
  if (size.height < minSize.height) {
    size.height = minSize.height;
  }
  SR_LOGD(L"SR_resize() setSize(%.1f, %.1f)", size.width, size.height);
  SR()->setSize(size);
  SR()->updateCaret();
  SR()->showCaret();
}

// WM_PAINT 에서만 호출중임
static void SR_redraw(HWND hwnd, HDC hdc, RECT rc) {
  if (!SR() || SR()->contentView() == nullptr) {
    return;
  }
  static int __cnt = 0;
  SR_LOGD(L"SR_redraw() (%d) [%03d, %03d, %03d, %03d]"
    , ++__cnt, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  SRRect rcDraw = SRRect(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
  CGContextPtr ctx = CGContext::create(WIN32GraphicContextImpl::create(hwnd, rcDraw, rcDraw, hdc));
  //ctx->intersectClipRect(rcClip);

  ctx->setDebugDraw(_debugDrawBorder);
  SR()->draw(ctx, rcDraw);
  ctx->flush();
}

// 윈도우(UIWindow)의 스크롤바 위치를 갱신해준다.
static void SR_vscroll(void* hwnd, int scrollCode) {
  SB_Code sbCode;
  switch (scrollCode) {
  case SB_LINEDOWN:
    sbCode = SR_SB_LINEDOWN;
    break;
  case SB_LINEUP:
    sbCode = SR_SB_LINEUP;
    break;
  case SB_PAGEDOWN:
    sbCode = SR_SB_PAGEDOWN;
    break;
  case SB_PAGEUP:
    sbCode = SR_SB_PAGEUP;
    break;
  case SB_THUMBTRACK:
    sbCode = SR_SB_THUMBTRACK;
    break;
  default:
    //SB_ENDSCROLL로 호출되고 있음
    //SR_ASSERT(0);
    return;
  }
  SR()->vscroll(sbCode);
}

static void SR_hscroll(void* hwnd, int scrollCode) {
  SB_Code sbCode;
  switch (scrollCode) {
  case SB_LINERIGHT:
    sbCode = SR_SB_LINERIGHT;
    break;
  case SB_LINELEFT:
    sbCode = SR_SB_LINELEFT;
    break;
  case SB_PAGERIGHT:
    sbCode = SR_SB_PAGERIGHT;
    break;
  case SB_PAGELEFT:
    sbCode = SR_SB_PAGELEFT;
    break;
  case SB_THUMBTRACK:
    sbCode = SR_SB_THUMBTRACK;
    break;
  default:
    //SB_ENDSCROLL로 호출되고 있음
    //SR_ASSERT(0);
    return;
  }
  SR()->hscroll(sbCode);
}

#include <Commdlg.h> // for CHOOSEFONT

static void _chooseFont(HWND hwnd) {
  CHOOSEFONT cf;            // common dialog box structure
  LOGFONT lf;        // logical font structure
  DWORD rgbCurrent;  // current text color
  HFONT hfont, hfontPrev;
  DWORD rgbPrev;

  HDC hdc = ::GetDC(hwnd);

  // set current style
  {
    memset(&lf, 0, sizeof(LOGFONT));
    lf.lfQuality = DEFAULT_QUALITY;
    UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
    SRIndex idx = std::min(textPos[0]._offset, textPos[1]._offset); // first caret
    SRRange effRange;
    auto color = std::dynamic_pointer_cast<SRColor>(
      _textStorage->attribute(kCTForegroundColorAttributeName, idx, &effRange));
    if (color) {
      rgbCurrent = RGB(color->_r * 255, color->_g * 255, color->_b * 255);
    }
    auto font = std::dynamic_pointer_cast<UIFont>(
      _textStorage->attribute(kCTFontAttributeName, idx, &effRange));
    if (font) {
      UIFontInfoPtr fInfo = font->fontInfo();
      ::wcsncpy(lf.lfFaceName, fInfo->_familyName, LF_FACESIZE);
      lf.lfHeight = -MulDiv(fInfo->_size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
      if (fInfo->_traits & SRBoldFontMask)
        lf.lfWeight = FW_BOLD;
      if (fInfo->_traits & SRItalicFontMask)
        lf.lfItalic = 1;
    }
  }

  // Initialize CHOOSEFONT
  ::ZeroMemory(&cf, sizeof(cf));
  cf.lStructSize = sizeof(cf);
  cf.hwndOwner = hwnd;
  cf.lpLogFont = &lf;
  cf.rgbColors = rgbCurrent;
  cf.Flags = CF_SCREENFONTS | CF_EFFECTS;

  if (::ChooseFont(&cf) == TRUE) {
    hfont = ::CreateFontIndirect(cf.lpLogFont);

    _textStorage->beginEditing();
    UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
    SRRange selectedRange(textPos[0]._offset, textPos[1]._offset - textPos[0]._offset);

    //FontStyle(lf->lfFaceName, lf->lfWeight, lf->lfHeight, lf->lfItalic, lf->lfUnderline, lf->lfStrikeOut, cf->rgbColors);
    LOGFONT* lf = cf.lpLogFont;
    SRFloat fontSize = -MulDiv(lf->lfHeight, 72, ::GetDeviceCaps(hdc, LOGPIXELSY));
    std::wstring fontName = lf->lfFaceName;
    if (lf->lfItalic) {
      fontName += L" Italic";
    }
    if (lf->lfWeight > 400) {
      fontName += L" Bold";
    }
    UIFontPtr fontIn = UIFont::fontWithName(fontName, fontSize);
    //auto dict = SRObjectDictionary::create();
    //dict->addValue(kCTFontAttributeName, fontIn);
    _textStorage->setAttribute(kCTFontAttributeName, fontIn, selectedRange);
    _textStorage->setAttribute(kCTForegroundColorAttributeName, std::make_shared<SRColor>(
      GetRValue(cf.rgbColors) / 255, GetGValue(cf.rgbColors) / 255, GetBValue(cf.rgbColors) / 255)
      , selectedRange);

    _textStorage->endEditing(); // 여기서 레이아웃팅됨
    SR()->invalidateRect(NULL, false);
  }
  ::ReleaseDC(hwnd, hdc);
}

static SRRange _rangeOfLine(const std::wstring& str, SRIndex idx) {
  //for (idx = 0; idx < str.length(); ++idx) {
  size_t eol = str.find('\n', idx);
  if (eol == std::string::npos) {
    SR_ASSERT(0);
    return SRRange();
  }
  size_t sol = 0;
  if (idx > 0) {
    size_t found = str.rfind('\n', idx - 1);
    if (found != std::string::npos) {
      sol = found + 1; // set next to the newline character
    }
  }
  return SRRange(sol, (eol - sol + 1));
}

static void _setAlignment(SRInt align) {
  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
  //SRRange selectedRange(textPos[0]._offset, textPos[1]._offset - textPos[0]._offset);
  SRIndex idx = textPos[0]._offset;
  SRRange effRange;
  auto paragraphStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    _textStorage->attribute(kCTParagraphStyleAttributeName, idx, &effRange));
  CTTextAlignment textAlign = kCTLeftTextAlignment;
  if (align == 1) {
    textAlign = kCTCenterTextAlignment;
  } else if (align == 2) {
    textAlign = kCTRightTextAlignment;
  }
  auto newParaStyle = paragraphStyle->copy();
  //auto newParaStyle = CTParagraphStyle::create(*paragraphStyle);
  newParaStyle->setValueForSpecifier(kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &textAlign);
  //effRange.length = 6;

  std::wstring str = _textStorage->getString()->data();
  SRRange range0 = _rangeOfLine(str, textPos[0]._offset);
  SRRange range1 = _rangeOfLine(str, textPos[1]._offset);
  effRange.location = range0.location;
  effRange.length = range1.rangeMax() - effRange.location;

  SR()->hideCaret();
  _textStorage->beginEditing();
  _textStorage->setAttribute(kCTParagraphStyleAttributeName, newParaStyle, effRange);
  _textStorage->endEditing(); // 여기서 레이아웃팅됨
  SR()->invalidateRect(NULL, false);
  SR()->updateCaret();
  SR()->showCaret();
}

static void _showPopupMenu(HWND hWnd, POINT point) {
  ::ClientToScreen(hWnd, &point);

  HMENU hMenu = ::CreatePopupMenu();
  SR_ASSERT(hMenu);
  if (!hMenu)
    return;

  ::HideCaret((HWND)hWnd);

  ::AppendMenu(hMenu, MF_STRING, IDM_SR_CHOOSE_FONT, _T("폰트 설정(&F1)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_LEFT, _T("왼쪽 정렬(&F2)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_CENTER, _T("가운데 정렬(&F3)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_RIGHT, _T("오른쪽 정렬(&F4)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN1, _T("1단(&F5)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN2, _T("2단(&F6)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN3, _T("3단(&F7)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_0, _T("데모0 None(&F8)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_1, _T("데모1 Table(&F9)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_2, _T("데모2 Text(&F11)"));

  //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("잘라내기(&T)\tCtrl+X"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_COPY, _T("복사(&C)\tCtrl+C"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_PASTE, _T("붙여넣기(&P)\tCtrl+V"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_SELECTALL, _T("모두 선택(&A)\tCtrl+A"));

  ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
  ::TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, point.x, point.y, 0, hWnd, NULL);

  ::DestroyMenu(hMenu);
  ::ShowCaret((HWND)hWnd);
}

static void _changeColumnCnt(HWND hWnd, SRInt columnCnt) {
  if (_columnCnt == columnCnt)
    return;
  _columnCnt = columnCnt;
  UITextPosition textPos[2]; // backup
  textPos[0] = SR()->textPositions()[0];
  textPos[1] = SR()->textPositions()[1];

  SR()->destroyCaret();
  SR_create(hWnd);
  SR()->invalidateRect(NULL, false);
  SR()->setTextPositions(&textPos[0], &textPos[1]); // restore
  SR()->updateCaret();
  SR()->showCaret();
}

static void _changePageSize(HWND hWnd, SRSize pageSize) {
  if (_pageSize == pageSize)
    return;
  _pageSize = pageSize;
  UITextPosition textPos[2]; // backup
  textPos[0] = SR()->textPositions()[0];
  textPos[1] = SR()->textPositions()[1];

  SR()->destroyCaret();
  SR_create(hWnd);
  SR()->invalidateRect(NULL, false);
  SR()->setTextPositions(&textPos[0], &textPos[1]); // restore
  SR()->updateCaret();
  SR()->showCaret();
}

static SRAttributedStringPtr _tableCellAttributedString(const SRTextBlockList& parentBlocks,
  const SRString& string, SRTextTablePtr table,
  SRColor backgroundColor, SRColor borderColor, SRInt startingRow,
  SRInt rowSpan, SRInt startingColumn, SRInt columnSpan,
  SRFloat border, SRFloat padding) {
  SR_ASSERT((startingRow + rowSpan) <= table->_numberOfRows);
  SR_ASSERT((startingColumn + columnSpan) <= table->_numberOfColumns);
  // 테이블(블럭)은 셀 블럭에 포함시킴
  SRTextTableCellBlockPtr cellBlock = SRTextTableCellBlock::create(table, startingRow, rowSpan, startingColumn, columnSpan);

  cellBlock->setBackgroundColor(backgroundColor);
  cellBlock->setBorderColor(borderColor);
  cellBlock->setWidth(border, SRTextTableCellBlock::Border); // html table 의 경우 테이블의 경계선 두께가 1 이상이면, 셀 경계선은 무조건 1 두께를 갖음
  cellBlock->setWidth(padding, SRTextTableCellBlock::Padding); // 셀 경계선으로부터의 패딩

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭부터 추가
  paragraphStyle->addTextBlock(cellBlock); // 셀 블럭 추가(동일한 테이블(블럭)을 포함함)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static void _insertTable(SRInt rowCnt, SRInt colCnt, SRFloat border, SRFloat padding) {
  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };

  // 현재 선택영역에 테이블 추가
  SRRange replaceRange(textPos[0]._offset, textPos[1]._offset - textPos[0]._offset);
  const SRTextBlockList& parentBlocks = getTextBlockList(_textStorage, replaceRange.location);
  //_textStorage->replaceCharacters(replaceRange, tableAttributedString(parentBlocks, 2));

  SRAttributedStringPtr tableString = SRAttributedString::create(L""); // "\n\n"
  SRTextTablePtr table = SRTextTable::create();
  //table->setWidth(0.0, SRTextTableCellBlock::Margin); // 불필요??? html table 속성에는 없는듯함
  table->setWidth(border, SRTextTableCellBlock::Border);
  table->setWidth(padding, SRTextTableCellBlock::Padding); // html table cellspacing 에 해당함(기본 2)

  if (1) {
    // 테이블 이전에 부모 블럭에 텍스트 추가(텍스트 블럭 추가)
    SRAttributedStringPtr str = SRAttributedString::create(L"\n"); // "\n\n"
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // 부모 블럭만 추가
    str->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
  }

  table->setNumberOfColumns(rowCnt);
  table->setNumberOfRows(colCnt);

  // 테이블 셀들에는 부모 블럭 & 셀 블럭(테이블(블럭)이 포함됨) 속성 추가
  for (SRIndex row = 0; row < rowCnt; ++row) {
    for (SRIndex col = 0; col < colCnt; ++col) {
      tableString->append(_tableCellAttributedString(parentBlocks, L"\n", table
        , SRColorGreen, SRColorRed, row, 1, col, 1, border, padding));
    }
  }

  auto font = std::dynamic_pointer_cast<UIFont>(
    _textStorage->attribute(kCTFontAttributeName, textPos[0]._offset, nullptr));
  if (font) {
    tableString->setAttribute(kCTFontAttributeName, font, SRRange(0, tableString->getLength()));
  }

  _textStorage->beginEditing();
  _textStorage->replaceCharacters(replaceRange, tableString);
  _textStorage->endEditing();
  SR()->invalidateRect(NULL, false);
}

static void _insertDemoImage(SRBool isWrap) {
  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(SR()->contentView());
  SRColumnViewPtr columnView = docView->firstColumnView();
  SRTextContainerPtr tc = columnView->textContainer();

  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
  SRRect caret = SR()->caretRect(textPos[0]);
  SRPoint point = caret.origin;
  SRPoint orgPoint = point;
  UIViewPtr hitView = SR()->hitTest(UIEvent(MOUSE_MOVE, point.x, point.y)); // 마우스 좌표로부터 뷰를 찾음
  if (hitView) {
    point = SR()->convertTo(SR()->localCoordinate(point), hitView);
    // root render 좌표로 변환
    SRColumnViewPtr colView = std::dynamic_pointer_cast<SRColumnView>(hitView);
    point.y += colView->_partialRect.origin.y;

    _textStorage->beginEditing();
    if (isWrap) {
      _addExclusionPath(_textStorage, tc, L".\\test_259x194_1.bmp", SRRect(point.x, point.y, 259 + 10, 194));
    } else {
      _insertImage(_textStorage, SRRange(textPos[0]._offset, textPos[1]._offset - textPos[0]._offset)
        , L".\\test.bmp", SRSize(50, 50));
    }
    _textStorage->endEditing();
    SR()->invalidateRect(NULL, false);
  } else {
    SR_ASSERT(0);
  }
}

static void _changeDemoType(HWND hWnd, SR_DEMO_TYPE demoType) {
  if (_demoType == demoType) {
    return;
  }
  _demoType = demoType;
  _demoChanged = true;
  SR_create(hWnd);
  _demoChanged = false;
  SR()->invalidateRect(NULL, false);
}

static void _changeDrawBorder(SRBool on) {
  if (_debugDrawBorder == on) {
    return;
  }
  _debugDrawBorder = on;
  SR()->invalidateRect(NULL, false);
}

static bool _isKeyDown(int vKey) {
  return ::GetAsyncKeyState(vKey) & 0x8000;
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
   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //  0, 0, 500 + 16, 300 + 58, NULL, NULL, hInstance, NULL); // NC size: +16, +58
   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
     0, 0, _defaultPageSize.width + 56, _defaultPageSize.height + 78
     , NULL, NULL, hInstance, NULL); // NC size: +16, +58

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
    case IDM_SR_CHOOSE_FONT:
      _chooseFont(hWnd);
      break;
    case IDM_SR_ALIGN_LEFT:
    case IDM_SR_ALIGN_CENTER:
    case IDM_SR_ALIGN_RIGHT:
      _setAlignment(wmId - IDM_SR_ALIGN_LEFT);
      break;
    case IDM_SR_MULTI_COLUMN1:
    case IDM_SR_MULTI_COLUMN2:
    case IDM_SR_MULTI_COLUMN3:
      _changeColumnCnt(hWnd, wmId - IDM_SR_MULTI_COLUMN1 + 1);
      break;
    case IDM_SR_DEMO_0:
      _changeDemoType(hWnd, SR_DEMO_NONE);
      break;
    case IDM_SR_DEMO_1:
      _changeDemoType(hWnd, SR_DEMO_TABLE_LIST);
      break;
    case IDM_SR_DEMO_2:
      _changeDemoType(hWnd, SR_DEMO_TEXT);
      break;
    case IDM_SR_PAGE_A4:
      /*
      A4: 21cm x 29.7cm, 792px * 1120px
      A5: 14.8cm x 21cm, 558px * 792px
      cm * 37.7 = px
      */
      _changePageSize(hWnd, SRSize(792, 1120));
      break;
    case IDM_SR_PAGE_A5:
      _changePageSize(hWnd, SRSize(558, 792));
      break;
    case IDM_SR_PAGE_Test:
      _changePageSize(hWnd, _defaultPageSize);
      break;
    case IDM_SR_DEBUG_BORDER_ON:
      _changeDrawBorder(true);
      break;
    case IDM_SR_DEBUG_BORDER_OFF:
      _changeDrawBorder(false);
      break;
    case IDM_SR_INSERT_TABLE1:
      //_textStorage->beginEditing();
      _insertTable(2, 2, 1.0, 2.0);
      //_insertTable(2, 2, 0.0, 0.0);
      //_textStorage->endEditing();
      //SR()->invalidateRect(NULL, false);
      break;
    case IDM_SR_INSERT_TABLE2:
      _insertTable(2, 2, 1.0, 0.0);
      break;
    case IDM_SR_INSERT_IMAGE_INLINE1:
      _insertDemoImage(false);
      break;
    case IDM_SR_INSERT_IMAGE_WRAP1:
      _insertDemoImage(true);
      break;
    default:
      return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
  case WM_PAINT:
  {
    SR_PROF_TIME_START(L"WM_PAINT");
    hdc = BeginPaint(hWnd, &ps);
    SR_redraw(hWnd, hdc, ps.rcPaint); // ~20ms
    EndPaint(hWnd, &ps);
    SR_PROF_TIME_END(L"WM_PAINT");
  }
    break;
  case WM_ERASEBKGND:
    return TRUE; // 배경을 지우지 않게 하여, 화면 갱신시 깜박임을 제거함
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
  case WM_MOUSEWHEEL:
  {
    short zDelta = HIWORD(wParam);
    if (zDelta < 0) {
      SR_vscroll(hWnd, SB_LINEDOWN);
    } else {
      SR_vscroll(hWnd, SB_LINEUP);
    }
  }
  break;
  case WM_SETCURSOR:
  {
    //SR_LOGD(L"[cursor] SREditor::WM_SETCURSOR");
    if (LOWORD(lParam) == HTCLIENT) {
      POINT pt;
      ::GetCursorPos(&pt);
      ::ScreenToClient(hWnd, &pt);
      if (SR()) {
        //SR_LOGD(L"[cursor] SREditor::WM_SETCURSOR call cursorUpdate()");
        UIEvent event(SET_CURSOR, pt.x, pt.y);
        SR()->cursorUpdate(event);
        return 0;
      } else {
        SR_ASSERT(0);
      }
    }
  }
  return DefWindowProc(hWnd, message, wParam, lParam);
  case WM_LBUTTONDOWN:
  {
    SRPoint point(LOWORD(lParam), HIWORD(lParam));
    SRUInt metaKey = NO_META;
    if (_isKeyDown(VK_SHIFT)) {
      metaKey |= SHIFT_KEY;
    }
    UIEvent event(LEFT_MOUSE_DOWN, point.x, point.y, metaKey);
    SR()->mouseDown(event);
    ::SetCapture(hWnd);
  }
  break;
  case WM_MOUSEMOVE:
  {
    SRPoint point(LOWORD(lParam), HIWORD(lParam));

    //	*CAUTION* When the mouse moves to the upper left, the point changes from 0 to 65534
    //	though it should be a negative value, so we changed value type as short.
    //point.x = std::max((short)0, (short)point.x);
    //point.y = std::max((short)0, (short)point.y); // 윈도우 위로 드래그시 0이 되서 위로 스크롤이 안되고 있다.
    point.x = (short)point.x;
    point.y = (short)point.y;

    UIEvent event(MOUSE_MOVE, point.x, point.y);
    event._pressedMouseButtons |= _isKeyDown(VK_LBUTTON) ? LEFT_MOUSE_BUTTON : NO_MOUSE_BUTTON;
    SR()->mouseMoved(event);
  }
  break;
  case WM_LBUTTONUP:
  {
    SRPoint point(LOWORD(lParam), HIWORD(lParam));
    UIEvent event(LEFT_MOUSE_UP, point.x, point.y);
    SR()->mouseUp(event);
    ::ReleaseCapture();
  }
  break;
  case WM_RBUTTONUP:
  {
    //_showPopupMenu(hWnd, { LOWORD(lParam), HIWORD(lParam) });
    //::MoveWindow(hWnd, 0, 0, 600, 300, TRUE);
  }
  break;
  case WM_CHAR:
  {
    SR_LOGD(L""); SR_PROF_TIME_START(L"WM_CHAR");
    SR()->hideCaret();
    UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
    //_textStorage->beginEditing();
    std::wstring str(1, (wchar_t)wParam);
    if (wParam == 0x08) { // Backspace
      str = L"";
      if (!SR()->hasSelection()) {
        // 블럭이 없는 경우에는 이전 문자 하나만 삭제
        textPos[0]._offset = std::max(0, textPos[0]._offset - 1);
      }
    } else if (wParam == 0x0D) { // Carriage return
      str = L"\n";
    }
    SR()->replace(UITextRange(textPos[0], textPos[1]), str); // ~40ms
    SR_PROF_TIME_STAMP(L"WM_CHAR:replace()");
    //_textStorage->endEditing(); // 여기서 레이아웃팅됨
    SR()->invalidateRect(NULL, false);
    SR()->updateWindow();

    UITextPosition newTextPos = textPos[0];
    // 추가된 글자 수 1만큼 오른쪽으로 캐럿을 이동
    if (!(wParam == 0x08)) { // Backspace
      newTextPos._offset += 1;
    }
    SR()->setTextPositions(&newTextPos, &newTextPos);
    SR()->updateCaret();
    SR()->showCaret();
    SR_PROF_TIME_END(L"WM_CHAR"); SR_LOGD(L"");
  }
  break;
  case WM_KEYDOWN: // 거의 항상 0ms
  {
    //SR_PROF_TIME_START(L"WM_KEYDOWN");
    UIKeyCode keyCode = NO_KEY;
    if (wParam == VK_LEFT) {
      keyCode = LEFT_KEY;
    } else if (wParam == VK_RIGHT) {
      keyCode = RIGHT_KEY;
    } else if (wParam == VK_UP) {
      keyCode = UP_KEY;
    } else if (wParam == VK_DOWN) {
      keyCode = DOWN_KEY;
    } else if (wParam == VK_DELETE) {
      keyCode = DELETE_KEY;
    } else if (wParam == VK_HOME) {
      keyCode = HOME_KEY;
    } else if (wParam == VK_END) {
      keyCode = END_KEY;
    } else if (wParam == 0x41) { // 'A'
      keyCode = A_KEY;
    } else if (wParam == VK_F1) {
      _chooseFont(hWnd);
      break;
    } else if (wParam == VK_F2) {
      _setAlignment(0);
      break;
    } else if (wParam == VK_F3) {
      _setAlignment(1);
      break;
    } else if (wParam == VK_F4) {
      _setAlignment(2);
      break;
    } else if (wParam == VK_F5) {
      _changeColumnCnt(hWnd, 1);
      break;
    } else if (wParam == VK_F6) {
      _changeColumnCnt(hWnd, 2);
      break;
    } else if (wParam == VK_F7) {
      _changeColumnCnt(hWnd, 3);
      break;
    } else if (wParam == VK_F8) {
      _changeDemoType(hWnd, SR_DEMO_NONE);
      break;
    } else if (wParam == VK_F9) {
      _changeDemoType(hWnd, SR_DEMO_TABLE_LIST);
      break;
    } else if (wParam == VK_F11) {
      _changeDemoType(hWnd, SR_DEMO_TEXT);
      break;
    }
    SRUInt metaKey = NO_META;
    if (_isKeyDown(VK_SHIFT)) { metaKey |= SHIFT_KEY; }
    if (_isKeyDown(VK_CONTROL)) { metaKey |= CTRL_KEY; }
    UIEvent keyEvent(KEY_DOWN, keyCode, metaKey);
    SR()->keyDown(keyEvent);
    //SR_PROF_TIME_END(L"WM_KEYDOWN");
  }
  break;
  case WM_SETFOCUS:
    SR()->createCaret();
    SR()->showCaret();
    break;
  case WM_KILLFOCUS:
    // The window is losing the keyboard focus, so destroy the caret.
    SR()->destroyCaret();
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
