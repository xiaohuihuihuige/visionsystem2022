#ifndef YOLOV5OPENVINO_H
#define YOLOV5OPENVINO_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <inference_engine.hpp>
#include <chrono>
#include <opencv2/dnn/dnn.hpp>
#include <cmath>

class yolov5openvino : public QObject
{
    Q_OBJECT

public:
    explicit yolov5openvino(QObject *parent= nullptr,int num=0,const std::string& device_="CPU",const std::string& model_path="",const std::string& namesFile="");
    ~yolov5openvino();

    typedef struct {
        float conf;
        int idx;
        std::string name;
        cv::Rect rect;
    } DetectObject;
    std::vector<float> anchor = {
        10, 13,  16, 30,  33, 23,	// 8倍降采样下的anchor比率		-	小目标
        30, 61,  62, 45,  59,119,	// 16倍降采样下的anchor比率	-	中目标
        116,90, 156,198, 373,326,	// 32倍降采样下的anchor比率	-	大目标
    };
    std::vector<DetectObject> detect_obj;//检测结果
    InferenceEngine::ExecutableNetwork _network;//推理网络
    InferenceEngine::InputsDataMap _inputinfo;//输入节点
    InferenceEngine::OutputsDataMap _outputinfo;//输出节点
    //参数区
    double _conf_thr; //置信度阈值,计算方法是框置信度乘以物品种类置信度
    double _nms_thr;  //nms最小重叠面积阈值
    std::vector<std::string> detect_calsses;//训练好的类别
    cv::Mat blackImgae;//空白图像
    cv::Mat origin_image;//源视图像
    cv::Mat check_img;//检测图像
    float m_img_size;//图像大小
    float scale_x,scale_y;//放大倍率
    int move_x,move_y;//移动数
    int m_num;//线程标记
    bool m_enable;//线程检测是否执行



    float m_colors[6][3] = { {1,0,1}, {0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,0,0} };
    int get_anchor_index(int scale_w, int scale_h);
    float get_stride(int scale_w, int scale_h);
    float sigmoid_function(float a);
    bool init(const std::string &onnxfile,const std::string& device);
    void Run(cv::Mat *image_out,QStringList *class_out,QList<cv::Rect> *out_rect);
    bool process_frame( cv::Mat& src,const float &conf_thr,const float &nms_thr);
    float get_color(int c, int x, int max);
    std::vector<std::string> loadNames(const std::string &path);

    void setCheckPar(cv::Mat src, int imageSize, float conf_threshold, float iou_threshold);
signals:
    void signal_result(int num,cv::Mat img_result,QStringList name,QList<cv::Rect> out_rect);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int imageSize,float conf_threshold, float iou_threshold );
};

#endif // YOLOV5OPENVINO_H
