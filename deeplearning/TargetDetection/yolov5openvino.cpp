#include "yolov5openvino.h"

yolov5openvino::yolov5openvino(QObject *parent,int num,const std::string& device_,const std::string& model_path,const std::string& namesFile) : QObject(parent)
{
    m_img_size=640.0;
    cv::Mat aa(cv::Size(m_img_size, m_img_size),CV_8UC(3), cv::Scalar(0,0,0));
    blackImgae=aa;                            //OpenVINO模型xml文件路径
    detect_calsses=loadNames(namesFile);
    m_num=num;
    init(model_path,device_);
}

yolov5openvino::~yolov5openvino()
{
    //_network.~ExecutableNetwork();
}
std::vector<std::string> yolov5openvino::loadNames(const std::string& path)
{
    // load class names
    std::vector<std::string> class_names;
    std::ifstream infile(path);
    if (infile.is_open())
    {
        std::string line;
        while (getline (infile,line))
        {
            class_names.emplace_back(line);
        }
        infile.close();
    }
    return class_names;
}

bool yolov5openvino::init(const std::string& onnxfile,const std::string& device)
{
    // Loading model to the device
    // 创建ie插件，查询支持硬件设备
    InferenceEngine::Core ie;
    /*std::vector<std::string> availableDevices = ie.GetAvailableDevices();
    for (int i = 0; i < availableDevices.size(); ++i)
    {
        std::cout << "supported device name: " << availableDevices[i] << std::endl;
    }*/
    // 加载检测模型
    auto network = ie.ReadNetwork(onnxfile);
    //auto network = ie.ReadNetwork("E:\\WorkSpace\\Qt\\opencvtest\\opencvtest\\yolov5s.onnx");
    // 设置输入格式
    _inputinfo=InferenceEngine::InputsDataMap(network.getInputsInfo());
    for (auto &item : _inputinfo)
    {
        auto input_data = item.second;
        input_data->setPrecision(InferenceEngine::Precision::FP32);
        input_data->setLayout(InferenceEngine::Layout::NCHW);
        input_data->getPreProcess().setResizeAlgorithm(InferenceEngine::RESIZE_BILINEAR);
        input_data->getPreProcess().setColorFormat(InferenceEngine::ColorFormat::RGB);
    }
    // 设置输出格式
    _outputinfo=InferenceEngine::OutputsDataMap(network.getOutputsInfo());
    for (auto &item : _outputinfo)
    {
        auto output_data = item.second;
        output_data->setPrecision(InferenceEngine::Precision::FP32);
        // 显示输出维度
        auto out_shape = output_data->getTensorDesc().getDims();
    }
    _network = ie.LoadNetwork(network, device);
    return true;
}

void yolov5openvino::slot_setCheckPar(int num, cv::Mat src, int imageSize, float conf_threshold, float iou_threshold)
{
    if(m_num!=num)return;
    setCheckPar(src, imageSize,conf_threshold, iou_threshold);
    m_enable=true;
}
void yolov5openvino::setCheckPar(cv::Mat src, int imageSize, float conf_threshold, float iou_threshold)
{
    origin_image=src.clone();
    cv::Mat op_mat;
    op_mat=src.clone();
    int channals=op_mat.channels();
    if(channals==1)
    {
        cvtColor(origin_image, origin_image, cv::COLOR_GRAY2BGR);
        cvtColor(op_mat, op_mat, cv::COLOR_GRAY2BGR);
    }
    scale_x=op_mat.cols*1.0/m_img_size;
    scale_y=op_mat.rows*1.0/m_img_size;
    int new_rows,new_cols;
    scale_x>=scale_y?scale_y=scale_x:scale_x=scale_y;
    new_cols=op_mat.cols/scale_x;
    new_rows=op_mat.rows/scale_y;
    move_x=(m_img_size-new_cols)/2;
    move_y=(m_img_size-new_rows)/2;
    cv::resize(op_mat,op_mat,cv::Size(new_cols,new_rows));
    check_img=blackImgae.clone();
    op_mat.copyTo(check_img(cv::Rect(move_x,move_y,new_cols,new_rows)));
    _conf_thr=conf_threshold;
    _nms_thr=iou_threshold;
}

void yolov5openvino::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    cv::Mat image_out;
    QList<cv::Rect> out_rect;
    QStringList class_out;
    Run(&image_out,&class_out,&out_rect);
    emit signal_result(m_num,origin_image,class_out,out_rect);
}

void yolov5openvino::Run(cv::Mat *image_out,QStringList *class_out,QList<cv::Rect> *out_rect)
{
    cv::Mat check_image=check_img.clone();
    process_frame(check_image,_conf_thr,_nms_thr);
    for(int i=0;i<detect_obj.size();++i)
    {
        (*class_out)<<QString::fromStdString(detect_obj[i].name);
        *out_rect<<detect_obj[i].rect;

        int xmin = detect_obj[i].rect.x;
        int ymin = detect_obj[i].rect.y;
        int width = detect_obj[i].rect.width;
        int height = detect_obj[i].rect.height;

        cv::Rect rect(xmin, ymin, width, height);//左上坐标（x,y）和矩形的长(x)宽(y)
        int class_idx = detect_obj[i].idx;
        int offset = class_idx*123457 % 80;
        float red = 255*get_color(2,offset,80);
        float green = 255*get_color(1,offset,80);
        float blue = 255*get_color(0,offset,80);
        cv::rectangle(origin_image, rect, cv::Scalar(blue, green, red),3, cv::LINE_8,0);
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << detect_obj[i].conf;
        std::string s = detect_obj[i].name + " " + ss.str();
        auto font_face = cv::FONT_HERSHEY_DUPLEX;
        auto font_scale = 1.0;
        int thickness = 1;
        cv::putText(origin_image, s, cv::Point(rect.tl().x, rect.tl().y+rect.height/20),
                    font_face , font_scale, cv::Scalar(red,blue, green), thickness);
    }
    *image_out=origin_image;
}

//处理图像获取结果,输入图像要为RGB通道图像
bool yolov5openvino::process_frame( cv::Mat& src,const float &conf_thr,const float &nms_thr)
{
    detect_obj.clear();
    cv::Mat blob_image=src.clone();
    cvtColor(blob_image, blob_image, cv::COLOR_BGR2RGB);
    // Create infer request
    InferenceEngine::InferRequest infer_request = _network.CreateInferRequest();
    //float scale_x = src.cols / 640.0;
    //float scale_y = src.rows / 640.0;
    // 设置输入图像数据并实现推理预测
    /* Iterating over all input blobs */
    for (auto &item : _inputinfo)
    {
        auto input_name = item.first;
        /* Getting input blob */
        auto input = infer_request.GetBlob(input_name);
        size_t num_channels = input->getTensorDesc().getDims()[1];
        size_t h = input->getTensorDesc().getDims()[2];
        size_t w = input->getTensorDesc().getDims()[3];
        size_t image_size = h * w;
        cv::resize(blob_image, blob_image, cv::Size(w, h));
        //cv::imwrite("E:\\a00.jpg",blob_image);
        // NCHW
        //推理图像的设置
        float *data = static_cast<float*>(input->buffer());
        //#pragma omp parallel for//并行计算
        for (int row = 0; row < h; row++)
        {
            for (int col = 0; col < w; col++)
            {
                for (int ch = 0; ch < num_channels; ch++)
                {
                    data[image_size * ch + row * w + col] = float(blob_image.at<cv::Vec3b>(row, col)[ch] / 255.0);
                }
            }
        }
    }
    // 执行预测
    infer_request.Infer();
    // 处理解析输出结果
    std::vector<cv::Rect>	boxes;
    std::vector<int>		classIds;
    std::vector<float>	confidences;
    // 输出解析
    for (auto &item : _outputinfo)
    {
        auto output_name = item.first;
        //cout << "output_name: " << output_name.c_str() << endl;
        auto output = infer_request.GetBlob(output_name);
        const float *output_blob = static_cast<InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type*>(output->buffer());
        const InferenceEngine::SizeVector outputDims = output->getTensorDesc().getDims();
        const int out_n = outputDims[0];//图片数，通常情况下是1
        const int out_c = outputDims[1];//通道数
        const int side_h = outputDims[2];//高
        const int side_w = outputDims[3];//宽
        const int side_data = outputDims[4];//训练的类别数+5
        float stride = get_stride(side_h, side_h);//获取采样倍数
        int anchor_index = get_anchor_index(side_h, side_h);
        int side_square = side_h * side_w;
        int side_data_square = side_square * side_data;
        int side_data_w = side_w * side_data;
        //#pragma omp parallel for
        for (int i = 0; i < side_square; ++i)
        {
            for (int c = 0; c < out_c; c++)
            {
                // 3个anchor
                int row = i / side_h;
                int col = i % side_h;
                int object_index = c * side_data_square + row * side_data_w + col * side_data;
                // 阈值过滤
                float conf= sigmoid_function(output_blob[object_index + 4]);
                if (conf < 0.25)
                {
                    continue;
                }
                // 解析cx,cy,width,height
                float x = (sigmoid_function(output_blob[object_index]) * 2 - 0.5 + col) * stride;
                float y = (sigmoid_function(output_blob[object_index + 1]) * 2 - 0.5 + row) * stride;
                float w = pow(sigmoid_function(output_blob[object_index + 2]) * 2, 2) * anchor[anchor_index + c * 2];
                float h = pow(sigmoid_function(output_blob[object_index + 3]) * 2, 2) * anchor[anchor_index + c * 2 + 1];
                float max_prob = -1;
                int class_index = -1;

                // 解析类别
                for (int d = 5; d < side_data; ++d)
                {
                    float prob = sigmoid_function(output_blob[object_index + d]);
                    if (prob > max_prob)
                    {
                        max_prob = prob;
                        class_index = d - 5;
                    }
                }

                // 转换为top-left, bottom-right坐标
                /*int x1 = cv::saturate_cast<int>((x - w / 2) * scale_x);	// top left x
                int y1 = cv::saturate_cast<int>((y - h / 2) * scale_y);	// top left y
                int x2 = cv::saturate_cast<int>((x + w / 2) * scale_x);	// bottom right x
                int y2 = cv::saturate_cast<int>((y + h / 2) * scale_y);	// bottom right y*/
                int x1 = cv::saturate_cast<int>(((x - w / 2)-move_x) * scale_x);	// top left x
                int y1 = cv::saturate_cast<int>(((y - h / 2)-move_y) * scale_y);	// top left y
                int x2 = cv::saturate_cast<int>(((x + w / 2)-move_x) * scale_x);	// bottom right x
                int y2 = cv::saturate_cast<int>(((y + h / 2)-move_y) * scale_y);	// bottom right y
                // 解析输出
                classIds.push_back(class_index);
                confidences.push_back((float)conf);
                boxes.push_back(cv::Rect(x1, y1, x2 - x1, y2 - y1));
            }
        }
    }

    std::vector<int> indices(100000);
    indices.clear();
    cv::dnn::NMSBoxes(boxes, confidences, conf_thr, nms_thr, indices);
//#pragma omp parallel for
    for (int i = 0; i < indices.size(); ++i)
    {
        int idx = indices[i];
        cv::Rect box = boxes[idx];
        detect_obj.push_back(DetectObject{confidences[idx],classIds[idx],detect_calsses[classIds[idx]],box});
    }

    return true;
}

float yolov5openvino::get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
    ratio -= i;
    float r = (1-ratio) * m_colors[i][c] + ratio*m_colors[j][c];
    return r;
}
int yolov5openvino::get_anchor_index(int scale_w, int scale_h)
{
    if (scale_w == 20)
    {	// 32倍降采样
        return 12;
    }
    if (scale_w == 40)
    {	// 16倍降采样
        return 6;
    }
    if (scale_w == 80)
    {	// 8倍降采样
        return 0;
    }
    return -1;
}
float yolov5openvino::get_stride(int scale_w, int scale_h)
{
    if (scale_w == 20)
    {	// 32倍降采样640/20
        return 32.0;
    }
    if (scale_w == 40)
    {	// 16倍降采样
        return 16.0;
    }
    if (scale_w == 80)
    {	// 8倍降采样
        return 8.0;
    }
    return -1;
}

float yolov5openvino::sigmoid_function(float a)
{
    return 1.f / (1.f + exp(-a));
}
