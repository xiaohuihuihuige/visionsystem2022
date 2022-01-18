#include "pro_gluewithseat.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
Pro_GlueWithSeat::Pro_GlueWithSeat(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}

Pro_GlueWithSeat::~Pro_GlueWithSeat()
{

}
//初始化参数
void Pro_GlueWithSeat::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_GlueWithSeat::openHalconWindow()
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
void Pro_GlueWithSeat::readROI()
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
void Pro_GlueWithSeat::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_GlueWithSeat::readPar()
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
void Pro_GlueWithSeat::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_GlueWithSeat::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_GlueWithSeat::slot_check(QString camName,QString step)
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
            QStringList name_type_list=key.split('#');
            if(name_type_list.size()<1)continue;

            HTuple hv_pipe_seat_area_width,hv_blank_area_height,hv_glue_area_pos,hv_is_glue_ok;
            HObject ho_pipe_seat_region,ho_blank_area,ho_glue_area,ho_rect_slant,ho_needle_rect;
            glue_check (m_SceneImage,  value, &ho_pipe_seat_region, &ho_blank_area, &ho_glue_area,
                        m_FloatParameter[QString::fromLocal8Bit("针座阈值")],
                    m_FloatParameter[QString::fromLocal8Bit("导管座检测高度")],
                    m_FloatParameter[QString::fromLocal8Bit("导管座宽度小")],
                    m_FloatParameter[QString::fromLocal8Bit("间隙检测高度")],
                    m_FloatParameter[QString::fromLocal8Bit("间隙闭运算高")],
                    m_FloatParameter[QString::fromLocal8Bit("间隙开运算宽")],
                    m_FloatParameter[QString::fromLocal8Bit("间隙高度小")] ,
                    m_FloatParameter[QString::fromLocal8Bit("点胶检测位置")],
                    m_FloatParameter[QString::fromLocal8Bit("点胶检测宽度")] ,
                    m_FloatParameter[QString::fromLocal8Bit("点胶检测高度")],
                    m_FloatParameter[QString::fromLocal8Bit("点胶阈值")],
                    m_FloatParameter[QString::fromLocal8Bit("点胶位置小")],
                    m_IntParameter[QString::fromLocal8Bit("针座类型")],
                    &hv_pipe_seat_area_width, &hv_blank_area_height, &hv_glue_area_pos, &hv_is_glue_ok);

            QString text;
            //判断导管座宽度
            text=QString::fromLocal8Bit("导管座宽度:")+QString::number(hv_pipe_seat_area_width.D(),'f',3);
            v_str_result.push_back(text);
            v_obj_result.push_back(ho_pipe_seat_region);
            if(hv_pipe_seat_area_width.D()>m_FloatParameter[QString::fromLocal8Bit("管道座宽度小")])
            {
                v_obj_bool.push_back(1);
                v_str_bool.push_back(1);
            }
            else
            {
                v_obj_bool.push_back(2);
                v_str_bool.push_back(2);
            }

            //判断间隙高度
            text=QString::fromLocal8Bit("间隙高度:")+QString::number(hv_blank_area_height.D(),'f',3);
            v_str_result.push_back(text);
            v_obj_result.push_back(ho_blank_area);
            if(hv_blank_area_height.D()<m_FloatParameter[QString::fromLocal8Bit("间隙高度小")])
            {
                v_obj_bool.push_back(1);
                v_str_bool.push_back(1);
            }
            else
            {
                v_obj_bool.push_back(2);
                v_str_bool.push_back(2);
            }

            //判断点胶
            text=QString::fromLocal8Bit("点胶高度:")+QString::number(hv_glue_area_pos.D(),'f',3);
            v_str_result.push_back(text);
            v_obj_result.push_back(ho_glue_area);
            if(hv_is_glue_ok.I()==1)
            {
                v_obj_bool.push_back(1);
                v_str_bool.push_back(1);
            }
            else
            {
                v_obj_bool.push_back(2);
                v_str_bool.push_back(2);
            }

            if(hv_is_glue_ok==1)
            {
                m_send_data<<1;
            }
            else
            {
                m_send_data<<2;
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
void Pro_GlueWithSeat::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_GlueWithSeat::writeImageAndReport()//写入图片和记录
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
void Pro_GlueWithSeat::writeCheckInfoToImage()//写检测结果到图片
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

        //ClearWindow(hv_WindowHandle_UI);
        emit signal_information_image(m_CamName,m_send_data[0],image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_GlueWithSeat::send_result()
{
    if(m_send_data[0]!=1)
    {
        if(m_IntParameter[QString::fromLocal8Bit("复制位")]==1)
            m_send_data<<m_send_data[0];
        emit signal_result(m_send_data,m_CamName,m_StepName);
    }

}
//选区变化
void Pro_GlueWithSeat::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_GlueWithSeat::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_GlueWithSeat::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_GlueWithSeat::slot_program_exit()
{

}
void Pro_GlueWithSeat::glue_check (HObject ho_Image, HObject ho_Rectangle, HObject *ho_pipe_seat_region,
                                   HObject *ho_blank_area, HObject *ho_glue_area, HTuple hv_seat_thr, HTuple hv_pipe_seat_check_height,
                                   HTuple hv_pipe_seat_width_min, HTuple hv_gap_check_height, HTuple hv_gap_close_h,
                                   HTuple hv_gap_open_w, HTuple hv_blank_height_min, HTuple hv_glue_pos, HTuple hv_glue_check_width,
                                   HTuple hv_glue_check_height, HTuple hv_glue_thr, HTuple hv_glue_pos_min, HTuple hv_needle_seat_type,
                                   HTuple *hv_pipe_seat_area_width, HTuple *hv_blank_area_height, HTuple *hv_glue_area_pos,
                                   HTuple *hv_is_glue_ok)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region, ho_RegionOpening;
    HObject  ho_RegionClosing, ho_RegionOpening1, ho_Rectangle6;
    HObject  ho_ImageReduced2, ho_Region2, ho_RegionOpening4;
    HObject  ho_Rectangle7, ho_RegionClosing1, ho_Rectangle1;
    HObject  ho_RegionIntersection, ho_Rectangle3, ho_RegionBottomHat;
    HObject  ho_RegionOpening2, ho_Rectangle2, ho_ImageReduced4;
    HObject  ho_RegionIntersection1, ho_RegionOpening5, ho_ConnectedRegions1;
    HObject  ho_SelectedRegions1, ho_RegionUnion1, ho_Rectangle5;
    HObject  ho_ImageReduced5, ho_Region4, ho_RegionOpening7;
    HObject  ho_RegionFillUp1, ho_RegionOpening8, ho_ConnectedRegions;
    HObject  ho_SelectedRegions, ho_RegionUnion, ho_Rectangle4;
    HObject  ho_RegionOpening3, ho_Rectangle8, ho_ImageReduced1;
    HObject  ho_Region3, ho_RegionOpening6, ho_Rectangle9, ho_RegionOpening9;
    HObject  ho_ImageResult, ho_Region1, ho_Rectangle10, ho_ConnectedRegions2;
    HObject  ho_SelectedRegions2;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2;
    HTuple  hv_Row, hv_Column, hv_Phi, hv_Length1, hv_Length2;
    HTuple  hv_Row11, hv_Column11, hv_Row21, hv_Column21, hv_Row13;
    HTuple  hv_Column13, hv_Row23, hv_Column23, hv_Row12, hv_Column12;
    HTuple  hv_Row22, hv_Column22, hv_Row5, hv_Column5, hv_Phi2;
    HTuple  hv_Length12, hv_Length22, hv_Max1, hv_Row14, hv_Column14;
    HTuple  hv_Row24, hv_Column24, hv_UsedThreshold, hv_Area;
    HTuple  hv_Row4, hv_Column4, hv_Max, hv_Row3, hv_Column3;
    HTuple  hv_Phi1, hv_Length11, hv_Length21, hv_Row15, hv_Column15;
    HTuple  hv_Row25, hv_Column25, hv_Row8, hv_Column8, hv_Phi3;
    HTuple  hv_Length13, hv_Length23, hv_Area3, hv_Row9, hv_Column9;
    HTuple  hv_Max2, hv_Area1, hv_Row6, hv_Column6, hv_Area2;
    HTuple  hv_Row7, hv_Column7;

    (*hv_pipe_seat_area_width) = 0;
    (*hv_blank_area_height) = 9999;
    (*hv_glue_area_pos) = 0;
    (*hv_is_glue_ok) = 0;
    GenEmptyObj(&(*ho_pipe_seat_region));
    GenEmptyObj(&(*ho_blank_area));
    GenEmptyObj(&(*ho_glue_area));
    if (0 != (hv_needle_seat_type==1))
    {
      //找到针座位置
      ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);
      Threshold(ho_ImageReduced, &ho_Region, 0, hv_seat_thr);
      OpeningCircle(ho_Region, &ho_RegionOpening, 3.5);
      SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
      ClosingRectangle1(ho_RegionOpening, &ho_RegionClosing, hv_Column2-hv_Column1,
          2);
      OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening1, (hv_Column2-hv_Column1)-100,
          2);
      SmallestRectangle2(ho_RegionOpening1, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1,
          &hv_Length2);

      //判断是否有导管座
      GenRectangle2(&ho_Rectangle6, hv_Row-hv_pipe_seat_check_height, hv_Column, 0,
          hv_Length1, hv_pipe_seat_check_height-50);
      ReduceDomain(ho_Image, ho_Rectangle6, &ho_ImageReduced2);
      Threshold(ho_ImageReduced2, &ho_Region2, 0, hv_seat_thr);
      OpeningCircle(ho_Region2, &ho_RegionOpening4, 3.5);
      SmallestRectangle1(ho_RegionOpening4, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+1, hv_Column21);
      Closing(ho_RegionOpening4, ho_Rectangle7, &ho_RegionClosing1);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+((hv_Row21-hv_Row11)/2),
          hv_Column11+1);
      Opening(ho_RegionOpening4, ho_Rectangle7, &(*ho_pipe_seat_region));
      SmallestRectangle1((*ho_pipe_seat_region), &hv_Row13, &hv_Column13, &hv_Row23,
          &hv_Column23);
      (*hv_pipe_seat_area_width) = hv_Column23-hv_Column13;
      if (0 != ((*hv_pipe_seat_area_width)<hv_pipe_seat_width_min))
      {
        return;
      }
      //计算是否导管座到位
      GenRectangle2(&ho_Rectangle1, hv_Row-hv_gap_check_height, hv_Column, 0, hv_Length1,
          hv_gap_check_height);
      Intersection(ho_Rectangle1, ho_RegionClosing, &ho_RegionIntersection);
      GenRectangle2(&ho_Rectangle3, hv_Row, hv_Column, 0, 1, hv_gap_close_h);
      BottomHat(ho_RegionIntersection, ho_Rectangle3, &ho_RegionBottomHat);
      OpeningRectangle1(ho_RegionBottomHat, &ho_RegionOpening2, hv_gap_open_w, 1);
      SmallestRectangle1(ho_RegionOpening2, &hv_Row12, &hv_Column12, &hv_Row22, &hv_Column22);
      GenRectangle1(&(*ho_blank_area), hv_Row12, hv_Column12, hv_Row22, hv_Column22);
      (*hv_blank_area_height) = hv_Row22-hv_Row12;
      if (0 != ((*hv_blank_area_height)>hv_blank_height_min))
      {
        return;
      }

      //计算是否无胶
      GenRectangle2(&ho_Rectangle2, hv_Row-hv_glue_pos, hv_Column, 0, hv_Length1, hv_glue_pos-hv_Length2);
      ReduceDomain(ho_ImageReduced2, ho_Rectangle2, &ho_ImageReduced4);
      Intersection(ho_RegionClosing, ho_Rectangle2, &ho_RegionIntersection1);
      OpeningRectangle1(ho_RegionIntersection1, &ho_RegionOpening5, (*hv_pipe_seat_area_width)+20,
          hv_Length2);
      Connection(ho_RegionOpening5, &ho_ConnectedRegions1);
      SmallestRectangle2(ho_ConnectedRegions1, &hv_Row5, &hv_Column5, &hv_Phi2, &hv_Length12,
          &hv_Length22);
      TupleMax(hv_Length12, &hv_Max1);
      SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions1, "rect2_len1", "and",
          hv_Max1-10, 99999);
      Union1(ho_SelectedRegions1, &ho_RegionUnion1);
      SmallestRectangle1(ho_RegionUnion1, &hv_Row14, &hv_Column14, &hv_Row24, &hv_Column24);
      GenRectangle2(&ho_Rectangle5, hv_Row14-(hv_glue_check_height/2), (hv_Column14+hv_Column24)/2,
          0, hv_glue_check_width/2, hv_glue_check_height/2);
      ReduceDomain(ho_Image, ho_Rectangle5, &ho_ImageReduced5);
      //threshold (ImageReduced5, Region4, 0, 50)
      BinaryThreshold(ho_ImageReduced5, &ho_Region4, "max_separability", "dark", &hv_UsedThreshold);
      OpeningCircle(ho_Region4, &ho_RegionOpening7, 3.5);
      FillUp(ho_RegionOpening7, &ho_RegionFillUp1);
      OpeningRectangle1(ho_RegionFillUp1, &ho_RegionOpening8, hv_glue_check_width*0.8,
          2);
      Connection(ho_RegionOpening8, &ho_ConnectedRegions);
      AreaCenter(ho_ConnectedRegions, &hv_Area, &hv_Row4, &hv_Column4);
      TupleMax(hv_Area, &hv_Max);
      SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_Max-20,
          99999);
      Union1(ho_SelectedRegions, &ho_RegionUnion);
      SmallestRectangle2(ho_RegionUnion, &hv_Row3, &hv_Column3, &hv_Phi1, &hv_Length11,
          &hv_Length21);
      GenRectangle2(&(*ho_glue_area), hv_Row3, hv_Column3, hv_Phi1, hv_Length11, hv_Length21);
      (*hv_glue_area_pos) = hv_Row-hv_Row3;
      if (0 != ((*hv_glue_area_pos)<=hv_glue_pos_min))
      {
        (*hv_is_glue_ok) = 0;
      }
      else
      {
        (*hv_is_glue_ok) = 1;
        return;
      }
    }
    else if (0 != (hv_needle_seat_type==2))
    {
      //找到针位置
      ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);
      Threshold(ho_ImageReduced, &ho_Region, 0, hv_seat_thr);
      OpeningCircle(ho_Region, &ho_RegionOpening, 3.5);
      SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
      ClosingRectangle1(ho_RegionOpening, &ho_RegionClosing, hv_Column2-hv_Column1,
          2);
      //导管座位置
      OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening1, (hv_Column2-hv_Column1)-30,
          2);
      SmallestRectangle2(ho_RegionOpening1, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1,
          &hv_Length2);
      //判断是否有导管座
      GenRectangle2(&ho_Rectangle6, hv_Row-hv_pipe_seat_check_height, hv_Column, hv_Phi,
          hv_Length1, hv_pipe_seat_check_height-50);
      ReduceDomain(ho_Image, ho_Rectangle6, &ho_ImageReduced2);
      Threshold(ho_ImageReduced2, &ho_Region2, 0, hv_seat_thr);
      OpeningCircle(ho_Region2, &ho_RegionOpening4, 3.5);
      SmallestRectangle1(ho_RegionOpening4, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+1, hv_Column21);
      Closing(ho_RegionOpening4, ho_Rectangle7, &ho_RegionClosing1);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+((hv_Row21-hv_Row11)/2),
          hv_Column11+1);
      Opening(ho_RegionOpening4, ho_Rectangle7, &(*ho_pipe_seat_region));
      SmallestRectangle1((*ho_pipe_seat_region), &hv_Row13, &hv_Column13, &hv_Row23,
          &hv_Column23);
      (*hv_pipe_seat_area_width) = hv_Column23-hv_Column13;
      if (0 != ((*hv_pipe_seat_area_width)<hv_pipe_seat_width_min))
      {
        return;
      }

      //针座位置
      GenRectangle1(&ho_Rectangle4, hv_Row+hv_Length2, hv_Column1, (hv_Row+hv_Length2)+hv_pipe_seat_check_height,
          hv_Column2);
      Intersection(ho_Rectangle4, ho_RegionClosing, &ho_RegionIntersection);
      OpeningRectangle1(ho_RegionIntersection, &ho_RegionOpening3, hv_pipe_seat_width_min,
          hv_Length2*2);
      SmallestRectangle1(ho_RegionOpening3, &hv_Row15, &hv_Column15, &hv_Row25, &hv_Column25);
      //计算角度
      GenRectangle1(&ho_Rectangle8, hv_Row15+100, ((hv_Column15+hv_Column25)/2)-(hv_glue_check_width/3),
          hv_Row25+100, ((hv_Column15+hv_Column25)/2)+(hv_glue_check_width/3));
      ReduceDomain(ho_Image, ho_Rectangle8, &ho_ImageReduced1);
      Threshold(ho_ImageReduced1, &ho_Region3, 0, 80);
      //opening_rectangle1 (Region3, RegionOpening6, 5, 50)
      OpeningCircle(ho_Region3, &ho_RegionOpening6, 5);
      SmallestRectangle2(ho_RegionOpening6, &hv_Row8, &hv_Column8, &hv_Phi3, &hv_Length13,
          &hv_Length23);
      GenRectangle2(&ho_Rectangle9, hv_Row8, hv_Column8, hv_Phi3, hv_Length13, hv_Length23);
      //计算间隙高度
      GenRectangle1(&(*ho_blank_area), hv_Row+hv_Length2, hv_Column15, hv_Row15, hv_Column25);
      (*hv_blank_area_height) = hv_Row15-(hv_Row+hv_Length2);
      if (0 != ((*hv_blank_area_height)>hv_blank_height_min))
      {
        return;
      }
      //计算是否无胶
      GenRectangle2(&ho_Rectangle2, hv_Row-hv_glue_pos, hv_Column, 0, hv_Length1, hv_glue_pos+hv_Length2);
      ReduceDomain(ho_ImageReduced2, ho_Rectangle2, &ho_ImageReduced4);
      Intersection(ho_RegionClosing, ho_Rectangle2, &ho_RegionIntersection1);
      OpeningRectangle1(ho_RegionIntersection1, &ho_RegionOpening5, (*hv_pipe_seat_area_width)+20,
          hv_Length2);
      Connection(ho_RegionOpening5, &ho_ConnectedRegions1);
      SmallestRectangle2(ho_ConnectedRegions1, &hv_Row5, &hv_Column5, &hv_Phi2, &hv_Length12,
          &hv_Length22);
      TupleMax(hv_Length12, &hv_Max1);
      SelectShape(ho_ConnectedRegions1, &ho_SelectedRegions1, "rect2_len1", "and",
          hv_Max1-10, 99999);
      Union1(ho_SelectedRegions1, &ho_RegionUnion1);
      SmallestRectangle1(ho_RegionUnion1, &hv_Row14, &hv_Column14, &hv_Row24, &hv_Column24);
      GenRectangle2(&ho_Rectangle5, hv_Row14-(hv_glue_check_height/2), (hv_Column14+hv_Column24)/2,
          0, hv_glue_check_width/2, hv_glue_check_height/2);
      ReduceDomain(ho_Image, ho_Rectangle5, &ho_ImageReduced5);
      //threshold (ImageReduced5, Region4, 0, 50)
      BinaryThreshold(ho_ImageReduced5, &ho_Region4, "max_separability", "dark", &hv_UsedThreshold);
      OpeningCircle(ho_Region4, &ho_RegionOpening7, 3.5);
      FillUp(ho_RegionOpening7, &ho_RegionFillUp1);
      OpeningRectangle1(ho_RegionFillUp1, &ho_RegionOpening8, hv_glue_check_width*0.8,
          2);
      Connection(ho_RegionOpening8, &ho_ConnectedRegions);
      AreaCenter(ho_ConnectedRegions, &hv_Area, &hv_Row4, &hv_Column4);
      TupleMax(hv_Area, &hv_Max);
      SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_Max-20,
          99999);
      Union1(ho_SelectedRegions, &ho_RegionUnion);
      SmallestRectangle2(ho_RegionUnion, &hv_Row3, &hv_Column3, &hv_Phi1, &hv_Length11,
          &hv_Length21);
      GenRectangle2(&(*ho_glue_area), hv_Row3, hv_Column3, hv_Phi1, hv_Length11, hv_Length21);
      (*hv_glue_area_pos) = hv_Row-hv_Row3;
      if (0 != ((*hv_glue_area_pos)<=hv_glue_pos_min))
      {
        (*hv_is_glue_ok) = 0;
      }
      else
      {
        (*hv_is_glue_ok) = 1;
        return;
      }
    }
    else if (0 != (hv_needle_seat_type==3))
    {
      //找到针位置
      ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);
      Threshold(ho_ImageReduced, &ho_Region, 0, hv_seat_thr);
      OpeningCircle(ho_Region, &ho_RegionOpening, 3.5);
      SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
      ClosingRectangle1(ho_RegionOpening, &ho_RegionClosing, hv_Column2-hv_Column1,
          2);
      //导管座位置
      OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening1, (hv_Column2-hv_Column1)-100,
          2);
      SmallestRectangle2(ho_RegionOpening1, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1,
          &hv_Length2);
      //判断是否有导管座
      GenRectangle2(&ho_Rectangle6, hv_Row-hv_pipe_seat_check_height, hv_Column, hv_Phi,
          hv_Length1, hv_pipe_seat_check_height-50);
      ReduceDomain(ho_Image, ho_Rectangle6, &ho_ImageReduced2);
      Threshold(ho_ImageReduced2, &ho_Region2, 0, hv_seat_thr);
      OpeningCircle(ho_Region2, &ho_RegionOpening4, 3.5);
      SmallestRectangle1(ho_RegionOpening4, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+1, hv_Column21);
      Closing(ho_RegionOpening4, ho_Rectangle7, &ho_RegionClosing1);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+((hv_Row21-hv_Row11)/2),
          hv_Column11+1);
      Opening(ho_RegionOpening4, ho_Rectangle7, &(*ho_pipe_seat_region));
      SmallestRectangle1((*ho_pipe_seat_region), &hv_Row13, &hv_Column13, &hv_Row23,
          &hv_Column23);
      (*hv_pipe_seat_area_width) = hv_Column23-hv_Column13;
      if (0 != ((*hv_pipe_seat_area_width)<hv_pipe_seat_width_min))
      {
        return;
      }

      //针座位置
      GenRectangle1(&ho_Rectangle4, hv_Row+hv_Length2, hv_Column1, (hv_Row+hv_Length2)+hv_pipe_seat_check_height,
          hv_Column2);
      Intersection(ho_Rectangle4, ho_RegionClosing, &ho_RegionIntersection);
      OpeningRectangle1(ho_RegionIntersection, &ho_RegionOpening3, 1, hv_pipe_seat_check_height/3);
      SmallestRectangle1(ho_RegionOpening3, &hv_Row15, &hv_Column15, &hv_Row25, &hv_Column25);
      OpeningRectangle1(ho_RegionOpening3, &ho_RegionOpening9, (hv_Column25-hv_Column15)*0.8,
          10);
      SmallestRectangle1(ho_RegionOpening9, &hv_Row15, &hv_Column15, &hv_Row25, &hv_Column25);
      //计算角度
      GenRectangle1(&ho_Rectangle8, hv_Row15+200, ((hv_Column15+hv_Column25)/2)-(hv_glue_check_width/2),
          hv_Row25+200, ((hv_Column15+hv_Column25)/2)+(hv_glue_check_width/2));
      ReduceDomain(ho_Image, ho_Rectangle8, &ho_ImageReduced1);
      Threshold(ho_ImageReduced1, &ho_Region3, 0, 128);
      OpeningCircle(ho_Region3, &ho_RegionOpening6, 3.5);
      SmallestRectangle2(ho_RegionOpening6, &hv_Row8, &hv_Column8, &hv_Phi3, &hv_Length13,
          &hv_Length23);
      GenRectangle2(&ho_Rectangle9, hv_Row8, hv_Column8, hv_Phi3, hv_Length13, hv_Length23);
      //计算间隙高度
      GenRectangle1(&(*ho_blank_area), hv_Row+hv_Length2, hv_Column15, hv_Row15, hv_Column25);
      (*hv_blank_area_height) = hv_Row15-(hv_Row+hv_Length2);
      if (0 != ((*hv_blank_area_height)>hv_blank_height_min))
      {
        return;
      }
      //计算是否无胶
      GenRectangle2(&ho_Rectangle2, hv_Row15-hv_glue_pos, hv_Column, hv_Phi3, hv_glue_check_height/2,
          hv_glue_check_width/2);
      ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced4);
      MultImage(ho_ImageReduced4, ho_ImageReduced4, &ho_ImageResult, 0.01, 0);


      Threshold(ho_ImageReduced4, &ho_Region1, 0, hv_glue_thr);
      OpeningCircle(ho_Region1, &ho_RegionOpening5, 3.5);
      FillUp(ho_RegionOpening5, &(*ho_glue_area));
      GenRectangle2(&ho_Rectangle10, hv_Row8, hv_Column8, hv_Phi3, hv_glue_check_height/8,
          hv_glue_check_width/8);
      Opening((*ho_glue_area), ho_Rectangle10, &(*ho_glue_area));
      Connection((*ho_glue_area), &ho_ConnectedRegions2);
      AreaCenter(ho_ConnectedRegions2, &hv_Area3, &hv_Row9, &hv_Column9);
      TupleMax(hv_Area3, &hv_Max2);
      SelectShape(ho_ConnectedRegions2, &ho_SelectedRegions2, "area", "and", hv_Max2-20,
          9999999);
      Union1(ho_SelectedRegions2, &(*ho_glue_area));
      AreaCenter((*ho_glue_area), &hv_Area1, &hv_Row6, &hv_Column6);
      AreaCenter(ho_Rectangle2, &hv_Area2, &hv_Row7, &hv_Column7);
      (*hv_glue_area_pos) = (hv_Area1*1.0)/hv_Area2;
      if (0 != ((*hv_glue_area_pos)>=hv_glue_pos_min))
      {
        (*hv_is_glue_ok) = 0;
      }
      else
      {
        (*hv_is_glue_ok) = 1;
        return;
      }
    }
    else if (0 != (hv_needle_seat_type==4))
    {
      //找到针座位置
      ReduceDomain(ho_Image, ho_Rectangle, &ho_ImageReduced);
      Threshold(ho_ImageReduced, &ho_Region, 0, hv_seat_thr);
      OpeningCircle(ho_Region, &ho_RegionOpening, 3.5);
      SmallestRectangle1(ho_RegionOpening, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
      ClosingRectangle1(ho_RegionOpening, &ho_RegionClosing, hv_Column2-hv_Column1,
          2);
      OpeningRectangle1(ho_RegionClosing, &ho_RegionOpening1, (hv_Column2-hv_Column1)-100,
          2);
      SmallestRectangle2(ho_RegionOpening1, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1,
          &hv_Length2);

      //判断是否有导管座
      GenRectangle2(&ho_Rectangle6, hv_Row-hv_pipe_seat_check_height, hv_Column, 0,
          hv_Length1, hv_pipe_seat_check_height-50);
      ReduceDomain(ho_Image, ho_Rectangle6, &ho_ImageReduced2);
      Threshold(ho_ImageReduced2, &ho_Region2, 0, hv_seat_thr);
      OpeningCircle(ho_Region2, &ho_RegionOpening4, 3.5);
      SmallestRectangle1(ho_RegionOpening4, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+1, hv_Column21);
      Closing(ho_RegionOpening4, ho_Rectangle7, &ho_RegionClosing1);
      GenRectangle1(&ho_Rectangle7, hv_Row11, hv_Column11, hv_Row11+((hv_Row21-hv_Row11)/2),
          hv_Column11+1);
      Opening(ho_RegionOpening4, ho_Rectangle7, &(*ho_pipe_seat_region));
      SmallestRectangle1((*ho_pipe_seat_region), &hv_Row13, &hv_Column13, &hv_Row23,
          &hv_Column23);
      (*hv_pipe_seat_area_width) = hv_Column23-hv_Column13;
      if (0 != ((*hv_pipe_seat_area_width)<hv_pipe_seat_width_min))
      {
        return;
      }
      //计算是否导管座到位
      GenRectangle2(&ho_Rectangle1, hv_Row-hv_gap_check_height, hv_Column, 0, hv_Length1,
          hv_gap_check_height);
      Intersection(ho_Rectangle1, ho_RegionClosing, &ho_RegionIntersection);
      GenRectangle2(&ho_Rectangle3, hv_Row, hv_Column, 0, 1, hv_gap_close_h);
      BottomHat(ho_RegionIntersection, ho_Rectangle3, &ho_RegionBottomHat);
      OpeningRectangle1(ho_RegionBottomHat, &ho_RegionOpening2, hv_gap_open_w, 1);
      SmallestRectangle1(ho_RegionOpening2, &hv_Row12, &hv_Column12, &hv_Row22, &hv_Column22);
      GenRectangle1(&(*ho_blank_area), hv_Row12, hv_Column12, hv_Row22, hv_Column22);
      (*hv_blank_area_height) = hv_Row22-hv_Row12;
      if (0 != ((*hv_blank_area_height)>hv_blank_height_min))
      {
        return;
      }
      //计算是否无胶
      GenRectangle2(&ho_Rectangle2, hv_Row-hv_glue_pos, hv_Column, HTuple(90).TupleRad(),
          hv_glue_check_height/2, hv_glue_check_width/2);
      ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced4);
      MultImage(ho_ImageReduced4, ho_ImageReduced4, &ho_ImageResult, 0.01, 0);
      Threshold(ho_ImageReduced4, &ho_Region1, 0, hv_glue_thr);
      OpeningCircle(ho_Region1, &ho_RegionOpening5, 3.5);
      FillUp(ho_RegionOpening5, &(*ho_glue_area));
      GenRectangle2(&ho_Rectangle10, hv_Row, hv_Column, HTuple(90).TupleRad(), hv_glue_check_height/8,
          hv_glue_check_width/8);
      Opening((*ho_glue_area), ho_Rectangle10, &(*ho_glue_area));
      Connection((*ho_glue_area), &ho_ConnectedRegions2);
      AreaCenter(ho_ConnectedRegions2, &hv_Area3, &hv_Row9, &hv_Column9);
      TupleMax(hv_Area3, &hv_Max2);
      SelectShape(ho_ConnectedRegions2, &ho_SelectedRegions2, "area", "and", hv_Max2-20,
          9999999);
      Union1(ho_SelectedRegions2, &(*ho_glue_area));
      AreaCenter((*ho_glue_area), &hv_Area1, &hv_Row6, &hv_Column6);
      AreaCenter(ho_Rectangle2, &hv_Area2, &hv_Row7, &hv_Column7);
      (*hv_glue_area_pos) = (hv_Area1*1.0)/hv_Area2;
      if (0 != ((*hv_glue_area_pos)>=hv_glue_pos_min))
      {
        (*hv_is_glue_ok) = 0;
      }
      else
      {
        (*hv_is_glue_ok) = 1;
        return;
      }
    }
    return;
}

