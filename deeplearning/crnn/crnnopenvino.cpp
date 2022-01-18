#include "crnnopenvino.h"

CrnnOpenvino::CrnnOpenvino(QObject *parent,int num,const std::string& device_,const std::string& model_path,const std::string& namesFile) : QObject(parent)
{
    //cv::imwrite("E:\\aaa1.jpg",blackImgae);
    detect_calsses=loadNames(namesFile);
    m_num=num;
    init(model_path,device_);
}

CrnnOpenvino::~CrnnOpenvino()
{
    //_network.~ExecutableNetwork();
}

bool CrnnOpenvino::init(const std::string& onnxfile,const std::string& device)
{
    // Loading model to the device
    // 创建ie插件，查询支持硬件设备
    InferenceEngine::Core ie;
    /*vector<string> availableDevices = ie.GetAvailableDevices();
            for (i = 0; i < availableDevices.size(); ++i) {
                cout << "supported device name: " << availableDevices[i] << endl;
            }*/
    // 加载检测模型
    auto network = ie.ReadNetwork(onnxfile);
    // 设置输入格式
    _inputinfo=InferenceEngine::InputsDataMap(network.getInputsInfo());
    for (auto &item : _inputinfo)
    {
        _inputname=item.first;
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
        _outputname=item.first;
        output_data = item.second;
        output_data->setPrecision(InferenceEngine::Precision::FP32);
        // 显示输出维度
        auto out_shape = output_data->getTensorDesc().getDims();
    }
    _network = ie.LoadNetwork(network, device);
    return true;
}
std::vector<std::string> CrnnOpenvino::loadNames(const std::string& path)
{
    std::ifstream in(path);
    std::ostringstream tmp;
    tmp << in.rdbuf();
    std::string keys = tmp.str();
    std::vector<std::string> words;
    int len = keys.length();
    int i = 0;
    while (i < len) {
        assert ((keys[i] & 0xF8) <= 0xF0);
        int next = 1;
        if ((keys[i] & 0x80) == 0x00) {
        } else if ((keys[i] & 0xE0) == 0xC0) {
            next = 2;
        } else if ((keys[i] & 0xF0) == 0xE0) {
            next = 3;
        } else if ((keys[i] & 0xF8) == 0xF0) {
            next = 4;
        }
        words.push_back(keys.substr(i, next));
        i += next;
    }
    return words;
}

void CrnnOpenvino::slot_setCheckPar(int num, cv::Mat src, int width,int height)
{
    if(m_num!=num)return;
    setCheckPar(src,width,height);
    m_enable=true;
}
void CrnnOpenvino::setCheckPar(cv::Mat src, int width,int height)
{
    g_width=150;
    g_height=32;
    check_img=src.clone();
    int channals=check_img.channels();
    if(channals==1)
    {
        cvtColor(check_img, check_img, cv::COLOR_GRAY2RGB);
    }
    else if(channals==3)
    {
        cvtColor(check_img, check_img, cv::COLOR_BGR2RGB);
    }
    int src_width=check_img.cols;
    int src_height=check_img.rows;
    double width_ratio=double(src_width)/150;
    double height_ratio=double(src_height)/32;
    double ratio=0.0;
    width_ratio>height_ratio?ratio=width_ratio:ratio=height_ratio;
    int new_width=src_width/ratio;
    int new_height=src_height/ratio;
    cv::resize(check_img,check_img,cv::Size(new_width,new_height));//150*32
    int width_boder_add=(150-new_width);
    int height_boder_add=(32-new_height)/2;
    copyMakeBorder(check_img, check_img, height_boder_add, height_boder_add, 0, width_boder_add,
                   cv::BORDER_REPLICATE);

}
void CrnnOpenvino::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    QString text=Run();
    emit signal_result(m_num,text);
}

QString CrnnOpenvino::Run()
{
    cv::Mat checkimg=check_img.clone();

    //if(src.empty())return "0";
    cv::resize(checkimg,checkimg,cv::Size(g_width,g_height));//150*32
    cv::imwrite("e:\\aa.jpg",check_img);
    cv::Mat blob_image=checkimg.clone();
    // Create infer request
    //double start=double(clock());
    // Create infer request
    InferenceEngine::InferRequest infer_request = _network.CreateInferRequest();
    //for (auto &item : _inputinfo)
    {
        //auto input_name = item.first;
        /* Getting input blob */
        auto input = infer_request.GetBlob(_inputname);
        size_t num_channels = input->getTensorDesc().getDims()[1];
        size_t h = input->getTensorDesc().getDims()[2];
        size_t w = input->getTensorDesc().getDims()[3];
        size_t image_size = h * w;
        // NCHW
        //cv::resize(blob_image, blob_image, cv::Size(w, h));
        //cv::Mat  image_resized_float;

        float *data = static_cast<float*>(input->buffer());
        //#pragma omp parallel for
        std::vector<float> mean = { 0.5, 0.5, 0.5 };
        std::vector<float> std = { 0.5, 0.5, 0.5 };
        for (int row = 0; row < h; row++)
        {
            for (int col = 0; col < w; col++)
            {
                for (int ch = 0; ch < num_channels; ch++)
                {
                    float newpixel = float(blob_image.at<cv::Vec3b>(row, col)[ch])/255.0;
                    newpixel -= mean[ch];
                    newpixel /= std[ch];
                    data[image_size * ch + row * w + col] = newpixel;
                }
            }
        }
    }
    // --------------------------- 7. Do inference --------------------------------------------------------
    /* Running the request synchronously */
    infer_request.Infer();
    // --------------------------- 8. Process output ------------------------------------------------------
    InferenceEngine::Blob::Ptr output = infer_request.GetBlob(_outputname);
    auto out_width = output_data->getDims().at(2);
    auto out_height = output_data->getDims().at(1);
    auto out_channels = output_data->getDims().at(0);
    auto output_result = output->buffer().
            as<InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type*>();
    std::vector<int> predChars;
    for (auto channel = 0; channel < out_channels; ++channel)
    {
        std::vector<float> channel_content;
        for (auto dy = 0; dy < out_height; ++dy)
        {
            for (auto dx = 0; dx < out_width; ++dx)
            {
                float value = output_result[dx + dy * out_width + channel * out_height * out_width];
                channel_content.push_back(value);
                //std::cout << value << " ";
            }
        }
        //std::cout << std::distance(channel_content.begin(), std::max_element(channel_content.begin(), channel_content.end())) <<std::endl;
        predChars.push_back(std::distance(channel_content.begin(), std::max_element(channel_content.begin(), channel_content.end())));
    }
    std::string realChars="";
    for(uint i=0; i<predChars.size(); i++)
    {
        if(predChars[i] != 0 && !(i>0 && predChars[i-1]==predChars[i]))
        {
            realChars +=detect_calsses[predChars[i]-1] ;
        }
    }
    QString text=QString::fromStdString(realChars);
    return text;
}
