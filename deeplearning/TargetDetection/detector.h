#ifndef DETECTOR_H
#define DETECTOR_H
#include<QObject>
#undef slots
#include <torch/torch.h>
#define slots Q_SLOTS
#include <torch/script.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <memory>
#include "utils.h"



class Detector :public QObject
{
    Q_OBJECT
public:
    /***
     * @brief constructor
     * @param model_path - path of the TorchScript weight file
     * @param device_type - inference with CPU/GPU
     */
    explicit Detector(QObject *parent= nullptr,int num=0,const std::string& model_path="",const std::string& namesFile="");
    ~Detector();

private:
    /***
     * @brief Padded resize
     * @param src - input image
     * @param dst - output image
     * @param out_size - desired output size
     * @return padding information - pad width, pad height and zoom scale
     */
    static std::vector<float> LetterboxImage(const cv::Mat& src, cv::Mat& dst, const cv::Size& out_size = cv::Size(640, 640));

    static inline torch::Tensor GetBoundingBoxIoU(const torch::Tensor& box1, const torch::Tensor& box2);

    /***
     * @brief Performs Non-Maximum Suppression (NMS) on inference results
     * @param detections - inference results from the network, example [1, 25200, 85], 85 = 4(xywh) + 1(obj conf) + 80(class score)
     * @param conf_thres - object confidence(objectness) threshold
     * @param iou_thres - IoU threshold for NMS algorithm
     * @return detections with shape: nx7 (batch_index, x1, y1, x2, y2, score, classification)
     */
    static torch::Tensor PostProcessing(const torch::Tensor& detections, float conf_thres = 0.4, float iou_thres = 0.6);

    /***
     * @brief Rescale coordinates to original input image
     * @param data - detection result after inference and nms
     * @param pad_w - width padding
     * @param pad_h - height padding
     * @param scale - zoom scale
     * @param img_shape - original input image shape
     * @return rescaled detections
     */
    static std::vector<Detection> ScaleCoordinates(const at::TensorAccessor<float, 2>& data,
                                                   float pad_w, float pad_h, float scale, const cv::Size& img_shape);


    /***
     * @brief inference module
     * @param img - input image
     * @param conf_threshold - confidence threshold
     * @param iou_threshold - IoU threshold for nms
     * @return detection result - bounding box, score, class index
     */

    void Run(cv::Mat *image_out, QStringList *classname,QList<cv::Rect> *out_rect);

    void setCheckPar(cv::Mat src, int imageSize, float conf_threshold, float iou_threshold);
    //获取RGB值
    float get_color(int c, int x, int max);
    //绘制检测结果
    cv::Mat drawBox(cv::Mat &img, const std::vector<Detection> &detections, const std::vector<std::string> &class_names,
                 bool label = true,float conf_threshold=0.5);
    //读取种类名称
    std::vector<std::string> loadNames(const std::string &path);

    torch::jit::script::Module module_;
    torch::Device device_;
    bool half_;
    float m_colors[6][3] = { {1,0,1}, {0,0,1},{0,1,1},{0,1,0},{1,1,0},{1,0,0} };
    std::vector<std::string> class_names;
    QStringList out_name;
    QList<cv::Rect> out_rect;


    int m_num;
    cv::Mat blackImgae;//空白图像
    cv::Mat origin_image;//源视图像
    cv::Mat check_img;//检测图像
    cv::Rect final_cut_rect;
    float m_img_size;//图像大小
    float m_conf_threshold,m_iou_threshold;
    bool m_enable;

signals:
    void signal_result(int num,cv::Mat img_result,QStringList name,QList<cv::Rect> out_rect);
public Q_SLOTS:
    void slot_run();
    void slot_setCheckPar(int num,cv::Mat src,int imageSize,float conf_threshold, float iou_threshold );
};
#endif
