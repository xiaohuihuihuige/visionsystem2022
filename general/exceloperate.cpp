#include "exceloperate.h"

ExcelOperate::ExcelOperate(QObject *parent) : QObject(parent)
{
    pApplication = new QAxObject();
    pApplication->setControl("Excel.Application");//连接Excel控件
    pApplication->dynamicCall("SetVisible(bool)", false);//false不显示窗体
    pApplication->setProperty("DisplayAlerts", false);//不显示任何警告信息。
}
void ExcelOperate::newExcel(const QString &fileName)
{
    pWorkBooks = pApplication->querySubObject("Workbooks");
    QFile file(fileName);
    if (file.exists())
    {
        pWorkBook = pWorkBooks->querySubObject("Open(const QString &)", fileName);
    }
    else
    {
        pWorkBooks->dynamicCall("Add");
        pWorkBook = pApplication->querySubObject("ActiveWorkBook");
        saveAs(fileName);
    }
    pSheets = pWorkBook->querySubObject("Sheets");

    int _sheetsCount =pSheets->property("Count").toInt();       ///< 当前excel文件里包含的sheet个数
    if(_sheetsCount>0)
    {
        pSheet = pSheets->querySubObject("Item(int)", 1);
        //pSheet =_curWorkbook->querySubObject("Worksheets(int)", 1);///< 当前sheet（实际就是第一个sheet）
    }

}
void ExcelOperate::appendSheet(const QString &sheetName)
{
    int _sheetsCount =pSheets->property("Count").toInt();       ///< 当前excel文件里包含的sheet个数
    QAxObject *pLastSheet = pSheets->querySubObject("Item(int)", _sheetsCount);
    pSheets->querySubObject("Add(QVariant)", pLastSheet->asVariant());
    _sheetsCount =pSheets->property("Count").toInt();
    pSheet = pSheets->querySubObject("Item(int)", _sheetsCount);
    pLastSheet->dynamicCall("Move(QVariant)", pSheet->asVariant());
    pSheet->setProperty("Name", sheetName);
}

//另存为文件
void ExcelOperate::saveAs( QString fileName)
{
    fileName.replace("/", "\\");
    pWorkBook->dynamicCall("SaveAs(QString)",fileName);
}

//保存文件
void ExcelOperate::save()
{
    pWorkBook->dynamicCall("Save()");
}

//关闭文件
void ExcelOperate::closeFile()
{
     pWorkBook->dynamicCall("Close (Boolean)", false); //关闭文件
}

void ExcelOperate::setCellValue(int row, int column, const QString &value)
{
    QAxObject *pRange = pSheet->querySubObject("Cells(int,int)", row, column);
    pRange->dynamicCall("Value", value);
}
void  ExcelOperate::freeExcel()
{
    if (pApplication != nullptr)
    {
        pApplication->dynamicCall("Quit()");
        delete pApplication;
        pApplication = nullptr;
    }
}
QList<QList<QVariant>> ExcelOperate::readAll()
{
    QList<QList<QVariant> > res;
    QVariant var;
    if (this->pSheet != nullptr && ! this->pSheet->isNull())
    {
        QAxObject *usedRange = this->pSheet->querySubObject("UsedRange");
        if(nullptr == usedRange || usedRange->isNull())
        {
            return res;
        }
        var = usedRange->dynamicCall("Value");
        delete usedRange;
    }
    castVariant2ListListVariant(var,res);
    return res;
}
///
/// \brief 把QVariant转为QList<QList<QVariant> >
/// \param var
/// \param res
///
void ExcelOperate::castVariant2ListListVariant(const QVariant &var, QList<QList<QVariant> > &res)
{
    QVariantList varRows = var.toList();
    if(varRows.isEmpty())
    {
        return;
    }
    const int rowCount = varRows.size();
    QVariantList rowData;
    for(int i=0;i<rowCount;++i)
    {
        rowData = varRows[i].toList();
        res.push_back(rowData);
    }
}

///
/// \brief 写入一个表格内容
/// \param cells
/// \return 成功写入返回true
/// \see readAllSheet
///
bool ExcelOperate::writeCurrentSheet(const QList<QList<QVariant> > &cells)
{
    if(cells.size() <= 0)
        return false;
    if(nullptr == this->pSheet || this->pSheet->isNull())
        return false;
    int row = cells.size();
    int col = cells.at(0).size();
    QString rangStr;
    convertToColName(col,rangStr);
    rangStr += QString::number(row);
    rangStr = "A1:" + rangStr;
    QAxObject *range = this->pSheet->querySubObject("Range(const QString&)",rangStr);
    if(nullptr == range || range->isNull())
    {
        return false;
    }
    bool succ = false;
    QVariant var;
    castListListVariant2Variant(cells,var);
    succ = range->setProperty("Value", var);
    delete range;
    return succ;
}
///
/// \brief 把QList<QList<QVariant> >转为QVariant
/// \param var
/// \param res
///
void ExcelOperate::castListListVariant2Variant( const  QList<QList<QVariant> > &var,QVariant &res)
{
    QVariant temp = QVariant(QVariantList());
    QVariantList record;

    int listSize = var.size();
    for (int i = 0; i < listSize;++i)
    {
        temp = var.at(i);
        record << temp;
    }
    temp = record;
    res = temp;
}
///
/// \brief 把列数转换为excel的字母列号
/// \param data 大于0的数
/// \return 字母列号，如1->A 26->Z 27 AA
///
void ExcelOperate::convertToColName(int data, QString &res)
{
    Q_ASSERT(data>0 && data<65535);
    int tempData = data / 26;
    if(tempData > 0)
    {
        int mode = data % 26;
        convertToColName(mode,res);
        convertToColName(tempData,res);
    }
    else
    {
        res=(to26AlphabetString(data)+res);
    }
}
///
/// \brief 数字转换为26字母
///
/// 1->A 26->Z
/// \param data
/// \return
///
QString ExcelOperate::to26AlphabetString(int data)
{
    QChar ch = data + 0x40;//A对应0x41
    return QString(ch);
}
/*//添加散点图
void ExcelOperate::addScatterChart()
{
    QAxObject *shapes = _curWorkSheet->querySubObject("Shapes");
    int usedColCount=2;
    QAxObject *emptyCell = _curWorkSheet->querySubObject("Cells(int,int)", 1, usedColCount+3 );
    // 先选中一个空白的地方，不然后面的SeriesCollection会先将所有行都做个散点图。。。
    emptyCell->dynamicCall("Select(void)");
    shapes->dynamicCall("AddChart(int)",-4169,100,100,500,800); // height width
    // 再将视野拖回到左上角区域
    QAxObject *range = _curWorkSheet->querySubObject("Range(QVariant)","$A$1:$B$1");
    range->dynamicCall("Select(void)");
    int nbChart = shapes->property("Count").toInt();
    QAxObject *shape = shapes->querySubObject("Range(int)",nbChart);

    //shape->dynamicCall("ScaleWidth(QVariant,QVariant)",2,0);
    shape->dynamicCall("Select(void)");

    QAxObject *chart = _curWorkbook->querySubObject("ActiveChart");
    QAxObject *seriesCollection = chart->querySubObject("SeriesCollection()");
    seriesCollection->dynamicCall("NewSeries (void)");
    int nb = seriesCollection->property("Count").toInt();
    series = chart->querySubObject("SeriesCollection(int)",nb);
    series->setProperty("Name","SeriesNo1");
    // 给该系列添加源数据
    // 这里数据来自源数据，非Excel文件，
    QList<QVariant> listXValues, listValues;
    for (int i=0; i<pointlist.size(); i++)
    {
        listXValues.push_back(pointlist[i].x());
        listValues.push_back(pointlist[i].y());
    }
    series->setProperty("XValues",listXValues);
    series->setProperty("Values",listValues);
    series->dynamicCall("Select(void)");

    delete chart;
    delete  seriesCollection;
    delete shape;
    delete range;
    delete emptyCell;
    delete shapes;
}
//添加趋势线
void ExcelOperate::addTrendLine()
{
    QAxObject *trendLines = series->querySubObject("Trendlines()");
    if (trendLines)
    {
        trendLines->dynamicCall("Add()");
        int nbTrendLines = trendLines->property("Count").toInt();
        QAxObject *trendLine = series->querySubObject("Trendlines(int)",nbTrendLines);
        // 设定图标格式为 “散点图”，数字-4133来自 枚举变量 XlTrendlineType  : xlLinear
        trendLine->setProperty("Type","xlLinear");       // XlTrendlineType  : xlLinear
        trendLine->setProperty("Name","TrendLineName");
        trendLine->setProperty("DisplayEquation",true);

        QAxObject *dataLabel = trendLine->querySubObject("DataLabel");
        QString strFormula = dataLabel->property("Formula").toString();
        delete dataLabel;
        delete trendLine;
        //qDebug() << strFormula;         // "y = -9.113ln(x) + 78.016"
        //double _cofA,_cofB;
        //getFormulaCoeffcient(strFormula,&_cofA,&_cofB);

    }
    delete trendLines;
}
void ExcelOperation::getFormulaCoeffcient(QString strFormula,double *_cofA,double *_cofB)
{

}*/
