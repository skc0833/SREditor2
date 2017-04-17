
static SRAttributedStringPtr tableCellAttributedString(const SRTextBlockList& parentBlocks
  , const SRString& string, SRTextTablePtr table
  , SRColor backgroundColor, SRColor borderColor, SRInt startingRow, SRInt rowSpan, SRInt startingColumn, SRInt columnSpan) {
  SR_ASSERT((startingRow + rowSpan) <= table->_numberOfRows);
  SR_ASSERT((startingColumn + columnSpan) <= table->_numberOfColumns);
  // ���̺�(��)�� �� ���� ���Խ�Ŵ
  SRTextTableCellBlockPtr cellBlock = SRTextTableCellBlock::create(table, startingRow, rowSpan, startingColumn, columnSpan);

  cellBlock->setBackgroundColor(backgroundColor);
  cellBlock->setBorderColor(borderColor);
  cellBlock->setWidth(1.0, SRTextTableCellBlock::Border); // html table �� ��� ���̺��� ��輱 �β��� 1 �̻��̸�, �� ��輱�� ������ 1 �β��� ����
  cellBlock->setWidth(1.0, SRTextTableCellBlock::Padding); // �� ��輱���κ����� �е�

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
  paragraphStyle->addTextBlock(cellBlock); // �� �� �߰�(������ ���̺�(��)�� ������)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static SRBool _debugShowMarkForDemo = false;

// �� ���� ���̺� ���� ������(������ ���̺� ���� ���������� ����)
// @$a$b$c$d$#$
static SRAttributedStringPtr tableAttributedString(const SRTextBlockList& parentBlocks, SRFloat border = 1.0f
  , SRFloat padding = 2.0f, int rowColSpanTest = 0) {
  SRAttributedStringPtr tableString = SRAttributedString::create(L""); // "\n\n"
  SRTextTablePtr table = SRTextTable::create();
  //table->setWidth(0.0, SRTextTableCellBlock::Margin); // ���ʿ�??? html table �Ӽ����� ���µ���
  table->setWidth(border, SRTextTableCellBlock::Border);
  table->setWidth(padding, SRTextTableCellBlock::Padding); // html table cellspacing �� �ش���(�⺻ 2)

  if (1) {
    // ���̺� ������ �θ� ���� �ؽ�Ʈ �߰�(�ؽ�Ʈ �� �߰�)
    SRAttributedStringPtr str = SRAttributedString::create(L"\n"); // "\n\n"
    if (_debugShowMarkForDemo) {
      str = SRAttributedString::create(L"@\n"); // "\n\n"
    }
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ���� �߰�
    str->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
    //tableString->addAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, tableString->getLength()));
  }

#if 0
  // 4 x 4, center rowspan=2, colspan=2
  table->setNumberOfColumns(4);
  table->setNumberOfRows(4);

  tableString->append(tableCellAttributedString(parentBlocks, L"00\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 0, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"01\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 1, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"02\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 2, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"03\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 3, 1));

  tableString->append(tableCellAttributedString(parentBlocks, L"10\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 0, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"11\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 2, 1, 2));
  tableString->append(tableCellAttributedString(parentBlocks, L"13\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 3, 1));

  tableString->append(tableCellAttributedString(parentBlocks, L"20\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 2, 1, 0, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"23\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 2, 1, 3, 1));

  tableString->append(tableCellAttributedString(parentBlocks, L"30\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 3, 1, 0, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"31\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 3, 1, 1, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"32\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 3, 1, 2, 1));
  tableString->append(tableCellAttributedString(parentBlocks, L"33\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 3, 1, 3, 1));
#else
  table->setNumberOfColumns(2);
  table->setNumberOfRows(2);

  // ���̺� ���鿡�� �θ� �� & �� ��(���̺�(��)�� ���Ե�) �Ӽ� �߰�
  if (rowColSpanTest == 1) { // rowspan
    tableString->append(tableCellAttributedString(parentBlocks, L"a\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 2, 0, 1));
    tableString->append(tableCellAttributedString(parentBlocks, L"b\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 1, 1, 1));
    tableString->append(tableCellAttributedString(parentBlocks, L"c\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 1, 1));
  } else if (rowColSpanTest == 2) { // colspan
    tableString->append(tableCellAttributedString(parentBlocks, L"a\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 0, 2));
    //tableString->append(tableCellAttributedString(parentBlocks, L"d\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1, 1, 1));
    tableString->append(tableCellAttributedString(parentBlocks, L"c\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 0, 1));
    tableString->append(tableCellAttributedString(parentBlocks, L"d\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1, 1, 1));
  } else {
    if (true || _debugShowMarkForDemo) {
      tableString->append(tableCellAttributedString(parentBlocks, L"a\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 0, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"b\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 1, 1, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"c\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 0, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"d\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1, 1, 1));
    } else {
      tableString->append(tableCellAttributedString(parentBlocks, L"January 9\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 0, 1, 0, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"February 6\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 0, 1, 1, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"March 15\n", table, SRColor(0, 1, 0), SRColor(1, 0, 0), 1, 1, 0, 1));
      tableString->append(tableCellAttributedString(parentBlocks, L"April 21\n", table, SRColor(0, 1, 1), SRColor(1, 0, 0), 1, 1, 1, 1));
    }
  }
#endif

  if (1) {
    // ���̺� ���Ŀ� �θ� ���� �ؽ�Ʈ �߰�
    SRAttributedStringPtr str = SRAttributedString::create(L""); // \n
    if (_debugShowMarkForDemo) {
      str = SRAttributedString::create(L"#\n");
    }

    // default TextBlock �Ӽ� �߰�
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
    str->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    tableString->append(str);
  }

  return tableString;
}

static SRAttributedStringPtr textListItemAttributedString(const SRTextBlockList& parentBlocks, const SRString& string, SRTextListPtr textList) {
  SRTextListBlockPtr block = SRTextListBlock::create(textList); // ����Ʈ(��)�� ����Ʈ ���� ���Խ�Ŵ

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
  paragraphStyle->addTextBlock(block); // �� �� �߰�(�� ���ȿ� ���̺�(��)�� ���Ե�)

  SRAttributedStringPtr cellString = SRAttributedString::create(string);
  cellString->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, cellString->getLength()));

  return cellString;
}

static SRAttributedStringPtr textListAttributedString(const SRTextBlockList& parentBlocks, SRTextList::MarkerFormat markerFormat) {
  SRAttributedStringPtr attrString = SRAttributedString::create(L""); // "\n\n"
  if (_debugShowMarkForDemo) {
    attrString = SRAttributedString::create(L"@\n"); // "\n\n"
  }
  SRTextListPtr textList = SRTextList::create(markerFormat);

  // default TextBlock �Ӽ� �߰�
  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(parentBlocks); // �θ� ���� �߰�

  // ���̺� ������ �θ� ���� �ؽ�Ʈ �߰�(�ؽ�Ʈ �� �߰�)
  attrString->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrString->getLength()));

  // ���̺� ���鿡�� �θ� �� & �� ��(���̺�(��)�� ���Ե�) �Ӽ� �߰�
  if (_debugShowMarkForDemo) {
    attrString->append(textListItemAttributedString(parentBlocks, L"A\n", textList)); // aa\nA\nB -> OK!
    attrString->append(textListItemAttributedString(parentBlocks, L"B\n", textList));
    attrString->append(textListItemAttributedString(parentBlocks, L"C\n", textList));
  } else {
    attrString->append(textListItemAttributedString(parentBlocks, L"January 9\n", textList)); // aa\nA\nB -> OK!
    attrString->append(textListItemAttributedString(parentBlocks, L"February 6\n", textList));
    attrString->append(textListItemAttributedString(parentBlocks, L"March 15\n", textList));
  }

  if (_debugShowMarkForDemo) {
    // ���̺� ���Ŀ� �θ� ���� �ؽ�Ʈ �߰�
    SRAttributedStringPtr str = SRAttributedString::create(L"#\n");

    // default TextBlock �Ӽ� �߰�
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(parentBlocks); // �θ� ������ �߰�
    str->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, str->getLength()));
    attrString->append(str);
  }

  // ������ ���ڴ� ���๮�ڷ� ������ ó����
  //attrString->append(SRAttributedString::create(L"\n"));

  return attrString;
}

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
    // ref ��ü�� �Ʒ� CTRunDelegate::create() ȣ��� �����ߴ� SRObjectDictionary ��ü��
    SRObjectDictionaryPtr imgUserObj = std::dynamic_pointer_cast<SRObjectDictionary>(ref);
    return std::dynamic_pointer_cast<CTImageRunInfo>(imgUserObj->valueForKey(IMAGE_RUN_KEY));
  }

  static SRFloat imgGetAscentCallback(SRObjectPtr ref) { return ptr(ref)->height(); }
  static SRFloat imgGetDescentCallback(SRObjectPtr ref) { return 0; }
  static SRFloat imgGetWidthCallback(SRObjectPtr ref) { return ptr(ref)->width(); }
  static void imgDeallocCallback(SRObjectPtr ref) { }
  static void imgOnDrawCallback(SRObjectPtr ref, CGContextPtr ctx, SRPoint pt) {
    // CTRun::draw() ���� �̹��� ����׽ÿ� ȣ���
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

static void _insertImage(SRAttributedStringPtr attrStr, SRRange range, const SRString& imgPath, SRSize imgSize) {
  CTRunDelegateCallbacks callbacks;
  callbacks.getAscent = CTImageRunInfo::imgGetAscentCallback;
  callbacks.getDescent = CTImageRunInfo::imgGetDescentCallback;
  callbacks.getWidth = CTImageRunInfo::imgGetWidthCallback;
  callbacks.dealloc = CTImageRunInfo::imgDeallocCallback;
  callbacks.onDraw = CTImageRunInfo::imgOnDrawCallback;

  // �ݹ� ȣ��� ���޹��� user object �� CTImageRunInfo �� ��� SRObjectDictionary Ÿ������ ����
  SRObjectDictionaryPtr imgUserObj = SRObjectDictionary::create();
  CTImageRunInfoPtr imgInfo = CTImageRunInfo::create(imgPath, imgSize.width, imgSize.height);
  imgUserObj->addValue(CTImageRunInfo::IMAGE_RUN_KEY, imgInfo);

  CTRunDelegatePtr delegate = CTRunDelegate::create(callbacks, std::dynamic_pointer_cast<SRObject>(imgUserObj));

  const SRObjectDictionaryPtr attribs = attrStr->attributes(range.location, nullptr);
  SRObjectDictionaryPtr newAttribs = SRObjectDictionary::create(*attribs); // �Ӽ��� �߰��ϹǷ� ��������� �Ѵ�.
  //newAttribs->addValue(kCTRunDelegateAttributeName, delegate);
  newAttribs->setValue(delegate, kCTRunDelegateAttributeName);

  // range ��ġ�� �Ӽ��� CTRunDelegate ��ü�� �� �߰��ϴ� ���
  attrStr->replaceCharacters(range, SRAttributedString::create(L"x", newAttribs)); // OK!!!
}

static const SRTextBlockList& getTextBlockList(const SRAttributedStringPtr parentAttrStr, SRUInt location) {
  CTParagraphStylePtr parentStyle = std::dynamic_pointer_cast<CTParagraphStyle>(
    parentAttrStr->attribute(kCTParagraphStyleAttributeName, location));
  return parentStyle->getTextBlockList();
}

static SRAttributedStringPtr _makeTestTableString(SRTextBlockList& rootBlocks) {
  _debugShowMarkForDemo = true;
  SRAttributedStringPtr attrStr = tableAttributedString(rootBlocks); // @$ a$b$c$d$ #$

#if 1 // insert inner tables
  // �θ� ���ڿ� replaceRange.location ��ġ�� �ڽ� ���ڿ� �߰�(�θ� ���� ��ӹ���)
  if (1) {
    // ù �÷��� �ڽ� ���̺� �߰�
    SRRange replaceRange(2, 2); // @$ |a$| b$c$d$ #$
    // rootBlocks(@$ ~ #$) + ���̺�0(a$b$c$d$) ��
    const SRTextBlockList& parentTextBlocks = getTextBlockList(attrStr, replaceRange.location);
    // @$ |@$ a$b$c$d$ #$| b$c$d$ #$
    attrStr->replaceCharacters(replaceRange, tableAttributedString(parentTextBlocks, 1.0f, 2.0f, 2));
  }
  if (1) {
    // ù �÷��� �ڽ� ����Ʈ �߰�
    SRRange replaceRange(4, 2); // @$ @$ |a$| b$c$d$ #$ b$c$d$ #$
    // rootBlocks + ���̺�0 + ���̺�1 ��
    const SRTextBlockList& parentTextBlocks = getTextBlockList(attrStr, replaceRange.location);
    // @$ @$ |@$ A$B$C$ #$| b$c$d$ #$ b$c$d$ #$
    attrStr->replaceCharacters(replaceRange, textListAttributedString(parentTextBlocks, SRTextList::DECIMAL));
  }
  if (1) {
    // ù ����Ʈ �����ۿ� �ڽ� ���̺� �߰�
    SRRange replaceRange(6, 2); // @$ @$ @$ |A$| B$C$ #$ b$c$d$ #$ b$c$d$#$
    // rootBlocks + ���̺�0 + ���̺�1 + ����Ʈ0 ��
    const SRTextBlockList& parentTextBlocks = getTextBlockList(attrStr, replaceRange.location);
    // @$ @$ @$ |@$ a$b$c$d$ #$| B$C$ #$ b$c$d $# $b$c$d$ #$
    attrStr->replaceCharacters(replaceRange, tableAttributedString(parentTextBlocks, 1.0f, 0.0f, 1));
  }

  if (1) {
    // �׽�Ʈ �̹��� �߰�
    // @$ @$ @$ @$ a$b$c$d$ #$ B$ |C|$ #$ b$c$d $# $b$c$d$ #$
    _insertImage(attrStr, SRRange(20, 1), L".\\test.bmp", SRSize(50, 50));
    _insertImage(attrStr, SRRange(20, 0), L".\\test1.bmp", SRSize(30, 30));
    for (int i = 2; i < 2; ++i) {
      if (i % 2 == 0) {
        _insertImage(attrStr, SRRange(20 + i * 2, 0), L".\\test.bmp", SRSize(70, 70));
      }
      else {
        _insertImage(attrStr, SRRange(20 + i * 2, 0), L".\\test1.bmp", SRSize(30, 30));
      }
    }
  }
#endif

  UIFontPtr font = UIFont::fontWithName(L"���� ���", 14.0);
  attrStr->setAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  if (1) {
    // �κ� ��Ʈ �Ӽ� ����
    SRRange currentRange(2, 1);
    UIFontPtr font = UIFont::fontWithName(L"Arial", 24.0); // 12.0
    attrStr->setAttribute(kCTFontAttributeName, font, currentRange);
    attrStr->setAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(2, 1));
  }

  _debugShowMarkForDemo = false;
  return attrStr;
}

//#define SR_LOCAL_TEST_STR

static SRAttributedStringPtr _makeTestTextString(SRTextBlockList& rootBlocks) {
  SRAttributedStringPtr attrStr = SRAttributedString::create(L"left, right mouse click!!!\n" \
    L"�ѱ�! supports table, list, image, paging, column, font, paragraph style\n" \
    L"left, right mouse click!!!\n" \
    L"�ѱ�! supports table, list, image, paging, column, font, paragraph style\n"
  );

  attrStr = SRAttributedString::create(L"Festivals, sights around Korea\n"); // 30
#ifdef SR_LOCAL_TEST_STR
  if (1) {
    attrStr = SRAttributedString::create(L"abc\nABC\n");
    //attrStr = SRAttributedString::create(L"a\nb\nc\nd\ne\nf\ng\nh\ni\nj\nk\n");
    //attrStr = SRAttributedString::create(L"abcdefghijkABCDEFGHIJKabcdefghijkABCDEFGHIJK\n");
    //attrStr = SRAttributedString::create(L"abcdefghi\njkABCDEFGHIJKabcdefghijk\n");
    //attrStr = SRAttributedString::create(L"abcde\nABCDE\n");

    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(rootBlocks);
    attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));

    //UIFontPtr font = UIFont::fontWithName(L"Arial", 28.0); // ������ ��Ʈ(28 -> ������ 20, 14 -> 20)
    UIFontPtr font = UIFont::fontWithName(L"Arial", 24.0);
    //UIFontPtr font = UIFont::fontWithName(L"����ü", 24.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
    attrStr->setAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));
    return attrStr;
  }
#endif

  CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
  paragraphStyle->addTextBlockList(rootBlocks);
  // alignment
  CTTextAlignment textAlign = kCTCenterTextAlignment;
  paragraphStyle->setValueForSpecifier(kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &textAlign);
  attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, SRRange(0, attrStr->getLength()));

  //UIFontPtr font = UIFont::fontWithName(L"Arial", 28.0); // ������ ��Ʈ(28 -> ������ 20, 14 -> 20)
  UIFontPtr font = UIFont::fontWithName(L"Arial", 24.0);
  //UIFontPtr font = UIFont::fontWithName(L"����ü", 24.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
  attrStr->setAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));

  if (1) {
    SRAttributedStringPtr astr = SRAttributedString::create(L"Published : 2017-04-07 17:00\n\n");
    SRRange range(attrStr->getLength(), astr->getLength());
    attrStr->append(astr);
    //_demoWrappingImagePos = attrStr->getLength();
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(rootBlocks);
    // alignment
    CTTextAlignment textAlign = kCTRightTextAlignment;
    paragraphStyle->setValueForSpecifier(kCTParagraphStyleSpecifierAlignment, sizeof(CTTextAlignment), &textAlign);
    attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);

    //UIFontPtr font = UIFont::fontWithName(L"Courier New", 28.0);
    UIFontPtr font = UIFont::fontWithName(L"Courier New Italic", 10.0);
    attrStr->setAttribute(kCTFontAttributeName, font, range);
  }
  _demoWrappingImagePos = attrStr->getLength();

  if (1) {
    SRAttributedStringPtr astr = SRAttributedString::create(L"The Jeju Cherry Blossom Festival will be held in the area of Jeonnong-ro and the entrance of Jeju National University.\n"
      L"The peak of the King Cherry Tree blossoms lasts for only two to three days, but the beautiful blossoms can be seen for weeks, until April 9.\n"
      L"There is no admission fee.\n"
      L"For more information, call the travel hotline at 1330 or visit www.visitjeju.net.Both provide information in Korean, English, Japanese and Chinese.\n"
      L"Every year, the festival showcases up to 3 million tulips and present various events across the town. It is being held from April 7 to April 16 this year.\n"
    );
    //astr = SRAttributedString::create(L"The Jeju Cherry Blossom Festival\n"); // @@
    SRRange range(attrStr->getLength(), astr->getLength());
    attrStr->append(astr);
    CTParagraphStylePtr paragraphStyle = CTParagraphStyle::defaultParagraphStyle();
    paragraphStyle->addTextBlockList(rootBlocks);
    attrStr->setAttribute(kCTParagraphStyleAttributeName, paragraphStyle, range);

    //UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
    UIFontPtr font = UIFont::fontWithName(L"Courier New", 12.0); // 28.0 -> w:19 h:37, 24 -> w:16 h:32, 14.0 -> w:10 h:19
    attrStr->setAttribute(kCTFontAttributeName, font, range);

    if (1) {
      // ����Ʈ ������ ����
      SRAttributedStringPtr astr = textListAttributedString(rootBlocks, SRTextList::CIRCLE);
      astr->setAttribute(kCTFontAttributeName, font, SRRange(0, astr->getLength()));
      SRRange replaceRange(180, 0);
      attrStr->replaceCharacters(replaceRange, astr);
    }
  }

  if (1) {
    // ���̺� ����
    SRAttributedStringPtr astr = tableAttributedString(rootBlocks); // @$ a$b$c$d$ #$

    if (1) {
      // ù �÷��� �ڽ� ���̺� �߰�
      SRRange replaceRange(1, 2); // $ |a$| b$c$d$ #$
      // rootBlocks(@$ ~ #$) + ���̺�0(a$b$c$d$) ��
      const SRTextBlockList& parentTextBlocks = getTextBlockList(astr, replaceRange.location);
      // @$ |@$ a$b$c$d$ #$| b$c$d$ #$
      astr->replaceCharacters(replaceRange, tableAttributedString(parentTextBlocks, 1.0, 0.0, 1));
      // TODO �ӽ÷� ���̺� ���� ���๮�ڸ� ����
      astr->replaceCharacters(SRRange(replaceRange.location, 1), SRString::create());
    }
    if (1) {
      // ù �÷��� �ڽ� ����Ʈ �߰�
      //SRRange replaceRange(1, 2); // $ |a$| b$c$d$ #$ b$c$d$ #$
      SRRange replaceRange(7, 2); // $ |a$| b$c$d$ #$ b$c$d$ #$
      // rootBlocks + ���̺�0 + ���̺�1 ��
      const SRTextBlockList& parentTextBlocks = getTextBlockList(astr, replaceRange.location);
      // @$ @$ |@$ A$B$C$ #$| b$c$d$ #$ b$c$d$ #$
      astr->replaceCharacters(replaceRange, textListAttributedString(parentTextBlocks, SRTextList::DECIMAL));
    }
    //UIFontPtr font = UIFont::fontWithName(L"���� ���", 14.0);
    UIFontPtr font = UIFont::fontWithName(L"Courier New", 12.0);
    astr->setAttribute(kCTFontAttributeName, font, SRRange(0, astr->getLength()));
    //attrStr->append(astr);
    attrStr->replaceCharacters(SRRange(459, 0), astr);
  }

  if (1) {
    // �κ� ��Ʈ �Ӽ� ����
    SRRange currentRange(61, 1); // �� "The Jeju Cherry Blossom ..." ���ڿ��� T ������ġ��
    UIFontPtr font = UIFont::fontWithName(L"Courier New Bold", 24.0); // 12.0
    attrStr->setAttribute(kCTFontAttributeName, font, currentRange);
    attrStr->setAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), currentRange);
    //attrStr->setAttribute(kCTForegroundColorAttributeName, SRColor::create(1, 0, 0), SRRange(2, 2));
  }

  // only list
  //SRAttributedStringPtr attrStr = textListAttributedString(rootBlocks, SRTextList::DECIMAL);
  //UIFontPtr font = UIFont::fontWithName(L"���� ���", 12.0);
  //attrStr->setAttribute(kCTFontAttributeName, font, SRRange(0, attrStr->getLength()));
  //return attrStr;

  return attrStr;
}
