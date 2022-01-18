#include "classfiedopenvino.h"

classfiedopenvino::classfiedopenvino(QObject *parent,int num,const std::string& device_,const std::string& model_path,const std::string& namesFile) : QObject(parent)
{
    //cv::imwrite("E:\\aaa1.jpg",blackImgae);
    detect_calsses=loadNames(namesFile);
    m_num=num;
    init(model_path,device_);
}

classfiedopenvino::~classfiedopenvino()
{
    //_network.~ExecutableNetwork();
}

template <class T>
void topResults(unsigned int n, InferenceEngine::TBlob<T>& input, std::vector<unsigned>& output)
{
    InferenceEngine::SizeVector dims = input.getTensorDesc().getDims();
    size_t input_rank = dims.size();
    if (!input_rank || !dims[0])
        THROW_IE_EXCEPTION << "Input blob has incorrect dimensions!";
    size_t batchSize = dims[0];
    std::vector<unsigned> indexes(input.size() / batchSize);
    n = static_cast<unsigned>(std::min<size_t>(size_t(n), input.size()));
    output.resize(n * batchSize);
    for (size_t i = 0; i < batchSize; i++)
    {
        size_t offset = i * (input.size() / batchSize);
        T* batchData = input.data();
        batchData += offset;

        std::iota(std::begin(indexes), std::end(indexes), 0);
        std::partial_sort(std::begin(indexes), std::begin(indexes) + n, std::end(indexes),
                          [&batchData](unsigned l, unsigned r)
        {
            return batchData[l] > batchData[r];
        });
        for (unsigned j = 0; j < n; j++)
        {
            output.at(i * n + j) = indexes.at(j);
        }
    }
}
void topResults(unsigned int n, InferenceEngine::Blob& input, std::vector<unsigned>& output)
{
#define TBLOB_TOP_RESULT(precision)                                                                             \
    case InferenceEngine::Precision::precision: {                                                               \
    using myBlobType = InferenceEngine::PrecisionTrait<InferenceEngine::Precision::precision>::value_type;  \
    InferenceEngine::TBlob<myBlobType>& tblob = dynamic_cast<InferenceEngine::TBlob<myBlobType>&>(input);   \
    topResults(n, tblob, output);                                                                           \
    break;                                                                                                  \
}

    switch (input.getTensorDesc().getPrecision())
    {
    TBLOB_TOP_RESULT(FP32);
    TBLOB_TOP_RESULT(FP64);
    TBLOB_TOP_RESULT(FP16);
    TBLOB_TOP_RESULT(Q78);
    TBLOB_TOP_RESULT(I16);
    TBLOB_TOP_RESULT(U8);
    TBLOB_TOP_RESULT(I8);
    TBLOB_TOP_RESULT(U16);
    TBLOB_TOP_RESULT(I32);
    TBLOB_TOP_RESULT(U32);
    TBLOB_TOP_RESULT(U64);
    TBLOB_TOP_RESULT(I64);
    default:
        THROW_IE_EXCEPTION << "cannot locate blob for precision: " << input.getTensorDesc().getPrecision();
    }

#undef TBLOB_TOP_RESULT
}

bool classfiedopenvino::init(const std::string& onnxfile,const std::string& device)
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
        auto output_data = item.second;
        output_data->setPrecision(InferenceEngine::Precision::FP32);
        // 显示输出维度
        auto out_shape = output_data->getTensorDesc().getDims();
    }
    _network = ie.LoadNetwork(network, device);
    return true;
}


std::vector<std::string> classfiedopenvino::loadNames(const std::string& path)
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

void classfiedopenvino::slot_setCheckPar(int num, cv::Mat src, int imageSize)
{
    if(m_num!=num)return;
    setCheckPar(src, imageSize);
    m_enable=true;
}

void classfiedopenvino::setCheckPar(cv::Mat src, int imageSize)
{
    image_size=imageSize;
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
    cv::resize(check_img,check_img,cv::Size(image_size,image_size));
}

void classfiedopenvino::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    Run();
    emit signal_result(m_num,class_detected);
}

QString classfiedopenvino::Run()
{
    process_frame(check_img);
    return class_detected;
}

//处理图像获取结果,输入图像要为RGB通道图像
bool classfiedopenvino::process_frame( cv::Mat& src)
{
    cv::Mat blob_image=src.clone();
    //cv::imwrite("E:\\aa.jpg",blob_image);
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
        float *data = static_cast<float*>(input->buffer());
        //#pragma omp parallel for
        std::vector<float> mean = { 0.485, 0.456, 0.406 };
        std::vector<float> std = { 0.229, 0.224, 0.225 };
        for (size_t row = 0; row < h; row++)
        {
            for (size_t col = 0; col < w; col++)
            {
                for (size_t ch = 0; ch < num_channels; ch++)
                {
                    float newpixel = double(blob_image.at<cv::Vec3b>(row, col)[ch])/255.0;
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
    std::vector<unsigned> _results;
    topResults(1, *output, _results);
    InferenceEngine::MemoryBlob::CPtr moutput = InferenceEngine::as<InferenceEngine::MemoryBlob>(output);
    // locked memory holder should be alive all time while access to its buffer happens
    auto moutputHolder = moutput->rmap();
    //const auto result = moutputHolder.as<const InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type*>()[_results[0]];
    int class_id = _results[0];
    //float conf = result;
    class_detected=QString::fromStdString(detect_calsses[class_id]);
    //std::cout << class_id << "   " << conf << std::endl;
    /*int type_num=1;
        for (size_t id = 0; id < type_num; ++id)
        {
            // Getting probability for resulting class
            InferenceEngine::MemoryBlob::CPtr moutput = InferenceEngine::as<InferenceEngine::MemoryBlob>(output);
            // locked memory holder should be alive all time while access to its buffer happens
            auto moutputHolder = moutput->rmap();

            const auto result = moutputHolder.
                as<const InferenceEngine::PrecisionTrait<InferenceEngine::Precision::FP32>::value_type*>()
                [_results[id]];
            std::cout << _results[id] << "   "<< result<<std::endl;
        }*/
    return true;
}
