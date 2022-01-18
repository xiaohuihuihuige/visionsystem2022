#include"deeplearning/Classified/classified.h"
#include "pro_heparingcap.h"
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

Pro_HeparingCap::Pro_HeparingCap(QObject *parent) : QObject(parent)
  ,m_openvino(nullptr)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_HeparingCap::~Pro_HeparingCap()
{
    slot_program_exit();
}
//初始化参数
void Pro_HeparingCap::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_HeparingCap::openHalconWindow()
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
void Pro_HeparingCap::readROI()
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
void Pro_HeparingCap::readTypeInfo()
{
    //slot_program_exit();

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
        m_openvino=new classfiedopenvino(nullptr,0,"CPU",weights,name_file);
    }
}
//根据启用的品类读取参数信息
void Pro_HeparingCap::readPar()
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
void Pro_HeparingCap::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_HeparingCap::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_HeparingCap::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
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
            HalconCpp::HObject image_part;
            HObject image_reduce,rect;
            get_check_roi(m_SceneImage,value,&rect,&image_part,m_FloatParameter[QString::fromLocal8Bit("实体低阈值")],
                    m_FloatParameter[QString::fromLocal8Bit("实体高阈值")],m_IntParameter[QString::fromLocal8Bit("裁剪尺寸")]);
            //checkFunc
            cv::Mat check_img=QtHalcon::HObjectToMat(image_part);
            if(check_img.empty())return;
            if(m_openvino==nullptr)
            {
                emit  signal_information_text(m_CamName,1,QString::fromLocal8Bit("图像分类未初始化"));
                return;
            }
            m_openvino->setCheckPar(check_img,m_IntParameter[QString::fromLocal8Bit("图像大小")]);
            QString type=m_openvino->Run();
            v_obj_result<<rect;
            v_obj_bool.push_back(type.toInt());

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
    if(v_obj_bool.size()>0)
    {
        m_send_data<<v_obj_bool[0];
    }
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

}
void Pro_HeparingCap::slot_check_result(int num,QString name)
{

}
//读取文件
void Pro_HeparingCap::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_HeparingCap::writeImageAndReport()//写入图片和记录
{
    QDateTime date=QDateTime::currentDateTime();
    QString dateDir;
    dateDir=m_ReportPath+"\\"+date.toString("yyMMdd")+"\\"+QString::number(v_obj_bool[0]);
    if(m_IntParameter[QString::fromLocal8Bit("保存检测图像")]==1)
    {
        QString timePath=dateDir+"\\"+date.toString("hhmmss")+".jpg";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_SceneImage,"jpg",timePath.toLocal8Bit().data());
    }
    if(m_IntParameter[QString::fromLocal8Bit("保存原始图像")]==1)
    {
        QString timePath=dateDir+"\\origin"+date.toString("hhmmss")+".png";
        GeneralFunc::isDirExist(dateDir,true);
        QtHalcon::saveHObject(m_OriginImage,"png",timePath.toLocal8Bit().data());
    }
}


//写或绘制检测信息到虚拟窗口
void Pro_HeparingCap::writeCheckInfoToImage()//写检测结果到图片
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
            QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/30).toLocal8Bit().data(),QString::number(width.I()/70).toLocal8Bit().data());
            QtHalcon::writeWindowString(hv_WindowHandle_UI,QString::number(v_obj_bool[i]).toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),roistrlist[0].toInt()+marginwidth,roistrlist[1].toInt()+marginwidth);

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

//发送结果
void Pro_HeparingCap::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_HeparingCap::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_HeparingCap::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_HeparingCap::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_HeparingCap::slot_program_exit()
{
    if(m_openvino!=nullptr)
    {
        delete m_openvino;
        m_openvino=nullptr;
    }
}

//检测
/*void Pro_HeparingCap::getHeparingRegion (HObject ho_Image, HObject ho_ROI_0, HObject *ho_Rectangle,
                                         HObject *ho_ImagePart, HTuple hv_back_thr_min, HTuple hv_back_open_size, HTuple hv_back_close_size,
                                         HTuple hv_thr_diff, HTuple hv_select_area_min, HTuple hv_min_width, HTuple hv_obj_thr_min,
                                         HTuple hv_obj_thr_max, HTuple hv_obj_open_size, HTuple hv_cut_size)
                                     {

                                       // Local iconic variables
                                       HObject  ho_ImageReduced3, ho_Region, ho_RegionOpening;
                                       HObject  ho_RegionClosing, ho_Rectangle2, ho_ImageReduced;
                                       HObject  ho_ImageGray, ho_ImageR, ho_ImageG, ho_ImageB, ho_ImageH_M;
                                       HObject  ho_ImageS_M, ho_ImageV_M, ho_Region1, ho_ConnectedRegions;
                                       HObject  ho_SelectedRegions, ho_RegionUnion, ho_Rectangle1;
                                       HObject  ho_ImageReduced1, ho_Region2, ho_RegionOpening1;
                                       HObject  ho_ImageReduced2;

                                       // Local control variables
                                       HTuple  hv_Row11, hv_Column11, hv_Row21, hv_Column21;
                                       HTuple  hv_Area, hv_Row, hv_Column, hv_Row1, hv_Column1;
                                       HTuple  hv_Row2, hv_Column2, hv_center_row, hv_center_col;

                                       ReduceDomain(ho_Image, ho_ROI_0, &ho_ImageReduced3);
                                       Threshold(ho_ImageReduced3, &ho_Region, hv_back_thr_min, 255);
                                       OpeningRectangle1(ho_Region, &ho_RegionOpening, hv_back_open_size, hv_back_open_size);
                                       ClosingRectangle1(ho_RegionOpening, &ho_RegionClosing, hv_back_close_size, 10);
                                       SmallestRectangle1(ho_RegionClosing, &hv_Row11, &hv_Column11, &hv_Row21, &hv_Column21);
                                       GenRectangle1(&ho_Rectangle2, hv_Row11, hv_Column11, hv_Row21, hv_Column21);
                                       ReduceDomain(ho_Image, ho_Rectangle2, &ho_ImageReduced);
                                       QtHalcon::departImage(ho_ImageReduced, &ho_ImageGray, &ho_ImageR, &ho_ImageG, &ho_ImageB,
                                           &ho_ImageH_M, &ho_ImageS_M, &ho_ImageV_M);
                                       VarThreshold(ho_ImageB, &ho_Region1, 10, 10, 0.3, hv_thr_diff, "dark");
                                       Connection(ho_Region1, &ho_ConnectedRegions);
                                       SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", hv_select_area_min,
                                           99999);
                                       Union1(ho_SelectedRegions, &ho_RegionUnion);
                                       AreaCenter(ho_RegionUnion, &hv_Area, &hv_Row, &hv_Column);
                                       if (0 != (hv_Area<100))
                                       {
                                         GenRectangle2(&ho_RegionUnion, 150, 640, 0, 150, 150);
                                       }
                                       SmallestRectangle1(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
                                       if (0 != ((hv_Row2-hv_Row1)<100))
                                       {
                                         GenRectangle2(&ho_RegionUnion, 100, 640, 0, 150, 150);
                                         SmallestRectangle1(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
                                       }
                                       if (0 != ((hv_Column2-hv_Column1)>(hv_cut_size*1.2)))
                                       {
                                         GenRectangle1(&ho_Rectangle1, hv_Row1, hv_Column1, hv_Row2, hv_Column2);
                                         ReduceDomain(ho_ImageB, ho_Rectangle1, &ho_ImageReduced1);
                                         Threshold(ho_ImageReduced1, &ho_Region2, hv_obj_thr_min, hv_obj_thr_max);
                                         OpeningRectangle1(ho_Region2, &ho_RegionOpening1, 1, hv_obj_open_size);
                                         SmallestRectangle1(ho_RegionOpening1, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
                                         if (0 != ((hv_Column2-hv_Column1)>hv_min_width))
                                         {
                                           OpeningRectangle1(ho_RegionOpening1, &ho_RegionOpening1, hv_obj_open_size,
                                               1);
                                           SmallestRectangle1(ho_RegionOpening1, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
                                         }
                                         //gen_rectangle1 (Rectangle, Row1-50, Column1-50, Row2, Column2+50)
                                       }
                                       hv_center_row = (hv_Row1+hv_Row2)/2;
                                       hv_center_col = (hv_Column1+hv_Column2)/2;
                                       GenRectangle1(&(*ho_Rectangle), hv_center_row-(hv_cut_size*0.7), hv_center_col-(hv_cut_size*0.5),
                                           hv_center_row+(hv_cut_size*0.3), hv_center_col+(hv_cut_size*0.5));
                                       ReduceDomain(ho_Image, (*ho_Rectangle), &ho_ImageReduced2);
                                       CropDomain(ho_ImageReduced2, &(*ho_ImagePart));
                                       return;
                                     }*/
void Pro_HeparingCap::get_check_roi (HObject ho_Image, HObject ho_ROI, HObject *ho_Rectangle1, HObject *ho_ImagePart,
    HTuple hv_thr_min, HTuple hv_thr_max, HTuple hv_cut_size)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_ImageGray, ho_ImageR;
    HObject  ho_ImageG, ho_ImageB, ho_ImageH_M, ho_ImageS_M;
    HObject  ho_ImageV_M, ho_Region, ho_ImageReduced1, ho_ImageScaled;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Row2, hv_Column2;
    HTuple  hv_width_center;

    ReduceDomain(ho_Image, ho_ROI, &ho_ImageReduced);

    QtHalcon::departImage(ho_ImageReduced, &ho_ImageGray, &ho_ImageR, &ho_ImageG, &ho_ImageB,
        &ho_ImageH_M, &ho_ImageS_M, &ho_ImageV_M);
    Threshold(ho_ImageB, &ho_Region, hv_thr_min, hv_thr_max);
    SmallestRectangle1(ho_Region, &hv_Row1, &hv_Column1, &hv_Row2, &hv_Column2);
    hv_width_center = (hv_Column2+hv_Column1)/2;
    GenRectangle1(&(*ho_Rectangle1), hv_Row1-(hv_cut_size*0.25), hv_width_center-(hv_cut_size/2),
        hv_Row1+(hv_cut_size*0.75), hv_width_center+(hv_cut_size/2));
    ReduceDomain(ho_Image, (*ho_Rectangle1), &ho_ImageReduced1);
    //MinMaxGray (ho_Rectangle1, ho_Image, 0, Min, Max, Range)
    //mult := 255/Range
    //add := -mult*Min
    //scale_image (ImageReduced1, ImageScaled, mult, add)
    //ScaleImageMax(ho_ImageReduced1, &ho_ImageScaled);
    CropDomain(ho_ImageReduced1, &(*ho_ImagePart));

    return;
}
