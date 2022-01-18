#include "imagecalibrate.h"
#include "ui_imagecalibrate.h"
#include"general/generalfunc.h"
#include"general/xmloperate.h"

ImageCalibrate::ImageCalibrate(QWidget *parent) :
    QWidget(parent),
    zoom_scale(1),
    ui(new Ui::ImageCalibrate)
{
    ui->setupUi(this);
}

ImageCalibrate::~ImageCalibrate()
{
    delete ui;
}
void ImageCalibrate::resizeEvent(QResizeEvent *event)
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
void ImageCalibrate::mousePressEvent(QMouseEvent *event)
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

void ImageCalibrate::mouseMoveEvent(QMouseEvent *event)
{
    QPoint s(ui->lbl_Image->pos());//控件在窗口内的坐标
    mousePoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
}

void ImageCalibrate::mouseReleaseEvent(QMouseEvent *event)
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

void ImageCalibrate::wheelEvent(QWheelEvent *event)
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

void ImageCalibrate::displayImage(HImage srcImg,HTuple hv_Window)
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

void ImageCalibrate::moveWnd(QPoint pointStart,QPoint pointEnd, HImage srcImg, HTuple hWindow)
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
void ImageCalibrate::initPar(QString propath)
{
    g_pro_path=propath;
    cam_config_path=propath+"\\camera\\CamConfig.xml";
    initUI();
}
//初始化UI
void ImageCalibrate::initUI()
{
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    g_cam_list=m_xml.getChild(QStringList());
    m_xml.closeXml();

    ui->cbbox_CamList->clear();
    ui->cbbox_CamList->addItems(g_cam_list);
}

void ImageCalibrate::on_pbtn_Read_Image_clicked()
{
    //读取图片
    QString filepath=GeneralFunc::getFilePath(g_cam_path,this);
    if(filepath=="")
    {
        return;
    }
    HalconCpp::ReadImage(&m_OriginImage,filepath.toStdString().data());
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
    HalconCpp::CopyImage(m_OriginImage,&m_SceneImage);
    displayImage(m_SceneImage,hv_WindowHandle);
}

void ImageCalibrate::on_cbbox_CamList_currentTextChanged(const QString &arg1)
{
    g_cam_select=arg1;
    g_cam_path=g_pro_path+"\\camera\\"+g_cam_select;
    ui->cbbox_CamStep->clear();
    ui->cbbox_CamStep->addItems(GeneralFunc::GetAllFolderName(g_cam_path));
    initCalPar();
}

void ImageCalibrate::on_cbbox_CamStep_currentTextChanged(const QString &arg1)
{
    g_cam_step_select=arg1;
    initCalPar();
}
void ImageCalibrate::initCalPar()
{
    g_cam_step_path=g_cam_path+"\\"+g_cam_step_select;
    //读取相机信息
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    QString resolution=m_xml.readNode(QStringList()<<g_cam_select<<"Resolution");
    QString camPar=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPar");
    QString camPose=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"CamPose");
    ratio=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"Ratio").toDouble();
    x_offset=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"X_Offset").toDouble();
    y_offset=m_xml.readNode(QStringList()<<g_cam_select<<g_cam_step_select<<"Y_Offset").toDouble();
    m_xml.closeXml();
    if(GeneralFunc::isFileExist(g_cam_step_path+"\\map.hobj",0))
    {
        QtHalcon::readHObject(&hv_mappedfix,"hobj",QString(g_cam_step_path+"\\map").toLocal8Bit().data());
    }
    ui->ledt_Ratio->setText(QString::number(ratio,'f',10));
    ui->ledt_X_Offset->setText(QString::number(x_offset,'f',10));
    ui->ledt_Y_Offset->setText(QString::number(y_offset,'f',10));
    //初始化标定信息
    QStringList relist=resolution.split(':');
    if(relist.size()==2)
    {
        image_width=relist[0].toDouble();
        image_height=relist[1].toDouble();
    }
    if (camPar!="")
    {
        hv_cam_par=QtHalcon::qstringToHtuple(camPar,',',"double");
    }
    if(camPose!="")
    {
        hv_cam_pose=QtHalcon::qstringToHtuple(camPose,',',"double");
    }
}
void ImageCalibrate::on_pbtn_Cal_Image_clicked()
{
    ratio=ui->ledt_Ratio->text().toDouble();
    x_offset=ui->ledt_X_Offset->text().toDouble();
    y_offset=ui->ledt_Y_Offset->text().toDouble();
    world_width=image_width*ratio;
    world_height=image_height*ratio;
    QtHalcon::calibrateImage(&hv_mappedfix, image_width, image_height,
                             world_width,world_height,hv_cam_par,hv_cam_pose,
                             ratio, x_offset, y_offset);
    MapImage(m_OriginImage,hv_mappedfix,&m_SceneImage);
    displayImage(m_SceneImage,hv_WindowHandle);
}

void ImageCalibrate::on_pbtn_Save_clicked()
{
    //写入相机内参
    XmlOperate m_xml;
    m_xml.openXml(cam_config_path);
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"WorldWidth",QString::number(world_width,'f',5));
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"WorldHeight",QString::number(world_height,'f',5));
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"X_Offset",QString::number(x_offset,'f',5));
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"Y_Offset",QString::number(y_offset,'f',5));
    m_xml.addNode(QStringList()<<g_cam_select<<g_cam_step_select<<"Ratio",QString::number(ratio,'f',10));
    m_xml.closeXml();
    QtHalcon::saveHObject(hv_mappedfix,"hobj",QString(g_cam_step_path+"\\map").toLocal8Bit().data());
}


