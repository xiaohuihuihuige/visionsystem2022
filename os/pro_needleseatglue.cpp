
#include"deeplearning/Classified/classified.h"
#include "pro_needleseatglue.h"
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

Pro_NeedleSeatGlue::Pro_NeedleSeatGlue(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    check_err=false;
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_NeedleSeatGlue::~Pro_NeedleSeatGlue()
{
    slot_program_exit();
}
//初始化参数
void Pro_NeedleSeatGlue::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_NeedleSeatGlue::openHalconWindow()
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
void Pro_NeedleSeatGlue::readROI()
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
void Pro_NeedleSeatGlue::readTypeInfo()
{
    //slot_program_exit();

    map_Thread.clear();
    if(m_StringParameter[QString::fromLocal8Bit("处理器")]=="GPU")
    {
        map_Classified.clear();
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
            for (int i=0;i<map_hobj.size();i++)
            {
                map_Classified[i]=new Classified(nullptr,i,weights,name_file);
                map_Thread[i]=new QThread(this);
                map_Classified[i]->moveToThread(map_Thread[i]);
                connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
                connect(map_Thread[i],&QThread::finished,map_Classified[i],&QObject::deleteLater);
                connect(this,&Pro_NeedleSeatGlue::signal_check_image,map_Classified[i],&Classified::slot_run);
                connect(this,&Pro_NeedleSeatGlue::signal_setCheckPar,map_Classified[i],&Classified::slot_setCheckPar);
                connect(map_Classified[i],&Classified::signal_result,this,&Pro_NeedleSeatGlue::slot_check_result);
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
            for (int i=0;i<map_hobj.size();i++)
            {

                map_openvino[i]=new classfiedopenvino(nullptr,i,"CPU",weights,name_file);
                map_Thread[i]=new QThread(this);
                map_openvino[i]->moveToThread(map_Thread[i]);
                connect(map_Thread[i],&QThread::finished,map_Thread[i],&QObject::deleteLater);
                connect(map_Thread[i],&QThread::finished,map_openvino[i],&QObject::deleteLater);
                connect(this,&Pro_NeedleSeatGlue::signal_check_image,map_openvino[i],&classfiedopenvino::slot_run);
                connect(this,&Pro_NeedleSeatGlue::signal_setCheckPar,map_openvino[i],&classfiedopenvino::slot_setCheckPar);
                connect(map_openvino[i],&classfiedopenvino::signal_result,this,&Pro_NeedleSeatGlue::slot_check_result);
                map_Thread[i]->start();
            }
        }
    }
}
//根据启用的品类读取参数信息
void Pro_NeedleSeatGlue::readPar()
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
void Pro_NeedleSeatGlue::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_NeedleSeatGlue::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
{
    if(camname!=m_CamName||step!=m_StepName)return;
    if(src.isNull()) return;
    QtHalcon::qImage2HImage(src,m_OriginImage);
    CopyImage(m_OriginImage,&m_SceneImage);
    CopyImage(m_OriginImage,&m_SceneImageShow);
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
void Pro_NeedleSeatGlue::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
    v_obj_result.clear();
    v_obj_bool.clear();
    v_str_result.clear();
    v_str_bool.clear();
    v_contour_result.clear();
    v_contour_bool.clear();
    m_send_data.clear();
    QMap<QString,HObject>::iterator iter=map_hobj.begin();
    int count=-1;
    while(iter!=map_hobj.end())
    {
        count++;
        QString key=iter.key();
        HObject value=iter.value();
        try
        {
            QStringList name_type_list=key.split('#');
            if(name_type_list.size()<1)continue;
            HalconCpp::HObject image_part;
            HObject image_reduce,rect;
            getCheckRegion(m_SceneImage,value,&rect,&image_part,m_FloatParameter[QString::fromLocal8Bit("阈值差值")],
                    m_IntParameter[QString::fromLocal8Bit("筛选面积小")],
                    m_FloatParameter[QString::fromLocal8Bit("针座尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("针尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("选区尺寸")]);
            v_obj_result<<rect;
            v_obj_bool.push_back(0);
            //checkFunc
            cv::Mat check_img=QtHalcon::HObjectToMat(image_part);
            emit signal_setCheckPar(count,check_img,m_IntParameter[QString::fromLocal8Bit("图像大小")]);

            emit signal_check_image();

        }
        catch (cv::Exception &e)
        {
            emit  signal_information_text(m_CamName,1,"cv");
            continue;
        }
        catch (HalconCpp::HException &e)
        {
            emit  signal_information_text(m_CamName,1,"halcon");
            continue;
        }
        iter++;
    }
    /*if(v_result_image.size()<1)
    {
        m_send_data<<2<<0;
        check_err=true;
        slot_check_result(0,QString());
    }*/
}

void Pro_NeedleSeatGlue::slot_check_result(int num,QString name)
{
    v_obj_bool[num]=name.toInt();
    check_count++;
    bool isneedleright=true;
    if(v_obj_bool[num]==1||(m_IntParameter[QString::fromLocal8Bit("无胶检测角度")]==1&&v_obj_bool[num]==2))
    {
        HObject ho_ResultContour;
        HTuple hv_is_angle_right,hv_is_needle_right,hv_Length2,hv_angle;
        check_needle_and_angle (m_SceneImage,v_obj_result[num], &ho_ResultContour,
                                m_IntParameter[QString::fromLocal8Bit("针长")], m_FloatParameter[QString::fromLocal8Bit("针尺寸")],
                m_FloatParameter[QString::fromLocal8Bit("角度差范围")], m_FloatParameter[QString::fromLocal8Bit("标准角度")],
                &hv_is_angle_right, &hv_is_needle_right, &hv_Length2, &hv_angle);
        QString text;
        text=QString::fromLocal8Bit("针宽:")+QString::number(hv_Length2.D()*2,'f',3);
        v_str_result[num].push_back(text);
        //判断是否折针
        if(hv_is_needle_right.I()==1)
        {
            v_str_bool[num].push_back(1);

            //针宽对了才计算角度
            {
                text=QString::fromLocal8Bit("角度:")+QString::number(hv_angle.D());
                v_contour_result<<ho_ResultContour;
                v_str_result[num].push_back(text);
                if(hv_is_angle_right.I()==1)
                {
                    v_contour_bool.push_back(1);
                    v_str_bool[num].push_back(1);
                }
                else
                {
                    v_contour_bool.push_back(2);
                    v_str_bool[num].push_back(2);
                    isneedleright=false;
                }
            }
        }
        else
        {
            v_str_bool[num].push_back(2);
            isneedleright=false;
        }
    }
    if(!isneedleright)
    {
        if(v_obj_bool[num]==1)
        {
            v_obj_bool[num]=2;
        }
        else if (v_obj_bool[num]==2)
        {
            v_obj_bool[num]=3;
        }
    }

    if(check_count!=v_obj_result.size())return;
    check_count=0;
    QStringList result;
    foreach (const int &info,v_obj_bool)
    {
        m_send_data<<info;
        result<<QString::number(info);
    }
    send_result();
    time_end = double(clock());
    m_runInfo=tr(QString::fromLocal8Bit("%1检测时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);

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
//读取文件
void Pro_NeedleSeatGlue::slot_read_image(QString cam,QString step,QString imgPath)
{
    if(cam==m_CamName&&step==m_StepName)
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

//写入记录
void Pro_NeedleSeatGlue::writeImageAndReport()//写入图片和记录
{
    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd");
    if(m_IntParameter[QString::fromLocal8Bit("保存检测图像")]==1)
    {
        GeneralFunc::isDirExist(dateDir,true);
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        QtHalcon::saveHObject(m_SceneImageShow,"jpg",timePath.toLocal8Bit().data());
    }
    if(m_IntParameter[QString::fromLocal8Bit("保存原始图像")]==1)
    {
        QString timePath=dateDir+"\\origin"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_OriginImage,"jpg",timePath.toLocal8Bit().data());
    }
}


//写或绘制检测信息到虚拟窗口
void Pro_NeedleSeatGlue::writeCheckInfoToImage()//写检测结果到图片
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
        QtHalcon::displayHObject(m_SceneImage,hv_WindowHandle_UI);
        SetDraw(hv_WindowHandle_UI,"margin");
        int marginwidth=m_IntParameter[QString::fromLocal8Bit("绘制边缘宽度")];//5
        SetLineWidth(hv_WindowHandle_UI,marginwidth);
        for(int i=0;i<v_obj_result.size();i++)
        {
            QString roistr=QtHalcon::getRoiStr("Rectangle1",v_obj_result[i]);
            QStringList roistrlist=roistr.split("|");
            QString colorstr;
            v_obj_bool[i]==1?colorstr="green":colorstr="red";
            SetColor(hv_WindowHandle_UI,colorstr.toLocal8Bit().data());
            DispObj(v_obj_result[i],hv_WindowHandle_UI);
            HTuple width,height;
            HalconCpp::GetImageSize(m_SceneImage,&width,&height);
            QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/50).toLocal8Bit().data(),QString::number(width.I()/120).toLocal8Bit().data());
            QtHalcon::writeWindowString(hv_WindowHandle_UI,QString::number(v_obj_bool[i]).toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),roistrlist[0].toInt()+marginwidth,roistrlist[1].toInt()+marginwidth);

            for (int j=0;j<v_str_result[i].size();j++)
            {
                int size=height.I()/40+10;
                int x=roistrlist[1].toInt();
                int y=20+size*j;
                QString colorstr;
                v_str_bool[i][j]==1?colorstr="green":colorstr="red";
                QtHalcon::writeWindowString(hv_WindowHandle_UI,v_str_result[i][j].toLocal8Bit().data(),
                                            colorstr.toLocal8Bit().data(),y,x);
            }

        }
        for(int i=0;i<v_contour_bool.size();i++)
        {
            QString colorstr;
            v_contour_bool[i]==1?colorstr="green":colorstr="red";
            SetColor(hv_WindowHandle_UI,colorstr.toLocal8Bit().data());
            DispObj(v_contour_result[i],hv_WindowHandle_UI);
        }


        DumpWindowImage(&m_SceneImageShow,hv_WindowHandle_UI);
        QImage image;
        QtHalcon::hImage2QImage(m_SceneImageShow,image);
        ClearWindow(hv_WindowHandle_UI);

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
void Pro_NeedleSeatGlue::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
//选区变化
void Pro_NeedleSeatGlue::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_NeedleSeatGlue::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_NeedleSeatGlue::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_NeedleSeatGlue::slot_program_exit()
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
// Local procedures
void Pro_NeedleSeatGlue::getCheckRegion (HObject ho_Image, HObject ho_ROI_0, HObject *ho_Rectangle, HObject *ho_ImagePart1,
                                         HTuple hv_thr_dif, HTuple hv_select_area_min, HTuple hv_seat_size, HTuple hv_needle_size,
                                         HTuple hv_check_size)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region, ho_ConnectedRegions;
    HObject  ho_SelectedRegions, ho_RegionUnion, ho_RegionClosing;
    HObject  ho_RegionOpening, ho_Rectangle1, ho_RegionTopHat;
    HObject  ho_ImageReduced1;

    // Local control variables
    HTuple  hv_Area, hv_Row, hv_Column, hv_Width;
    HTuple  hv_Height, hv_Area1, hv_Row3, hv_Column3, hv_Row1;
    HTuple  hv_Column1, hv_Row2, hv_Column2, hv_h_side;

    ReduceDomain(ho_Image, ho_ROI_0, &ho_ImageReduced);
    VarThreshold(ho_ImageReduced, &ho_Region, 15, 15, 0.2, hv_thr_dif, "dark");
    Connection(ho_Region, &ho_ConnectedRegions);
    SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_select_area_min,
                999999);
    Union1(ho_SelectedRegions, &ho_RegionUnion);
    ClosingRectangle1(ho_RegionUnion, &ho_RegionClosing, hv_seat_size, hv_seat_size);
    OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening, hv_needle_size, 1);
    GenRectangle1(&ho_Rectangle1, 0, 0, 1, hv_seat_size);
    TopHat(ho_RegionOpening, ho_Rectangle1, &ho_RegionTopHat);
    OpeningCircle(ho_RegionTopHat, &ho_RegionTopHat, 3.5);
    AreaCenter(ho_RegionTopHat, &hv_Area, &hv_Row, &hv_Column);
    GetImageSize(ho_Image, &hv_Width, &hv_Height);
    if (0 != (hv_Area<(hv_select_area_min*2)))
    {
        AreaCenter(ho_ROI_0, &hv_Area1, &hv_Row3, &hv_Column3);
        GenRectangle1(&ho_RegionTopHat, hv_Row3-60, hv_Column3-60, hv_Row3+60, hv_Column3+60);
    }
    SmallestRectangle1(ho_RegionTopHat, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    hv_h_side = (hv_check_size-(hv_Column2-hv_Column1))/2;
    GenRectangle1(&(*ho_Rectangle), hv_Row1, hv_Column1-hv_h_side, hv_Row1+hv_check_size,
                  (hv_Column1-hv_h_side)+hv_check_size);
    ReduceDomain(ho_ImageReduced, (*ho_Rectangle), &ho_ImageReduced1);
    CropDomain(ho_ImageReduced1, &(*ho_ImagePart1));
    return;
}

void Pro_NeedleSeatGlue::check_needle_and_angle (HObject ho_Image, HObject ho_Rectangle, HObject *ho_ResultContour,
                                                 HTuple hv_needle_length, HTuple hv_needle_size, HTuple hv_angle_min, HTuple hv_angle_std,
                                                 HTuple *hv_is_angle_right, HTuple *hv_is_needle_right, HTuple *hv_Length2, HTuple *hv_angle)
{

    // Local iconic variables
    HObject  ho_Rectangle2, ho_ImageReduced, ho_Region;
    HObject  ho_RegionErosion, ho_RegionClosing, ho_ImageReduced1;
    HObject  ho_Region1, ho_Rectangle1, ho_ResultContour_1, ho_Edge_Cross;
    HObject  ho_ResultContour_2;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2;
    HTuple  hv_UsedThreshold, hv_UsedThreshold1, hv_Row, hv_Column;
    HTuple  hv_Phi, hv_Length1, hv_edge_strength, hv_Black_To_White;
    HTuple  hv_First_Edge, hv_Row13, hv_Column13, hv_Row23;
    HTuple  hv_Column23, hv_Line1, hv_Line2, hv_intersection_point_1;

    (*hv_is_angle_right) = 0;
    (*hv_is_needle_right) = 0;
    GenEmptyObj(&(*ho_ResultContour));
    SmallestRectangle1(ho_Rectangle, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    GenRectangle1(&ho_Rectangle2, hv_Row1-hv_needle_length, hv_Column1, hv_Row1, hv_Column2);
    ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced);
    BinaryThreshold(ho_ImageReduced, &ho_Region, "max_separability", "light", &hv_UsedThreshold);
    ErosionRectangle1(ho_Region, &ho_RegionErosion, 1, 20);
    ClosingRectangle1(ho_RegionErosion, &ho_RegionClosing, 20, 1);
    ReduceDomain(ho_Image, ho_RegionClosing, &ho_ImageReduced1);
    BinaryThreshold(ho_ImageReduced1, &ho_Region1, "max_separability", "dark", &hv_UsedThreshold1);
    SmallestRectangle2(ho_Region1, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1, &(*hv_Length2));
    if (0 != (((*hv_Length2)*2)<hv_needle_size))
    {
        (*hv_is_needle_right) = 1;
        GenRectangle1(&ho_Rectangle1, hv_Row1, hv_Column1, ((hv_Row1+hv_Row2)/2)+20,
                      (hv_Column1+hv_Column2)/2);
        //边缘强度（边缘强度越高保留边缘点越清晰）
        hv_edge_strength = 15;
        //由黑到白（由0到255为增加，故为'positive'）
        hv_Black_To_White = 1;
        //第一条边（查找方为从直线左侧到直线右侧，直线起点到终点为前）
        hv_First_Edge = 0;
        SmallestRectangle1(ho_Rectangle1, &hv_Row13, &hv_Column13, &hv_Row23, &hv_Column23);
        ReduceDomain(ho_Image, ho_Rectangle1, &ho_ImageReduced1);
        QtHalcon::findLineInRectangle1(ho_ImageReduced1, &ho_ResultContour_1, &ho_Edge_Cross, hv_edge_strength,
                                       hv_Black_To_White, hv_First_Edge, ((hv_Row13.TupleConcat(hv_Column13)).TupleConcat(hv_Row23)).TupleConcat(hv_Column23),
                                       0, &hv_Line1);
        SmallestRectangle1(ho_Rectangle2, &hv_Row13, &hv_Column13, &hv_Row23, &hv_Column23);
        ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced1);
        QtHalcon::findLineInRectangle1(ho_ImageReduced1, &ho_ResultContour_2, &ho_Edge_Cross, hv_edge_strength,
                                       hv_Black_To_White, hv_First_Edge, ((hv_Row13.TupleConcat(hv_Column13)).TupleConcat(hv_Row23)).TupleConcat(hv_Column23),
                                       0, &hv_Line2);
        ConcatObj(ho_ResultContour_1, ho_ResultContour_2, &(*ho_ResultContour));
        //两条线的交点
        QtHalcon::linesIntersection(hv_Line1, hv_Line2, &hv_intersection_point_1, &(*hv_angle));
        if (0 != (HTuple((*hv_angle)<=(hv_angle_std+hv_angle_min)).TupleAnd((*hv_angle)>=(hv_angle_std-hv_angle_min))))
        {
            (*hv_is_angle_right) = 1;
        }
        else
        {
            (*hv_is_angle_right) = 0;
        }
    }
    else
    {
        (*hv_is_needle_right) = 0;
    }
    return;
}
