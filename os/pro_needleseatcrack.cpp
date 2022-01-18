
#include"deeplearning/TargetDetection/detector.h"
#include "pro_needleseatcrack.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
#include"deeplearning/TargetDetection/yolov5openvino.h"

Pro_NeedleSeatCrack::Pro_NeedleSeatCrack(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    check_err=false;
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    //HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_NeedleSeatCrack::~Pro_NeedleSeatCrack()
{
    slot_program_exit();
}
//初始化参数
void Pro_NeedleSeatCrack::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
{
    m_ProName=pro;
    m_ProPath=m_ExePath+"\\Pro\\"+m_ProName;
    m_StationName=station;
    m_station_path=m_ProPath+"\\station\\"+m_StationName; //工位路径
    m_CamName=cam;
    m_CamPath=m_station_path+"\\"+m_CamName;
    m_StepName=step;
    m_StepPath=m_CamPath+"\\"+m_StepName;
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
    //readTypeInfo();
    openHalconWindow();
}
void Pro_NeedleSeatCrack::openHalconWindow()
{
    QStringList sizelist=m_cam_info.resolution.split(':');
    //打开一个隐藏得窗口用于绘图
    HTuple len;
    TupleLength(hv_WindowHandle_UI,&len);
    if(len>0)
    {
        CloseWindow(hv_WindowHandle_UI);
    }
        OpenWindow(0,0,sizelist[0].toInt(),sizelist[1].toInt(),0,"invisible","",&hv_WindowHandle_UI);
}
//根据启用的品类，读取选区信息
void Pro_NeedleSeatCrack::readROI()
{
    QString roiPath=m_TypePath+"\\roi";
    map_hobj.clear();
    QStringList  roilist=GeneralFunc::getFileListName(roiPath,QStringList()<<"*.*");;
    for (int i=0;i<roilist.size();i++)
    {
        QStringList roi_type=roilist[i].split('#');
        if(roi_type.size()<2)continue;
        if(roi_type[0].mid(0,8)=="modelroi")
        {
            XmlOperate m_xml;
            m_xml.openXml(m_TypePath+"\\ROIConfig.xml");
            map_model_par[roi_type[0]].angle_Start=m_xml.readNode(QStringList()<<roi_type[0]<<"Angle_Start").toDouble();
            map_model_par[roi_type[0]].min_Score=m_xml.readNode(QStringList()<<roi_type[0]<<"Min_Score").toDouble();
            map_model_par[roi_type[0]].numToFind=m_xml.readNode(QStringList()<<roi_type[0]<<"NumToFind").toInt();
            map_model_par[roi_type[0]].max_Overlap=m_xml.readNode(QStringList()<<roi_type[0]<<"Max_Overlap").toDouble();
            map_model_par[roi_type[0]].sub_Pixel=m_xml.readNode(QStringList()<<roi_type[0]<<"Sub_Pixel").toLocal8Bit().data();
            map_model_par[roi_type[0]].num_Levels=QtHalcon::qstringToHtuple(m_xml.readNode(QStringList()<<roi_type[0]<<"Num_Levels"),',',"int");
            map_model_par[roi_type[0]].greediness=m_xml.readNode(QStringList()<<roi_type[0]<<"Greediness").toDouble();
            if(GeneralFunc::isFileExist(QString(m_TypePath+"\\"+roi_type[0]+".shm"),0))
            {
                HalconCpp::ReadShapeModel(QString(m_TypePath+"\\"+roi_type[0]+".shm").toLocal8Bit().data(),&map_model_par[roi_type[0]].modelID);
            }
            map_model_par[roi_type[0]].row_M=m_xml.readNode(QStringList()<<roi_type[0]<<"Row_M").toDouble();//模板位置
            map_model_par[roi_type[0]].col_M=m_xml.readNode(QStringList()<<roi_type[0]<<"Col_M").toDouble();
            map_model_par[roi_type[0]].ang_M=m_xml.readNode(QStringList()<<roi_type[0]<<"Ang_M").toDouble();
            map_model_par[roi_type[0]].movementOfObject_Model_M=QtHalcon::qstringToHtuple(m_xml.readNode(QStringList()<<roi_type[0]<<"MovementOfObject_Model_M"),',',"double");//模板变换矩阵
            m_xml.closeXml();
            continue;
        }
        switch (EnumChange::string2enum_roitype(roi_type[1]))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            if(GeneralFunc::isFileExist(QString(roiPath+"\\"+roilist[i]+".hobj"),0))
            {
                ReadObject(&map_hobj[roilist[i]],QString(roiPath+"\\"+roilist[i]+".hobj").toLocal8Bit().data());
            }
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            if(GeneralFunc::isFileExist(QString(roiPath+"\\"+roilist[i]),0))
            {
                ReadObject(&map_hobj[roilist[i]],QString(roiPath+"\\"+roilist[i]).toLocal8Bit().data());
            }
            break;
        }


    }
}

//读取品种文件
void Pro_NeedleSeatCrack::readTypeInfo()
{
    //slot_program_exit();

    map_Thread.clear();
    if(m_StringParameter[QString::fromLocal8Bit("处理器")]=="GPU")
    {
        map_detector.clear();
        m_weightfile= m_TypePath+"\\best.torchscript_cuda.pt";//权重文件
        m_namesfile= m_TypePath+"\\voc.names";//权重文件
        std::string weights=m_weightfile.toStdString();
        std::string name_file =m_namesfile.toStdString();
        if(!GeneralFunc::isFileExist(m_weightfile,false)||!GeneralFunc::isFileExist(m_namesfile,false))
        {
            emit  signal_information_text(m_CamName,1,QString::fromLocal8Bit("不存在权重文件"));
        }
        else
        {
            for (int i=0;i<1;i++)
            {
                map_detector[i]=new Detector(nullptr,i,weights,name_file);
                map_Thread[i]=new QThread(this);
                map_detector[i]->moveToThread(map_Thread[i]);
                connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
                connect(map_Thread[i],&QThread::finished,map_detector[i],&QObject::deleteLater);
                connect(this,&Pro_NeedleSeatCrack::signal_check_image,map_detector[i],&Detector::slot_run);
                connect(this,&Pro_NeedleSeatCrack::signal_setCheckPar,map_detector[i],&Detector::slot_setCheckPar);
                connect(map_detector[i],&Detector::signal_result,this,&Pro_NeedleSeatCrack::slot_check_result);
                map_Thread[i]->start();
            }
        }
    }
    else if(m_StringParameter[QString::fromLocal8Bit("处理器")]=="CPU")
    {
        map_openvino.clear();
        m_weightfile= m_TypePath+"\\best.onnx";//权重文件
        m_namesfile= m_TypePath+"\\voc.names";//权重文件
        std::string weights=m_weightfile.toStdString();
        std::string name_file =m_namesfile.toStdString();
        if(!GeneralFunc::isFileExist(m_weightfile,false)||!GeneralFunc::isFileExist(m_namesfile,false))
        {
            emit  signal_information_text(m_CamName,1,QString::fromLocal8Bit("不存在权重文件"));
        }
        else
        {
            for (int i=0;i<1;i++)
            {
                map_openvino[i]=new yolov5openvino(nullptr,i,"CPU",weights,name_file);
                map_Thread[i]=new QThread(this);
                map_openvino[i]->moveToThread(map_Thread[i]);
                connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
                connect(map_Thread[i],&QThread::finished,map_openvino[i],&QObject::deleteLater);
                connect(this,&Pro_NeedleSeatCrack::signal_check_image,map_openvino[i],&yolov5openvino::slot_run);
                connect(this,&Pro_NeedleSeatCrack::signal_setCheckPar,map_openvino[i],&yolov5openvino::slot_setCheckPar);
                connect(map_openvino[i],&yolov5openvino::signal_result,this,&Pro_NeedleSeatCrack::slot_check_result);
                map_Thread[i]->start();
            }
        }
    }

}
//根据启用的品类读取参数信息
void Pro_NeedleSeatCrack::readPar()
{
    XmlOperate m_xml_par;
    m_xml_par.openXml(m_TypePath+"\\ParConfig.xml");
    QStringList parTypeList=QStringList()<<"IntPar"<<"FloatPar"<<"StringPar";
    QStringList parlist=m_xml_par.getChild(QStringList()<<parTypeList[0]);
    for (int i=0;i<parlist.size();i++)
    {
        m_IntParameter[parlist[i]]=m_xml_par.readNode(QStringList()<<parTypeList[0]<<parlist[i]).toInt();
    }
    parlist=m_xml_par.getChild(QStringList()<<parTypeList[1]);
    for (int i=0;i<parlist.size();i++)
    {
        m_FloatParameter[parlist[i]]=m_xml_par.readNode(QStringList()<<parTypeList[1]<<parlist[i]).toFloat();
    }
    parlist=m_xml_par.getChild(QStringList()<<parTypeList[2]);
    for (int i=0;i<parlist.size();i++)
    {
        m_StringParameter[parlist[i]]=m_xml_par.readNode(QStringList()<<parTypeList[2]<<parlist[i]);
    }
    m_xml_par.closeXml();

    //读取相机配置
    QString xmlPath=m_ProPath+"\\camera\\CamConfig.xml";
    XmlOperate m_xml;
    m_xml.openXml(xmlPath);
    m_cam_info.resolution=m_xml.readNode(QStringList()<<m_CamName<<"Resolution");
    QStringList sizelist=m_cam_info.resolution.split(':');
    if(sizelist.size()!=2)
    {
        m_cam_info.resolution="2448:2048";
        sizelist=m_cam_info.resolution.split(':');
    }
    int image_width=sizelist[0].toInt();
    int image_height=sizelist[1].toInt();
    if(m_IntParameter[QString::fromLocal8Bit("校正图像")]==1)
    {
        m_cam_info.map_step_cal[m_StepName].isCal=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"IsCal");
        if(m_cam_info.map_step_cal[m_StepName].isCal=="1")
        {
            m_cam_info.map_step_cal[m_StepName].ratio=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"Ratio");
            m_cam_info.map_step_cal[m_StepName].camPar=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"CamPar");
            m_cam_info.map_step_cal[m_StepName].camPose=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"CamPose");
            m_cam_info.map_step_cal[m_StepName].X_Offset=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"X_Offset");
            m_cam_info.map_step_cal[m_StepName].Y_Offset=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"Y_Offset");
            m_cam_info.map_step_cal[m_StepName].worldWidth=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"WorldWidth");
            m_cam_info.map_step_cal[m_StepName].worldHeight=m_xml.readNode(QStringList()<<m_CamName<<m_StepName<<"WorldHeight");
            HTuple hv_cam_par=QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[m_StepName].camPar,',',"double");
            HTuple  hv_cam_pose=QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[m_StepName].camPose,',',"double");
            QtHalcon::calibrateImage(&m_MapFix, image_width, image_height,
                                     m_cam_info.map_step_cal[m_StepName].worldWidth.toDouble(),m_cam_info.map_step_cal[m_StepName].worldHeight.toDouble(),
                                     hv_cam_par,hv_cam_pose,
                                     m_cam_info.map_step_cal[m_StepName].ratio.toDouble(),
                                     m_cam_info.map_step_cal[m_StepName].X_Offset.toDouble(), m_cam_info.map_step_cal[m_StepName].Y_Offset.toDouble());
        }
    }
    m_xml.closeXml();
    //删除超出保存日期的记录
    GeneralFunc::deleteFolderOutOfRange(m_ReportPath,m_IntParameter[QString::fromLocal8Bit("保存天数")]);

    readTypeInfo();
}

//计算点击处的像素信息
void Pro_NeedleSeatCrack::slot_press_point(QString cam,QPoint point, QSize size)
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
//接收一帧
void Pro_NeedleSeatCrack::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
{
    if(camname!=m_CamName||step!=m_StepName)return;
    if(src.isNull()) return;
    QtHalcon::qImage2HImage(src,m_OriginImage);
    CopyImage(m_OriginImage,&m_SceneImage);
    //CopyImage(m_OriginImage,&m_SceneImageShow);
    if(ischeck)
    {
        slot_check(m_CamName,step);
    }
    else
    {
        emit signal_information_image(m_CamName,0,src);
    }
}

//检测
void Pro_NeedleSeatCrack::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;
    time_start = double(clock());//开始计时
    v_obj_result.clear();
    v_obj_bool.clear();
    m_send_data.clear();
    v_result_image.clear();
    QMap<QString,HObject>::iterator iter=map_hobj.begin();

    while(iter!=map_hobj.end())
    {
        QString key=iter.key();
        HObject value=iter.value();
        try
        {
            QStringList name_type_list=key.split('#');
            if(name_type_list.size()<1)continue;
            HalconCpp::HObject ho_ObjectsConcatPart,ho_ObjectsConcatRegion;
            HTuple hv_Number;
            HObject image_reduce,rect;
            getCheckObj ( m_SceneImage, value, &ho_ObjectsConcatPart, &ho_ObjectsConcatRegion,
                          m_FloatParameter[QString::fromLocal8Bit("背景阈值低")], m_FloatParameter[QString::fromLocal8Bit("背景闭运算尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("阈值差值")], m_FloatParameter[QString::fromLocal8Bit("针座闭运算尺寸水平")],
                    m_FloatParameter[QString::fromLocal8Bit("针座闭运算尺寸垂直")],m_FloatParameter[QString::fromLocal8Bit("实体筛选宽度")],
                    m_FloatParameter[QString::fromLocal8Bit("实体筛选高度")],m_FloatParameter[QString::fromLocal8Bit("物体高度")],
                    &hv_Number);

            int count=hv_Number.I();
            //#pragma omp parallel for num_threads(8)
            for (int i=0;i<count;i++)
            {
                HObject image_part,image_region;
                HalconCpp::SelectObj(ho_ObjectsConcatPart,&image_part,i+1);

                cv::Mat check_img=QtHalcon::HObjectToMat(image_part);

                emit signal_setCheckPar(i,check_img,m_IntParameter[QString::fromLocal8Bit("图像大小")],
                        m_FloatParameter[QString::fromLocal8Bit("置信度")],m_FloatParameter[QString::fromLocal8Bit("冗余度")]);
                HalconCpp::SelectObj(ho_ObjectsConcatRegion,&image_region,i+1);
                v_result_image<<check_img;
                v_obj_result<<image_region;
                v_obj_bool<<1;
                emit signal_check_image();
            }
        }
        catch (cv::Exception &e)
        {
            emit  signal_information_text(m_CamName,1,"cv");
        }
        catch (HalconCpp::HException &e)
        {
            emit  signal_information_text(m_CamName,1,"halcon");
        }
        iter++;
    }
    /*if(v_result_image.size()<1)
    {
        m_send_data<<2<<0;
        check_err=true;
        slot_check_result(0,cv::Mat(),QStringList());
    }*/
}
void Pro_NeedleSeatCrack::slot_check_result(int num,cv::Mat img_result,QStringList name,QList<cv::Rect> rect)
{
    if(!check_err)
    {
        if(name.size()>0)v_obj_bool[num]=2;
        v_result_image[num]=img_result;
        check_count++;
        if(check_count!=v_obj_result.size())return;
        check_count=0;
        if(v_obj_bool.contains(2))
        {
            m_send_data<<2;
        }
        else
        {
            m_send_data<<1;
        }
    }

    send_result();
    time_end = double(clock());
    m_runInfo=tr(QString::fromLocal8Bit("%1检测时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
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
//读取文件
int image_Index;

void Pro_NeedleSeatCrack::slot_read_image(QString cam,QString step,QString imgPath)
{
    if(cam==m_CamName&&step==m_StepName)
    {
        QFileInfo FileInfo(imgPath);
        if(FileInfo.isFile())
        {
            HalconCpp::ReadImage(&m_OriginImage,imgPath.toLocal8Bit().data());
            if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
            QImage image;
            QtHalcon::hImage2QImage(m_OriginImage,image);
            CopyImage(m_OriginImage,&m_SceneImage);
            emit signal_information_image(m_CamName,0,image);
        }
        else if(FileInfo.isDir())
        {
            QStringList fileList=GeneralFunc::getFileListPath(imgPath,QStringList()<<"*.jpg"<<"*.bmp"<<"*.png");
            image_Index=0;
            QTimer *timer=new QTimer(this);
            connect(timer,&QTimer::timeout,this,[=]()
            {
                HalconCpp::ReadImage(&m_OriginImage,fileList[image_Index].toLocal8Bit().data());
                if(!QtHalcon::testHObjectEmpty(m_OriginImage))
                {
                    CopyImage(m_OriginImage,&m_SceneImage);
                    slot_check(m_CamName,m_StepName);
                    image_Index++;
                }
            });
            timer->start(500);

        }
    }
}

//写入记录
void Pro_NeedleSeatCrack::writeImageAndReport()//写入图片和记录
{
    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd");
    if(m_IntParameter[QString::fromLocal8Bit("保存检测图像")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        cv::imwrite(timePath.toLocal8Bit().data(),m_SceneImageShow);
        //QtHalcon::saveHObject(m_SceneImageShow,"jpg",timePath.toLocal8Bit().data());
    }
    if(m_IntParameter[QString::fromLocal8Bit("保存原始图像")]==1)
    {
        QString timePath=dateDir+"\\origin"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        //cv::imwrite(timePath.toLocal8Bit().data(),m_OriginImage);
        QtHalcon::saveHObject(m_OriginImage,"jpg",timePath.toLocal8Bit().data());
    }
}

//写或绘制检测信息到虚拟窗口
void Pro_NeedleSeatCrack::writeCheckInfoToImage()//写检测结果到图片
{
    try
    {
        if(m_IntParameter[QString::fromLocal8Bit("绘制边缘宽度")]<1)
        {
            emit  signal_information_text(m_CamName,1,QString::fromLocal8Bit("绘制图像宽度未设置"));
            QImage image;
            QtHalcon::hImage2QImage(m_OriginImage,image);
            emit signal_information_image(m_CamName,v_obj_bool[0],image);
            return;
        }
        //opencv方法70ms
        cv::Mat sec_mat=QtHalcon::HObjectToMat(m_SceneImage);
        cv::cvtColor(sec_mat,sec_mat,cv::COLOR_GRAY2BGR);
        //#pragma omp parallel for num_threads(8)
        for(int i=0;i<v_result_image.size();i++)
        {
            try
            {
                QString roistr=QtHalcon::getRoiStr("Rectangle1",v_obj_result[i]);
                //std::cout<<roistr.toStdString()<<std::endl;
                QStringList roistrlist=roistr.split("|");
                cv::Rect roi_rect = cv::Rect(roistrlist[1].toInt(),roistrlist[0].toInt(),
                        v_result_image[i].cols,v_result_image[i].rows);
                //#pragma omp atomic
                cv::Mat copy;
                cv::resize(v_result_image[i],copy,cv::Size(roi_rect.width,roi_rect.height));
                copy.copyTo(sec_mat(roi_rect));
                if(v_obj_bool[i]==1)
                {
                    cv::rectangle(sec_mat, roi_rect, cv::Scalar(0, 255, 0),3, cv::LINE_8,0);
                }
                else
                {
                    cv::rectangle(sec_mat, roi_rect, cv::Scalar(0, 0, 255),3, cv::LINE_8,0);
                }
                //std::cout<<v_result_image[i].rows<<"::"<<v_result_image[i].cols<<std::endl;

            }
            catch (cv::Exception &e)
            {
                continue;
            }
        }
        QImage image=QtOpencv::cvMat2QImage(sec_mat);
        m_SceneImageShow=sec_mat.clone();
        if(v_obj_bool.contains(2))
        {
            emit signal_information_image(m_CamName,2,image);
        }
        else
        {
            emit signal_information_image(m_CamName,1,image);
        }
    }
    catch (HalconCpp::HException & ecp)
    {
    }

}

//发送结果
void Pro_NeedleSeatCrack::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_NeedleSeatCrack::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_NeedleSeatCrack::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_NeedleSeatCrack::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_NeedleSeatCrack::slot_program_exit()
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

void Pro_NeedleSeatCrack::getCheckObj (HObject ho_Image, HObject ho_ROI, HObject *ho_ObjectsConcat, HObject *ho_ObjectsConcat1,
                                       HTuple hv_back_thr_min, HTuple hv_back_close_size, HTuple hv_back_thr_dif, HTuple hv_seat_close_size_h,
                                       HTuple hv_seat_close_size_v, HTuple hv_seat_select_width_min, HTuple hv_seat_select_height_min,
                                       HTuple hv_obj_height, HTuple *hv_Number)
{

    // Local iconic variables
    HObject  ho_ImageReduced3, ho_Region3, ho_RegionClosing1;
    HObject  ho_RegionErosion, ho_ImageReduced1, ho_Region1;
    HObject  ho_ConnectedRegions1, ho_SelectedRegions1, ho_RegionUnion1;
    HObject  ho_RegionClosing2, ho_ConnectedRegions2, ho_SelectedRegions4;
    HObject  ho_SelectedRegions5, ho_ObjectSelected1, ho_Rectangle;
    HObject  ho_ImageReduced, ho_Region, ho_ConnectedRegions;
    HObject  ho_SelectedRegions, ho_RegionUnion, ho_RegionClosing;
    HObject  ho_RegionOpening, ho_Rectangle1, ho_ImageReduced2;
    HObject  ho_ImagePart;

    // Local control variables
    HTuple  hv_Index1, hv_Row12, hv_Column12, hv_Row22;
    HTuple  hv_Column22, hv_Row1, hv_Column1, hv_Row2, hv_Column2;
    HTuple  hv_Row11, hv_Column11, hv_Row21, hv_Column21;


    ReduceDomain(ho_Image, ho_ROI, &ho_ImageReduced3);
    Threshold(ho_ImageReduced3, &ho_Region3, hv_back_thr_min, 255);
    ClosingRectangle1(ho_Region3, &ho_RegionClosing1, hv_back_close_size, hv_back_close_size);
    ErosionRectangle1(ho_RegionClosing1, &ho_RegionErosion, 20, 20);
    ReduceDomain(ho_ImageReduced3, ho_RegionErosion, &ho_ImageReduced1);
    VarThreshold(ho_ImageReduced1, &ho_Region1, 15, 15, 0.2, hv_back_thr_dif, "dark");
    Connection(ho_Region1, &ho_ConnectedRegions1);
    SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions1, "area", "and", 150, 99999);
    Union1(ho_SelectedRegions1, &ho_RegionUnion1);
    ClosingRectangle1(ho_RegionUnion1, &ho_RegionClosing2, hv_seat_close_size_h, hv_seat_close_size_v);
    Connection(ho_RegionClosing2, &ho_ConnectedRegions2);
    SelectShape(ho_ConnectedRegions2, &ho_SelectedRegions4, "width", "and", hv_seat_select_width_min,
                99999);
    SelectShape(ho_SelectedRegions4, &ho_SelectedRegions5, "height", "and", hv_seat_select_height_min,
                99999);
    CountObj(ho_SelectedRegions5, &(*hv_Number));
    GenEmptyObj(&(*ho_ObjectsConcat));
    GenEmptyObj(&(*ho_ObjectsConcat1));
    {
        HTuple end_val17 = (*hv_Number);
        HTuple step_val17 = 1;
        for (hv_Index1=1; hv_Index1.Continue(end_val17, step_val17); hv_Index1 += step_val17)
        {
            SelectObj(ho_SelectedRegions5, &ho_ObjectSelected1, hv_Index1);
            SmallestRectangle1(ho_ObjectSelected1, &hv_Row12, &hv_Column12, &hv_Row22, &hv_Column22);
            GenRectangle1(&ho_Rectangle, hv_Row12, hv_Column12, hv_Row22-200, hv_Column22);
            ReduceDomain(ho_ImageReduced1, ho_Rectangle, &ho_ImageReduced);
            LocalThreshold(ho_ImageReduced, &ho_Region, "adapted_std_deviation", "dark",
                           HTuple(), HTuple());
            Connection(ho_Region, &ho_ConnectedRegions);
            SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", 150, 99999);
            Union1(ho_SelectedRegions, &ho_RegionUnion);
            ClosingRectangle1(ho_RegionUnion, &ho_RegionClosing, (hv_Column22-hv_Column12)/6,
                              50);
            OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening, (hv_Column22-hv_Column12)/5,
                              100);
            SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
            if (0 != (hv_Row1<hv_Row12))
            {
                hv_Row1 = hv_Row12+1;
            }
            GenRectangle1(&ho_Rectangle1, hv_Row12, hv_Column12, hv_Row1, hv_Column22);
            SmallestRectangle1(ho_Rectangle1, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
            if (0 != (HTuple((hv_Row21-hv_Row11)<700).TupleOr((hv_Row21-hv_Row11)>1100)))
            {
                GenRectangle1(&ho_Rectangle1, hv_Row12, hv_Column12, hv_Row12+hv_obj_height,
                              hv_Column22);
            }
            ReduceDomain(ho_ImageReduced, ho_Rectangle1, &ho_ImageReduced2);
            CropDomain(ho_ImageReduced2, &ho_ImagePart);
            ConcatObj(ho_ImagePart, (*ho_ObjectsConcat), &(*ho_ObjectsConcat));
            ConcatObj(ho_Rectangle1, (*ho_ObjectsConcat1), &(*ho_ObjectsConcat1));
        }
    }
    CountObj((*ho_ObjectsConcat1), &(*hv_Number));
    return;
}


