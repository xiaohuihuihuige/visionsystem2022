#include"deeplearning/Classified/classified.h"
#include "pro_siliconetube.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
#include"deeplearning/Classified/classfiedopenvino.h"


Pro_SiliconeTube::Pro_SiliconeTube(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    ng_num=0;
    check_err=false;
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
    //打开一个隐藏得窗口用于绘图
    OpenWindow(0,0,1280,720,0,"invisible","",&hv_WindowHandle_UI);
}
Pro_SiliconeTube::~Pro_SiliconeTube()
{
    slot_program_exit();
}
void Pro_SiliconeTube::initGlobalPar(QString pro,QString station,QString cam,QString type)
{
    m_ProName=pro;
    m_ProPath=m_ExePath+"\\Pro\\"+m_ProName;
    m_StationName=station;
    m_station_path=m_ProPath+"\\"+m_StationName; //工位路径
    m_CamName=cam;
    m_CamPath=m_station_path+"\\"+m_CamName;
    m_TypeName=type;
    m_TypePath=m_CamPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_TypeName;
    readROI();
    readPar();
    //readTypeInfo();
}

//根据启用的品类，读取选区信息
void Pro_SiliconeTube::readROI()
{
    map_obj.clear();
    ConfigFileOperate tconfig(m_TypePath+"\\ROIConfig.ini");
    QStringList  roilist=tconfig.childKeys();
    for (int i=0;i<roilist.size();i++)
    {
        map_obj[roilist[i]]=tconfig.readKeyValue(roilist[i]);
    }
}

void Pro_SiliconeTube::readTypeInfo()
{
    slot_program_exit();

    map_Thread.clear();
    if(m_StringParameter[QString::fromLocal8Bit("处理器")]=="GPU")
    {
        map_Classified.clear();
        m_weightfile= m_TypePath+"\\best.torchscript";//权重文件
        m_namesfile= m_TypePath+"\\voc.names";//权重文件
        std::string weights=m_weightfile.toStdString();
        std::string name_file =m_namesfile.toStdString();
        for (int i=0;i<map_obj.size();i++)
        {
            map_Classified[i]=new Classified(nullptr,i,weights,name_file);
            map_Thread[i]=new QThread(this);
            map_Classified[i]->moveToThread(map_Thread[i]);
            connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
            connect(map_Thread[i],&QThread::finished,map_Classified[i],&QObject::deleteLater);
            connect(this,&Pro_SiliconeTube::signal_check_image,map_Classified[i],&Classified::slot_run);
            connect(this,&Pro_SiliconeTube::signal_setCheckPar,map_Classified[i],&Classified::slot_setCheckpar);
            connect(map_Classified[i],&Classified::signal_result,this,&Pro_SiliconeTube::slot_check_result);
            map_Thread[i]->start();
        }
    }
    else
    {
        map_openvino.clear();
        m_weightfile= m_TypePath+"\\best.onnx";//权重文件
        m_namesfile= m_TypePath+"\\voc.names";//权重文件
        std::string weights=m_weightfile.toStdString();
        std::string name_file =m_namesfile.toStdString();
        for (int i=0;i<map_obj.size();i++)
        {
            if(m_StringParameter[QString::fromLocal8Bit("处理器")]!="CPU")
            {
                map_openvino[i]=new classfiedopenvino(nullptr,i,"GPU",weights,name_file);
            }
            else
            {
                map_openvino[i]=new classfiedopenvino(nullptr,i,"CPU",weights,name_file);
            }
            map_Thread[i]=new QThread(this);
            map_openvino[i]->moveToThread(map_Thread[i]);
            connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
            connect(map_Thread[i],&QThread::finished,map_openvino[i],&QObject::deleteLater);
            connect(this,&Pro_SiliconeTube::signal_check_image,map_openvino[i],&classfiedopenvino::slot_run);
            connect(this,&Pro_SiliconeTube::signal_setCheckPar,map_openvino[i],&classfiedopenvino::slot_setCheckpar);
            connect(map_openvino[i],&classfiedopenvino::signal_result,this,&Pro_SiliconeTube::slot_check_result);
            map_Thread[i]->start();
        }
    }
}
//根据启用的品类读取参数信息
void Pro_SiliconeTube::readPar()
{
    QStringList keys,values;
    ConfigFileOperate tconfig(m_TypePath+"\\ParConfig.ini");

    tconfig.readSection("IntPar",&keys,&values);
    m_IntParameter.clear();
    for(int i=0;i<keys.size();i++)
    {
        m_IntParameter[keys[i]]=values[i].toInt();
    }

    tconfig.readSection("FloatPar",&keys,&values);
    m_FloatParameter.clear();
    for(int i=0;i<keys.size();i++)
    {
        m_FloatParameter[keys[i]]=values[i].toFloat();
    }

    tconfig.readSection("StringPar",&keys,&values);
    m_StringParameter.clear();
    for(int i=0;i<keys.size();i++)
    {
        m_StringParameter[keys[i]]=values[i];
    }
    GeneralFunc::deleteFolderOutOfRange(m_ReportPath,m_IntParameter[QString::fromLocal8Bit("保存天数")]);

    readTypeInfo();
}

void Pro_SiliconeTube::slot_press_point(QString cam,QPoint point, QSize size)
{
    if(cam!=m_CamName)
        return;
    try
    {
        if(QtHalcon::testHObjectEmpty(m_SceneImage))return;
        //计算目标处的灰度值，并显示到状态栏标签
        HTuple grayValue,pos,grayValue_H;
        QtHalcon::getMousePosGray(m_SceneImage,point,size,&pos,&grayValue);
        if(grayValue.TupleLength()==1)
        {
            int r_value;
            r_value=grayValue.I();
            m_runInfo=QString("(%1,%2) RGB:%3,%4,%5").arg(QString::number(pos[0].I()),QString::number(pos[1].I()),
                    QString::number(r_value),QString::number(r_value),
                    QString::number(r_value));
            emit  signal_information_text(m_CamName,2,m_runInfo);
        }
        else if(grayValue.TupleLength()==3)
        {
            int r_value,g_value,b_value;
            r_value=grayValue[0].I();
            g_value=grayValue[1].I();
            b_value=grayValue[2].I();
            m_runInfo=QString("(%1,%2) RGB:%3,%4,%5").arg(QString::number(pos[0].I()),QString::number(pos[1].I()),
                    QString::number(r_value),QString::number(g_value),QString::number(b_value));
            emit  signal_information_text(m_CamName,2,m_runInfo);
        }
    }
    catch (cv::Exception & e)
    {

    }
}
void Pro_SiliconeTube::slot_one_frame(QImage src,QString camname,bool ischeck)
{
    if(camname!=m_CamName)return;
    if(src.isNull()) return;
    QtHalcon::qImage2HImage(src,m_OriginImage);
    CopyImage(m_OriginImage,&m_SceneImage);
    CopyImage(m_OriginImage,&m_SceneImageShow);
    if(ischeck)
    {
        slot_check(m_CamName);
    }
    else
    {
        emit signal_information_image(m_CamName,0,src);
    }
}

void Pro_SiliconeTube::slot_check(QString camName)
{
    if(camName!=m_CamName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
    v_obj_result.clear();
    v_obj_bool.clear();
    m_send_data.clear();
    v_result_image.clear();
    QMap<QString,QString>::iterator iter=map_obj.begin();
    int count=-1;
    while(iter!=map_obj.end())
    {
        count++;
        QString key=iter.key();
        QString value=iter.value();
        try
        {
            QStringList name_type_list=key.split('@');
            if(name_type_list.size()<1)continue;
            HObject roi;
            QtHalcon::getRoi(name_type_list[1],value,&roi);
            HObject ho_Rectangle,ho_ImagePart;
            getCheckROI (m_SceneImage,roi, &ho_Rectangle,&ho_ImagePart,
                         m_FloatParameter[QString::fromLocal8Bit("最小阈值")]);
            v_result_image<<ho_ImagePart;
            v_obj_result<<ho_Rectangle;

            //checkFunc
            cv::Mat check_img=QtHalcon::HObjectToMat(ho_ImagePart);
            if(check_img.empty())return;
            emit signal_setCheckPar(count,check_img,m_IntParameter[QString::fromLocal8Bit("图像大小")],
                    m_IntParameter[QString::fromLocal8Bit("裁剪大小")]);
            v_obj_bool<<1;
            emit signal_check_image();
        }
        catch (cv::Exception &e)
        {
            emit  signal_information_text(m_CamName,1,"cv"+QString(e.what()));
        }
        catch (HalconCpp::HException &e)
        {
            emit  signal_information_text(m_CamName,1,"halcon"+QString(e.ErrorMessage()));
        }
        iter++;
    }
    /*if(v_result_image.size()<1)
    {
        m_send_data<<0<<2;
        check_err=true;
        slot_check_result(0,QString());
    }*/
}
void Pro_SiliconeTube::slot_check_result(int num,QString name)
{

    v_obj_bool[num]=name.toInt();
    check_count++;
    if(check_count!=v_obj_result.size())return;
    check_count=0;
    if(v_obj_bool.contains(1))
    {
        ng_num=0;
    }
    else if(v_obj_bool.contains(2))
    {
        ng_num++;
    }
    m_send_data<<name.toInt();
    //m_send_data<<ng_num;

    //m_send_data.push_front(0);
    send_result();
    time_end = double(clock());
    m_runInfo=tr(QString::fromLocal8Bit("%1检测时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);

    QStringList result;
    for (int i=0;i<v_obj_bool.size();i++)
    {
        result<<QString::number(v_obj_bool[i]);
    }
    m_runInfo=tr(QString::fromLocal8Bit("%1检测结果:%2").arg(m_CamName,result.join("|")).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);

    time_start = double(clock());
    writeCheckInfoToImage();
    time_end = double(clock());
    m_runInfo=tr(QString::fromLocal8Bit("%1绘制时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);

    time_start = double(clock());
    writeImageAndReport();
    time_end = double(clock());
    m_runInfo=tr(QString::fromLocal8Bit("%1保存时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);

    check_err=false;
}
void Pro_SiliconeTube::slot_read_image(QString cam,QString imgPath)
{

    if(cam==m_CamName)
    {
        HalconCpp::ReadImage(&m_OriginImage,imgPath.toLocal8Bit().data());
        if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
        QImage image;
        QtHalcon::hImage2QImage(m_OriginImage,image);
        CopyImage(m_OriginImage,&m_SceneImage);
        CopyImage(m_OriginImage,&m_SceneImageShow);
        emit signal_information_image(m_CamName,0,image);
    }
}

void Pro_SiliconeTube::writeImageAndReport()//写入图片和记录
{

    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd")+"\\"+QString::number(v_obj_bool[0]);
    if(m_IntParameter[QString::fromLocal8Bit("保存JPG")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_SceneImageShow,"jpg",timePath.toLocal8Bit().data());
    }
    if(m_IntParameter[QString::fromLocal8Bit("保存PNG")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".png";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_SceneImage,"png",timePath.toLocal8Bit().data());
    }
}

void Pro_SiliconeTube::writeCheckInfoToImage()//写检测结果到图片
{
    try
    {
        QtHalcon::displayHObject(m_SceneImage,hv_WindowHandle_UI);
        SetDraw(hv_WindowHandle_UI,"margin");
        int marginwidth=8;
        SetLineWidth(hv_WindowHandle_UI,marginwidth);
        for(int i=0;i<v_obj_result.size();i++)
        {
            QString roistr=QtHalcon::getRoiStr("Rectangle1",v_obj_result[i]);
            QStringList roistrlist=roistr.split("|");
            std::string color_str;
            if(v_obj_bool[i]==1)
            {
                color_str="green";
            }
            else
            {
                color_str="red";
            }
            SetColor(hv_WindowHandle_UI,color_str.data());
            DispObj(v_obj_result[i],hv_WindowHandle_UI);
            HTuple width,height;
            HalconCpp::GetImageSize(m_SceneImage,&width,&height);
            QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/15).toStdString().data(),QString::number(width.I()/35).toStdString().data());
            QtHalcon::writeWindowString(hv_WindowHandle_UI,QString::number(v_obj_bool[i]).toStdString().data(),
                                        color_str.data(),roistrlist[0].toInt()+marginwidth,roistrlist[1].toInt()+marginwidth);

        }

        DumpWindowImage(&m_SceneImageShow,hv_WindowHandle_UI);
        QImage image;
        QtHalcon::hImage2QImage(m_SceneImageShow,image);
        ClearWindow(hv_WindowHandle_UI);
        emit signal_information_image(m_CamName,v_obj_bool[0],image);
        /*if(v_obj_bool.contains(1))
        {
            emit signal_information_image(m_CamName,1,image);
        }
        else
        {
            emit signal_information_image(m_CamName,2,image);
        }*/
    }
    catch (HalconCpp::HException & ecp)
    {

    }
}

void Pro_SiliconeTube::send_result()
{
    emit signal_result(m_send_data,m_CamName);
}
void Pro_SiliconeTube::slot_roi_change(QString cam)
{
    if(cam==m_CamName)
    {
        readROI();
    }
}

void Pro_SiliconeTube::slot_type_change(QString type)
{
    initGlobalPar(m_ProName,m_StationName,m_CamName,type);
}

void Pro_SiliconeTube::slot_par_change(QString cam)
{
    if(cam==m_CamName)
    {
        readPar();
    }
}
void Pro_SiliconeTube::slot_program_exit()
{
    if(map_Thread.size()<1)return;
    QMap<int,QThread*>::iterator iter=map_Thread.begin();
    while (iter!=map_Thread.end())
    {
        if(map_Thread[iter.key()]!=nullptr)
        {
            map_Thread[iter.key()]->exit(0);
            map_Thread[iter.key()]->wait();
            delete map_Thread[iter.key()];
            map_Thread[iter.key()]=nullptr;

        }
        iter++;
    }

}

void Pro_SiliconeTube::getCheckROI (HObject ho_Image1, HObject ho_ROI_0, HObject *ho_Rectangle1, HObject *ho_ImagePart,
                                    HTuple hv_min_thr)
{

    // Local iconic variables
    HObject  ho_ImageReduced1, ho_Region, ho_RegionOpening;
    HObject   ho_ImageReduced;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2;

    ReduceDomain(ho_Image1, ho_ROI_0, &ho_ImageReduced1);
    Threshold(ho_ImageReduced1, &ho_Region, hv_min_thr, 255);
    OpeningCircle(ho_Region, &ho_RegionOpening, 6.0);
    SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    GenRectangle1(&(*ho_Rectangle1), hv_Row1-15, hv_Column1-10, hv_Row1+70, hv_Column1+120);
    ReduceDomain(ho_Image1, *ho_Rectangle1, &ho_ImageReduced);
    CropDomain(ho_ImageReduced, &(*ho_ImagePart));
    return;
}
