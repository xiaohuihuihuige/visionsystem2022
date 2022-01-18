#ifndef CRNNOPENVINO_H
#define CRNNOPENVINO_H
#include <QObject>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <inference_engine.hpp>
#include <chrono>
#include <opencv2/dnn/dnn.hpp>
#include <cmath>

class CrnnOpenvino : public QObject
{
    Q_OBJECT
public:
    explicit CrnnOpenvino(QObject *parent= nullptr,int num=0,const std::string& device_="CPU",const std::string& model_path="",const std::string& namesFile="");
    ~CrnnOpenvino();
    InferenceEngine::ExecutableNetwork _network;//推理网络
    InferenceEngine::InputsDataMap _inputinfo;//输入节点
    InferenceEngine::OutputsDataMap _outputinfo;//输出节点
    InferenceEngine::DataPtr output_data;
    std::string _inputname ,_outputname;
    //参数区
    std::vector<std::string> detect_calsses;
    cv::Mat check_img;
    int g_width,g_height;
    int m_num;
    bool m_enable;

    bool init(const std::string &onnxfile,const std::string& device);
    QString Run();
    bool process_frame( cv::Mat& src);
    std::vector<std::string> loadNames(const std::string &path);

    void setCheckPar(cv::Mat src, int width, int height);
signals:
    void signal_result(int num,QString name);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int width,int height );
};

#endif // CRNNOPENVINO_H
