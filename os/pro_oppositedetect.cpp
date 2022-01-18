#include "pro_oppositedetect.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
Pro_OppositeDetect::Pro_OppositeDetect(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    ng_num=0;
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_OppositeDetect::~Pro_OppositeDetect()
{
    slot_program_exit();
}
//初始化参数
void Pro_OppositeDetect::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
    readTypeInfo();
    readPar();
    openHalconWindow();
}
void Pro_OppositeDetect::openHalconWindow()
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
void Pro_OppositeDetect::readROI()
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
void Pro_OppositeDetect::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_OppositeDetect::readPar()
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
void Pro_OppositeDetect::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_OppositeDetect::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_OppositeDetect::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
    v_str_result.clear();
    v_str_bool.clear();
    v_obj_result.clear();
    v_obj_bool.clear();
    m_send_data.clear();
    _final_result=0;
    QList<int> check_result;
    try
    {
        HTuple hv_Success,hv_MovementOfObject,hv_Row,hv_Column,hv_Angle,hv_Score,hv_Obj1_To_Obj2;
        HObject ho_ModelAtNewPosition,ho_Cross;

        QtHalcon::checkModel(m_SceneImage, &ho_ModelAtNewPosition, &ho_Cross, map_model_par["modelroi_S"].modelID, map_model_par["modelroi_S"].numToFind,
                             map_model_par["modelroi_S"].angle_Start, map_model_par["modelroi_S"].min_Score,
                map_model_par["modelroi_S"].max_Overlap, map_model_par["modelroi_S"].sub_Pixel, map_model_par["modelroi_S"].num_Levels,
                             map_model_par["modelroi_S"].greediness, &hv_Success, &hv_MovementOfObject, &hv_Row, &hv_Column, &hv_Angle,
                             &hv_Score);
        QtHalcon::matrixFromTrans1ToTrans2(map_model_par["modelroi_s"].movementOfObject_Model_M, hv_MovementOfObject, &hv_Obj1_To_Obj2);
        v_obj_result.push_back(ho_ModelAtNewPosition);
        v_obj_bool.push_back(1);
        QMap<QString,HObject>::iterator iter=map_hobj.begin();
        while(iter!=map_hobj.end())
        {
            QString key=iter.key();
            HObject value=iter.value();
            try
            {
                QStringList name_type_list=key.split('#');
                if(name_type_list.size()<1)continue;
                if(name_type_list[0].mid(0,8)=="modelroi")
                {
                    iter++;
                    continue;
                }
                HTuple hv_Number,hv_is_right,hv_width,hv_height;
                HObject ho_SelectedRegions,ho_RegionAffineTrans,obj_concat;
                HalconCpp::AffineTransRegion(value, &ho_RegionAffineTrans, hv_Obj1_To_Obj2, "nearest_neighbor");
                check_side(m_SceneImage, ho_RegionAffineTrans, &ho_SelectedRegions,
                           m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔最小宽度")],
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔最大宽度")],
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔最小高度")],
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔最大高度")],
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔阈值低")],
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("孔阈值高")],
                        &hv_is_right,&hv_width,&hv_height);

                QString text;
                HTuple length;
                TupleLength(hv_width,&length);
                text=name_type_list[0]+QString::fromLocal8Bit("孔宽:")+QString::number(length.I()>0?hv_width.I():0);
                v_str_result.push_back(text);
                TupleLength(hv_width,&length);
                text=name_type_list[0]+QString::fromLocal8Bit("孔高:")+QString::number(length.I()>0?hv_height.I():0);
                v_str_result.push_back(text);
                if(hv_is_right.I()== 1 &&hv_width.I()>0&&hv_height.I()>0)
                {
                    v_obj_result.push_back(ho_SelectedRegions);
                    v_obj_bool.push_back(1);
                    check_result<<1;
                    v_str_bool.push_back(1);
                    v_str_bool.push_back(1);
                }
                else
                {
                    v_obj_result.push_back(ho_RegionAffineTrans);
                    v_obj_bool.push_back(2);
                    check_result<<2;
                    v_str_bool.push_back(2);
                    v_str_bool.push_back(2);
                }
            }
            catch (HalconCpp::HException &e)
            {
                check_result<<2;
                emit  signal_information_text(m_CamName,1,"halcon");
            }
            iter++;
        }
    }
    catch (HalconCpp::HException &e)
    {
        check_result<<2;
        emit  signal_information_text(m_CamName,1,"halcon");
    }
    if(check_result.contains(2))
    {
        _final_result=2;
    }
    else
    {
        _final_result=1;
    }
    if(_final_result==1)
    {
        m_send_data<<2<<1;
    }
    else
    {
        m_send_data<<1<<2;
    }
    send_result();

    m_runInfo=tr(QString::fromLocal8Bit("%1检测结果:%2").arg(m_CamName,QString::number(_final_result)).toUtf8());
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
void Pro_OppositeDetect::slot_read_image(QString cam,QString imgPath,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
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
void Pro_OppositeDetect::writeImageAndReport()//写入图片和记录
{
    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd")+"\\"+QString::number(_final_result);
    if(m_IntParameter[QString::fromLocal8Bit("保存检测图像")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_SceneImageShow,"jpg",timePath.toLocal8Bit().data());
    }
    if(m_IntParameter[QString::fromLocal8Bit("保存原始图像")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".png";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_OriginImage,"png",timePath.toLocal8Bit().data());
    }
}


//写或绘制检测信息到虚拟窗口
void Pro_OppositeDetect::writeCheckInfoToImage()//写检测结果到图片
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
        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/50).toLocal8Bit().data(),QString::number(width.I()/80).toLocal8Bit().data());
        for (int i=0;i<v_str_result.size();i++)
        {
            int size=height.I()/40+40;
            int x=20;
            int y=20+size*i;
            QString colorstr;
            v_str_bool[i]==1?colorstr="green":colorstr="red";
            QtHalcon::writeWindowString(hv_WindowHandle_UI,v_str_result[i].toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),y,x);
        }
        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/20).toLocal8Bit().data(),QString::number(width.I()/40).toLocal8Bit().data());
        if(_final_result==1)
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
        emit signal_information_image(m_CamName,_final_result,image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_OppositeDetect::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_OppositeDetect::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_OppositeDetect::slot_type_change(QString type)
{
    initGlobalPar(m_ProName,m_StationName,m_CamName,m_StepName,type);
}
//参数变化
void Pro_OppositeDetect::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_OppositeDetect::slot_program_exit()
{

}
void Pro_OppositeDetect::check_side (HObject ho_Image, HObject ho_ROI, HObject *ho_hole_region, HTuple hv_hole_width_min,
                                     HTuple hv_hole_width_max, HTuple hv_hole_height_min, HTuple hv_hole_height_max,
                                     HTuple hv_hole_thr_min, HTuple hv_hole_thr_max, HTuple *hv_is_right, HTuple *hv_width,
                                     HTuple *hv_height)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region, ho_RegionFillUp;
    HObject  ho_ConnectedRegions, ho_SelectedRegions1, ho_SelectedRegions2;

    // Local control variables
    HTuple  hv_Area, hv_Row, hv_Column, hv_Row1, hv_Column1;
    HTuple  hv_Row2, hv_Column2;

    (*hv_is_right) = 0;
    (*hv_width) = 0;
    (*hv_height) = 0;
    GenEmptyObj(&(*ho_hole_region));
    ReduceDomain(ho_Image, ho_ROI, &ho_ImageReduced);
    Threshold(ho_ImageReduced, &ho_Region, hv_hole_thr_min, hv_hole_thr_max);
    FillUp(ho_Region, &ho_RegionFillUp);
    Connection(ho_RegionFillUp, &ho_ConnectedRegions);
    SelectShape(ho_ConnectedRegions, &ho_SelectedRegions1, "width", "and", hv_hole_width_min,
                hv_hole_width_max);
    SelectShape(ho_SelectedRegions1, &ho_SelectedRegions2, "height", "and", hv_hole_height_min,
                hv_hole_height_max);
    Union1(ho_SelectedRegions2, &(*ho_hole_region));
    AreaCenter((*ho_hole_region), &hv_Area, &hv_Row, &hv_Column);
    SmallestRectangle1((*ho_hole_region), &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    (*hv_width) = hv_Column2-hv_Column1;
    (*hv_height) = hv_Row2-hv_Row1;
    if (0 != ((hv_Area.TupleLength())>0))
    {
        (*hv_is_right) = 1;
    }
    return;
}
