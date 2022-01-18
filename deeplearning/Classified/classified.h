#ifndef CLASSIFIED_H
#define CLASSIFIED_H
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




class Classified : public QObject
{
    Q_OBJECT
public:
    explicit Classified(QObject *parent= nullptr,int num=0,const std::string& model_path="",const std::string& namesFile="");
    ~Classified();
    std::vector<std::string> class_names;
    torch::jit::script::Module module_;
    torch::Device device_;
    bool half_;
    int m_num;
    cv::Mat check_img;
    int m_img_size;
    bool m_enable;


    void loadModel(const std::string &path);
    std::vector<std::string> loadNames(const std::string &path);

    cv::Mat pilResize(cv::Mat &img, int size);
    cv::Mat pilCropCenter(cv::Mat &img, int output_size);
    cv::Mat setNorm(cv::Mat &img,int imageSize);
    cv::Mat setMean(cv::Mat &image_resized_float);
    QString run();
    void setCheckPar(cv::Mat src, int imageSize);

signals:
    void signal_result(int num,QString name);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int imageSize);

};

#endif // CLASSIFIED_H
