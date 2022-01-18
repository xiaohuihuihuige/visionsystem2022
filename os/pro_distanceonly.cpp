#include "pro_distanceonly.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"

Pro_DistanceOnly::Pro_DistanceOnly(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_DistanceOnly::~Pro_DistanceOnly()
{

}
//初始化参数
void Pro_DistanceOnly::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_DistanceOnly::openHalconWindow()
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
void Pro_DistanceOnly::readROI()
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
void Pro_DistanceOnly::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_DistanceOnly::readPar()
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
void Pro_DistanceOnly::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_DistanceOnly::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_DistanceOnly::slot_check(QString camName,QString step)
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

            HTuple hv_is_distance_ok,hv_is_fold,hv_distance,hv_Area_fold,
                    hv_pipe_width_pixel,hv_is_top,hv_area_top,hv_slant_width,hv_is_slant_ok;
            HObject m_region_top,ho_region_fold,m_arrow_distance,ho_rect_slant,ho_needle_rect;


            check_distance_and_fold (m_SceneImage, &m_arrow_distance, &m_region_top, &ho_region_fold,&ho_rect_slant,&ho_needle_rect,
                                     m_FloatParameter[QString::fromLocal8Bit("针尖阈值")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面高度小")],m_FloatParameter[QString::fromLocal8Bit("斜面高度大")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面宽度小")], m_FloatParameter[QString::fromLocal8Bit("斜面宽度大")],
                    m_FloatParameter[QString::fromLocal8Bit("熔管顶部位置")],
                    m_FloatParameter[QString::fromLocal8Bit("来距离小")]/m_FloatParameter[QString::fromLocal8Bit("像元尺寸")],
                    m_FloatParameter[QString::fromLocal8Bit("来距离大")]/m_FloatParameter[QString::fromLocal8Bit("像元尺寸")],
                    m_IntParameter[QString::fromLocal8Bit("针尖开运算尺寸")], m_IntParameter[QString::fromLocal8Bit("针尖宽度大")],
                    m_IntParameter[QString::fromLocal8Bit("褶皱检测区域高")],
                    m_FloatParameter[QString::fromLocal8Bit("褶皱位置")],
                    m_FloatParameter[QString::fromLocal8Bit("管厚度")], m_FloatParameter[QString::fromLocal8Bit("管阈值")],
                    m_IntParameter[QString::fromLocal8Bit("褶皱面积小")],
                    m_IntParameter[QString::fromLocal8Bit("针宽度")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面宽度")],
                    m_IntParameter[QString::fromLocal8Bit("检测莱距离")],
                    m_IntParameter[QString::fromLocal8Bit("检测针尖")],
                    m_IntParameter[QString::fromLocal8Bit("检测斜面")],
                    m_IntParameter[QString::fromLocal8Bit("检测褶皱")],
                    m_FloatParameter[QString::fromLocal8Bit("熔头阈值")],
                    m_IntParameter[QString::fromLocal8Bit("熔头面积小")],
                    m_IntParameter[QString::fromLocal8Bit("无方向")],
                    m_IntParameter[QString::fromLocal8Bit("角度矩形位置")],
                    &hv_distance, &hv_is_distance_ok,&hv_area_top,
                    &hv_is_top, &hv_Area_fold, &hv_is_fold,&hv_slant_width,&hv_is_slant_ok);
            QString text;
            if(m_IntParameter[QString::fromLocal8Bit("无方向")]==1)
            {
                //判断莱距离
                text=QString::fromLocal8Bit("莱距离:")+QString::number(hv_distance.D()*m_FloatParameter[QString::fromLocal8Bit("像元尺寸")]/1000,'f',3);
                v_str_result.push_back(text);
                v_obj_result.push_back(m_arrow_distance);
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
                if(hv_is_distance_ok.I()==1)
                {
                    m_send_data<<1;
                }
                else
                {
                    m_send_data<<2;
                }
            }
            else
            {
                if(m_IntParameter[QString::fromLocal8Bit("检测针尖")]==1)
                {
                    //判断针尖
                    text=QString::fromLocal8Bit("针尖宽度:")+QString::number(hv_area_top.I());
                    v_str_result.push_back(text);
                    v_obj_result.push_back(m_region_top);
                    if(hv_is_top.I()==1)
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

                if(m_IntParameter[QString::fromLocal8Bit("检测莱距离")]==1)
                {
                    //判断莱距离
                    text=QString::fromLocal8Bit("莱距离:")+QString::number(hv_distance.D()*m_FloatParameter[QString::fromLocal8Bit("像元尺寸")]/1000,'f',3);
                    v_str_result.push_back(text);
                    v_obj_result.push_back(m_arrow_distance);
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


                if(m_IntParameter[QString::fromLocal8Bit("检测褶皱")]==1)
                {
                    //判断褶皱
                    if(hv_Area_fold.TupleLength()>0)
                    {
                        text=QString::fromLocal8Bit("褶皱面积:")+QString::number(hv_Area_fold.I(),'f',3);
                        v_str_result.push_back(text);
                        v_obj_result.push_back(ho_region_fold);
                        if(hv_is_fold.I()==1)
                        {
                            v_str_bool.push_back(1);
                            v_obj_bool.push_back(1);
                        }
                        else
                        {
                            v_str_bool.push_back(2);
                            v_obj_bool.push_back(2);
                        }
                    }
                }

                if(m_IntParameter[QString::fromLocal8Bit("检测斜面")]==1)
                {
                    //判断莱距离
                    text=QString::fromLocal8Bit("斜面宽度:")+QString::number(hv_slant_width.D(),'f',3);
                    v_str_result.push_back(text);
                    v_obj_result.push_back(ho_rect_slant);
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
                if(((m_IntParameter[QString::fromLocal8Bit("检测针尖")]==1&&hv_is_top.I()==1)||m_IntParameter[QString::fromLocal8Bit("检测针尖")]==0)
                        &&((m_IntParameter[QString::fromLocal8Bit("检测莱距离")]==1&&hv_is_distance_ok.I()==1)||m_IntParameter[QString::fromLocal8Bit("检测莱距离")]==0)
                        &&((m_IntParameter[QString::fromLocal8Bit("检测褶皱")]==1&&hv_is_fold.I()==1)||m_IntParameter[QString::fromLocal8Bit("检测褶皱")]==0)
                        &&((m_IntParameter[QString::fromLocal8Bit("检测斜面")]==1&&hv_is_slant_ok.I()==1)||m_IntParameter[QString::fromLocal8Bit("检测斜面")]==0))
                {
                    m_send_data<<1;
                }
                else
                {
                    m_send_data<<2;
                }
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
void Pro_DistanceOnly::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_DistanceOnly::writeImageAndReport()//写入图片和记录
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
void Pro_DistanceOnly::writeCheckInfoToImage()//写检测结果到图片
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
        DumpWindowImage(&m_SceneImageShow,hv_WindowHandle_UI);

        QImage image;
        QtHalcon::hImage2QImage(m_SceneImageShow,image);

        //ClearWindow(hv_WindowHandle_UI);
        emit signal_information_image(m_CamName,m_send_data[0],image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_DistanceOnly::send_result()
{
    if(m_send_data[0]!=1)
    {
        if(m_IntParameter[QString::fromLocal8Bit("复制位")]==1)
            m_send_data<<m_send_data[0];
        emit signal_result(m_send_data,m_CamName,m_StepName);
    }
}
//选区变化
void Pro_DistanceOnly::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_DistanceOnly::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_DistanceOnly::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_DistanceOnly::slot_program_exit()
{

}
void Pro_DistanceOnly::check_distance_and_fold (HObject ho_Image, HObject *ho_Arrow_distance, HObject *ho_Region_top,
                                                HObject *ho_region_fold, HObject *ho_rect_slant, HObject *ho_needle_rect, HTuple hv_top_thr,
                                                HTuple hv_top_h_min, HTuple hv_top_h_max, HTuple hv_top_w_min, HTuple hv_top_w_max,
                                                HTuple hv_pos_p, HTuple hv_distance_min, HTuple hv_distance_max, HTuple hv_top_opening_size,
                                                HTuple hv_top_width_min, HTuple hv_height_fold_area, HTuple hv_fold_pos, HTuple hv_pipe_thick,
                                                HTuple hv_thr_fold, HTuple hv_fold_area_min, HTuple hv_needle_width, HTuple hv_slant_width_min,
                                                HTuple hv_check_dis, HTuple hv_check_top, HTuple hv_check_slant, HTuple hv_check_fold,
                                                HTuple hv_melt_thr, HTuple hv_melt_min_area, HTuple hv_no_direction, HTuple hv_rect_row,
                                                HTuple *hv_distance, HTuple *hv_is_distance_ok, HTuple *hv_top_width, HTuple *hv_is_top,
                                                HTuple *hv_Area_fold, HTuple *hv_is_fold, HTuple *hv_slant_width, HTuple *hv_is_slant_ok)
{

    // Local iconic variables
    HObject  ho_Rectangle2, ho_ImageReduced1, ho_ImageResult;
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
    HObject  ho_Rectangle5, ho_Rectangle18, ho_Contours, ho_Arrow1;
    HObject  ho_Arrow2, ho_Region, ho_RegionOpening6, ho_RegionClosing1;
    HObject  ho_RegionFillUp, ho_ConnectedRegions, ho_SelectedRegions;
    HObject  ho_SelectedRegions1, ho_RegionOpening2, ho_Rectangle4;
    HObject  ho_ImageReduced6, ho_Region2, ho_RegionOpening10;
    HObject  ho_RegionClosing4, ho_ImageReduced5, ho_Region6;
    HObject  ho_RegionOpening7, ho_ConnectedRegions9, ho_SelectedRegions9;
    HObject  ho_RegionUnion5, ho_ImageResult2, ho_RegionOpening13;
    HObject  ho_ConnectedRegions4, ho_SelectedRegions4, ho_RegionUnion2;
    HObject  ho_Rectangle_t, ho_ImageReduced3, ho_Region_t, ho_ImageReduced4;
    HObject  ho_Region5, ho_RegionOpening9, ho_RegionDifference;
    HObject  ho_RegionFillUp2, ho_RegionOpening5, ho_Rectangle_p;
    HObject  ho_Image_mult, ho_Region_p, ho_RegionClosing6, ho_RegionFillUp4;
    HObject  ho_Rectangle15, ho_RegionOpening12, ho_RegionClosing7;
    HObject  ho_ConnectedRegions8, ho_SelectedRegions8, ho_RegionUnion1;
    HObject  ho_Rectangle9, ho_RegionDifference1, ho_Rectangle16;
    HObject  ho_RegionOpening3, ho_RegionClosing2, ho_ConnectedRegions3;
    HObject  ho_SelectedRegions6, ho_RegionUnion10, ho_RegionDilation;
    HObject  ho_Rectangle7, ho_ImageReduced, ho_Region4, ho_ConnectedRegions6;
    HObject  ho_SelectedRegions5, ho_RegionUnion3, ho_RegionClosing10;
    HObject  ho_RegionFillUp1, ho_Rectangle1, ho_Rectangle, ho_Rectangle6;
    HObject  ho_ImageReduced2, ho_Region3, ho_ConnectedRegions1;
    HObject  ho_SelectedRegions3, ho_RegionUnion, ho_RegionClosing;
    HObject  ho_ConnectedRegions2, ho_SelectedRegions2;

    // Local control variables
    HTuple  hv_needle_width_out, hv_Row6, hv_Column6;
    HTuple  hv_Phi, hv_Length12, hv_Length22, hv_row_needle;
    HTuple  hv_col_needle, hv_Row113, hv_Column113, hv_Row213;
    HTuple  hv_Column213, hv_Row114, hv_Column114, hv_Row214;
    HTuple  hv_Column214, hv_UsedThreshold4, hv_Area5, hv_Row31;
    HTuple  hv_Column31, hv_Max7, hv_Area6, hv_Row32, hv_Column32;
    HTuple  hv_Row111, hv_Column111, hv_Row211, hv_Column211;
    HTuple  hv_b, hv_Column2, hv_Row2, hv_Row3, hv_Column3;
    HTuple  hv_IsOverlapping, hv_offset, hv_Area, hv_Row, hv_Column;
    HTuple  hv_Row8, hv_Column8, hv_Phi0, hv_Length1, hv_Length2;
    HTuple  hv_Number2, hv_Row7, hv_Column7, hv_Phi3, hv_Length14;
    HTuple  hv_Length24, hv_Row110, hv_Column110, hv_Row210;
    HTuple  hv_Column210, hv_Row112, hv_Column112, hv_Row212;
    HTuple  hv_Column212, hv_UsedThreshold1, hv_Area4, hv_Row20;
    HTuple  hv_Column20, hv_Max6, hv_Row30, hv_Column30, hv_Phi4;
    HTuple  hv_Length15, hv_Length25, hv_UsedThreshold, hv_top_check_height;
    HTuple  hv_row_dis_t, hv_col_dis_t, hv_UsedThreshold2, hv_Row16;
    HTuple  hv_Column16, hv_Row27, hv_Column27, hv_height_top_bottom_1;
    HTuple  hv_UsedThreshold3, hv_Row17, hv_Column17, hv_Row28;
    HTuple  hv_Column28, hv_height_top_bottom_2, hv_row_dis_p;
    HTuple  hv_col_dis_p, hv_Min, hv_Max1, hv_Range, hv_Area3;
    HTuple  hv_Row18, hv_Column18, hv_Max5, hv_Row9, hv_Column9;
    HTuple  hv_Phi2, hv_Length13, hv_Length23, hv_length_cut;
    HTuple  hv_Row11, hv_Column11, hv_Row21, hv_Column21, hv_Area2;
    HTuple  hv_Row10, hv_Column10, hv_Max, hv_Area1, hv_Row1;
    HTuple  hv_Column1, hv_Row14, hv_Column14, hv_Row24, hv_Column24;
    HTuple  hv_Row13, hv_Column13, hv_Row25, hv_Column25, hv_row_m;
    HTuple  hv_col_m, hv_leng_m, hv_Row4, hv_Column4, hv_Number;

    (*hv_is_top) = 0;
    (*hv_top_width) = 0;
    (*hv_is_fold) = 0;
    (*hv_Area_fold) = 0;
    (*hv_is_distance_ok) = 0;
    (*hv_distance) = 0;
    (*hv_is_slant_ok) = 0;
    (*hv_slant_width) = 0;
    hv_needle_width_out = 0;
    GenEmptyObj(&(*ho_Arrow_distance));
    GenEmptyObj(&(*ho_rect_slant));
    GenEmptyObj(&(*ho_Region_top));
    GenEmptyObj(&(*ho_region_fold));
    GenEmptyObj(&(*ho_needle_rect));
    if (0 != (hv_no_direction==1))
    {
      //计算针的角度
      GenRectangle2(&ho_Rectangle2, hv_rect_row, 1224, HTuple(90).TupleRad(), 300,
          1224);
      ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced1);
      MultImage(ho_ImageReduced1, ho_ImageReduced1, &ho_ImageResult, 0.5, 0);
      Threshold(ho_ImageResult, &ho_Region1, hv_melt_thr, 255);
      Connection(ho_Region1, &ho_ConnectedRegions12);
      SelectShape(ho_ConnectedRegions12, &ho_SelectedRegions13, "area", "and", 500,
          99999);
      SelectShape(ho_SelectedRegions13, &ho_SelectedRegions18, "height", "and", 100,
          99999);
      SelectShape(ho_SelectedRegions18, &ho_SelectedRegions19, "width", "and", 0, (hv_needle_width+(2*hv_pipe_thick))*2);
      Union1(ho_SelectedRegions19, &ho_RegionUnion7);
      SmallestRectangle2(ho_RegionUnion7, &hv_Row6, &hv_Column6, &hv_Phi, &hv_Length12,
          &hv_Length22);
      GenRectangle2(&ho_Rectangle3, hv_Row6, hv_Column6, hv_Phi, hv_Length12, hv_Length22);
      hv_row_needle = 1024;
      if (0 != (hv_Phi>0))
      {
        hv_col_needle = hv_Column6+(((hv_Row6-hv_row_needle)/2)*(hv_Phi.TupleCos()));
      }
      else
      {
        hv_col_needle = hv_Column6-(((hv_Row6-hv_row_needle)/2)*(hv_Phi.TupleCos()));
      }
      GenRectangle2(&ho_Rectangle10, hv_row_needle, hv_col_needle, hv_Phi, 1024, hv_Length22*2);

      //计算针区域
      ReduceDomain(ho_Image, ho_Rectangle10, &ho_ImageReduced8);
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
      ReduceDomain(ho_Image, ho_Rectangle11, &ho_ImageReduced9);
      MultImage(ho_ImageReduced9, ho_ImageReduced9, &ho_ImageResult1, 5, 0);
      BinaryThreshold(ho_ImageResult1, &ho_Region9, "max_separability", "light", &hv_UsedThreshold4);
      Connection(ho_Region9, &ho_ConnectedRegions13);
      SelectShape(ho_ConnectedRegions13, &ho_SelectedRegions14, "area", "and", hv_melt_min_area,
          999999999);

      Union1(ho_SelectedRegions14, &ho_RegionUnion8);
      GenRectangle2(&ho_Rectangle12, hv_Row6, hv_Column6, hv_Phi, 1, (hv_needle_width+hv_pipe_thick)/2);
      Closing(ho_RegionUnion8, ho_Rectangle12, &ho_RegionClosing3);
      GenRectangle2(&ho_Rectangle13, hv_Row6, hv_Column6, hv_Phi, 1, hv_needle_width/2);
      Opening(ho_RegionClosing3, ho_Rectangle13, &ho_RegionOpening11);
      Connection(ho_RegionOpening11, &ho_ConnectedRegions11);
      AreaCenter(ho_ConnectedRegions11, &hv_Area5, &hv_Row31, &hv_Column31);
      TupleMax(hv_Area5, &hv_Max7);
      SelectShape(ho_ConnectedRegions11, &ho_SelectedRegions12, "area", "and", hv_Max7-10,
          99999999999);
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
      hv_b = hv_Row114-(hv_Column211*((-hv_Phi).TupleTan()));
      hv_Column2 = hv_Column211+1;
      hv_Row2 = hv_b+(hv_Column2*((-hv_Phi).TupleTan()));
      IntersectionLineContourXld(ho_Contours, hv_Row114, hv_Column211, hv_Row2, hv_Column2,
          &hv_Row3, &hv_Column3, &hv_IsOverlapping);
      DistancePp(HTuple(hv_Row3[0]), HTuple(hv_Column3[0]), HTuple(hv_Row3[1]), HTuple(hv_Column3[1]),
          &(*hv_distance));
      hv_offset = 50;
      QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset,
          HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset, 10, 10);
      QtHalcon::gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset,
          HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset, 10, 10);
      ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_Arrow_distance));
      if (0 != (HTuple((*hv_distance)>hv_distance_min).TupleAnd((*hv_distance)<hv_distance_max)))
      {
        (*hv_is_distance_ok) = 1;
      }
    }
    else
    {
      Threshold(ho_Image, &ho_Region, hv_top_thr/2, 255);
      OpeningCircle(ho_Region, &ho_RegionOpening6, 3.5);
      ClosingRectangle1(ho_RegionOpening6, &ho_RegionClosing1, 10, 2);
      FillUp(ho_RegionClosing1, &ho_RegionFillUp);
      Connection(ho_RegionFillUp, &ho_ConnectedRegions);
      SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "height", "and", hv_top_h_min,
          hv_top_h_max);
      SelectShape(ho_SelectedRegions, &ho_SelectedRegions1, "width", "and", hv_top_w_min,
          hv_top_w_max);
      AreaCenter(ho_SelectedRegions1, &hv_Area, &hv_Row, &hv_Column);
      SmallestRectangle2(ho_SelectedRegions1, &hv_Row8, &hv_Column8, &hv_Phi0, &hv_Length1,
          &hv_Length2);
      CountObj(ho_SelectedRegions1, &hv_Number2);
      if (0 != (hv_Number2<1))
      {
        return;
      }
      //计算斜面宽度
      if (0 != (hv_check_slant==1))
      {
        //计算斜面宽度
        OpeningCircle(ho_SelectedRegions1, &ho_RegionOpening2, 3.5);
        SmallestRectangle2(ho_RegionOpening2, &hv_Row7, &hv_Column7, &hv_Phi3, &hv_Length14,
            &hv_Length24);
        GenRectangle2(&(*ho_rect_slant), hv_Row7, hv_Column7, hv_Phi3, hv_Length14,
            hv_Length24);
        //计算是否针尖上方有导管
        SmallestRectangle1(ho_SelectedRegions1, &hv_Row110, &hv_Column110, &hv_Row210,
            &hv_Column210);
        GenRectangle1(&ho_Rectangle4, 0, hv_Column110-(hv_Length24/2), hv_Row110-5,
            hv_Column210+(hv_Length24/2));
        ReduceDomain(ho_Image, ho_Rectangle4, &ho_ImageReduced6);
        MultImage(ho_ImageReduced6, ho_ImageReduced6, &ho_ImageReduced6, 5, 0);
        Threshold(ho_ImageReduced6, &ho_Region2, hv_melt_thr, 255);
        OpeningCircle(ho_Region2, &ho_RegionOpening10, 2.5);
        SmallestRectangle1(ho_RegionOpening10, &hv_Row112, &hv_Column112, &hv_Row212,
            &hv_Column212);
        //计算是否包针
        ClosingRectangle1(ho_SelectedRegions1, &ho_RegionClosing4, 20, 20);
        ReduceDomain(ho_Image, ho_RegionClosing4, &ho_ImageReduced5);
        BinaryThreshold(ho_ImageReduced5, &ho_Region6, "max_separability", "light",
            &hv_UsedThreshold1);
        OpeningCircle(ho_Region6, &ho_RegionOpening7, 2.0);
        Connection(ho_RegionOpening7, &ho_ConnectedRegions9);
        AreaCenter(ho_ConnectedRegions9, &hv_Area4, &hv_Row20, &hv_Column20);
        TupleMax(hv_Area4, &hv_Max6);
        SelectShape(ho_ConnectedRegions9, &ho_SelectedRegions9, "area", "and", hv_Max6-10,
            99999);
        Union1(ho_SelectedRegions9, &ho_RegionUnion5);
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


      //计算针的角度
      GenRectangle2(&ho_Rectangle2, hv_rect_row, hv_Column8, HTuple(90).TupleRad(),
          150, 800);
      ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced1);
      MultImage(ho_ImageReduced1, ho_ImageReduced1, &ho_ImageResult2, 1, 0);
      BinaryThreshold(ho_ImageResult2, &ho_Region1, "max_separability", "light", &hv_UsedThreshold);
      OpeningCircle(ho_Region1, &ho_RegionOpening13, 3.5);
      Connection(ho_RegionOpening13, &ho_ConnectedRegions4);
      SelectShape(ho_ConnectedRegions4, &ho_SelectedRegions4, "height", "and", 200,
          99999);
      Union1(ho_SelectedRegions4, &ho_RegionUnion2);
      SmallestRectangle2(ho_RegionUnion2, &hv_Row6, &hv_Column6, &hv_Phi, &hv_Length12,
          &hv_Length22);
      GenRectangle2(&ho_Rectangle3, hv_Row6, hv_Column6, hv_Phi, hv_Length12, hv_Length22);

      if (0 != (hv_check_dis==1))
      {
        //计算斜面区域
        hv_top_check_height = 40;
        hv_row_dis_t = hv_Row8+((hv_Length1-(hv_top_check_height/2))*((hv_Phi.TupleSin()).TupleAbs()));
        if (0 != (hv_Phi>0))
        {
          hv_col_dis_t = hv_Column8-((hv_Length1-(hv_top_check_height/2))*(hv_Phi.TupleCos()));
        }
        else
        {
          hv_col_dis_t = hv_Column8+((hv_Length1-(hv_top_check_height/2))*(hv_Phi.TupleCos()));
        }
        GenRectangle2(&ho_Rectangle_t, hv_row_dis_t, hv_col_dis_t, hv_Phi, hv_top_check_height/2,
            hv_Length2);
        ReduceDomain(ho_Image, ho_Rectangle_t, &ho_ImageReduced3);
        BinaryThreshold(ho_ImageReduced3, &ho_Region_t, "max_separability", "light",
            &hv_UsedThreshold2);
        Threshold(ho_ImageReduced3, &ho_Region_t, hv_UsedThreshold2*0.7, 255);
        OpeningCircle(ho_Region_t, &ho_Region_t, 3.5);
        SmallestRectangle1(ho_Region_t, &hv_Row16, &hv_Column16, &hv_Row27, &hv_Column27);
        hv_height_top_bottom_1 = hv_Row27-hv_Row16;
        ReduceDomain(ho_ImageReduced3, ho_Region_t, &ho_ImageReduced4);
        BinaryThreshold(ho_ImageReduced4, &ho_Region5, "max_separability", "dark",
            &hv_UsedThreshold3);
        OpeningCircle(ho_Region5, &ho_RegionOpening9, 2.0);
        Difference(ho_Region_t, ho_RegionOpening9, &ho_RegionDifference);
        FillUp(ho_RegionDifference, &ho_RegionFillUp2);
        OpeningRectangle1(ho_RegionFillUp2, &ho_RegionOpening5, 2, (hv_height_top_bottom_1/2)+5);
        SmallestRectangle1(ho_RegionOpening5, &hv_Row17, &hv_Column17, &hv_Row28, &hv_Column28);
        hv_height_top_bottom_2 = hv_Row28-hv_Row17;

        if (0 != (hv_height_top_bottom_2>=(hv_height_top_bottom_1*0.90)))
        {
          //计算管区域
          hv_row_dis_p = (hv_Row8+(hv_Length1*((hv_Phi.TupleSin()).TupleAbs())))+(hv_pos_p*((hv_Phi.TupleSin()).TupleAbs()));
          if (0 != (hv_Phi>0))
          {
            hv_col_dis_p = (hv_Column8-(hv_Length1*(hv_Phi.TupleCos())))-(hv_pos_p*(hv_Phi.TupleCos()));
          }
          else
          {
            hv_col_dis_p = (hv_Column8+(hv_Length1*(hv_Phi.TupleCos())))+(hv_pos_p*(hv_Phi.TupleCos()));
          }
          GenRectangle2(&ho_Rectangle_p, hv_row_dis_p, hv_col_dis_p, hv_Phi, hv_pos_p,
              hv_Length2*2);
          ReduceDomain(ho_Image, ho_Rectangle_p, &ho_ImageReduced3);
          MinMaxGray(ho_Rectangle_p, ho_ImageReduced3, 0, &hv_Min, &hv_Max1, &hv_Range);

          MultImage(ho_ImageReduced3, ho_ImageReduced3, &ho_Image_mult, 5, 0);
          Threshold(ho_Image_mult, &ho_Region_p, hv_melt_thr, 255);
          ClosingRectangle1(ho_Region_p, &ho_RegionClosing6, 15, 15);
          FillUp(ho_RegionClosing6, &ho_RegionFillUp4);
          GenRectangle2(&ho_Rectangle15, hv_Row6, hv_Column6, hv_Phi, 0.1, (hv_needle_width/2)+1);
          Opening(ho_RegionFillUp4, ho_Rectangle15, &ho_RegionOpening12);
          ClosingCircle(ho_RegionOpening12, &ho_RegionClosing7, 5.0);
          Connection(ho_RegionClosing7, &ho_ConnectedRegions8);
          AreaCenter(ho_ConnectedRegions8, &hv_Area3, &hv_Row18, &hv_Column18);
          TupleMax(hv_Area3, &hv_Max5);
          SelectShape(ho_ConnectedRegions8, &ho_SelectedRegions8, "area", "and", hv_Max5-10,
              99999);
          Union1(ho_SelectedRegions8, &ho_Rectangle_p);
          //smallest_rectangle2 (RegionUnion4, Row5, Column5, Phi1, Length11, Length21)
          //gen_rectangle2 (Rectangle_p, Row5, Column5, Phi1, Length11, Length21)
          //smallest_rectangle2 (RegionUnion4, Row19, Column19, Row29, Column29)
          //gen_rectangle1 (Rectangle_p, Row19, Column19, Row29, Column29)

          //计算莱距离区域
          Union2(ho_Region_t, ho_Rectangle_p, &ho_RegionUnion1);
          SmallestRectangle2(ho_RegionUnion1, &hv_Row9, &hv_Column9, &hv_Phi2, &hv_Length13,
              &hv_Length23);
          hv_length_cut = hv_Length13*((hv_Phi.TupleAbs()).TupleSin());
          GenRectangle2(&ho_Rectangle9, hv_Row9, hv_Column9, hv_Phi, hv_length_cut-10,
              hv_Length23-10);
          Difference(ho_Rectangle9, ho_RegionUnion1, &ho_RegionDifference1);
          GenRectangle2(&ho_Rectangle16, hv_Row9, hv_Column9, hv_Phi, 0.1, hv_Length23-11);
          Opening(ho_RegionDifference1, ho_Rectangle16, &ho_RegionOpening3);
          SmallestRectangle1(ho_RegionOpening3, &hv_Row11, &hv_Column11, &hv_Row21,
              &hv_Column21);
          ClosingRectangle1(ho_RegionOpening3, &ho_RegionClosing2, 1, ((hv_Row21-hv_Row11)/2)+1);
          Connection(ho_RegionClosing2, &ho_ConnectedRegions3);
          AreaCenter(ho_ConnectedRegions3, &hv_Area2, &hv_Row10, &hv_Column10);
          TupleMax(hv_Area2, &hv_Max);
          SelectShape(ho_ConnectedRegions3, &ho_SelectedRegions6, "area", "and", hv_Area2-5,
              99999999);
          Union1(ho_SelectedRegions6, &ho_RegionUnion10);
          AreaCenter(ho_RegionUnion10, &hv_Area1, &hv_Row1, &hv_Column1);
          if (0 != (hv_Area1<1))
          {
            GenRectangle1(&ho_RegionUnion10, 0, 0, 1, 1);
          }
          DilationCircle(ho_RegionUnion10, &ho_RegionDilation, 0.5);
          GenContourRegionXld(ho_RegionDilation, &ho_Contours, "border");

          hv_b = hv_Row1-(hv_Column1*((-hv_Phi).TupleTan()));
          hv_Column2 = hv_Column1+20;
          hv_Row2 = hv_b+(hv_Column2*((-hv_Phi).TupleTan()));
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
              10, 10);

          IntersectionLineContourXld(ho_Contours, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
              &hv_Row3, &hv_Column3, &hv_IsOverlapping);
          DistancePp(HTuple(hv_Row3[0]), HTuple(hv_Column3[0]), HTuple(hv_Row3[1]),
              HTuple(hv_Column3[1]), &(*hv_distance));
          hv_offset = 50;
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow1, HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset,
              HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset, 10, 10);
          QtHalcon::gen_arrow_contour_xld(&ho_Arrow2, HTuple(hv_Row3[1]), HTuple(hv_Column3[1])+hv_offset,
              HTuple(hv_Row3[0]), HTuple(hv_Column3[0])+hv_offset, 10, 10);
          ConcatObj(ho_Arrow1, ho_Arrow2, &(*ho_Arrow_distance));
          if (0 != (HTuple((*hv_distance)>hv_distance_min).TupleAnd((*hv_distance)<hv_distance_max)))
          {
            (*hv_is_distance_ok) = 1;
          }
        }
      }
      //检测针尖
      if (0 != (hv_check_top==1))
      {
        GenRectangle2(&ho_Rectangle7, hv_Row8, hv_Column8, hv_Phi, hv_Length1+10, hv_Length2+10);
        ReduceDomain(ho_Image, ho_Rectangle7, &ho_ImageReduced);
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
        Intersection(ho_RegionFillUp1, ho_Rectangle1, &(*ho_Region_top));
        SmallestRectangle1((*ho_Region_top), &hv_Row13, &hv_Column13, &hv_Row25, &hv_Column25);
        GenRectangle1(&ho_Rectangle, hv_Row13, hv_Column13, hv_Row25, hv_Column25);
        (*hv_top_width) = hv_Column25-hv_Column13;
        if (0 != ((*hv_top_width)<hv_top_width_min))
        {
          (*hv_is_top) = 1;
        }
      }
      //检测褶皱
      if (0 != (hv_check_fold==1))
      {
        hv_row_m = hv_Row8+(hv_fold_pos*((hv_Phi.TupleSin()).TupleAbs()));
        if (0 != (hv_Phi>0))
        {
          hv_col_m = hv_Column8-(hv_fold_pos*(hv_Phi.TupleCos()));
        }
        else
        {
          hv_col_m = hv_Column8+(hv_fold_pos*(hv_Phi.TupleCos()));
        }
        hv_leng_m = hv_height_fold_area/2;
        GenRectangle2(&ho_Rectangle6, hv_row_m, hv_col_m, hv_Phi, hv_leng_m, hv_Length2+hv_pipe_thick);
        ReduceDomain(ho_Image, ho_Rectangle6, &ho_ImageReduced2);
        VarThreshold(ho_ImageReduced2, &ho_Region3, 15, 15, 0.3, hv_thr_fold, "light");
        Connection(ho_Region3, &ho_ConnectedRegions1);
        SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions3, "area", "and", 10,
            99999);
        Union1(ho_SelectedRegions3, &ho_RegionUnion);
        ClosingRectangle1(ho_RegionUnion, &ho_RegionClosing, 10, 10);
        Connection(ho_RegionClosing, &ho_ConnectedRegions2);
        SelectShape(ho_ConnectedRegions2, &ho_SelectedRegions2, "area", "and", hv_fold_area_min,
            99999);
        SelectShape(ho_SelectedRegions2, &(*ho_region_fold), "width", "and", hv_needle_width/2,
            99999);
        AreaCenter((*ho_region_fold), &(*hv_Area_fold), &hv_Row4, &hv_Column4);
        CountObj((*ho_region_fold), &hv_Number);
        if (0 != (hv_Number<1))
        {
          (*hv_is_fold) = 1;
        }
      }
    }
    return;
}
