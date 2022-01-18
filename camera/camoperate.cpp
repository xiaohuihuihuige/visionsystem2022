#include "camoperate.h"
#include "ui_camoperate.h"
#include<QVBoxLayout>

//cam1
   //类型
   //


CamOperate::CamOperate(QWidget *parent) :
    QWidget(parent),
    tabWidget(nullptr),
    cameraPar(nullptr),
    cameracalibrate(nullptr),
    imageCal(nullptr),
    ui(new Ui::CamOperate)
{
    ui->setupUi(this);

    this->setWindowTitle(QString::fromLocal8Bit("相机设置"));
    setWindowFlag(Qt::Window);
    setWindowModality(Qt::WindowModal);
    tabWidget=new QTabWidget(this);
    ui->verticalLayout->addWidget(tabWidget);
    connect(tabWidget,&QTabWidget::tabBarClicked,this,&CamOperate::tab_clicked );

}

CamOperate::~CamOperate()
{
    if(tabWidget!=nullptr)
    {
        delete tabWidget;
        tabWidget=nullptr;
    }
    delete ui;
}
void CamOperate::closeEvent(QCloseEvent *event)
{

    if(cameraPar!=nullptr)
    {
        delete cameraPar;
        cameraPar=nullptr;
    }
    if(cameracalibrate!=nullptr)
    {
        delete cameracalibrate;
        cameracalibrate=nullptr;
    }
    if(imageCal!=nullptr)
    {
        delete imageCal;
        imageCal=nullptr;
    }
    event->accept();
}
//初始化变量
void CamOperate::initPar(QString path)
{
    g_pro_path=path;
    initUI();
}
//初始化UI
void CamOperate::initUI()
{
    cameraPar=new CameraPar();
    cameraPar->initPar(g_pro_path);
    tabWidget->addTab(cameraPar,QString::fromLocal8Bit("相机参数"));

    cameracalibrate=new CameraCalibrate();
    cameracalibrate->initPar(g_pro_path);
    tabWidget->addTab(cameracalibrate,QString::fromLocal8Bit("相机标定"));


    imageCal =new ImageCalibrate();
    imageCal->initPar(g_pro_path);
    tabWidget->addTab(imageCal,QString::fromLocal8Bit("图像校正"));

}


void CamOperate::tab_clicked(int index)
{
    QString  tabLabel=tabWidget->tabText(index);
    if(tabLabel==QString::fromLocal8Bit("相机参数"))
    {
        cameraPar->initPar(g_pro_path);
    }
    else if(tabLabel==QString::fromLocal8Bit("相机标定"))
    {
        cameracalibrate->initPar(g_pro_path);
    }
    else if(tabLabel==QString::fromLocal8Bit("图像校正"))
    {
        imageCal->initPar(g_pro_path);
    }

}

