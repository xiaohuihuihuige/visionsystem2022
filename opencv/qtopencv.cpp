#include "qtopencv.h"

QtOpencv::QtOpencv()
{

}

cv::Mat QtOpencv::qImageToMat(const QImage &image)
{
    cv::Mat mat;
    switch (image.format())
    {
    case QImage::
    Format_ARGB32:
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        break;
    case QImage::Format_RGB888:
        mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(mat, mat,cv::COLOR_RGB2BGR);
        break;
    case QImage::Format_Indexed8:
        mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void*)image.constBits(), image.bytesPerLine());
        break;
    }
    return mat;
}


QImage QtOpencv::cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        //return image.copy();
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        cv::Mat img3chan;
        cv::cvtColor(mat,img3chan,cv::COLOR_BGRA2RGB);//CV_RGBA2RGB表示4通道转成3通道
        // Copy input Mat
        const uchar *pSrc = (const uchar*)img3chan.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, img3chan.cols, img3chan.rows, img3chan.step, QImage::Format_RGB888);
        return image.copy();
    }
    else
    {
        return QImage();
    }
}

int QtOpencv::RGB2BGR( unsigned char* pRgbData, unsigned int nWidth, unsigned int nHeight )
{
    if ( NULL == pRgbData )
    {
        return 1;
    }

    for (unsigned int j = 0; j < nHeight; j++)
    {
        for (unsigned int i = 0; i < nWidth; i++)
        {
            unsigned char red = pRgbData[j * (nWidth * 3) + i * 3];
            pRgbData[j * (nWidth * 3) + i * 3]     = pRgbData[j * (nWidth * 3) + i * 3 + 2];
            pRgbData[j * (nWidth * 3) + i * 3 + 2] = red;
        }
    }

    return 0;
}


cv::Mat QtOpencv::unsignchar2Mat(int height,int width,int channel, unsigned char * pData)
{
    cv::Mat srcImage;
    if ( channel == 1 )
    {
        srcImage = cv::Mat(height, width, CV_8UC1, pData);
    }
    else if ( channel == 3 )
    {
        RGB2BGR(pData, width, height);
        srcImage = cv::Mat(height, width, CV_8UC3, pData);
    }
    else
    {
        return srcImage;
    }
    if ( NULL == srcImage.data )
    {
        return srcImage;
    }
    return srcImage;
}

void QtOpencv::showImage(cv::Mat image, QWidget *widget)
{
    QImage qimage=cvMat2QImage(image);
    QPixmap image1=QPixmap::fromImage(qimage);//重新调整图像大小以适应窗口
    widget->setAutoFillBackground(true); // 这句要加上, 否则可能显示不出背景图.
    QPalette palette = widget->palette();
    palette.setBrush(QPalette::Window,
                     QBrush(image1.scaled(widget->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation))); // 使用平滑的缩放方式
    widget->setPalette(palette); // 至此, 已给widget加上了背景图.
}

void QtOpencv::showImage(cv::Mat image, QLabel *lbl)
{
    QImage qimage=cvMat2QImage(image);
    QPixmap image1=QPixmap::fromImage(qimage);//重新调整图像大小以适应窗口
    QPixmap image2=image1.scaled(lbl->size()-QSize(2,2),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);//重新调整图像大小以适应窗口
    lbl->setPixmap(image2);
}

void QtOpencv::GetStringSize(HDC hDC, const char* str, int* w, int* h)
{
    SIZE size;
    GetTextExtentPoint32A(hDC, str, strlen(str), &size);
    if (w != 0) *w = size.cx;
    if (h != 0) *h = size.cy;
}

void QtOpencv::putTextZH(cv::Mat &dst, const char* str, cv::Point org, cv::Scalar color, int fontSize, const char* fn, bool italic, bool underline)
{
    CV_Assert(dst.data != 0 && (dst.channels() == 1 || dst.channels() == 3||dst.channels() == 4));
    if(dst.channels() == 1)
    {
        cv::cvtColor(dst,dst,cv::COLOR_GRAY2BGR);
    }
    else if(dst.channels() == 1)
    {
        cv::cvtColor(dst,dst,cv::COLOR_BGRA2RGB);

    }
    int x, y, r, b;
    if (org.x > dst.cols || org.y > dst.rows) return;
    x = org.x < 0 ? -org.x : 0;
    y = org.y < 0 ? -org.y : 0;

    LOGFONTA lf;
    lf.lfHeight = -fontSize;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 5;
    lf.lfItalic = italic;   //斜体
    lf.lfUnderline = underline; //下划线
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = PROOF_QUALITY;
    lf.lfPitchAndFamily = 0;
    strcpy_s(lf.lfFaceName, fn);

    HFONT hf = CreateFontIndirectA(&lf);
    HDC hDC = CreateCompatibleDC(0);
    HFONT hOldFont = (HFONT)SelectObject(hDC, hf);

    int strBaseW = 0, strBaseH = 0;
    int singleRow = 0;
    char buf[1 << 12];
    strcpy_s(buf, str);
    char *bufT[1 << 12];  // 这个用于分隔字符串后剩余的字符，可能会超出。
    //处理多行
    {
        int nnh = 0;
        int cw, ch;

        const char* ln = strtok_s(buf, "\n",bufT);
        while (ln != 0)
        {
            GetStringSize(hDC, ln, &cw, &ch);
            strBaseW = max(strBaseW, cw);
            strBaseH = max(strBaseH, ch);

            ln = strtok_s(0, "\n",bufT);
            nnh++;
        }
        singleRow = strBaseH;
        strBaseH *= nnh;
    }

    if (org.x + strBaseW < 0 || org.y + strBaseH < 0)
    {
        SelectObject(hDC, hOldFont);
        DeleteObject(hf);
        DeleteObject(hDC);
        return;
    }

    r = org.x + strBaseW > dst.cols ? dst.cols - org.x - 1 : strBaseW - 1;
    b = org.y + strBaseH > dst.rows ? dst.rows - org.y - 1 : strBaseH - 1;
    org.x = org.x < 0 ? 0 : org.x;
    org.y = org.y < 0 ? 0 : org.y;

    BITMAPINFO bmp = { 0 };
    BITMAPINFOHEADER& bih = bmp.bmiHeader;
    int strDrawLineStep = strBaseW * 3 % 4 == 0 ? strBaseW * 3 : (strBaseW * 3 + 4 - ((strBaseW * 3) % 4));

    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = strBaseW;
    bih.biHeight = strBaseH;
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = strBaseH * strDrawLineStep;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    void* pDibData = 0;
    HBITMAP hBmp = CreateDIBSection(hDC, &bmp, DIB_RGB_COLORS, &pDibData, 0, 0);

    CV_Assert(pDibData != 0);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hDC, hBmp);

    //color.val[2], color.val[1], color.val[0]
    SetTextColor(hDC, RGB(255, 255, 255));
    SetBkColor(hDC, 0);
    //SetStretchBltMode(hDC, COLORONCOLOR);

    strcpy_s(buf, str);
    const char* ln = strtok_s(buf, "\n",bufT);
    int outTextY = 0;
    while (ln != 0)
    {
        TextOutA(hDC, 0, outTextY, ln, strlen(ln));
        outTextY += singleRow;
        ln = strtok_s(0, "\n",bufT);
    }
    uchar* dstData = (uchar*)dst.data;
    int dstStep = dst.step / sizeof(dstData[0]);
    unsigned char* pImg = (unsigned char*)dst.data + org.x * dst.channels() + org.y * dstStep;
    unsigned char* pStr = (unsigned char*)pDibData + x * 3;
    for (int tty = y; tty <= b; ++tty)
    {
        unsigned char* subImg = pImg + (tty - y) * dstStep;
        unsigned char* subStr = pStr + (strBaseH - tty - 1) * strDrawLineStep;
        for (int ttx = x; ttx <= r; ++ttx)
        {
            for (int n = 0; n < dst.channels(); ++n){
                double vtxt = subStr[n] / 255.0;
                int cvv = vtxt * color.val[n] + (1 - vtxt) * subImg[n];
                subImg[n] = cvv > 255 ? 255 : (cvv < 0 ? 0 : cvv);
            }

            subStr += 3;
            subImg += dst.channels();
        }
    }

    SelectObject(hDC, hOldBmp);
    SelectObject(hDC, hOldFont);
    DeleteObject(hf);
    DeleteObject(hBmp);
    DeleteDC(hDC);
}

//"row|col|W|H"
void QtOpencv::drawRectangle1(cv::Mat srcdst, QString rectstr,const cv::Scalar& color,int thick)
{
    QStringList strlist=rectstr.split('|');
    cv::Rect rect=cv::Rect(strlist[0].toInt(),strlist[1].toInt(),strlist[2].toInt(),strlist[3].toInt());
    cv::rectangle(srcdst,rect,color, thick);
}
void QtOpencv::calculateRectInMat(cv::Mat image,QPoint firstPoint,QPoint lastPoint,QSize controlsize, QString *rectstr)
{
    //得宽高
    int w = image.cols;
    int h = image.rows;
    float row_ratio,col_ratio;
    row_ratio=float(w)/controlsize.width();
    col_ratio=float(h)/controlsize.height();
    int row,col,width,height;
    row=int(firstPoint.x()*row_ratio);
    col=int(firstPoint.y()*col_ratio);
    width=int((lastPoint.x()-firstPoint.x())*row_ratio);
    height=int((lastPoint.y()-firstPoint.y())*col_ratio);
    *rectstr=QString("%1|%2|%3|%4").arg(QString::number(row),QString::number(col),QString::number(width),QString::number(height));
}

void QtOpencv::calculateRect2InMat(cv::Mat src,cv::RotatedRect rotaterect,QSize controlsize, QString *rectstr)
{
    //得宽高
    if(src.empty())return;
    double ratio_x=double(src.cols)/controlsize.width();
    double ratio_y=double(src.rows)/controlsize.height();
    cv::RotatedRect rect=cv::RotatedRect(cv::Point2f(rotaterect.center.x*ratio_x,rotaterect.center.y*ratio_y),
                                         cv::Size(rotaterect.size.width*ratio_x,rotaterect.size.height*ratio_y),
                                         rotaterect.angle);
    *rectstr=QString("%1|%2|%3|%4|%5").arg(QString::number(rect.center.x,'f',3),QString::number(rect.center.y,'f',3),
                                           QString::number(rect.size.width,'f',3),QString::number(rect.size.height,'f',3),
                                           QString::number(rotaterect.angle,'f',3));
}


void QtOpencv::getMousePosGray(cv::Mat image,QPoint point,QSize controlsize,QPoint *pos,QVector<int> *value )
{
    QVector<int> grayValue=QVector<int>();
    //得宽高
    int w = image.cols;
    int h = image.rows;
    float row_ratio,col_ratio;
    row_ratio=float(w)/controlsize.width();
    col_ratio=float(h)/controlsize.height();
    int row,col;
    row=int(point.y()*col_ratio);
    col=int(point.x()*row_ratio);
    *pos=QPoint(row,col);
    int channels = image.channels();
    if (channels == 1)
    {
        //得到初始位置的迭代器
        cv::Mat_<uchar>::iterator it = image.begin<uchar>();
        //得到终止位置的迭代器
        cv::Mat_<uchar>::iterator itend = image.end<uchar>();
        int pixel = *(it + row * w + col);
        *value<<pixel;
    }
    else if (channels == 3)
    {
        //得到初始位置的迭代器
        cv::Mat_<cv::Vec3b>::iterator it = image.begin<cv::Vec3b>();
        //得到终止位置的迭代器
        cv::Mat_<cv::Vec3b>::iterator itend = image.end<cv::Vec3b>();
        //读取
        it = it + row * w + col;
        int b = (*it)[0];
        int g = (*it)[1];
        int r = (*it)[2];
        *value<<b<<g<<r;
    }
    else if (channels == 4)
    {
        //得到初始位置的迭代器
        cv::Mat_<cv::Vec4b>::iterator it = image.begin<cv::Vec4b>();
        //得到终止位置的迭代器
        cv::Mat_<cv::Vec4b>::iterator itend = image.end<cv::Vec4b>();
        //读取
        it = it + row * w + col;
        int b = (*it)[0];
        int g = (*it)[1];
        int r = (*it)[2];
        int a = (*it)[3];
        *value<<b<<g<<r<<a;
    }
}
//分离RGB图像
bool QtOpencv::departRGBImage(cv::Mat src,cv::Mat *r_image,cv::Mat *g_image,cv::Mat *b_image)
{
    if(src.empty())return false;
    cv::Mat img =src.clone();
    if(img.channels()!=3)return false;
    cv::Mat srcImage_B, srcImage_G, srcImage_R; //R、G、B各个通道
    std::vector<cv::Mat> channels_BGR;
    split(src, channels_BGR);
    //0通道为B分量，1通道为G分量，2通道为R分量。因为：RGB色彩空间在opencv中默认通道顺序为BGR！！！
    srcImage_B = channels_BGR.at(0);
    srcImage_G = channels_BGR.at(1);
    srcImage_R = channels_BGR.at(2);
    *b_image=srcImage_B.clone();
    *g_image=srcImage_G.clone();
    *r_image=srcImage_R.clone();
    return true;
}
//图像上绘制旋转矩形
bool QtOpencv::drawRotateRect(cv::Mat src,cv::RotatedRect rotaterect,QSize controlsize,cv::Scalar color, cv::Mat *dst)
{
    if(src.empty())return false;
    double ratio_x=double(src.cols)/controlsize.width();
    double ratio_y=double(src.rows)/controlsize.height();
    cv::RotatedRect rect=cv::RotatedRect( cv::Point2f(rotaterect.center.x*ratio_x,rotaterect.center.y*ratio_y),
                                          cv::Size(rotaterect.size.width*ratio_x,rotaterect.size.height*ratio_y),
                                          rotaterect.angle);
    cv::Mat dstImg=src.clone();
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);

    cv::Point2f rectPoints[4];
    rect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(dstImg,contour,0,color,5);
    *dst=dstImg.clone();
    return true;
}
//图像上绘制旋转矩形
bool QtOpencv::drawRotateRect(cv::Mat src,cv::RotatedRect rotaterect,cv::Scalar color,cv::Mat *dst)
{
    if(src.empty())return false;
    cv::Mat dstImg=src.clone();
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);

    cv::Point2f rectPoints[4];
    rotaterect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(dstImg,contour,0,color,5);
    *dst=dstImg.clone();
    return true;
}
//图像上绘制旋转矩形
bool QtOpencv::drawRotateRect(cv::Mat src,QString strrotaterect,cv::Scalar color,cv::Mat *dst)
{
    if(src.empty())return false;
    QStringList strlist=strrotaterect.split('|');
    if(strlist.size()<5)
    {
        *dst=src.clone();
        return false;
    }
    cv::Point2f center_=cv::Point2f(strlist[0].toFloat(),strlist[1].toFloat());
    cv::Size size_=cv::Size(strlist[2].toFloat(),strlist[3].toFloat());
    double angle= strlist[4].toDouble();
    cv::RotatedRect rect=cv::RotatedRect(center_,size_,angle);
    cv::Mat dstImg=src.clone();
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);
    cv::Point2f rectPoints[4];
    rect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(dstImg,contour,0,color,5);
    *dst=dstImg.clone();
    return true;
}
//获取旋转矩形区域，原始图像，旋转矩形，控件尺寸，是否摆正（摆正默认裁剪），是否裁剪，输出图像
bool QtOpencv::getRotateRectRoi(cv::Mat src,cv::RotatedRect rotaterect,QSize controlsize,bool isStraighten,bool iscut,cv::Mat *dst)
{
    if(src.empty())return false;
    double ratio_x=double(src.cols)/controlsize.width();
    double ratio_y=double(src.rows)/controlsize.height();
    cv::RotatedRect rect=cv::RotatedRect( cv::Point2f(rotaterect.center.x*ratio_x,rotaterect.center.y*ratio_y),
                                          cv::Size(rotaterect.size.width*ratio_x,rotaterect.size.height*ratio_y),
                                          rotaterect.angle);

    cv::Mat dstImg;
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);

    cv::Point2f rectPoints[4];
    rect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(mask,contour,0,cv::Scalar::all(255),-1);
    src.copyTo(dstImg,mask);
    if(isStraighten)
    {
        cv::Point2f center = rect.center;//外接矩形中心点坐标
        cv::Mat rot_mat = getRotationMatrix2D(center, rect.angle, 1.0);//求旋转矩阵
        cv::Size dst_sz(src.size());
        cv::warpAffine(src, dstImg, rot_mat, dst_sz);//原图像旋转
        *dst = dstImg(cv::Rect(center.x - (rect.size.width / 2), center.y - (rect.size.height/2), rect.size.width, rect.size.height));//提取ROI
        return  true;
    }
    if(iscut)
    {
        cv::Point2f point_upleft,point_bottomright;
        point_upleft = rect.center;
        for (int j = 0; j < 4; j++)
        {
            point_upleft.x = rectPoints[j].x < point_upleft.x ? rectPoints[j].x : point_upleft.x;    //矩形的右边长
            if(point_upleft.x<0)point_upleft.x=0;
            point_bottomright.x = rectPoints[j].x > point_bottomright.x ? rectPoints[j].x : point_bottomright.x;     //矩形的左边长
            if(point_bottomright.x>dstImg.cols)point_bottomright.x=dstImg.cols;
            point_upleft.y = rectPoints[j].y < point_upleft.y ? rectPoints[j].y : point_upleft.y;    //矩形的上边长
            if(point_upleft.y<0)point_upleft.y=0;
            point_bottomright.y = rectPoints[j].y > point_bottomright.y ? rectPoints[j].y : point_bottomright.y;     //矩形的下边长
            if(point_bottomright.y>dstImg.rows)point_bottomright.y=dstImg.rows;
        }
        cv::Rect rect1=cv::Rect(point_upleft,point_bottomright);
        *dst=dstImg(rect1);
        return true;
    }
    return true;
}
//获取旋转矩形区域，原始图像，旋转矩形，是否摆正（摆正默认裁剪），是否裁剪，输出图像
bool QtOpencv::getRotateRectRoi(cv::Mat src,QString strrotaterect,bool isStraighten,bool iscut,cv::Mat *dst)
{
    if(src.empty())return false;
    QStringList strlist=strrotaterect.split('|');
    if(strlist.size()<5)
    {
        *dst=src.clone();
        return false;
    }
    cv::Point2f center_=cv::Point2f(strlist[0].toFloat(),strlist[1].toFloat());
    cv::Size size_=cv::Size(strlist[2].toFloat(),strlist[3].toFloat());
    double angle= strlist[4].toDouble();

    cv::RotatedRect rect=cv::RotatedRect(center_,size_,angle);

    cv::Mat dstImg;
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);

    cv::Point2f rectPoints[4];
    rect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(mask,contour,0,cv::Scalar::all(255),-1);
    src.copyTo(dstImg,mask);
    if(isStraighten)
    {
        cv::Point2f center = rect.center;//外接矩形中心点坐标
        cv::Mat rot_mat = getRotationMatrix2D(center, rect.angle, 1.0);//求旋转矩阵
        cv::Size dst_sz(src.size());
        cv::warpAffine(src, dstImg, rot_mat, dst_sz);//原图像旋转
        *dst = dstImg(cv::Rect(center.x - (rect.size.width / 2), center.y - (rect.size.height/2), rect.size.width, rect.size.height));//提取ROI
        return  true;
    }
    if(iscut)
    {
        cv::Point2f point_upleft,point_bottomright;
        point_upleft = rect.center;
        for (int j = 0; j < 4; j++)
        {
            point_upleft.x = rectPoints[j].x < point_upleft.x ? rectPoints[j].x : point_upleft.x;    //矩形的右边长
            if(point_upleft.x<0)point_upleft.x=0;
            point_bottomright.x = rectPoints[j].x > point_bottomright.x ? rectPoints[j].x : point_bottomright.x;     //矩形的左边长
            if(point_bottomright.x>dstImg.cols)point_bottomright.x=dstImg.cols;
            point_upleft.y = rectPoints[j].y < point_upleft.y ? rectPoints[j].y : point_upleft.y;    //矩形的上边长
            if(point_upleft.y<0)point_upleft.y=0;
            point_bottomright.y = rectPoints[j].y > point_bottomright.y ? rectPoints[j].y : point_bottomright.y;     //矩形的下边长
            if(point_bottomright.y>dstImg.rows)point_bottomright.y=dstImg.rows;
        }
        cv::Rect rect1=cv::Rect(point_upleft,point_bottomright);
        *dst=dstImg(rect1);
        return true;
    }
    return true;
}

//获取旋转矩形区域，原始图像，旋转矩形，是否摆正（摆正默认裁剪），是否裁剪，输出图像
bool QtOpencv::getRotateRectRoi(cv::Mat src,cv::RotatedRect rect,bool isStraighten,bool iscut,cv::Mat *dst)
{
    if(src.empty())return false;
    cv::Mat dstImg;
    cv::Mat mask = cv::Mat::zeros(src.size(),CV_8UC1);

    cv::Point2f rectPoints[4];
    rect.points(rectPoints);
    std::vector<std::vector<cv::Point>> contour;
    std::vector<cv::Point> pts;
    for (int i=0;i<sizeof (rectPoints)/sizeof (rectPoints[0]);i++)
    {
        pts.push_back(rectPoints[i]);
    }
    contour.push_back(pts);
    drawContours(mask,contour,0,cv::Scalar::all(255),-1);
    src.copyTo(dstImg,mask);
    if(isStraighten)
    {
        cv::Point2f center = rect.center;//外接矩形中心点坐标
        cv::Mat rot_mat = getRotationMatrix2D(center, rect.angle, 1.0);//求旋转矩阵
        cv::Size dst_sz(src.size());
        cv::warpAffine(src, dstImg, rot_mat, dst_sz);//原图像旋转
        *dst = dstImg(cv::Rect(center.x - (rect.size.width / 2), center.y - (rect.size.height/2), rect.size.width, rect.size.height));//提取ROI
        return  true;
    }
    if(iscut)
    {
        cv::Point2f point_upleft,point_bottomright;
        point_upleft = rect.center;
        for (int j = 0; j < 4; j++)
        {
            point_upleft.x = rectPoints[j].x < point_upleft.x ? rectPoints[j].x : point_upleft.x;    //矩形的右边长
            if(point_upleft.x<0)point_upleft.x=0;
            point_bottomright.x = rectPoints[j].x > point_bottomright.x ? rectPoints[j].x : point_bottomright.x;     //矩形的左边长
            if(point_bottomright.x>dstImg.cols)point_bottomright.x=dstImg.cols;
            point_upleft.y = rectPoints[j].y < point_upleft.y ? rectPoints[j].y : point_upleft.y;    //矩形的上边长
            if(point_upleft.y<0)point_upleft.y=0;
            point_bottomright.y = rectPoints[j].y > point_bottomright.y ? rectPoints[j].y : point_bottomright.y;     //矩形的下边长
            if(point_bottomright.y>dstImg.rows)point_bottomright.y=dstImg.rows;
        }
        cv::Rect rect1=cv::Rect(point_upleft,point_bottomright);
        *dst=dstImg(rect1);
        return true;
    }
    return true;
}
//水平拼接direction为0，垂直为1
cv::Mat QtOpencv::connectMat(QList<cv::Mat> matlist,int direction)
{
    if(matlist.size()<1)return cv::Mat();

    if(direction==0)
    {
        int width=0;
        int height=matlist[0].rows;
        cv::Mat  resultImg;
        int matstart=0;
        for (int i=0;i<matlist.size();i++)
        {
            cv::Mat dst;
            cv::resize(matlist[i],dst,cv::Size(matlist[i].cols,height));
            width+=dst.cols;
            cv::Mat ori=resultImg.clone();
            if(matlist[0].channels()==1)
            {
                resultImg= cv::Mat(height, width, CV_8UC1, cv::Scalar::all(0));
            }
            else if(matlist[0].channels()==3)
            {
                resultImg= cv::Mat(height, width, CV_8UC3, cv::Scalar::all(0));
            }
            cv::Mat ROI = resultImg(cv::Rect(0, 0, ori.cols, ori.rows));
            ori.copyTo(ROI);
            ROI = resultImg(cv::Rect(matstart, 0, dst.cols, dst.rows));
            dst.copyTo(ROI);
            matstart+=dst.cols;

        }
        return resultImg;
    }
    else if(direction==1)
    {
        int width=matlist[0].cols;
        int height=0;
        cv::Mat  resultImg;
        int matstart=0;
        for (int i=0;i<matlist.size();i++)
        {
            cv::Mat dst;
            cv::resize(matlist[i],dst,cv::Size(width,matlist[i].rows));
            height+=dst.rows;
            cv::Mat ori=resultImg.clone();
            if(matlist[0].channels()==1)
            {
                resultImg= cv::Mat(height, width, CV_8UC1, cv::Scalar::all(0));
            }
            else if(matlist[0].channels()==3)
            {
                resultImg= cv::Mat(height, width, CV_8UC3, cv::Scalar::all(0));
            }
            cv::Mat ROI = resultImg(cv::Rect(0, 0, ori.cols, ori.rows));
            ori.copyTo(ROI);
            ROI = resultImg(cv::Rect(0, matstart, dst.cols, dst.rows));
            dst.copyTo(ROI);
            matstart+=dst.rows;

        }
        return resultImg;
    }
}

/*cv::Mat labels, stats, centroids;
int roinum=cv::connectedComponentsWithStats(check_img, labels, stats,centroids);*/
cv::Mat QtOpencv::selectShape(cv::Mat src,cv::ConnectedComponentsTypes select_type,int min,int max)
{
    cv::Mat labels, stats, centroids;
    int roiNum=cv::connectedComponentsWithStats(src, labels, stats,centroids);
    std::vector<int> colors(roiNum);
    colors[0] =0; // background pixels remain black.
    switch (select_type)
    {
    case cv::CC_STAT_TOP:
        for( int i = 1; i < roiNum; i++ )
        {
            int a=stats.at<int>(i, cv::CC_STAT_TOP);
            if( stats.at<int>(i, cv::CC_STAT_TOP)>=min && stats.at<int>(i, cv::CC_STAT_TOP) <= max  )
                colors[i] = 255; // small regions are painted with black too.
            else
                colors[i] = 0;
        }
        break;
    case cv::CC_STAT_LEFT:
        for( int i = 1; i < roiNum; i++ )
        {
            int a=stats.at<int>(i, cv::CC_STAT_LEFT);
            if( stats.at<int>(i, cv::CC_STAT_LEFT)>=min && stats.at<int>(i, cv::CC_STAT_LEFT) <= max  )
                colors[i] = 255; // small regions are painted with black too.
            else
                colors[i] = 0;
        }
        break;
    case cv::CC_STAT_WIDTH:
        for( int i = 1; i < roiNum; i++ )
        {
            int a=stats.at<int>(i, cv::CC_STAT_WIDTH);
            if( stats.at<int>(i, cv::CC_STAT_WIDTH)>=min && stats.at<int>(i, cv::CC_STAT_WIDTH) <= max  )
                colors[i] = 255; // small regions are painted with black too.
            else
                colors[i] = 0;
        }
        break;
    case cv::CC_STAT_HEIGHT:
        for( int i = 1; i < roiNum; i++ )
        {
            int a=stats.at<int>(i, cv::CC_STAT_HEIGHT);
            if( stats.at<int>(i, cv::CC_STAT_HEIGHT)>=min && stats.at<int>(i, cv::CC_STAT_HEIGHT) <= max  )
                colors[i] = 255; // small regions are painted with black too.
            else
                colors[i] = 0;
        }
        break;
    case cv::CC_STAT_AREA:
        for( int i = 1; i < roiNum; i++ )
        {
            int a=stats.at<int>(i, cv::CC_STAT_AREA);
            if( stats.at<int>(i, cv::CC_STAT_AREA)>=min && stats.at<int>(i, cv::CC_STAT_AREA) <= max  )
                colors[i] = 255; // small regions are painted with black too.
            else
                colors[i] = 0;
        }
        break;
    }
    cv::Mat img = cv::Mat::zeros(src.size(), CV_8UC1);
    for( int y = 0; y < img.rows; y++ )
    {
        for( int x = 0; x < img.cols; x++ )
        {
            int label = labels.at<int>(y, x);
            CV_Assert(0 <= label && label <= roiNum);
            img.at<uchar>(y, x) = colors[label];
        }
    }
    return img;
}
void QtOpencv::rotateImage(cv::Mat &src,cv::Mat &dst,float angle)
{
    if(angle==90.0)
    {
        dst= cv::Mat(src.rows, src.cols,src.depth());
        cv::transpose(src, dst);
        cv::flip(dst, dst, 0);  //rotate 90
    }
    else if (angle==180)
    {
        flip(src, dst, -1);
    }
    else if (angle==270)
    {
        dst= cv::Mat(src.rows, src.cols,src.depth());
        cv::transpose(src, dst);
        cv::flip(dst, dst, 1);  //rotate 90
    }
    else
    {
        float radian = (float) (angle /180.0 * CV_PI);
        //填充图像
        int maxBorder =(int) (max(src.cols, src.rows)* 1.414 ); //即为sqrt(2)*max
        int dx = (maxBorder - src.cols)/2;
        int dy = (maxBorder - src.rows)/2;
        copyMakeBorder(src, dst, dy, dy, dx, dx, cv::BORDER_CONSTANT);

        //旋转
        cv::Point2f center( (float)(dst.cols/2) , (float) (dst.rows/2));
        cv::Mat affine_matrix =getRotationMatrix2D( center, angle, 1.0 );//求得旋转矩阵
        warpAffine(dst, dst, affine_matrix, dst.size());

        //计算图像旋转之后包含图像的最大的矩形
        float sinVal = abs(sin(radian));
        float cosVal = abs(cos(radian));
        cv::Size targetSize( (int)(src.cols * cosVal +src.rows * sinVal),
                             (int)(src.cols * sinVal + src.rows * cosVal) );

        //剪掉多余边框
        int x = (dst.cols - targetSize.width) / 2;
        int y = (dst.rows - targetSize.height) / 2;
        cv::Rect rect(x, y, targetSize.width, targetSize.height);
        dst = cv::Mat(dst,rect);
        return;
    }
}

void QtOpencv::erisionRectangle1(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(width, height));
    cv::erode(src, dst,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);

}
void QtOpencv::dilateRectangle1(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(width, height));
    cv::dilate(src, dst,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);

}
void QtOpencv::erisionCircle(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(width, height));
    cv::erode(src, dst,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
void QtOpencv::dilateCircle(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(width, height));
    cv::dilate(src, dst,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
void QtOpencv::openingRectangle1(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(width, height));
    cv::morphologyEx(src, dst,cv::MORPH_OPEN,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
void QtOpencv::closingRectangle1(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_RECT, cv::Size(width, height));
    cv::morphologyEx(src, dst,cv::MORPH_CLOSE,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
void QtOpencv::openingCircle(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(width, height));
    cv::morphologyEx(src, dst,cv::MORPH_OPEN,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
void QtOpencv::closingCircle(cv::Mat src,cv::Mat dst,int width,int height)
{
    cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(width, height));
    cv::morphologyEx(src, dst,cv::MORPH_CLOSE,element);
    //cv::threshold(dst,dst,250,255,cv::THRESH_BINARY);
}
cv::Mat QtOpencv::paintImageToRect(cv::Mat src,cv::Mat paintImage,cv::Rect rect)
{
    cv::Mat dst=src.clone();
    cv::Mat image;
    cv::resize(paintImage,image,rect.size());
    cv::Mat ROI = dst(rect);
    image.copyTo(ROI);
    return dst;
}
cv::Mat QtOpencv::scaleImage(cv::Mat src,cv::Size size,int channels)
{
    cv::Mat op_mat=src.clone();
    //新建空白图像
    cv::Mat image_scale(size,CV_8UC(channels), cv::Scalar(0,0,0));
    //计算缩放比例
    float scale_x=float(op_mat.cols)/size.width;
    float scale_y=float(op_mat.rows)/size.height;
    float scale_ratio=(scale_x>scale_y?scale_x:scale_y);
    //计算缩放后的尺寸
    int new_rows,new_cols;
    scale_x>=scale_y?scale_y=scale_x:scale_x=scale_y;
    new_cols=int(op_mat.cols/scale_ratio);
    new_cols>size.width?new_cols=size.width:new_cols;
    new_rows=int(op_mat.rows/scale_ratio);
    new_rows>size.height?new_rows=size.height:new_rows;
    //计算偏移尺寸
    int move_x,move_y;
    move_x=(size.width-new_cols)/2;
    move_y=(size.height-new_rows)/2;
    //等比例缩放并边缘为0
    cv::resize(op_mat,op_mat,cv::Size(new_cols,new_rows));
    op_mat.copyTo(image_scale(cv::Rect(move_x,move_y,new_cols,new_rows)));
    return image_scale;
}

int QtOpencv::trainTemplate(cv::Mat img, std::string filePath, float angle_middle, float angle_range, float scale_start, float scale_step)
{

    int num_feature;
    num_feature = 1024;
    std::vector<int> level;
    level.push_back(2);
    level.push_back(4);
    line2Dup::Detector detector(num_feature, level, 15.0f, 55.0f);
    std::string mode = "train";
    if (mode == "train" || mode == "none")
    {
        /*int templ_width, templ_height;
        //Mat img = imread(prefix + "case3/train10.png");
        int widadd=0,hei=0;
        img.cols%2==0?widadd=0:widadd=1;
        img.rows%2==0?hei=0:hei=1;
        cv::copyMakeBorder(img, img, 0, hei, 0, widadd, cv::BORDER_REPLICATE);
        templ_width=(((img.cols/8)*8+8)-img.cols)/2;
        templ_height=(((img.rows/8)*8+8)-img.rows)/2;
        cv::Mat destImage;
        copyMakeBorder(img, img, templ_height, templ_height, templ_width, templ_width, cv::BORDER_REPLICATE);
        cv::Mat mask = cv::Mat(img.size(), CV_8UC1);
        mask.setTo(0);
        cv::Rect r1(templ_width, templ_height, img.size().width - 2*templ_width, img.size().height - templ_height*2);//修复模版提取Mask
        mask(r1).setTo(255);*/
        int templ_width1, templ_width2, templ_height1, templ_height2;
        if (1)//边界扩充，扩大检测范围，保证边缘处查找，保证模版宽高为8的倍数
        {
            int length_sqrt = (int)cvSqrt(img.size().width*img.size().width*1.0 + img.size().height*img.size().height*1.0);

            int length_8 = length_sqrt - (length_sqrt + 8) % 8;
            templ_width1 = (length_8 - img.size().width) / 2;
            templ_width2 = length_8 - img.size().width - templ_width1;
            templ_height1 = (length_8 - img.size().height) / 2;
            templ_height2 = length_8 - img.size().height - templ_height1;
            cv::Mat destImage, test_img_origin;
            test_img_origin = img.clone();
            int top_y = templ_height1, down_y = templ_height2, left_x = templ_width1, right_x = templ_width2;
            copyMakeBorder(img, destImage, top_y, down_y, left_x, right_x, cv::BORDER_CONSTANT);
            img = destImage.clone();
        }
        cv::Mat mask = cv::Mat(img.size(), CV_8UC1);
        mask.setTo(0);
        cv::Rect r1(templ_width1 + 3, templ_height1 + 3, img.size().width - templ_width2 - templ_width1 - 6, img.size().height - templ_height2 - templ_height1 - 6);//修复模版提取Mask
        mask(r1).setTo(255);

        shape_based_matching::shapeInfo shapes(img, mask);
        shapes.angle_range.push_back(angle_middle - angle_range);
        shapes.angle_range.push_back(angle_middle + angle_range);
        shapes.angle_step = 1;
        if (scale_start == 1.0)
        {
            shapes.scale_range.push_back(scale_start);

        }
        else
        {
            shapes.scale_range.push_back(scale_start);
            shapes.scale_range.push_back(2.0 - scale_start);
        }
        shapes.scale_step = scale_step;
        shapes.produce_infos();

        std::vector<cv::Vec3b> color;
        color.push_back({255,0,0});
        color.push_back({0,255,0});
        color.push_back({0,0,255});
        color.push_back({255,255,0});
        color.push_back({255,0,255});
        color.push_back({0,0,255});
        color.push_back({128,0,0});
        color.push_back({0,128,0});
        color.push_back({0,0,128});
        std::vector<shape_based_matching::shapeInfo::shape_and_info> infos_have_templ;
        std::string class_id = "test";
        cv::Mat resultMat;
        bool iscreatemodel;
        for (auto it = shapes.infos.begin(); it != shapes.infos.end(); it++)
        {
            resultMat = (*it).src.clone();
            //imshow("ModelshowWnd", (*it).src);
            int templ_id = detector.addTemplate((*it).src, class_id, (*it).mask, (*it).angle, 1);
            if (templ_id != -1)
            {
                infos_have_templ.push_back((*it));
                auto templ = detector.getTemplates("test", templ_id);
                iscreatemodel = true;
                if (templ.size()<1)
                {
                    iscreatemodel = false;
                }
                /*for (int i = 0; i<templ[0].features.size(); i++)
                {
                    auto feat = templ[0].features[i];
                    cv::circle(resultMat, { feat.x , feat.y }, 2, color[feat.label], -1);
                }*/
            }
        }
        std::string pathchar_templ, path_info;
        pathchar_templ = filePath + "_templ.ymal";
        path_info = filePath + "_info.ymal";
        QFile file_templ(pathchar_templ.data());
        if (file_templ.exists())
        {
            remove(pathchar_templ.data());
        }
        QFile file_info(path_info.data());
        if (file_info.exists())
        {
            remove(path_info.data());
        }
        if (iscreatemodel)
        {
            detector.writeClasses(pathchar_templ);
            shapes.save_infos(infos_have_templ, shapes.src, shapes.mask, path_info);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}


inline bool QtOpencv::CompareScore(MatchResult a, MatchResult b)
{
    return a.score > b.score;
}

//原始图像，查找个数，最大重叠率，最大遮盖率，匹配结果数组
//原始图像，查找个数，最大重叠率，最大遮盖率，匹配结果数组
int QtOpencv::findContourModel(cv::Mat src, std::string filePath, int find_number, float min_score, float max_overlap, float max_cover, std::vector<MatchResult> *matchResult)
{
    int num_feature;
    num_feature = 1024;
    std::vector<int> level;
    level.push_back(2);
    level.push_back(4);
    line2Dup::Detector detector2(num_feature, level, 15.0f, 55.0f);
    std::vector<std::string> ids;
    ids.push_back("test");
    std::string pathchar_templ, path_info;
    pathchar_templ = filePath + "_templ.ymal";
    path_info = filePath + "_info.ymal";
    QFile fit(pathchar_templ.data());
    if (fit.exists())
    {
        detector2.readClasses(ids, pathchar_templ);
    }
    else
    {
        return -1;
    }
    auto templ = detector2.getTemplates("test", 0);
    int templ_width = templ[0].width;
    int templ_height = templ[0].height;
    cv::Mat destImage, test_img;//边界扩充，扩大检测范围，保证边缘处查找
    test_img = src.clone();
    int top_y = templ_height, down_y = templ_height, left_x = templ_width, right_x = templ_width;
    copyMakeBorder(test_img, destImage, top_y, down_y, left_x, right_x, cv::BORDER_REPLICATE);
    test_img = destImage.clone();
    int stride = 32;
    int n = test_img.rows / stride;
    int m = test_img.cols / stride;
    cv::Rect roi(0, 0, stride*m, stride*n);
    test_img = test_img(roi).clone();
    auto matches = detector2.match(test_img, min_score, ids);//第二个参数为匹配最低分数（百分制），越大则搜索越严格，速度越快
    size_t top5 = 300;
    if (top5>matches.size()) top5 = matches.size();
    std::vector<cv::Rect> boxes;
    std::vector<float> scores;
    std::vector<int> idxs(10000);
    for (auto it = matches.begin(); it != matches.end(); it++)
    {
        cv::Rect box;
        box.x = (*it).x;
        box.y = (*it).y;
        auto templ = detector2.getTemplates("test",(*it).template_id);
        box.width = templ[0].width;
        box.height = templ[0].height;
        boxes.push_back(box);
        scores.push_back((*it).similarity);
    }
    cv::dnn::NMSBoxes(boxes, scores, 0, max_overlap, idxs);//第四个参数越大则相同位置重叠的匹配保留得越多（极大值抑制
    std::vector<MatchResult> vMatchResult;
    for (auto it = idxs.begin(); it != idxs.end(); it++)
    {
        auto match = matches[(*it)];
        match.x = match.x - templ_width;
        match.y = match.y - templ_height;
        auto templ = detector2.getTemplates("test",match.template_id);
        /*int x = templ[0].width + match.x;
        int y = templ[0].height + match.y;
        int r = templ[0].width / 2;*/
        MatchResult mMatchResult;
        mMatchResult.features = templ[0].features;
        mMatchResult.score = match.similarity;
        mMatchResult.x = match.x /*+ templ_width / 2*/;
        mMatchResult.y = match.y /*+ templ_height / 2*/;
        mMatchResult.model_width = templ_width;
        mMatchResult.model_height = templ_height;
        mMatchResult.a = -templ[0].tl_a;
        vMatchResult.push_back(mMatchResult);
    }
    if (vMatchResult.size() == 0)
    {
        return 0;
    }
    sort(vMatchResult.begin(), vMatchResult.end(), CompareScore);
    for (int i = 0; i < vMatchResult.size() && (i < find_number || find_number == 0); i++)
    {
        (*matchResult).push_back(vMatchResult[i]);
    }
    return 1;
}


cv::Mat QtOpencv::imgTranslate(cv::Mat &matSrc, int xOffset, int yOffset, bool bScale)
{
    // 判断是否改变图像大小,并设定被复制ROI
    int nRows = matSrc.rows;
    int nCols = matSrc.cols;
    int nRowsRet = 0;
    int nColsRet = 0;
    cv::Rect rectSrc;
    cv::Rect rectRet;
    if (bScale)
    {
        nRowsRet = nRows + abs(yOffset);
        nColsRet = nCols + abs(xOffset);
        rectSrc.x = 0;
        rectSrc.y = 0;
        rectSrc.width = nCols;
        rectSrc.height = nRows;
    }
    else
    {
        nRowsRet = matSrc.rows;
        nColsRet = matSrc.cols;
        if (xOffset >= 0)
        {
            rectSrc.x = 0;
        }
        else
        {
            rectSrc.x = abs(xOffset);
        }
        if (yOffset >= 0)
        {
            rectSrc.y = 0;
        }
        else
        {
            rectSrc.y = abs(yOffset);
        }
        rectSrc.width = nCols - abs(xOffset);
        rectSrc.height = nRows - abs(yOffset);
    }
    // 修正输出的ROI
    if (xOffset >= 0)
    {
        rectRet.x = xOffset;
    }
    else
    {
        rectRet.x = 0;
    }
    if (yOffset >= 0)
    {
        rectRet.y = yOffset;
    }
    else
    {
        rectRet.y = 0;
    }
    rectRet.width = rectSrc.width;
    rectRet.height = rectSrc.height;
    // 复制图像
    cv::Mat matRet(nRowsRet, nColsRet, matSrc.type(), cv::Scalar(255));
    matSrc(rectSrc).copyTo(matRet(rectRet));
    return matRet;
}


//// type 为0时使用仿射变换方法，type为1时使用透视变换方法
//int QtOpencv::imgFineTrans(cv::Mat &src_model, cv::Mat &src_scene, cv::Mat &dst, int Range, int Step, int type)
//{
//    cv::Mat Diffimg_Min;
//    cv::Mat Diffimg;
//    cv::Mat OffsetImage;
//    cv::Mat RotateImage;
//    cv::Mat WrapImage;
//    cv::Mat OpeningImage;
//    int min_bad = 999;
//    int s = 1;
//   cv:: Mat element = getStructuringElement(0, cv::Size(2 * s + 1, 2 * s + 1), cv::Point(s, s));//提取边界
//                                                                                    //erode(Diffimg_Min, Erodeimg, element);//腐蚀
//    if (type)
//    {


//        min_bad = src_model.size().width* src_model.size().height;

//        for (int i = -Range; i < Range; i += Step)
//        {
//            for (int j = -Range; j < Range; j += Step)
//            {
//                for (int k = -Range; k < Range; k += Step)
//                {
//                    for (int l = -Range; l < Range; l += Step)
//                    {
//                        for (int m = -Range; m < Range; m += Step)
//                        {
//                            for (int n = -Range; n < Range; n += Step)
//                            {
//                                for (int o = -Range; o < Range; o += Step)
//                                {
//                                    for (int p = -Range; p < Range; p += Step)
//                                    {
//                                        //Mat ho_ImageOffset;
//                                        int img_height = src_model.rows;
//                                        int img_width = src_model.cols;

//                                        std::vector<cv::Point2f> corners(4);
//                                        corners[0] = cv::Point2f(0, 0);
//                                        corners[1] = cv::Point2f(img_width - 1, 0);
//                                        corners[2] = cv::Point2f(0, img_height - 1);
//                                        corners[3] = cv::Point2f(img_width - 1, img_height - 1);
//                                        std::vector<cv::Point2f> corners_trans(4);
//                                        corners_trans[0] = cv::Point2f(0 + i, 0 + j);
//                                        corners_trans[1] = cv::Point2f(img_width - 1 + k, 0 + l);
//                                        corners_trans[2] = cv::Point2f(0 + m, img_height - 1 + n);
//                                        corners_trans[3] = cv::Point2f(img_width - 1 + o, img_height - 1 + p);
//                                        cv::Mat Trans = getPerspectiveTransform(corners, corners_trans);
//                                        warpPerspective(src_scene, WrapImage, Trans, cv::Size(src_model.cols, src_model.rows), CV_INTER_NN, 0, cv::Scalar(255));

//                                        //ho_ImageOffset = imgTranslate(src_scene, i, j, 0);
//                                        absdiff(WrapImage, src_model, Diffimg);//用帧差法求绝对值

//                                        morphologyEx(Diffimg, OpeningImage, cv::MORPH_OPEN, element);
//                                        int bad = bSums(OpeningImage);//调用函数bSums

//                                        if (bad < min_bad)
//                                        {
//                                            Diffimg_Min = WrapImage.clone();
//                                            min_bad = bad;
//                                            //imshow("SceneshowWnd", Diffimg_Min);
//                                            //waitKey(300);
//                                            if (min_bad == 0)
//                                            {
//                                                Diffimg_Min.copyTo(dst, dst);
//                                                return min_bad;
//                                            }
//                                        }
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//        }

//        Diffimg_Min.copyTo(dst, dst);
//    }
//    else
//    {
//        min_bad = src_model.size().width* src_model.size().height;
//        for (float a = -1.0; a < 1.0; a += 0.5)
//        {
//            Point center(src_model.cols / 2, src_model.rows / 2);
//            Mat rot_mat = cv::getRotationMatrix2D(center, a, 1.0);

//            warpAffine(src_scene, RotateImage, rot_mat, src_model.size(), CV_INTER_NN, 0, Scalar(255));
//            for (int x = -Range; x < Range; x += Step)
//            {
//                for (int y = -Range; y < Range; y += Step)
//                {


//                    OffsetImage = imgTranslate(RotateImage, x, y, 0);
//                    absdiff(OffsetImage, src_model, Diffimg);//用帧差法求绝对值
//                    morphologyEx(Diffimg, OpeningImage, MORPH_OPEN, element);
//                    int bad = bSums(OpeningImage);//调用函数bSums

//                    if (bad < min_bad)
//                    {
//                        Diffimg_Min = OffsetImage.clone();
//                        min_bad = bad;
//                        if (min_bad == 0)
//                        {
//                            OffsetImage.copyTo(dst, dst);
//                            return min_bad;
//                        }
//                    }
//                }
//            }
//        }
//        Diffimg_Min.copyTo(dst, dst);
//    }

//    //return Diffimg_Min.clone();
//    return min_bad;
//}







