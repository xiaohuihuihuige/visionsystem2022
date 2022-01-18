#ifndef GENERALFUNC_H
#define GENERALFUNC_H

#include<QVector>
#include<string>
#include<vector>
#include<QTableWidget>
#include<QHeaderView>
#include<QLabel>
#include<QMap>
#include<QStatusBar>
#include<QComboBox>
#include"QZXing.h"
#include<QFileInfo>
#include <qfile.h>
#include <qtextstream.h>
#include <fstream>
#include <sstream>
#include <io.h>
#include<QFileDialog>
#include<QString>
#include<QObject>
#include<QTreeWidget>
#include<QDomDocument>
#include<QDir>
#include<QTimer>
#include<QObject>
#include<QDateTime>
#include<QMessageBox>
#include<QLayout>
#include<QPushButton>
#include<QDesktopServices>
#include<QLineEdit>

namespace GeneralFunc
{
//判断文件夹是否存在
static bool isDirExist(QString fullPath,bool iscreate)
{
    QDir dir(fullPath);
    if(dir.exists())//判断文件夹是否存在
    {
        return true;
    }
    else
    {
        if(iscreate)
        {
            bool ok=dir.mkpath(fullPath);//没有则创建一个
            return ok;
        }
        return false;
    }
}
//判断文件是否存在
static bool isFileExist(QString filePath,bool isCreate)
{
    QFile file(filePath);
    if(file.exists())//判断文件是否存在
    {
        return true;
    }
    else
    {
        if(isCreate)
        {
            bool ok=file.open(QIODevice::WriteOnly);//没有则创建一个
            file.close();
            return ok;
        }
        else
        {
            return false;
        }

    }
}
//写入csv文件
static void writeCsv(QString csvPath, QVector<QVector<QString>> writeData, char rowSplit)
{
    QFile outFile(csvPath);
    QStringList lines;
    for (int i=0;i<writeData.size();i++)
    {
        QString lineData=QString("");
        for (int j=0;j<writeData[i].size();j++)
        {
            lineData+=writeData[i][j]+rowSplit;
        }
        lineData.chop(1);
        lineData+=QString("\n");
        lines<<lineData;
    }
    /*如果以ReadWrite方式打开，不会把所有数据清除，如果文件原来的长度为100字节，写操作写入了30字节，那么还剩余70字节，并且这70字节和文件打开之前保持一直*/
    if (outFile.open(QIODevice::WriteOnly))
    {
        for (int i = 0; i < lines.size(); i++)
        {
            outFile.write(lines[i].toStdString().c_str());/*写入每一行数据到文件*/
        }
        outFile.close();
    }
}

//读取csv文件
static int readCSV(std::string FilePath, char RowSeparator, char ColSeparator, QVector<QString> *header,QVector<QVector<QString>> *readData)
{
    std::ifstream inFile(FilePath, std::ios::in);
    std::string lineStr;
    QVector<QVector<QString>> strArray;
    //按照自定字符分割
    while (getline(inFile, lineStr, RowSeparator))
    {
        // 存成二维表结构
        std::stringstream ss(lineStr);
        std::string str;
        QVector<QString>  lineArray;
        // 按照指定字符分隔
        while (getline(ss, str, ColSeparator))
        {
            lineArray.push_back(QString::fromStdString(str));
        }
        strArray.push_back(lineArray);
    }
    if(strArray.size()>0)
    {
        *header=strArray[0];
        strArray.erase(strArray.begin());
        (*readData) = strArray;
    }
    return 0;
}
//添加记录到文件
static void appendCsv(QString csvPath, QString appendData)
{
    GeneralFunc::isFileExist(csvPath,1);
    QFile outFile(csvPath);
    if (outFile.open(QIODevice::Append))
    {
        outFile.write(QString( appendData+"\n").toLocal8Bit());
        outFile.close();
    }
}
//按行读取文本文件
static QStringList readFileLineToVector(QString filePath)
{
    if(!isFileExist(filePath,0))
        return QStringList();
    std::ifstream inFile(filePath.toStdString(), std::ios::in);
    std::string lineStr;
    QStringList strArray;
    //按照自定字符分割
    while (getline(inFile, lineStr, '\n'))
    {
        // 存成二维表结构
        strArray<<QString::fromLocal8Bit(lineStr.data());
    }
    return strArray;
}


//设置表格表头
static void setTableWidgetHeader(QTableWidget * tbl,QStringList header)
{
    int num=tbl->columnCount();
    for (int i=0;i<num;i++)
    {
        tbl->removeColumn(0);
    }
    QStringList tabHeader;
    for (int i=0;i<header.size();i++)
    {
        tabHeader<<header[i];
        int col = tbl->columnCount();
        tbl->insertColumn(col);
    }
    tbl->setHorizontalHeaderLabels(tabHeader);
    tbl->horizontalHeader()->setStretchLastSection(true);//自适应列宽，末尾列自动填满控件区域
    //tbl->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);//等分控件宽度
    tbl->setEditTriggers(QAbstractItemView::DoubleClicked);


}
//设置表格数据
static void setTableWidgetData(QTableWidget * tbl,QVector<QVector<QString>> tabData)
{
    int num=tbl->rowCount();
    for (int i=0;i<num;i++)
    {
        tbl->removeRow(0);
    }
    for (int i=0;i<tabData.size();i++)
    {
        int row = tbl->rowCount();
        tbl->insertRow(row);
        for (int j=0;j<tabData[i].size();j++)
        {
            QTableWidgetItem *item=new QTableWidgetItem(tabData[i][j]);
            tbl->setItem(i,j,item);
        }
    }
}

//设置表格数据
static void setTableWidgetData(QTableWidget * tbl,QString tabData, char RowSeparator, char ColSeparator)
{
    QStringList rowdata=tabData.split(RowSeparator);
    QVector<QVector<QString>> tabdata;
    tabdata.clear();
    for (int i=0;i<rowdata.size();i++)
    {
        QStringList coldata=rowdata[i].split(ColSeparator);
        QVector<QString> rowvec;
        rowvec.clear();
        for (int j=0;j<coldata.size();j++)
        {
            rowvec<<coldata[j];
        }
        tabdata<<rowvec;
    }
    setTableWidgetData(tbl,tabdata);
}
//显示Pixmap图像到lbl
static void showImage(QLabel *lbl, QPixmap image)
{
    QPixmap image2=image.scaled(lbl->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);//重新调整图像大小以适应窗口
    lbl->setPixmap(image2);
}
//显示image图像到lbl
static void showImage(QLabel *lbl, QImage image)
{
    QPixmap image1=QPixmap::fromImage(image);
    QPixmap image2=image1.scaled(lbl->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);//重新调整图像大小以适应窗口
    lbl->setPixmap(image2);
}
//显示Pixmap图像到Widget
static void showImage(QWidget *widget, QPixmap image)
{
    widget->setAutoFillBackground(true); // 这句要加上, 否则可能显示不出背景图.
            QPalette palette = widget->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(image.scaled(widget->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation))); // 使用平滑的缩放方式
    widget->setPalette(palette); // 至此, 已给widget加上了背景图.
}
//显示image图像到Widget
static void showImage(QWidget *widget, QImage image)
{
    QPixmap image1=QPixmap::fromImage(image);
    widget->setAutoFillBackground(true); // 这句要加上, 否则可能显示不出背景图.
            QPalette palette = widget->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(image1.scaled(widget->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation))); // 使用平滑的缩放方式
    widget->setPalette(palette); // 至此, 已给widget加上了背景图.
}

//获取文件路径
static QString getFilePath(QString path,QWidget *parent)
{
    QString fileName =QFileDialog::getOpenFileName(parent,
                                                   "open file",
                                                   path,
                                                   QString::fromLocal8Bit("Image files(*.bmp *.jpg *.png );;文本文件(*.txt *.csv);;All files (*.*)"));//打开打开文件对话框
    if(fileName.isEmpty())
    {
        return QString("");
    }
    else
    {
        return fileName;
    }
}
static QString getFolderPath(QString path,QWidget *parent)
{
    QString foldername =QFileDialog::getExistingDirectory(parent,
                                                   "select Foler",
                                                   path,
                                                   QFileDialog::ShowDirsOnly);//打开打开文件对话框
    if(foldername.isEmpty())
    {
        return QString("");
    }
    else
    {
        return foldername;
    }
}

//保存图像
static bool saveImage(QWidget *parent,QImage image)
{
    if(!image.isNull())
    {
        QString fileName = QFileDialog::getSaveFileName(parent,
                                                        QObject::tr("Save Image"),
                                                        "",
                                                        QObject::tr("bmpImage(*.bmp);;jpgImage(*.jpg);;pngImage(*.png)"));//打开保存文件对话框
        if (fileName.isNull())
        {
            return false;
        }
        else
        {
            QImage saveImage;
            saveImage=image;//获取加载图片
            //将图片重新命名并保存至刚刚创建的文件夹里
            saveImage.save(fileName);//QImage保存方法
            return  true;
        }
    }
    else
    {
        return  false;
    }
}
//读取图像文件
static QImage getImageFromFile(QWidget *parent,QString Folder)
{
    QString filePath=getFilePath(Folder,parent);
    if(filePath=="")
    {
        return  QImage();
    }
    QImage image;
    image.load(filePath);
    return image;
}


//获取文件夹路径
static QStringList GetAllFolderPath(QString dirPath)
{
    QStringList folders;
    QDir dir(dirPath);
    dir.setFilter(QDir::Dirs);
    foreach(QFileInfo fullDir, dir.entryInfoList())
    {
        if(fullDir.fileName() == "." || fullDir.fileName() == "..") continue;
        folders<<fullDir.absoluteFilePath();
    }
    return folders;
}
//获取文件夹名
static QStringList GetAllFolderName(QString dirPath)
{
    QStringList folders;
    QDir dir(dirPath);
    dir.setFilter(QDir::Dirs);
    foreach(QFileInfo fullDir, dir.entryInfoList())
    {
        if(fullDir.fileName() == "." || fullDir.fileName() == "..") continue;
        folders<<fullDir.absoluteFilePath().mid(dirPath.length()+1);
    }
    return folders;
}
//获取文件名
static QStringList getFileListName(const QString &path,QStringList fileType)//"*.jpg" << "*.png"
{
    QDir dir(path);
    QStringList nameFilters;
    nameFilters = fileType;
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    QStringList returnList=QStringList();
    for (int i=0;i<files.size();i++)
    {
        QStringList strlist=files[i].split(".");
        returnList<<strlist[0];
    }
    return returnList;
}
//获取文件路径
static QStringList getFileListPath(const QString &path,QStringList fileType)//"*.jpg" << "*.png"
{
    QDir dir(path);
    QStringList nameFilters;
    nameFilters = fileType;
    QStringList files = dir.entryList(nameFilters, QDir::Files|QDir::Readable, QDir::Name);
    QStringList returnList=QStringList();
    for (int i=0;i<files.size();i++)
    {
        returnList<<path+"\\"+files[i];
    }
    return returnList;
}

//删除文件或文件夹
static bool deleteFileOrFolder(const QString &strPath)//要删除的文件夹或文件的路径
{
    if (strPath.isEmpty() || !QDir().exists(strPath))//是否传入了空的路径||路径是否存在
        return false;

    QFileInfo FileInfo(strPath);

    if (FileInfo.isFile())//如果是文件
        QFile::remove(strPath);
    else if (FileInfo.isDir())//如果是文件夹
    {
        QDir qDir(strPath);
        qDir.removeRecursively();
    }
    return true;
}
//buffer转Qimage
static bool qimageFromBuffer(QImage *image, int nWidth, int nHeight,unsigned char *pbuffer,QString type)
{
    if(type=="mono")
    {
        *image = QImage(pbuffer,nWidth, nHeight, QImage::Format_Indexed8);

    }
    else if ("rgb")
    {
        *image = QImage(pbuffer,nWidth, nHeight, QImage::Format_RGB888);
    }
    return true;
}
//单个字符转十六进制字符
static QString qCharToQStringHex(QChar single,int length)
{
    return QString("%1").arg(single.toLatin1(),length,16,QLatin1Char('0')).toUpper();
}
//转化int为hex字符，如1转化为31，length1为int转化为str的长度，length2为每个char转化后的长度
static QString intToQStringHex(int single,int length1, int length2)
{
    QString strint=QString("%1").arg(single,length1,10,QLatin1Char('0'));
    QString backstr="";
    for (int i=0;i<strint.length();i++)
    {
        backstr+=qCharToQStringHex(strint.at(i),length2);
    }
    return backstr;
}
//bytesize=4或者2
static QByteArray int2ByteArray1(int value,int bytesize)
{
    static QList<int64_t> andArray = {0x000000FF, 0x0000FF00, 0x00FF0000,0xFF000000};
    QByteArray bytearray;
    bytearray.resize(bytesize);
    for (int i=0;i<bytesize;i++)
    {
        bytearray[i] = (unsigned char )(andArray[i] & value>>8*i);
    }
    return bytearray;
}

static QByteArray int2ByteArray2(int value)
{
    QByteArray arrayi;
    int len_intVar = sizeof(value);
    arrayi.resize(len_intVar);
    memcpy(arrayi.data(), &value, len_intVar);
    return arrayi;
}

static QByteArray intArray2ByteArray(QList<int> vint)
{
    int listsize=vint.size();
    int*  iVars=(int*)malloc(listsize * sizeof(int));
    std::fill_n(iVars, listsize, 0);
    for (int i=0;i<vint.size();i++)
    {
        iVars[i]=vint[i];
    }
    QByteArray arrayf;
    int len_fVars = listsize * sizeof(int); // 4*4 = 16 (一个float占4个字节)
    arrayf.resize(len_fVars);
    memcpy(arrayf.data(), &iVars, len_fVars);
    return arrayf;
}
static int byteArray2Int1(QByteArray bytearray)
{
    static QList<int64_t> andArray = {0x000000FF, 0x0000FF00, 0x00FF0000,0xFF000000};
    int value=0;
    for (int i=0;i<bytearray.size();i++)
    {
        if(i==0)
            value=(bytearray[i]<<(8*i))& andArray[i];
        else
            value|=(bytearray[i]<<(8*i))& andArray[i];
    }
    return value;
}

static int byteArray2Int2(QByteArray bytearray)
{
    // QByteArray 转 int
    // array 数据接上面
    int  outIntVar;
    memcpy(&outIntVar, bytearray.data(), bytearray.size());
    return outIntVar;
}

static QList<int> byteArray2IntArray(QByteArray byte)
{
    //  QByteArray 转 int[]
    int  *outintvar;
    memcpy(&outintvar, byte.data(), byte.size());
    QList<int> vint;
    for (int i=0;i<byte.size()/sizeof (int);i++)
    {
        vint<<outintvar[i];
    }
    return vint;

}
static QByteArray float2ByteArray1(float data)
{
    QByteArray byte_data;

    char* data_char = (char*)&data;
    for(int index = 3; index >= 0; index--)
    {
        byte_data.append(data_char[index]);
    }
    return byte_data;
}

static QByteArray float2ByteArray2(float data)
{
    QByteArray arrayi;
    int len_floatVar = sizeof(data);
    arrayi.resize(len_floatVar);
    memcpy(arrayi.data(), &data, len_floatVar);
    return arrayi;

}

static QByteArray floatArray2ByteArray(QList<float> fVar)
{
    int listsize=fVar.size();
    float*  fVars=(float*)malloc(listsize * sizeof(float));
    std::fill_n(fVars, listsize, 0.0);
    for (int i=0;i<fVar.size();i++)
    {
        fVars[i]=fVar[i];
    }

    QByteArray arrayf;
    int len_fVars = listsize * sizeof (float); // 4*4 = 16 (一个float占4个字节)
    arrayf.resize(len_fVars);
    memcpy(arrayf.data(), &fVars, len_fVars);
    return arrayf;
}

static float byteArray2Float1(QByteArray byte)
{
    float result = 0;
    int size = byte.size();
    char* data_char = byte.data();
    char* p = (char*)&result;
    for(int index = 0; index < size; index++)
    {
        *(p + index) = *(data_char + size - 1 - index);
    }
    return result;
}

static float byteArray2Float2(QByteArray byte)
{
    // array 数据接上面
    float  outfloatVar;
    memcpy(&outfloatVar, byte.data(), byte.size());
    return outfloatVar;
}

static QList<float> byteArray2FloatArray(QByteArray byte)
{
    //  QByteArray 转 float[]
    float  *outFvar;
    memcpy(&outFvar, byte.data(), byte.size());
    QList<float> vfloat;
    for (int i=0;i<byte.size()/sizeof (float);i++)
    {
        vfloat<<outFvar[i];
    }
    delete outFvar;
    return vfloat;

}
/*QbyteArray和int、float互转
{
// int 转 QByteArray
short  intVar = 199;

QByteArray arrayi;
int len_intVar = sizeof(intVar);
arrayi.resize(len_intVar);
memcpy(arrayi.data(), &intVar, len_intVar);

// QByteArray 转 int
// array 数据接上面
int  outIntVar;
memcpy(&outIntVar, arrayi.data(), len_intVar);
}


{
// int[] 转 QByteArray
int  intVara[4] = {15,2,11,0};//初始化变量赋值
QByteArray arrayia;
int len_intVara = sizeof(intVara);
arrayia.resize(len_intVara);
//转换 int[] -> QByteArray
memcpy(arrayia.data(), &intVara, len_intVara);

// QByteArray 转 int[]
// array 数据接上面
int  outIntVari[4];
memcpy(&outIntVari, arrayia.data(), len_intVara);
}


{
// float 转 QByteArray
float  floatVar = 199;

QByteArray arrayi;
int len_floatVar = sizeof(floatVar);
arrayi.resize(len_floatVar);
memcpy(arrayi.data(), &floatVar, len_floatVar);

// QByteArray 转 float
// array 数据接上面
float  outfloatVar;
memcpy(&outfloatVar, arrayi.data(), len_floatVar);
}

{
// float[] 转 QByteArray
float  fVar[4] = { 199.1, 2.3, 9.5, 0.2 };//初始化变量赋值
QByteArray arrayf;
int len_fVar = sizeof(fVar); // 4*4 = 16 (一个float占4个字节)
arrayf.resize(len_fVar);
memcpy(arrayf.data(), &fVar, len_fVar);

//  QByteArray 转 float[]
float  outFvar[4];
memcpy(&outFvar, arrayf.data(), len_fVar);
}*/
//添加时间到状态栏
static void setClockToStatusBar(QStatusBar *statusBar,QWidget *parent)
{
    QLabel *lblTime=new QLabel(parent);
    statusBar->addPermanentWidget(lblTime);
    QTimer *timeNow=new QTimer(parent);
    QObject::connect(timeNow,&QTimer::timeout,[=](){
        QDateTime time=QDateTime::currentDateTime();
        QString strtime=time.toString("yyyy/MM/dd hh:mm:ss");
        lblTime->setText(strtime);
    });
    timeNow->start(1000);
}
//添加链接按钮到状态栏
static void setLinkButtonToStatusBar(QString text,QString link,QStatusBar *statusBar,QWidget *parent)
{
    QLabel *linklbl=new QLabel(parent);
    linklbl->setOpenExternalLinks(true);
    linklbl->setText(QString::fromLocal8Bit("<a style='color:gray;' href=\"%1\">%2</a>").arg(link,text));
    statusBar->addWidget(linklbl,0);
}

//删除超出日期得文件夹
static void deleteFolderOutOfRange(QString dir,int dayNum)
{
    QStringList folderlist = GetAllFolderPath(dir);
    for (int i=0;i<folderlist.size()-dayNum;i++)
    {
        deleteFileOrFolder(folderlist.at(i));
    }
}

//拷贝文件夹：
static bool copyDirectoryFiles(const QString &fromDir, const QString &toDir, bool coverFileIfExist)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);
    if(!targetDir.exists()){    /**< 如果目标目录不存在，则进行创建 */
        if(!targetDir.mkdir(targetDir.absolutePath()))
            return false;
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    foreach(QFileInfo fileInfo, fileInfoList){
        if(fileInfo.fileName() == "." || fileInfo.fileName() == "..")
            continue;

        if(fileInfo.isDir()){    /**< 当为目录时，递归的进行copy */
            if(!copyDirectoryFiles(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()),
                coverFileIfExist))
                return false;
        }
        else{            /**< 当允许覆盖操作时，将旧文件进行删除操作 */
            if(coverFileIfExist && targetDir.exists(fileInfo.fileName())){
                targetDir.remove(fileInfo.fileName());
            }

            /// 进行文件copy
            if(!QFile::copy(fileInfo.filePath(),
                targetDir.filePath(fileInfo.fileName()))){
                    return false;
            }
        }
    }
    return true;
}

//vector中是否含有某个元素
static bool isElementInStdVector(std::vector<std::string> v,std::string element)
{
    std::vector<std::string>::iterator it;
    it=find(v.begin(),v.end(),element);
    if (it!=v.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}
//移除tab所有行
static void removeTableAllRow(QTableWidget *table)
{
    int rowcount=table->rowCount();
    for(int i=0;i<rowcount;i++)
    {
        table->removeRow(0);
    }
}
//已运行提示
static void runExeRemind(QWidget *widgetprogress)
{
    widgetprogress->resize(300,200);
    QHBoxLayout *layout = new QHBoxLayout();
    QLabel *label_text=new QLabel();
    label_text->setText(QString::fromLocal8Bit("程序正在打开，请稍后..."));
    layout->addWidget(label_text);
    widgetprogress->setLayout(layout);
}

//初始化combobox
static void addItemsToCombobox(QComboBox* cbbox,QStringList itemList,QString itemUse)
{
    cbbox->clear();
    cbbox->addItem(itemUse);
    for (int i=0;i<itemList.size();i++)
    {
        if(itemList[i]==itemUse)
            continue;
        cbbox->addItem(itemList[i]);
    }
}
static void clearLayout(QLayout* layout, bool deleteWidgets)
{
    while (QLayoutItem* item = layout->takeAt(0))
    {
        if (deleteWidgets)
        {
            if (QWidget* widget = item->widget())
                widget->deleteLater();
        }
        if (QLayout* childLayout = item->layout())
            clearLayout(childLayout, deleteWidgets);
        delete item;
    }
}
static QString decodeQImage(QImage image,int codetype)
{
    QZXing decoder;
    //mandatory settings
    switch (codetype)
    {
    case 0:
        decoder.setDecoder(QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_CODE_128 );
        break;
    case 1:
        decoder.setDecoder(QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_CODE_128 );
        break;
    case 2:
        decoder.setDecoder(QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_CODE_128 );
        break;
    case 3:
        decoder.setDecoder(QZXing::DecoderFormat_QR_CODE | QZXing::DecoderFormat_CODE_128 );
        break;
    }
    //optional settings
    decoder.setSourceFilterType(QZXing::SourceFilter_ImageNormal);
    decoder.setTryHarderBehaviour(QZXing::TryHarderBehaviour_ThoroughScanning | QZXing::TryHarderBehaviour_Rotate);
    //trigger decode
    QString result = decoder.decodeImage(image);
    return result;
}


/*bool ParameterSet::eventFilter(QObject *obj, QEvent *event)
{

    if( event->type() == QEvent::MouseButtonPress)
    {
        // if yes, we need to cast the event into a QMouseEvent type
        QMouseEvent * pMouseEvent = static_cast<QMouseEvent *>(event);
        // check whether it's mid button pressed
        if (pMouseEvent->button() == Qt::RightButton)
        {
            //do the processing and return true
            int row=ui->tblwgtPar->currentRow();
            int col= ui->tblwgtPar->currentColumn();
            QComboBox *parTypeCombobox=new QComboBox(this);
            parTypeCombobox->addItems(parTypeList);
            ui->tblwgtPar->setCellWidget(row,col,parTypeCombobox);
            return true;   //一定要返回true，如果你不想别的object也能接收到这个event
        }
        return QWidget::eventFilter(obj,event);
    }
    return QWidget::eventFilter(obj,event);
}*/
//bool eventFilter(QObject *obj, QEvent *event);//事件过滤
//ui->tblwgtPar->viewport()->installEventFilter(this);//安装事件过滤器

template<class T>
void mult (T a1,T a2,T &a3)
{
    a3=a1*a2;
}



#define _T QString::fromLocal8Bit


};

#endif // GENERALFUNC_H
