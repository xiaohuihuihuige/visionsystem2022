#ifndef QTHALCON_H
#define QTHALCON_H

#include "HalconCpp.h"
#include<QImage>
#include"opencv2/opencv.hpp"
//#include"opencv2/highgui.hpp"

using namespace  HalconCpp;
class QtHalcon
{
public:
    QtHalcon();
    struct ModelPar
    {
        //查找模板参数
        HTuple angle_Start;
        HTuple min_Score;
        HTuple numToFind;
        HTuple max_Overlap;
        HTuple sub_Pixel;
        HTuple num_Levels;
        HTuple greediness;
        HTuple row_M;//模板位置
        HTuple col_M;
        HTuple ang_M;
        HTuple modelID;//模板
        HTuple movementOfObject_Model_M;//模板变换矩阵
    };

    //Hobject转cv::Mat
    static cv::Mat HObjectToMat(HalconCpp::HObject H_img);
    //cv::Mat转Hobject
    static HalconCpp::HObject MatToHObject(cv::Mat &cv_img);
    //QImage转HImage
    static bool qImage2HImage(QImage &from, HalconCpp::HObject &to);
    //Himae转QImage
    static bool hImage2QImage(HalconCpp::HObject &from, QImage &to);
    //数据流转Hobject
    static BOOL hobjectFromBuffer(HalconCpp::HObject *ho_Image, int nWidth, int nHeight,
                                  unsigned char *pbuffer,HTuple type);

    //显示图像
    static void displayHObject(HObject showObject,HTuple window);
    //显示部分图像
    static void displayImagePart(HObject showObject,HObject region , HTuple window);
    //识别字符
    static void recognizeString(HObject ho_ImageIn, HObject ho_ROIIn, HTuple hv_MinThreshold,
                                HTuple hv_MaxThreshold, HTuple hv_StringType,
                                HTuple hv_H_Or_V, HTuple *hv_StringOut);
    //把RGB图像分成不同通道的图像
    static void departImage(HObject ho_Image, HObject *ho_ImageGray, HObject *ho_ImageR,
                            HObject *ho_ImageG, HObject *ho_ImageB, HObject *ho_ImageH,
                            HObject *ho_ImageS, HObject *ho_ImageV);
    //把两个矩阵变换合成一个
    static void matrixFromTrans1ToTrans2(HTuple hv_Trans1, HTuple hv_Trans2, HTuple *hv_Trans1_MoveTo_Trans2);
    //查找模板
    static void checkModel(HObject ho_Image_S, HObject *ho_ModelAtNewPosition_S, HObject *ho_Cross_S,
                           HTuple hv_ModelID, HTuple hv_NumToFind, HTuple hv_Angle_Start,
                           HTuple hv_Min_Score, HTuple hv_Max_Overlap, HTuple hv_Sub_Pixel,
                           HTuple hv_Num_Levels, HTuple hv_Greediness, HTuple *hv_Result,
                           HTuple *hv_MovementOfObject_Scene, HTuple *hv_RowCheck, HTuple *hv_ColumnCheck,
                           HTuple *hv_AngleCheck, HTuple *hv_ScoreCheck);
    //创建模板
    static void createModel(HObject ho_Image_M, HObject ho_ROI_M, HObject *ho_Cross_M, HTuple hv_Row_Center,
                            HTuple hv_Column_Center, HTuple hv_Num_Level, HTuple hv_Angle_Start,
                            HTuple hv_Min_Contrast, HTuple hv_Contrast, HTuple hv_Optimization,
                            HTuple hv_AngleStep, HTuple *hv_ModelID, HTuple *hv_MovementOfObject_Model);
    //多个点拟合一条直线
    static void lineFromPoints(HTuple hv_Row_M, HTuple hv_Col_M, HTuple hv_New_Col, HTuple *hv_New_Row);
    //找直线
    static void findLine(HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                         HObject *ho_ROI_Show, HTuple hv_ROI_CenterLine, HTuple hv_ROI_Tolerance,
                         HTuple hv_ROI_Space, HTuple hv_MeasureThreshold, HTuple hv_MeasureSigma,
                         HTuple hv_Black_To_White, HTuple hv_First_Edge, HTuple *hv_LineParameter);
    //找圆
    static void findCircle(HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                           HObject *ho_ROI_Show, HTuple hv_ROI_CenterCircle, HTuple hv_ROI_Tolerance,
                           HTuple hv_ROI_Space, HTuple hv_MeasureThreshold, HTuple hv_MeasureSigma,
                           HTuple hv_Black_To_White, HTuple hv_First_Edge, HTuple *hv_CircleParameter);
    //绘制箭头
    static void gen_arrow_contour_xld(HObject *ho_Arrow, HTuple hv_Row1, HTuple hv_Column1,
                                      HTuple hv_Row2, HTuple hv_Column2, HTuple hv_HeadLength, HTuple hv_HeadWidth);

    /**
     * @brief 保存图像
     * @param ho_Image 输入图像
     * @param hv_Type   保存图像类型"jpg","bmp","png","hobj"
     * @param hv_Path  保存路径
     */
    static void saveHObject(HObject ho_Image, HTuple hv_Type, HTuple hv_Path);
    //删除文件
    static void deleteFile(HTuple hv_Type, HTuple hv_Path);
    //读取Hobject
    static void readHObject(HObject *ho_Image, HTuple hv_Type, HTuple hv_Path);

    //两条线交点
    static void linesIntersection (HTuple hv_LineParameter_1, HTuple hv_LineParameter_2, HTuple *hv_intersection_point,
                                   HTuple *hv_degAngle);
    //两条平行线的距离，第三条线为垂直的线
    static void distancell(HTuple hv_LineParameter_1, HTuple hv_LineParameter_2, HTuple hv_LineParameter_V,
                           HTuple *hv_intersection_point_1, HTuple *hv_intersection_point_2, HTuple *hv_Distance);


    //绘制多边形选区
    static void drawPolygonROI(HTuple window, HObject *outROI);
    //绘制正矩形选区
    static void drawRectangle1ROI(HTuple window, HObject *outROI, QString *roi_str);
    //绘制旋转矩形选区
    static void drawRectangle2ROI(HTuple window, HObject *outROI, QString *roi_str);
    //找旋转矩形的四个顶点，顺序：左上，右上，右下，左下
    static void getRectangle2Corner(HTuple hv_phi, HTuple hv_length1, HTuple hv_length2, HTuple hv_rowCenter,
                                    HTuple hv_columnCenter, HTuple *hv_row, HTuple *hv_column);
    //找旋转矩形中间的线
    static void findLineInRectangle2(HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                                     HTuple hv_MeasureThreshold, HTuple hv_Black_To_White, HTuple hv_First_Edge,
                                     HTuple hv_Rect_Row, HTuple hv_Rect_Column, HTuple hv_Rect_Phi, HTuple hv_Rect_Length1,
                                     HTuple hv_Rect_Length2, HTuple *hv_Line);

    static void findLineInRectangle1 (HObject ho_Image, HObject *ho_ResultContour, HObject *ho_Edge_Cross,
                                      HTuple hv_MeasureThreshold, HTuple hv_Black_To_White, HTuple hv_First_Edge, HTuple hv_Rect,
                                      HTuple hv_isHor, HTuple *hv_Line);
    //灰度拉伸
    static void scaleImageRange(HObject ho_Image, HObject *ho_ImageScaled, HTuple hv_Min, HTuple hv_Max);
    //获取鼠标点击的位置和灰度
    static void getMousePosGray(HTuple hv_WindowHandle, HObject image, HTuple *pos, HTuple *value);
    static void getMousePosGray(HObject image, QPoint point, QSize size,HTuple *pos, HTuple *value);
    //设置窗口显示的字体类型
    static void setWindowFont (HTuple window,  HTuple font,HTuple rowheight, HTuple colWidth);
    //显示文字到窗口
    static void writeWindowString(HTuple window,HTuple strWrite, HTuple color, HTuple row, HTuple col);
    //绘制圆形选区
    static void drawCircleROI(HTuple window, HObject *outROI, QString *roi_str);
    //绘制样条曲线
    static void drawNurb(HTuple window, HObject *outROI);
    //绘制轮廓
    static void drawXld(HTuple window, HObject *outROI);

    static bool testHObjectEmpty(HObject image);
    static void getRoi(QString type, QString roistr, HObject *obj);
    static void paintImage(HObject ho_Image, HObject ho_ImagePart, HObject ho_ROI, HObject *ho_MultiChannelImage);
    static QString getRoiStr(QString type, HObject obj);
    static QString htupleDoubleToString(HTuple tuple, char sig=',');
    static QString htupleIntToString(HTuple tuple, char sig=',');
    static QString htupleStringToString(HTuple tuple, char sig=',');
    static HTuple qstringToHtuple(QString str, char seg, QString type);
    static  void calibrateImage(HObject *ho_MapFixed, HTuple hv_image_width, HTuple hv_image_height,
                                HTuple hv_CameraParameters, HTuple hv_CameraPose, HTuple hv_ToCalibrationPlane,
                                HTuple hv_WorldWidth, HTuple hv_ImageToWorld_Ratio_In, HTuple hv_Zoom_Ratio_In,
                                HTuple hv_X_Offset, HTuple hv_Y_Offset, HTuple hv_Z_Offset, HTuple hv_A_Offset,
                                HTuple *hv_ImageToWorld_Ratio_Out, HTuple *hv_Zoom_Ratio_Out, HTuple *hv_WorldHeight);

    static  void calibrateImage(HObject *ho_MapFixed, HTuple hv_image_width, HTuple hv_image_height, HTuple hv_world_width,
                                HTuple hv_world_height, HTuple hv_CameraParameters, HTuple hv_CameraPose,
                                HTuple hv_ImageToWorld_Ratio_In, HTuple hv_X_Offset, HTuple hv_Y_Offset);
    static  void transImagePointToWorldPoint(HTuple hv_Row, HTuple hv_Column, HTuple hv_ImageToWorld_Ratio,
                                             HTuple hv_WorldWidth, HTuple hv_WorldHeight, HTuple hv_X_Offset,
                                             HTuple hv_Y_Offset, HTuple hv_Unit, HTuple *hv_x, HTuple *hv_y);
    static void transWorldPointToImagePoint(HTuple hv_x, HTuple hv_y, HTuple hv_ImageToWorld_Ratio, HTuple hv_WorldHeight, HTuple hv_WorldWidth,
                                            HTuple hv_X_Offset, HTuple hv_Y_Offset, HTuple hv_Unit, HTuple *hv_Row, HTuple *hv_Column);
    static void getPointOffset (HTuple hv_row1, HTuple hv_col1, HTuple hv_ang1, HTuple hv_row2,
                                HTuple hv_col2, HTuple hv_ang2, HTuple hv_center_row, HTuple hv_center_col, HTuple hv_ImageToWorld_Ratio,
                                HTuple hv_WorldHeight, HTuple hv_WorldWidth, HTuple hv_X_Offset, HTuple hv_Y_Offset,
                                HTuple hv_Unit, HTuple *hv_Row, HTuple *hv_Column, HTuple *hv_RowTrans3, HTuple *hv_ColTrans3,
                                HTuple *hv_x, HTuple *hv_y);
};

#endif // QTHALCON_H
