//#include "yolact.h"

//Yolact::Yolact(QObject *parent,int num,const std::string& model_path,const std::string& namesFile) : QObject(parent),
//  device_(torch::cuda::is_available()?torch::kCUDA:torch::kCPU),
//  m_enable(false)
//{
//    try
//    {
//        //std::cout<<model_path;
//        // Deserialize the ScriptModule from a file using torch::jit::load().
//        std::string path="";
//        if(torch::cuda::is_available())
//        {
//            path=model_path+"_cuda.pt";
//        }
//        else
//        {
//            path=model_path+"_cpu.pt";
//        }
//        half_=false;
//        module_ = torch::jit::load(path.data());

//    }
//    catch (const c10::Error& e)
//    {
//        std::cerr << "Error loading the model!\n";
//        std::exit(EXIT_FAILURE);
//    }
//    m_num=num;
//    module_.to(device_);

//    module_.eval();

//    class_names=loadNames(namesFile);
//}
//inline float Yolact::intersection_area(const Object& a, const Object& b)
//{
//    cv::Rect_<float> inter = a.rect & b.rect;
//    return inter.area();
//}

//void Yolact::qsort_descent_inplace(std::vector<Object>& objects, int left, int right)
//{
//    int i = left;
//    int j = right;
//    float p = objects[(left + right) / 2].prob;

//    while (i <= j)
//    {
//        while (objects[i].prob > p)
//            i++;

//        while (objects[j].prob < p)
//            j--;

//        if (i <= j)
//        {
//            // swap
//            std::swap(objects[i], objects[j]);

//            i++;
//            j--;
//        }
//    }

//    #pragma omp parallel sections
//    {
//        #pragma omp section
//        {
//            if (left < j) qsort_descent_inplace(objects, left, j);
//        }
//        #pragma omp section
//        {
//            if (i < right) qsort_descent_inplace(objects, i, right);
//        }
//    }
//}

//void Yolact::qsort_descent_inplace(std::vector<Object>& objects)
//{
//    if (objects.empty())
//        return;

//    qsort_descent_inplace(objects, 0, objects.size() - 1);
//}

//void Yolact::nms_sorted_bboxes(const std::vector<Object>& objects, std::vector<int>& picked, float nms_threshold)
//{
//    picked.clear();

//    const int n = objects.size();

//    std::vector<float> areas(n);
//    for (int i = 0; i < n; i++)
//    {
//        areas[i] = objects[i].rect.area();
//    }

//    for (int i = 0; i < n; i++)
//    {
//        const Object& a = objects[i];

//        int keep = 1;
//        for (int j = 0; j < (int)picked.size(); j++)
//        {
//            const Object& b = objects[picked[j]];

//            // intersection over union
//            float inter_area = intersection_area(a, b);
//            float union_area = areas[i] + areas[picked[j]] - inter_area;
//            //             float IoU = inter_area / union_area
//            if (inter_area / union_area > nms_threshold)
//                keep = 0;
//        }

//        if (keep)
//            picked.push_back(i);
//    }
//}
////裁剪
//cv::Mat  Yolact::pilCropCenter(cv::Mat  &img, int output_size)
//{
//    cv::Rect imgRect;
//    imgRect.x = int(round((img.cols - output_size) / 2.));
//    imgRect.y = int(round((img.rows - output_size) / 2.));
//    imgRect.width = output_size;
//    imgRect.height = output_size;

//    return img(imgRect).clone();
//}

////均匀化
//cv::Mat  Yolact::setNorm(cv::Mat  &img,int imageSize,int crop_size)
//{
//    cv::Mat  img_rgb;
//    //cvtColor(img, img_rgb, cv::COLOR_RGB2BGR);
//    img_rgb=img.clone();
//    //cv::imwrite("E:\\aa.jpg",img_rgb);
//    //cv::Mat  img_resize = pilResize(img_rgb, imageSize);
//    cv::Mat img_resize;
//    cv::resize(img_rgb,img_resize,cv::Size(imageSize,imageSize));
//    cv::Mat  img_crop = pilCropCenter(img_resize, imageSize-crop_size);

//    cv::Mat  image_resized_float;
//    img_crop.convertTo(image_resized_float, CV_32F, 1.0 / 255.0);

//    return image_resized_float;
//}
////滤波
//cv::Mat  Yolact::setMean(cv::Mat  &image_resized_float)
//{
//    std::vector<float> mean = { 0.485, 0.456, 0.406 };
//    std::vector<float> std = { 0.229, 0.224, 0.225 };
//    std::vector<cv::Mat> image_resized_split;
//    split(image_resized_float, image_resized_split);
//    for (int ch = 0; ch < image_resized_split.size(); ch++)
//    {
//        image_resized_split[ch] -= mean[ch];
//        image_resized_split[ch] /= std[ch];
//    }
//    cv::Mat image_resized_merge;
//    merge(image_resized_split, image_resized_merge);

//    return image_resized_merge;
//}


////int Yolact::detect_yolact(const cv::Mat& src, std::vector<Object>& objects)
////{
////    const int imageSize = 550;
////    const int crop_size = 0;
////    torch::NoGradGuard no_grad;
////    // TODO: check_img_size()

////    /*** Pre-process ***/
////    int channels=src.channels();
////    cv::Mat rgbimg;
////    if(channels==1)
////    {
////        cv::cvtColor(src,rgbimg,cv::COLOR_GRAY2BGR);
////    }
////    else if(channels==3)
////    {
////        cv::cvtColor(src,rgbimg,cv::COLOR_RGB2BGR);
////    }

////    cv::Mat image_resized_float = setNorm(rgbimg,imageSize,crop_size);
////    //mean
////    cv::Mat image_resized_merge = setMean(image_resized_float);
////    auto tensor_img = torch::from_blob(image_resized_merge.data, {1, image_resized_merge.rows, image_resized_merge.cols, image_resized_merge.channels()}).to(device_);

////    tensor_img = tensor_img.permute({0, 3, 1, 2}).contiguous();  // BHWC -> BCHW (Batch, Channel, Height, Width)

////    if (half_) {
////        tensor_img = tensor_img.to(torch::kHalf);
////    }

////    std::vector<torch::jit::IValue> inputs;
////    inputs.emplace_back(tensor_img);
////    /*** Inference ***/
////    // TODO: add synchronize point

////    // inference
////    torch::jit::IValue output = module_.forward(inputs).toTuple()->elements()[0];

////    /*** Post-process ***/
////    auto detections = output.toTensor();



////    const float mean_vals[3] = {123.68f, 116.78f, 103.94f};
////    const float norm_vals[3] = {1.0 / 58.40f, 1.0 / 57.12f, 1.0 / 57.38f};

////    int num_class = confidence.w;
////    int num_priors = confidence.h;

////    // make priorbox
////    ncnn::Mat priorbox(4, num_priors);
////    {
////        const int conv_ws[5] = {69, 35, 18, 9, 5};
////        const int conv_hs[5] = {69, 35, 18, 9, 5};

////        const float aspect_ratios[3] = {1.f, 0.5f, 2.f};
////        const float scales[5] = {24.f, 48.f, 96.f, 192.f, 384.f};

////        float* pb = priorbox;

////        for (int p = 0; p < 5; p++)
////        {
////            int conv_w = conv_ws[p];
////            int conv_h = conv_hs[p];

////            float scale = scales[p];

////            for (int i = 0; i < conv_h; i++)
////            {
////                for (int j = 0; j < conv_w; j++)
////                {
////                    // +0.5 because priors are in center-size notation
////                    float cx = (j + 0.5f) / conv_w;
////                    float cy = (i + 0.5f) / conv_h;

////                    for (int k = 0; k < 3; k++)
////                    {
////                        float ar = aspect_ratios[k];

////                        ar = sqrt(ar);

////                        float w = scale * ar / 550;
////                        float h = scale / ar / 550;

////                        // This is for backward compatability with a bug where I made everything square by accident
////                        // cfg.backbone.use_square_anchors:
////                        h = w;

////                        pb[0] = cx;
////                        pb[1] = cy;
////                        pb[2] = w;
////                        pb[3] = h;

////                        pb += 4;
////                    }
////                }
////            }
////        }
////    }

////    const float confidence_thresh = 0.05f;
////    const float nms_threshold = 0.5f;
////    const int keep_top_k = 200;

////    std::vector<std::vector<Object> > class_candidates;
////    class_candidates.resize(num_class);

////    for (int i = 0; i < num_priors; i++)
////    {
////        const float* conf = confidence.row(i);
////        const float* loc = location.row(i);
////        const float* pb = priorbox.row(i);
////        const float* maskdata = mask.row(i);

////        // find class id with highest score
////        // start from 1 to skip background
////        int label = 0;
////        float score = 0.f;
////        for (int j = 1; j < num_class; j++)
////        {
////            float class_score = conf[j];
////            if (class_score > score)
////            {
////                label = j;
////                score = class_score;
////            }
////        }

////        // ignore background or low score
////        if (label == 0 || score <= confidence_thresh)
////            continue;

////        // CENTER_SIZE
////        float var[4] = {0.1f, 0.1f, 0.2f, 0.2f};

////        float pb_cx = pb[0];
////        float pb_cy = pb[1];
////        float pb_w = pb[2];
////        float pb_h = pb[3];

////        float bbox_cx = var[0] * loc[0] * pb_w + pb_cx;
////        float bbox_cy = var[1] * loc[1] * pb_h + pb_cy;
////        float bbox_w = (float)(exp(var[2] * loc[2]) * pb_w);
////        float bbox_h = (float)(exp(var[3] * loc[3]) * pb_h);

////        float obj_x1 = bbox_cx - bbox_w * 0.5f;
////        float obj_y1 = bbox_cy - bbox_h * 0.5f;
////        float obj_x2 = bbox_cx + bbox_w * 0.5f;
////        float obj_y2 = bbox_cy + bbox_h * 0.5f;

////        // clip
////        obj_x1 = std::max(std::min(obj_x1 * src.cols, (float)(src.cols - 1)), 0.f);
////        obj_y1 = std::max(std::min(obj_y1 * src.rows, (float)(src.rows - 1)), 0.f);
////        obj_x2 = std::max(std::min(obj_x2 * src.cols, (float)(src.cols - 1)), 0.f);
////        obj_y2 = std::max(std::min(obj_y2 * src.rows, (float)(src.rows - 1)), 0.f);

////        // append object
////        Object obj;
////        obj.rect = cv::Rect_<float>(obj_x1, obj_y1, obj_x2 - obj_x1 + 1, obj_y2 - obj_y1 + 1);
////        obj.label = label;
////        obj.prob = score;
////        obj.maskdata = std::vector<float>(maskdata, maskdata + mask.w);

////        class_candidates[label].push_back(obj);
////    }

////    objects.clear();
////    for (int i = 0; i < (int)class_candidates.size(); i++)
////    {
////        std::vector<Object>& candidates = class_candidates[i];

////        qsort_descent_inplace(candidates);

////        std::vector<int> picked;
////        nms_sorted_bboxes(candidates, picked, nms_threshold);

////        for (int j = 0; j < (int)picked.size(); j++)
////        {
////            int z = picked[j];
////            objects.push_back(candidates[z]);
////        }
////    }

////    qsort_descent_inplace(objects);

////    // keep_top_k
////    if (keep_top_k < (int)objects.size())
////    {
////        objects.resize(keep_top_k);
////    }

////    // generate mask
////    for (int i = 0; i < (int)objects.size(); i++)
////    {
////        Object& obj = objects[i];

////        cv::Mat mask(maskmaps.h, maskmaps.w, CV_32FC1);
////        {
////            mask = cv::Scalar(0.f);

////            for (int p = 0; p < maskmaps.c; p++)
////            {
////                const float* maskmap = maskmaps.channel(p);
////                float coeff = obj.maskdata[p];
////                float* mp = (float*)mask.data;

////                // mask += m * coeff
////                for (int j = 0; j < maskmaps.w * maskmaps.h; j++)
////                {
////                    mp[j] += maskmap[j] * coeff;
////                }
////            }
////        }

////        cv::Mat mask2;
////        cv::resize(mask, mask2, cv::Size(img_w, img_h));

////        // crop obj box and binarize
////        obj.mask = cv::Mat(img_h, img_w, CV_8UC1);
////        {
////            obj.mask = cv::Scalar(0);

////            for (int y = 0; y < img_h; y++)
////            {
////                if (y < obj.rect.y || y > obj.rect.y + obj.rect.height)
////                    continue;

////                const float* mp2 = mask2.ptr<const float>(y);
////                uchar* bmp = obj.mask.ptr<uchar>(y);

////                for (int x = 0; x < img_w; x++)
////                {
////                    if (x < obj.rect.x || x > obj.rect.x + obj.rect.width)
////                        continue;

////                    bmp[x] = mp2[x] > 0.5f ? 255 : 0;
////                }
////            }
////        }
////    }

////    return 0;
////}

//void Yolact::draw_objects(const cv::Mat& bgr, const std::vector<Object>& objects)
//{
//    static const char* class_names[] = {"background",
//                                        "person", "bicycle", "car", "motorcycle", "airplane", "bus",
//                                        "train", "truck", "boat", "traffic light", "fire hydrant",
//                                        "stop sign", "parking meter", "bench", "bird", "cat", "dog",
//                                        "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe",
//                                        "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
//                                        "skis", "snowboard", "sports ball", "kite", "baseball bat",
//                                        "baseball glove", "skateboard", "surfboard", "tennis racket",
//                                        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl",
//                                        "banana", "apple", "sandwich", "orange", "broccoli", "carrot",
//                                        "hot dog", "pizza", "donut", "cake", "chair", "couch",
//                                        "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
//                                        "mouse", "remote", "keyboard", "cell phone", "microwave", "oven",
//                                        "toaster", "sink", "refrigerator", "book", "clock", "vase",
//                                        "scissors", "teddy bear", "hair drier", "toothbrush"
//                                       };

//    static const unsigned char colors[81][3] = {
//        {56, 0, 255},
//        {226, 255, 0},
//        {0, 94, 255},
//        {0, 37, 255},
//        {0, 255, 94},
//        {255, 226, 0},
//        {0, 18, 255},
//        {255, 151, 0},
//        {170, 0, 255},
//        {0, 255, 56},
//        {255, 0, 75},
//        {0, 75, 255},
//        {0, 255, 169},
//        {255, 0, 207},
//        {75, 255, 0},
//        {207, 0, 255},
//        {37, 0, 255},
//        {0, 207, 255},
//        {94, 0, 255},
//        {0, 255, 113},
//        {255, 18, 0},
//        {255, 0, 56},
//        {18, 0, 255},
//        {0, 255, 226},
//        {170, 255, 0},
//        {255, 0, 245},
//        {151, 255, 0},
//        {132, 255, 0},
//        {75, 0, 255},
//        {151, 0, 255},
//        {0, 151, 255},
//        {132, 0, 255},
//        {0, 255, 245},
//        {255, 132, 0},
//        {226, 0, 255},
//        {255, 37, 0},
//        {207, 255, 0},
//        {0, 255, 207},
//        {94, 255, 0},
//        {0, 226, 255},
//        {56, 255, 0},
//        {255, 94, 0},
//        {255, 113, 0},
//        {0, 132, 255},
//        {255, 0, 132},
//        {255, 170, 0},
//        {255, 0, 188},
//        {113, 255, 0},
//        {245, 0, 255},
//        {113, 0, 255},
//        {255, 188, 0},
//        {0, 113, 255},
//        {255, 0, 0},
//        {0, 56, 255},
//        {255, 0, 113},
//        {0, 255, 188},
//        {255, 0, 94},
//        {255, 0, 18},
//        {18, 255, 0},
//        {0, 255, 132},
//        {0, 188, 255},
//        {0, 245, 255},
//        {0, 169, 255},
//        {37, 255, 0},
//        {255, 0, 151},
//        {188, 0, 255},
//        {0, 255, 37},
//        {0, 255, 0},
//        {255, 0, 170},
//        {255, 0, 37},
//        {255, 75, 0},
//        {0, 0, 255},
//        {255, 207, 0},
//        {255, 0, 226},
//        {255, 245, 0},
//        {188, 255, 0},
//        {0, 255, 18},
//        {0, 255, 75},
//        {0, 255, 151},
//        {255, 56, 0},
//        {245, 255, 0}
//    };

//    cv::Mat image = bgr.clone();

//    int color_index = 0;

//    for (size_t i = 0; i < objects.size(); i++)
//    {
//        const Object& obj = objects[i];

//        if (obj.prob < 0.15)
//            continue;

//        fprintf(stderr, "%d = %.5f at %.2f %.2f %.2f x %.2f\n", obj.label, obj.prob,
//                obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

//        const unsigned char* color = colors[color_index++];

//        cv::rectangle(image, obj.rect, cv::Scalar(color[0], color[1], color[2]));

//        char text[256];
//        sprintf(text, "%s %.1f%%", class_names[obj.label], obj.prob * 100);

//        int baseLine = 0;
//        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

//        int x = obj.rect.x;
//        int y = obj.rect.y - label_size.height - baseLine;
//        if (y < 0)
//            y = 0;
//        if (x + label_size.width > image.cols)
//            x = image.cols - label_size.width;

//        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
//                      cv::Scalar(255, 255, 255), -1);

//        cv::putText(image, text, cv::Point(x, y + label_size.height),
//                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

//        // draw mask
//        for (int y = 0; y < image.rows; y++)
//        {
//            const uchar* mp = obj.mask.ptr(y);
//            uchar* p = image.ptr(y);
//            for (int x = 0; x < image.cols; x++)
//            {
//                if (mp[x] == 255)
//                {
//                    p[0] = cv::saturate_cast<uchar>(p[0] * 0.5 + color[0] * 0.5);
//                    p[1] = cv::saturate_cast<uchar>(p[1] * 0.5 + color[1] * 0.5);
//                    p[2] = cv::saturate_cast<uchar>(p[2] * 0.5 + color[2] * 0.5);
//                }
//                p += 3;
//            }
//        }
//    }

//    //cv::imwrite("result.png", image);
//    //cv::imshow("image", image);
//}


////int main(int argc, char** argv)
////{
////    if (argc != 2)
////    {
////        fprintf(stderr, "Usage: %s [imagepath]\n", argv[0]);
////        return -1;
////    }

////    const char* imagepath = argv[1];

////    cv::Mat m = cv::imread(imagepath, 1);
////    if (m.empty())
////    {
////        fprintf(stderr, "cv::imread %s failed\n", imagepath);
////        return -1;
////    }

////    std::vector<Object> objects;
////    detect_yolact(m, objects);

////    draw_objects(m, objects);

////    return 0;
////}
