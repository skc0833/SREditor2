// SREditor2.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#define NOMINMAX  // for minwindef.h
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
static const SRFloat _spaceBetweenColumn = 10; // �÷� ������ ����

static void _insertDemoImage(SRBool isWrap);

// �ּ� ������ ũ�⸦ ���Ͻ�Ŵ(������ ũ�Ⱑ �ּ� ��Ʈ ������ ��� crash �߻��ϹǷ�)
SRSize _minWindowSize() {
  SRSize size(14, 14); // @@ �ӽ÷� �ּ� ��Ʈ ũ��(14) �̻����� ������(���� 1�� ���� ���� �̻����� ����???)
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

// �Ӽ� ����� ���ڿ��� ������ ��������
static SRAttributedStringPtr SR_getTestAttrString() {
  if (!_demoChanged && _textStorage) {
    return _textStorage;
  }

  SRAttributedStringPtr attrStr = SRAttributedString::create(L"\n");

  SRTextBlockPtr rootBlock = SRTextBlock::create(SRTextBlock::Text);
  rootBlock->setWidth(0.0, SRTextBlock::Margin); // �÷��� ��輱 ~ ������ ��輱
  //rootBlock->setWidth(1.0, SRTextBlock::Border); // ������ ��輱 �β�
  rootBlock->setWidth(10.0, SRTextBlock::Padding); // ������ ��輱 ~ ������
  SRTextBlockList rootBlocks;
  rootBlocks.push_back(rootBlock);

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(rootBlocks);
  attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));

  UIFontPtr font = UIFont::fontWithName(L"����ü", 24.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
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

// ������ ũ�� ȹ��
static SRRect _getPageRect(SRRect docViewRect) {
  SRRect pageRect = docViewRect;
  if (_pageSize.height > 0) {
    pageRect.size = _pageSize; // ������ ũ�⸦ ���� ũ��� ����
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
  // ������ ũ�⿡�� ������ ������ �� ���� ũ�⸦ ����
  SRRect columnRect = pageRect;
  columnRect.size.width -= _pageLayoutMargin.left + _pageLayoutMargin.right;
  columnRect.size.height -= _pageLayoutMargin.top + _pageLayoutMargin.bottom;
  // �÷� ������ ������ ���� �÷� ���� - 1 ��ŭ ���� ����
  columnRect.size.width -= (columnCnt - 1) * _spaceBetweenColumn;
  columnRect.size.width /= columnCnt;
  SR_ASSERT(columnRect.origin.x == 0 && columnRect.origin.y == 0);
  return columnRect;
}

// ������ �������� �÷��� �߰��Ѵ�.
static void SR_addColumnView(UIWindowPtr window, SRLayoutManagerPtr layoutMgr, int columnCnt, SRRect partialRect) {
  const int COLUMN_CNT = columnCnt;

  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window->contentView());
  docView->setLayoutMargins(_docLayoutMargin);
  UIViewList pageViews = docView->getSubviews();
  SRPageViewPtr lastPageView;
  UIViewList columnViews;

  // ������ ũ�� ȹ��
  SRRect pageRect = _getPageRect(docView->getFrame());

  if (pageViews.size() > 0) {
    // ������ �������� ���� ȹ��
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
    // �÷��� ���ų� ���� ���, ������ �߰�
    SRPageViewPtr pageView = SRPageView::create(pageRect, COLUMN_CNT);
    //pageView->setBackgroundColor(SRColor(0, 0.5, 0.5)); // û�ϻ�
    pageView->setBackgroundColor(SRColorWhite);
    pageView->setBorderColor(SRColorBlue);
    pageView->setBorderWidth(1.0f); // ������ ��輱
    pageView->setLayoutMargins(_pageLayoutMargin); // �������� �ȿ� �ڽĺ�(�÷���)�� ��ġ�� ������ ����
    // ����, ���� ������ ���̸� Ŭ���� ���� �������� Ŭ���ǵ��� ����(like ms-word)
    SRRect hitTestRect = pageRect;
    hitTestRect.size.height += _docLayoutMargin.bottom;
    docView->addSubview(pageView);
    docView->sizeToFit(); // �ڽĺ� ũ�⿡ �����.
    lastPageView = pageView;
  } else {
    // �� ��° ���� �÷��� �ٷ� ���� �÷��� ������ �������� ����
    SRColumnViewPtr lastColumnView = std::dynamic_pointer_cast<SRColumnView>(*(columnViews.rbegin()));
    columnRect = lastColumnView->getFrame();
    columnRect.origin.x = columnRect.left() + columnRect.width();
    columnRect.origin.x += _spaceBetweenColumn; // �÷� ������ŭ ������ ���
    colIndex = lastColumnView->_colIndex + 1;
  }

  // �÷��� �߰�
  SRColumnViewPtr columnView = SRColumnView::create(columnRect, colIndex);
  const SRTextContainerList& tcs = layoutMgr->textContainers();
  SR_ASSERT(tcs.size() == 1);
  SRTextContainerPtr textContainer = tcs.at(0);
  columnView->init(textContainer);
  if (partialRect.size.width == 0) {
    partialRect.size = columnRect.size; // ���ʿ� �÷� ũ��� �����ϱ� ������
  }
  columnView->setPartialRect(partialRect); // ���� �÷��䰡 ǥ���� ù��° �÷��� ���� ����(����¡)
  columnView->setBackgroundColor(SRColorWhite);
  columnView->setBorderColor(SRColorGreen);
  //columnView->setBorderWidth(1.0f); // �÷� ��輱
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
        // ���� �ѹ��� ���� ���̾ƿ��ϰ� ����
        SR_ASSERT(0);
      }

      SRRect usedRect = sender->usedRectForTextContainer(container);
      SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(window()->contentView());
      SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(docView->getSubviews().at(0));
      SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));

      // �÷��� ũ��� ����¡
      SRRect frame = columnView->getFrame();
      docView->paginate(frame.size, usedRect.size);

      docView->removeSubviews();

      for (int i = 0; i < docView->getPageCount(); ++i) {
        SRRect columnRect = docView->getPage(i);
        SR_addColumnView(window(), sender, _columnCnt, columnRect);
      }
      
      docView->alignCenter();
      SR()->updateScrollBar(); // �ڽĺ�� �߰��� ������ ��ũ�� ������ ��������� ��ũ�ѹ� ��ġ�� �°� �ȴ�.
    }
  }
  UIWindowPtr window() const {
    return _window.lock();
  }
  UIWindowWeakPtr _window;
};
using LayoutMgrDelegatePtr = std::shared_ptr<LayoutMgrDelegate>;

static SRRect _getWindowRect(HWND hwnd) {
  // ��ũ�ѹ� ũ�⸸ŭ �߰�
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
  // ��� ���Ľ�Ų��.
  //docView->setAutoresizingMask(UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin);
  docView->setAutoCenterInSuperview(true);

  SR()->setContentView(docView); // �����쿡 �������並 ����

  SRRect columnRect;
  SR_addColumnView(SR(), layoutManager, _columnCnt, columnRect); // ���ʿ� �ʿ���

  _textStorage->beginEditing();
  _textStorage->setAttributedString(attrString);
  _textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�

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
  SRRect _rect; // ù��° �÷��䳻�� ��ǥ
};

using ExclusionPathItemPtr = std::shared_ptr<ExclusionPathItem>;
static std::vector<ExclusionPathItemPtr> _exclusionPathItems;

static void _drawExclusionPath(CGContextPtr ctx, SRObjectPtr obj) {
  ExclusionPathItemPtr exObj = std::dynamic_pointer_cast<ExclusionPathItem>(obj);
  SRRect rc = exObj->_rect;
  ctx->setClipBox(rc); // needs set clipbox for drawing!
  ctx->setTextPosition(SRPoint(0, rc.height())); // �̹��� ���̸�ŭ �׸� ��ġ�� �����.
  ctx->drawImage(exObj->_path, rc);
  //ctx->drawRect(rc);
}

static void _addExclusionPath(SRAttributedStringPtr astr, SRTextContainerPtr tc, SRString path, SRRect rect) {
  ExclusionPathItemPtr item = std::make_shared<ExclusionPathItem>();
  item->_path = path;
  item->_rect = rect;
  CTParagraphStylePtr paraStyle = std::dynamic_pointer_cast<CTParagraphStyle>(astr->attribute(kCTParagraphStyleAttributeName, 0, nullptr));

  SRTextBlockPtr rootBlock = *(paraStyle->getTextBlockList().begin());
  //item->_rect.origin += rootBlock->getWidthAll(); // ��Ʈ�� ������ŭ �̵�
  tc->addExclusionPath(SRTextContainer::ExclusionPath(item->_rect, item, _drawExclusionPath));
}

// WM_CREATE ������ ȣ������
static void SR_create(void* hwnd) {
  // ������ ��ũ�ѹٰ� �����ִ� ������ ���װ� �����ϸ� �ذ�å�� ��ã��
  // SetWindowPos(), MoveWindow() ��� ���۸����� ã�� ����� ���� ������
  ::ShowScrollBar((HWND)hwnd, SB_BOTH, FALSE); // ���� ������ �߰��ƾ��� ��ũ�ѹٸ� ����
  ::SetScrollPos((HWND)hwnd, SB_BOTH, 0, TRUE); // ��ũ�� ��ġ �ʱ�ȭ(��ũ�ѹٸ� ���絵 ��ġ�� ���� �����Ƿ�)

  SRRect windowRect = _getWindowRect((HWND)hwnd); // ��ũ�ѹ� ũ�⵵ ���Ե�
  SR_LOGD(L"SR_create() _columnCnt=%d, windowRect=[%.1f, %.1f, %.1f, %.1f]", _columnCnt
    , windowRect.left(), windowRect.top(), windowRect.width(), windowRect.height());

  if (false && _srWindow) { // for debug
    int ocnt = SRObject::debugInfo()._obj_cnt;
    int rcnt = __sr_debug_render_cnt();
    _srWindow = nullptr; // ~UIWindow() ȣ���
    int ocnt2 = SRObject::debugInfo()._obj_cnt;
    int rcnt2 = __sr_debug_render_cnt();
  }

  _srWindow = UIWindow::create(windowRect, WIN32WindowImpl::create(hwnd)); // _srWindow ����� ���⼭ ~UIWindow() ȣ���
  SR()->setBackgroundColor(SRColorGray);

  SRAttributedStringPtr attrString = SR_getTestAttrString();

  // SRTextStorage -> SRLayoutManager -> SRTextContainer -> UITextView
  // https://developer.apple.com/library/content/documentation/Cocoa/Conceptual/TextLayout/Tasks/DrawingStrings.html
  _textStorage = SRTextStorage::initWithString(L"");
  SRLayoutManagerPtr layoutManager = SRLayoutManager::create();
  _textStorage->addLayoutManager(layoutManager);

  SR()->setTextStorage(_textStorage);

  SRRect docRect = windowRect; // ���� ��ü ������ ������ ���̾ƿ��� �õ���

  // ���ʿ� �ѹ��� �����̳ʸ� �߰���Ŵ(����¡�� ��ü�� ���̾ƿ��� ������ ũ��� �ڸ��� ��)
  SRSize containerSize = _getColumnRect(docRect, _columnCnt).size;
  containerSize.height = SRFloatMax;
  SRTextContainerPtr textContainer = SRTextContainer::create(containerSize);
  layoutManager->addTextContainer(textContainer);

  // SRLayoutManagerDelegate ��ü �߰�
  LayoutMgrDelegatePtr layoutDelegate = std::make_shared<LayoutMgrDelegate>(SR());
  layoutManager->setDelegate(layoutDelegate);

  SRDocumentViewPtr docView = _layoutDocument(layoutManager, docRect, attrString);

  // ��ũ�ѹٰ� �߰��ž� �ϴ� ���, ������ ��ũ�ѹٸ� ������ ������ �ٽ� �ѹ� ���̾ƿ���Ŵ
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
    // ����� ũ��(��ũ�ѹٸ�ŭ �پ��)�� layoutManager���� textContainer�� ����
    containerSize = _getColumnRect(docRect, _columnCnt).size;
    containerSize.height = SRFloatMax;
    textContainer->setSize(containerSize);
    docView = _layoutDocument(layoutManager, docRect, attrString);
  } else {
    //::ShowScrollBar((HWND)hwnd, SB_BOTH, FALSE); // @@ ��Ȥ ��ũ�ѹٰ� ���� �ִ� ��츦 workaround -> ����������
  }

  // �����
  if (_demoType == SR_DEMO_TEXT) {
    UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
    textPos[0]._offset = textPos[1]._offset = _demoWrappingImagePos;
    SR()->setTextPositions(&textPos[0], &textPos[1]);
    _insertDemoImage(true); // @@
  }

  // ���� ĳ�� ��ġ ����
  SR()->hideCaret();
  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };
  textPos[0]._offset = textPos[1]._offset = 0;
  SR()->setTextPositions(&textPos[0], &textPos[1]);
  SR()->updateCaret();
  SR()->showCaret();
}

// WM_SIZE ������ ȣ������
static void SR_resize(void* hwnd, int w, int h) {
  //SR_LOGD(L"SR_resize() [%03d, %03d]", w, h);
  if (!SR() || SR()->contentView() == nullptr) {
    SR_ASSERT(0);
    return;
  }
  if (w == 0 && h == 0) {
    // ������ �ּ�ȭ�� �ƹ� ó���� ���� �ʰ���
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

// WM_PAINT ������ ȣ������
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

// ������(UIWindow)�� ��ũ�ѹ� ��ġ�� �������ش�.
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
    //SB_ENDSCROLL�� ȣ��ǰ� ����
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
    //SB_ENDSCROLL�� ȣ��ǰ� ����
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

    _textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
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
  _textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
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

  ::AppendMenu(hMenu, MF_STRING, IDM_SR_CHOOSE_FONT, _T("��Ʈ ����(&F1)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_LEFT, _T("���� ����(&F2)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_CENTER, _T("��� ����(&F3)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_ALIGN_RIGHT, _T("������ ����(&F4)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN1, _T("1��(&F5)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN2, _T("2��(&F6)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_MULTI_COLUMN3, _T("3��(&F7)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_0, _T("����0 None(&F8)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_1, _T("����1 Table(&F9)"));
  ::AppendMenu(hMenu, MF_STRING, IDM_SR_DEMO_2, _T("����2 Text(&F11)"));

  //::AppendMenu(hMenu, MF_STRING, IDM_SR_CUT, _T("�߶󳻱�(&T)\tCtrl+X"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_COPY, _T("����(&C)\tCtrl+C"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_PASTE, _T("�ٿ��ֱ�(&P)\tCtrl+V"));
  //::AppendMenu(hMenu, MF_STRING, IDM_SR_SELECTALL, _T("��� ����(&A)\tCtrl+A"));

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
  // ���̺�(��)�� �� ���� ���Խ�Ŵ
  SRTextTableCellBlockPtr cellBlock = SRTextTableCellBlock::create(table, startingRow, rowSpan, startingColumn, columnSpan);

  cellBlock->setBackgroundColor(backgroundColor);
  cellBlock->setBorderColor(borderColor);
  cellBlock->setWidth(border, SRTextTableCellBlock::Border); // html table �� ��� ���̺��� ��輱 �β��� 1 �̻��̸�, �� ��輱�� ������ 1 �β��� ����
  cellBlock->setWidth(padding, SRTextTableCellBlock::Padding); // �� ��輱���κ����� �е�

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
  paragraphStyle->addTextBlock(cellBlock); // �� �� �߰�(������ ���̺�(��)�� ������)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static void _insertTable(SRInt rowCnt, SRInt colCnt, SRFloat border, SRFloat padding) {
  UITextPosition textPos[2] = { SR()->textPositions()[0], SR()->textPositions()[1] };

  // ���� ���ÿ����� ���̺� �߰�
  SRRange replaceRange(textPos[0]._offset, textPos[1]._offset - textPos[0]._offset);
  const SRTextBlockList& parentBlocks = getTextBlockList(_textStorage, replaceRange.location);
  //_textStorage->replaceCharacters(replaceRange, tableAttributedString(parentBlocks, 2));

  SRAttributedStringPtr tableString = SRAttributedString::create(L""); // "\n\n"
  SRTextTablePtr table = SRTextTable::create();
  //table->setWidth(0.0, SRTextTableCellBlock::Margin); // ���ʿ�??? html table �Ӽ����� ���µ���
  table->setWidth(border, SRTextTableCellBlock::Border);
  table->setWidth(padding, SRTextTableCellBlock::Padding); // html table cellspacing �� �ش���(�⺻ 2)

  if (1) {
    // ���̺� ������ �θ� ���� �ؽ�Ʈ �߰�(�ؽ�Ʈ �� �߰�)
    SRAttributedStringPtr str = SRAttributedString::create(L"\n"); // "\n\n"
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ���� �߰�
    str->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
  }

  table->setNumberOfColumns(rowCnt);
  table->setNumberOfRows(colCnt);

  // ���̺� ���鿡�� �θ� �� & �� ��(���̺�(��)�� ���Ե�) �Ӽ� �߰�
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
  UIViewPtr hitView = SR()->hitTest(UIEvent(MOUSE_MOVE, point.x, point.y)); // ���콺 ��ǥ�κ��� �並 ã��
  if (hitView) {
    point = SR()->convertTo(SR()->localCoordinate(point), hitView);
    // root render ��ǥ�� ��ȯ
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
    return TRUE; // ����� ������ �ʰ� �Ͽ�, ȭ�� ���Ž� �������� ������
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
    //point.y = std::max((short)0, (short)point.y); // ������ ���� �巡�׽� 0�� �Ǽ� ���� ��ũ���� �ȵǰ� �ִ�.
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
        // ���� ���� ��쿡�� ���� ���� �ϳ��� ����
        textPos[0]._offset = std::max(0, textPos[0]._offset - 1);
      }
    } else if (wParam == 0x0D) { // Carriage return
      str = L"\n";
    }
    SR()->replace(UITextRange(textPos[0], textPos[1]), str); // ~40ms
    SR_PROF_TIME_STAMP(L"WM_CHAR:replace()");
    //_textStorage->endEditing(); // ���⼭ ���̾ƿ��õ�
    SR()->invalidateRect(NULL, false);
    SR()->updateWindow();

    UITextPosition newTextPos = textPos[0];
    // �߰��� ���� �� 1��ŭ ���������� ĳ���� �̵�
    if (!(wParam == 0x08)) { // Backspace
      newTextPos._offset += 1;
    }
    SR()->setTextPositions(&newTextPos, &newTextPos);
    SR()->updateCaret();
    SR()->showCaret();
    SR_PROF_TIME_END(L"WM_CHAR"); SR_LOGD(L"");
  }
  break;
  case WM_KEYDOWN: // ���� �׻� 0ms
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
