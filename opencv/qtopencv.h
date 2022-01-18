#ifndef QTOPENCV_H
#define QTOPENCV_H
#include"opencv2/opencv.hpp"
#include"opencv2/highgui.hpp"
#include <opencv2/imgproc/types_c.h>
#include "opencv2/imgproc/imgproc_c.h"
#include<QImage>
#include <QWidget>
#include "Windows.h"
#include <QLabel>
#include"line2Dup.h"
#include<QFile>
//#include "MatSource.h"
class QtOpencv
{
public:
    QtOpencv();
    static QImage cvMat2QImage(const cv::Mat& mat);
    static cv::Mat qImageToMat(const QImage &image);

    static int RGB2BGR(unsigned char *pRgbData, unsigned int nWidth, unsigned int nHeight);
    static cv::Mat unsignchar2Mat(int height, int width, int channel, unsigned char *pData);
    static void showImage(cv::Mat image, QWidget *widget);
    static void showImage(cv::Mat image, QLabel *lbl);

    static void GetStringSize(HDC hDC, const char* str, int* w, int* h);
    static void putTextZH(cv::Mat &dst, const char* str, cv::Point org, cv::Scalar color, int fontSize,
                          const char *fn = "Arial", bool italic = false, bool underline = false);

    static void drawRectangle1(cv::Mat srcdst, QString rectstr, const cv::Scalar &color, int thick);
    static void calculateRectInMat(cv::Mat image, QPoint firstPoint, QPoint lastPoint, QSize controlsize, QString *rectstr);
    static void calculateRect2InMat(cv::Mat src, cv::RotatedRect rotaterect, QSize controlsize, QString *rectstr);

    static void getMousePosGray(cv::Mat image,QPoint point,QSize controlsize,QPoint *pos,QVector<int> *value );
    static bool departRGBImage(cv::Mat src, cv::Mat *r_image, cv::Mat *g_image, cv::Mat *b_image);
    static bool getRotateRectRoi(cv::Mat src,cv::RotatedRect rotaterect,QSize controlsize,bool isStraighten,bool iscut,cv::Mat *dst);
    static bool getRotateRectRoi(cv::Mat src,QString strrotaterect,bool isStraighten,bool iscut,cv::Mat *dst);
    static bool getRotateRectRoi(cv::Mat src, cv::RotatedRect rote, bool isStraighten, bool iscut, cv::Mat *dst);
    static bool drawRotateRect(cv::Mat src, cv::RotatedRect rotaterect, QSize controlsize,cv::Scalar color, cv::Mat *dst);
    static bool drawRotateRect(cv::Mat src, cv::RotatedRect rotaterect, cv::Scalar color,cv::Mat *dst);
    static bool drawRotateRect(cv::Mat src, QString strrotaterect,cv::Scalar color, cv::Mat *dst);
    static cv::Mat connectMat(QList<cv::Mat> matlist,int direction);
    static cv::Mat selectShape(cv::Mat src,cv::ConnectedComponentsTypes select_type,int min,int max);
    static void rotateImage(cv::Mat &src, cv::Mat &dst, float angle);
    static void erisionRectangle1(cv::Mat src,cv::Mat dst,int width,int height);
    static void dilateRectangle1(cv::Mat src,cv::Mat dst,int width,int height);
    static void erisionCircle(cv::Mat src,cv::Mat dst,int width,int height);
    static void dilateCircle(cv::Mat src,cv::Mat dst,int width,int height);
    static void openingRectangle1(cv::Mat src,cv::Mat dst,int width,int height);
    static void closingRectangle1(cv::Mat src,cv::Mat dst,int width,int height);
    static void openingCircle(cv::Mat src,cv::Mat dst,int width,int height);
    static void closingCircle(cv::Mat src,cv::Mat dst,int width,int height);
    static cv::Mat paintImageToRect(cv::Mat src,cv::Mat paintImage,cv::Rect rect);
    static cv::Mat scaleImage(cv::Mat src, cv::Size size, int channels);


    struct MatchResult
    {
        std::vector<line2Dup::Feature> features;//检测点
        float model_width;
        float model_height;
        float score;
        float x;
        float y;
        float a;
    };

    static int trainTemplate(cv::Mat img, std::string filePath, float angle_middle, float angle_range, float scale_start, float scale_step);
    static bool CompareScore(MatchResult a, MatchResult b);
    static int findContourModel(cv::Mat src, std::string filePath, int find_number, float min_score, float max_overlap, float max_cover, std::vector<MatchResult> *matchResult);
    cv::Mat imgTranslate(cv::Mat &matSrc, int xOffset, int yOffset, bool bScale);

};

#endif // QTOPENCV_H
