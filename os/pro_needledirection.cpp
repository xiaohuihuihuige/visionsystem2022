#include "pro_needledirection.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"

Pro_NeedleDirection::Pro_NeedleDirection(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}

Pro_NeedleDirection::~Pro_NeedleDirection()
{

}
//初始化参数
void Pro_NeedleDirection::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_NeedleDirection::openHalconWindow()
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
void Pro_NeedleDirection::readROI()
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
void Pro_NeedleDirection::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_NeedleDirection::readPar()
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
void Pro_NeedleDirection::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_NeedleDirection::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_NeedleDirection::slot_check(QString camName,QString step)
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

            HTuple hv_cant_height,hv_cant_width,hv_needle_angle,hv_cant_angle,
                    hv_angle_dif,hv_dir,hv_angle,hv_c,hv_r;
            HObject ho_cant_rect,ho_needle_rect;


            get_cant_rotate_angle (m_SceneImage, &ho_cant_rect, &ho_needle_rect,
                                   m_FloatParameter[QString::fromLocal8Bit("斜面高度小")],
                    m_FloatParameter[QString::fromLocal8Bit("斜面高度大")] ,
                    m_FloatParameter[QString::fromLocal8Bit("斜面宽度小")] ,
                    m_FloatParameter[QString::fromLocal8Bit("斜面宽度大")] ,
                    m_FloatParameter[QString::fromLocal8Bit("背景阈值")] , &hv_cant_height, &hv_cant_width,
                    &hv_needle_angle, &hv_cant_angle, &hv_angle_dif, &hv_dir,
                    &hv_angle, &hv_c, &hv_r);

            QString text;

            text=QString::fromLocal8Bit("斜面高度:")+QString::number(hv_cant_height.D());
            v_str_result.push_back(text);
            v_str_bool.push_back(1);

            text=QString::fromLocal8Bit("斜面宽度:")+QString::number(hv_cant_width.D());
            v_str_result.push_back(text);
            v_str_bool.push_back(1);

//            text=QString::fromLocal8Bit("针角度:")+QString::number(hv_needle_angle.D());
//            v_str_result.push_back(text);
//            v_str_bool.push_back(1);

//            text=QString::fromLocal8Bit("斜面角度:")+QString::number(hv_cant_angle.D());
//            v_str_result.push_back(text);
//            v_str_bool.push_back(1);

            text=QString::fromLocal8Bit("角度:")+QString::number(hv_angle.D());
            v_str_result.push_back(text);
            v_str_bool.push_back(1);


            text=QString::fromLocal8Bit("方向:")+QString::number(hv_dir.D());
            v_str_result.push_back(text);
            v_str_bool.push_back(1);

//            text=QString::fromLocal8Bit("斜面长度:")+QString::number(hv_c.D());
//            v_str_result.push_back(text);
//            v_str_bool.push_back(1);

//            text=QString::fromLocal8Bit("针宽度:")+QString::number(hv_r.D());
//            v_str_result.push_back(text);
//            v_str_bool.push_back(1);

            v_obj_result.push_back(ho_cant_rect);
            v_obj_bool.push_back(1);

            v_obj_result.push_back(ho_needle_rect);
            v_obj_bool.push_back(1);

            m_send_data<<hv_dir.I()<<int(hv_angle.D()*69.4444);

        }
        catch (HalconCpp::HException &e)
        {
            emit  signal_information_text(m_CamName,1,"halcon");
        }
        iter++;
    }
    send_result();
    m_runInfo=tr(QString::fromLocal8Bit("%1检测结果:%2").arg(m_CamName,QString::number(m_send_data[1])).toUtf8());
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
void Pro_NeedleDirection::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_NeedleDirection::writeImageAndReport()//写入图片和记录
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
void Pro_NeedleDirection::writeCheckInfoToImage()//写检测结果到图片
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
        emit signal_information_image(m_CamName,1,image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_NeedleDirection::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_NeedleDirection::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_NeedleDirection::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_NeedleDirection::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_NeedleDirection::slot_program_exit()
{

}
void Pro_NeedleDirection::get_cant_rotate_angle (HObject ho_Image, HObject *ho_cant_rect, HObject *ho_needle_rect,
                                                 HTuple hv_cant_height_min, HTuple hv_cant_height_max, HTuple hv_cant_width_min,
                                                 HTuple hv_cant_width_max, HTuple hv_back_thr, HTuple *hv_cant_height, HTuple *hv_cant_width,
                                                 HTuple *hv_needle_angle, HTuple *hv_cant_angle, HTuple *hv_angle_dif, HTuple *hv_dir,
                                                 HTuple *hv_angle, HTuple *hv_c, HTuple *hv_r)
{

    // Local iconic variables
    HObject  ho_Region, ho_RegionFillUp1, ho_RegionOpening1;
    HObject  ho_ConnectedRegions1, ho_SelectedRegions1, ho_Region1;
    HObject  ho_RegionFillUp, ho_RegionOpening, ho_RegionClosing;
    HObject  ho_ConnectedRegions, ho_SelectedRegions, ho_RegionUnion;
    HObject  ho_Rectangle, ho_RegionOpening2, ho_Contours;

    // Local control variables
    HTuple  hv_Area1, hv_Row2, hv_Column2, hv_Max1;
    HTuple  hv_Row3, hv_Column3, hv_Phi1, hv_Length11, hv_Length21;
    HTuple  hv_UsedThreshold, hv_Area, hv_Row, hv_Column, hv_Max;
    HTuple  hv_Row11, hv_Column11, hv_Row21, hv_Column21, hv_Row1;
    HTuple  hv_Column1, hv_Phi, hv_Length1, hv_Length2, hv_Row4;
    HTuple  hv_Column4, hv_Phi2, hv_Length12, hv_Length22, hv_b;
    HTuple  hv_Row_needle, hv_Column_needle, hv_IsOverlapping;
    HTuple  hv_Row_cant, hv_col_cant, hv_RowProj0, hv_ColProj0;
    HTuple  hv_RowProj1, hv_ColProj1, hv_Distance, hv_Distance1;
    HTuple  hv_Distance2, hv_Distance3;

    GenEmptyObj(&(*ho_cant_rect));
    GenEmptyObj(&(*ho_needle_rect));
    (*hv_cant_height) = 0;
    (*hv_cant_width) = 0;
    (*hv_needle_angle) = 0;
    (*hv_cant_angle) = 0;
    (*hv_angle_dif) = 0;
    (*hv_dir) = 0;
    (*hv_angle) = 0;
    (*hv_c) = 0;
    (*hv_r) = 0;
    //计算针区域
    Threshold(ho_Image, &ho_Region, hv_back_thr, 255);
    FillUp(ho_Region, &ho_RegionFillUp1);
    OpeningCircle(ho_RegionFillUp1, &ho_RegionOpening1, 3.5);
    Connection(ho_RegionOpening1, &ho_ConnectedRegions1);
    AreaCenter(ho_ConnectedRegions1, &hv_Area1, &hv_Row2, &hv_Column2);
    TupleMax(hv_Area1, &hv_Max1);
    SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions1, "area", "and", hv_Max1-100,
        99999999);
    SmallestRectangle2(ho_SelectedRegions1, &hv_Row3, &hv_Column3, &hv_Phi1, &hv_Length11,
        &hv_Length21);
    GenRectangle2(&(*ho_needle_rect), hv_Row3, hv_Column3, hv_Phi1, hv_Length11, hv_Length21);

    //threshold (Image, Region, back_thr, 255)
    //* closing_rectangle1 (Region, RegionClosing1, cant_width_min, cant_height_min)
    //* reduce_domain (Image, RegionClosing1, ImageReduced1)
    BinaryThreshold(ho_Image, &ho_Region1, "max_separability", "light", &hv_UsedThreshold);
    FillUp(ho_Region1, &ho_RegionFillUp);
    OpeningCircle(ho_RegionFillUp, &ho_RegionOpening, 3.5);
    ClosingCircle(ho_RegionOpening, &ho_RegionClosing, 3.5);
    Connection(ho_RegionClosing, &ho_ConnectedRegions);
    AreaCenter(ho_ConnectedRegions, &hv_Area, &hv_Row, &hv_Column);
    TupleMax(hv_Area, &hv_Max);
    SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_Max-100,
        9999999);
    Union1(ho_SelectedRegions, &ho_RegionUnion);
    SmallestRectangle1(ho_RegionUnion, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
    SmallestRectangle2(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Phi, &hv_Length1,
        &hv_Length2);
    GenRectangle2(&(*ho_cant_rect), hv_Row1, hv_Column1, hv_Phi, hv_Length1, hv_Length2);
    (*hv_cant_height) = hv_Row21-hv_Row11;
    (*hv_cant_width) = hv_Column21-hv_Column11;
    if (0 != ((*hv_cant_height)>hv_cant_height_max))
    {
      Threshold(ho_Image, &ho_Region1, 150, 255);
      FillUp(ho_Region1, &ho_RegionFillUp);
      OpeningCircle(ho_RegionFillUp, &ho_RegionOpening, 3.5);
      ClosingCircle(ho_RegionOpening, &ho_RegionClosing, 3.5);
      Connection(ho_RegionClosing, &ho_ConnectedRegions);
      AreaCenter(ho_ConnectedRegions, &hv_Area, &hv_Row, &hv_Column);
      TupleMax(hv_Area, &hv_Max);
      SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_Max-100,
          99999);
      Union1(ho_SelectedRegions, &ho_RegionUnion);
      SmallestRectangle1(ho_RegionUnion, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
      SmallestRectangle2(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Phi, &hv_Length1,
          &hv_Length2);
      GenRectangle2(&(*ho_cant_rect), hv_Row1, hv_Column1, hv_Phi, hv_Length1, hv_Length2);
      (*hv_cant_height) = hv_Row21-hv_Row11;
      (*hv_cant_width) = hv_Column21-hv_Column11;
    }
    GenRectangle2(&ho_Rectangle, hv_Row1, hv_Column1, hv_Phi, 0, hv_Length2/2);
    Opening(ho_RegionUnion, ho_Rectangle, &ho_RegionOpening2);
    SmallestRectangle2(ho_RegionOpening2, &hv_Row4, &hv_Column4, &hv_Phi2, &hv_Length12,
        &hv_Length22);

    if (0 != (HTuple(HTuple(HTuple(HTuple(HTuple((*hv_cant_height)>hv_cant_height_min).TupleAnd((*hv_cant_height)<hv_cant_height_max)).TupleAnd((*hv_cant_width)>hv_cant_width_min)).TupleAnd((*hv_cant_width)<hv_cant_width_max)).TupleAnd(hv_Length12>(hv_Length1/2))).TupleAnd(hv_Length2>(hv_Length21/2.5))))
    {
      //计算针直线
      GenContourRegionXld((*ho_needle_rect), &ho_Contours, "border");
      hv_b = hv_Row3-(hv_Column3*((-hv_Phi1).TupleTan()));
      hv_Column2 = hv_Column3+20;
      hv_Row2 = hv_b+(hv_Column2*((-hv_Phi1).TupleTan()));
      IntersectionLineContourXld(ho_Contours, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
          &hv_Row_needle, &hv_Column_needle, &hv_IsOverlapping);


      //计算斜面顶点和低点
      GenContourRegionXld((*ho_cant_rect), &ho_Contours, "border");
      hv_b = hv_Row1-(hv_Column1*((-hv_Phi).TupleTan()));
      hv_Column2 = hv_Column1+20;
      hv_Row2 = hv_b+(hv_Column2*((-hv_Phi).TupleTan()));
      IntersectionLineContourXld(ho_Contours, hv_Row1, hv_Column1, hv_Row2, hv_Column2,
          &hv_Row_cant, &hv_col_cant, &hv_IsOverlapping);
      ProjectionPl(HTuple(hv_Row_cant[0]), HTuple(hv_col_cant[0]), HTuple(hv_Row_needle[0]),
          HTuple(hv_Column_needle[0]), HTuple(hv_Row_needle[1]), HTuple(hv_Column_needle[1]),
          &hv_RowProj0, &hv_ColProj0);
      ProjectionPl(HTuple(hv_Row_cant[1]), HTuple(hv_col_cant[1]), HTuple(hv_Row_needle[0]),
          HTuple(hv_Column_needle[0]), HTuple(hv_Row_needle[1]), HTuple(hv_Column_needle[1]),
          &hv_RowProj1, &hv_ColProj1);
      //gen_arrow_contour_xld (Arrow1, RowProj0, ColProj0, RowProj1, ColProj1, 10, 10)
      //gen_arrow_contour_xld (Arrow2, Row_cant[0], col_cant[0], Row_cant[1], col_cant[1], 10, 10)
      //gen_arrow_contour_xld (Arrow2, Row_cant[1], col_cant[1], RowProj1, ColProj1, 10, 10)

      DistancePp(hv_RowProj0, hv_ColProj0, hv_RowProj1, hv_ColProj1, &hv_Distance);
      DistancePp(hv_RowProj0, hv_ColProj0, HTuple(hv_Row_cant[1]), HTuple(hv_col_cant[1]),
          &hv_Distance1);
      DistancePp(HTuple(hv_Row_cant[1]), HTuple(hv_col_cant[1]), hv_RowProj1, hv_ColProj1,
          &hv_Distance2);
      DistancePp(HTuple(hv_Row_cant[0]), HTuple(hv_col_cant[0]), hv_RowProj0, hv_ColProj0,
          &hv_Distance3);
      if (0 != ((hv_Phi1.TupleDeg())<0))
      {
        (*hv_needle_angle) = 180-((hv_Phi1.TupleDeg()).TupleAbs());
      }
      else
      {
        (*hv_needle_angle) = hv_Phi1.TupleDeg();
      }
      if (0 != ((hv_Phi.TupleDeg())<0))
      {
        (*hv_cant_angle) = 180-((hv_Phi.TupleDeg()).TupleAbs());
      }
      else
      {
        (*hv_cant_angle) = hv_Phi.TupleDeg();
      }
      (*hv_angle_dif) = (*hv_cant_angle)-(*hv_needle_angle);
      if (0 != ((*hv_angle_dif)>0))
      {
        (*hv_dir) = 1;
      }
      else
      {
        (*hv_dir) = 2;
      }

      //project_length := Distance1*sin(acos(Distance/Distance1))
      (*hv_c) = hv_Distance2;
      (*hv_r) = hv_Length21;
      (*hv_angle) = (((*hv_c)/(*hv_r)).TupleAsin()).TupleDeg();

    }
    else
    {
      if (0 != (HTuple(HTuple(((hv_Phi.TupleDeg()).TupleAbs())<3).TupleOr((90-((hv_Phi.TupleDeg()).TupleAbs()))<3)).TupleOr((((hv_Length1*2)-(hv_Length2*2)).TupleAbs())<50)))
      {
        (*hv_dir) = 1;
        (*hv_angle) = 180;
      }
      else
      {
        (*hv_angle) = 90;
        if (0 != ((hv_Phi.TupleDeg())>0))
        {
          (*hv_dir) = 2;
        }
        else
        {
          (*hv_dir) = 1;
        }
      }
    }
    return;
}
