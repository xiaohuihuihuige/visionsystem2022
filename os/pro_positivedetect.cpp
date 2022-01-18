#include "pro_positivedetect.h"
#include <QApplication>
#include"general/configfileoperate.h"
#include"general/generalfunc.h"
#include"communication/communication.h"
#include"camera/camerapar.h"
#include<QThread>
#include<QTime>
#include"parameterset.h"
#include"general/xmloperate.h"
Pro_PositiveDetect::Pro_PositiveDetect(QObject *parent) : QObject(parent)
{
    m_ExePath=qApp->applicationDirPath();
    ng_num=0;
    HalconCpp::GenEmptyObj(&m_OriginImage);
    HalconCpp::GenEmptyObj(&m_SceneImage);
    HalconCpp::GenEmptyObj(&m_SceneImageShow);
    SetWindowAttr("background_color","black");
}
Pro_PositiveDetect::~Pro_PositiveDetect()
{
    slot_program_exit();
}
//初始化参数
void Pro_PositiveDetect::initGlobalPar(QString pro,QString station,QString cam,QString step,QString type)
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
    m_ReportPath=m_ExePath+"\\Report\\"+m_ProName+"\\"+m_StationName+"\\"+m_CamName+"\\"+m_TypeName;
    readPar();

    readROI();
    readTypeInfo();
    openHalconWindow();
}
void Pro_PositiveDetect::openHalconWindow()
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
void Pro_PositiveDetect::readROI()
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
            map_model_par[roi_type[0]].sub_Pixel=m_xml.readNode(QStringList()<<roi_type[0]<<"Sub_Pixel").remove('\'').toLocal8Bit().data();
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
void Pro_PositiveDetect::readTypeInfo()
{

}
//根据启用的品类读取参数信息
void Pro_PositiveDetect::readPar()
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
void Pro_PositiveDetect::slot_press_point(QString cam,QPoint point, QSize size)
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
void Pro_PositiveDetect::slot_one_frame(QImage src,QString camname,bool ischeck,QString step)
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
void Pro_PositiveDetect::slot_check(QString camName,QString step)
{
    if(camName!=m_CamName||step!=m_StepName)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

    time_start = double(clock());//开始计时

    str_write_pos.clear();
    roi_obj.clear();
    str_write_pos_bool.clear();

    v_str_result.clear();
    v_str_bool.clear();
    v_obj_result.clear();
    v_obj_bool.clear();

    _final_result=0;
    m_send_data.clear();

    QList<int> check_result;
    try
    {


        HObject ho_ModelAtNewPosition,ho_Cross,ho_RegionAffineTrans;
        HTuple hv_Success,hv_MovementOfObject,hv_Row,hv_Column,hv_Angle,hv_Score,hv_Obj1_To_Obj2,hv_is_positive;

        if(m_IntParameter[QString::fromLocal8Bit("校正图像")]==1)
        {
            MapImage(m_SceneImage,m_MapFix,&m_SceneImage);
        }
        check_is_positive(m_SceneImage, &ho_ModelAtNewPosition, &ho_Cross,map_model_par["modelroi_F"].modelID, map_model_par["modelroi_B"].modelID,
                map_model_par["modelroi_F"].movementOfObject_Model_M,map_model_par["modelroi_B"]. movementOfObject_Model_M,
                map_model_par["modelroi_F"].numToFind,map_model_par["modelroi_F"].angle_Start, map_model_par["modelroi_F"].min_Score,
                map_model_par["modelroi_F"].max_Overlap, map_model_par["modelroi_F"].sub_Pixel, map_model_par["modelroi_F"].num_Levels,
                map_model_par["modelroi_F"].greediness, &hv_Success, &hv_MovementOfObject, &hv_Row, &hv_Column, &hv_Angle,
                &hv_Score, &hv_Obj1_To_Obj2, &hv_is_positive);
        v_obj_result.push_back(ho_ModelAtNewPosition);
        v_obj_bool.push_back(1);

        bool is_positive;
        hv_is_positive.I()==1?is_positive=true:is_positive=false;

        QMap<QString,HObject>::iterator iter=map_hobj.begin();
        while(iter!=map_hobj.end())
        {
            QString key=iter.key();
            QString direction=key.split("_").at(0);
            if((!is_positive && direction=="front")||(is_positive && direction=="back")||direction=="modelroi"||direction=="dir"||direction=="check")
            {
                iter++;
                continue;
            }
            HTuple hv_is_have,hv_Area_screw,hv_Area_hole;
            HObject ho_hole_region;
            HObject value=iter.value();

            QStringList name_type_list=key.split('#');
            if(name_type_list.size()<1)continue;
            HObject roi=value;
            try
            {
                HalconCpp::AffineTransRegion(roi, &ho_RegionAffineTrans, hv_Obj1_To_Obj2, "nearest_neighbor");
                check_circle_hole(m_SceneImage, ho_RegionAffineTrans, &ho_hole_region ,
                                  m_IntParameter[QString::fromLocal8Bit("开运算大小")],
                        m_FloatParameter[name_type_list[0]+QString::fromLocal8Bit("孔阈值小")],
                        m_FloatParameter[name_type_list[0]+QString::fromLocal8Bit("孔阈值大")],
                        m_FloatParameter[name_type_list[0]+QString::fromLocal8Bit("孔面积小")],
                        m_FloatParameter[name_type_list[0]+QString::fromLocal8Bit("孔面积大")],
                        name_type_list[1].toLocal8Bit().data(),
                        m_IntParameter[name_type_list[0]+QString::fromLocal8Bit("第一条边")],
                        &hv_is_have, &hv_Area_hole);

                HTuple area,row,col;
                AreaCenter(ho_RegionAffineTrans,&area,&row,&col);
                str_write_pos[name_type_list[0]]=QPoint(int(row.D()),int(col.D()));

            }
            catch (HalconCpp::HException &e)
            {
                check_result<<2;
                emit  signal_information_text(m_CamName,1,"halcon");
            }

            QString text;
            HTuple length;
            TupleLength(hv_Area_hole,&length);
            text=name_type_list[0]+QString::fromLocal8Bit("孔面积:")+QString::number(length.I()>0?hv_Area_hole.I():0);
            v_str_result<<text;

            if(hv_is_have.I()==1)
            {
                v_obj_result.push_back(ho_hole_region);
                v_obj_bool.push_back(1);
                roi_obj[name_type_list[0]]=ho_hole_region;
                //HalconCpp::CopyObj(ho_hole_region,&,1,1);
                v_str_bool<<1;
                check_result<<1;
            }
            else
            {
                v_obj_result.push_back(ho_RegionAffineTrans);
                v_obj_bool.push_back(2);
                v_str_bool<<2;
                check_result<<2;
            }

            iter++;
        }

        if(m_IntParameter[QString::fromLocal8Bit("测量直径")]==1)
        {
            QMap<QString,HObject>::iterator iter_hole=roi_obj.begin();
            while(iter_hole!=roi_obj.end())
            {
                QString roi_name=iter_hole.key();
                if(!m_FloatParameter.contains(roi_name+QString::fromLocal8Bit("孔直径")))
                {
                    iter_hole++;
                    continue;
                }
                HTuple row,col,r;
                HalconCpp::SmallestCircle(iter_hole.value(),&row,&col,&r);
                float r_value=r.D()*m_cam_info.map_step_cal[m_StepName].ratio.toDouble()*2*1000;
                QString text=roi_name+QString::fromLocal8Bit("孔直径:")+QString::number(r_value,'f',1);
                v_str_result<<text;

                if(r_value>=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔直径")]-m_FloatParameter[roi_name+QString::fromLocal8Bit("孔直径公差")]
                        &&r_value<=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔直径")]+m_FloatParameter[roi_name+QString::fromLocal8Bit("孔直径公差")])
                {
                    v_str_bool<<1;
                }
                else
                {
                    v_str_bool<<2;
                    check_result<<2;
                }
                iter_hole++;
            }
        }
        if(m_IntParameter[QString::fromLocal8Bit("测量长孔")]==1)
        {

            QMap<QString,HObject>::iterator iter_hole=roi_obj.begin();
            while(iter_hole!=roi_obj.end())
            {
                QString roi_name=iter_hole.key();
                if(!m_FloatParameter.contains(roi_name+QString::fromLocal8Bit("孔长")))
                {
                    iter_hole++;
                    continue;
                }
                HTuple row,col,phi,length1,length2;
                HalconCpp::SmallestRectangle2(iter_hole.value(),&row,&col,&phi,&length1,&length2);
                float length1_value=length1.D()*2*m_cam_info.map_step_cal[m_StepName].ratio.toDouble()*1000;
                QString text=roi_name+QString::fromLocal8Bit("孔长:")+QString::number(length1_value,'f',1);
                v_str_result<<text;

                if(length1_value>=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔长")]-m_FloatParameter[roi_name+QString::fromLocal8Bit("孔长公差")]
                        &&length1_value<=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔长")]+m_FloatParameter[roi_name+QString::fromLocal8Bit("孔长公差")])
                {
                    v_str_bool<<1;
                }
                else
                {
                    v_str_bool<<2;
                    check_result<<2;
                }
                float length2_value=length2.D()*2*m_cam_info.map_step_cal[m_StepName].ratio.toDouble()*1000;
                text=roi_name+QString::fromLocal8Bit("孔宽:")+QString::number(length2_value,'f',1);
                v_str_result<<text;
                if(length2_value>=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔宽")]-m_FloatParameter[roi_name+QString::fromLocal8Bit("孔宽公差")]
                        &&length2_value<=m_FloatParameter[roi_name+QString::fromLocal8Bit("孔宽")]+m_FloatParameter[roi_name+QString::fromLocal8Bit("孔宽公差")])
                {
                    v_str_bool<<1;
                }
                else
                {
                    v_str_bool<<2;
                    check_result<<2;
                }
                iter_hole++;
            }


        }



        if(m_IntParameter[QString::fromLocal8Bit("测量孔距")]==1)
        {
            QMap<QString,float>::iterator iter_float=m_FloatParameter.begin();
            while(iter_float!=m_FloatParameter.end())
            {
                QString key=iter_float.key();
                QStringList par_list=key.split('_');
                if(par_list.size()<4)
                {
                    iter_float++;
                    continue;
                }
                if(par_list[3]!=QString::fromLocal8Bit("孔距H"))
                {
                    iter_float++;
                    continue;
                }
                QString dir_str;
                is_positive?dir_str=QString("dir_F#Rectangle1"):dir_str=QString("dir_B#Rectangle1");
                if(!map_hobj.contains(dir_str))
                {
                    m_runInfo=QString::fromLocal8Bit("不存在选区")+dir_str;
                    emit  signal_information_text(m_CamName,1,m_runInfo);
                    iter_float++;
                    continue;
                }
                HTuple hv_Distance_H,hv_Distance_V,point_h,point_v;
                HObject ho_ResultContour_2,ho_RegionAffineTrans,ho_Arrow4,ho_Arrow5;
                HObject roi=map_hobj[dir_str];
                try
                {
                    QStringList name_type_list=dir_str.split('#');
                    HalconCpp::AffineTransRegion(roi, &ho_RegionAffineTrans, hv_Obj1_To_Obj2, "nearest_neighbor");
                    QString roi1_str,roi2_str;
                    roi1_str=par_list[0]+"_"+par_list[1];
                    roi2_str=par_list[0]+"_"+par_list[2];
                    if(!roi_obj.contains(roi1_str)||!roi_obj.contains(roi2_str))
                    {
                        iter_float++;
                        continue;
                    }
                    check_distance_H_V(m_SceneImage, roi_obj[roi1_str], roi_obj[roi2_str], ho_RegionAffineTrans,
                                       &ho_ResultContour_2, &ho_Arrow4, &ho_Arrow5, m_IntParameter[QString::fromLocal8Bit("方向边缘强度")],
                            &hv_Distance_H, &hv_Distance_V,&point_h,&point_v);
                }
                catch (HalconCpp::HException &e)
                {
                    check_result<<2;
                    emit  signal_information_text(m_CamName,1,"halcon");
                }
                v_obj_result.push_back(ho_ResultContour_2);
                v_obj_bool.push_back(1);

                float dis_h=hv_Distance_H.D()*m_cam_info.map_step_cal[m_StepName].ratio.toDouble()*1000;
                float dis_v=hv_Distance_V.D()*m_cam_info.map_step_cal[m_StepName].ratio.toDouble()*1000;

                str_write_pos[QString::number(dis_h,'f',1)]=QPoint(point_h[0].D(),point_h[1].D());
                str_write_pos[QString::number(dis_v,'f',1)]=QPoint(point_v[0].D(),point_v[1].D());

                QString text;

                text=key+":"+QString::number(dis_h,'f',1);
                v_str_result<<text;
                v_obj_result.push_back(ho_Arrow4);
                if(dis_h>iter_float.value()
                        -m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("公差H")]
                        &&dis_h<iter_float.value()
                        +m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("公差H")])
                {
                    v_str_bool<<1;
                    v_obj_bool.push_back(1);
                    str_write_pos_bool[QString::number(dis_h,'f',1)]=1;
                }
                else
                {
                    v_str_bool<<2;
                    v_obj_bool.push_back(2);
                    str_write_pos_bool[QString::number(dis_h,'f',1)]=2;
                    check_result<<2;
                }


                text=par_list[0]+"_"+par_list[1]+"_"+par_list[2]+QString::fromLocal8Bit("_孔距V:")+QString::number(dis_v,'f',1);
                v_str_result<<text;
                v_obj_result.push_back(ho_Arrow5);
                if(dis_v>m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("孔距V")]
                        -m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("公差V")]
                        &&dis_v<m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("孔距V")]
                        +m_FloatParameter[par_list[0]+"_"+par_list[1]+"_"+par_list[2]+"_"+QString::fromLocal8Bit("公差V")])
                {
                    v_str_bool<<1;
                    v_obj_bool.push_back(1);
                    str_write_pos_bool[QString::number(dis_v,'f',1)]=1;
                }
                else
                {
                    v_str_bool<<2;
                    v_obj_bool.push_back(2);
                    str_write_pos_bool[QString::number(dis_v,'f',1)]=2;
                    check_result<<2;
                }
                iter_float++;
            }
        }
    }
    catch (HalconCpp::HException &e)
    {
        check_result<<2;
        emit  signal_information_text(m_CamName,1,"halcon");
    }

    if(check_result.contains(2) ||check_result.size()<1)
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
void Pro_PositiveDetect::slot_read_image(QString cam,QString step,QString imgPath)
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
void Pro_PositiveDetect::writeImageAndReport()//写入图片和记录
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
void Pro_PositiveDetect::writeCheckInfoToImage()//写检测结果到图片
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
            //DispXld(v_obj_result[i],hv_WindowHandle_UI);
        }

        HTuple width,height;
        HalconCpp::GetImageSize(m_SceneImage,&width,&height);
        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/60).toLocal8Bit().data(),QString::number(width.I()/90).toLocal8Bit().data());
        for (int i=0;i<v_str_result.size();i++)
        {
            int size=height.I()/50+20;
            int x=20;
            int y=20+size*i;
            QString colorstr;
            v_str_bool[i]==1?colorstr="green":colorstr="red";
            QtHalcon::writeWindowString(hv_WindowHandle_UI,v_str_result[i].toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),y,x);
        }

        QtHalcon::setWindowFont(hv_WindowHandle_UI,"Microsoft YaHei UI",QString::number(height.I()/150).toLocal8Bit().data(),QString::number(width.I()/180).toLocal8Bit().data());

        QMap<QString,QPoint>::iterator iter=str_write_pos.begin();
        while(iter!=str_write_pos.end())
        {
            QString roi_name=iter.key();
            QString colorstr="blue";

            QtHalcon::writeWindowString(hv_WindowHandle_UI,roi_name.toLocal8Bit().data(),
                                        colorstr.toLocal8Bit().data(),iter.value().x(),iter.value().y());
            iter++;
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
        if(_final_result==1)
        {
            ng_num=0;
        }
        else
        {
            ng_num++;
        }
        emit signal_information_image(m_CamName,_final_result,image);
    }
    catch (HalconCpp::HException & ecp)
    {
    }
}

//发送结果
void Pro_PositiveDetect::send_result()
{
    emit signal_result(m_send_data,m_CamName,m_StepName);
}
//选区变化
void Pro_PositiveDetect::slot_roi_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readROI();
    }
}
//品种变化
void Pro_PositiveDetect::slot_type_change(QString type)
{
    initGlobalPar(m_ProName,m_StationName,m_CamName,m_StepName,type);
}
//参数变化
void Pro_PositiveDetect::slot_par_change(QString cam,QString step)
{
    if(cam==m_CamName&&m_StepName==step)
    {
        readPar();
    }
}
//程序结束
void Pro_PositiveDetect::slot_program_exit()
{

}
void Pro_PositiveDetect::check_circle_hole (HObject ho_Image, HObject ho_ROI, HObject *ho_hole_region,
                                            HTuple hv_opening_size, HTuple hv_hole_thr_min, HTuple hv_hole_thr_max, HTuple hv_hole_area_min,
                                            HTuple hv_hole_area_max, HTuple hv_roi_type, HTuple hv_check_best, HTuple *hv_is_have,
                                            HTuple *hv_Area_hole)
{
    // Local iconic variables
    HObject  ho_ImageReduced, ho_Region1, ho_RegionClosing;
    HObject  ho_RegionOpening, ho_ConnectedRegions1, ho_EmptyObject;
    HObject  ho_ObjectSelected, ho_Circle2, ho_RegionFillUp1;
    HObject  ho_SelectedRegions, ho_Region, ho_RegionFillUp;
    HObject  ho_ImageReduced1, ho_RegionOpening1;

    // Local control variables
    HTuple  hv_is_black, hv_Number, hv_Index, hv_Row2;
    HTuple  hv_Column2, hv_Radius2, hv_Area, hv_Row4, hv_Column4;
    HTuple  hv_Area2, hv_Row5, hv_Column5, hv_Row3, hv_Column3;
    HTuple  hv_UsedThreshold1;

    (*hv_is_have) = 0;
    (*hv_Area_hole) = 0;
    GenEmptyObj(&(*ho_hole_region));
    ReduceDomain(ho_Image, ho_ROI, &ho_ImageReduced);

    if (0 != (hv_hole_thr_min==0))
    {
        hv_is_black = 1;
    }
    else
    {
        hv_is_black = 0;
    }
    if (0 != (hv_is_black==1))
    {
        Threshold(ho_ImageReduced, &ho_Region1, hv_hole_thr_min, hv_hole_thr_max);
        ClosingCircle(ho_Region1, &ho_RegionClosing, hv_opening_size);
        OpeningCircle(ho_RegionClosing, &ho_RegionOpening, hv_opening_size);
        if (0 != (hv_roi_type==HTuple("Circle")))
        {
            Connection(ho_RegionOpening, &ho_ConnectedRegions1);
            CountObj(ho_ConnectedRegions1, &hv_Number);
            GenEmptyObj(&ho_EmptyObject);
            {
                HTuple end_val18 = hv_Number;
                HTuple step_val18 = 1;
                for (hv_Index=1; hv_Index.Continue(end_val18, step_val18); hv_Index += step_val18)
                {
                    SelectObj(ho_ConnectedRegions1, &ho_ObjectSelected, hv_Index);
                    SmallestCircle(ho_ObjectSelected, &hv_Row2, &hv_Column2, &hv_Radius2);
                    GenCircle(&ho_Circle2, hv_Row2, hv_Column2, hv_Radius2);
                    AreaCenter(ho_ObjectSelected, &hv_Area, &hv_Row4, &hv_Column4);
                    AreaCenter(ho_Circle2, &hv_Area2, &hv_Row5, &hv_Column5);
                    if (0 != (HTuple(HTuple(hv_Area2>hv_hole_area_min).TupleAnd(hv_Area2<hv_hole_area_max)).TupleAnd(((hv_Area*1.0)/hv_Area2)>0.5)))
                    {
                        ConcatObj(ho_EmptyObject, ho_Circle2, &ho_EmptyObject);
                    }
                }
            }
            Union1(ho_EmptyObject, &(*ho_hole_region));
            AreaCenter((*ho_hole_region), &(*hv_Area_hole), &hv_Row3, &hv_Column3);
        }
        else
        {
            Connection(ho_RegionOpening, &ho_ConnectedRegions1);
            FillUp(ho_ConnectedRegions1, &ho_RegionFillUp1);
            SelectShape(ho_RegionFillUp1, &ho_SelectedRegions, "area", "and", hv_hole_area_min,
                        hv_hole_area_max);
            Union1(ho_SelectedRegions, &(*ho_hole_region));
            AreaCenter((*ho_hole_region), &(*hv_Area_hole), &hv_Row3, &hv_Column3);
        }
    }
    else
    {
        VarThreshold(ho_ImageReduced, &ho_Region, 15, 15, 0.2, 5, "dark");
        //binary_threshold (ImageReduced, Region, 'max_separability', 'dark', UsedThreshold)
        //closing_circle (Region, RegionClosing1, opening_size)
        FillUp(ho_Region, &ho_RegionFillUp);
        ReduceDomain(ho_ImageReduced, ho_RegionFillUp, &ho_ImageReduced1);
        BinaryThreshold(ho_ImageReduced1, &ho_Region1, "max_separability", "light", &hv_UsedThreshold1);
        OpeningRectangle1(ho_Region1, &ho_RegionOpening1, 1.5*hv_opening_size, 1.5*hv_opening_size);
        ErosionRectangle1(ho_RegionOpening1, &ho_RegionOpening, hv_opening_size, hv_opening_size);
        if (0 != (hv_roi_type==HTuple("Circle")))
        {
            Connection(ho_RegionOpening, &ho_ConnectedRegions1);
            CountObj(ho_ConnectedRegions1, &hv_Number);
            GenEmptyObj(&ho_EmptyObject);
            {
                HTuple end_val50 = hv_Number;
                HTuple step_val50 = 1;
                for (hv_Index=1; hv_Index.Continue(end_val50, step_val50); hv_Index += step_val50)
                {
                    SelectObj(ho_ConnectedRegions1, &ho_ObjectSelected, hv_Index);
                    SmallestCircle(ho_ObjectSelected, &hv_Row2, &hv_Column2, &hv_Radius2);
                    GenCircle(&ho_Circle2, hv_Row2, hv_Column2, hv_Radius2);
                    AreaCenter(ho_ObjectSelected, &hv_Area, &hv_Row4, &hv_Column4);
                    AreaCenter(ho_Circle2, &hv_Area2, &hv_Row5, &hv_Column5);
                    if (0 != (HTuple(HTuple(hv_Area2>hv_hole_area_min).TupleAnd(hv_Area2<hv_hole_area_max)).TupleAnd(((hv_Area*1.0)/hv_Area2)>0.4)))
                    {
                        ConcatObj(ho_EmptyObject, ho_Circle2, &ho_EmptyObject);
                    }
                }
            }
            Union1(ho_EmptyObject, &(*ho_hole_region));
            AreaCenter((*ho_hole_region), &(*hv_Area_hole), &hv_Row3, &hv_Column3);
        }
        else
        {
            Connection(ho_RegionOpening, &ho_ConnectedRegions1);
            FillUp(ho_ConnectedRegions1, &ho_RegionFillUp1);
            SelectShape(ho_RegionFillUp1, &ho_SelectedRegions, "area", "and", hv_hole_area_min,
                        hv_hole_area_max);
            Union1(ho_SelectedRegions, &(*ho_hole_region));
            AreaCenter((*ho_hole_region), &(*hv_Area_hole), &hv_Row3, &hv_Column3);
        }
    }

    if (0 != ((*hv_Area_hole)>1))
    {
        (*hv_is_have) = 1;
    }
}

void Pro_PositiveDetect::check_is_positive (HObject ho_Image2, HObject *ho_ModelAtNewPosition, HObject *ho_Cross,
                                            HTuple hv_ModelID_1, HTuple hv_ModelID_2, HTuple hv_MovementOfObject_Model_M_1,
                                            HTuple hv_MovementOfObject_Model_M_2, HTuple hv_NumToFind, HTuple hv_Angle_Start,
                                            HTuple hv_Min_Score, HTuple hv_Max_Overlap, HTuple hv_Sub_Pixel, HTuple hv_Num_Levels,
                                            HTuple hv_Greediness, HTuple *hv_Success, HTuple *hv_MovementOfObject, HTuple *hv_Row,
                                            HTuple *hv_Column, HTuple *hv_Angle, HTuple *hv_Score, HTuple *hv_Obj1_To_Obj2,
                                            HTuple *hv_is_positive)
{

    // Local iconic variables
    HObject  ho_ModelAtNewPosition_1, ho_Cross_1;
    HObject  ho_ModelAtNewPosition_2, ho_Cross_2;

    // Local control variables
    HTuple  hv_Success_1, hv_MovementOfObject_1, hv_Row_1;
    HTuple  hv_Column_1, hv_Angle_1, hv_Score_1, hv_Success_2;
    HTuple  hv_MovementOfObject_2, hv_Row_2, hv_Column_2, hv_Angle_2;
    HTuple  hv_Score_2;

    (*hv_is_positive) = 0;
    QtHalcon::checkModel(ho_Image2, &ho_ModelAtNewPosition_1, &ho_Cross_1, hv_ModelID_1, hv_NumToFind,
                         hv_Angle_Start, hv_Min_Score, hv_Max_Overlap, hv_Sub_Pixel, hv_Num_Levels,
                         hv_Greediness, &hv_Success_1, &hv_MovementOfObject_1, &hv_Row_1, &hv_Column_1,
                         &hv_Angle_1, &hv_Score_1);
    QtHalcon::checkModel(ho_Image2, &ho_ModelAtNewPosition_2, &ho_Cross_2, hv_ModelID_2, hv_NumToFind,
                         hv_Angle_Start, hv_Min_Score, hv_Max_Overlap, hv_Sub_Pixel, hv_Num_Levels,
                         hv_Greediness, &hv_Success_2, &hv_MovementOfObject_2, &hv_Row_2, &hv_Column_2,
                         &hv_Angle_2, &hv_Score_2);
    if (0 != (hv_Score_1>hv_Score_2))
    {
        (*hv_Success) = hv_Success_1;
        (*hv_MovementOfObject) = hv_MovementOfObject_1;
        (*hv_Row) = hv_Row_1;
        (*hv_Column) = hv_Column_1;
        (*hv_Angle) = hv_Angle_1;
        (*hv_Score) = hv_Score_1;
        (*ho_ModelAtNewPosition)=ho_ModelAtNewPosition_1;
        //CopyObj(ho_ModelAtNewPosition_1, &(*ho_ModelAtNewPosition), 1, 1);
        CopyObj(ho_Cross_1, &(*ho_Cross), 1, 1);
        QtHalcon::matrixFromTrans1ToTrans2(hv_MovementOfObject_Model_M_1, (*hv_MovementOfObject),
                                           &(*hv_Obj1_To_Obj2));
        (*hv_is_positive) = 1;
    }
    else
    {
        (*hv_Success) = hv_Success_2;
        (*hv_MovementOfObject) = hv_MovementOfObject_2;
        (*hv_Row) = hv_Row_2;
        (*hv_Column) = hv_Column_2;
        (*hv_Angle) = hv_Angle_2;
        (*hv_Score) = hv_Score_2;
        (*ho_ModelAtNewPosition)=ho_ModelAtNewPosition_2;
        //CopyObj(ho_ModelAtNewPosition_2, &(*ho_ModelAtNewPosition), 1, 1);
        CopyObj(ho_Cross_2, &(*ho_Cross), 1, 1);
        QtHalcon::matrixFromTrans1ToTrans2(hv_MovementOfObject_Model_M_2, (*hv_MovementOfObject),
                                           &(*hv_Obj1_To_Obj2));
        (*hv_is_positive) = 2;
    }



    return;
}

void Pro_PositiveDetect::check_distance_H_V (HObject ho_Image, HObject ho_region_0, HObject ho_region_1,
                                             HObject ho_region_dir, HObject *ho_line_dir, HObject *ho_line_H, HObject *ho_line_V,
                                             HTuple hv_edge_strength, HTuple *hv_Distance_H, HTuple *hv_Distance_V, HTuple *hv_point_h,
                                             HTuple *hv_point_v)
{

    // Local iconic variables
    HObject  ho_ImageReduced, ho_Edge_Cross, ho_Arrow;
    HObject  ho_line_H1, ho_line_H2, ho_line_V1, ho_line_V2;

    // Local control variables
    HTuple  hv_Row1, hv_Column1, hv_Radius_1, hv_Row3;
    HTuple  hv_Column3, hv_Radius_2, hv_Row, hv_Column, hv_Phi;
    HTuple  hv_Length1, hv_Length2, hv_Black_To_White, hv_First_Edge;
    HTuple  hv_Line2, hv_b, hv_Column_4, hv_Row_4, hv_RowProj1;
    HTuple  hv_ColProj1;


    (*hv_Distance_H) = 0;
    (*hv_Distance_V) = 0;
    GenEmptyObj(&(*ho_line_dir));
    GenEmptyObj(&(*ho_line_H));
    GenEmptyObj(&(*ho_line_V));

    SmallestCircle(ho_region_0, &hv_Row1, &hv_Column1, &hv_Radius_1);
    SmallestCircle(ho_region_1, &hv_Row3, &hv_Column3, &hv_Radius_2);
    ReduceDomain(ho_Image, ho_region_dir, &ho_ImageReduced);


    SmallestRectangle2(ho_region_dir, &hv_Row, &hv_Column, &hv_Phi, &hv_Length1, &hv_Length2);
    //边缘强度（边缘强度越高保留边缘点越清晰）
    //由黑到白（由0到255为增加，故为'positive'）
    hv_Black_To_White = -1;
    //第一条边（查找方为从直线左侧到直线右侧，直线起点到终点为前）
    hv_First_Edge = -1;

    QtHalcon::findLineInRectangle2(ho_ImageReduced, &(*ho_line_dir), &ho_Edge_Cross, hv_edge_strength,
                                   hv_Black_To_White, hv_First_Edge, hv_Row, hv_Column, hv_Phi, hv_Length1, hv_Length2,
                                   &hv_Line2);

    //smallest_rectangle1 (region_dir, Row13, Column13, Row23, Column23)
    //findLineInRectangle1 (ImageReduced, line_dir, Edge_Cross, edge_strength, Black_To_White, First_Edge, [Row13, Column13, Row23, Column23], 0, Line2)
    QtHalcon::gen_arrow_contour_xld(&ho_Arrow, HTuple(hv_Line2[0]), HTuple(hv_Line2[1]), HTuple(hv_Line2[2]),
            HTuple(hv_Line2[3]), 10, 10);
    hv_b = hv_Row1-(((HTuple(hv_Line2[2])-HTuple(hv_Line2[0]))/(HTuple(hv_Line2[3])-HTuple(hv_Line2[1])))*hv_Column1);
    hv_Column_4 = hv_Column1+30;
    hv_Row_4 = ((hv_Column_4*(HTuple(hv_Line2[2])-HTuple(hv_Line2[0])))/(HTuple(hv_Line2[3])-HTuple(hv_Line2[1])))+hv_b;
    QtHalcon::gen_arrow_contour_xld(&ho_Arrow, hv_Row_4, hv_Column_4, hv_Row1, hv_Column1, 10,
                                    10);
    ProjectionPl(hv_Row3, hv_Column3, hv_Row_4, hv_Column_4, hv_Row1, hv_Column1, &hv_RowProj1,
                 &hv_ColProj1);
    QtHalcon::gen_arrow_contour_xld(&ho_line_H1, hv_Row3, hv_Column3, hv_RowProj1, hv_ColProj1,
                                    10, 10);
    QtHalcon::gen_arrow_contour_xld(&ho_line_H2, hv_RowProj1, hv_ColProj1, hv_Row3, hv_Column3,
                                    10, 10);
    ConcatObj(ho_line_H1, ho_line_H2, &(*ho_line_H));
    DistancePp(hv_Row3, hv_Column3, hv_RowProj1, hv_ColProj1, &(*hv_Distance_H));
    QtHalcon::gen_arrow_contour_xld(&ho_line_V1, hv_Row1, hv_Column1, hv_RowProj1, hv_ColProj1,
                                    10, 10);
    QtHalcon::gen_arrow_contour_xld(&ho_line_V2, hv_RowProj1, hv_ColProj1, hv_Row1, hv_Column1,
                                    10, 10);
    ConcatObj(ho_line_V1, ho_line_V2, &(*ho_line_V));
    DistancePp(hv_Row1, hv_Column1, hv_RowProj1, hv_ColProj1, &(*hv_Distance_V));
    (*hv_point_h) = HTuple();
    (*hv_point_h)[0] = (hv_Row3+hv_RowProj1)/2;
    (*hv_point_h)[1] = (hv_Column3+hv_ColProj1)/2;
    (*hv_point_v) = HTuple();
    (*hv_point_v)[0] = (hv_Row1+hv_RowProj1)/2;
    (*hv_point_v)[1] = (hv_Column1+hv_ColProj1)/2;

    return;
}

