#include "mainwindow.h"
#include "ui_mainwindow.h"
#include"communication/communication.h"
#include"communication/lightcontrol.h"
#include"parameterset.h"
#include"modelparset.h"
#include"reportview.h"
#include"general/generalfunc.h"
#include<QDateTime>
#include<QMessageBox>
#include"general/xmloperate.h"
#include"stationrunning.h"
#include"stationset.h"
#include"about.h"
#include<QProcess>
#include"typeset.h"
#include"general/configfileoperate.h"
#include<stdio.h>
#include<QLineEdit>
#include<QInputDialog>
#include"camera/camoperate.h"
#include"login.h"
#include<QThreadPool>
//#include<general/jsonoperate.h>


//#include<QtConcurrent/QtConcurrent>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    g_p_StationSet(nullptr),
    g_p_Communication(nullptr),
    g_p_CamOperate(nullptr),
    g_p_Par(nullptr),
    g_p_ModelParSet(nullptr),
    g_p_ReportView(nullptr),
    g_p_TypeSet(nullptr),
    g_p_about(nullptr),
    g_p_LightControl(nullptr),
    g_p_Login(nullptr),
    gLayout1(nullptr),
    m_isRunning(false),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qRegisterMetaType<QList<int>>("QList<int>");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<QModbusDevice::State>("QModbusDevice::State");
    qRegisterMetaType<QModbusDevice::Error>("QModbusDevice::Error");
    qRegisterMetaType<QList<cv::Rect>>("QList<cv::Rect>");


    //qRegisterMetaType<ComInfo>("ComInfo");
    setWindowState(Qt::WindowMaximized);
    //初始化应用路径、显示系统时间
    m_ExePath=qApp->applicationDirPath();
    lblPosInfo=new QLabel(this);
    ui->statusBar->addPermanentWidget(lblPosInfo);//添加显示位置信息标签到状态栏

    GeneralFunc::setClockToStatusBar(ui->statusBar,this);//添加时间标签到状态栏
    GeneralFunc::setLinkButtonToStatusBar(QString::fromLocal8Bit("百度一下，你就知道"),"www.baidu.com",ui->statusBar,this);
    connect(ui->actionOpenImage,&QAction::triggered,this,&MainWindow::on_pbtnReadImage_clicked);
    connect(ui->actionExit,&QAction::triggered,this,&MainWindow::close);
    connect(ui->actionEditPro,&QAction::triggered,this,&MainWindow::on_pbtnStationSet_clicked);
    connect(ui->actionEditCam,&QAction::triggered,this,&MainWindow::on_pbtnCamSet_clicked);
    connect(ui->actionEditCom,&QAction::triggered,this,&MainWindow::on_pbtnComSet_clicked);
    connect(ui->actionEditROI,&QAction::triggered,this,&MainWindow::on_pbtnEditROI_clicked);
    connect(ui->actionEditPar,&QAction::triggered,this,&MainWindow::on_pbtnEditCheckPar_clicked);
    connect(ui->actionReport,&QAction::triggered,this,&MainWindow::on_pbtnReportView_clicked);
    connect(ui->actionLightSet,&QAction::triggered,this,&MainWindow::on_pbtnLightSet_clicked);
    connect(ui->actionEditType,&QAction::triggered,this,&MainWindow::on_pbtnTypeSet_clicked);
    connect(ui->actionBlackOrange,&QAction::triggered,this,&MainWindow::on_actionSkinChange_triggered);
    connect(ui->actionWhite,&QAction::triggered,this,&MainWindow::on_actionSkinChange_triggered);
    connect(ui->actionMacOS,&QAction::triggered,this,&MainWindow::on_actionSkinChange_triggered);
    connect(ui->actionUbuntu,&QAction::triggered,this,&MainWindow::on_actionSkinChange_triggered);
    readProConfig();//读取工程信息

    //changeSkin(":/image/image/default.qss");
    //changeSkin(":/image/image/blackorange.qss");
    //changeControlEnable(false);
    readDataCount();
    //startWork();
    //initMes();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startWork()
{
    ui->chkboxInitCam->setCheckState(Qt::Checked);
    on_chkboxInitCam_clicked();
    on_pbtnRunning_clicked();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //关闭前先关闭线程
    emit signal_start_running(false);
    if(ui->chkboxInitCam->checkState()==Qt::Checked)
    {
        ui->chkboxInitCam->setCheckState(Qt::Unchecked);
        emit signal_init_cam(false);
    }
    emit signal_closing();
    QThread::msleep(200);

    QMap<QString,QThread*>::iterator iter=map_Thread.begin();
    while(iter!=map_Thread.end())
    {
        //map_Thread[iter.key()]->terminate();
        //map_Thread[iter.key()]->exit(0);
        map_Thread[iter.key()]->quit();
        map_Thread[iter.key()]->wait();
        //map_Thread[iter.key()]->requestInterruption();
        //map_Thread[iter.key()]->wait();
        delete map_Thread[iter.key()];
        map_Thread[iter.key()]=nullptr;
        iter++;
    }
    if(g_p_StationSet!=nullptr)
    {
        delete g_p_StationSet;
        g_p_StationSet=nullptr;
    }
    if(g_p_Communication!=nullptr)
    {
        delete g_p_Communication;
        g_p_Communication=nullptr;
    }
    if(g_p_CamOperate!=nullptr)
    {
        delete g_p_CamOperate;
        g_p_CamOperate=nullptr;
    }
    if(g_p_Par!=nullptr)
    {
        delete g_p_Par;
        g_p_Par=nullptr;
    }
    if(g_p_ModelParSet!=nullptr)
    {
        delete g_p_ModelParSet;
        g_p_ModelParSet=nullptr;
    }
    if(g_p_ReportView!=nullptr)
    {
        delete g_p_ReportView;
        g_p_ReportView=nullptr;
    }
    if(g_p_TypeSet!=nullptr)
    {
        delete g_p_TypeSet;
        g_p_TypeSet=nullptr;
    }
    if(g_p_about!=nullptr)
    {
        delete g_p_about;
        g_p_about=nullptr;
    }
    if(g_p_LightControl!=nullptr)
    {
        delete g_p_LightControl;
        g_p_LightControl=nullptr;
    }
    if(g_p_Login!=nullptr)
    {
        delete g_p_Login;
        g_p_Login=nullptr;
    }
    event->accept();
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMap<QString,QLabel*>::iterator iter=map_show_label.begin();
    while(iter!=map_show_label.end())
    {
        QString camname=iter.key();
        if(map_show_label[camname]!=nullptr)
        {
            int label_length=min(map_show_widget[camname]->width(),map_show_widget[camname]->height())/12;
            //调整显示结果label
            QSize label_size=QSize(label_length,label_length);
            map_show_label[camname]->resize(label_size);
            QPoint label_point=QPoint(ui->centralWidget->pos().x()+ui->widget->x()+map_show_widget[camname]->x()+map_show_widget[camname]->width()-label_size.width(),
                                      ui->centralWidget->pos().y()+ui->widget->y()+map_show_widget[camname]->y()+map_show_widget[camname]->height()-label_size.height());
            map_show_label[camname]->move(label_point);
            map_show_label[camname]->show();

            QSize count_label_size=QSize(label_length,label_length/2);
            int font_size=label_length/2;

            //调整ok计数label
            map_ok_label[camname]->resize(count_label_size);
            map_ok_label[camname]->setStyleSheet(QString("QLabel{background-color:rgb(50,200,50);color: rgb(0, 0, 0);font-size: %1px;}").arg(QString::number(font_size)));
            QPoint ok_point=QPoint(label_point.x(),label_point.y()-count_label_size.height()*3);
            map_ok_label[camname]->move(ok_point);
            map_ok_label[camname]->show();

            //调整ng计数label
            map_ng_label[camname]->resize(count_label_size);
            map_ng_label[camname]->setStyleSheet(QString("QLabel{background-color:rgb(200,50,50);color: rgb(0, 0, 0);font-size: %1px;}").arg(QString::number(font_size)));
            QPoint ng_point=QPoint(label_point.x(),label_point.y()-count_label_size.height()*2);
            map_ng_label[camname]->move(ng_point);
            map_ng_label[camname]->show();

            //调整ng计数label
            map_qu_label[camname]->resize(count_label_size);
            map_qu_label[camname]->setStyleSheet(QString("QLabel{background-color:rgb(200,200,50);color: rgb(0, 0, 0);font-size: %1px;}").arg(QString::number(font_size)));
            QPoint qu_point=QPoint(label_point.x(),label_point.y()-count_label_size.height());
            map_qu_label[camname]->move(qu_point);
            map_qu_label[camname]->show();
        }
        ++iter;
    }
}
//启动加载进度条
void MainWindow::show_progress_bar(int precent)
{
    //为0，显示加载进度条篇，0-100更新进度条，100关闭进度条
    if(precent==0)
    {
        bar=new RingsProgressbar();
        bar->resize(300,300);
        bar->setWindowFlags(Qt::FramelessWindowHint);//无边框
        bar->setAttribute(Qt::WA_TranslucentBackground);//背景透明
        bar->show();
        bar->setPersent(0);
    }
    else if(precent==100)
    {
        bar->setPersent(precent);
        bar->close();
        delete bar;
        bar=nullptr;
    }
    else
    {
        bar->setPersent(precent);
    }
    qApp->processEvents();
}
void MainWindow::readDataCount()
{
    QDateTime time=QDateTime::currentDateTime();
    QString timestr=time.toString("yyyyMMdd");
    ConfigFileOperate countconfig(m_ExePath+"\\DataCount\\"+timestr+".ini");
    QStringList camlist=countconfig.childGroup();
    for (QString cam : camlist)
    {
        countconfig.setSection(cam);
        QStringList countlist=countconfig.childKeys();
        for (QString  count: countlist)
        {
            cam_re_count[cam][count]=countconfig.readKeyValue(count).toInt();
        }
    }
}


//读取项目文件夹
void MainWindow::readProConfig()
{
    ConfigFileOperate mconfig(m_ExePath+"\\Config.ini");
    QString qssfile=QString(":/image/image/%1.qss").arg(mconfig.readKeyValue("QssStyle"));
    changeSkin(qssfile);

    m_AllProPath=m_ExePath+"\\Pro";

    ConfigFileOperate tconfig(m_AllProPath+"\\Pro.ini");//初始化ini类
    m_usePro=tconfig.readKeyValue(QString("UsePro"));//读取当前启用项目

    m_ProList=GeneralFunc::GetAllFolderName(m_AllProPath);//得到项目列表
    m_ProPath=m_AllProPath+"\\"+m_usePro;//项目文件夹
    //更新项目下拉框
    GeneralFunc::addItemsToCombobox(ui->cbboxProList,m_ProList,m_usePro);
}
/*void MainWindow::initLightSet()
{
    LightControl* pLightControl=new LightControl(this);
    ConfigFileOperate config(m_ProPath+"\\LightConfig.ini");
    pLightControl->openCom(config.readKeyValue("LightCom"));
    QString command="3";
    for(int i=1;i<5;i++)
    {
        QString channel=QString::number(i);
        QString value=config.readKeyValue("CH"+channel);
        pLightControl->setCommand(command,channel,value);
        QThread::msleep(20);
    }
}*/

void MainWindow::initShowWindow()
{
    //清除布局
    if(gLayout1!=nullptr)
    {
        GeneralFunc::clearLayout(gLayout1,true);
        delete gLayout1;
    }
    //清除类
    QMap<QString,QtDrawingPaperEditor*>::iterator iter=map_show_widget.begin();
    while(iter!=map_show_widget.end())
    {
        delete map_show_widget[iter.key()];
        map_show_widget[iter.key()]=nullptr;
        delete map_show_label[iter.key()];
        map_show_label[iter.key()]=nullptr;
        delete map_ok_label[iter.key()];
        map_ok_label[iter.key()]=nullptr;
        delete map_ng_label[iter.key()];
        map_ng_label[iter.key()]=nullptr;
        delete map_qu_label[iter.key()];
        map_qu_label[iter.key()]=nullptr;
        ++iter;
    }
    map_show_widget.clear();
    map_show_label.clear();
    map_ok_label.clear();
    map_ng_label.clear();
    map_qu_label.clear();
    //开始初始化
    QStringList cam_all=m_camList;
    gLayout1=new QGridLayout();
    int colcount=2+cam_all.size()/6;
    int rowcount=std::ceil(cam_all.size()/double(colcount));
    for (int i=0;i<rowcount;i++)
    {
        for(int j=0;j<colcount;j++)
        {
            int s=i*colcount+j;
            if(s>=cam_all.size())break;
            map_show_widget[cam_all[s]]=new QtDrawingPaperEditor(ui->widget);
            map_show_widget[cam_all[s]]->camname=cam_all[s];
            connect(map_show_widget[cam_all[s]],&QtDrawingPaperEditor::signal_PressPoint,this,&MainWindow::slot_press_point);//连接控件点击事件
            gLayout1->setSpacing(3);
            gLayout1->setMargin(0);
            gLayout1->addWidget(map_show_widget[cam_all[s]],i,j);

            map_show_label[cam_all[s]]=new QLabel(this);
            map_show_label[cam_all[s]]->hide();

            map_ok_label[cam_all[s]]=new QLabel(this);
            map_ok_label[cam_all[s]]->hide();

            map_ng_label[cam_all[s]]=new QLabel(this);
            map_ng_label[cam_all[s]]->hide();

            map_qu_label[cam_all[s]]=new QLabel(this);
            map_qu_label[cam_all[s]]->hide();
        }
    }
    ui->widget->setLayout(gLayout1);
    //setWindowState(Qt::WindowMaximized);
}
//初始化工位
void MainWindow::initStation()
{

    show_progress_bar(0);//初始化加载进度条
    QMap<QString,QThread*>::iterator iter=map_Thread.begin();
    while(iter!=map_Thread.end())
    {
        map_Thread[iter.key()]->exit(0);
        map_Thread[iter.key()]->wait();
        //map_Thread[iter.key()]->terminate();
        //map_Thread[iter.key()]->requestInterruption();
        delete map_Thread[iter.key()];
        map_Thread[iter.key()]=nullptr;

        delete map_lblConnectInfo[iter.key()];
        map_lblConnectInfo[iter.key()]=nullptr;

        iter++;
    }

    map_Thread.clear();
    map_stations.clear();
    show_progress_bar(50);//初始化加载进度条
    //初始化工位线程和工位信息，链接信号槽
    for (int i=0;i<m_StationList.size();i++ )
    {
        map_stations[m_StationList[i]]=new StationRunning();
        map_stations[m_StationList[i]]->initStationCom(m_usePro,m_StationList[i]);
        if(!map_stations[m_StationList[i]]->is_station_exist)//判断工位是否配置，是否存在
        {
            delete map_stations[m_StationList[i]];
            map_stations[m_StationList[i]]=nullptr;
            map_stations.remove(m_StationList[i]);
            ui->cbboxStationList->removeItem(ui->cbboxStationList->findText(m_StationList[i]));
            m_StationList.removeAt(i);
            i--;
            continue;
        }
        //添加状态显示标签
        map_lblConnectInfo[m_StationList[i]]=new QLabel(this);
        map_lblConnectInfo[m_StationList[i]]->setStyleSheet("QLabel{background-color:rgb(255,0,0);color: rgb(0, 0, 0);}");
        ui->statusBar->addPermanentWidget(map_lblConnectInfo[m_StationList[i]]);//添加显示连接标签到状态栏
        map_lblConnectInfo[m_StationList[i]]->setText(tr(m_StationList[i].toUtf8()));

        //初始化
        map_stations[m_StationList[i]]->initStationType(m_useType);
        map_Thread[m_StationList[i]]=new QThread();
        map_stations[m_StationList[i]]->moveToThread(map_Thread[m_StationList[i]]);
        connect(map_Thread[m_StationList[i]],&QThread::finished,map_Thread[m_StationList[i]],&QObject::deleteLater);
        connect(map_Thread[m_StationList[i]],&QThread::finished,map_stations[m_StationList[i]],&QObject::deleteLater);
        connect(this,&MainWindow::signal_cam_change,map_stations[m_StationList[i]],&StationRunning::slot_cam_change);
        connect(this,&MainWindow::signal_com_change,map_stations[m_StationList[i]],&StationRunning::slot_com_change);
        connect(this,&MainWindow::signal_roi_change,map_stations[m_StationList[i]],&StationRunning::slot_roi_change);
        connect(this,&MainWindow::signal_type_change,map_stations[m_StationList[i]],&StationRunning::slot_type_change);
        connect(this,&MainWindow::signal_par_change,map_stations[m_StationList[i]],&StationRunning::slot_par_change);
        connect(this,&MainWindow::signal_init_cam,map_stations[m_StationList[i]],&StationRunning::slot_init_cam);
        connect(this,&MainWindow::signal_start_running,map_stations[m_StationList[i]],&StationRunning::slot_start_running);
        connect(this,&MainWindow::signal_photo,map_stations[m_StationList[i]],&StationRunning::slot_photo);
        connect(this,&MainWindow::signal_read_image,map_stations[m_StationList[i]],&StationRunning::slot_read_image);
        connect(this,&MainWindow::signal_check,map_stations[m_StationList[i]],&StationRunning::slot_check);
        connect(this,&MainWindow::signal_read_plc_photo,map_stations[m_StationList[i]],&StationRunning::slot_read_plc_photo);
        connect(this,&MainWindow::signal_force_set,map_stations[m_StationList[i]],&StationRunning::slot_force_set);
        connect(this,&MainWindow::signal_press_point,map_stations[m_StationList[i]],&StationRunning::slot_press_point);
        connect(this,&MainWindow::signal_closing,map_stations[m_StationList[i]],&StationRunning::slot_program_exit);
        connect(map_stations[m_StationList[i]],&StationRunning::signal_init_cam_result,this,&MainWindow::slot_init_cam_result);
        connect(map_stations[m_StationList[i]],&StationRunning::signal_information_text,this,&MainWindow::slot_information_text);
        connect(map_stations[m_StationList[i]],&StationRunning::signal_information_image,this,&MainWindow::slot_information_image);
        //线程启动
        map_Thread[m_StationList[i]]->start();
    }
    //关闭进度条
    show_progress_bar(100);
}

//读取品种信息
void MainWindow::readStationConfig()
{   
    m_StationList.clear();
    m_camList.clear();
    m_TypeList.clear();
    m_StationList=GeneralFunc::GetAllFolderName(m_AllStationPath);
    m_TypeList.clear();
    for(int i=0;i<m_StationList.size();i++)
    {
        QString station_folder=m_AllStationPath+"\\"+m_StationList[i];
        QStringList cam_name_list=GeneralFunc::GetAllFolderName(station_folder);
        for (int j=0;j<cam_name_list.size();j++)
        {
            if(!m_camList.contains(cam_name_list[j]))
            {
                m_camList<<cam_name_list[j];
            }
            QString cam_folder=station_folder+"\\"+cam_name_list[j];
            QStringList step_name_list=GeneralFunc::GetAllFolderName(cam_folder);
            for (int k=0;k<step_name_list.size();k++)
            {
                QString step_folder=cam_folder+"\\"+step_name_list[k];
                QStringList type_name_list=GeneralFunc::GetAllFolderName(step_folder);
                for (int b=0;b<type_name_list.size();b++)
                {
                    if(!m_TypeList.contains(type_name_list[b]))
                    {
                        m_TypeList<<type_name_list[b];
                    }
                }
            }
        }
    }
    //初始化工位列表
    ui->cbboxStationList->clear();
    ui->cbboxStationList->addItems(m_StationList);
    //初始化相机列表
    ui->cbboxStationCamList->clear();
    ui->cbboxStationCamList->addItems(m_camList);
    //初始化品种列表
    GeneralFunc::addItemsToCombobox(ui->cbboxTypeList,m_TypeList,m_useType);

}

//更改选用的项目工程
void MainWindow::on_cbboxProList_currentTextChanged(const QString &arg1)
{
    if(arg1=="")return;
    m_usePro=arg1;
    //将启用工程写入的配置文件
    ConfigFileOperate tconfig(m_AllProPath+"\\Pro.ini");
    tconfig.setKeyValue(QString("UsePro"),m_usePro);

    m_ProPath=m_AllProPath+"\\"+m_usePro;
    m_AllStationPath=m_ProPath+"\\station";

    //读启用的品种
    ConfigFileOperate rconfig=ConfigFileOperate(m_AllStationPath+"\\TypeConfig.ini");
    m_useType=rconfig.readKeyValue(QString("UseType"));

    //读取工位配置
    readStationConfig();

    //初始化工位
    initStation();

    initShowWindow();
    this->resize(this->size() - QSize(1, 1));
}

//更改工位的信息
void MainWindow::on_cbboxStationList_currentTextChanged(const QString &arg1)
{
    m_station_name=arg1;
    m_edit_type_path=m_AllStationPath+"\\"+m_station_name+"\\"+m_cam_name+"\\"+m_cam_step+"\\"+m_useType;
    /*QStringList cam_name_list=GeneralFunc::GetAllFolderName(m_ProPath+"\\"+m_station_name);
    ui->cbboxStationCamList->clear();
    ui->cbboxStationCamList->addItems(cam_name_list);*/
}

//更改启用的品种
void MainWindow::on_cbboxTypeList_currentTextChanged(const QString &arg1)
{
    //更改启用品种
    if(arg1=="")return;
    //读取品种的配置文件
    if(m_TypeList.contains(arg1))
    {
        m_useType=arg1;
    }
    ConfigFileOperate rconfig=ConfigFileOperate(m_AllStationPath+"\\TypeConfig.ini");
    rconfig.setKeyValue(QString("UseType"),m_useType);
    m_edit_type_path=m_AllStationPath+"\\"+m_station_name+"\\"+m_cam_name+"\\"+m_cam_step+"\\"+m_useType;
    emit signal_type_change(m_useType);
}

//更改相机
void MainWindow::on_cbboxStationCamList_currentTextChanged(const QString &arg1)
{
    m_cam_name=arg1;
    m_edit_type_path=m_AllStationPath+"\\"+m_station_name+"\\"+m_cam_name+"\\"+m_cam_step+"\\"+m_useType;

    QStringList stepList=GeneralFunc::GetAllFolderName(m_AllStationPath+"\\"+m_station_name+"\\"+m_cam_name);
    ui->cbboxCamStep->clear();
    ui->cbboxCamStep->addItems(stepList);
}

void MainWindow::on_cbboxCamStep_currentTextChanged(const QString &arg1)
{
    m_cam_step=arg1;
    m_edit_type_path=m_AllStationPath+"\\"+m_station_name+"\\"+m_cam_name+"\\"+m_cam_step+"\\"+m_useType;

}

//工位设置
void MainWindow::on_pbtnStationSet_clicked()
{
    if(g_p_StationSet!=nullptr)
    {
        disconnect(g_p_StationSet,&StationSet::signal_station_change,this,&MainWindow::readProConfig);
        delete g_p_StationSet;
        g_p_StationSet=nullptr;
    }
    g_p_StationSet=new StationSet();
    g_p_StationSet->initPar();
    g_p_StationSet->show();
    connect(g_p_StationSet,&StationSet::signal_station_change,this,&MainWindow::readProConfig);

}
//通信设置
void MainWindow::on_pbtnComSet_clicked()
{
    if(g_p_Communication!=nullptr)
    {
        disconnect(g_p_Communication,&Communication::signal_com_change,this,&MainWindow::slot_com_change);
        delete g_p_Communication;
        g_p_Communication=nullptr;
    }
    g_p_Communication=new Communication();
    g_p_Communication->initPar(m_AllStationPath);
    g_p_Communication->show();
    connect(g_p_Communication,&Communication::signal_com_change,this,&MainWindow::slot_com_change);
}

//相机配置
void MainWindow::on_pbtnCamSet_clicked()
{
    if(ui->chkboxInitCam->checkState()==Qt::Checked)
    {
        QMessageBox::warning(this,"warning",
                             tr(QString::fromLocal8Bit("请关闭相机后进行相机设置").toUtf8()));
        return;
    }

    if(g_p_CamOperate!=nullptr)
    {
        disconnect(g_p_CamOperate->cameraPar,&CameraPar::signal_cam_change,this,&MainWindow::slot_cam_change);
        delete g_p_CamOperate;
        g_p_CamOperate=nullptr;
    }
    g_p_CamOperate=new CamOperate();
    g_p_CamOperate->initPar(m_ProPath);
    g_p_CamOperate->show();
    connect(g_p_CamOperate->cameraPar,&CameraPar::signal_cam_change,this,&MainWindow::slot_cam_change);
}


void MainWindow::on_pbtnTypeSet_clicked()
{
    if(g_p_TypeSet!=nullptr)
    {
        disconnect(g_p_TypeSet,&TypeSet::signal_type_change,this,&MainWindow::slot_type_change);
        delete g_p_TypeSet;
        g_p_TypeSet=nullptr;
    }
    g_p_TypeSet=new TypeSet();
    g_p_TypeSet->initPar(m_ProPath);
    g_p_TypeSet->show();
    connect(g_p_TypeSet,&TypeSet::signal_type_change,this,&MainWindow::slot_type_change);
}


//检测参数配置
void MainWindow::on_pbtnEditCheckPar_clicked()
{
    if(g_p_Par!=nullptr)
    {
        disconnect(g_p_Par,&ParameterSet::signal_par_change,this,&MainWindow::slot_par_change);
        delete g_p_Par;
        g_p_Par=nullptr;
    }
    g_p_Par=new ParameterSet();
    g_p_Par->initPar(m_edit_type_path);
    g_p_Par->show();
    connect(g_p_Par,&ParameterSet::signal_par_change,this,&MainWindow::slot_par_change);
}

//ROI设置
void MainWindow::on_pbtnEditROI_clicked()
{
    if(g_p_ModelParSet!=nullptr)
    {
        disconnect(g_p_ModelParSet,&ModelParSet::signal_roi_change,this,&MainWindow::slot_roi_change);
        delete g_p_ModelParSet;
        g_p_ModelParSet=nullptr;
    }
    g_p_ModelParSet=new ModelParSet();
    g_p_ModelParSet->initPar(m_usePro,m_station_name,m_cam_name,m_cam_step,m_useType);
    g_p_ModelParSet->show();
    connect(g_p_ModelParSet,&ModelParSet::signal_roi_change,this,&MainWindow::slot_roi_change);
}

//拍照信号
void MainWindow::on_pbtnTakePhoto_clicked()
{
    emit signal_photo(m_station_name,m_cam_name,m_cam_step);
}
//检测信号
void MainWindow::on_pbtnCheck_clicked()
{
    emit signal_check(m_station_name,m_cam_name,m_cam_step);
}
//初始化相机信号
void MainWindow::on_chkboxInitCam_clicked()
{
    ui->chkboxInitCam->setEnabled(false);
    if(ui->chkboxInitCam->checkState()==Qt::Checked)
    {
        emit signal_init_cam(true);
    }
    else if(ui->chkboxInitCam->checkState()==Qt::Unchecked)
    {
        emit signal_init_cam(false);
    }
}
//初始化相机返回结果
void MainWindow::slot_init_cam_result(bool isSucceed)
{
    if(!isSucceed)
    {
        ui->chkboxInitCam->setCheckState(Qt::Unchecked);
    }
    else
    {
    }
    ui->chkboxInitCam->setEnabled(true);
}

//开始运行
void MainWindow::on_pbtnRunning_clicked()
{ 
    m_isRunning=!m_isRunning;
    emit signal_start_running(m_isRunning);
    if(m_isRunning)
    {
        ui->pbtnRunning->setText(tr(QString::fromLocal8Bit("停止运行").toUtf8()));
    }
    else
    {
        ui->pbtnRunning->setText(tr(QString::fromLocal8Bit("开始运行").toUtf8()));
    }
}

//1:列表框内的信息，2：坐标的像素信息，3：跳出警告对话框信息，4：通信正常 5：通信异常
void MainWindow::slot_information_text(QString name,int type,QString information)
{
    switch (type)
    {
    case 1:
        if(information=="")
        {
            showReasult(name,0);
            ui->lstwgtRunInfo->clear();
            return;
        }
        else
        {
            ui->lstwgtRunInfo->addItem(information);
            ui->lstwgtRunInfo->scrollToBottom();
        }
        break;
    case 2:
        lblPosInfo->setText(information);
        break;
    case 3:
        QMessageBox::warning(this,"waring",information);
        break;
    case 4:
        if(map_lblConnectInfo[name]!=nullptr)
            map_lblConnectInfo[name]->setStyleSheet("QLabel{background-color:rgb(0,255,0);color: rgb(0, 0, 0);}");
        break;
    case 5:
        if(map_lblConnectInfo[name]!=nullptr)
            map_lblConnectInfo[name]->setStyleSheet("QLabel{background-color:rgb(255,0,0);color: rgb(0, 0, 0);}");
        break;
    }
}

//显示检测图像
//1:ok图像，2：NG图像，3：运行图像，4：疑问图像，5：原始图像
void MainWindow::slot_information_image(QString camname,int type,QImage image)//显示检测结果,1:显示结果+显示图像，2：显示图像
{
    m_ImageShow=image;
    map_show_widget[camname]->initImage(m_ImageShow);
    showReasult(camname, type);
}
//显示运行结果
void MainWindow::showReasult(QString camname,int showtype)//1:ok图像，2：NG图像，3：运行图像，：疑问图像
{
    if(!map_show_label.contains(camname))return;
    QDateTime time=QDateTime::currentDateTime();
    QString datastr=time.toString("yyyyMMdd");
    QString timestr=time.toString("hhmm");
    if(timestr=="0000"&&cam_re_count[camname]["OK_Num"]>100)
    {
        cam_re_count[camname]["OK_Num"]=0;
        cam_re_count[camname]["NG_Num"]=0;
        cam_re_count[camname]["QU_Num"]=0;
    }
    ConfigFileOperate countconfig(m_ExePath+"\\DataCount\\"+datastr+".ini");
    GeneralFunc::isDirExist(m_ExePath+"\\DataCount",true);
    switch(showtype)
    {
    case 0:
        GeneralFunc::showImage(map_show_label[camname],QPixmap(":/image/image/RN.png"));
        break;
    case 1:
        GeneralFunc::showImage(map_show_label[camname],QPixmap(":/image/image/OK.png"));
        cam_re_count[camname]["OK_Num"]++;
        map_ok_label[camname]->setText(QString::number(cam_re_count[camname]["OK_Num"]));
        map_ok_label[camname]->show();
        countconfig.writeSection(camname,"OK_Num",QString::number(cam_re_count[camname]["OK_Num"]));
        break;
    case 2:
        if(m_usePro=="GlueWithSeat"||m_usePro=="DistanceOnly"||m_usePro=="DistanceAndGlue")
        {
            if(ui->pbtnRunning->text()==QString::fromLocal8Bit("停止运行"))
                on_pbtnRunning_clicked();
        }
        GeneralFunc::showImage(map_show_label[camname],QPixmap(":/image/image/NG.png"));
        cam_re_count[camname]["NG_Num"]++;
        map_ng_label[camname]->setText(QString::number(cam_re_count[camname]["NG_Num"]));
        map_ng_label[camname]->show();
        countconfig.writeSection(camname,"NG_Num",QString::number(cam_re_count[camname]["NG_Num"]));
        break;

    default:
        GeneralFunc::showImage(map_show_label[camname],QPixmap(":/image/image/QU.png"));
        cam_re_count[camname]["QU_Num"]++;
        map_qu_label[camname]->setText(QString::number(cam_re_count[camname]["QU_Num"]));
        map_qu_label[camname]->show();
        countconfig.writeSection(camname,"QU_Num",QString::number(cam_re_count[camname]["QU_Num"]));
        break;
    }
}

//通信更改
void MainWindow::slot_com_change(QString com)
{
    QMessageBox::information(this,
                             "Info",
                             QString::fromLocal8Bit("通信修改,重启软件"));
    //emit signal_com_change(com);
}
//相机设置更改
void MainWindow::slot_cam_change(QString cam)
{
    emit signal_cam_change(cam);
}
//参数修改更改
void MainWindow::slot_par_change()
{
    emit signal_par_change(m_station_name,m_cam_name,m_cam_step);
}
//检测选区更改
void MainWindow::slot_roi_change()
{
    emit signal_roi_change(m_station_name,m_cam_name,m_cam_step);
}

void MainWindow::slot_type_change(QString type_name,bool isNew)
{
    if(isNew)
    {
        m_TypeList<<type_name;
        ui->cbboxTypeList->addItem(type_name);
        ui->cbboxTypeList->setCurrentText(type_name);
    }
    else
    {
        m_TypeList.removeOne(type_name);
        ui->cbboxTypeList->removeItem(ui->cbboxTypeList->findText(type_name));
    }
}
//屏幕点击事件
void MainWindow::slot_press_point(QString camname,QPoint point)
{
    emit signal_press_point(camname,point,map_show_widget[camname]->size());
}

//读取文件，获取文件的路径
void MainWindow::on_pbtnReadImage_clicked()
{
    //获得最后打开的路径
    ConfigFileOperate tconfig(m_edit_type_path+"\\Config.ini");
    QString folder=tconfig.readSection(QString("Path"),QString("FilePath"));
    //打开文件对话框
    QString filepath=GeneralFunc::getFilePath(folder,this);
    //写入刚才打开的路径
    tconfig.writeSection(QString("Path"),QString("FilePath"),filepath);
    if(filepath=="")
    {
        return;
    }
    emit signal_read_image(m_station_name,m_cam_name,m_cam_step,filepath);
}

void MainWindow::on_pbtnReadImageFolder_clicked()
{
#if 0
    QMessageBox::information(this,
                             "Info",
                             QString::fromLocal8Bit("功能正在添加中"));
#else
    //获得最后打开的路径
    ConfigFileOperate tconfig(m_edit_type_path+"\\Config.ini");
    QString folder=tconfig.readSection(QString("Path"),QString("FolderPath"));
    //打开文件对话框
    QString filepath=GeneralFunc::getFolderPath(folder,this);
    //写入刚才打开的路径
    tconfig.writeSection(QString("Path"),QString("FolderPath"),filepath);
    if(filepath=="")
    {
        return;
    }
    emit signal_read_image(m_station_name,m_cam_name,m_cam_step,filepath);
#endif
}

//强制OK
void MainWindow::on_pbtnForceOK_clicked()
{
    emit signal_force_set(m_station_name,m_cam_name,m_cam_step,true);
}
//强制NG
void MainWindow::on_pbtnForceNG_clicked()
{
    emit signal_force_set(m_station_name,m_cam_name,m_cam_step,false);
}

//记录查看
void MainWindow::on_pbtnReportView_clicked()
{
    if(g_p_ReportView!=nullptr)
    {
        delete g_p_ReportView;
        g_p_ReportView=nullptr;
    }
    g_p_ReportView=new ReportView();
    g_p_ReportView->show();
}

//读取PLC
void MainWindow::on_pbtnReadOnceTakePhoto_clicked()
{
    emit signal_read_plc_photo(m_station_name,m_cam_name,m_cam_step);
}
void MainWindow::on_actionChinese_triggered()
{
    QMessageBox::information(this,"info","no fun");
}
void MainWindow::on_actionEnglish_triggered()
{
    QMessageBox::information(this,"info","no fun");
}
void MainWindow::on_actionSkinChange_triggered()
{
    QAction *btn=qobject_cast<QAction*>(sender());
    QString qssfile=QString(":/image/image/%1.qss").arg(btn->text());
    ConfigFileOperate tconfig(m_ExePath+"\\Config.ini");
    tconfig.setKeyValue("QssStyle",btn->text());
    changeSkin(qssfile);
}

void MainWindow::on_actionHandbook_triggered()
{
    QString path=QString("explorer %1").arg("D:\\OperatingManual.pdf");
    QProcess::execute(path);
}
void MainWindow::on_actionAbout_triggered()
{
    if(g_p_about!=nullptr)
    {
        delete g_p_about;
        g_p_about=nullptr;
    }
    g_p_about=new About(this);
    g_p_about->show();
}
void changeLanguage(QString str_language)
{

}
//更改皮肤
void MainWindow::changeSkin(QString str_skin)
{
    QFile qss(str_skin);
    qss.open(QFile::ReadOnly);
    qApp->setStyleSheet(qss.readAll());
    qss.close();
}

void MainWindow::on_actionLogin_triggered()
{
    /*if(login_state)
    {
        login_state=false;
        changeControlEnable(login_state);
        ui->actionLogin->setText(QString::fromLocal8Bit("登录"));
        return;
    }

    ConfigFileOperate tconfig(m_ExePath+"\\Config.ini");
    QString password=tconfig.readKeyValue(QString("Password"));
    //输入字符串
    QString dlgTitle="login";
    QString txtLabel="password";
    QString defaultInput="";
    QLineEdit::EchoMode echoMode=QLineEdit::Normal;//正常文字输入
    //QLineEdit::EchoMode echoMode=QLineEdit::Password;//密码输入
    bool ok=false;
    QString text = QInputDialog::getText(this, dlgTitle,txtLabel, echoMode,defaultInput, &ok);
    if (ok && !text.isEmpty())
    {
        if(text==password)
        {
            login_state=true;
            changeControlEnable(login_state);
            ui->actionLogin->setText(QString::fromLocal8Bit("退出登录"));
        }
        else if (text.indexOf("#")!=-1)
        {
            QStringList list=text.split('#');
            if(password!=list[0])
            {
                QMessageBox::warning(this,QString::fromLocal8Bit("警告"),
                                     QString::fromLocal8Bit("输入原密码错误"));
                return;
            }
            password=list[1];
            tconfig.setKeyValue(QString("Password"),password);
        }
        else
        {
            QMessageBox::warning(this,QString::fromLocal8Bit("warning"),QString::fromLocal8Bit("密码错误"));
            return;
        }

    }*/

    userLogin();
}
//登录操作
void MainWindow::userLogin()
{
    if(login_state)
    {
        login_state=false;
        changeControlEnable(login_state);
        ui->actionLogin->setText(QString::fromLocal8Bit("登录"));
        return;
    }
    if(g_p_Login!=nullptr)
    {
        delete g_p_Login;
        g_p_Login=nullptr;
    }
    g_p_Login=new Login();
    g_p_Login->show();
    QMetaObject::Connection dis;
    dis = connect(g_p_Login,&Login::signal_loginSuccess,[=](){
        login_state=true;
        changeControlEnable(login_state);
        ui->actionLogin->setText(QString::fromLocal8Bit("退出登录"));
        disconnect(dis);
    }
    );
}
//根据登录状态更改控件状态
void MainWindow::changeControlEnable(bool isEnabe)
{
    ui->pbtnStationSet->setEnabled(isEnabe);
    ui->pbtnCamSet->setEnabled(isEnabe);
    ui->pbtnComSet->setEnabled(isEnabe);
    ui->pbtnEditROI->setEnabled(isEnabe);
    ui->pbtnEditCheckPar->setEnabled(isEnabe);
    ui->pbtnLightSet->setEnabled(isEnabe);

}
void MainWindow::on_pbtnLightSet_clicked()
{
    /*if(g_p_LightControl!=nullptr)
    {
        delete g_p_LightControl;
        g_p_LightControl=nullptr;
    }
    g_p_LightControl=new LightControl();
    g_p_LightControl->initPar(m_ProPath);
    g_p_LightControl->show();*/
}

/*QString mesIPPort;
TcpClient *_pmesClient;
void MainWindow::initMes()
{
    XmlOperate m_xml;
    m_xml.openXml(m_AllStationPath+"\\ComConfig.xml");
    QStringList comlist=m_xml.getChild(QStringList());
    if(!comlist.contains("MES"))return;
     mesIPPort=m_xml.readNode(QStringList()<<"MES"<<"TcpIPPort");
    m_xml.closeXml();
    _pmesClient=new TcpClient();
    int res= _pmesClient->initTcpClient(mesIPPort,"");
    //添加状态显示标签
    map_lblConnectInfo["MES"]=new QLabel(this);
    if(res==0)
    {
        map_lblConnectInfo["MES"]->setStyleSheet("QLabel{background-color:rgb(0,255,0);color: rgb(0, 0, 0);}");
    }
    else
    {
        map_lblConnectInfo["MES"]->setStyleSheet("QLabel{background-color:rgb(255,0,0);color: rgb(0, 0, 0);}");
    }
    ui->statusBar->addPermanentWidget(map_lblConnectInfo["MES"]);//添加显示连接标签到状态栏
    map_lblConnectInfo["MES"]->setText(tr("MES"));
    connect(_pmesClient,&TcpClient::signal_state,this,[=](bool state){
        if(state)
        {
            map_lblConnectInfo["MES"]->setStyleSheet("QLabel{background-color:rgb(0,255,0);color: rgb(0, 0, 0);}");
        }
        else
        {
            map_lblConnectInfo["MES"]->setStyleSheet("QLabel{background-color:rgb(255,0,0);color: rgb(0, 0, 0);}");
        }
                //_pTcpClient->initTcpClient(tcpIPPort,"");
    });
    //connect(this,&MainWindow::signal_MesDataSend,_pTcpClient,&TcpClient::sendStr);

}*/






