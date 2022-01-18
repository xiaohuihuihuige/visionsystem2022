#include "modelparset.h"
#include "ui_modelparset.h"
#include"general/generalfunc.h"
#include<QMessageBox>
#include"general/configfileoperate.h"
#include"camera/hikvisioncamera.h"
#include<QPainter>
#include<QCursor>
#include<QLineEdit>
#include<QInputDialog>
#include"general/xmloperate.h"

//选区名
//选区类型
//选区功能一
//功能参数
//选区功能二
//功能参数


ModelParSet::ModelParSet(QWidget *parent) :
    QWidget(parent),
    isDrawing(false),
    zoom_scale(1),
    isNew(false),
    m_Combobox_cur(nullptr),
    ui(new Ui::ModelParSet)
{
    ui->setupUi(this);
    //setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
    ui->tblwgtROI->viewport()->installEventFilter(this);
    GenEmptyObj(&m_OriginImage);
    GenEmptyObj(&m_SceneImage_Draw);
    //添加选区类别
    this->setFocusPolicy(Qt::StrongFocus);
    m_cam_info=CamInfo();

}

ModelParSet::~ModelParSet()
{
    delete ui;
}
bool ModelParSet::eventFilter(QObject *obj, QEvent *e)
{
    if (obj == ui->tblwgtROI->viewport())
    {
        if (e->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent * pMouseEvent = static_cast<QMouseEvent *>(e);
            // check whether it's mid button pressed
            if (pMouseEvent->button() == Qt::RightButton)
            {
                //do the processing and return true
                mouse_right_click_in_tblwgt();
            }
        }
    }
    return QWidget::eventFilter(obj,e);
}

void ModelParSet::resizeEvent(QResizeEvent *event)
{
    if(hv_WindowHandle.Length()>0)
    {
        HalconCpp::CloseWindow(hv_WindowHandle);
    }
    //初始化窗口
    Hlong windID = Hlong(this->ui->label->winId());
    HalconCpp::OpenWindow(0,0,ui->label->width(),ui->label->height(),windID,"visible","",&hv_WindowHandle);
    SetColor(hv_WindowHandle,"green");
    SetDraw(hv_WindowHandle,"margin");
    SetLineWidth(hv_WindowHandle,3);
}

void ModelParSet::mousePressEvent(QMouseEvent *event)
{
    if(isDrawing)return;


    QPoint s(ui->label->pos());//控件在窗口内的坐标
    lastPoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
    if(ui->label->rect().contains(lastPoint))
    {
        if(event->button()==Qt::RightButton)
        {
            if(keyLast.mid(0,8)=="modelroi")
            {
                create_and_show_model();
            }
        }
        if(event->button()==Qt::LeftButton)
        {
            if(QtHalcon::testHObjectEmpty(m_SceneImage_Draw))return;
            mouseDown = true;
            firstPoint = lastPoint;
            HTuple pos,value ,length;
            QtHalcon::getMousePosGray( hv_WindowHandle,m_SceneImage_Draw,&pos,&value);
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
            ui->lblPosInfo->setText(posinfo);
        }
    }
}

void ModelParSet::mouseMoveEvent(QMouseEvent *event)
{
    QPoint s(ui->label->pos());//控件在窗口内的坐标
    mousePoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标

    /*HTuple num;
    CountObj(m_SceneImage_Draw,&num);
    if(!(num.I()>0))return;
    if(isDrawing||event->button()==Qt::RightButton)return;
    QPoint s(ui->label->pos());//控件在窗口内的坐标
    lastPoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
    if(ui->label->rect().contains(lastPoint))
    {
        windowPoint = lastPoint;
    }
    else
    {
        windowPoint=QPoint(-1,-1);
    }*/
}

void ModelParSet::mouseReleaseEvent(QMouseEvent *event)
{

    if(isDrawing||event->button()==Qt::RightButton)return;
    QPoint s(ui->label->pos());//控件在窗口内的坐标
    lastPoint = QPoint(event->pos().x() - s.x(), event->pos().y() - s.y());//鼠标在控件上的坐标
    if(ui->label->rect().contains(lastPoint))
    {
        if(QtHalcon::testHObjectEmpty(m_SceneImage_Draw))return;

        if(lastPoint.x()!=0&&lastPoint.y()!=0&&zoom_scale>1)
        {
            moveWnd(firstPoint,lastPoint, m_SceneImage_Draw, hv_WindowHandle);
            mouseDown = false;
        }
    }

}

void ModelParSet::wheelEvent(QWheelEvent *event)
{
    if(isDrawing)return;
    if(QtHalcon::testHObjectEmpty(m_SceneImage_Draw))return;
    if(ui->label->rect().contains(mousePoint))
    {
        short zDelta =short(event->delta());
        if (zDelta>0)           //图像放大
        {
            if (zoom_scale<6)      //最大放大6倍
            {
                zoom_scale = zoom_scale*1.05;
                displayImage(m_SceneImage_Draw, hv_WindowHandle);
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
                displayImage(m_SceneImage_Draw, hv_WindowHandle);
            }
        }
    }

}

void ModelParSet::displayImage(HImage srcImg,HTuple hv_Window)
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

void ModelParSet::moveWnd(QPoint pointStart,QPoint pointEnd, HImage srcImg, HTuple hWindow)
{
    QRect m_rPic=ui->label->rect();
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

void ModelParSet::initPar(QString pro,QString station,QString cam,QString step,QString type)
{
    QString exePath=qApp->applicationDirPath();
    m_ProPath=exePath+"\\Pro\\"+pro;
    m_cam=cam;
    m_step=step;
    m_type=type;
    m_type_path=m_ProPath+"\\station\\"+station+"\\"+cam+"\\"+m_step+"\\"+type;
    m_Typeroi_path=m_type_path+"\\roi";
    //如果没有选区文件夹则船舰一个
    GeneralFunc::isDirExist(m_Typeroi_path,1);

    readCamInfo();
    //读取选区
    readROI();
    initUI();
}

void ModelParSet::closeEvent(QCloseEvent *event)
{
    //在关闭之前先判读那相机是否关闭，若为否，则忽略关闭事件
    if(m_cam_info.isOpen)
    {
        QMessageBox::warning(this,"Warning",tr(QString::fromLocal8Bit("请先关闭相机,再关闭此页面").toUtf8()));
        event->ignore();
        return;
    }
    emit signal_roi_change();
    event->accept();
}

//读入图片
void ModelParSet::on_pbtnReadImage_clicked()
{
    //读取图片
    QString filepath=GeneralFunc::getFilePath(m_type_path,this);
    if(filepath=="")
    {
        return;
    }
    HalconCpp::ReadImage(&m_OriginImage,filepath.toLocal8Bit().data());
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
    HalconCpp::CopyImage(m_OriginImage,&m_SceneImage_Draw);
    displayImage(m_SceneImage_Draw,hv_WindowHandle);
}

//拍摄照片
void ModelParSet::on_pbtnTakePhoto_clicked()
{
    //拍照
    GenEmptyObj(&m_OriginImage);
    GenEmptyObj(&m_SceneImage_Draw);
    if(m_cam_info.isOpen)
    {
        EnumChange::CamType camtype=EnumChange::string2enum_cam(m_cam_info.camType);
        switch (camtype)
        {
        case EnumChange::HikCam:
            m_cam_info.hikvisionCamera->doSoftwareOnce();
            break;
        case EnumChange::WebCam:
            m_cam_info.webCam->doSoftwareOnce();
            break;
        }
    }
}
void ModelParSet::slot_one_frame(QImage image,QString cam)
{
    QtHalcon::qImage2HImage(image,m_OriginImage);
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;
    CopyImage(m_OriginImage,&m_SceneImage_Draw);
    QtHalcon::displayHObject(m_SceneImage_Draw,hv_WindowHandle);
}
void ModelParSet::readCamInfo()
{
    QString xmlPath=m_ProPath+"\\camera\\CamConfig.xml";
    //读取相机配置
    XmlOperate m_xml;
    m_xml.openXml(xmlPath);
    m_cam_info.camType=m_xml.readNode(QStringList()<<m_cam<<"CamType");
    m_cam_info.cameraModelSerial=m_xml.readNode(QStringList()<<m_cam<<"CameraModelSerial");
    m_cam_info.exposureTime=m_xml.readNode(QStringList()<<m_cam<<"ExposureTime");
    m_cam_info.resolution=m_xml.readNode(QStringList()<<m_cam<<"Resolution");
    m_cam_info.gain=m_xml.readNode(QStringList()<<m_cam<<"Gain");
    m_cam_info.map_step_cal[m_step].isCal=m_xml.readNode(QStringList()<<m_cam<<m_step<<"IsCal");
    if(m_cam_info.map_step_cal[m_step].isCal=="1")
    {
        m_cam_info.map_step_cal[m_step].ratio=m_xml.readNode(QStringList()<<m_cam<<m_step<<"Ratio");
        m_cam_info.map_step_cal[m_step].camPar=m_xml.readNode(QStringList()<<m_cam<<m_step<<"CamPar");
        m_cam_info.map_step_cal[m_step].camPose=m_xml.readNode(QStringList()<<m_cam<<m_step<<"CamPose");
        m_cam_info.map_step_cal[m_step].X_Offset=m_xml.readNode(QStringList()<<m_cam<<m_step<<"X_Offset");
        m_cam_info.map_step_cal[m_step].Y_Offset=m_xml.readNode(QStringList()<<m_cam<<m_step<<"Y_Offset");
        m_cam_info.map_step_cal[m_step].worldWidth=m_xml.readNode(QStringList()<<m_cam<<m_step<<"WorldWidth");
        m_cam_info.map_step_cal[m_step].worldHeight=m_xml.readNode(QStringList()<<m_cam<<m_step<<"WorldHeight");
    }
    m_xml.closeXml();
}
//读取选区文件
void ModelParSet::readROI()
{
    map_hobj.clear();
    QStringList  roilist=GeneralFunc::getFileListName(m_Typeroi_path,QStringList()<<"*.*");;
    for (int i=0;i<roilist.size();i++)
    {
        QStringList roi_type=roilist[i].split('#');

        switch (EnumChange::string2enum_roitype(roi_type[1]))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roilist[i]+".hobj"),0))
            {
                ReadObject(&map_hobj[roilist[i]],QString(m_Typeroi_path+"\\"+roilist[i]+".hobj").toLocal8Bit().data());
            }
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roilist[i]),0))
            {
                ReadObject(&map_hobj[roilist[i]],QString(m_Typeroi_path+"\\"+roilist[i]).toLocal8Bit().data());

            }
            break;
        }
    }
}
//初始化UI界面
void ModelParSet::initUI()
{
    GeneralFunc::removeTableAllRow(ui->tblwgtROI);
    //初始化表头
    GeneralFunc::setTableWidgetHeader(ui->tblwgtROI,
                                      QStringList()
                                      <<tr(QString::fromLocal8Bit("选区名").toUtf8())
                                      <<tr(QString::fromLocal8Bit("选区类型").toUtf8()));
    ui->tblwgtROI->setColumnWidth(0,200);
    if(map_hobj.size()<1)
        return;
    isNew=true;

    QMap<QString,HObject>::iterator iter=map_hobj.begin();
    while(iter!=map_hobj.end())
    {
        QStringList liststr=iter.key().split('#');
        if(liststr.size()<2)
        {
            continue;
        }
        QString name=liststr[0];
        QString roitype=liststr[1];

        int row=ui->tblwgtROI->rowCount();
        ui->tblwgtROI->insertRow(row);

        QTableWidgetItem *item=new QTableWidgetItem(name);
        ui->tblwgtROI->setItem(row,0,item);

        item=new QTableWidgetItem(roitype);
        ui->tblwgtROI->setItem(row,1,item);
        iter++;
    }
    isNew=false;

}

void ModelParSet::on_pbtnAdd_clicked()
{
    //增加选区
    isNew=true;
    int row=ui->tblwgtROI->rowCount();
    ui->tblwgtROI->insertRow(row);
    QTableWidgetItem *item=new QTableWidgetItem(QString("Mark"));
    ui->tblwgtROI->setItem(row,0,item);

    item=new QTableWidgetItem(QString("Rectangle1"));
    ui->tblwgtROI->setItem(row,1,item);

    isNew=false;

}

void ModelParSet::on_pbtnEdit_clicked()
{
    //修改选区
    int row=ui->tblwgtROI->currentRow();
    if(row<0)return;
    QString roiType=ui->tblwgtROI->item(row,1)->text();

    QString region_name=ui->tblwgtROI->item(row,0)->text();
    HObject ho_object;
    QString obj_str;
    QString save_type;
    isDrawing=true;
    switch (EnumChange::string2enum_roitype(roiType))
    {
    case EnumChange::Rectangle1:
        QtHalcon::drawRectangle1ROI(hv_WindowHandle,&ho_object,&obj_str);
        save_type="hobj";
        break;
    case EnumChange::Rectangle2:
        QtHalcon::drawRectangle2ROI(hv_WindowHandle,&ho_object,&obj_str);
        save_type="hobj";
        break;
    case EnumChange::Circle:
        QtHalcon::drawCircleROI(hv_WindowHandle,&ho_object,&obj_str);
        save_type="hobj";
        break;
    case EnumChange::Polygon:
        QtHalcon::drawPolygonROI(hv_WindowHandle,&ho_object);
        save_type="hobj";
        break;
    case EnumChange::Xld:
        QtHalcon::drawXld(hv_WindowHandle,&ho_object);
        save_type="xld";
        break;
    case EnumChange::Cirque:
    {
        HObject ho_object1,ho_object2;
        QString obj_str1,obj_str2;
        QtHalcon::drawCircleROI(hv_WindowHandle,&ho_object1,&obj_str1);
        QtHalcon::drawCircleROI(hv_WindowHandle,&ho_object2,&obj_str2);
        QStringList circle_list2=obj_str2.split('|');
        obj_str=obj_str1+'|'+circle_list2[2];
        QtHalcon::getRoi(roiType,obj_str,&ho_object);
        save_type="hobj";
        break;
    }
    case EnumChange::Nurb:
        QtHalcon::drawNurb(hv_WindowHandle,&ho_object);
        save_type="xld";
        break;
    }
    isDrawing=false;
    map_hobj[region_name+"#"+roiType]=ho_object;
    if(GeneralFunc::isDirExist(m_Typeroi_path,1))
    {
        QtHalcon::saveHObject(ho_object,save_type.toLocal8Bit().data(),QString(m_Typeroi_path+"\\"+region_name+"#"+roiType).toLocal8Bit().data());
    }
    HalconCpp::DispObj(map_hobj[region_name+"#"+roiType],hv_WindowHandle);
}


void ModelParSet::on_tblwgtROI_cellChanged(int row, int column)
{
    //单元格内容变化事件
    if(isNew)
    {
        return;
    }
    switch (column)
    {
    case 0:
    {
        //修改第一列时，删除原有的，添加新的
        keyNow=ui->tblwgtROI->item(row,0)->text();
        QString roiType=ui->tblwgtROI->item(row,1)->text();
        if(!map_hobj.contains(keyLast+"#"+roiType))
        {
            GenEmptyObj(&map_hobj[keyLast+"#"+roiType]);
        }
        map_hobj[keyNow+"#"+roiType]=map_hobj[keyLast+"#"+roiType];
        map_hobj.remove(keyLast+"#"+roiType);
        switch (EnumChange::string2enum_roitype(roiType))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            QtHalcon::saveHObject(map_hobj[keyNow+"#"+roiType],"hobj",QString(m_Typeroi_path+"\\"+keyNow+"#"+roiType).toLocal8Bit().data());
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+keyLast+"#"+roiType+".hobj"),0))
            {
                QtHalcon::deleteFile("hobj",QString(m_Typeroi_path+"\\"+keyLast+"#"+roiType).toLocal8Bit().data());
            }
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            QtHalcon::saveHObject(map_hobj[keyNow+"#"+roiType],"xld",QString(m_Typeroi_path+"\\"+keyNow+"#"+roiType).toLocal8Bit().data());
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+keyLast+"#"+roiType),0))
            {
                QtHalcon::deleteFile("xld",QString(m_Typeroi_path+"\\"+keyLast+"#"+roiType).toLocal8Bit().data());
            }
            break;
        }
        break;
    }
    case 1:
    {
        //修改第2列时，删除原有的，添加新的
        QString roi_name=ui->tblwgtROI->item(row,0)->text();
        QString roiType=ui->tblwgtROI->item(row,1)->text();
        if(!map_hobj.contains(roi_name+"#"+keyLast))
        {
            GenEmptyObj(&map_hobj[roi_name+"#"+keyLast]);
        }
        //赋值变量，保存文件
        map_hobj[roi_name+"#"+roiType]=map_hobj[roi_name+"#"+keyLast];
        switch (EnumChange::string2enum_roitype(roiType))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            QtHalcon::saveHObject(map_hobj[roi_name+"#"+roiType],"hobj",QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType).toLocal8Bit().data());
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            QtHalcon::saveHObject(map_hobj[roi_name+"#"+roiType],"xld",QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType).toLocal8Bit().data());
            break;
        }

        //清楚变量，删除文件
        map_hobj.remove(roi_name+"#"+keyLast);
        switch (EnumChange::string2enum_roitype(keyLast))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roi_name+"#"+keyLast+".hobj"),0))
            {
                QtHalcon::deleteFile("hobj",QString(m_Typeroi_path+"\\"+roi_name+"#"+keyLast).toLocal8Bit().data());
            }
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roi_name+"#"+keyLast),0))
            {
                QtHalcon::deleteFile("xld",QString(m_Typeroi_path+"\\"+roi_name+"#"+keyLast).toLocal8Bit().data());
            }
            break;
        }
        break;
    }
    default:
        break;
    }
}

void ModelParSet::on_tblwgtROI_cellClicked(int row, int column)
{
    //单元格点击事件，点击第一列和第三列时，显示选区
    if(column==1)return;
    //单机行显示内容
    keyLast=ui->tblwgtROI->item(row,column)->text();
    QString roiType=ui->tblwgtROI->item(row,1)->text();
    if(map_hobj.contains(keyLast+"#"+roiType))
    {
        DispObj(map_hobj[keyLast+"#"+roiType],hv_WindowHandle);
        if(keyLast.mid(0,8)=="modelroi")
        {
            readModelPar(keyLast);
            if(GeneralFunc::isFileExist(QString(m_type_path+"\\"+keyLast+".shm"),0))
            {
                HTuple m_ModelID;
                HObject ho_ShapeModel,ho_ShapeModel_trans;
                HalconCpp::ReadShapeModel(QString(m_type_path+"\\"+keyLast+".shm").toLocal8Bit().data(),&m_ModelID);
                GetShapeModelContours(&ho_ShapeModel, m_ModelID, 1);

                QString end;
                QStringList roistrList=keyLast.split("_");
                end=roistrList[1];

                QString xmlPath=m_type_path+"\\ROIConfig.xml";
                //读取相机配置
                XmlOperate m_xml;
                m_xml.openXml(xmlPath);
                HTuple HomMat2D=QtHalcon::qstringToHtuple(m_xml.readNode(QStringList()<<keyLast<<"MovementOfObject_Model_M"),',',"double");
                if(HomMat2D.TupleLength()<3)return;
                HalconCpp::AffineTransContourXld(ho_ShapeModel, &ho_ShapeModel_trans, HomMat2D);
                DispObj(ho_ShapeModel_trans,hv_WindowHandle);
                m_xml.closeXml();
            }
        }
    }
}

void ModelParSet::on_pbtnSave_clicked()
{
    if(QtHalcon::testHObjectEmpty(m_OriginImage))return;

    //输入字符串
    QString dlgTitle=QString::fromLocal8Bit("保存图片");
    QString txtLabel=QString::fromLocal8Bit("文件名");
    QString defaultInput="image0";
    QLineEdit::EchoMode echoMode=QLineEdit::Normal;//正常文字输入
    //QLineEdit::EchoMode echoMode=QLineEdit::Password;//密码输入
    bool ok=false;
    QString text = QInputDialog::getText(this, dlgTitle,txtLabel, echoMode,defaultInput, &ok);
    if (ok && !text.isEmpty())
    {
        try
        {
            //保存模板图片
            QString saveFolder=m_type_path;
            GeneralFunc::isDirExist(saveFolder,true);
            QtHalcon::saveHObject(m_OriginImage,"jpg",QString(m_type_path+"\\"+text+".jpg").toLocal8Bit().data());
        }
        catch (HalconCpp::HException &e)
        {
            QMessageBox::warning(this,QString::fromLocal8Bit("warning"),QString::fromLocal8Bit("保存图像错误"));
            return;
        }
    }

}

void ModelParSet::on_pbtnDelete_clicked()
{
    //删除一个选区
    int row=ui->tblwgtROI->currentRow();
    if(row<0)return;
    QString roi_name=ui->tblwgtROI->item(row,0)->text();
    QString roiType=ui->tblwgtROI->item(row,1)->text();
    if(map_hobj.contains(roi_name+"#"+roiType))
    {
        map_hobj.remove(roi_name+"#"+roiType);
        switch (EnumChange::string2enum_roitype(roiType))
        {
        case EnumChange::Rectangle1:
        case EnumChange::Rectangle2:
        case EnumChange::Circle:
        case EnumChange::Polygon:
        case EnumChange::Cirque:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType+".hobj"),0))
            {
                QtHalcon::deleteFile("hobj",QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType).toLocal8Bit().data());
            }
            break;
        case EnumChange::Nurb:
        case EnumChange::Xld:
            if(GeneralFunc::isFileExist(QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType),0))
            {
                QtHalcon::deleteFile("xld",QString(m_Typeroi_path+"\\"+roi_name+"#"+roiType).toLocal8Bit().data());
            }
            break;
        }
    }
    ui->tblwgtROI->removeRow(row);
}

void ModelParSet::on_pbtnCamOpen_clicked()
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
            issuccess= m_cam_info.hikvisionCamera->initCamera(m_cam,m_cam_info.cameraModelSerial,"1","7",m_cam_info.exposureTime,m_cam_info.gain,"0");
            if(issuccess)
            {
                m_cam_info.isOpen=true;
                connect(m_cam_info.hikvisionCamera,&HikvisionCamera::getOneFrame,this,&ModelParSet::slot_one_frame);
                ui->pbtnCamOpen->setText(tr(QString::fromLocal8Bit("关闭相机").toUtf8()));
            }
            break;
        case EnumChange::WebCam:
            m_cam_info.webCam=new WebCam(this);
            issuccess=m_cam_info.webCam->initCamera(m_cam,m_cam_info.cameraModelSerial,m_cam_info.resolution,"1",m_cam_info.exposureTime,m_cam_info.grabDelay);
            if(issuccess)
            {
                m_cam_info.isOpen=true;
                connect(m_cam_info.webCam,&WebCam::getOneFrame,this,&ModelParSet::slot_one_frame);
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
            disconnect(m_cam_info.hikvisionCamera,&HikvisionCamera::getOneFrame,this,&ModelParSet::slot_one_frame);
            m_cam_info.hikvisionCamera->closeDevice();
            delete m_cam_info.hikvisionCamera;
            m_cam_info.hikvisionCamera=nullptr;
            break;
        case EnumChange::WebCam:
            disconnect(m_cam_info.webCam,&WebCam::getOneFrame,this,&ModelParSet::slot_one_frame);
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

void ModelParSet::keyPressEvent(QKeyEvent *ev)
{
    if(ev->key() == Qt::Key_M)
    {
        //QMessageBox::warning(this,"aa","aa");
        create_and_show_model();
        return;
    }
    QWidget::keyPressEvent(ev);
}
void ModelParSet::keyReleaseEvent(QKeyEvent *ev)
{
    /*if(ev->key() == Qt::Key_M)
    {
        return;
    }
    QWidget::keyReleaseEvent(ev);*/
}

void ModelParSet::create_and_show_model()
{
    try
    {
        //获取模板区域
        int row=ui->tblwgtROI->currentRow();
        int col=ui->tblwgtROI->currentColumn();
        if(col==1)return;
        QString modelName=ui->tblwgtROI->item(row,0)->text();
        if(keyLast.mid(0,8)!="modelroi")return;
        QString roiType=ui->tblwgtROI->item(row,1)->text();
        HObject model_Region;
        if(map_hobj.contains(keyLast+"#"+roiType))
        {
            model_Region=map_hobj[keyLast+"#"+roiType];
        }
        //获取训练模板参数
        HTuple modelID = HTuple();
        HTuple num_Level=ui->ledtNumLevel->text().toInt();
        HTuple angle_Start=HTuple(ui->ledtAngleStart->text().toFloat());
        HTuple angleStep = HTuple(ui->ledtAngleStep->text().toFloat());
        HTuple min_Contrast=ui->ledtMinContrast->text().toInt();
        HTuple contrast=QtHalcon::qstringToHtuple(ui->ledtComtrast->text(),',',"int");;
        HTuple optimization=QtHalcon::qstringToHtuple(ui->ledtOptimization->text(),',',"string");;
        HTuple m_Row_Center_M=ui->ledt_Center_Row->text().toInt();
        HTuple m_Column_Center_M=ui->ledt_Center_Col->text().toInt();
        //获取查找模板参数
        HTuple numToFind = ui->ledtNumToFind->text().toInt();
        HTuple min_Score=ui->ledtMinScore->text().toFloat();
        HTuple max_Overlap=ui->ledtMaxOverlap->text().toFloat();
        HTuple sub_Pixel=ui->ledtSubPixel->text().remove('\'').toLocal8Bit().data();
        HTuple greediness=ui->ledtGreedness->text().toFloat();
        HTuple num_Levels=QtHalcon::qstringToHtuple(ui->ledtNumLevels->text(),',',"int");

        //训练模板
        HObject ho_Cross_M;
        HTuple movementOfObject_Model_M;
        QtHalcon::createModel(m_SceneImage_Draw, model_Region, &ho_Cross_M, m_Row_Center_M, m_Column_Center_M,
                              num_Level, angle_Start, min_Contrast, contrast, optimization,
                              angleStep, &modelID, &movementOfObject_Model_M);
        if(!(modelID.TupleLength()>0))return;
        HalconCpp::WriteShapeModel(modelID, QString(m_type_path+"\\"+modelName+".shm").toLocal8Bit().data());
        //查找模板
        HObject ho_ModelAtNewPosition,ho_Cross;
        HTuple hv_Success,hv_MovementOfObject_Model_S,hv_Row,hv_Column,hv_Angle,hv_Score;
        QtHalcon::checkModel(m_SceneImage_Draw, &ho_ModelAtNewPosition, &ho_Cross, modelID, numToFind,
                             angle_Start, min_Score, max_Overlap, sub_Pixel, num_Levels,
                             greediness, &hv_Success, &hv_MovementOfObject_Model_S, &hv_Row, &hv_Column,
                             &hv_Angle, &hv_Score);
        DispObj(ho_ModelAtNewPosition,hv_WindowHandle);


        QString xmlPath=m_type_path+"\\ROIConfig.xml";
        //读取相机配置
        XmlOperate m_xml;
        m_xml.openXml(xmlPath);
        m_xml.addNode(QStringList()<<modelName<<"MovementOfObject_Model_M",QtHalcon::htupleDoubleToString(movementOfObject_Model_M,','));
        m_xml.addNode(QStringList()<<modelName<<"Row_M",QString::number(hv_Row.D()));
        m_xml.addNode(QStringList()<<modelName<<"Col_M",QString::number(hv_Column.D()));
        m_xml.addNode(QStringList()<<modelName<<"Ang_M",QString::number(hv_Angle.D()));
        m_xml.closeXml();

    }
    catch (HalconCpp::HException &e)
    {
        QMessageBox::warning(this,"warning",QString::fromLocal8Bit("模板创建失败，请检查参数是否正确"));
        return;
    }


}

void ModelParSet::on_pbtnWriteModelPar_clicked()
{
    int row=ui->tblwgtROI->currentRow();
    int col=ui->tblwgtROI->currentColumn();
    if(col==1)
    {
        QMessageBox::warning(this,"warning",QString::fromLocal8Bit("请选中选区名"));
        return;
    }
    QString modelName=ui->tblwgtROI->item(row,0)->text();
    if(keyLast.mid(0,8)!="modelroi")
    {
        QMessageBox::warning(this,"warning",QString::fromLocal8Bit("请选择一个模板选区"));
        return;
    }
    XmlOperate m_xml;
    m_xml.openXml(m_type_path+"\\ROIConfig.xml");
    m_xml.addNode(QStringList()<<modelName<<"Num_Level",ui->ledtNumLevel->text());
    m_xml.addNode(QStringList()<<modelName<<"Angle_Start",ui->ledtAngleStart->text());
    m_xml.addNode(QStringList()<<modelName<<"Optimization",ui->ledtOptimization->text());
    m_xml.addNode(QStringList()<<modelName<<"Contrast",ui->ledtComtrast->text());
    m_xml.addNode(QStringList()<<modelName<<"Min_Contrast",ui->ledtMinContrast->text());
    m_xml.addNode(QStringList()<<modelName<<"AngleStep",ui->ledtAngleStep->text());
    m_xml.addNode(QStringList()<<modelName<<"Min_Score",ui->ledtMinScore->text());
    m_xml.addNode(QStringList()<<modelName<<"NumToFind",ui->ledtNumToFind->text());
    m_xml.addNode(QStringList()<<modelName<<"Max_Overlap",ui->ledtMaxOverlap->text());
    m_xml.addNode(QStringList()<<modelName<<"Sub_Pixel",ui->ledtSubPixel->text());
    m_xml.addNode(QStringList()<<modelName<<"Num_Levels",ui->ledtNumLevels->text());
    m_xml.addNode(QStringList()<<modelName<<"Greediness",ui->ledtGreedness->text());
    m_xml.addNode(QStringList()<<modelName<<"Center_Col",ui->ledt_Center_Col->text());
    m_xml.addNode(QStringList()<<modelName<<"Center_Row",ui->ledt_Center_Row->text());
    m_xml.closeXml();
}

void ModelParSet::readModelPar(QString roi_name)
{
    XmlOperate m_xml;
    m_xml.openXml(m_type_path+"\\ROIConfig.xml");
    ui->ledtNumLevel->setText(m_xml.readNode(QStringList()<<roi_name<<"Num_Level"));
    ui->ledtAngleStart->setText(m_xml.readNode(QStringList()<<roi_name<<"Angle_Start"));
    ui->ledtOptimization->setText(m_xml.readNode(QStringList()<<roi_name<<"Optimization"));
    ui->ledtComtrast->setText(m_xml.readNode(QStringList()<<roi_name<<"Contrast"));
    ui->ledtMinContrast->setText(m_xml.readNode(QStringList()<<roi_name<<"Min_Contrast"));
    ui->ledtAngleStep->setText(m_xml.readNode(QStringList()<<roi_name<<"AngleStep"));
    ui->ledtMinScore->setText(m_xml.readNode(QStringList()<<roi_name<<"Min_Score"));
    ui->ledtNumToFind->setText(m_xml.readNode(QStringList()<<roi_name<<"NumToFind"));
    ui->ledtMaxOverlap->setText(m_xml.readNode(QStringList()<<roi_name<<"Max_Overlap"));
    ui->ledtSubPixel->setText(m_xml.readNode(QStringList()<<roi_name<<"Sub_Pixel"));
    ui->ledtNumLevels->setText(m_xml.readNode(QStringList()<<roi_name<<"Num_Levels"));
    ui->ledtGreedness->setText(m_xml.readNode(QStringList()<<roi_name<<"Greediness"));
    ui->ledt_Center_Col->setText(m_xml.readNode(QStringList()<<roi_name<<"Center_Col"));
    ui->ledt_Center_Row->setText(m_xml.readNode(QStringList()<<roi_name<<"Center_Row"));
    m_xml.closeXml();
}

void ModelParSet::on_pbtnCalImage_clicked()
{

    HTuple  hv_CameraParameters, hv_CameraPose, hv_TmpCtrl_PlateDescription;
    HTuple  hv_TmpCtrl_FindCalObjParNames, hv_TmpCtrl_FindCalObjParValues;
    HTuple  hv_CalibHandle, hv_TmpCtrl_MarkRows, hv_TmpCtrl_MarkColumns;
    HTuple  hv_TmpCtrl_Ind, hv_TmpCtrl_RectificationWidth, hv_TmpCtrl_RectificationPose;
    HTuple  hv_ToCalibrationPlane, hv_WorldWidth, hv_ImageToWorld_Ratio;
    HTuple  hv_Zoom_Ratio, hv_X_Offset, hv_Y_Offset, hv_A_Offset;
    HTuple  hv_Z_Offset, hv_ImageToWorld_Ratio_Out, hv_Zoom_Ratio_Out;
    HTuple  hv_WorldHeight;
    HObject ho_MapFixed;
    if(m_cam_info.map_step_cal[m_step].camPar==""||m_cam_info.map_step_cal[m_step].camPose=="")return;

    hv_CameraParameters = QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[m_step].camPar,',',"double");
    hv_CameraPose = QtHalcon::qstringToHtuple(m_cam_info.map_step_cal[m_step].camPose,',',"double");
    //*******计算map图像***************
    int width=m_cam_info.resolution.split(":")[0].toInt();
    int height=m_cam_info.resolution.split(":")[1].toInt();
    QtHalcon::calibrateImage(&ho_MapFixed,width,height,m_cam_info.map_step_cal[m_step].worldWidth.toDouble(),
                             m_cam_info.map_step_cal[m_step].worldHeight.toDouble(),hv_CameraParameters,hv_CameraPose,
                             m_cam_info.map_step_cal[m_step].ratio.toDouble(),m_cam_info.map_step_cal[m_step].X_Offset.toDouble(),
                             m_cam_info.map_step_cal[m_step].Y_Offset.toDouble());
    HalconCpp::MapImage(m_OriginImage,ho_MapFixed,&m_OriginImage);
    HTuple x_size,y_size;
    GetImageSize(m_OriginImage,&x_size,&y_size);

    if(hv_WindowHandle.Length()>0)
    {
        HalconCpp::CloseWindow(hv_WindowHandle);
    }
    //初始化窗口
    Hlong windID = Hlong(this->ui->label->winId());
    HalconCpp::OpenWindow(0,0,ui->label->width(),ui->label->height(),windID,"visible","",&hv_WindowHandle);
    SetColor(hv_WindowHandle,"green");
    SetDraw(hv_WindowHandle,"margin");
    SetLineWidth(hv_WindowHandle,3);

    CopyImage(m_OriginImage,&m_SceneImage_Draw);
    QtHalcon::displayHObject(m_SceneImage_Draw,hv_WindowHandle);

}


/*void ModelParSet::box_text_change(const QString &text)
{
    keyNow=text;
    int row=ui->tblwgtROI->currentRow();
    int col=ui->tblwgtROI->currentColumn();
    ui->tblwgtROI->removeCellWidget(row,col);
    ui->tblwgtROI->item(row,col)->setText(keyNow);
    disconnect(m_Combobox_cur,&QComboBox::currentTextChanged,this,&ModelParSet::box_text_change);
    ui->tblwgtROI->setCurrentCell(-1,-1);
    //delete m_Combobox_cur;
    //m_Combobox_cur=nullptr;
}

void ModelParSet::mouse_right_click_in_tblwgt()
{
    int row=ui->tblwgtROI->currentRow();
    int col=ui->tblwgtROI->currentColumn();
    if(col!=1)
    {
        ui->tblwgtROI->removeCellWidget(rowLast,1);
        delete  m_Combobox_cur;
        m_Combobox_cur=nullptr;
        return;
    }
    m_Combobox_cur=new QComboBox(this);
    keyLast=ui->tblwgtROI->item(row,col)->text();
    m_Combobox_cur->addItems(roi_type_list);
    m_Combobox_cur->setCurrentText(keyLast);
    ui->tblwgtROI->setCellWidget(row,col,m_Combobox_cur);
    connect(m_Combobox_cur,&QComboBox::currentTextChanged,this,&ModelParSet::box_text_change);
}*/
void ModelParSet::mouse_right_click_in_tblwgt()
{
    int row=ui->tblwgtROI->currentRow();
    int column=ui->tblwgtROI->currentColumn();
    if(m_Combobox_cur!=nullptr)
    {
        ui->tblwgtROI->removeCellWidget(rowLast,1);
        delete  m_Combobox_cur;
        m_Combobox_cur=nullptr;
    }
    if(column==1)
    {
        disconnect(textchange);
        rowLast=row;
        m_Combobox_cur=new QComboBox();
        keyLast=ui->tblwgtROI->item(row,1)->text();
        m_Combobox_cur->addItems(roi_type_list);
        m_Combobox_cur->setCurrentText(keyLast);
        ui->tblwgtROI->setCellWidget(row,column,m_Combobox_cur);
        textchange=connect(m_Combobox_cur,&QComboBox::currentTextChanged,this,[=](QString text){
            QTableWidgetItem *itemtext=new QTableWidgetItem(text);
            ui->tblwgtROI->setItem(row,column,itemtext);
        });
    }
}

void ModelParSet::on_pbtnSendSignal_clicked()
{
    emit signal_roi_change();
}
