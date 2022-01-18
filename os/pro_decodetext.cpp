#include "pro_decodetext.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"

Pro_DecodeText::Pro_DecodeText(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}

Pro_DecodeText::~Pro_DecodeText()
{
    slot_program_exit();
}
//初始化参数
void Pro_DecodeText::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
void Pro_DecodeText::openHalconWindow()
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
void Pro_DecodeText::readROI()
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
void Pro_DecodeText::readTypeInfo()
{
    //slot_program_exit();

}
//根据启用的品类读取参数信息
void Pro_DecodeText::readPar()
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
        else
        {
            m_IntParameter[QString::fromLocal8Bit("校正图像")]=0;
        }
    }
    m_xml.closeXml();

    //删除超出保存日期的记录
    GeneralFunc::deleteFolderOutOfRange(m_ReportPath,m_IntParameter[QString::fromLocal8Bit("保存天数")]);
}

//计算点击处的像素信息
void Pro_DecodeText::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_DecodeText::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_DecodeText::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时
    v_obj_result.clear();
    v_obj_bool.clear();

    v_str_result.clear();
    v_str_bool.clear();

    v_result_image.clear();
    v_result_image_rect.clear();

    map_string_point.clear();
    map_string_color.clear();

    m_send_data.clear();
    try
    {
        HObject imagereduce;
        ReduceDomain(m_SceneImage,map_hobj["findroi#Rectangle1"],&imagereduce);
        HObject ho_ModelAtNewPosition,ho_Cross;
        HTuple hv_Success,hv_MovementOfObject,hv_Row,hv_Column,hv_Angle,hv_Score,hv_Obj1_To_Obj2;
        QtHalcon::checkModel(imagereduce, &ho_ModelAtNewPosition, &ho_Cross, map_model_par["modelroi_S"].modelID, map_model_par["modelroi_S"].numToFind,
                map_model_par["modelroi_S"].angle_Start, map_model_par["modelroi_S"].min_Score,
                map_model_par["modelroi_S"].max_Overlap, map_model_par["modelroi_S"].sub_Pixel,
                map_model_par["modelroi_S"].num_Levels,map_model_par["modelroi_S"].greediness,
                &hv_Success, &hv_MovementOfObject,&hv_Row, &hv_Column, &hv_Angle,&hv_Score);

        v_obj_result.push_back(ho_ModelAtNewPosition);
        v_obj_bool.push_back(1);
        //*计算变化矩阵
        QtHalcon::matrixFromTrans1ToTrans2(map_model_par["modelroi_S"].movementOfObject_Model_M,hv_MovementOfObject, &hv_Obj1_To_Obj2);

        //条码识别
        HObject coderoi,image_part;
        AffineTransRegion(map_hobj["code#Rectangle2"],&coderoi,hv_Obj1_To_Obj2,"nearest_neighbor");
        HalconCpp::ReduceDomain(m_SceneImage,coderoi,&imagereduce);
        HalconCpp::CropDomain(imagereduce,&image_part);
        QImage image;
        QtHalcon::hImage2QImage(image_part,image);
        QString codestr=GeneralFunc::decodeQImage(image,0);
        QString text;
        text=QString::fromLocal8Bit("条码:")+codestr;
        v_str_result.push_back(text);
        v_str_bool.push_back(1);
        v_obj_result.push_back(coderoi);
        v_obj_bool.push_back(1);

        //识别型号
        HObject roi_trans;
        HTuple  hv_string_out;
        HTuple num_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("typeroi位置数字")],',',"int");
        HTuple upper_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("typeroi位置大写字母")],',',"int");
        HTuple lower_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("typeroi位置小写字母")],',',"int");
        AffineTransRegion(map_hobj["typeroi#Rectangle2"],&roi_trans,hv_Obj1_To_Obj2,"nearest_neighbor");
        recognize_string (m_SceneImage ,  roi_trans,m_FloatParameter[QString::fromLocal8Bit("字符阈值小")] ,
                m_FloatParameter[QString::fromLocal8Bit("字符阈值大")], m_FloatParameter[QString::fromLocal8Bit("旋转角度")],
                m_FloatParameter[QString::fromLocal8Bit("字符宽度")], m_FloatParameter[QString::fromLocal8Bit("字符宽度容差比例")],
                num_pos,upper_pos, lower_pos, &hv_string_out);
        text=QString::fromLocal8Bit("型号:")+QString::fromLocal8Bit(hv_string_out.S());
        v_str_result.push_back(text);
        v_str_bool.push_back(1);
        v_obj_result.push_back(roi_trans);
        v_obj_bool.push_back(1);
        //识别序列号
         num_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("serialroi位置数字")],',',"int");
         upper_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("serialroi位置大写字母")],',',"int");
         lower_pos=QtHalcon::qstringToHtuple(m_StringParameter[QString::fromLocal8Bit("serialroi位置小写字母")],',',"int");
        AffineTransRegion(map_hobj["serialroi#Rectangle2"],&roi_trans,hv_Obj1_To_Obj2,"nearest_neighbor");
        recognize_string (m_SceneImage ,  roi_trans,m_FloatParameter[QString::fromLocal8Bit("字符阈值小")] ,
                m_FloatParameter[QString::fromLocal8Bit("字符阈值大")], m_FloatParameter[QString::fromLocal8Bit("旋转角度")],
                m_FloatParameter[QString::fromLocal8Bit("字符宽度")], m_FloatParameter[QString::fromLocal8Bit("字符宽度容差比例")],
                num_pos,upper_pos, lower_pos, &hv_string_out);
        text=QString::fromLocal8Bit("序列号:")+QString::fromLocal8Bit(hv_string_out.S());
        v_str_result.push_back(text);
        v_str_bool.push_back(1);
        v_obj_result.push_back(roi_trans);
        v_obj_bool.push_back(1);

    }
    catch (cv::Exception &e)
    {
        emit  signal_information_text(m_CamName,1,QString::fromStdString(e.msg));
    }
    catch (HalconCpp::HException &e)
    {
        emit  signal_information_text(m_CamName,1,QString::fromStdString(e.ErrorMessage().Text()));
    }

    for (int i=0;i<v_obj_bool.size();i++)
    {
        m_send_data<<v_obj_bool[i];
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
/*bool compare(int a)
{
    return a>1;
}
    //QList<int> relist=QtConcurrent::blockingFiltered(v_obj_bool,compare);
*/

//读取文件
void Pro_DecodeText::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_DecodeText::writeImageAndReport()//写入图片和记录
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
void Pro_DecodeText::writeCheckInfoToImage()//写检测结果到图片
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
        SetLineWidth(hv_WindowHandle_UI,m_IntParameter[QString::fromLocal8Bit("绘制边缘宽度")]);//2
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
            int size=height.I()/30+20;
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
        emit signal_information_image(m_CamName,1,image);
    }
    catch (HalconCpp::HException & ecp)
    {
        emit  signal_information_text(m_CamName,1,QString::fromStdString(ecp.ErrorMessage().Text()));

    }
}

//发送结果
void Pro_DecodeText::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_DecodeText::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_DecodeText::slot_type_change(QString type)
{
    m_TypeName=type;
    m_TypePath=m_StepPath+"\\"+m_TypeName;
    if(!GeneralFunc::isDirExist(m_TypePath,false))return;
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_StepName+"\\"+m_TypeName;
    readROI();
    readPar();
}
//参数变化
void Pro_DecodeText::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_DecodeText::slot_program_exit()
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

void Pro_DecodeText::straightenOcr (HObject ho_Image, HObject ho_ROI_0, HObject *ho_ImagePart)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region, ho_ImageAffineTrans;
    HObject  ho_RegionAffineTrans, ho_ImageReduced1, ho_Region1;
    HObject  ho_Rectangle, ho_ImageReduced2;

    // Local control variables
    HTuple  hv_Row, hv_Column, hv_Phi, hv_Length1;
    HTuple  hv_Length2, hv_HomMat2D, hv_Row1, hv_Column1, hv_Row2;
    HTuple  hv_Column2, hv_add, hv_height_ratio, hv_width_ratio;
    HTuple  hv_height, hv_width;

    ReduceDomain(ho_Image, ho_ROI_0, &ho_ImageReduced);
    Threshold(ho_ImageReduced, &ho_Region, 0, 50);

    SmallestRectangle2(ho_Region, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1, &hv_Length2);
    VectorAngleToRigid(hv_Row, hv_Column, hv_Phi, hv_Row, hv_Column, 0, &hv_HomMat2D);
    AffineTransImage(ho_Image, &ho_ImageAffineTrans, hv_HomMat2D, "constant", "false");

    AffineTransRegion(ho_ROI_0, &ho_RegionAffineTrans, hv_HomMat2D, "nearest_neighbor");
    ReduceDomain(ho_ImageAffineTrans, ho_RegionAffineTrans, &ho_ImageReduced1);
    CropDomain(ho_ImageReduced1, &(*ho_ImagePart));
    return;
}
void Pro_DecodeText::recognize_string (HObject ho_ImageIn, HObject ho_ROIIn, HTuple hv_MinThreshold,
                                       HTuple hv_MaxThreshold, HTuple hv_rotate_angle, HTuple hv_gap_in_word, HTuple hv_gap_allow_ratio,
                                       HTuple hv_number_position, HTuple hv_upper_position, HTuple hv_lower_position,
                                       HTuple *hv_string_out)
                                   {

                                     // Local iconic variables
                                     HObject  ho_ImageAffinTrans, ho_RegionAffineTrans;
                                     HObject  ho_ImageReduced, ho_ImageScaled, ho_RegionErosion;
                                     HObject  ho_ImageReduced1, ho_Region, ho_ConnectedRegions;
                                     HObject  ho_SelectedRegions, ho_RegionUnion, ho_Rectangle;
                                     HObject  ho_RegionAffineTrans1, ho_ImageAffinTrans1, ho_RegionOpening;
                                     HObject  ho_Split, ho_SortedDate, ho_numObject, ho_ObjectSelected;
                                     HObject  ho_upperObject, ho_lowerObject;

                                     // Local control variables
                                     HTuple  hv_number_OCRHandle, hv_upper_letter_OCRHandle;
                                     HTuple  hv_lower_letter_OCRHandle, hv_Row, hv_Column, hv_Phi;
                                     HTuple  hv_Length1, hv_Length2, hv_HomMat2D, hv_Min, hv_Max;
                                     HTuple  hv_Range, hv_mult, hv_add, hv_Row1, hv_Column1;
                                     HTuple  hv_Phi1, hv_Length11, hv_Length21, hv_HomMat2D1;
                                     HTuple  hv_Index, hv_Class, hv_Confidence, hv_num_StringOut;
                                     HTuple  hv_DateScore, hv_upper_letter_StringOut, hv_lower_letter_StringOut;
                                     HTuple  hv_string_length, hv_str_all_tuple, hv_Selected;

                                     if (0 != ((hv_number_position.TupleLength())>0))
                                     {
                                       ReadOcrClassMlp("Industrial_0-9+_NoRej.omc", &hv_number_OCRHandle);
                                     }
                                     if (0 != ((hv_upper_position.TupleLength())>0))
                                     {
                                       ReadOcrClassMlp("Industrial_A-Z+_NoRej.omc", &hv_upper_letter_OCRHandle);
                                     }
                                     if (0 != ((hv_lower_position.TupleLength())>0))
                                     {
                                       ReadOcrClassMlp("D:/Program Files/MVTec/HALCON-12.0/ocr/OCRB.omc", &hv_lower_letter_OCRHandle);
                                     }

                                     SmallestRectangle2(ho_ROIIn, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1, &hv_Length2);
                                     VectorAngleToRigid(hv_Row, hv_Column, 0, hv_Row, hv_Column, hv_rotate_angle.TupleRad(),
                                         &hv_HomMat2D);
                                     AffineTransImage(ho_ImageIn, &ho_ImageAffinTrans, hv_HomMat2D, "constant", "false");
                                     AffineTransRegion(ho_ROIIn, &ho_RegionAffineTrans, hv_HomMat2D, "nearest_neighbor");
                                     ReduceDomain(ho_ImageAffinTrans, ho_RegionAffineTrans, &ho_ImageReduced);

                                     MinMaxGray(ho_ImageReduced, ho_ImageReduced, 0, &hv_Min, &hv_Max, &hv_Range);
                                     hv_mult = 255/hv_Range;
                                     hv_add = (-hv_mult)*hv_Min;
                                     ScaleImage(ho_ImageReduced, &ho_ImageScaled, hv_mult, hv_add);
                                     ErosionCircle(ho_RegionAffineTrans, &ho_RegionErosion, 3.5);
                                     ReduceDomain(ho_ImageScaled, ho_RegionErosion, &ho_ImageReduced1);
                                     Threshold(ho_ImageReduced1, &ho_Region, hv_MinThreshold, hv_MaxThreshold);
                                     Connection(ho_Region, &ho_ConnectedRegions);
                                     SelectShape(ho_ConnectedRegions, &ho_SelectedRegions, "area", "and", 10, 9999999);
                                     //select_shape (ConnectedRegions, SelectedRegions, ['area','height'], 'and', [3,5], [9999999,20])
                                     Union1(ho_SelectedRegions, &ho_RegionUnion);
                                     SmallestRectangle2(ho_RegionUnion, &hv_Row1, &hv_Column1, &hv_Phi1, &hv_Length11,
                                         &hv_Length21);
                                     GenRectangle2(&ho_Rectangle, hv_Row1, hv_Column1, hv_Phi1, hv_Length11, hv_Length21);
                                     VectorAngleToRigid(hv_Row1, hv_Column1, hv_Phi1, hv_Row1, hv_Column1, 0, &hv_HomMat2D1);
                                     AffineTransRegion(ho_RegionUnion, &ho_RegionAffineTrans1, hv_HomMat2D1, "nearest_neighbor");
                                     AffineTransImage(ho_ImageAffinTrans, &ho_ImageAffinTrans1, hv_HomMat2D1, "constant",
                                         "false");
                                     OpeningCircle(ho_RegionAffineTrans1, &ho_RegionOpening, 1);
                                     PartitionDynamic(ho_RegionOpening, &ho_Split, hv_gap_in_word, hv_gap_allow_ratio);
                                     SortRegion(ho_Split, &ho_SortedDate, "first_point", "true", "column");


                                     if (0 != ((hv_number_position.TupleLength())>0))
                                     {
                                       GenEmptyObj(&ho_numObject);
                                       {
                                       HTuple end_val39 = (hv_number_position.TupleLength())-1;
                                       HTuple step_val39 = 1;
                                       for (hv_Index=0; hv_Index.Continue(end_val39, step_val39); hv_Index += step_val39)
                                       {
                                         SelectObj(ho_SortedDate, &ho_ObjectSelected, HTuple(hv_number_position[hv_Index])+1);
                                         ConcatObj(ho_numObject, ho_ObjectSelected, &ho_numObject);
                                       }
                                       }
                                       DoOcrWordMlp(ho_numObject, ho_ImageAffinTrans1, hv_number_OCRHandle, ".*", 5,
                                           5, &hv_Class, &hv_Confidence, &hv_num_StringOut, &hv_DateScore);
                                     }
                                     if (0 != ((hv_upper_position.TupleLength())>0))
                                     {
                                       GenEmptyObj(&ho_upperObject);
                                       {
                                       HTuple end_val47 = (hv_upper_position.TupleLength())-1;
                                       HTuple step_val47 = 1;
                                       for (hv_Index=0; hv_Index.Continue(end_val47, step_val47); hv_Index += step_val47)
                                       {
                                         SelectObj(ho_SortedDate, &ho_ObjectSelected, HTuple(hv_upper_position[hv_Index])+1);
                                         ConcatObj(ho_upperObject, ho_ObjectSelected, &ho_upperObject);
                                       }
                                       }
                                       DoOcrWordMlp(ho_upperObject, ho_ImageAffinTrans1, hv_upper_letter_OCRHandle,
                                           ".*", 5, 5, &hv_Class, &hv_Confidence, &hv_upper_letter_StringOut, &hv_DateScore);
                                     }
                                     if (0 != ((hv_lower_position.TupleLength())>0))
                                     {
                                       GenEmptyObj(&ho_lowerObject);
                                       {
                                       HTuple end_val55 = (hv_lower_position.TupleLength())-1;
                                       HTuple step_val55 = 1;
                                       for (hv_Index=0; hv_Index.Continue(end_val55, step_val55); hv_Index += step_val55)
                                       {
                                         SelectObj(ho_SortedDate, &ho_ObjectSelected, HTuple(hv_lower_position[hv_Index])+1);
                                         ConcatObj(ho_lowerObject, ho_ObjectSelected, &ho_lowerObject);
                                       }
                                       }
                                       DoOcrWordMlp(ho_SortedDate, ho_ImageAffinTrans1, hv_lower_letter_OCRHandle, ".*",
                                           5, 5, &hv_Class, &hv_Confidence, &hv_lower_letter_StringOut, &hv_DateScore);
                                     }
                                     hv_string_length = ((hv_number_position.TupleLength())+(hv_upper_position.TupleLength()))+(hv_lower_position.TupleLength());
                                     TupleGenConst(hv_string_length, "1", &hv_str_all_tuple);
                                     {
                                     HTuple end_val63 = (hv_number_position.TupleLength())-1;
                                     HTuple step_val63 = 1;
                                     for (hv_Index=0; hv_Index.Continue(end_val63, step_val63); hv_Index += step_val63)
                                     {
                                       TupleStrBitSelect(hv_num_StringOut, hv_Index, &hv_Selected);
                                       hv_str_all_tuple[HTuple(hv_number_position[hv_Index])] = hv_Selected;
                                     }
                                     }
                                     {
                                     HTuple end_val67 = (hv_upper_position.TupleLength())-1;
                                     HTuple step_val67 = 1;
                                     for (hv_Index=0; hv_Index.Continue(end_val67, step_val67); hv_Index += step_val67)
                                     {
                                       TupleStrBitSelect(hv_upper_letter_StringOut, hv_Index, &hv_Selected);
                                       hv_str_all_tuple[HTuple(hv_upper_position[hv_Index])] = hv_Selected;
                                     }
                                     }
                                     {
                                     HTuple end_val71 = (hv_lower_position.TupleLength())-1;
                                     HTuple step_val71 = 1;
                                     for (hv_Index=0; hv_Index.Continue(end_val71, step_val71); hv_Index += step_val71)
                                     {
                                       TupleStrBitSelect(hv_lower_letter_StringOut, hv_Index, &hv_Selected);
                                       hv_str_all_tuple[HTuple(hv_lower_position[hv_Index])] = hv_Selected;
                                     }
                                     }
                                     (*hv_string_out) = "";
                                     {
                                     HTuple end_val76 = (hv_str_all_tuple.TupleLength())-1;
                                     HTuple step_val76 = 1;
                                     for (hv_Index=0; hv_Index.Continue(end_val76, step_val76); hv_Index += step_val76)
                                     {
                                       (*hv_string_out) += HTuple(hv_str_all_tuple[hv_Index]);
                                     }
                                     }

                                     return;
                                   }
