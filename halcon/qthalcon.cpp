#include "qthalcon.h"

QtHalcon::QtHalcon()
{

}

cv::Mat QtHalcon::HObjectToMat(HObject Hobj)
{
    HTuple htCh = HTuple();
    HTuple cType;
    cv::Mat Image;
    ConvertImageType(Hobj, &Hobj, "byte");
    CountChannels(Hobj, &htCh);
    HTuple wid;
    HTuple hgt;
    int W, H;
    if (htCh[0].I() == 1)
    {
        HTuple ptr;
        GetImagePointer1(Hobj, &ptr, &cType, &wid, &hgt);
        W = (Hlong)wid;
        H = (Hlong)hgt;
        Image.create(H, W, CV_8UC1);
        uchar* pdata = (uchar*)ptr[0].L();
        memcpy(Image.data, pdata, W*H);
    }
    else if (htCh[0].I() == 3)
    {
        HTuple ptrR, ptrG, ptrB;
        GetImagePointer3(Hobj, &ptrR, &ptrG, &ptrB, &cType, &wid, &hgt);
        W = (Hlong)wid;
        H = (Hlong)hgt;
        Image.create(H, W, CV_8UC3);
        std::vector<cv::Mat> vecM(3);
        vecM[2].create(H, W, CV_8UC1);
        vecM[1].create(H, W, CV_8UC1);
        vecM[0].create(H, W, CV_8UC1);
        uchar* pr = (uchar*)ptrR[0].L();
        uchar* pg = (uchar*)ptrG[0].L();
        uchar* pb = (uchar*)ptrB[0].L();
        memcpy(vecM[2].data, pr, W*H);
        memcpy(vecM[1].data, pg, W*H);
        memcpy(vecM[0].data, pb, W*H);
        merge(vecM, Image);
    }
    return Image;

}

HalconCpp::HObject QtHalcon::MatToHObject(cv::Mat& cv_img)
{
    HalconCpp::HObject H_img;

    if (cv_img.channels() == 1)
    {
        int height = cv_img.rows, width = cv_img.cols;
        int size = height * width;
        uchar *temp = new uchar[size];

        memcpy(temp, cv_img.data, size);
        HalconCpp::GenImage1(&H_img, "byte", width, height, (Hlong)(temp));

        delete[] temp;
    }
    else if (cv_img.channels() == 3)
    {
        int height = cv_img.rows, width = cv_img.cols;
        int size= height * width;
        uchar *B = new uchar[size];
        uchar *G = new uchar[size];
        uchar *R = new uchar[size];

        for (int i = 0; i < height; i++)
        {
            uchar *p = cv_img.ptr<uchar>(i);
            for (int j = 0; j < width; j++)
            {
                B[i * width + j] = p[3 * j];
                G[i * width + j] = p[3 * j + 1];
                R[i * width + j] = p[3 * j + 2];
            }
        }
        HalconCpp::GenImage3(&H_img, "byte", width, height, (Hlong)(R), (Hlong)(G), (Hlong)(B));

        delete[] R;
        delete[] G;
        delete[] B;
    }
    return H_img;
}
bool QtHalcon::qImage2HImage(QImage &from, HalconCpp::HObject &to)
{
    HImage image;
    GenEmptyObj(&image);
    if(from.isNull()) return false;
    int width = from.width(), height = from.height();
    QImage::Format format = from.format();

    if(format == QImage::Format_RGB32 ||
            format == QImage::Format_ARGB32 ||
            format == QImage::Format_ARGB32_Premultiplied)
    {
        image.GenImageInterleaved(from.bits(), "rgbx", width, height, 0,  "byte", width, height, 0, 0, 8, 0);
        CopyImage(image,&to);
        return true;
    }
    else if(format == QImage::Format_RGB888)
    {
        image.GenImageInterleaved(from.bits(), "rgb", width, height, 0,  "byte", width, height, 0, 0, 8, 0);

        CopyImage(image,&to);
        return true;
    }
    else if(format == QImage::Format_BGR30)
    {
        image.GenImageInterleaved(from.bits(), "bgr", width, height, 0,  "byte", width, height, 0, 0, 8, 0);
        CopyImage(image,&to);
        return true;
    }
    else if(format == QImage::Format_Grayscale8 || format == QImage::Format_Indexed8)
    {
        image.GenImage1("byte", width, height, from.bits());
        CopyImage(image,&to);
        return true;
    }
    return false;
}

bool QtHalcon::hImage2QImage(HalconCpp::HObject &from, QImage &to)
{
    Hlong width;
    Hlong height;
    HImage image;
    CopyImage(from,&image);
    image.GetImageSize(&width, &height);

    HTuple channels = image.CountChannels();
    HTuple type = image.GetImageType();

    if( strcmp(type[0].S(), "byte" )) // 如果不是 byte 类型，则失败
    {
        return false;
    }

    QImage::Format format;
    switch(channels[0].I())
    {
    case 1:
        format = QImage::Format_Grayscale8;
        break;
    case 3:
        format = QImage::Format_RGB32;
        break;
    default:
        return false;
    }

    if(to.width() != width || to.height() != height || to.format() != format)
    {
        to = QImage(static_cast<int>(width),static_cast<int>(height),format);
    }
    HString Type;
    if(channels[0].I() == 1)
    {
        unsigned char * pSrc = reinterpret_cast<unsigned char *>( image.GetImagePointer1(&Type, &width, &height) );
        memcpy( to.bits(), pSrc, static_cast<size_t>(width) * static_cast<size_t>(height));
        return true;
    }
    else if(channels[0].I() == 3)
    {
        uchar *R, *G, *B;
        image.GetImagePointer3(reinterpret_cast<void **>(&R),
                               reinterpret_cast<void **>(&G),
                               reinterpret_cast<void **>(&B), &Type, &width, &height);

        for(int row = 0; row < height; row ++)
        {
            QRgb* line = reinterpret_cast<QRgb*>(to.scanLine(row));
            for(int col = 0; col < width; col ++)
            {
                line[col] = qRgb(*R++, *G++, *B++);
            }
        }
        return true;
    }
    return false;

}
void QtHalcon::displayHObject(HObject showObject, HTuple window)
{
    HTuple width,height;
    GetImageSize(showObject,&width,&height);
    SetPart(window, 0, 0,height-1 ,width-1);
    DispObj(showObject,window);
}
void QtHalcon::displayImagePart(HObject showObject,HObject region , HTuple window)
{
    HTuple width,height,Row1, Column1, Row2, Column2;
    SmallestRectangle1 (region, &Row1, &Column1, &Row2, &Column2);
    SetPart(window, Row1, Column1, Row2, Column2);
    DispObj(showObject,window);
}


void QtHalcon::recognizeString (HObject ho_ImageIn, HObject ho_ROIIn, HTuple hv_MinThreshold,
                                HTuple hv_MaxThreshold, HTuple hv_StringType, HTuple hv_H_Or_V, HTuple *hv_StringOut)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region, ho_ConnectedRegions;
    HObject  ho_SortedDate;

    // Local control variables
    HTuple  hv_OCRHandle, hv_Class, hv_Confidence;
    HTuple  hv_DateScore;

    if (0 != (hv_StringType==HTuple("0-9")))
    {
        ReadOcrClassMlp("Industrial_0-9+_NoRej.omc", &hv_OCRHandle);
    }
    else if (0 != (hv_StringType==HTuple("A-Z")))
    {
        ReadOcrClassMlp("Industrial_A-Z+_NoRej.omc", &hv_OCRHandle);
    }
    else if (0 != (hv_StringType==HTuple("0-9A-Z")))
    {
        ReadOcrClassMlp("Industrial_0-9A-Z_NoRej.omc", &hv_OCRHandle);
    }
    ReduceDomain(ho_ImageIn, ho_ROIIn, &ho_ImageReduced);
    Threshold(ho_ImageReduced, &ho_Region, hv_MinThreshold, hv_MaxThreshold);
    Connection(ho_Region, &ho_ConnectedRegions);
    if (0 != (hv_H_Or_V==HTuple("H")))
    {
        SortRegion(ho_ConnectedRegions, &ho_SortedDate, "first_point", "true", "column");
    }
    else if (0 != (hv_H_Or_V==HTuple("V")))
    {
        SortRegion(ho_ConnectedRegions, &ho_SortedDate, "first_point", "true", "row");
    }
    DoOcrWordMlp(ho_SortedDate, ho_ImageReduced, hv_OCRHandle, ".*", 5, 5, &hv_Class,
                 &hv_Confidence, &(*hv_StringOut), &hv_DateScore);
    return;
}
void QtHalcon::departImage (HObject ho_Image, HObject *ho_ImageGray, HObject *ho_ImageR, HObject *ho_ImageG,
                            HObject *ho_ImageB, HObject *ho_ImageH, HObject *ho_ImageS, HObject *ho_ImageV)
{

    Rgb1ToGray(ho_Image, &(*ho_ImageGray));
    Decompose3(ho_Image, &(*ho_ImageR), &(*ho_ImageG), &(*ho_ImageB));
    TransFromRgb((*ho_ImageR), (*ho_ImageG), (*ho_ImageB), &(*ho_ImageH), &(*ho_ImageS),
                 &(*ho_ImageV), "hsv");
    return;
}
void QtHalcon::matrixFromTrans1ToTrans2 (HTuple hv_Trans1, HTuple hv_Trans2, HTuple *hv_Trans1_MoveTo_Trans2)
{
    // Local control variables
    HTuple  hv_ReturnOfObject_Scene;
    //计算仿射变换的逆变换
    HomMat2dInvert(hv_Trans1, &hv_ReturnOfObject_Scene);
    //将模版的仿射变换矩阵与场景的仿射变换矩阵合并
    HomMat2dCompose(hv_Trans2, hv_ReturnOfObject_Scene, &(*hv_Trans1_MoveTo_Trans2));
    return;
}

void QtHalcon::createModel (HObject ho_Image_M, HObject ho_ROI_M, HObject *ho_Cross_M, HTuple hv_Row_Center,
                            HTuple hv_Column_Center, HTuple hv_Num_Level, HTuple hv_Angle_Start, HTuple hv_Min_Contrast,
                            HTuple hv_Contrast, HTuple hv_Optimization, HTuple hv_AngleStep, HTuple *hv_ModelID,
                            HTuple *hv_MovementOfObject_Model)
{
    // Local iconic variables
    HObject  ho_ImageReduced_M;

    // Local control variables
    HTuple  hv_Area_ROI, hv_Row_ROI, hv_Column_ROI;

    GenCrossContourXld(&(*ho_Cross_M), hv_Row_Center, hv_Column_Center, 20, 0);
    AreaCenter(ho_ROI_M, &hv_Area_ROI, &hv_Row_ROI, &hv_Column_ROI);
    ReduceDomain(ho_Image_M, ho_ROI_M, &ho_ImageReduced_M);
    CreateShapeModel(ho_ImageReduced_M, hv_Num_Level, hv_Angle_Start.TupleRad(), ((hv_Angle_Start.TupleAbs())*2).TupleRad(),
        hv_AngleStep.TupleRad(), hv_Optimization, "use_polarity", hv_Contrast, hv_Min_Contrast,
        &(*hv_ModelID));
    if (0 != (HTuple(hv_Row_Center!=-1).TupleAnd(hv_Column_Center!=-1)))
    {
      SetShapeModelOrigin((*hv_ModelID), hv_Row_Center-hv_Row_ROI, hv_Column_Center-hv_Column_ROI);
      VectorAngleToRigid(0, 0, 0, hv_Row_Center, hv_Column_Center, 0, &(*hv_MovementOfObject_Model));
    }
    else
    {
      VectorAngleToRigid(0, 0, 0, hv_Row_ROI, hv_Column_ROI, 0, &(*hv_MovementOfObject_Model));
    }
    //
    return;
}

void QtHalcon::checkModel (HObject ho_Image_S, HObject *ho_ModelAtNewPosition_S, HObject *ho_Cross_S,
                           HTuple hv_ModelID, HTuple hv_NumToFind, HTuple hv_Angle_Start, HTuple hv_Min_Score,
                           HTuple hv_Max_Overlap, HTuple hv_Sub_Pixel, HTuple hv_Num_Levels, HTuple hv_Greediness,
                           HTuple *hv_Result, HTuple *hv_MovementOfObject_Scene, HTuple *hv_RowCheck, HTuple *hv_ColumnCheck,
                           HTuple *hv_AngleCheck, HTuple *hv_ScoreCheck)
{

    // Local iconic variables
    HObject  ho_ShapeModel, ho_ModelAtNewPosition_Temp;
    HObject  ho_Cross_Temp;
    // Local control variables
    HTuple  hv_I;

    //find_shape_model (Image_S, ModelID, rad(0), rad(360), 0.5, 1, 0.5, 'least_squares', [10,1], 1, RowCheck, ColumnCheck, AngleCheck, ScoreCheck)
    FindShapeModel(ho_Image_S, hv_ModelID, hv_Angle_Start.TupleRad(), ((hv_Angle_Start.TupleAbs())*2).TupleRad(),
                   hv_Min_Score, hv_NumToFind, hv_Max_Overlap, hv_Sub_Pixel, hv_Num_Levels, hv_Greediness,
                   &(*hv_RowCheck), &(*hv_ColumnCheck), &(*hv_AngleCheck), &(*hv_ScoreCheck));
    GenEmptyObj(&(*ho_Cross_S));
    GenEmptyObj(&(*ho_ModelAtNewPosition_S));
    if (0 != (((*hv_ScoreCheck).TupleLength())>0))
    {
        (*hv_Result) = 1;
        {
            HTuple end_val6 = ((*hv_ScoreCheck).TupleLength())-1;
            HTuple step_val6 = 1;
            for (hv_I=0; hv_I.Continue(end_val6, step_val6); hv_I += step_val6)
            {
                VectorAngleToRigid(0, 0, 0, HTuple((*hv_RowCheck)[hv_I]), HTuple((*hv_ColumnCheck)[hv_I]),
                                   HTuple((*hv_AngleCheck)[hv_I]), &(*hv_MovementOfObject_Scene));
                GetShapeModelContours(&ho_ShapeModel, hv_ModelID, 1);
                AffineTransContourXld(ho_ShapeModel, &ho_ModelAtNewPosition_Temp, (*hv_MovementOfObject_Scene));
                GenCrossContourXld(&ho_Cross_Temp, (*hv_RowCheck), (*hv_ColumnCheck), 20, (*hv_AngleCheck));
                ConcatObj((*ho_ModelAtNewPosition_S), ho_ModelAtNewPosition_Temp, &(*ho_ModelAtNewPosition_S)
                          );
                ConcatObj((*ho_Cross_S), ho_Cross_Temp, &(*ho_Cross_S));
                //stop ()
            }
        }
        (*hv_AngleCheck) = (*hv_AngleCheck).TupleDeg();

    }
    else
    {
        (*hv_Result) = 0;
        (*hv_RowCheck) = 0;
        (*hv_ColumnCheck) = 0;
        (*hv_AngleCheck) = 0;
        (*hv_ScoreCheck) = 0;
    }
    return;
}

void QtHalcon::lineFromPoints (HTuple hv_Row_M, HTuple hv_Col_M, HTuple hv_New_Col, HTuple *hv_New_Row)

{

    // Local iconic variables

    // Local control variables
    HTuple  hv_len, hv_Row_S, hv_Col_S, hv_Col_S_T_Col_S;
    HTuple  hv_Col_S_T_Row_S, hv_inv_Col_S_T_Col_S, hv_beta;
    HTuple  hv_Values;

    TupleLength(hv_Col_M, &hv_len);
    CreateMatrix(hv_len, 1, hv_Row_M, &hv_Row_S);
    CreateMatrix(hv_len, 2, 1, &hv_Col_S);
    SetValueMatrix(hv_Col_S, HTuple::TupleGenSequence(0,hv_len-1,1), HTuple(hv_len,0),
                   hv_Col_M);
    MultMatrix(hv_Col_S, hv_Col_S, "ATB", &hv_Col_S_T_Col_S);
    MultMatrix(hv_Col_S, hv_Row_S, "ATB", &hv_Col_S_T_Row_S);
    InvertMatrix(hv_Col_S_T_Col_S, "general", 0, &hv_inv_Col_S_T_Col_S);
    MultMatrix(hv_inv_Col_S_T_Col_S, hv_Col_S_T_Row_S, "AB", &hv_beta);
    GetFullMatrix(hv_beta, &hv_Values);
    (*hv_New_Row) = (HTuple(hv_Values[0])*hv_New_Col)+HTuple(hv_Values[1]);
    return;
}

/*找直线*/
void QtHalcon::findLine (HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                         HObject *ho_ROI_Show, HTuple hv_ROI_CenterLine, HTuple hv_ROI_Tolerance, HTuple hv_ROI_Space,
                         HTuple hv_MeasureThreshold, HTuple hv_MeasureSigma, HTuple hv_Black_To_White,
                         HTuple hv_First_Edge, HTuple *hv_LineParameter)

{

    // Local iconic variables
    HObject  ho_CenterLineContour, ho_MeasureContour_Model;
    HObject  ho_ModelSide1, ho_ModelSide2, ho_Arrow1, ho_Arrow2;
    HObject  ho_MeasureContour_Scene;

    // Local control variables
    HTuple  hv_Transition, hv_Select, hv_MetrologyHandle;
    HTuple  hv_Width, hv_Height, hv_Index1, hv_Row, hv_Column;
    HTuple  hv_Row1, hv_Col1, hv_Row2, hv_Col2, hv_MRow, hv_MColumn;

    //
    if (0 != (hv_Black_To_White==-1))
    {
        hv_Transition = "all";
    }
    else if (0 != (hv_Black_To_White==0))
    {
        hv_Transition = "negative";
    }
    else if (0 != (hv_Black_To_White==1))
    {
        hv_Transition = "positive";
    }
    //
    if (0 != (hv_First_Edge==-1))
    {
        hv_Select = "all";
    }
    else if (0 != (hv_First_Edge==0))
    {
        hv_Select = "last";
    }
    else if (0 != (hv_First_Edge==1))
    {
        hv_Select = "first";
    }
    //
    //创建测量句柄
    CreateMetrologyModel(&hv_MetrologyHandle);
    //设置测量图像大小
    GetImageSize(ho_Image, &hv_Width, &hv_Height);
    SetMetrologyModelImageSize(hv_MetrologyHandle, hv_Width, hv_Height);
    //添加直线测量元素
    AddMetrologyObjectLineMeasure(hv_MetrologyHandle, HTuple(hv_ROI_CenterLine[0]),
            HTuple(hv_ROI_CenterLine[1]), HTuple(hv_ROI_CenterLine[2]), HTuple(hv_ROI_CenterLine[3]),
            hv_ROI_Tolerance/2, hv_ROI_Space/2, hv_MeasureSigma, hv_MeasureThreshold, HTuple(),
            HTuple(), &hv_Index1);
    //设置测量点间隔
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", hv_ROI_Space);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_transition", hv_Transition);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_select", hv_Select);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "min_score", 0.3);
    //获取模版测量ROI
    GetMetrologyObjectModelContour(&ho_CenterLineContour, hv_MetrologyHandle, "all",
                                   1.5);
    GetMetrologyObjectMeasures(&ho_MeasureContour_Model, hv_MetrologyHandle, "all",
                               "all", &hv_Row, &hv_Column);
    //
    //生成选区及方向箭头
    //计算平行轮廓
    GenParallelContourXld(ho_CenterLineContour, &ho_ModelSide1, "regression_normal",
                          -hv_ROI_Tolerance);
    GenParallelContourXld(ho_CenterLineContour, &ho_ModelSide2, "regression_normal",
                          hv_ROI_Tolerance);
    GetContourXld(ho_ModelSide1, &hv_Row1, &hv_Col1);
    GetContourXld(ho_ModelSide2, &hv_Row2, &hv_Col2);
    gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row1[0]), HTuple(hv_Col1[0]), HTuple(hv_Row2[0]),
            HTuple(hv_Col2[0]), hv_ROI_Space, hv_ROI_Space);
    gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row1[(hv_Row1.TupleLength())-1]), HTuple(hv_Col1[(hv_Col1.TupleLength())-1]),
            HTuple(hv_Row2[(hv_Row2.TupleLength())-1]), HTuple(hv_Col2[(hv_Col2.TupleLength())-1]),
            hv_ROI_Space, hv_ROI_Space);
    //
    ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_ROI_Show));
    ConcatObj((*ho_ROI_Show), ho_ModelSide1, &(*ho_ROI_Show));
    ConcatObj((*ho_ROI_Show), ho_ModelSide2, &(*ho_ROI_Show));
    //
    //
    //运行测量算法
    ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);
    //获取测量结果
    GetMetrologyObjectResult(hv_MetrologyHandle, "all", "all", "result_type", "all_param",
                             &(*hv_LineParameter));
    //获取结果图形
    GetMetrologyObjectResultContour(&(*ho_ResultContour), hv_MetrologyHandle, "all",
                                    "all", 1.5);
    //获取场景测量ROI及边缘点坐标
    GetMetrologyObjectMeasures(&ho_MeasureContour_Scene, hv_MetrologyHandle, "all",
                               "all", &hv_MRow, &hv_MColumn);
    //根据边缘点坐标绘制十字标记
    GenCrossContourXld(&(*ho_Edge_Cross), hv_MRow, hv_MColumn, 6, HTuple(45).TupleRad());
    //
    //dev_display (ContCircle)
    //disp_message (WindowHandle, 'Angle = ' + Angle$'.5' + '°', 'window', 12, 12, 'black', 'true')
    ClearMetrologyModel(hv_MetrologyHandle);
    return;
}
void QtHalcon::findCircle (HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                           HObject *ho_ROI_Show, HTuple hv_ROI_CenterCircle, HTuple hv_ROI_Tolerance, HTuple hv_ROI_Space,
                           HTuple hv_MeasureThreshold, HTuple hv_MeasureSigma, HTuple hv_Black_To_White,
                           HTuple hv_First_Edge, HTuple *hv_CircleParameter)
{

    // Local iconic variables
    HObject  ho_CenterLineContour, ho_MeasureContour_Model;
    HObject  ho_ModelSide1, ho_ModelSide2, ho_Arrow1, ho_Arrow2;
    HObject  ho_MeasureContour_Scene;

    // Local control variables
    HTuple  hv_Transition, hv_Select, hv_MetrologyHandle;
    HTuple  hv_Width, hv_Height, hv_Index, hv_Row, hv_Column;
    HTuple  hv_Row1, hv_Col1, hv_Row2, hv_Col2, hv_MRow, hv_MColumn;

    //
    if (0 != (hv_Black_To_White==-1))
    {
        hv_Transition = "all";
    }
    else if (0 != (hv_Black_To_White==0))
    {
        hv_Transition = "negative";
    }
    else if (0 != (hv_Black_To_White==1))
    {
        hv_Transition = "positive";
    }
    //
    if (0 != (hv_First_Edge==-1))
    {
        hv_Select = "all";
    }
    else if (0 != (hv_First_Edge==0))
    {
        hv_Select = "last";
    }
    else if (0 != (hv_First_Edge==1))
    {
        hv_Select = "first";
    }
    //
    //创建测量句柄
    CreateMetrologyModel(&hv_MetrologyHandle);
    //设置测量图像大小
    GetImageSize(ho_Image, &hv_Width, &hv_Height);
    SetMetrologyModelImageSize(hv_MetrologyHandle, hv_Width, hv_Height);
    //添加圆测量元素
    AddMetrologyObjectCircleMeasure(hv_MetrologyHandle, HTuple(hv_ROI_CenterCircle[0]),
            HTuple(hv_ROI_CenterCircle[1]), HTuple(hv_ROI_CenterCircle[2]), hv_ROI_Tolerance,
            hv_ROI_Space/2, hv_MeasureSigma, hv_MeasureThreshold, HTuple(), HTuple(), &hv_Index);
    //设置测量点间隔
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_distance", hv_ROI_Space);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_transition", hv_Transition);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "measure_select", hv_Select);
    SetMetrologyObjectParam(hv_MetrologyHandle, "all", "min_score", 0.3);
    //获取模版测量ROI
    GetMetrologyObjectModelContour(&ho_CenterLineContour, hv_MetrologyHandle, "all",
                                   1.5);
    GetMetrologyObjectMeasures(&ho_MeasureContour_Model, hv_MetrologyHandle, "all",
                               "all", &hv_Row, &hv_Column);
    //
    //生成选区及方向箭头
    //计算平行轮廓
    GenParallelContourXld(ho_CenterLineContour, &ho_ModelSide1, "regression_normal",
                          -hv_ROI_Tolerance);
    GenParallelContourXld(ho_CenterLineContour, &ho_ModelSide2, "regression_normal",
                          hv_ROI_Tolerance);
    GetContourXld(ho_ModelSide1, &hv_Row1, &hv_Col1);
    GetContourXld(ho_ModelSide2, &hv_Row2, &hv_Col2);
    gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row1[0]), HTuple(hv_Col1[0]), HTuple(hv_Row2[0]),
            HTuple(hv_Col2[0]), hv_ROI_Space, hv_ROI_Space);
    gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row1[(hv_Row1.TupleLength())-1]), HTuple(hv_Col1[(hv_Col1.TupleLength())-1]),
            HTuple(hv_Row2[(hv_Row2.TupleLength())-1]), HTuple(hv_Col2[(hv_Col2.TupleLength())-1]),
            hv_ROI_Space, hv_ROI_Space);
    //
    ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_ROI_Show));
    ConcatObj((*ho_ROI_Show), ho_ModelSide1, &(*ho_ROI_Show));
    ConcatObj((*ho_ROI_Show), ho_ModelSide2, &(*ho_ROI_Show));
    //
    //
    //运行测量算法
    ApplyMetrologyModel(ho_Image, hv_MetrologyHandle);
    //获取测量结果
    GetMetrologyObjectResult(hv_MetrologyHandle, "all", "all", "result_type", "all_param",
                             &(*hv_CircleParameter));
    //获取结果图形
    GetMetrologyObjectResultContour(&(*ho_ResultContour), hv_MetrologyHandle, "all",
                                    "all", 1.5);
    //获取场景测量ROI及边缘点坐标
    GetMetrologyObjectMeasures(&ho_MeasureContour_Scene, hv_MetrologyHandle, "all",
                               "all", &hv_MRow, &hv_MColumn);
    //根据边缘点坐标绘制十字标记
    GenCrossContourXld(&(*ho_Edge_Cross), hv_MRow, hv_MColumn, 6, HTuple(45).TupleRad());

    ClearMetrologyModel(hv_MetrologyHandle);
    return;
}

void QtHalcon::gen_arrow_contour_xld (HObject *ho_Arrow, HTuple hv_Row1, HTuple hv_Column1,
                                      HTuple hv_Row2, HTuple hv_Column2, HTuple hv_HeadLength, HTuple hv_HeadWidth)

{

    // Local iconic variables
    HObject  ho_TempArrow;

    // Local control variables
    HTuple  hv_Length, hv_ZeroLengthIndices, hv_DR;
    HTuple  hv_DC, hv_HalfHeadWidth, hv_RowP1, hv_ColP1, hv_RowP2;
    HTuple  hv_ColP2, hv_Index;

    //This procedure generates arrow shaped XLD contours,
    //pointing from (Row1, Column1) to (Row2, Column2).
    //If starting and end point are identical, a contour consisting
    //of a single point is returned.
    //
    //input parameteres:
    //Row1, Column1: Coordinates of the arrows' starting points
    //Row2, Column2: Coordinates of the arrows' end points
    //HeadLength, HeadWidth: Size of the arrow heads in pixels
    //
    //output parameter:
    //Arrow: The resulting XLD contour
    //
    //The input tuples Row1, Column1, Row2, and Column2 have to be of
    //the same length.
    //HeadLength and HeadWidth either have to be of the same length as
    //Row1, Column1, Row2, and Column2 or have to be a single element.
    //If one of the above restrictions is violated, an error will occur.
    //
    //
    //Init
    GenEmptyObj(&(*ho_Arrow));
    //
    //Calculate the arrow length
    DistancePp(hv_Row1, hv_Column1, hv_Row2, hv_Column2, &hv_Length);
    //
    //Mark arrows with identical start and end point
    //(set Length to -1 to avoid division-by-zero exception)
    hv_ZeroLengthIndices = hv_Length.TupleFind(0);
    if (0 != (hv_ZeroLengthIndices!=-1))
    {
        hv_Length[hv_ZeroLengthIndices] = -1;
    }
    //
    //Calculate auxiliary variables.
    hv_DR = (1.0*(hv_Row2-hv_Row1))/hv_Length;
    hv_DC = (1.0*(hv_Column2-hv_Column1))/hv_Length;
    hv_HalfHeadWidth = hv_HeadWidth/2.0;
    //
    //Calculate end points of the arrow head.
    hv_RowP1 = (hv_Row1+((hv_Length-hv_HeadLength)*hv_DR))+(hv_HalfHeadWidth*hv_DC);
    hv_ColP1 = (hv_Column1+((hv_Length-hv_HeadLength)*hv_DC))-(hv_HalfHeadWidth*hv_DR);
    hv_RowP2 = (hv_Row1+((hv_Length-hv_HeadLength)*hv_DR))-(hv_HalfHeadWidth*hv_DC);
    hv_ColP2 = (hv_Column1+((hv_Length-hv_HeadLength)*hv_DC))+(hv_HalfHeadWidth*hv_DR);
    //
    //Finally create output XLD contour for each input point pair
    {
        HTuple end_val45 = (hv_Length.TupleLength())-1;
        HTuple step_val45 = 1;
        for (hv_Index=0; hv_Index.Continue(end_val45, step_val45); hv_Index += step_val45)
        {
            if (0 != (HTuple(hv_Length[hv_Index])==-1))
            {
                //Create_ single points for arrows with identical start and end point
                GenContourPolygonXld(&ho_TempArrow, HTuple(hv_Row1[hv_Index]), HTuple(hv_Column1[hv_Index]));
            }
            else
            {
                //Create arrow contour
                GenContourPolygonXld(&ho_TempArrow, ((((HTuple(hv_Row1[hv_Index]).TupleConcat(HTuple(hv_Row2[hv_Index]))).TupleConcat(HTuple(hv_RowP1[hv_Index]))).TupleConcat(HTuple(hv_Row2[hv_Index]))).TupleConcat(HTuple(hv_RowP2[hv_Index]))).TupleConcat(HTuple(hv_Row2[hv_Index])),
                                     ((((HTuple(hv_Column1[hv_Index]).TupleConcat(HTuple(hv_Column2[hv_Index]))).TupleConcat(HTuple(hv_ColP1[hv_Index]))).TupleConcat(HTuple(hv_Column2[hv_Index]))).TupleConcat(HTuple(hv_ColP2[hv_Index]))).TupleConcat(HTuple(hv_Column2[hv_Index])));
            }
            ConcatObj((*ho_Arrow), ho_TempArrow, &(*ho_Arrow));
        }
    }
    return;
}

void QtHalcon::saveHObject (HObject ho_Image, HTuple hv_Type, HTuple hv_Path)
{
    if (0 != (hv_Type==HTuple("jpg")))
    {
        WriteImage(ho_Image, "jpeg", 0, hv_Path);
    }
    else if (0 != (hv_Type==HTuple("bmp")))
    {
        WriteImage(ho_Image, "bmp", 0, hv_Path);
    }
    else if (0 != (hv_Type==HTuple("png")))
    {
        WriteImage(ho_Image, "png", 0, hv_Path);
    }
    else if (0 != (hv_Type==HTuple("hobj")))
    {
        WriteObject(ho_Image,  hv_Path);
    }
    else if (0 != (hv_Type==HTuple("xld")))
    {
        WriteContourXldArcInfo(ho_Image,  hv_Path);
    }
    return;
}

void QtHalcon::deleteFile (HTuple hv_Type, HTuple hv_Path)
{
    if (0 != (hv_Type==HTuple("jpg")))
    {
        HalconCpp::DeleteFile(hv_Path+".jpg");
    }
    else if (0 != (hv_Type==HTuple("bmp")))
    {
        HalconCpp::DeleteFile(hv_Path+".bmp");
    }
    else if (0 != (hv_Type==HTuple("png")))
    {
        HalconCpp::DeleteFile(hv_Path+".png");
    }
    else if (0 != (hv_Type==HTuple("hobj")))
    {
        HalconCpp::DeleteFile(hv_Path+".hobj");
    }
    else if (0 != (hv_Type==HTuple("xld")))
    {
        HalconCpp::DeleteFile(hv_Path);
    }
    return;
}

void QtHalcon::readHObject (HObject *ho_Image,HTuple hv_Type, HTuple hv_Path)
{
    if (0 != (hv_Type==HTuple("jpg")))
    {
        ReadImage(&(*ho_Image),hv_Path+".jpg");
    }
    else if (0 != (hv_Type==HTuple("bmp")))
    {
        ReadImage(&(*ho_Image),hv_Path+".bmp");
    }
    else if (0 != (hv_Type==HTuple("png")))
    {
        ReadImage(&(*ho_Image),hv_Path+".png");
    }
    else if (0 != (hv_Type==HTuple("hobj")))
    {
        ReadObject(&(*ho_Image),hv_Path+".hobj");
    }
    else if (0 != (hv_Type==HTuple("xld")))
    {
        ReadObject(&(*ho_Image),hv_Path);
    }
    return;
}



void QtHalcon::linesIntersection (HTuple hv_LineParameter_1, HTuple hv_LineParameter_2, HTuple *hv_intersection_point,
                                  HTuple *hv_degAngle)
{

    // Local iconic variables

    // Local control variables
    HTuple  hv_row, hv_col, hv_IsOverlapping, hv_Angle;

    (*hv_intersection_point) = HTuple();
    IntersectionLines(HTuple(hv_LineParameter_1[0]), HTuple(hv_LineParameter_1[1]),
            HTuple(hv_LineParameter_1[2]), HTuple(hv_LineParameter_1[3]), HTuple(hv_LineParameter_2[0]),
            HTuple(hv_LineParameter_2[1]), HTuple(hv_LineParameter_2[2]), HTuple(hv_LineParameter_2[3]),
            &hv_row, &hv_col, &hv_IsOverlapping);
    AngleLl(HTuple(hv_LineParameter_1[0]), HTuple(hv_LineParameter_1[1]), HTuple(hv_LineParameter_1[2]),
            HTuple(hv_LineParameter_1[3]), HTuple(hv_LineParameter_2[0]), HTuple(hv_LineParameter_2[1]),
            HTuple(hv_LineParameter_2[2]), HTuple(hv_LineParameter_2[3]), &hv_Angle);
    (*hv_degAngle) = hv_Angle.TupleDeg();
    (*hv_intersection_point)[0] = hv_row;
    (*hv_intersection_point)[1] = hv_col;
    //
    return;
}

void QtHalcon::distancell (HTuple hv_LineParameter_1, HTuple hv_LineParameter_2, HTuple hv_LineParameter_V,
                           HTuple *hv_intersection_point_1, HTuple *hv_intersection_point_2, HTuple *hv_Distance)
{

    // Local iconic variables
    HObject  ho_Arrow;
    HTuple angle;
    int aa;
    aa=hv_LineParameter_1.TupleLength().I();
    aa=hv_LineParameter_2.TupleLength().I();
    aa=hv_LineParameter_V.TupleLength().I();
    linesIntersection(hv_LineParameter_1, hv_LineParameter_V, &(*hv_intersection_point_1),&angle);
    linesIntersection(hv_LineParameter_2, hv_LineParameter_V, &(*hv_intersection_point_2),&angle);
    DistancePp((*hv_intersection_point_1)[0].D(), (*hv_intersection_point_1)[1].D(),
            (*hv_intersection_point_2)[0].D(), (*hv_intersection_point_2)[1].D(),
            &(*hv_Distance));
    gen_arrow_contour_xld(&ho_Arrow, HTuple((*hv_intersection_point_1)[0]), HTuple((*hv_intersection_point_1)[1]),
            HTuple((*hv_intersection_point_2)[0]), HTuple((*hv_intersection_point_2)[1]),
            5, 5);
    return;
}
//灰度拉伸
void QtHalcon::scaleImageRange (HObject ho_Image, HObject *ho_ImageScaled, HTuple hv_Min,
                                HTuple hv_Max)
{

    // Local iconic variables
    HObject  ho_SelectedChannel, ho_LowerRegion, ho_UpperRegion;

    // Local control variables
    HTuple  hv_LowerLimit, hv_UpperLimit, hv_Mult;
    HTuple  hv_Add, hv_Channels, hv_Index, hv_MinGray, hv_MaxGray;
    HTuple  hv_Range;

    if (0 != ((hv_Min.TupleLength())==2))
    {
        hv_LowerLimit = ((const HTuple&)hv_Min)[1];
        hv_Min = ((const HTuple&)hv_Min)[0];
    }
    else
    {
        hv_LowerLimit = 0.0;
    }
    if (0 != ((hv_Max.TupleLength())==2))
    {
        hv_UpperLimit = ((const HTuple&)hv_Max)[1];
        hv_Max = ((const HTuple&)hv_Max)[0];
    }
    else
    {
        hv_UpperLimit = 255.0;
    }
    //
    //Calculate scaling parameters
    hv_Mult = ((hv_UpperLimit-hv_LowerLimit).TupleReal())/(hv_Max-hv_Min);
    hv_Add = ((-hv_Mult)*hv_Min)+hv_LowerLimit;
    //
    //Scale image
    ScaleImage(ho_Image, &ho_Image, hv_Mult, hv_Add);
    //
    //Clip gray values if necessary
    //This must be done for each channel separately
    CountChannels(ho_Image, &hv_Channels);
    {
        HTuple end_val48 = hv_Channels;
        HTuple step_val48 = 1;
        for (hv_Index=1; hv_Index.Continue(end_val48, step_val48); hv_Index += step_val48)
        {
            AccessChannel(ho_Image, &ho_SelectedChannel, hv_Index);
            MinMaxGray(ho_SelectedChannel, ho_SelectedChannel, 0, &hv_MinGray, &hv_MaxGray,
                       &hv_Range);
            Threshold(ho_SelectedChannel, &ho_LowerRegion, (hv_MinGray.TupleConcat(hv_LowerLimit)).TupleMin(),
                      hv_LowerLimit);
            Threshold(ho_SelectedChannel, &ho_UpperRegion, hv_UpperLimit, (hv_UpperLimit.TupleConcat(hv_MaxGray)).TupleMax());
            PaintRegion(ho_LowerRegion, ho_SelectedChannel, &ho_SelectedChannel, hv_LowerLimit,
                        "fill");
            PaintRegion(ho_UpperRegion, ho_SelectedChannel, &ho_SelectedChannel, hv_UpperLimit,
                        "fill");
            if (0 != (hv_Index==1))
            {
                CopyObj(ho_SelectedChannel, &(*ho_ImageScaled), 1, 1);
            }
            else
            {
                AppendChannel((*ho_ImageScaled), ho_SelectedChannel, &(*ho_ImageScaled));
            }
        }
    }
    return;
}

void QtHalcon::drawNurb(HTuple window, HObject *outROI)
{
    HObject xldout;
    HTuple row,col,weight;
    DrawNurbs(outROI, window,"true","true","true","true",3,&row,&col,&weight);
    //HalconCpp::DispObj(*outROI,window);
}

void QtHalcon::drawXld(HTuple window, HObject *outROI )
{
    HObject xldout;
    HalconCpp::DrawXld(outROI,window,"true", "true", "true", "true");
    //HalconCpp::DispObj((*outROI),window);
}

void QtHalcon::drawPolygonROI(HTuple window, HObject *outROI)
{
    HObject xldout,xldout2;
    HalconCpp::DrawXld(&xldout,window,"true", "true", "true", "true");
    CloseContoursXld(xldout,&xldout2);
    GenRegionContourXld(xldout2,&(*outROI),"filled");
    HalconCpp::DispObj((*outROI),window);
}

void QtHalcon::drawRectangle2ROI(HTuple window, HObject *outROI , QString *roi_str)
{
    HTuple hv_Row1,hv_Column1,hv_Phi1,hv_Length12,hv_Length22;
    HalconCpp::DrawRectangle2(window, &hv_Row1, &hv_Column1, &hv_Phi1, &hv_Length12,
                              &hv_Length22);

    GenRectangle2(outROI,hv_Row1, hv_Column1, hv_Phi1,
                  hv_Length12, hv_Length22);
    //HalconCpp::DispObj((*outROI),window);
    *roi_str=QString("%1|%2|%3|%4|%5").arg(
                QString::number(hv_Row1.D()),QString::number(hv_Column1.D()),QString::number(hv_Phi1.D(),'f',3),QString::number(hv_Length12.D()),QString::number(hv_Length22.D()));
}
void QtHalcon::drawRectangle1ROI(HTuple window, HObject *outROI, QString *roi_str)
{
    HTuple hv_Row1,hv_Column1, hv_Row2,hv_Column2;
    HalconCpp::DrawRectangle1(window, &hv_Row1, &hv_Column1, &hv_Row2,&hv_Column2);

    GenRectangle1(outROI,hv_Row1, hv_Column1, hv_Row2,hv_Column2);

    //HalconCpp::DispObj((*outROI),window);
    *roi_str=QString("%1|%2|%3|%4").arg(
                QString::number(hv_Row1.D()),QString::number(hv_Column1.D()),QString::number(hv_Row2.D()),QString::number(hv_Column2.D()));
}

void QtHalcon::drawCircleROI(HTuple window, HObject *outROI , QString *roi_str)
{
    HTuple hv_Row1,hv_Column1,hv_Radius;
    HalconCpp::DrawCircle(window, &hv_Row1, &hv_Column1, &hv_Radius);
    GenCircle(outROI,hv_Row1, hv_Column1, hv_Radius);
    //HalconCpp::DispObj((*outROI),window);
    *roi_str=QString("%1|%2|%3").arg(
                QString::number(hv_Row1.D()),QString::number(hv_Column1.D()),QString::number(hv_Radius.D(),'f',3));
}

//彩色图转化函数：
BOOL QtHalcon::hobjectFromBuffer(HalconCpp::HObject *ho_Image, int nWidth, int nHeight,unsigned char *pbuffer,HTuple type)
{
    HImage hi_image;
    if (type=="mono")
    {
        hi_image.GenImage1("byte",  nWidth, nHeight,(void*)pbuffer);
    }
    else if(type=="bgr")
    {
        hi_image.GenImageInterleaved((void*)pbuffer, "bgr", nWidth, nHeight, 0,  "byte", nWidth, nHeight, 0, 0, 8, 0);
    }
    else if (type=="rgb")
    {
        hi_image.GenImageInterleaved((void*)pbuffer, "rgb", nWidth, nHeight, 0,  "byte", nWidth, nHeight, 0, 0, 8, 0);
    }
    else if (type=="bgrx")
    {
        hi_image.GenImageInterleaved((void*)pbuffer, "bgrx", nWidth, nHeight, 0,  "byte", nWidth, nHeight, 0, 0, 8, 0);
    }
    else if (type=="rgbx")
    {
        hi_image.GenImageInterleaved((void*)pbuffer, "rgbx", nWidth, nHeight, 0,  "byte", nWidth, nHeight, 0, 0, 8, 0);
    }
    HalconCpp::CopyImage(hi_image,ho_Image);
    return TRUE;
}
void QtHalcon::getRectangle2Corner (HTuple hv_phi, HTuple hv_length1, HTuple hv_length2, HTuple hv_rowCenter,
                                    HTuple hv_columnCenter, HTuple *hv_row, HTuple *hv_column)
{

    // Local iconic variables

    // Local control variables
    HTuple  hv_xLength1, hv_xLength2, hv_yLength1;
    HTuple  hv_yLength2, hv_mColumnUpLeft, hv_nRowUpLeft, hv_mColumnUpRight;
    HTuple  hv_nRowUpRight, hv_mColumnDownRight, hv_nRowDownRight;
    HTuple  hv_mColumnDownLeft, hv_nRowDownLeft;

    //
    if (0 != (hv_phi>=0))
    {
        hv_xLength1 = hv_length1*(hv_phi.TupleCos());
        hv_xLength2 = hv_length2*(hv_phi.TupleSin());
        hv_yLength1 = hv_length1*(hv_phi.TupleSin());
        hv_yLength2 = hv_length2*(hv_phi.TupleCos());
        //左上
        hv_mColumnUpLeft = (hv_columnCenter-hv_xLength1)-hv_xLength2;
        hv_nRowUpLeft = (hv_rowCenter+hv_yLength1)-hv_yLength2;
        //右上
        hv_mColumnUpRight = (hv_columnCenter+hv_xLength1)-hv_xLength2;
        hv_nRowUpRight = (hv_rowCenter-hv_yLength1)-hv_yLength2;
        //右下
        hv_mColumnDownRight = (hv_columnCenter+hv_xLength1)+hv_xLength2;
        hv_nRowDownRight = (hv_rowCenter-hv_yLength1)+hv_yLength2;
        //左下
        hv_mColumnDownLeft = (hv_columnCenter-hv_xLength1)+hv_xLength2;
        hv_nRowDownLeft = (hv_rowCenter+hv_yLength1)+hv_yLength2;
        //
        //
    }
    else
    {
        hv_xLength1 = hv_length2*((hv_phi.TupleSin()).TupleAbs());
        hv_xLength2 = hv_length1*(hv_phi.TupleCos());
        hv_yLength1 = hv_length2*(hv_phi.TupleCos());
        hv_yLength2 = hv_length1*((hv_phi.TupleSin()).TupleAbs());
        //左上
        hv_mColumnUpLeft = (hv_columnCenter+hv_xLength1)-hv_xLength2;
        hv_nRowUpLeft = (hv_rowCenter-hv_yLength1)-hv_yLength2;
        //右上
        hv_mColumnUpRight = (hv_columnCenter+hv_xLength1)+hv_xLength2;
        hv_nRowUpRight = (hv_rowCenter-hv_yLength1)+hv_yLength2;
        //右下
        hv_mColumnDownRight = (hv_columnCenter-hv_xLength1)+hv_xLength2;
        hv_nRowDownRight = (hv_rowCenter+hv_yLength1)+hv_yLength2;
        //
        //左下
        hv_mColumnDownLeft = (hv_columnCenter-hv_xLength1)-hv_xLength2;
        hv_nRowDownLeft = (hv_rowCenter+hv_yLength1)-hv_yLength2;
    }
    (*hv_row) = HTuple();
    (*hv_column) = HTuple();
    (*hv_row)[0] = hv_nRowUpLeft;
    (*hv_column)[0] = hv_mColumnUpLeft;
    (*hv_row)[1] = hv_nRowUpRight;
    (*hv_column)[1] = hv_mColumnUpRight;
    (*hv_row)[2] = hv_nRowDownRight;
    (*hv_column)[2] = hv_mColumnDownRight;
    (*hv_row)[3] = hv_nRowDownLeft;
    (*hv_column)[3] = hv_mColumnDownLeft;
    return;
}

//void QtHalcon::findLineInRectangle2 (HObject ho_Image, HObject ho_ROI_Rect, HObject *ho_ResultContour,
//    HObject *ho_Edge_Cross, HTuple hv_MeasureThreshold, HTuple hv_Black_To_White,
//    HTuple hv_First_Edge, HTuple hv_Rect_Phi, HTuple hv_Rect_Length1, HTuple hv_Rect_Length2,
//    HTuple hv_Rect_Row, HTuple hv_Rect_Column, HTuple *hv_Line)
//{

//  // Local iconic variables
//  HObject  ho_RegionLines, ho_ROI_Show;

//  // Local control variables
//  HTuple  hv_row, hv_column, hv_ROI_Tolerance, hv_ROI_Space;
//  HTuple  hv_MeasureSigma, hv_Line_Model;


//  getRectangle2Corner(hv_Rect_Phi, hv_Rect_Length1, hv_Rect_Length2, hv_Rect_Row,
//      hv_Rect_Column, &hv_row, &hv_column);
//  GenRegionLine(&ho_RegionLines, HTuple(hv_row[0]), HTuple(hv_column[0]), HTuple(hv_row[1]),
//      HTuple(hv_column[1]));
//  GenRegionLine(&ho_RegionLines, HTuple(hv_row[1]), HTuple(hv_column[1]), HTuple(hv_row[2]),
//      HTuple(hv_column[2]));
//  GenRegionLine(&ho_RegionLines, HTuple(hv_row[2]), HTuple(hv_column[2]), HTuple(hv_row[3]),
//      HTuple(hv_column[3]));
//  //***********找直线***************
//  hv_ROI_Tolerance = hv_Rect_Length2;
//  hv_ROI_Space = 5;
//  //平滑系数（高斯平滑系数,系数越低越精确）
//  hv_MeasureSigma = 0.5;
//  //输入的直线参数
//  hv_Line_Model.Clear();
//  hv_Line_Model.Append((HTuple(hv_row[0])+HTuple(hv_row[3]))/2);
//  hv_Line_Model.Append((HTuple(hv_column[0])+HTuple(hv_column[3]))/2);
//  hv_Line_Model.Append((HTuple(hv_row[1])+HTuple(hv_row[2]))/2);
//  hv_Line_Model.Append((HTuple(hv_column[1])+HTuple(hv_column[2]))/2);
//  findLine(ho_Image, &(*ho_ResultContour), &(*ho_Edge_Cross), &ho_ROI_Show, hv_Line_Model,
//      hv_ROI_Tolerance, hv_ROI_Space, hv_MeasureThreshold, hv_MeasureSigma, hv_Black_To_White,
//      hv_First_Edge, &(*hv_Line));

//  return;
//}

void QtHalcon::findLineInRectangle2 (HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                                     HTuple hv_MeasureThreshold, HTuple hv_Black_To_White, HTuple hv_First_Edge, HTuple hv_Rect_Row,
                                     HTuple hv_Rect_Column, HTuple hv_Rect_Phi, HTuple hv_Rect_Length1, HTuple hv_Rect_Length2,
                                     HTuple *hv_Line)
{

    // Local iconic variables
    HObject  ho_RegionLines, ho_ROI_Show;

    // Local control variables
    HTuple  hv_row, hv_column, hv_ROI_Tolerance, hv_ROI_Space;
    HTuple  hv_MeasureSigma, hv_Line_Model;

    //
    getRectangle2Corner(hv_Rect_Phi, hv_Rect_Length1, hv_Rect_Length2, hv_Rect_Row,
                        hv_Rect_Column, &hv_row, &hv_column);
    GenRegionLine(&ho_RegionLines, HTuple(hv_row[0]), HTuple(hv_column[0]), HTuple(hv_row[1]),
            HTuple(hv_column[1]));
    GenRegionLine(&ho_RegionLines, HTuple(hv_row[1]), HTuple(hv_column[1]), HTuple(hv_row[2]),
            HTuple(hv_column[2]));
    GenRegionLine(&ho_RegionLines, HTuple(hv_row[2]), HTuple(hv_column[2]), HTuple(hv_row[3]),
            HTuple(hv_column[3]));
    //***********找直线***************
    hv_ROI_Tolerance = hv_Rect_Length2*2;
    hv_ROI_Space = 5;
    //平滑系数（高斯平滑系数,系数越低越精确）
    hv_MeasureSigma = 0.5;
    //输入的直线参数
    hv_Line_Model.Clear();
    if (0 != (hv_Rect_Phi>0))
    {
        hv_Line_Model.Clear();
        hv_Line_Model.Append((HTuple(hv_row[0])+HTuple(hv_row[3]))/2);
        hv_Line_Model.Append((HTuple(hv_column[0])+HTuple(hv_column[3]))/2);
        hv_Line_Model.Append((HTuple(hv_row[1])+HTuple(hv_row[2]))/2);
        hv_Line_Model.Append((HTuple(hv_column[1])+HTuple(hv_column[2]))/2);
    }
    else
    {
        hv_Line_Model.Clear();
        hv_Line_Model.Append((HTuple(hv_row[1])+HTuple(hv_row[2]))/2);
        hv_Line_Model.Append((HTuple(hv_column[1])+HTuple(hv_column[2]))/2);
        hv_Line_Model.Append((HTuple(hv_row[0])+HTuple(hv_row[3]))/2);
        hv_Line_Model.Append((HTuple(hv_column[0])+HTuple(hv_column[3]))/2);
    }
    findLine(ho_Image, &(*ho_ResultContour), &(*ho_Edge_Cross), &ho_ROI_Show, hv_Line_Model,
             hv_ROI_Tolerance, hv_ROI_Space, hv_MeasureThreshold, hv_MeasureSigma, hv_Black_To_White,
             hv_First_Edge, &(*hv_Line));
    //
    return;
}
void QtHalcon::findLineInRectangle1 (HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                                     HTuple hv_MeasureThreshold, HTuple hv_Black_To_White, HTuple hv_First_Edge, HTuple hv_Rect,
                                     HTuple hv_isHor, HTuple *hv_Line)
{

    // Local iconic variables
    HObject  ho_Rectangle, ho_ROI_Show;

    // Local control variables
    HTuple  hv_ROI_Tolerance, hv_Line_Model, hv_ROI_Space;
    HTuple  hv_MeasureSigma;

    //
    GenRectangle1(&ho_Rectangle, HTuple(hv_Rect[0]), HTuple(hv_Rect[1]), HTuple(hv_Rect[2]),
            HTuple(hv_Rect[3]));
    if (0 != (hv_isHor==1))
    {
        hv_ROI_Tolerance = HTuple(hv_Rect[2])-HTuple(hv_Rect[0]);
        hv_Line_Model.Clear();
        hv_Line_Model.Append((HTuple(hv_Rect[0])+HTuple(hv_Rect[2]))/2);
        hv_Line_Model.Append(HTuple(hv_Rect[1]));
        hv_Line_Model.Append((HTuple(hv_Rect[0])+HTuple(hv_Rect[2]))/2);
        hv_Line_Model.Append(HTuple(hv_Rect[3]));
    }
    else
    {
        hv_ROI_Tolerance = HTuple(hv_Rect[3])-HTuple(hv_Rect[1]);
        hv_Line_Model.Clear();
        hv_Line_Model.Append(HTuple(hv_Rect[0]));
        hv_Line_Model.Append((HTuple(hv_Rect[1])+HTuple(hv_Rect[3]))/2);
        hv_Line_Model.Append(HTuple(hv_Rect[2]));
        hv_Line_Model.Append((HTuple(hv_Rect[1])+HTuple(hv_Rect[3]))/2);
    }
    //***********找直线***************
    hv_ROI_Space = 5;
    //平滑系数（高斯平滑系数,系数越低越精确）
    hv_MeasureSigma = 0.5;
    //输入的直线参数
    findLine(ho_Image, &(*ho_ResultContour), &(*ho_Edge_Cross), &ho_ROI_Show, hv_Line_Model,
             hv_ROI_Tolerance, hv_ROI_Space, hv_MeasureThreshold, hv_MeasureSigma, hv_Black_To_White,
             hv_First_Edge, &(*hv_Line));
    //
    return;
}

void QtHalcon::getMousePosGray(HTuple hv_WindowHandle,HObject image,HTuple *pos,HTuple *value )
{
    HTuple row,col,botton,grayValue;
    HalconCpp::GetMposition(hv_WindowHandle,&row,&col,&botton);
    GetGrayval(image,row,col,&grayValue);
    *value=grayValue;
    (*pos)=HTuple();
    (*pos)[0]=row;
    (*pos)[1]=col;
}
void QtHalcon::getMousePosGray(HObject image,QPoint point,QSize size,HTuple *pos,HTuple *value )
{
    HTuple rows,cols,grayValue;
    float x_ratio,y_ratio;
    GetImageSize(image,&cols,&rows);
    x_ratio=float(cols.I())/size.width();
    y_ratio=float(rows.I())/size.height();
    //GetGrayval(image,point.y()*y_ratio,point.x()*x_ratio,value);
    (*pos)=HTuple();
    (*pos)[0]=int(point.y()*y_ratio);
    (*pos)[1]=int(point.x()*x_ratio);
    GetGrayval(image,(*pos)[0],(*pos)[1],value);
}

//设置窗口显示的字体：window=窗口句柄，font="Microsoft YaHei UI""Arial"到系统的字体名字找一个，
//rowheight=行高，colWidth=字体大小
void QtHalcon::setWindowFont (HTuple window,  HTuple font,HTuple rowheight, HTuple colWidth)
{
    SetFont(window, "-"+font+"-"+rowheight+"-"+colWidth+"-");
}

//QString.toStdstring().data()=HTuple
void QtHalcon::writeWindowString (HTuple window,HTuple strWrite, HTuple color, HTuple row, HTuple col)
{
    HalconCpp::SetColor(window,color);
    SetTposition (window, row, col);
    WriteString(window,strWrite);
}
bool QtHalcon::testHObjectEmpty(HObject image)
{
    HObject empty_obj;
    HTuple is_empty;
    HalconCpp::GenEmptyObj(&empty_obj);
    HalconCpp::TestEqualObj(image,empty_obj,&is_empty);
    if (is_empty.I()==1)
    {
        return true;
    }
    return false;
}

void QtHalcon::getRoi(QString type,QString roistr,HObject *obj)
{
    GenEmptyObj(&(*obj));
    QStringList str_list=roistr.split("|");
    if(type=="Rectangle1")
    {
        if(str_list.size()!=4)
        {
            return;
        }
        HalconCpp::GenRectangle1(&(*obj),str_list[0].toFloat(), str_list[1].toFloat(), str_list[2].toFloat(),str_list[3].toFloat());
    }
    else if(type=="Rectangle2")
    {
        if(str_list.size()!=5)
        {
            return;
        }
        HalconCpp::GenRectangle2(&(*obj),str_list[0].toFloat(), str_list[1].toFloat(), str_list[2].toFloat(),str_list[3].toFloat(),str_list[4].toFloat());

    }
    else if(type=="Circle")
    {
        if(str_list.size()!=3)
        {
            return;
        }
        HalconCpp::GenCircle(&(*obj),str_list[0].toFloat(), str_list[1].toFloat(), str_list[2].toFloat());
    }
    else if(type=="Cirque")
    {
        if(str_list.size()!=4)
        {
            return;
        }
        HObject obj1,obj2;

        float max_r,min_r;
        max_r=std::max(str_list[2].toFloat(),str_list[3].toFloat());
        min_r=std::min(str_list[2].toFloat(),str_list[3].toFloat());
        HalconCpp::GenCircle(&obj1,str_list[0].toFloat(), str_list[1].toFloat(),max_r);
        HalconCpp::GenCircle(&obj2,str_list[0].toFloat(), str_list[1].toFloat(), min_r);
        Difference(obj1,obj2,&(*obj));
    }
}
QString QtHalcon::getRoiStr(QString type,HObject obj)
{
    QString roistr="";
    if(type=="Rectangle1")
    {
        HTuple row1,col1,row2,col2;
        HalconCpp::SmallestRectangle1(obj,&row1,&col1,&row2,&col2);
        roistr=QString("%1|%2|%3|%4").arg(QString::number(row1.I()),QString::number(col1.I()),
                                          QString::number(row2.I()),QString::number(col2.I()));
    }
    else if(type=="Rectangle2")
    {
        HTuple row1,col1,phi,length1,length2;
        HalconCpp::SmallestRectangle2(obj,&row1,&col1,&phi,&length1,&length2);
        roistr=QString("%1|%2|%3|%4|%5").arg(QString::number(row1.D(),'f',3),QString::number(col1.D(),'f',3),
                                             QString::number(phi.D(),'f',3),
                                             QString::number(length1.D(),'f',3),QString::number(length2.D(),'f',3));
    }
    else if(type=="Circle")
    {
        HTuple row1,col1,rad;
        HalconCpp::SmallestCircle(obj,&row1,&col1,&rad);
        roistr=QString("%1|%2|%3").arg(QString::number(row1.D(),'f',3),QString::number(col1.D(),'f',3),
                                       QString::number(rad.D(),'f',3));
    }
    return roistr;
}


void QtHalcon::paintImage (HObject ho_Image, HObject ho_ImagePart, HObject ho_ROI, HObject *ho_MultiChannelImage)
{

    // Local iconic variables
    HObject  ho_ImageR, ho_ImageG, ho_ImageB, ho_ImagePartR;
    HObject  ho_ImagePartG, ho_ImagePartB, ho_DupImageR, ho_DupImageG;
    HObject  ho_DupImageB;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2;
    HTuple  hv_PartRows, hv_PartColumns, hv_count, hv_startRs;
    HTuple  hv_startCs, hv_Rows, hv_Cols, hv_Grayval, hv_Grayval1;
    HTuple  hv_Grayval2;

    SmallestRectangle1(ho_ROI, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    //rgb分离
    Decompose3(ho_Image, &ho_ImageR, &ho_ImageG, &ho_ImageB);
    Decompose3(ho_ImagePart, &ho_ImagePartR, &ho_ImagePartG, &ho_ImagePartB);

    GetRegionPoints(ho_ImagePart, &hv_PartRows, &hv_PartColumns);
    hv_count = hv_PartRows.TupleLength();
    hv_startRs[HTuple::TupleGenSequence(0,hv_count-1,1)] = hv_Row1;
    hv_startCs[HTuple::TupleGenSequence(0,hv_count-1,1)] = hv_Column1;
    //将所有像素坐标加上偏移量
    hv_Rows = hv_startRs+hv_PartRows;
    hv_Cols = hv_startCs+hv_PartColumns;
    //r绘制
    GetGrayval(ho_ImagePartR, hv_PartRows, hv_PartColumns, &hv_Grayval);
    CopyImage(ho_ImageR, &ho_DupImageR);
    SetGrayval(ho_DupImageR, hv_Rows, hv_Cols, hv_Grayval);
    //g绘制
    GetGrayval(ho_ImagePartG, hv_PartRows, hv_PartColumns, &hv_Grayval1);
    CopyImage(ho_ImageG, &ho_DupImageG);
    SetGrayval(ho_DupImageG, hv_Rows, hv_Cols, hv_Grayval1);
    //b绘制
    GetGrayval(ho_ImagePartB, hv_PartRows, hv_PartColumns, &hv_Grayval2);
    CopyImage(ho_ImageB, &ho_DupImageB);
    SetGrayval(ho_DupImageB, hv_Rows, hv_Cols, hv_Grayval2);
    Compose3(ho_DupImageR, ho_DupImageG, ho_DupImageB, &(*ho_MultiChannelImage));
    return;
}

QString QtHalcon::htupleDoubleToString(HTuple tuple,char sig)
{
    int len=tuple.TupleLength();
    QString combineStr="";
    if(len>1)
    {
        combineStr+="[";
        for (int i=0;i<len;i++)
        {
            combineStr+=QString::number(tuple[i].D(),'f',10);
            if(i!=len-1)
                combineStr+=sig;
        }
        combineStr+="]";
    }
    else
    {
        combineStr=QString::number(tuple[0].D());
    }
    return combineStr;
}
QString QtHalcon::htupleIntToString(HTuple tuple,char sig)
{
    int len=tuple.TupleLength();
    QString combineStr="";
    if(len>1)
    {
        combineStr+="[";
        for (int i=0;i<len;i++)
        {
            combineStr+=QString::number(tuple[i].I());
            if(i!=len-1)
                combineStr+=sig;
        }
        combineStr+="]";
    }
    else
    {
        combineStr=QString::number(tuple[0].I());
    }
    return combineStr;
}
QString QtHalcon::htupleStringToString(HTuple tuple,char sig)
{
    int len=tuple.TupleLength();
    QString combineStr="";
    if(len>1)
    {
        combineStr+="[";
        for (int i=0;i<len;i++)
        {
            combineStr+="\'"+QString::number(tuple[i].I())+'\'';
            if(i!=len-1)
                combineStr+=sig;
        }
        combineStr+="]";
    }
    else
    {
        combineStr=QString::fromUtf8(tuple[0].S());
    }
    return combineStr;
}
HTuple QtHalcon::qstringToHtuple(QString str,char seg,QString type)
{
    HTuple tup;
    QString str1=str.remove(QChar('['), Qt::CaseInsensitive).remove(QChar(']'), Qt::CaseInsensitive);
    if(str1=="")
    {
        return HTuple();
    }
    QStringList strlist=str1.split(seg);
    if(type=="int")
    {
        for (int i=0;i<strlist.size();i++)
        {
            tup[i]=strlist[i].toInt();
        }
    }
    else if(type=="double")
    {
        for (int i=0;i<strlist.size();i++)
        {
            tup[i]=strlist[i].toDouble();
        }
    }
    else if(type=="string")
    {
        for (int i=0;i<strlist.size();i++)
        {
            tup[i]=strlist[i].remove('\'').toStdString().data();
        }
    }
    return tup;
}

void QtHalcon::calibrateImage (HObject *ho_MapFixed, HTuple hv_image_width, HTuple hv_image_height,
                               HTuple hv_CameraParameters, HTuple hv_CameraPose, HTuple hv_ToCalibrationPlane,
                               HTuple hv_WorldWidth, HTuple hv_ImageToWorld_Ratio_In, HTuple hv_Zoom_Ratio_In,
                               HTuple hv_X_Offset, HTuple hv_Y_Offset, HTuple hv_Z_Offset, HTuple hv_A_Offset,
                               HTuple *hv_ImageToWorld_Ratio_Out, HTuple *hv_Zoom_Ratio_Out, HTuple *hv_WorldHeight)
{

    // Local iconic variables

    // Local control variables
    HTuple  hv_Width, hv_Height, hv_ImageWidth_Out;
    HTuple  hv_ImageHeight_Out, hv_CameraPoseRotate, hv_TmpCtrl_RectificationPose;
    HTuple  hv_CamParOriginal, hv_CamParVirtualFixed;

    if (0 != hv_ToCalibrationPlane)
    {
        hv_Width = hv_image_width;
        hv_Height = hv_image_height;
        if (0 != (hv_Zoom_Ratio_In==0))
        {
            (*hv_WorldHeight) = (hv_WorldWidth*hv_Height)/hv_Width;
            hv_ImageWidth_Out = hv_WorldWidth/hv_ImageToWorld_Ratio_In;
            hv_ImageHeight_Out = (*hv_WorldHeight)/hv_ImageToWorld_Ratio_In;
            (*hv_Zoom_Ratio_Out) = hv_ImageWidth_Out/hv_Width;
            (*hv_ImageToWorld_Ratio_Out) = hv_ImageToWorld_Ratio_In;
        }
        else if (0 != (hv_ImageToWorld_Ratio_In==0))
        {
            (*hv_WorldHeight) = (hv_WorldWidth*hv_Height)/hv_Width;
            hv_ImageWidth_Out = hv_Width*hv_Zoom_Ratio_In;
            (*hv_ImageToWorld_Ratio_Out) = hv_WorldWidth/hv_ImageWidth_Out;
            hv_ImageHeight_Out = (*hv_WorldHeight)/hv_ImageToWorld_Ratio_In;
            (*hv_Zoom_Ratio_Out) = hv_Zoom_Ratio_In;
        }
        hv_CameraPoseRotate = hv_CameraPose;
        hv_CameraPoseRotate[5] = HTuple(hv_CameraPose[5])-hv_A_Offset;

        //Calibration 01: Adjust origin so the plate is roughly centered
        SetOriginPose(hv_CameraPoseRotate, -((0.5*hv_WorldWidth)+hv_X_Offset), -((0.5*(*hv_WorldHeight))+hv_Y_Offset),
                      -hv_Z_Offset, &hv_TmpCtrl_RectificationPose);

        //Calibration 01: Generate the rectification map
        GenImageToWorldPlaneMap(&(*ho_MapFixed), hv_CameraParameters, hv_TmpCtrl_RectificationPose,
                                hv_Width, hv_Height, hv_ImageWidth_Out, hv_ImageHeight_Out, (*hv_ImageToWorld_Ratio_Out),
                                "bilinear");
        //Calibration 01: Now, images can be rectified using the rectification map

        //标定板标定到世界
    }
    else
    {
        hv_CamParOriginal = hv_CameraParameters;
        hv_CamParVirtualFixed = hv_CamParOriginal;
        hv_CamParVirtualFixed[1] = 0;
        GenRadialDistortionMap(&(*ho_MapFixed), hv_CamParOriginal, hv_CamParVirtualFixed,
                               "bilinear");
    }
    return;
}
void QtHalcon::calibrateImage (HObject *ho_MapFixed, HTuple hv_image_width, HTuple hv_image_height,
                               HTuple hv_world_width, HTuple hv_world_height, HTuple hv_CameraParameters, HTuple hv_CameraPose,
                               HTuple hv_ImageToWorld_Ratio_In, HTuple hv_X_Offset, HTuple hv_Y_Offset)
{

    // Local iconic variables

    // Local control variables
    HTuple  hv_CameraPoseRotate, hv_TmpCtrl_RectificationPose;

    hv_CameraPoseRotate = hv_CameraPose;
    //Calibration 01: Adjust origin so the plate is roughly centered
    SetOriginPose(hv_CameraPoseRotate, -((0.5*hv_world_width)+hv_X_Offset), -((0.5*hv_world_height)+hv_Y_Offset),
                  0, &hv_TmpCtrl_RectificationPose);
    //Calibration 01: Generate the rectification map
    GenImageToWorldPlaneMap(&(*ho_MapFixed), hv_CameraParameters, hv_TmpCtrl_RectificationPose,
                            hv_image_width, hv_image_height, hv_image_width, hv_image_height, hv_ImageToWorld_Ratio_In,
                            "bilinear");
    //Calibration 01: Now, images can be rectified using the rectification map

    return;
}
void QtHalcon::transImagePointToWorldPoint (HTuple hv_Row, HTuple hv_Column, HTuple hv_ImageToWorld_Ratio,
                                            HTuple hv_WorldWidth, HTuple hv_WorldHeight, HTuple hv_X_Offset, HTuple hv_Y_Offset,
                                            HTuple hv_Unit, HTuple *hv_x, HTuple *hv_y)
{

    // Local iconic variables

    if (0 != (hv_Unit==HTuple("mm")))
    {
        (*hv_x) = ((hv_Column*hv_ImageToWorld_Ratio)-((0.5*hv_WorldWidth)+hv_X_Offset))*1000;
        (*hv_y) = ((hv_Row*hv_ImageToWorld_Ratio)-((0.5*hv_WorldHeight)+hv_Y_Offset))*1000;
    }
    else if (0 != (hv_Unit==HTuple("m")))
    {
        (*hv_x) = (hv_Column*hv_ImageToWorld_Ratio)-((0.5*hv_WorldWidth)+hv_X_Offset);
        (*hv_y) = (hv_Row*hv_ImageToWorld_Ratio)-((0.5*hv_WorldHeight)+hv_Y_Offset);
    }
    return;
}
void QtHalcon::transWorldPointToImagePoint (HTuple hv_x, HTuple hv_y, HTuple hv_ImageToWorld_Ratio,
                                            HTuple hv_WorldHeight, HTuple hv_WorldWidth, HTuple hv_X_Offset, HTuple hv_Y_Offset,
                                            HTuple hv_Unit, HTuple *hv_Row, HTuple *hv_Column)
{

    // Local iconic variables

    if (0 != (hv_Unit==HTuple("mm")))
    {
        (*hv_Row) = ((hv_y/1000)+((0.5*hv_WorldHeight)+hv_Y_Offset))/hv_ImageToWorld_Ratio;
        (*hv_Column) = ((hv_x/1000)+((0.5*hv_WorldWidth)+hv_X_Offset))/hv_ImageToWorld_Ratio;
    }
    else if (0 != (hv_Unit==HTuple("m")))
    {
        (*hv_Row) = (hv_y+((0.5*hv_WorldHeight)+hv_Y_Offset))/hv_ImageToWorld_Ratio;
        (*hv_Column) = (hv_x+((0.5*hv_WorldWidth)+hv_X_Offset))/hv_ImageToWorld_Ratio;
    }
    return;
}
void QtHalcon::getPointOffset (HTuple hv_row1, HTuple hv_col1, HTuple hv_ang1, HTuple hv_row2,
                               HTuple hv_col2, HTuple hv_ang2, HTuple hv_center_row, HTuple hv_center_col, HTuple hv_ImageToWorld_Ratio,
                               HTuple hv_WorldHeight, HTuple hv_WorldWidth, HTuple hv_X_Offset, HTuple hv_Y_Offset,
                               HTuple hv_Unit, HTuple *hv_Row, HTuple *hv_Column, HTuple *hv_RowTrans3, HTuple *hv_ColTrans3,
                               HTuple *hv_x, HTuple *hv_y)
{

    // Local iconic variables

    // Local control variables
    HTuple  hv_HomMat2D;

    //gen_rectangle2 (rect_0, row1, col1, rad(ang1), 743.829, 123.824)
    //gen_cross_contour_xld (Cross, row1, col1, 500, rad(ang1))
    //gen_rectangle2 (rect_1, row2, col2, rad(ang2), 743.829, 123.824)
    //gen_cross_contour_xld (Cross2, row2, col2, 500, rad(ang2))
    VectorAngleToRigid(hv_row1, hv_col1, hv_ang1, hv_row2, hv_col2, hv_ang2.TupleRad(),
                       &hv_HomMat2D);

    if (0 != (HTuple(hv_center_row==0).TupleAnd(hv_center_col==0)))
    {
        transWorldPointToImagePoint(0.0, 0.0, hv_ImageToWorld_Ratio, hv_WorldHeight,
                                    hv_WorldWidth, hv_X_Offset, hv_Y_Offset, hv_Unit, &(*hv_Row), &(*hv_Column));
    }
    else
    {
        (*hv_Row) = hv_center_row;
        (*hv_Column) = hv_center_col;
    }
    //gen_cross_contour_xld (Cross1, Row, Column, 3000, 0)
    AffineTransPixel(hv_HomMat2D, (*hv_Row), (*hv_Column), &(*hv_RowTrans3), &(*hv_ColTrans3));
    //gen_cross_contour_xld (Cross4, RowTrans3, ColTrans3, 3000, rad(10))
    transImagePointToWorldPoint((*hv_RowTrans3), (*hv_ColTrans3), hv_ImageToWorld_Ratio,
                                hv_WorldWidth, hv_WorldHeight, hv_X_Offset, hv_Y_Offset, hv_Unit, &(*hv_x),
                                &(*hv_y));

    return;
}

