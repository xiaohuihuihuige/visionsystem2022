#include "classified.h"

//初始化类
Classified::Classified(QObject *parent ,int num,const std::string& model_path,const std::string& namesFile):
    QObject(parent),
    device_(torch::cuda::is_available()?torch::kCUDA:torch::kCPU)
{
    m_enable=false;
    m_num=num;
    loadModel(model_path);
    class_names=loadNames(namesFile);
}

Classified::~Classified()
{

}
//加载模型
void Classified::loadModel(const std::string &model_path)
{
    try
    {
        std::string path="";
        path=model_path;
        //path=model_path+"_cpu.pt";
        //device_=torch::kCPU;
        module_ = torch::jit::load(path);
        half_=false;
    }
    catch (const c10::Error& e)
    {
        std::exit(EXIT_FAILURE);
    }
    module_.to(device_);
    module_.eval();
}

//加载类名
std::vector<std::string> Classified::loadNames(const std::string &path)
{
    // load class names
    std::vector<std::string> class_names;
    std::ifstream infile(path);
    if (infile.is_open()) {
        std::string line;
        while (getline (infile,line)) {
            class_names.emplace_back(line);
        }
        infile.close();
    }
    return class_names;
}
void Classified::slot_setCheckPar(int num,cv::Mat src,int imageSize)
{
    if(m_num!=num)return;
    setCheckPar(src, imageSize);
    m_enable=true;
}
void Classified::setCheckPar(cv::Mat src, int imageSize)
{
    check_img=src.clone();
    m_img_size=imageSize;
}

void Classified::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    QString result=run();
    emit signal_result(m_num,result);
}

//执行检测
QString Classified::run()
{
    //if(src.empty())return "0";
    cv::Mat src=check_img.clone();
    int channels=src.channels();
    cv::Mat rgbimg;
    if(channels==1)
    {
        cv::cvtColor(src,rgbimg,cv::COLOR_GRAY2RGB);
    }
    else if(channels==3)
    {
        cv::cvtColor(src,rgbimg,cv::COLOR_BGR2RGB);
    }

    cv::Mat image_resized_float = setNorm(rgbimg,m_img_size);
    //mean
    cv::Mat image_resized_merge = setMean(image_resized_float);
    auto img_tensor = torch::from_blob(image_resized_merge.data, { m_img_size, m_img_size, 3 }, torch::kFloat32);
    auto img_tensor_ = torch::unsqueeze(img_tensor, 0);
    img_tensor_ = img_tensor_.permute({ 0, 3, 1, 2 });

    // Create a vector of inputs.
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(img_tensor_.to(device_));

    torch::Tensor prob = module_.forward(inputs).toTensor();
    torch::Tensor output = torch::softmax(prob, 1);
    auto predict = torch::max(output, 1);
    auto max_index = std::get<1>(predict).item<int>();
    return QString::fromStdString(class_names[int(max_index)]);
}

//resize
cv::Mat Classified::pilResize(cv::Mat  &img, int size)
{
    int imgWidth = img.cols;
    int imgHeight = img.rows;
    if ((imgWidth <= imgHeight && imgWidth == size) || (imgHeight <= imgWidth && imgHeight == size)) {
        return img;
    }
    cv::Mat output;
    if (imgWidth < imgHeight) {
        int outWidth = size;
        int outHeight = int(size * imgHeight / float(imgWidth));
        resize(img, output, cv::Size(outWidth, outHeight));
    }
    else {
        int outHeight = size;
        int outWidth = int(size * imgWidth / float(imgHeight));
        resize(img, output, cv::Size(outWidth, outHeight));
    }

    return output;
}

//裁剪
cv::Mat  Classified::pilCropCenter(cv::Mat  &img, int output_size)
{
    cv::Rect imgRect;
    imgRect.x = int(round((img.cols - output_size) / 2.));
    imgRect.y = int(round((img.rows - output_size) / 2.));
    imgRect.width = output_size;
    imgRect.height = output_size;

    return img(imgRect).clone();
}

//均匀化
cv::Mat  Classified::setNorm(cv::Mat  &img,int imageSize)
{
    cv::Mat  img_rgb;
    //cvtColor(img, img_rgb, cv::COLOR_RGB2BGR);
    img_rgb=img.clone();
    //cv::imwrite("E:\\aa.jpg",img_rgb);
    //cv::Mat  img_resize = pilResize(img_rgb, imageSize);
    cv::Mat img_resize;
    cv::resize(img_rgb,img_resize,cv::Size(imageSize,imageSize));
    cv::Mat  img_crop = pilCropCenter(img_resize, imageSize);

    cv::Mat  image_resized_float;
    img_crop.convertTo(image_resized_float, CV_32F, 1.0 / 255.0);

    return image_resized_float;
}
//滤波
cv::Mat  Classified::setMean(cv::Mat  &image_resized_float)
{
    std::vector<float> mean = { 0.485, 0.456, 0.406 };
    std::vector<float> std = { 0.229, 0.224, 0.225 };
    std::vector<cv::Mat> image_resized_split;
    split(image_resized_float, image_resized_split);
    for (int ch = 0; ch < image_resized_split.size(); ch++)
    {
        image_resized_split[ch] -= mean[ch];
        image_resized_split[ch] /= std[ch];
    }
    cv::Mat image_resized_merge;
    merge(image_resized_split, image_resized_merge);

    return image_resized_merge;
}




