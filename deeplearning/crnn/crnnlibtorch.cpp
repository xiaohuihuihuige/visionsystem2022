#include "crnnlibtorch.h"

CrnnLibtorch::CrnnLibtorch(QObject *parent ,int num,const std::string& model_path,const std::string& namesFile):
    QObject(parent),
    device_(torch::cuda::is_available()?torch::kCUDA:torch::kCPU)
{
    m_enable=false;
    m_num=num;
    loadModel(model_path);
    class_names=loadNames(namesFile);
}

CrnnLibtorch::~CrnnLibtorch()
{

}
//加载模型
void CrnnLibtorch::loadModel(const std::string &model_path)
{
    try
    {
        //device_=torch::kCPU;
        module_ = torch::jit::load(model_path);
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
std::vector<std::string> CrnnLibtorch::loadNames(const std::string &path)
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
void CrnnLibtorch::slot_setCheckPar(int num,cv::Mat src,int width,int height )//100*32
{
    if(m_num!=num)return;
    m_enable=true;
    setCheckPar(src,width,height );
}
void CrnnLibtorch::setCheckPar(cv::Mat src,int width,int height )//100*32
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


void CrnnLibtorch::slot_run()
{
    if(!m_enable)return;
    m_enable=false;
    QString text=run();
    emit signal_result(m_num,text);
}
//执行检测
QString CrnnLibtorch::run()
{
    torch::Tensor  input=setMean(check_img);
    //std::cout<<inputs.size()<<std::endl;
    std::vector<torch::jit::IValue> inputs;
    inputs.push_back(input.to(device_));
    torch::Tensor output = module_.forward(inputs).toTensor();
    std::vector<int> predChars;
    int numImgs = output.sizes()[1];
    if(numImgs == 1)
    {
        for(uint i=0; i<output.sizes()[0]; i++)
        {
            auto maxRes = output[i].max(1, true);
            int maxIdx = std::get<1>(maxRes).item<float>();
            predChars.push_back(maxIdx);
            //std::cout << maxIdx << std::endl;
        }
        // 字符转录处理
        std::string realChars="";
        for(uint i=0; i<predChars.size(); i++)
        {
            if(predChars[i] != 0 && !(i>0 && predChars[i-1]==predChars[i]))
            {
                realChars +=class_names[predChars[i]-1] ;
            }
        }
        //std::cout << realChars << std::endl;
        return QString::fromStdString(realChars);
    }
    else
    {
        std::vector<std::string> realCharLists;
        std::vector<std::vector<int>> predictCharLists;
        for (int i=0; i<output.sizes()[1]; i++)
        {
            std::vector<int> temp;
            for(int j=0; j<output.sizes()[0]; j++)
            {
                auto max_result = (output[j][i]).max(0, true);
                int max_index = std::get<1>(max_result).item<float>();//predict value
                temp.push_back(max_index);

            }
            predictCharLists.push_back(temp);
        }

        for(auto vec : predictCharLists)
        {
            std::string text = "";
            for(uint i=0; i<vec.size(); i++)
            {
                if(vec[i] != 0)
                {
                    if(!(i>0 && vec[i-1]==vec[i]))
                    {
                        text += this->class_names[vec[i]];
                    }
                }
            }
            realCharLists.push_back(text);
        }
        /*for(auto t : realCharLists)
        {
            std::cout << t << std::endl;
        }*/
        return QString::fromStdString(realCharLists[0]);
    }
}
//滤波
torch::Tensor  CrnnLibtorch::setMean(cv::Mat  &input,bool isbath)
{
    int resize_h = int(input.cols * 32 / input.rows);
    cv::resize(input, input, cv::Size(g_width, g_height)/*,0,0,cv::INTER_AREA*/);//150*32
    //input.convertTo(input,CV_32FC1, 1.0 / 255.0);
    torch::Tensor imgTensor;
    if(isbath){
        imgTensor = torch::from_blob(input.data, {g_height, g_width, 1}, torch::kByte);
        imgTensor = imgTensor.permute({2,0,1});
    }else
    {
        imgTensor = torch::from_blob(input.data, {1,g_height, g_width, 1}, torch::kByte);
        imgTensor = imgTensor.permute({0,3,1,2});
    }
    imgTensor = imgTensor.toType(torch::kFloat);
    imgTensor = imgTensor.div(255);
    imgTensor = imgTensor.sub(0.5);
    imgTensor = imgTensor.div(0.5);
    return imgTensor;
}

