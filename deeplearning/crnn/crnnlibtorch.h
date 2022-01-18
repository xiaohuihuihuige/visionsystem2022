#ifndef CRNNLIBTORCH_H
#define CRNNLIBTORCH_H
#include<QObject>
#undef slots
#include <torch/torch.h>
#define slots Q_SLOTS
#include <torch/script.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <memory>
#include<QString>

class CrnnLibtorch : public QObject
{
    Q_OBJECT
public:
    explicit CrnnLibtorch(QObject *parent= nullptr,int num=0,const std::string& model_path="",const std::string& namesFile="");
    ~CrnnLibtorch();
    std::vector<std::string> class_names;
    torch::jit::script::Module module_;
    torch::Device device_;
    bool half_;
    int m_num;
    cv::Mat check_img;
    int g_width,g_height;
    bool m_enable;

    void loadModel(const std::string &path);
    std::vector<std::string> loadNames(const std::string &path);
    QString run();
    torch::Tensor  setMean(cv::Mat  &input,bool isbath=false);    
    void setCheckPar(cv::Mat src, int width, int height);
signals:
    void signal_result(int num,QString name);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int width,int height);
};

#endif // CRNNLIBTORCH_H
