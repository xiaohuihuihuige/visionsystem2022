#ifndef PUBLICSTRUCT_H
#define PUBLICSTRUCT_H

#include <QString>
#include<QMap>
#include<QTimer>
#include"camera/hikvisioncamera.h"
#include"camera/webcam.h"
#include"communication/fxprotocol.h"
#include"communication/modbustcp.h"
#include"communication/modbusrtu.h"
#include"communication/mcnet.h"
#include"communication/hostlink.h"
#include"communication/serialportnet.h"
#include"communication/tcpclient.h"
#include"communication/serialrelay.h"
#include"communication/weikongplc.h"
#include<QMetaEnum>
#include<QObject>

//const QString protocol_type_list[]={"ModbusTCP","MELSEC_MC","TCP","ModbusRTU","MELSEC_FX3U","HostLink","Serial"};
static QStringList protocol_type_list=QStringList()<<"ModbusTCP"<<"MELSEC_MC"<<"TCP"<<"ModbusRTU"
                                                  <<"MELSEC_FX3U"<<"HostLink"<<"Serial"<<"SerialRelay"<<"WeiKongPLC";

/*const QString funtion_list[]={"Pro_PipeDetect","Pro_HeparingCap","Pro_NeedleSeatGlue","Pro_ConnectSeatGlue",
                                    "Pro_Distance","Pro_NeedleSeatCrack","Pro_ConnectSeatCrack","Pro_TubeBase",
                                   "Pro_SiliconeTube"};*/
static QStringList funtion_list=QStringList()<<"Pro_PipeDetect"<<"Pro_HeparingCap"<<"Pro_NeedleSeatGlue"
                                            <<"Pro_ConnectSeatGlue"<<"Pro_Distance"<<"Pro_NeedleSeatCrack"
                                           <<"Pro_ConnectSeatCrack"<<"Pro_TubeBase"<<"Pro_SiliconeTube"
                                          <<"Pro_DecodeText"<<"Pro_DistanceOnly"<<"Pro_GlueWithSeat"
                                         <<"Pro_NeedleDirection"<<"Pro_MetalCheck"<<"Pro_MeltPlugRowCheck"
                                        <<"Pro_OppositeDetect"<<"Pro_PositiveDetect"<<"CrnnTest"<<"Pro_AllObjDetect";

//const QString cam_type_list[]={"HikCam","WebCam","DahuaCam","DahengCam","BaslerCam"};
static QStringList cam_type_list=QStringList()<<"HikCam"<<"WebCam"<<"DahuaCam"<<"DahengCam"<<"BaslerCam";

static QStringList roi_type_list=QStringList()<<"Rectangle1"<<"Rectangle2"<<"Circle"<<"Polygon"<<"Xld"<<"Cirque"<<"Nurb";

class EnumChange:public QObject
{
    Q_OBJECT
public:
    enum CamType
    {
        HikCam=0,
        WebCam,
        DahuaCam,
        DahengCam,
        BaslerCam
    };
    Q_ENUM(CamType)
    enum PLCCom
    {
        ModbusTCP=0,
        MELSEC_MC,
        TCP,
        ModbusRTU,
        MELSEC_FX3U,
        HostLink,
        Serial,
        SerialRelay,
        WeiKongPLC
    };
    Q_ENUM(PLCCom)
    enum Func
    {
        Pro_PipeDetect=0,
        Pro_HeparingCap,
        Pro_NeedleSeatGlue,
        Pro_ConnectSeatGlue,
        Pro_Distance,
        Pro_NeedleSeatCrack,
        Pro_ConnectSeatCrack,
        Pro_TubeBase,
        Pro_SiliconeTube,
        Pro_DecodeText,
        Pro_DistanceOnly,
        Pro_GlueWithSeat,
        Pro_NeedleDirection,
        Pro_MetalCheck,
        Pro_MeltPlugRowCheck,
        Pro_OppositeDetect,
        Pro_PositiveDetect,
        CrnnTest,
        Pro_AllObjDetect
    };
    Q_ENUM(Func)

    enum ROIType
    {
        Rectangle1=0,
        Rectangle2,
        Circle,
        Polygon,
        Xld,
        Cirque,
        Nurb,
    };
    Q_ENUM(ROIType)


    /*struct Par
    {
        QString par_name;
        QString par_type;
        QString par_value;
    };
    enum ParType
    {
        Par_Int=0,
        Par_Float,
        Par_String,
        Par_IntArr,
        Par_FloatArr,
        Par_StringArr,
    };
    Q_ENUM(ParType)*/

    static CamType string2enum_cam(QString enumration)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<CamType>();
        return CamType(metaEnum.keyToValue(enumration.toUtf8()));
    }
    static PLCCom string2enum_plc(QString enumration)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<PLCCom>();
        return PLCCom(metaEnum.keyToValue(enumration.toUtf8()));
    }
    static Func string2enum_pro(QString enumration)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<Func>();
        return Func(metaEnum.keyToValue(enumration.toUtf8()));
    }
    static ROIType string2enum_roitype(QString enumration)
    {
        QMetaEnum metaEnum = QMetaEnum::fromType<ROIType>();
        return ROIType(metaEnum.keyToValue(enumration.toUtf8()));
    }
    //enumchange::CamType cat=enumchange::CamType(string2enum(iter->camType));

};

//cam1
    //类型
    //型号:序列号
    //图像宽:图像高
    //曝光
    //增益
    //采集延时
    //触发模式
    //触发源
    //是否开启
    //步骤一
        //校正
        //相机内参
        //相机外参
        //比例
        //世界宽:世界高
        //X偏移:Y偏移
    //步骤二
        //不校正
    //步骤三
        //校正
        //相机内参
        //相机外参
        //比例
        //世界宽:世界高
        //X偏移:Y偏移

//cameraInfo
struct CamInfo
{
    QString camType;//相机类型:HikCam,WebCam,DahuaCam,DahengCam,BaslerCam,
    QString cameraModelSerial;//相机型号和序列号
    QString resolution;    //分辨率
    QString exposureTime;//相机曝光时间
    QString gain;//相机曝光时间
    QString grabDelay;//相机延时采图
    QString triggerMode;//触发模式
    QString triggerSource;//触发源
    QString camStepAll;
    bool isOpen;//相机是否开启
    HikvisionCamera *hikvisionCamera;//相机类
    WebCam *webCam;//usb免驱相机
    struct CalInfo
    {
        QString isCal;//是否校正
        QString camPar;//相机内参
        QString camPose;//相机位姿
        QString ratio;//像素比例
        QString worldWidth;//世界宽
        QString worldHeight;//世界高
        QString X_Offset;//x偏移
        QString Y_Offset;//y偏移
    };
    QMap<QString,CalInfo> map_step_cal;
    CamInfo()
    {
        camType="";
        cameraModelSerial="";
        resolution="2448:2048";
        exposureTime="100";
        gain="0.0";
        grabDelay="10";
        triggerMode="1";
        triggerSource="7";
        isOpen=false;
        hikvisionCamera=nullptr;
        webCam=nullptr;
    }
};

struct ComInfo
{
    QString plcType;//plc类型
    int serverID;//服务端口
    int readTriggerSignal; //是否实时读取
    int sec;//秒表计时周期
    bool isConnect;    //是否链接
    QMap<QString,int> addr;//地址

    QString tcpIPPort;//IP端口
    ModbusTcp *modbusTcp;//Modbus PLC通讯类
    MCNet *mcNet;//Modbus PLC通讯类
    TcpClient *tcpClient;//tcp PLC通讯类

    QString portname;//端口名
    QString parity;//校验//odd为奇校验，even为偶校验
    int baudrate;//波特率
    int dataBits;//数据位
    int stopBits;//停止位
    ModbusRTU *modbusRTU;//modbusRTU
    FXProtocol *fxProtocol;//FX3U PLC通讯类
    HostLink *hostLink;//FX3U PLC通讯类
    SerialPortNet *serialPortNet;//Serial PLC通讯类
    SerialRelay *serialRelay;//串口继电器
    WeiKongPLC *weiKongPLC;//维控plc

    ComInfo()
    {
        plcType=protocol_type_list[0];
        serverID=0;
        readTriggerSignal=0;
        sec=100;
        isConnect=false;
        addr=QMap<QString,int>();

        tcpIPPort="127.0.0.1:3000";
        modbusTcp=nullptr;
        mcNet=nullptr;
        tcpClient=nullptr;

        portname="COM1";
        parity="No";
        baudrate=9600;
        dataBits=8;
        stopBits=1;
        modbusRTU=nullptr;
        fxProtocol=nullptr;
        hostLink=nullptr;
        serialPortNet=nullptr;
        serialRelay=nullptr;

    }
};





#endif // PUBLICSTRUCT_H
