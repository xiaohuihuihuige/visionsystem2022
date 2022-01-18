
#include "detector.h"


Detector::Detector(QObject *parent,int num,const std::string& model_path,const std::string& namesFile):
    QObject(parent),
    device_(torch::cuda::is_available()?torch::kCUDA:torch::kCPU),
    m_enable(false)
{
    try
    {
        //std::cout<<model_path;
        // Deserialize the ScriptModule from a file using torch::jit::load().
        std::string path="";
        path=model_path;
        //path=model_path+"_cpu.pt";
        //device_=torch::kCPU;
        half_=false;
        module_ = torch::jit::load(path.data());
    }
    catch (const c10::Error& e)
    {
        std::cerr << "Error loading the model!\n";
        std::exit(EXIT_FAILURE);
    }
    m_num=num;
    module_.to(device_);
    module_.eval();
    class_names=loadNames(namesFile);
    m_img_size=640.0;
    cv::Mat aa(cv::Size(m_img_size, m_img_size),CV_8UC(3), cv::Scalar(0,0,0));
    blackImgae=aa;
}

Detector::~Detector()
{

}

std::vector<float> Detector::LetterboxImage(const cv::Mat& src, cv::Mat& dst, const cv::Size& out_size)
{

    auto in_h = static_cast<float>(src.rows);
    auto in_w = static_cast<float>(src.cols);
    float out_h = out_size.height;
    float out_w = out_size.width;

    float scale = std::min(out_w / in_w, out_h / in_h);

    int mid_h = static_cast<int>(in_h * scale);
    int mid_w = static_cast<int>(in_w * scale);

    cv::resize(src, dst, cv::Size(mid_w, mid_h));
    int top = (static_cast<int>(out_h) - mid_h) / 2;
    int down = (static_cast<int>(out_h)- mid_h + 1) / 2;
    int left = (static_cast<int>(out_w)- mid_w) / 2;
    int right = (static_cast<int>(out_w)- mid_w + 1) / 2;

    cv::copyMakeBorder(dst, dst, top, down, left, right, cv::BORDER_CONSTANT, cv::Scalar(0, 0, 0));

    std::vector<float> pad_info{static_cast<float>(left), static_cast<float>(top), scale};
    return pad_info;
}


// returns the IoU of bounding boxes
torch::Tensor Detector::GetBoundingBoxIoU(const torch::Tensor& box1, const torch::Tensor& box2) {
    // get the coordinates of bounding boxes
    const torch::Tensor& b1_x1 = box1.select(1, 0);
    const torch::Tensor& b1_y1 = box1.select(1, 1);
    const torch::Tensor& b1_x2 = box1.select(1, 2);
    const torch::Tensor& b1_y2 = box1.select(1, 3);

    const torch::Tensor& b2_x1 = box2.select(1, 0);
    const torch::Tensor& b2_y1 = box2.select(1, 1);
    const torch::Tensor& b2_x2 = box2.select(1, 2);
    const torch::Tensor& b2_y2 = box2.select(1, 3);

    // get the coordinates of the intersection rectangle
    torch::Tensor inter_rect_x1 =  torch::max(b1_x1, b2_x1);
    torch::Tensor inter_rect_y1 =  torch::max(b1_y1, b2_y1);
    torch::Tensor inter_rect_x2 =  torch::min(b1_x2, b2_x2);
    torch::Tensor inter_rect_y2 =  torch::min(b1_y2, b2_y2);

    // calculate intersection area
    torch::Tensor inter_area = torch::max(inter_rect_x2 - inter_rect_x1 + 1,torch::zeros(inter_rect_x2.sizes()))
            * torch::max(inter_rect_y2 - inter_rect_y1 + 1, torch::zeros(inter_rect_x2.sizes()));

    // calculate union area
    torch::Tensor b1_area = (b1_x2 - b1_x1 + 1)*(b1_y2 - b1_y1 + 1);
    torch::Tensor b2_area = (b2_x2 - b2_x1 + 1)*(b2_y2 - b2_y1 + 1);

    // calculate IoU
    torch::Tensor iou = inter_area / (b1_area + b2_area - inter_area);

    return iou;
}


torch::Tensor Detector::PostProcessing(const torch::Tensor& detections, float conf_thres, float iou_thres) {
    constexpr int item_attr_size = 5;
    int batch_size = detections.size(0);
    auto num_classes = detections.size(2) - item_attr_size;  // 80 for coco dataset

    // get candidates which object confidence > threshold
    auto conf_mask = detections.select(2, 4).ge(conf_thres).unsqueeze(2);

    // compute overall score = obj_conf * cls_conf, similar to x[:, 5:] *= x[:, 4:5]
    detections.slice(2, item_attr_size, item_attr_size + num_classes) *=
            detections.select(2, 4).unsqueeze(2);

    // convert bounding box format from (center x, center y, width, height) to (x1, y1, x2, y2)
    torch::Tensor box = torch::zeros(detections.sizes(), detections.options());
    box.select(2, Det::tl_x) = detections.select(2, 0) - detections.select(2, 2).div(2);
    box.select(2, Det::tl_y) = detections.select(2, 1) - detections.select(2, 3).div(2);
    box.select(2, Det::br_x) = detections.select(2, 0) + detections.select(2, 2).div(2);
    box.select(2, Det::br_y) = detections.select(2, 1) + detections.select(2, 3).div(2);
    detections.slice(2, 0, 4) = box.slice(2, 0, 4);

    bool is_initialized = false;
    torch::Tensor output = torch::zeros({0, 7});

    // iterating all images in the batch
    //#pragma omp parallel for num_threads(8)
    for (int batch_i = 0; batch_i < batch_size; batch_i++)
    {
        auto det = torch::masked_select(detections[batch_i], conf_mask[batch_i]).view({-1, num_classes + item_attr_size});

        // if none remain then process next image
        if (det.size(0) == 0) {
            continue;
        }

        // get the max classes score at each result (e.g. elements 5-84)
        std::tuple<torch::Tensor, torch::Tensor> max_classes = torch::max(det.slice(1, item_attr_size, item_attr_size + num_classes), 1);

        // class score
        auto max_conf_score = std::get<0>(max_classes);
        // index
        auto max_conf_index = std::get<1>(max_classes);

        max_conf_score = max_conf_score.to(torch::kFloat32).unsqueeze(1);
        max_conf_index = max_conf_index.to(torch::kFloat32).unsqueeze(1);

        // shape: n * 6, top-left x/y (0,1), bottom-right x/y (2,3), score(4), class index(5)
        det = torch::cat({det.slice(1, 0, 4), max_conf_score, max_conf_index}, 1);

        // get unique classes
        std::vector<torch::Tensor> img_classes;

        auto len = det.size(0);
        for (int i = 0; i < len; i++)
        {
            bool found = false;
            for (const auto& cls : img_classes) {
                auto ret = (det[i][Det::class_idx] == cls);
                if (torch::nonzero(ret).size(0) > 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                img_classes.emplace_back(det[i][Det::class_idx]);
            }
        }

        // iterating all unique classes
        for (const auto& cls : img_classes)
        {
            auto cls_mask = det * (det.select(1, Det::class_idx) == cls).to(torch::kFloat32).unsqueeze(1);
            auto class_mask_index =  torch::nonzero(cls_mask.select(1, Det::score)).squeeze();
            auto bbox_by_class = det.index_select(0, class_mask_index).view({-1, 6});

            // sort by confidence (descending)
            std::tuple<torch::Tensor,torch::Tensor> sort_ret = torch::sort(bbox_by_class.select(1, 4), -1, true);
            auto conf_sort_index = std::get<1>(sort_ret);

            bbox_by_class = bbox_by_class.index_select(0, conf_sort_index.squeeze()).cpu();
            int num_by_class = bbox_by_class.size(0);

            // Non-Maximum Suppression (NMS)
            for(int i = 0; i < num_by_class - 1; i++)
            {
                auto iou = GetBoundingBoxIoU(bbox_by_class[i].unsqueeze(0), bbox_by_class.slice(0, i + 1, num_by_class));
                auto iou_mask = (iou < iou_thres).to(torch::kFloat32).unsqueeze(1);

                bbox_by_class.slice(0, i + 1, num_by_class) *= iou_mask;

                // remove from list
                auto non_zero_index = torch::nonzero(bbox_by_class.select(1, 4)).squeeze();
                bbox_by_class = bbox_by_class.index_select(0, non_zero_index).view({-1, 6});
                // update remain number of detections
                num_by_class = bbox_by_class.size(0);
            }

            torch::Tensor batch_index = torch::zeros({bbox_by_class.size(0), 1}).fill_(batch_i);

            if (!is_initialized) {
                output = torch::cat({batch_index, bbox_by_class}, 1);
                is_initialized = true;
            }
            else {
                auto out = torch::cat({batch_index, bbox_by_class}, 1);
                output = torch::cat({output,out}, 0);
            }
        }
    }

    return output;
}


std::vector<Detection> Detector::ScaleCoordinates(const at::TensorAccessor<float, 2>& data,
                                                  float pad_w, float pad_h, float scale, const cv::Size& img_shape) {
    auto clip = [](float n, float lower, float upper) {
        return std::max(lower, std::min(n, upper));
    };

    std::vector<Detection> detections;
    //#pragma omp parallel for num_threads(8)
    for (int i = 0; i < data.size(0) ; i++)
    {
        Detection detection;
        float x1 = (data[i][Det::tl_x] - pad_w)/scale;  // x padding
        float y1 = (data[i][Det::tl_y] - pad_h)/scale;  // y padding
        float x2 = (data[i][Det::br_x] - pad_w)/scale;  // x padding
        float y2 = (data[i][Det::br_y] - pad_h)/scale;  // y padding

        x1 = clip(x1, 0, img_shape.width);
        y1 = clip(y1, 0, img_shape.height);
        x2 = clip(x2, 0, img_shape.width);
        y2 = clip(y2, 0, img_shape.height);

        detection.bbox = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2));
        detection.score = data[i][Det::score];
        detection.class_idx = data[i][Det::class_idx];
        detections.emplace_back(detection);
    }
    return detections;
}

float Detector::get_color(int c, int x, int max)
{
    float ratio = ((float)x/max)*5;
    int i = floor(ratio);
    int j = ceil(ratio);
    ratio -= i;
    float r = (1-ratio) * m_colors[i][c] + ratio*m_colors[j][c];
    return r;
}


cv::Mat Detector::drawBox(cv::Mat& img,const std::vector<Detection>& detections,
                          const std::vector<std::string>& class_names,bool label,float conf_threshold)
{
    out_name.clear();
    out_rect.clear();
    for (const auto& detection : detections)
    {
        const auto& box = detection.bbox;
        float score = detection.score;
        int class_idx = detection.class_idx;
        int offset = class_idx*123457 % 80;
        float red = 255*get_color(2,offset,80);
        float green = 255*get_color(1,offset,80);
        float blue = 255*get_color(0,offset,80);

        if(score<conf_threshold)
        {
            continue;
        }
        cv::rectangle(img, box, cv::Scalar(blue, green, red), 2);

        if (label)
        {
            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << score;
            std::string s = class_names[class_idx] + " " + ss.str();

            out_name.push_back(QString::fromStdString(class_names[class_idx]));
            out_rect.push_back(box);
            auto font_face = cv::FONT_HERSHEY_DUPLEX;
            auto font_scale = 1.0;
            int thickness = 1;
            int baseline=0;
            auto s_size = cv::getTextSize(s, font_face, font_scale, thickness, &baseline);
            cv::rectangle(img,
                          cv::Point(box.tl().x, box.tl().y - s_size.height - 5),
                          cv::Point(box.tl().x + s_size.width, box.tl().y),
                          cv::Scalar(red, green, blue), -1);
            cv::putText(img, s, cv::Point(box.tl().x, box.tl().y - 5),
                        font_face , font_scale, cv::Scalar(red,blue, green), thickness);
        }
    }
    return img;
}
std::vector<std::string> Detector::loadNames(const std::string& path)
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

void Detector::slot_setCheckPar(int num,cv::Mat src,int imageSize,float conf_threshold, float iou_threshold)
{
    if(m_num!=num)return;
    setCheckPar( src, imageSize, conf_threshold,  iou_threshold);
    m_enable=true;
}

void Detector::setCheckPar(cv::Mat src,int imageSize,float conf_threshold, float iou_threshold)
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
    float scale_x,scale_y;
    int move_x,move_y;
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
    final_cut_rect=cv::Rect(move_x,move_y,new_cols,new_rows);
    op_mat.copyTo(check_img(final_cut_rect));
    m_conf_threshold=conf_threshold;
    m_iou_threshold=iou_threshold;
}
void Detector::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    cv::Mat image_out;
    QStringList class_out;
    QList<cv::Rect> rect_out;
    Run(&image_out,&class_out,&rect_out);
    emit signal_result(m_num,image_out,class_out,rect_out);
}

void Detector::Run(cv::Mat *image_out, QStringList *classname,QList<cv::Rect> *rect_out)
{
    /*double time_start;
    double time_end;
    QString m_runInfo;
    time_start = double(clock());*/
    torch::NoGradGuard no_grad;
    cv::Mat img_input = check_img.clone();
    cv::Mat draw_img=check_img.clone();
    cv::cvtColor(img_input, img_input, cv::COLOR_BGR2RGB);  // BGR -> RGB
    std::vector<float> pad_info = LetterboxImage(img_input, img_input, cv::Size(m_img_size,m_img_size));
    const float pad_w = pad_info[0];
    const float pad_h = pad_info[1];
    const float scale = pad_info[2];
    img_input.convertTo(img_input, CV_32FC3, 1.0f / 255.0f);  // normalization 1/255
    auto tensor_img = torch::from_blob(img_input.data, {1, img_input.rows, img_input.cols, img_input.channels()}).to(device_);
    tensor_img = tensor_img.permute({0, 3, 1, 2}).contiguous();  // BHWC -> BCHW (Batch, Channel, Height, Width)
    if (half_)
    {
        tensor_img = tensor_img.to(torch::kHalf);
    }
    //time_end = double(clock());
    //m_runInfo=QString::fromLocal8Bit("耗时:%2ms").arg(QString::number(time_end-time_start));
    //std::cout<<m_runInfo.toLocal8Bit().data();
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(tensor_img);
    /*** Inference ***/
    //time_start = double(clock());
    // inference
    torch::jit::IValue output = module_.forward(inputs).toTuple()->elements()[0].toTensor();
    //time_end = double(clock());
    //m_runInfo=QString::fromLocal8Bit("耗时:%2ms").arg(QString::number(time_end-time_start));
    //std::cout<<m_runInfo.toLocal8Bit().data();
    //time_start = double(clock());
    /*** Post-process ***/
    auto detections = output.toTensor();
    // result: n * 7
    // batch index(0), top-left x/y (1,2), bottom-right x/y (3,4), score(5), class id(6)
    auto result = PostProcessing(detections, m_conf_threshold, m_iou_threshold);
    // Note - only the first image in the batch will be used for demo
    auto idx_mask = result * (result.select(1, 0) == 0).to(torch::kFloat32).unsqueeze(1);
    auto idx_mask_index =  torch::nonzero(idx_mask.select(1, 1)).squeeze();
    const auto& data = result.index_select(0, idx_mask_index).slice(1, 1, 7);
    // use accessor to access tensor elements efficiently
    // remap to original image and list bounding boxes for debugging purpose
    std::vector<Detection> det = ScaleCoordinates(data.accessor<float, 2>(), pad_w, pad_h, scale, check_img.size());
    cv::Mat image_draw_final=drawBox(draw_img, det, class_names,true,m_conf_threshold);
    cv::resize(image_draw_final(final_cut_rect),*image_out,cv::Size(origin_image.cols,origin_image.rows));
    *classname=out_name;
    *rect_out=out_rect;
    //time_end = double(clock());
    //m_runInfo=QString::fromLocal8Bit("耗时:%2ms").arg(QString::number(time_end-time_start));
    //std::cout<<m_runInfo.toLocal8Bit().data();


}
