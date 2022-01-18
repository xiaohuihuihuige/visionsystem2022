#include "pro_distance.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"

Pro_Distance::Pro_Distance(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_Distance::~Pro_Distance()
{

}
//初始化参数
void Pro_Distance::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
    readTypeInfo();
    openHalconWindow();
}
void Pro_Distance::openHalconWindow()
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
void Pro_Distance::readROI()
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
void Pro_Distance::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_Distance::readPar()
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
}

//计算点击处的像素信息
void Pro_Distance::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_Distance::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_Distance::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
    v_str_result.clear();
    v_str_bool.clear();
    v_obj_result.clear();
    v_obj_bool.clear();
    m_send_data.clear();
    QMap<QString,HObject>::iterator iter=map_hobj.begin();
    while(iter!=map_hobj.end())
    {
        QString key=iter.key();
        HObject value=iter.value();
        try
        {
            /*QStringList name_type_list=key.split('#');
            if(name_type_list.size()<1)continue;
            HObject roi;
            QtHalcon::getRoi(name_type_list[1],value,&roi);*/

            HTuple hv_is_distance_ok, hv_distance, hv_is_top_ok,
                    hv_top_width, hv_is_fold_ok,hv_fold_area, hv_is_slant_ok,
                    hv_slant_width, hv_is_melt_ok, hv_melt_angle;
            HObject ho_distance_arrow,ho_top_region,ho_melt_contours,ho_fold_region,ho_slant_rect,ho_needle_rect;

            check_distance_melt_top (m_SceneImage, &ho_distance_arrow, &ho_melt_contours,
                                     &ho_top_region, &ho_fold_region, &ho_slant_rect, &ho_needle_rect,
                                     m_IntParameter[QString::fromLocal8Bit("方向有无")], m_IntParameter[QString::fromLocal8Bit("检测莱距离")],
                    m_IntParameter[QString::fromLocal8Bit("检测熔头角度")],m_IntParameter[QString::fromLocal8Bit("检测斜面")],
                    m_IntParameter[QString::fromLocal8Bit("检测针尖")], m_IntParameter[QString::fromLocal8Bit("检测熔头褶皱")],
                    m_FloatParameter[QString::fromLocal8Bit("针角度计算矩形高度")],m_FloatParameter[QString::fromLocal8Bit("针宽度")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面阈值")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面高度小")], m_FloatParameter[QString::fromLocal8Bit("斜面高度大")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面宽度小")] , m_FloatParameter[QString::fromLocal8Bit("斜面宽度大")] ,
                    m_FloatParameter[QString::fromLocal8Bit("针尖开运算尺寸")] , m_FloatParameter[QString::fromLocal8Bit("针尖宽度小")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面矩形宽度小")],
                    m_FloatParameter[QString::fromLocal8Bit("管厚度")] ,m_FloatParameter[QString::fromLocal8Bit("管位置")],
                    m_FloatParameter[QString::fromLocal8Bit("莱距离小")]/m_FloatParameter[QString::fromLocal8Bit("像素尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("莱距离大")]/m_FloatParameter[QString::fromLocal8Bit("像素尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("褶皱位置")], m_FloatParameter[QString::fromLocal8Bit("褶皱阈值")] ,
                    m_FloatParameter[QString::fromLocal8Bit("褶皱面积小")] ,
                    m_FloatParameter[QString::fromLocal8Bit("熔头阈值")], m_FloatParameter[QString::fromLocal8Bit("熔头面积小")],
                    m_FloatParameter[QString::fromLocal8Bit("熔头边缘强度")] ,m_FloatParameter[QString::fromLocal8Bit("熔头角度小")] ,
                    &hv_is_distance_ok, &hv_distance, &hv_is_top_ok,
                    &hv_top_width, &hv_is_fold_ok,&hv_fold_area, &hv_is_slant_ok,
                    &hv_slant_width, &hv_is_melt_ok, &hv_melt_angle);

            v_obj_result.push_back(ho_needle_rect);
            v_obj_bool.push_back(1);

            QString text;
            if(m_IntParameter[QString::fromLocal8Bit("检测莱距离")]==1)
            {
                text=QString::fromLocal8Bit("莱距离:")+QString::number(hv_distance.D()*m_FloatParameter[QString::fromLocal8Bit("像素尺寸")]/1000,'f',3);
                v_str_result.push_back(text);
                v_obj_result.push_back(ho_distance_arrow);
                if(hv_is_distance_ok.I()==1)
                {
                    v_obj_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            if(m_IntParameter[QString::fromLocal8Bit("检测熔头角度")]==1)
            {
                text=QString::fromLocal8Bit("熔头角度:")+QString::number(hv_melt_angle.D(),'f',3);
                v_str_result.push_back(text);
                v_obj_result.push_back(ho_melt_contours);
                if(hv_is_melt_ok.I()==1)
                {
                    v_obj_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            if(m_IntParameter[QString::fromLocal8Bit("检测斜面")]==1)
            {
                text=QString::fromLocal8Bit("斜面宽度:")+QString::number(hv_slant_width.D(),'f',3);
                v_str_result.push_back(text);
                v_obj_result.push_back(ho_slant_rect);
                if(hv_is_slant_ok.I()==1)
                {
                    v_obj_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            if(m_IntParameter[QString::fromLocal8Bit("检测针尖")]==1)
            {
                text=QString::fromLocal8Bit("针尖宽度:")+QString::number(hv_top_width.I());
                v_str_result.push_back(text);
                v_obj_result.push_back(ho_top_region);
                if(hv_is_top_ok.I()==1)
                {
                    v_obj_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            if(m_IntParameter[QString::fromLocal8Bit("检测熔头褶皱")]==1)
            {
                text=QString::fromLocal8Bit("褶皱面积:")+QString::number(hv_fold_area.I());
                v_str_result.push_back(text);
                v_obj_result.push_back(ho_fold_region);
                if(hv_is_fold_ok.I()==1)
                {
                    v_obj_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            if(v_obj_bool.contains(2))
            {
                m_send_data<<2;
            }
            else
            {
                m_send_data<<1;
            }
        }
        catch (HalconCpp::HException &e)
        {
            m_send_data<<2;
            emit  signal_information_text(m_CamName,1,"halcon");
        }
        iter++;
    }
    send_result();
    m_runInfo=tr(QString::fromLocal8Bit("%1检测结果:%2").arg(m_CamName,QString::number(m_send_data[0])).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);
    time_end = double(clock());//结束采图计时
    m_runInfo=tr(QString::fromLocal8Bit("%1检测时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);
    time_start = double(clock());
    writeCheckInfoToImage();
    time_end = double(clock());//结束绘图计时
    m_runInfo=tr(QString::fromLocal8Bit("%1绘制时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);
    time_start = double(clock());
    writeImageAndReport();
    time_end = double(clock());

    m_runInfo=tr(QString::fromLocal8Bit("%1保存时间:%2ms").arg(m_CamName,QString::number(time_end-time_start)).toUtf8());
    emit  signal_information_text(m_CamName,1,m_runInfo);
}
//读取文件
void Pro_Distance::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_Distance::writeImageAndReport()//写入图片和记录
{
    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd")+"\\"+QString::number(m_send_data[0]);
    if(m_IntParameter[QString::fromLocal8Bit("保存检测图像")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
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
void Pro_Distance::writeCheckInfoToImage()//写检测结果到图片
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
        SetLineWidth(hv_WindowHandle_UI,m_IntParameter[QString::fromLocal8Bit("绘制边缘宽度")]);//4
        for(int i=0;i<v_obj_result.size();i++)
        {
            QString colorstr;
            v_obj_bool[i]==1?colorstr="green":colorstr="red";
            SetColor(hv_WindowHandle_UI,colorstr.toLocal8Bit().data());
            DispObj(v_obj_result[i],hv_WindowHandle_UI);
        }

        HTuple width,height;
        HalconCpp::GetImageSize(m_SceneImage,&width,&height);
        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/30).toLocal8Bit().data(),QString::number(width.I()/70).toLocal8Bit().data());
        for (int i=0;i<v_str_result.size();i++)
        {
            int size=height.I()/25+50;
            int x=50;
            int y=50+size*i;
            QString colorstr;
            v_str_bool[i]==1?colorstr="green":colorstr="red";
            QtHalcon::writeWindowString(hv_WindowHandle_UI,v_str_result[i].toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),y,x);
        }
        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/20).toLocal8Bit().data(),QString::number(width.I()/40).toLocal8Bit().data());
        if(m_send_data.contains(1))
        {
            QtHalcon::writeWindowString(hv_WindowHandle_UI,"OK","green",height.I()/20,width.I()-height.I()/15-100);
        }
        else
        {
            QtHalcon::writeWindowString(hv_WindowHandle_UI,"NG","red",height.I()/20,width.I()-height.I()/15-100);
        }
        DumpWindowImage(&m_SceneImageShow,hv_WindowHandle_UI);
        QImage image;
        QtHalcon::hImage2QImage(m_SceneImageShow,image);
        ClearWindow(hv_WindowHandle_UI);
        emit signal_information_image(m_CamName,m_send_data[0],image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_Distance::send_result()
{
    if(m_IntParameter[QString::fromLocal8Bit("复制位")]==1)
        m_send_data<<m_send_data[0];
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_Distance::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_Distance::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_Distance::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_Distance::slot_program_exit()
{

}
void Pro_Distance::check_distance_melt_top (HObject ho_Image, HObject *ho_distance_arrow, HObject *ho_melt_contours,
                                            HObject *ho_top_region, HObject *ho_fold_region, HObject *ho_slant_rect, HObject *ho_needle_rect,
                                            HTuple hv_is_direction, HTuple hv_is_check_distance, HTuple hv_is_check_melt,
                                            HTuple hv_is_check_slant, HTuple hv_is_check_top, HTuple hv_is_check_fold, HTuple hv_needle_angle_rect_height,
                                            HTuple hv_needle_width, HTuple hv_top_thr, HTuple hv_top_h_min, HTuple hv_top_h_max,
                                            HTuple hv_top_w_min, HTuple hv_top_w_max, HTuple hv_top_opening_size, HTuple hv_top_width_min,
                                            HTuple hv_slant_width_min, HTuple hv_pipe_thick, HTuple hv_pipe_position, HTuple hv_distance_min,
                                            HTuple hv_distance_max, HTuple hv_fold_pos, HTuple hv_fold_thr, HTuple hv_fold_area_min,
                                            HTuple hv_melt_thr, HTuple hv_melt_min_area, HTuple hv_melt_side_edge_strength,
                                            HTuple hv_melt_angle_min, HTuple *hv_is_distance_ok, HTuple *hv_distance, HTuple *hv_is_top_ok,
                                            HTuple *hv_top_width, HTuple *hv_is_fold_ok, HTuple *hv_fold_area, HTuple *hv_is_slant_ok,
                                            HTuple *hv_slant_width, HTuple *hv_is_melt_ok, HTuple *hv_melt_angle)
{

    // Local iconic variables
    HObject  ho_Region, ho_ConnectedRegions, ho_SelectedRegions;
    HObject  ho_RegionUnion, ho_Rectangle, ho_ImageReduced, ho_ImageScaled;
    HObject  ho_Rectangle8, ho_ImageReduced7, ho_ImageResult3;
    HObject  ho_Region7, ho_Rectangle19, ho_RegionClosing5, ho_RegionOpening6;
    HObject  ho_RegionClosing1, ho_RegionFillUp, ho_SelectedRegions1;
    HObject  ho_slant_region, ho_RegionOpening2, ho_Rectangle4;
    HObject  ho_ImageReduced6, ho_Region2, ho_RegionOpening10;
    HObject  ho_RegionClosing4, ho_ImageReduced5, ho_Region6;
    HObject  ho_RegionOpening7, ho_ConnectedRegions9, ho_SelectedRegions10;
    HObject  ho_RegionUnion5, ho_region_p, ho_Rectangle_t, ho_ImageReduced3;
    HObject  ho_Region_t, ho_ImageReduced4, ho_ImageScaled1;
    HObject  ho_Region5, ho_Region11, ho_RegionClosing9, ho_RegionOpening9;
    HObject  ho_ConnectedRegions4, ho_SelectedRegions4, ho_Rectangle_p;
    HObject  ho_Region_p, ho_RegionFillUp5, ho_Rectangle21, ho_RegionOpening1;
    HObject  ho_RegionClosing6, ho_RegionFillUp4, ho_Rectangle20;
    HObject  ho_ImageReduced10, ho_Region10, ho_RegionFillUp2;
    HObject  ho_RegionClosing8, ho_Rectangle15, ho_RegionOpening12;
    HObject  ho_ConnectedRegions8, ho_SelectedRegions8, ho_RegionUnion1;
    HObject  ho_Rectangle9, ho_RegionDifference1, ho_Rectangle16;
    HObject  ho_RegionOpening3, ho_RegionClosing2, ho_ConnectedRegions3;
    HObject  ho_SelectedRegions6, ho_RegionUnion10, ho_RegionDilation;
    HObject  ho_Contours, ho_Arrow1, ho_Arrow2, ho_ImageReduced1;
    HObject  ho_ResultContour_1, ho_Edge_Cross, ho_Rectangle5;
    HObject  ho_ResultContour_2, ho_Rectangle7, ho_Region4, ho_ConnectedRegions6;
    HObject  ho_SelectedRegions5, ho_RegionUnion3, ho_RegionClosing10;
    HObject  ho_RegionFillUp1, ho_Rectangle1, ho_Rectangle6;
    HObject  ho_ImageReduced2, ho_Region3, ho_ConnectedRegions1;
    HObject  ho_SelectedRegions3, ho_RegionClosing, ho_ConnectedRegions2;
    HObject  ho_SelectedRegions2, ho_Rectangle2, ho_ImageResult;
    HObject  ho_Region1, ho_ConnectedRegions12, ho_SelectedRegions13;
    HObject  ho_SelectedRegions18, ho_SelectedRegions19, ho_RegionUnion7;
    HObject  ho_Rectangle3, ho_Rectangle10, ho_ImageReduced8;
    HObject  ho_Region8, ho_RegionFillUp3, ho_RegionOpening16;
    HObject  ho_ConnectedRegions14, ho_SelectedRegions15, ho_SelectedRegions16;
    HObject  ho_SelectedRegions20, ho_RegionUnion9, ho_Rectangle14;
    HObject  ho_RegionIntersection1, ho_Rectangle17, ho_Rectangle11;
    HObject  ho_ImageReduced9, ho_ImageResult1, ho_Region9, ho_ConnectedRegions13;
    HObject  ho_SelectedRegions14, ho_RegionUnion8, ho_Rectangle12;
    HObject  ho_RegionClosing3, ho_Rectangle13, ho_RegionOpening11;
    HObject  ho_ConnectedRegions11, ho_SelectedRegions12, ho_RegionUnion6;
    HObject  ho_Rectangle18;

    // Local control variables
    HTuple  hv_noise_area, hv_Width, hv_Height, hv_UsedThreshold5;
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2, hv_Min;
    HTuple  hv_Max, hv_Range, hv_mult, hv_add, hv_UsedThreshold6;
    HTuple  hv_Row12, hv_Column12, hv_Phi5, hv_Length16, hv_Length26;
    HTuple  hv_anele_needle, hv_Area, hv_Row, hv_Column, hv_Row8;
    HTuple  hv_Column8, hv_Phi0, hv_Length1, hv_Length2, hv_Number2;
    HTuple  hv_Row7, hv_Column7, hv_Phi3, hv_Length14, hv_Length24;
    HTuple  hv_Row110, hv_Column110, hv_Row210, hv_Column210;
    HTuple  hv_Row112, hv_Column112, hv_Row212, hv_Column212;
    HTuple  hv_UsedThreshold1, hv_Row30, hv_Column30, hv_Phi4;
    HTuple  hv_Length15, hv_Length25, hv_top_check_height, hv_row_dis_t;
    HTuple  hv_col_dis_t, hv_UsedThreshold2, hv_Row16, hv_Column16;
    HTuple  hv_Row27, hv_Column27, hv_height_top_bottom_1, hv_Min1;
    HTuple  hv_Max1, hv_Range1, hv_UsedThreshold3, hv_UsedThreshold;
    HTuple  hv_Row17, hv_Column17, hv_Row28, hv_Column28, hv_height_top_bottom_2;
    HTuple  hv_row_dis_p, hv_col_dis_p, hv_open_height, hv_row_open;
    HTuple  hv_col_open, hv_Row5, hv_Column5, hv_Phi, hv_Length11;
    HTuple  hv_Length21, hv_Row9, hv_Column9, hv_Phi2, hv_Length13;
    HTuple  hv_Length23, hv_length_cut, hv_Row11, hv_Column11;
    HTuple  hv_Row21, hv_Column21, hv_Area1, hv_b, hv_Row3;
    HTuple  hv_Column3, hv_IsOverlapping, hv_offset, hv_Row19;
    HTuple  hv_Column19, hv_Row26, hv_Column26, hv_Row22, hv_Column22;
    HTuple  hv_Line1, hv_Row13, hv_Column13, hv_Row23, hv_Column23;
    HTuple  hv_Line2, hv_intersection_point, hv_Row14, hv_Column14;
    HTuple  hv_Row24, hv_Column24, hv_Row25, hv_Column25, hv_row_m;
    HTuple  hv_col_m, hv_leng_m, hv_Row4, hv_Column4, hv_Number;
    HTuple  hv_Row6, hv_Column6, hv_Length12, hv_Length22, hv_row_needle;
    HTuple  hv_col_needle, hv_Row113, hv_Column113, hv_Row213;
    HTuple  hv_Column213, hv_Row114, hv_Column114, hv_Row214;
    HTuple  hv_Column214, hv_UsedThreshold4, hv_Area6, hv_Row32;
    HTuple  hv_Column32, hv_Row111, hv_Column111, hv_Row211;
    HTuple  hv_Column211;

    (*hv_is_top_ok) = 0;
    (*hv_top_width) = 0;
    (*hv_is_fold_ok) = 0;
    (*hv_fold_area) = 0;
    (*hv_is_distance_ok) = 0;
    (*hv_distance) = 0;
    (*hv_is_slant_ok) = 0;
    (*hv_slant_width) = 0;
    (*hv_is_melt_ok) = 0;
    (*hv_melt_angle) = 0;
    GenEmptyObj(&(*ho_distance_arrow));
    GenEmptyObj(&(*ho_melt_contours));
    GenEmptyObj(&(*ho_slant_rect));
    GenEmptyObj(&(*ho_top_region));
    GenEmptyObj(&(*ho_fold_region));
    GenEmptyObj(&(*ho_needle_rect));
    hv_noise_area = 500;
    GetImageSize(ho_Image, &hv_Width, &hv_Height);
    //计算针的区域
    BinaryThreshold(ho_Image, &ho_Region, "max_separability", "light", &hv_UsedThreshold5);
    Connection(ho_Region, &ho_ConnectedRegions);
    SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_noise_area,
        99999999);
    Union1(ho_SelectedRegions, &ho_RegionUnion);
    SmallestRectangle1(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    GenRectangle1(&ho_Rectangle, 0, hv_Column1-100, hv_Height, hv_Column2+100);
    //计算检测的区域，去除其他区域
    ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);
    MinMaxGray(ho_Rectangle, ho_Image, 0, &hv_Min, &hv_Max, &hv_Range);
    hv_mult = 255/hv_Range;
    hv_add = (-hv_mult)*hv_Min;
    ScaleImage(ho_ImageReduced, &ho_ImageScaled, hv_mult, hv_add);
    //计算针的角度
    GenRectangle1(&ho_Rectangle8, hv_Height-hv_needle_angle_rect_height, hv_Column1-100,
        hv_Height, hv_Column2+100);
    ReduceDomain(ho_ImageScaled, ho_Rectangle8, &ho_ImageReduced7);
    MultImage(ho_ImageReduced7, ho_ImageReduced7, &ho_ImageResult3, 0.5, 0);
    BinaryThreshold(ho_ImageResult3, &ho_Region7, "max_separability", "light", &hv_UsedThreshold6);
    SmallestRectangle2(ho_Region7, &hv_Row12, &hv_Column12, &hv_Phi5, &hv_Length16,
        &hv_Length26);
    GenRectangle2(&ho_Rectangle19, hv_Row12, hv_Column12, hv_Phi5, 1, hv_Length26);
    Closing(ho_Region7, ho_Rectangle19, &ho_RegionClosing5);
    GenRectangle2(&ho_Rectangle19, hv_Row12, hv_Column12, hv_Phi5, hv_Length26/2, 1);
    Opening(ho_RegionClosing5, ho_Rectangle19, &(*ho_needle_rect));
    OrientationRegion((*ho_needle_rect), &hv_anele_needle);

    //若是针斜面有方向
    if (0 != (hv_is_direction==1))
    {
      Threshold(ho_ImageScaled, &ho_Region, hv_top_thr/2, 255);
      OpeningCircle(ho_Region, &ho_RegionOpening6, 3.5);
      ClosingRectangle1(ho_RegionOpening6, &ho_RegionClosing1, 10, 2);
      FillUp(ho_RegionClosing1, &ho_RegionFillUp);
      Connection(ho_RegionFillUp, &ho_ConnectedRegions);
      SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "height", "and", hv_top_h_min,
          hv_top_h_max);
      SelectShape(ho_SelectedRegions, &ho_SelectedRegions1, "width", "and", hv_top_w_min,
          hv_top_w_max);
      Union1(ho_SelectedRegions1, &ho_slant_region);
      AreaCenter(ho_slant_region, &hv_Area, &hv_Row, &hv_Column);
      SmallestRectangle2(ho_slant_region, &hv_Row8, &hv_Column8, &hv_Phi0, &hv_Length1,
          &hv_Length2);
      CountObj(ho_slant_region, &hv_Number2);
      if (0 != (hv_Number2<1))
      {
        return;
      }
      //计算斜面宽度
      if (0 != (hv_is_check_slant==1))
      {
        //计算斜面宽度
        OpeningCircle(ho_slant_region, &ho_RegionOpening2, 3.5);
        SmallestRectangle2(ho_RegionOpening2, &hv_Row7, &hv_Column7, &hv_Phi3, &hv_Length14,
            &hv_Length24);
        GenRectangle2(&(*ho_slant_rect), hv_Row7, hv_Column7, hv_Phi3, hv_Length14,
            hv_Length24);
        //计算是否针尖上方有导管
        SmallestRectangle1(ho_slant_region, &hv_Row110, &hv_Column110, &hv_Row210,
            &hv_Column210);
        GenRectangle1(&ho_Rectangle4, 0, hv_Column110-(hv_Length24/2), hv_Row110-5,
            hv_Column210+(hv_Length24/2));
        ReduceDomain(ho_ImageScaled, ho_Rectangle4, &ho_ImageReduced6);
        MultImage(ho_ImageReduced6, ho_ImageReduced6, &ho_ImageReduced6, 5, 0);
        Threshold(ho_ImageReduced6, &ho_Region2, 200, 255);
        OpeningCircle(ho_Region2, &ho_RegionOpening10, 2.5);
        SmallestRectangle1(ho_RegionOpening10, &hv_Row112, &hv_Column112, &hv_Row212,
            &hv_Column212);
        //计算是否包针
        ClosingRectangle1(ho_slant_region, &ho_RegionClosing4, 20, 20);
        ReduceDomain(ho_ImageScaled, ho_RegionClosing4, &ho_ImageReduced5);
        BinaryThreshold(ho_ImageReduced5, &ho_Region6, "max_separability", "light",
            &hv_UsedThreshold1);
        OpeningCircle(ho_Region6, &ho_RegionOpening7, 2.0);
        Connection(ho_RegionOpening7, &ho_ConnectedRegions9);
        SelectShapeStd(ho_ConnectedRegions9, &ho_SelectedRegions10, "max_area", 70);
        Union1(ho_SelectedRegions10, &ho_RegionUnion5);
        SmallestRectangle2(ho_RegionUnion5, &hv_Row30, &hv_Column30, &hv_Phi4, &hv_Length15,
            &hv_Length25);
        (*hv_slant_width) = hv_Length24*2;
        if (0 != (HTuple(HTuple((*hv_slant_width)>hv_slant_width_min).TupleAnd((hv_Length15*2)>((hv_Length1*2)*0.9))).TupleAnd((hv_Column212-hv_Column112)<(hv_slant_width_min/2))))
        {
          (*hv_is_slant_ok) = 1;
        }
        else
        {
          return;
        }
      }
      GenEmptyObj(&ho_region_p);
      if (0 != (hv_is_check_distance==1))
      {
        //计算斜面区域
        hv_top_check_height = 40;
        hv_row_dis_t = hv_Row8+((hv_Length1-(hv_top_check_height/2))*((hv_anele_needle.TupleSin()).TupleAbs()));
        if (0 != (hv_anele_needle>0))
        {
          hv_col_dis_t = hv_Column8-((hv_Length1-(hv_top_check_height/2))*(hv_anele_needle.TupleCos()));
        }
        else
        {
          hv_col_dis_t = hv_Column8+((hv_Length1-(hv_top_check_height/2))*(hv_anele_needle.TupleCos()));
        }

        GenRectangle2(&ho_Rectangle_t, hv_row_dis_t, hv_col_dis_t, hv_anele_needle,
            hv_top_check_height/2, hv_Length2);
        ReduceDomain(ho_ImageScaled, ho_Rectangle_t, &ho_ImageReduced3);
        BinaryThreshold(ho_ImageReduced3, &ho_Region_t, "max_separability", "light",
            &hv_UsedThreshold2);
        Threshold(ho_ImageReduced3, &ho_Region_t, hv_UsedThreshold2*0.7, 255);
        OpeningCircle(ho_Region_t, &ho_Region_t, 3.5);
        SmallestRectangle1(ho_Region_t, &hv_Row16, &hv_Column16, &hv_Row27, &hv_Column27);
        hv_height_top_bottom_1 = hv_Row27-hv_Row16;
        ReduceDomain(ho_ImageReduced3, ho_Region_t, &ho_ImageReduced4);
        MinMaxGray(ho_Region_t, ho_ImageReduced4, 0, &hv_Min1, &hv_Max1, &hv_Range1);
        hv_mult = 255/hv_Range1;
        hv_add = (-hv_mult)*hv_Min1;
        ScaleImage(ho_ImageReduced4, &ho_ImageScaled1, hv_mult, hv_add);
        BinaryThreshold(ho_ImageScaled1, &ho_Region5, "max_separability", "dark", &hv_UsedThreshold3);
        BinaryThreshold(ho_ImageReduced4, &ho_Region11, "max_separability", "dark",
            &hv_UsedThreshold);
        Threshold(ho_ImageReduced4, &ho_Region6, 0, (hv_UsedThreshold3+hv_UsedThreshold)/2);
        ClosingCircle(ho_Region6, &ho_RegionClosing9, 2.0);
        OpeningRectangle1(ho_RegionClosing9, &ho_RegionOpening9, 6.0, 2.0);
        Connection(ho_RegionOpening9, &ho_ConnectedRegions4);
        SelectShapeStd(ho_ConnectedRegions4, &ho_SelectedRegions4, "max_area", 70);
        SmallestRectangle1(ho_SelectedRegions4, &hv_Row17, &hv_Column17, &hv_Row28,
            &hv_Column28);
        hv_height_top_bottom_2 = (hv_Row28-hv_Row17)+1;

        if (0 != (hv_height_top_bottom_2<(hv_height_top_bottom_1*0.10)))
        {
          //计算管区域
          hv_row_dis_p = (hv_Row8+(hv_Length1*((hv_anele_needle.TupleSin()).TupleAbs())))+(hv_pipe_position*((hv_anele_needle.TupleSin()).TupleAbs()));
          if (0 != (hv_anele_needle>0))
          {
            hv_col_dis_p = (hv_Column8-(hv_Length1*(hv_anele_needle.TupleCos())))-(hv_pipe_position*(hv_anele_needle.TupleCos()));
          }
          else
          {
            hv_col_dis_p = (hv_Column8+(hv_Length1*(hv_anele_needle.TupleCos())))+(hv_pipe_position*(hv_anele_needle.TupleCos()));
          }
          //获取管的区域
          GenRectangle2(&ho_Rectangle_p, hv_row_dis_p, hv_col_dis_p, hv_anele_needle,
              hv_pipe_position, hv_Length2*2);
          ReduceDomain(ho_ImageScaled, ho_Rectangle_p, &ho_ImageReduced3);
          Threshold(ho_ImageReduced3, &ho_Region_p, hv_melt_thr, 255);
          FillUp(ho_Region_p, &ho_RegionFillUp5);
          GenRectangle2(&ho_Rectangle21, hv_row_dis_p, hv_col_dis_p, hv_anele_needle,
              4, 3);
          Opening(ho_RegionFillUp5, ho_Rectangle21, &ho_RegionOpening1);
          GenRectangle2(&ho_Rectangle21, hv_row_dis_p, hv_col_dis_p, hv_anele_needle,
              2, 2);
          Closing(ho_RegionOpening1, ho_Rectangle21, &ho_RegionClosing6);
          FillUp(ho_RegionClosing6, &ho_RegionFillUp4);
          //计算开运算的值
          hv_open_height = 15;
          hv_row_open = hv_Row8+((hv_Length1-(hv_open_height/2))*((hv_anele_needle.TupleSin()).TupleAbs()));
          if (0 != (hv_anele_needle>0))
          {
            hv_col_open = hv_Column8-((hv_Length1-(hv_open_height/2))*(hv_anele_needle.TupleCos()));
          }
          else
          {
            hv_col_open = hv_Column8+((hv_Length1-(hv_open_height/2))*(hv_anele_needle.TupleCos()));
          }
          GenRectangle2(&ho_Rectangle20, hv_row_open, hv_col_open, hv_anele_needle,
              hv_open_height/2, hv_Length2*1.5);
          ReduceDomain(ho_ImageScaled, ho_Rectangle20, &ho_ImageReduced10);
          Threshold(ho_ImageReduced10, &ho_Region10, hv_melt_thr, 255);
          FillUp(ho_Region10, &ho_RegionFillUp2);
          ClosingRectangle1(ho_RegionFillUp2, &ho_RegionClosing8, 2, hv_open_height/3);
          OpeningRectangle1(ho_RegionClosing8, &ho_RegionOpening1, 1, hv_open_height*0.75);
          SmallestRectangle2(ho_RegionOpening1, &hv_Row5, &hv_Column5, &hv_Phi, &hv_Length11,
              &hv_Length21);
          GenRectangle2(&ho_Rectangle15, hv_Row5, hv_Column5, hv_Phi, hv_Length11+2,
              1);
          Opening(ho_RegionFillUp4, ho_Rectangle15, &ho_RegionOpening12);
          Connection(ho_RegionOpening12, &ho_ConnectedRegions8);
          SelectShapeStd(ho_ConnectedRegions8, &ho_SelectedRegions8, "max_area", 70);
          Union1(ho_SelectedRegions8, &ho_region_p);

          //计算莱距离区域
          Union2(ho_Region_t, ho_region_p, &ho_RegionUnion1);
          SmallestRectangle2(ho_RegionUnion1, &hv_Row9, &hv_Column9, &hv_Phi2, &hv_Length13,
              &hv_Length23);
          hv_length_cut = hv_Length13*((hv_anele_needle.TupleAbs()).TupleSin());
          GenRectangle2(&ho_Rectangle9, hv_Row9, hv_Column9, hv_anele_needle, hv_length_cut-10,
              hv_Length23-10);
          Difference(ho_Rectangle9, ho_RegionUnion1, &ho_RegionDifference1);
          GenRectangle2(&ho_Rectangle16, hv_Row9, hv_Column9, hv_anele_needle, 0.1,
              hv_Length23-11);
          Opening(ho_RegionDifference1, ho_Rectangle16, &ho_RegionOpening3);
          SmallestRectangle1(ho_RegionOpening3, &hv_Row11, &hv_Column11, &hv_Row21,
              &hv_Column21);
          ClosingRectangle1(ho_RegionOpening3, &ho_RegionClosing2, 1, ((hv_Row21-hv_Row11)/2)+1);
          Connection(ho_RegionClosing2, &ho_ConnectedRegions3);
          SelectShapeStd(ho_ConnectedRegions3, &ho_SelectedRegions6, "max_area", 70);
          Union1(ho_SelectedRegions6, &ho_RegionUnion10);
          AreaCenter(ho_RegionUnion10, &hv_Area1, &hv_Row1, &hv_Column1);
          if (0 != (hv_Area1<1))
          {
            GenRectangle1(&ho_RegionUnion10, 0, 0, 1, 1);
          }
          DilationCircle(ho_RegionUnion10, &ho_RegionDilation, 0.5);
          GenContourRegionXld(ho_RegionDilation, &ho_Contours, "border");

          hv_b = hv_Row1-(hv_Column1*((-hv_anele_needle).TupleTan()));
          hv_Column2 = hv_Column1+20;
          hv_Row2 = hv_b+(hv_Column2*((-hv_anele_needle).TupleTan()));
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
              10, 10);

          IntersectionLineContourXld(ho_Contours, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
              &hv_Row3, &hv_Column3, &hv_IsOverlapping);
          DistancePp(HTuple(hv_Row3[0]), HTuple(hv_Column3[0]), HTuple(hv_Row3[1]),
              HTuple(hv_Column3[1]), &(*hv_distance));
          hv_offset = 0;
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset,
              HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset, 10, 10);
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset,
              HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset, 10, 10);
          ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_distance_arrow));
          if (0 != (HTuple((*hv_distance)>hv_distance_min).TupleAnd((*hv_distance)<hv_distance_max)))
          {
            (*hv_is_distance_ok) = 1;
          }
        }
      }

      if (0 != (hv_is_check_melt==1))
      {
        //检测熔头角度
        SmallestRectangle1(ho_region_p, &hv_Row19, &hv_Column19, &hv_Row26, &hv_Column26);
        if (0 != ((hv_Row19.TupleLength())<1))
        {
          return;
        }
        GenRectangle1(&ho_Rectangle4, hv_Row19, hv_Column19-20, hv_Row26, (hv_Column26+hv_Column19)/2);
        SmallestRectangle1(ho_Rectangle4, &hv_Row12, &hv_Column12, &hv_Row22, &hv_Column22);
        ReduceDomain(ho_ImageScaled, ho_Rectangle4, &ho_ImageReduced1);
        QtHalcon::findLineInRectangle1(ho_ImageReduced1, &ho_ResultContour_1, &ho_Edge_Cross,
            hv_melt_side_edge_strength, 0, 0, ((hv_Row12.TupleConcat(hv_Column12)).TupleConcat(hv_Row22)).TupleConcat(hv_Column22),
            0, &hv_Line1);

        GenRectangle1(&ho_Rectangle5, hv_Row19, (hv_Column26+hv_Column19)/2, hv_Row26,
            hv_Column26+20);
        SmallestRectangle1(ho_Rectangle5, &hv_Row13, &hv_Column13, &hv_Row23, &hv_Column23);
        ReduceDomain(ho_ImageScaled, ho_Rectangle5, &ho_ImageReduced1);
        QtHalcon::findLineInRectangle1(ho_ImageReduced1, &ho_ResultContour_2, &ho_Edge_Cross,
            hv_melt_side_edge_strength, 1, 1, ((hv_Row13.TupleConcat(hv_Column13)).TupleConcat(hv_Row23)).TupleConcat(hv_Column23),
            0, &hv_Line2);
        ConcatObj(ho_ResultContour_1, ho_ResultContour_2, &(*ho_melt_contours));
        QtHalcon::linesIntersection(hv_Line1, hv_Line2, &hv_intersection_point, &(*hv_melt_angle));
        if (0 != ((*hv_melt_angle)>hv_melt_angle_min))
        {
          (*hv_is_melt_ok) = 1;
        }
      }


      //检测针尖
      if (0 != (hv_is_check_top==1))
      {
        GenRectangle2(&ho_Rectangle7, hv_Row8, hv_Column8, hv_anele_needle, hv_Length1+10,
            hv_Length2+10);
        ReduceDomain(ho_ImageScaled, ho_Rectangle7, &ho_ImageReduced);
        Threshold(ho_ImageReduced, &ho_Region4, hv_top_thr, 255);
        Connection(ho_Region4, &ho_ConnectedRegions6);
        SelectShape(ho_ConnectedRegions6, &ho_SelectedRegions5, "area", "and", 150,
            99999);
        Union1(ho_SelectedRegions5, &ho_RegionUnion3);
        ClosingRectangle1(ho_RegionUnion3, &ho_RegionClosing10, 10, 10);
        FillUp(ho_RegionClosing10, &ho_RegionFillUp1);
        SmallestRectangle1(ho_RegionFillUp1, &hv_Row14, &hv_Column14, &hv_Row24, &hv_Column24);
        GenRectangle1(&ho_Rectangle1, hv_Row14, hv_Column14, hv_Row14+hv_top_opening_size,
            hv_Column24);
        Intersection(ho_RegionFillUp1, ho_Rectangle1, &(*ho_top_region));
        SmallestRectangle1((*ho_top_region), &hv_Row13, &hv_Column13, &hv_Row25, &hv_Column25);
        GenRectangle1(&ho_Rectangle, hv_Row13, hv_Column13, hv_Row25, hv_Column25);
        (*hv_top_width) = hv_Column25-hv_Column13;
        if (0 != ((*hv_top_width)<hv_top_width_min))
        {
          (*hv_is_top_ok) = 1;
        }
      }
      //检测褶皱
      if (0 != (hv_is_check_fold==1))
      {
        hv_row_m = hv_Row8+(hv_fold_pos*((hv_anele_needle.TupleSin()).TupleAbs()));
        if (0 != (hv_anele_needle>0))
        {
          hv_col_m = hv_Column8-(hv_fold_pos*(hv_anele_needle.TupleCos()));
        }
        else
        {
          hv_col_m = hv_Column8+(hv_fold_pos*(hv_anele_needle.TupleCos()));
        }
        hv_leng_m = (hv_Row26-hv_Row19)/2;
        GenRectangle2(&ho_Rectangle6, hv_row_m, hv_col_m, hv_anele_needle, hv_leng_m,
            hv_Length2+hv_pipe_thick);
        ReduceDomain(ho_ImageScaled, ho_Rectangle6, &ho_ImageReduced2);
        VarThreshold(ho_ImageReduced2, &ho_Region3, 15, 15, 0.3, hv_fold_thr, "light");
        Connection(ho_Region3, &ho_ConnectedRegions1);
        SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions3, "area", "and", 10,
            99999);
        Union1(ho_SelectedRegions3, &ho_RegionUnion);
        ClosingRectangle1(ho_RegionUnion, &ho_RegionClosing, 10, 10);
        Connection(ho_RegionClosing, &ho_ConnectedRegions2);
        SelectShape(ho_ConnectedRegions2, &ho_SelectedRegions2, "area", "and", hv_fold_area_min,
            99999);
        SelectShape(ho_SelectedRegions2, &(*ho_fold_region), "width", "and", hv_needle_width/2,
            99999);
        AreaCenter((*ho_fold_region), &(*hv_fold_area), &hv_Row4, &hv_Column4);
        CountObj((*ho_fold_region), &hv_Number);
        if (0 != (hv_Number<1))
        {
          (*hv_is_fold_ok) = 1;
        }
      }
    }
    else
    {
      //计算针的角度
      GenRectangle2(&ho_Rectangle2, hv_needle_angle_rect_height, 1224, HTuple(90).TupleRad(),
          300, 1224);
      ReduceDomain(ho_ImageScaled, ho_Rectangle2, &ho_ImageReduced1);
      MultImage(ho_ImageReduced1, ho_ImageReduced1, &ho_ImageResult, 0.5, 0);
      Threshold(ho_ImageResult, &ho_Region1, hv_melt_thr, 255);
      Connection(ho_Region1, &ho_ConnectedRegions12);
      SelectShape(ho_ConnectedRegions12, &ho_SelectedRegions13, "area", "and", 500,
          99999);
      SelectShape(ho_SelectedRegions13, &ho_SelectedRegions18, "height", "and", 100,
          99999);
      SelectShape(ho_SelectedRegions18, &ho_SelectedRegions19, "width", "and", 0, (hv_needle_width+(2*hv_pipe_thick))*2);
      Union1(ho_SelectedRegions19, &ho_RegionUnion7);
      SmallestRectangle2(ho_RegionUnion7, &hv_Row6, &hv_Column6, &hv_anele_needle,
          &hv_Length12, &hv_Length22);
      GenRectangle2(&ho_Rectangle3, hv_Row6, hv_Column6, hv_anele_needle, hv_Length12,
          hv_Length22);
      hv_row_needle = 1024;
      if (0 != (hv_anele_needle>0))
      {
        hv_col_needle = hv_Column6+(((hv_Row6-hv_row_needle)/2)*(hv_anele_needle.TupleCos()));
      }
      else
      {
        hv_col_needle = hv_Column6-(((hv_Row6-hv_row_needle)/2)*(hv_anele_needle.TupleCos()));
      }
      GenRectangle2(&ho_Rectangle10, hv_row_needle, hv_col_needle, hv_anele_needle,
          1024, hv_Length22*2);

      //计算针区域
      ReduceDomain(ho_ImageScaled, ho_Rectangle10, &ho_ImageReduced8);
      MultImage(ho_ImageReduced8, ho_ImageReduced8, &ho_ImageResult, 1.0, 0);
      Threshold(ho_ImageResult, &ho_Region8, hv_top_thr, 255);

      FillUp(ho_Region8, &ho_RegionFillUp3);
      OpeningRectangle1(ho_RegionFillUp3, &ho_RegionOpening16, 2, 2);

      Connection(ho_RegionFillUp3, &ho_ConnectedRegions14);
      SelectShape(ho_ConnectedRegions14, &ho_SelectedRegions15, "area", "and", hv_fold_area_min,
          9999999);
      SelectShape(ho_SelectedRegions15, &ho_SelectedRegions16, "rect2_len2", "and",
          0, (hv_needle_width+(2*hv_pipe_thick))*1.5);
      SelectShape(ho_SelectedRegions16, &ho_SelectedRegions20, "height", "and", hv_top_h_min/4,
          99999);
      //计算针尖区域
      Union1(ho_SelectedRegions20, &ho_RegionUnion9);
      SmallestRectangle1(ho_RegionUnion9, &hv_Row113, &hv_Column113, &hv_Row213, &hv_Column213);
      GenRectangle1(&ho_Rectangle14, hv_Row113, hv_Column113, hv_Row113+hv_top_h_min,
          hv_Column213);
      Intersection(ho_RegionUnion9, ho_Rectangle14, &ho_RegionIntersection1);
      SmallestRectangle1(ho_RegionIntersection1, &hv_Row114, &hv_Column114, &hv_Row214,
          &hv_Column214);
      GenRectangle1(&ho_Rectangle17, hv_Row114, hv_Column114, hv_Row214, hv_Column214);
      //计算管区域
      //*****
      GenRectangle1(&ho_Rectangle11, hv_Row114, hv_Column114-hv_needle_width, 2048,
          hv_Column214+hv_needle_width);
      ReduceDomain(ho_ImageScaled, ho_Rectangle11, &ho_ImageReduced9);
      MultImage(ho_ImageReduced9, ho_ImageReduced9, &ho_ImageResult1, 5, 0);
      BinaryThreshold(ho_ImageResult1, &ho_Region9, "max_separability", "light", &hv_UsedThreshold4);
      Connection(ho_Region9, &ho_ConnectedRegions13);
      SelectShape(ho_ConnectedRegions13, &ho_SelectedRegions14, "area", "and", hv_melt_min_area,
          999999999);

      Union1(ho_SelectedRegions14, &ho_RegionUnion8);
      GenRectangle2(&ho_Rectangle12, hv_Row6, hv_Column6, hv_anele_needle, 1, (hv_needle_width+hv_pipe_thick)/2);
      Closing(ho_RegionUnion8, ho_Rectangle12, &ho_RegionClosing3);
      GenRectangle2(&ho_Rectangle13, hv_Row6, hv_Column6, hv_anele_needle, 1, hv_needle_width/2);
      Opening(ho_RegionClosing3, ho_Rectangle13, &ho_RegionOpening11);
      Connection(ho_RegionOpening11, &ho_ConnectedRegions11);
      SelectShapeStd(ho_ConnectedRegions11, &ho_SelectedRegions12, "max_area", 70);
      Union1(ho_SelectedRegions12, &ho_RegionUnion6);
      AreaCenter(ho_RegionUnion6, &hv_Area6, &hv_Row32, &hv_Column32);
      if (0 != (hv_Area6<1))
      {
        CopyObj(ho_Rectangle17, &ho_RegionUnion6, 1, 1);
      }

      SmallestRectangle1(ho_RegionUnion6, &hv_Row111, &hv_Column111, &hv_Row211, &hv_Column211);
      GenRectangle1(&ho_Rectangle5, hv_Row111, hv_Column111, hv_Row211, hv_Column211);

      //计算莱距离
      if (0 != (hv_Row111>hv_Row114))
      {
        GenRectangle1(&ho_Rectangle18, hv_Row114, 0, hv_Row111, 2448);
      }
      else
      {
        GenRectangle1(&ho_Rectangle18, hv_Row111, 0, hv_Row114, 2448);
      }
      GenContourRegionXld(ho_Rectangle18, &ho_Contours, "border");
      hv_b = hv_Row114-(hv_Column211*((-hv_anele_needle).TupleTan()));
      hv_Column2 = hv_Column211+1;
      hv_Row2 = hv_b+(hv_Column2*((-hv_anele_needle).TupleTan()));
      IntersectionLineContourXld(ho_Contours, hv_Row114, hv_Column211, hv_Row2, hv_Column2,
          &hv_Row3, &hv_Column3, &hv_IsOverlapping);
      DistancePp(HTuple(hv_Row3[0]), HTuple(hv_Column3[0]), HTuple(hv_Row3[1]), HTuple(hv_Column3[1]),
          &(*hv_distance));
      hv_offset = 50;
      QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset,
          HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset, 10, 10);
      QtHalcon::gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset,
          HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset, 10, 10);
      ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_distance_arrow));
      if (0 != (HTuple((*hv_distance)>hv_distance_min).TupleAnd((*hv_distance)<hv_distance_max)))
      {
        (*hv_is_distance_ok) = 1;
      }
    }
    return;
}


