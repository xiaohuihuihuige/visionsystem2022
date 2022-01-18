//#ifndef YOLACT_H
//#define YOLACT_H
//#include<QObject>
//#undef slots
//#include <torch/torch.h>
//#define slots Q_SLOTS
//#include <torch/script.h>
//#include <opencv2/opencv.hpp>
//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <memory>

//class Yolact : public QObject
//{
//    Q_OBJECT
//public:
//    explicit Yolact(QObject *parent= nullptr,int num=0,const std::string& model_path="",const std::string& namesFile="");

//    struct Object
//    {
//        cv::Rect_<float> rect;
//        int label;
//        float prob;
//        std::vector<float> maskdata;
//        cv::Mat mask;
//    };

//    //读取种类名称
//    std::vector<std::string> loadNames(const std::string &path);

//    torch::jit::script::Module module_;
//    torch::Device device_;
//    bool half_;
//    int m_num;
//    cv::Mat check_img;
//    int m_img_size;
//    float m_conf_threshold,m_iou_threshold;
//    bool m_enable;
//    std::vector<std::string> class_names;
//    QStringList class_detected;
//    static inline float intersection_area(const Object &a, const Object &b);

//    static void qsort_descent_inplace(std::vector<Object> &objects, int left, int right);
//    static void qsort_descent_inplace(std::vector<Object> &objects);
//    static void nms_sorted_bboxes(const std::vector<Object> &objects, std::vector<int> &picked, float nms_threshold);
//    int detect_yolact(const cv::Mat &bgr, std::vector<Object> &objects);
//    static void draw_objects(const cv::Mat &bgr, const std::vector<Object> &objects);
//    cv::Mat pilCropCenter(cv::Mat &img, int output_size);
//    cv::Mat setNorm(cv::Mat &img, int imageSize, int crop_size);
//    cv::Mat setMean(cv::Mat &image_resized_float);
//signals:
//    void signal_result(int num,cv::Mat img_result,QStringList name);
//public Q_SLOTS:
//    void slot_run();
//    void slot_setCheckpar(int num,cv::Mat src,int imageSize,float conf_threshold, float iou_threshold );
//};

//#endif // YOLACT_H
