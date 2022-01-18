#include "cameracalibrate.h"
#include "ui_cameracalibrate.h"
#include"general/generalfunc.h"
#include<QDateTime>
#include<QMessageBox>
#include"general/xmloperate.h"

CameraCalibrate::CameraCalibrate(QWidget *parent) :
    QWidget(parent),
    zoom_scale(1),
    g_cal_par_config(nullptr),
    ui(new Ui::CameraCalibrate)
{
    ui->setupUi(this);
    GenEmptyObj(&m_OriginImage);
    GenEmptyObj(&m_SceneImage);

}

CameraCalibrate::~CameraCalibrate()
{
    if(g_cal_par_config!=nullptr)
    {
        delete g_cal_par_config;
        g_cal_par_config=nullptr;
    }
    delete ui;
}
void CameraCalibrate::resizeEvent(QResizeEvent *event)
{
    if(hv_WindowHandle.Length()>0)
    {
        HalconCpp::CloseWindow(hv_WindowHandle);
    }
    //初始化窗口
    Hlong windID = Hlong(this->ui->lbl_Image->winId());
    HalconCpp::OpenWindow(0,0,ui->lbl_Image->width(),ui->lbl_Image->height(),windID,"visible","",&hv_WindowHandle);
    SetColor(hv_WindowHandle,"green");
    SetDraw(hv_WindowHandle,"margin");
    SetLineWidth(hv_WindowHandle,2);
}
void CameraCalibrate::mousePressEvent(QMouseEvent *event)
{

    QPoint s(ui->lbl_Image->pos());//控件在窗口内的坐标
    lastPoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
    if(ui->lbl_Image->rect().contains(lastPoint))
    {
        if(QtHalcon::testHObjectEmpty(m_SceneImage))return;
        mouseDown = true;
        firstPoint = lastPoint;
        HTuple pos,value ,length;
        QtHalcon::getMousePosGray( hv_WindowHandle,m_SceneImage,&pos,&value);
        TupleLength(value,&length);
        QString posinfo;
        if(length.I()==1)
        {
            posinfo=QString("(row,col):(%1,%2);RGB:(%3,%4,%5)").arg(QString::number(pos[0].I()),QString::number(pos[1].I()),
                    QString::number(value[0].I()),QString::number(value[0].I()),QString::number(value[0].I()));
        }
        else
        {
            posinfo=QString("(row,col):(%1,%2);RGB:(%3,%4,%5)").arg(QString::number(pos[0].I()),QString::number(pos[1].I()),
                    QString::number(value[0].I()),QString::number(value[1].I()),QString::number(value[2].I()));
        }
        ui->lbl_PosInfo->setText(posinfo);
    }
}

void CameraCalibrate::mouseMoveEvent(QMouseEvent *event)
{
    QPoint s(ui->lbl_Image->pos());//控件在窗口内的坐标
    mousePoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
}

void CameraCalibrate::mouseReleaseEvent(QMouseEvent *event)
{
    QPoint s(ui->lbl_Image->pos());//控件在窗口内的坐标
    lastPoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
    if(ui->lbl_Image->rect().contains(lastPoint))
    {
        if(QtHalcon::testHObjectEmpty(m_SceneImage))return;

        if(lastPoint.x()!=0&&lastPoint.y()!=0&&zoom_scale>1)
        {
            moveWnd(firstPoint,lastPoint, m_SceneImage, hv_WindowHandle);
            mouseDown = false;
        }
    }

}

void CameraCalibrate::wheelEvent(QWheelEvent *event)
{
    if(QtHalcon::testHObjectEmpty(m_SceneImage))return;
    if(ui->lbl_Image->rect().contains(mousePoint))
    {
        short zDelta =short(event->delta());
        if (zDelta>0)           //图像放大
        {
            if (zoom_scale<6)      //最大放大6倍
            {
                zoom_scale = zoom_scale*1.05;
                displayImage(m_SceneImage, hv_WindowHandle);
            }
        }
        else if (zDelta<0)                   //图像缩小
        {
            if (zoom_scale>1)
            {
                zoom_scale = zoom_scale / 1.05;
                if (zoom_scale < 1)
                {
                    zoom_scale = 1;
                }
                displayImage(m_SceneImage, hv_WindowHandle);
            }
        }
    }

}

void CameraCalibrate::displayImage(HImage srcImg,HTuple hv_Window)
{
    double dWidth = srcImg.Width().I();
    double dHeight = srcImg.Height().I();
    double dWidth2 = dWidth / zoom_scale;
    double dHeight2 = dHeight / zoom_scale;
    if (dHeight / 2 - dHeight2 / 2 >= m_dYOffset && dHeight / 2 + dHeight2 / 2 - m_dYOffset <= dHeight)
    {
        m_dDispImagePartRow0 = dHeight / 2 - dHeight2 / 2 - m_dYOffset;
        m_dDispImagePartRow1 = dHeight / 2 + dHeight2 / 2 - m_dYOffset;
    }
    else if (dHeight / 2 - dHeight2 / 2 <= m_dYOffset)
    {
        m_dYOffset = dHeight / 2 - dHeight2 / 2;
        m_dDispImagePartRow0 = dHeight / 2 - dHeight2 / 2 - m_dYOffset;
        m_dDispImagePartRow1 = dHeight / 2 + dHeight2 / 2 - m_dYOffset;
    }
    else if (dHeight / 2 + dHeight2 / 2 - m_dYOffset >= dHeight)
    {
        m_dYOffset = dHeight / 2 + dHeight2 / 2 - dHeight;
        m_dDispImagePartRow0 = dHeight / 2 - dHeight2 / 2 - m_dYOffset;
        m_dDispImagePartRow1 = dHeight / 2 + dHeight2 / 2 - m_dYOffset;
    }
    if (dWidth / 2 - dWidth2 / 2 >= m_dXOffset && dWidth / 2 + dWidth2 / 2 - m_dXOffset <= dWidth)
    {
        m_dDispImagePartCol0 = dWidth / 2 - dWidth2 / 2 - m_dXOffset;
        m_dDispImagePartCol1 = dWidth / 2 + dWidth2 / 2 - m_dXOffset;
    }
    else if (dWidth / 2 - dWidth2 / 2 <= m_dXOffset)
    {
        m_dXOffset = dWidth / 2 - dWidth2 / 2;
        m_dDispImagePartCol0 = dWidth / 2 - dWidth2 / 2 - m_dXOffset;
        m_dDispImagePartCol1 = dWidth / 2 + dWidth2 / 2 - m_dXOffset;
    }
    else if (dWidth / 2 + dWidth2 / 2 - m_dXOffset >= dWidth)
    {
        m_dXOffset = dWidth / 2 + dWidth2 / 2 - dWidth;
        m_dDispImagePartCol0 = dWidth / 2 - dWidth2 / 2 - m_dXOffset;
        m_dDispImagePartCol1 = dWidth / 2 + dWidth2 / 2 - m_dXOffset;
    }
    SetPart(hv_Window, m_dDispImagePartRow0, m_dDispImagePartCol0, m_dDispImagePartRow1-1, m_dDispImagePartCol1-1);
    DispObj(srcImg, hv_Window);
}

void CameraCalibrate::moveWnd(QPoint pointStart,QPoint pointEnd, HImage srcImg, HTuple hWindow)
{
    QRect m_rPic=ui->lbl_Image->rect();
    double wWidth = m_rPic.right() - m_rPic.left();
    double wHeight = m_rPic.bottom() - m_rPic.top();
    double dWidth = srcImg.Width().I();
    double dHeight = srcImg.Height().I();
    int xOffset = pointEnd.x() - pointStart.x();
    int yOffset = pointEnd.y() - pointStart.y();
    m_dXOffset = m_dXOffset + (pointEnd.x() - pointStart.x())*dWidth / wWidth / zoom_scale;
    m_dYOffset = m_dYOffset + (pointEnd.y() - pointStart.y())*dHeight / wHeight / zoom_scale;
    if (m_dDispImagePartRow0 >= yOffset *dHeight / wHeight / zoom_scale && m_dDispImagePartRow1 - yOffset *dHeight / wHeight / zoom_scale <= dHeight)
    {
        m_dDispImagePartRow0 = m_dDispImagePartRow0 - yOffset *dHeight / wHeight / zoom_scale;
        m_dDispImagePartRow1 = m_dDispImagePartRow1 - yOffset *dHeight / wHeight / zoom_scale;
    }
    else if (m_dDispImagePartRow0 <= yOffset *dHeight / wHeight / zoom_scale)
    {
        m_dDispImagePartRow1 = m_dDispImagePartRow1 - m_dDispImagePartRow0;
        m_dDispImagePartRow0 = m_dDispImagePartRow0 - m_dDispImagePartRow0;
    }
    else if (m_dDispImagePartRow1 - yOffset *dHeight / wHeight / zoom_scale >= dHeight)
    {
        m_dDispImagePartRow0 = m_dDispImagePartRow0 - m_dDispImagePartRow1 + dHeight;
        m_dDispImagePartRow1 = m_dDispImagePartRow1 - m_dDispImagePartRow1 + dHeight;
    }
    if (m_dDispImagePartCol0 >= xOffset *dWidth / wWidth / zoom_scale && m_dDispImagePartCol1 - xOffset *dWidth / wWidth / zoom_scale <= dWidth)
    {
        m_dDispImagePartCol0 = m_dDispImagePartCol0 - xOffset *dWidth / wWidth / zoom_scale;
        m_dDispImagePartCol1 = m_dDispImagePartCol1 - xOffset *dWidth / wWidth / zoom_scale;
    }
    else if (m_dDispImagePartCol0 <= xOffset *dWidth / wWidth / zoom_scale)
    {
        m_dDispImagePartCol1 = m_dDispImagePartCol1 - m_dDispImagePartCol0;
        m_dDispImagePartCol0 = m_dDispImagePartCol0 - m_dDispImagePartCol0;
    }
    else if (m_dDispImagePartCol1 - xOffset *dWidth / wWidth / zoom_scale >= dWidth)
    {
        m_dDispImagePartCol0 = m_dDispImagePartCol0 - m_dDispImagePartCol1 + dWidth;
        m_dDispImagePartCol1 = m_dDispImagePartCol1 - m_dDispImagePartCol1 + dWidth;
    }
    ClearWindow(hWindow);
    SetPart(hWindow, m_dDispImagePartRow0, m_dDispImagePartCol0, m_dDispImagePartRow1-1, m_dDispImagePartCol1-1);
    DispObj(srcImg, hWindow);
}


//初始化变量
void CameraCalibrate::initPar(QString propath)
{
    g_pro_path=propath;
    cam_config_path=propath+"\\camera\\CamConfig.xml";
    initUI();
}
//初始化UI
void CameraCalibrate::initUI()
{
    GeneralFunc::setTableWidgetHeader(ui->tblwgt_ImageList,
                                      QStringList()
                                      <<tr(QString::fromLocal8Bit("图像").toUtf8())
                                      <<tr(QString::fromLocal8Bit("状态").toUtf8()));
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    g_cam_list=m_xml.getChild(QStringList());
    m_xml.closeXml();

    ui->cbboxCamList->clear();
    ui->cbboxCamList->addItems(g_cam_list);

}
void CameraCalibrate::on_pbtn_GenCalTab_clicked()
{
    try
    {
        float circle_Diameter=ui->ledt_CircleDiameter->text().toFloat();
        GenCaltab(7,7,circle_Diameter/1000*2,0.5,QString(g_cam_step_path+"\\cam.descr").toLocal8Bit().data(),
                  QString(g_cam_step_path+"\\cam.ps").toLocal8Bit().data());
        g_cal_par_config->setKeyValue("CircleDiameter",ui->ledt_CircleDiameter->text());
        ui->ledt_CalTabPath->setText(g_cam_step_path+"\\cam.descr");
    }
    catch (HException &e)
    {
        QMessageBox::warning(this,"warning",QString::fromLocal8Bit("参数错误，请检查"));
    }

}

void CameraCalibrate::on_pbtn_CalTabSelect_clicked()
{
    QString filepath=GeneralFunc::getFilePath(g_cam_step_path,this);
    if(filepath=="")
    {
        return;
    }
    ui->ledt_CalTabPath->setText(filepath);
}

void CameraCalibrate::on_cbboxCamList_currentTextChanged(const QString &arg1)
{
    if(arg1=="")return;
    g_cam_select=arg1;
    g_cam_path=g_pro_path+"\\camera\\"+g_cam_select;
    GeneralFunc::isDirExist(g_cam_path,1);
    //读取相机配置
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_cam_info.camType=m_xml.readNode(QStringList()<<g_cam_select<<"CamType");
    m_cam_info.cameraModelSerial=m_xml.readNode(QStringList()<<g_cam_select<<"CameraModelSerial");
    m_cam_info.exposureTime=m_xml.readNode(QStringList()<<g_cam_select<<"ExposureTime");
    m_cam_info.resolution=m_xml.readNode(QStringList()<<g_cam_select<<"Resolution");
    m_cam_info.gain=m_xml.readNode(QStringList()<<g_cam_select<<"Gain");
    m_cam_info.camStepAll=m_xml.readNode(QStringList()<<g_cam_select<<"CamStepAll");
    m_xml.closeXml();
    ui->cbboxCamStep->clear();
    ui->cbboxCamStep->addItems(m_cam_info.camStepAll.split(':'));
    initCalPar();
}

void CameraCalibrate::on_cbboxCamStep_currentTextChanged(const QString &arg1)
{
    if(arg1=="")return;
    g_cam_step_select=arg1;
    initCalPar();
}
void CameraCalibrate::initCalPar()
{
    if(g_cal_par_config!=nullptr)
    {
        delete g_cal_par_config;
        g_cal_par_config=nullptr;
    }
    g_cam_step_path=g_cam_path+"\\"+g_cam_step_select;
    GeneralFunc::isDirExist(g_cam_step_path,1);
    g_cal_par_config=new ConfigFileOperate(g_cam_step_path+"\\CalPar.ini");
    //读取标定配置文件，并初始化控件
    ui->ledt_CellSize->setText(g_cal_par_config->readKeyValue("CellSize"));
    ui->ledt_FocalLength->setText(g_cal_par_config->readKeyValue("FocalLength"));
    ui->ledt_CircleDiameter->setText(g_cal_par_config->readKeyValue("CircleDiameter"));
    ui->ledt_CalTabPath->setText(g_cal_par_config->readKeyValue("CalTabPath"));
    GeneralFunc::removeTableAllRow(ui->tblwgt_ImageList);
    map_hobj.clear();
    QStringList image_list=GeneralFunc::getFileListName(g_cam_step_path,QStringList()<<"*.bmp");
    for (int i=0;i<image_list.size();i++)
    {
        int row_count=ui->tblwgt_ImageList->rowCount();
        ui->tblwgt_ImageList->insertRow(row_count);
        ui->tblwgt_ImageList->setItem(row_count,0,new QTableWidgetItem(image_list[i]));
        ui->tblwgt_ImageList->setItem(row_count,1,new QTableWidgetItem(""));
        QtHalcon::readHObject(&map_hobj[image_list[i]],"bmp",QString(g_cam_step_path+"\\"+image_list[i]).toStdString().data());
    }


    //读取相机信息
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_cam_info.map_step_cal[g_cam_step_select].isCal=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"IsCal");
    m_cam_info.map_step_cal[g_cam_step_select].camPar=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPar");
    m_cam_info.map_step_cal[g_cam_step_select].camPose=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPose");
    m_xml.closeXml();
    ui->cbbox_IsCal->setCurrentIndex(m_cam_info.map_step_cal[g_cam_step_select].isCal.toInt());
    if(m_cam_info.map_step_cal[g_cam_step_select].isCal=="1")
    {
        //初始化标定信息
        CreateCalibData("calibration_object", 1, 1, &hv_cal_data_ID);
        if (m_cam_info.map_step_cal[g_cam_step_select].camPar!="")
        {
            ui->ledt_CamPar->setText(m_cam_info.map_step_cal[g_cam_step_select].camPar);
            hv_cam_par=QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[g_cam_step_select].camPar,',',"double");
            SetCalibDataCamParam(hv_cal_data_ID, 0, ui->cbbox_CamType->currentText().toLocal8Bit().data(), hv_cam_par);
        }
        if(m_cam_info.map_step_cal[g_cam_step_select].camPose!="")
        {
            ui->ledt_CamPose->setText(m_cam_info.map_step_cal[g_cam_step_select].camPose);
            hv_cam_pose=QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[g_cam_step_select].camPose,',',"double");
        }
        if(ui->ledt_CalTabPath->text()!="")
        {
            SetCalibDataCalibObject(hv_cal_data_ID, 0, ui->ledt_CalTabPath->text().toLocal8Bit().data());
        }
    }

}



void CameraCalibrate::on_pbtn_SaveConfig_clicked()
{
    g_cal_par_config->setKeyValue("CellSize",ui->ledt_CellSize->text());
    g_cal_par_config->setKeyValue("FocalLength",ui->ledt_FocalLength->text());
    g_cal_par_config->setKeyValue("CircleDiameter",ui->ledt_CircleDiameter->text());
    g_cal_par_config->setKeyValue("CalTabPath",ui->ledt_CalTabPath->text());

    //初始化相机参数
    hv_cam_start_par.Clear();
    hv_cam_start_par[0] = ui->ledt_FocalLength->text().toDouble()/1000;
    hv_cam_start_par[1] = 0.0;
    hv_cam_start_par[2] = ui->ledt_CellSize->text().toDouble()/1000000;
    hv_cam_start_par[3] = ui->ledt_CellSize->text().toDouble()/1000000;
    QStringList list_resolution=m_cam_info.resolution.split(':');
    if(list_resolution.size()==2)
    {
        hv_cam_start_par.Append(list_resolution[0].toInt()/2);
        hv_cam_start_par.Append(list_resolution[1].toInt()/2);
        hv_cam_start_par.Append(list_resolution[0].toInt());
        hv_cam_start_par.Append(list_resolution[1].toInt());

        SetCalibDataCamParam(hv_cal_data_ID, 0, ui->cbbox_CamType->currentText().toLocal8Bit().data(), hv_cam_start_par);
        SetCalibDataCalibObject(hv_cal_data_ID, 0, ui->ledt_CalTabPath->text().toLocal8Bit().data());
    }
    else
    {
        QMessageBox::warning(this,"warning",QString::fromLocal8Bit("找不到相机分辨率"));
    }
}

void CameraCalibrate::on_pbtnCamOpen_clicked()
{
    //打开相机
    if(ui->pbtnCamOpen->text()==tr(QString::fromLocal8Bit("打开相机").toUtf8()))
    {
        bool issuccess;
        EnumChange::CamType camtype=EnumChange::string2enum_cam(m_cam_info.camType);
        switch (camtype)
        {
        case EnumChange::HikCam:
            m_cam_info.hikvisionCamera=new HikvisionCamera(this);
            issuccess= m_cam_info.hikvisionCamera->initCamera(ui->cbboxCamList->currentText(),m_cam_info.cameraModelSerial,"0","7",m_cam_info.exposureTime,m_cam_info.gain,"0");
            if(issuccess)
            {
                m_cam_info.isOpen=true;
                connect(m_cam_info.hikvisionCamera,&HikvisionCamera::getOneFrame,this,&CameraCalibrate::slot_one_frame);
                ui->pbtnCamOpen->setText(tr(QString::fromLocal8Bit("关闭相机").toUtf8()));
            }
            break;
        case EnumChange::WebCam:
            m_cam_info.webCam=new WebCam(this);
            issuccess=m_cam_info.webCam->initCamera(ui->cbboxCamList->currentText(),m_cam_info.cameraModelSerial,m_cam_info.resolution,"1",m_cam_info.exposureTime,m_cam_info.grabDelay);
            if(issuccess)
            {
                m_cam_info.isOpen=true;
                connect(m_cam_info.webCam,&WebCam::getOneFrame,this,&CameraCalibrate::slot_one_frame);
                ui->pbtnCamOpen->setText(tr(QString::fromLocal8Bit("关闭相机").toUtf8()));
            }
            break;
        case EnumChange::DahuaCam:
            break;
        case EnumChange::DahengCam:
            break;
        case EnumChange::BaslerCam:
            break;
        }
    }
    else if(ui->pbtnCamOpen->text()==tr(QString::fromLocal8Bit("关闭相机").toUtf8()))
    {
        EnumChange::CamType camtype=EnumChange::string2enum_cam(m_cam_info.camType);
        switch (camtype)
        {
        case EnumChange::HikCam:
            disconnect(m_cam_info.hikvisionCamera,&HikvisionCamera::getOneFrame,this,&CameraCalibrate::slot_one_frame);
            m_cam_info.hikvisionCamera->closeDevice();
            delete m_cam_info.hikvisionCamera;
            m_cam_info.hikvisionCamera=nullptr;
            break;
        case EnumChange::WebCam:
            disconnect(m_cam_info.webCam,&WebCam::getOneFrame,this,&CameraCalibrate::slot_one_frame);
            m_cam_info.webCam->closeDevice();
            delete m_cam_info.webCam;
            m_cam_info.webCam=nullptr;
            break;
        case EnumChange::DahuaCam:
            break;
        case EnumChange::DahengCam:
            break;
        case EnumChange::BaslerCam:
            break;
        }
        m_cam_info.isOpen=false;
        ui->pbtnCamOpen->setText(tr(QString::fromLocal8Bit("打开相机").toUtf8()));

    }
}
void CameraCalibrate::slot_one_frame(QImage image,QString cam)
{
    QtHalcon::qImage2HImage(image,m_OriginImage);
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
    CopyImage(m_OriginImage,&m_SceneImage);
    QtHalcon::displayHObject(m_SceneImage,hv_WindowHandle);
    if(map_hobj.size()>1)
    {
        HObject ho_Caltab;
        HTuple camPose;
        HTuple hv_CaltabName,hv_RCoord,hv_CCoord,hv_Index,hv_PoseForCalibrationPlate;
        hv_CaltabName=ui->ledt_CalTabPath->text().toLocal8Bit().data();
        FindCalibObject(m_SceneImage, hv_cal_data_ID, 0, 0, 1, HTuple(), HTuple());
        GetCalibDataObservContours(&ho_Caltab, hv_cal_data_ID, "caltab", 0, 0, 1);
        GetCalibDataObservPoints(hv_cal_data_ID, 0, 0, 1, &hv_RCoord, &hv_CCoord, &hv_Index,
                                 &camPose);
        DispObj(ho_Caltab, hv_WindowHandle);
        DispCaltab(hv_WindowHandle, hv_CaltabName, hv_cam_par, camPose,
                   1);
        DispCircle(hv_WindowHandle, hv_RCoord, hv_CCoord, HTuple(hv_RCoord.TupleLength(),1.5));
    }
}



void CameraCalibrate::on_pbtn_SavePhoto_clicked()
{
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
    QDateTime time=QDateTime::currentDateTime();
    QString save_name=g_cam_step_path+"\\"+time.toString("hhmmss")+".bmp";
    QtHalcon::saveHObject(m_OriginImage,"bmp",save_name.toLocal8Bit().data());

    int row_all=ui->tblwgt_ImageList->rowCount();
    ui->tblwgt_ImageList->insertRow(row_all);
    ui->tblwgt_ImageList->setItem(row_all,0,new QTableWidgetItem(time.toString("hhmmss")));
    ui->tblwgt_ImageList->setItem(row_all,1,new QTableWidgetItem(""));

    map_hobj[time.toString("hhmmss")]=m_OriginImage;
    QMap<QString,HObject>::iterator iter=map_hobj.begin();
    int number=0;
    HObject ho_Caltab;
    HTuple hv_Error;
    while(iter!=map_hobj.end())
    {
        FindCalibObject(iter.value(), hv_cal_data_ID, 0, 0, number, HTuple(), HTuple());
        GetCalibDataObservContours(&ho_Caltab, hv_cal_data_ID, "caltab", 0, 0, number);
        number++;
        iter++;
    }
    CalibrateCameras(hv_cal_data_ID, &hv_Error);
    GetCalibData(hv_cal_data_ID, "camera", 0, "params", &hv_cam_par);
}

void CameraCalibrate::on_pbtn_SetReferencePose_clicked()
{
    HObject select_obj=map_hobj[ui->tblwgt_ImageList->item(ui->tblwgt_ImageList->currentRow(),0)->text()];
    HObject ho_Caltab;
    HTuple hv_CaltabName,hv_RCoord,hv_CCoord,hv_Index,hv_PoseForCalibrationPlate;
    hv_CaltabName=ui->ledt_CalTabPath->text().toLocal8Bit().data();
    FindCalibObject(select_obj, hv_cal_data_ID, 0, 0, 1, HTuple(), HTuple());
    GetCalibDataObservContours(&ho_Caltab, hv_cal_data_ID, "caltab", 0, 0, 1);
    GetCalibDataObservPoints(hv_cal_data_ID, 0, 0, 1, &hv_RCoord, &hv_CCoord, &hv_Index,
                             &hv_cam_pose);
    DispObj(ho_Caltab, hv_WindowHandle);
    DispCaltab(hv_WindowHandle, hv_CaltabName, hv_cam_par, hv_cam_pose,
               1);
    DispCircle(hv_WindowHandle, hv_RCoord, hv_CCoord, HTuple(hv_RCoord.TupleLength(),1.5));
    QString save_par=QtHalcon::htupleDoubleToString(hv_cam_pose,',');
    ui->ledt_CamPose->setText(save_par);
    //写入相机位姿
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPose",save_par);
    m_xml.closeXml();
}

void CameraCalibrate::on_pbtn_RemoveImageOne_clicked()
{

    QMessageBox::StandardButton btn=QMessageBox::question(this,
                                                          QString::fromLocal8Bit("提示"),
                                                          QString::fromLocal8Bit("确实要移除此图片吗?"),
                                                          QMessageBox::Yes|QMessageBox::No);
    if(btn==QMessageBox::Yes)
    {
        QString remove_name=ui->tblwgt_ImageList->item(ui->tblwgt_ImageList->currentRow(),0)->text();
        map_hobj.remove(remove_name);
        QString remove_file_name=g_cam_step_path+"\\"+remove_name+".bmp";
        GeneralFunc::deleteFileOrFolder(remove_file_name);
    }

}

void CameraCalibrate::on_pbtn_RemoveImageAll_clicked()
{
    QMessageBox::StandardButton btn=QMessageBox::question(this,
                                                          QString::fromLocal8Bit("提示"),
                                                          QString::fromLocal8Bit("确实要移除全部吗?"),
                                                          QMessageBox::Yes|QMessageBox::No);
    if(btn==QMessageBox::Yes)
    {
        QMap<QString,HObject>::iterator iter=map_hobj.begin();
        while(iter!=map_hobj.end())
        {
            QString remove_file_name=g_cam_step_path+"\\"+iter.key()+".bmp";
            GeneralFunc::deleteFileOrFolder(remove_file_name);
            iter++;
        }
    }
}

void CameraCalibrate::on_pbtn_Cal_clicked()
{
    QMap<QString,HObject>::iterator iter=map_hobj.begin();
    int number=0;
    HObject ho_Caltab;
    HTuple hv_Error;
    while(iter!=map_hobj.end())
    {
        FindCalibObject(iter.value(), hv_cal_data_ID, 0, 0, number, HTuple(), HTuple());
        GetCalibDataObservContours(&ho_Caltab, hv_cal_data_ID, "caltab", 0, 0, number);
        number++;
        iter++;
    }
    CalibrateCameras(hv_cal_data_ID, &hv_Error);
    GetCalibData(hv_cal_data_ID, "camera", 0, "params", &hv_cam_par);
    QString save_par=QtHalcon::htupleDoubleToString(hv_cam_par,',');
    ui->ledt_CamPar->setText(save_par);
    //写入相机内参
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"IsCal","1");
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPar",save_par);
    m_xml.closeXml();

}

void CameraCalibrate::on_tblwgt_ImageList_cellClicked(int row, int column)
{
    QString select_obj_name=ui->tblwgt_ImageList->item(row,0)->text();
    m_SceneImage=map_hobj[ui->tblwgt_ImageList->item(ui->tblwgt_ImageList->currentRow(),0)->text()];
    HObject ho_Caltab;
    HTuple hv_CaltabName,hv_RCoord,hv_CCoord,hv_Index,hv_PoseForCalibrationPlate;
    hv_CaltabName=ui->ledt_CalTabPath->text().toLocal8Bit().data();
    displayImage(m_SceneImage,hv_WindowHandle);
    SetCalibDataCamParam(hv_cal_data_ID, 0, ui->cbbox_CamType->currentText().toLocal8Bit().data(), hv_cam_par);
    SetCalibDataCalibObject(hv_cal_data_ID, 0, hv_CaltabName);
    FindCalibObject(m_SceneImage, hv_cal_data_ID, 0, 0, 1, HTuple(), HTuple());
    GetCalibDataObservContours(&ho_Caltab, hv_cal_data_ID, "caltab", 0, 0, 1);
    GetCalibDataObservPoints(hv_cal_data_ID, 0, 0, 1, &hv_RCoord, &hv_CCoord, &hv_Index,
                             &hv_PoseForCalibrationPlate);
    DispObj(ho_Caltab, hv_WindowHandle);
    DispCaltab(hv_WindowHandle, hv_CaltabName, hv_cam_par, hv_PoseForCalibrationPlate,
               1);
    DispCircle(hv_WindowHandle, hv_RCoord, hv_CCoord, HTuple(hv_RCoord.TupleLength(),1.5));
}

