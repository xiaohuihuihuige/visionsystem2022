#ifndef CLASSFIEDOPENVINO_H
#define CLASSFIEDOPENVINO_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <inference_engine.hpp>
#include <chrono>
#include <opencv2/dnn/dnn.hpp>
#include <cmath>

class classfiedopenvino : public QObject
{
    Q_OBJECT
public:
    explicit classfiedopenvino(QObject *parent= nullptr,int num=0,const std::string& device_="CPU",const std::string& model_path="",const std::string& namesFile="");
    ~classfiedopenvino();
    InferenceEngine::ExecutableNetwork _network;//推理网络
    InferenceEngine::InputsDataMap _inputinfo;//输入节点
    InferenceEngine::OutputsDataMap _outputinfo;//输出节点
    std::string _inputname ,_outputname;
    //参数区
    std::vector<std::string> detect_calsses;
    cv::Mat check_img;
    float image_size;
    QString class_detected;
    int m_num;
    bool m_enable;

    bool init(const std::string &onnxfile,const std::string& device);
    bool process_frame( cv::Mat& src);
    std::vector<std::string> loadNames(const std::string &path);
    QString Run();
    void setCheckPar(cv::Mat src, int imageSize);
signals:
    void signal_result(int num,QString name);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int imageSize );
};

#endif // CLASSFIEDOPENVINO_H

