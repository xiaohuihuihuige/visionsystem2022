#-------------------------------------------------
#
# Project created by QtCreator 2020-09-15T10:56:22
#
#-------------------------------------------------

QT       += core gui  network serialbus xml serialport axcontainer charts #concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = VisionSystem
TEMPLATE = app



# You can also make your code fail to compile if you use deprecated APIs.# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++17

#TRANSLATIONS = Translate_EN.ts \
#Translate_CN.ts

SOURCES += \
        about.cpp \
        camera/MvCamera.cpp \
        camera/cameracalibrate.cpp \
        camera/camerapar.cpp \
        camera/camoperate.cpp \
        camera/camrunning.cpp \
        camera/hikvisioncamera.cpp \
        camera/imagecalibrate.cpp \
        camera/webcam.cpp \
        communication/communication.cpp \
        communication/fxprotocol.cpp \
        communication/hostlink.cpp \
        communication/lightcontrol.cpp \
        communication/mcnet.cpp \
        communication/modbusrtu.cpp \
        communication/modbustcp.cpp \
        communication/serialportnet.cpp \
        communication/serialrelay.cpp \
        communication/taidaplc.cpp \
        communication/tcpclient.cpp \
        communication/weikongplc.cpp \
        controls/animationprogressbar.cpp \
        deeplearning/Classified/classfiedopenvino.cpp \
        deeplearning/Classified/classified.cpp \
        deeplearning/TargetDetection/detector.cpp \
        deeplearning/TargetDetection/yolov5openvino.cpp \
        deeplearning/crnn/crnnlibtorch.cpp \
        deeplearning/crnn/crnnopenvino.cpp \
        deeplearning/segmentation/yolact.cpp \
        drawingpapereditor.cpp \
        general/configfileoperate.cpp \
        general/exceloperate.cpp \
        general/jsonoperate.cpp \
        general/xmloperate.cpp \
        halcon/qthalcon.cpp \
        login.cpp \
        main.cpp \
        mainwindow.cpp \
        modelparset.cpp \
        opencv/line2Dup.cpp \
        opencv/qtopencv.cpp \
        os/crnntest.cpp \
        os/pro_allobjdetect.cpp \
        os/pro_connectseatcrack.cpp \
        os/pro_connectseatglue.cpp \
        os/pro_decodetext.cpp \
        os/pro_distance.cpp \
        os/pro_distanceonly.cpp \
        os/pro_gluewithseat.cpp \
        os/pro_heparingcap.cpp \
        os/pro_meltplugrowcheck.cpp \
        os/pro_metalcheck.cpp \
        os/pro_needledirection.cpp \
        os/pro_needleseatcrack.cpp \
        os/pro_needleseatglue.cpp \
        os/pro_oppositedetect.cpp \
        os/pro_pipedetect.cpp \
        os/pro_positivedetect.cpp \
        os/pro_siliconetube.cpp \
        os/pro_tubebase.cpp \
        parameterset.cpp \
        controls/ringsprogressbar.cpp \
        reportview.cpp \
        stationrunning.cpp \
        stationset.cpp \
        typeset.cpp

HEADERS += \
        about.h \
        camera/MvCamera.h \
        camera/MvCameraDefine.h \
        camera/cameracalibrate.h \
        camera/camerapar.h \
        camera/camoperate.h \
        camera/camrunning.h \
        camera/hikvisioncamera.h \
        camera/imagecalibrate.h \
        camera/webcam.h \
        communication/communication.h \
        communication/fxprotocol.h \
        communication/hostlink.h \
        communication/lightcontrol.h \
        communication/mcnet.h \
        communication/modbusrtu.h \
        communication/modbustcp.h \
        communication/serialportnet.h \
        communication/serialrelay.h \
        communication/taidaplc.h \
        communication/tcpclient.h \
        communication/weikongplc.h \
        controls/animationprogressbar.h \
        deeplearning/Classified/classfiedopenvino.h \
        deeplearning/Classified/classified.h \
        deeplearning/TargetDetection/detector.h \
        deeplearning/TargetDetection/utils.h \
        deeplearning/TargetDetection/yolov5openvino.h \
        deeplearning/crnn/crnnlibtorch.h \
        deeplearning/crnn/crnnopenvino.h \
        deeplearning/segmentation/yolact.h \
        drawingpapereditor.h \
        general/configfileoperate.h \
        general/exceloperate.h \
        general/generalfunc.h \
        general/jsonoperate.h \
        general/xmloperate.h \
        halcon/qthalcon.h \
        login.h \
        mainwindow.h \
        modelparset.h \
        opencv/line2Dup.h \
        opencv/qtopencv.h \
        os/crnntest.h \
        os/pro_allobjdetect.h \
        os/pro_connectseatcrack.h \
        os/pro_connectseatglue.h \
        os/pro_decodetext.h \
        os/pro_distance.h \
        os/pro_distanceonly.h \
        os/pro_gluewithseat.h \
        os/pro_heparingcap.h \
        os/pro_meltplugrowcheck.h \
        os/pro_metalcheck.h \
        os/pro_needledirection.h \
        os/pro_needleseatcrack.h \
        os/pro_needleseatglue.h \
        os/pro_oppositedetect.h \
        os/pro_pipedetect.h \
        os/pro_positivedetect.h \
        os/pro_siliconetube.h \
        os/pro_tubebase.h \
        parameterset.h \
        publicstruct.h \
        controls/ringsprogressbar.h \
        reportview.h \
        stationrunning.h \
        stationset.h \
        typeset.h

FORMS += \
        about.ui \
        camera/cameracalibrate.ui \
        camera/camerapar.ui \
        camera/camoperate.ui \
        camera/imagecalibrate.ui \
        communication/communication.ui \
        communication/lightcontrol.ui \
        login.ui \
        mainwindow.ui \
        modelparset.ui \
        parameterset.ui \
        reportview.ui \
        stationset.ui \
        typeset.ui

#设置程序在release的情况下以管理员模式运行
#CONFIG(release, debug|release){
#QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
#}
win32 : QMAKE_CXXFLAGS += /openmp
LIBS += -fopenmp
#windows绘图库导入
INCLUDEPATH += $$quote( C:\Program Files (x86)\10\Include\10.0.17763.0\um\ )
DEPENDPATH += $$quote( C:\Program Files (x86)\10\Lib\10.0.17763.0\um\x64\ )
LIBS += -lgdi32



#添加mvs
INCLUDEPATH += $$quote( D:/Program Files (x86)/MVS/Development/Includes\ )
DEPENDPATH += $$quote( D:/Program Files (x86)/MVS/Development/Libraries/win64 )
DEPENDPATH += $$quote( D:/Program Files (x86)/MVS/Development/Libraries/GenICam/win64 )
LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\win64\MvCameraControl.lib )
LIBS += -L'D:/Program Files (x86)/MVS/Development/Libraries/GenICam/win64/' \
-lCLAllSerial_MD_VC120_v3_0_MVS_v3_1_0      \
-lCLProtocol_MD_VC120_v3_0_MVS_v3_1_0       \
-lCLSerCOM                                  \
-lGCBase_MD_VC120_v3_0_MVS_v3_1_0           \
-lGenApi_MD_VC120_v3_0_MVS_v3_1_0           \
-lGenCP_MD_VC120_v3_0_MVS_v3_1_0            \
-lLog_MD_VC120_v3_0_MVS_v3_1_0              \
-llog4cpp_MD_VC120_v3_0_MVS_v3_1_0          \
-lMathParser_MD_VC120_v3_0_MVS_v3_1_0       \
-lNodeMapData_MD_VC120_v3_0_MVS_v3_1_0      \
-lXmlParser_MD_VC120_v3_0_MVS_v3_1_0

#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\win64\MvCameraControl.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\CLAllSerial_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\CLProtocol_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\CLSerCOM.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\GCBase_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\GenApi_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\GenCP_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\Log_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\log4cpp_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\MathParser_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\NodeMapData_MD_VC120_v3_0_MVS_v3_1_0.lib )
#LIBS += $$quote( D:\Program Files (x86)\MVS\Development\Libraries\GenICam\win64\XmlParser_MD_VC120_v3_0_MVS_v3_1_0.lib )

#添加halcon
INCLUDEPATH += $$quote( D:/Program Files/MVTec/HALCON-12.0/include )
INCLUDEPATH += $$quote( D:/Program Files/MVTec/HALCON-12.0/include/halconcpp )

LIBS    += $$quote( D:/Program Files/MVTec/HALCON-12.0/lib/x64-win64/halconcpp.lib )
LIBS    +=$$quote( D:/Program Files/MVTec/HALCON-12.0/lib/x64-win64/halcon.lib )

#添加openvino
INCLUDEPATH += $$quote( C:/Intel/openvino_2021.3.394/deployment_tools/inference_engine/include )
DEPENDPATH += $$quote( C:/Intel/openvino_2021.3.394/deployment_tools/inference_engine/include )
win32:CONFIG(release, debug|release): LIBS += -L'C:/Intel/openvino_2021.3.394/deployment_tools/inference_engine/lib/intel64/Release/' \
-linference_engine_c_api                \
-linference_engine_ir_reader            \
-linference_engine_legacy               \
-linference_engine_lp_transformations   \
-linference_engine_preproc              \
-linference_engine_transformations      \
-linference_engine_onnx_reader          \
-linference_engine
win32:CONFIG(debug, debug|release): LIBS += -L'C:/Intel/openvino_2021.3.394/deployment_tools/inference_engine/lib/intel64/Debug/' \
-linference_engine_c_apid               \
-linference_engine_ir_readerd           \
-linference_engine_legacyd              \
-linference_engine_lp_transformationsd  \
-linference_engine_preprocd             \
-linference_engine_transformationsd     \
-linference_engine_onnx_readerd         \
-linference_engined

#添加opencv
#INCLUDEPATH += $$quote( C:/Intel/openvino_2021.3.394/opencv/include )
#win32:CONFIG(release, debug|release): LIBS += -L'C:/Intel/openvino_2021.3.394/opencv/lib/' \
#-lopencv_calib3d452     \
#-lopencv_core452        \
#-lopencv_dnn452         \
#-lopencv_features2d452  \
#-lopencv_flann452       \
#-lopencv_highgui452     \
#-lopencv_imgcodecs452   \
#-lopencv_imgproc452     \
#-lopencv_ml452          \
#-lopencv_objdetect452   \
#-lopencv_photo452       \
#-lopencv_stitching452   \
#-lopencv_video452       \
#-lopencv_videoio452
#win32:CONFIG(debug, debug|release): LIBS += -L'C:/Intel/openvino_2021.3.394/opencv/lib/' \
#-lopencv_calib3d452d    \
#-lopencv_core452d       \
#-lopencv_dnn452d        \
#-lopencv_features2d452d \
#-lopencv_flann452d      \
#-lopencv_highgui452d    \
#-lopencv_imgcodecs452d  \
#-lopencv_imgproc452d    \
#-lopencv_ml452d         \
#-lopencv_objdetect452d  \
#-lopencv_photo452d      \
#-lopencv_stitching452d  \
#-lopencv_video452d      \
#-lopencv_videoio452d

#添加opencv
INCLUDEPATH += $$quote( D:/Program Files/opencv-4.1.0-x64-v15/install/include/opencv2\ )
INCLUDEPATH += $$quote( D:/Program Files/opencv-4.1.0-x64-v15/install/include\ )
DEPENDPATH += $$quote( D:/Program Files/opencv-4.1.0-x64-v15/install/x64/vc15/lib )
win32:CONFIG(release, debug|release): LIBS += -L'D:/Program Files/opencv-4.1.0-x64-v15/install/x64/vc15/lib/' \
-lopencv_calib3d410     \
-lopencv_core410        \
-lopencv_dnn410         \
-lopencv_features2d410  \
-lopencv_flann410       \
-lopencv_highgui410     \
-lopencv_imgcodecs410   \
-lopencv_imgproc410     \
-lopencv_ml410          \
-lopencv_objdetect410   \
-lopencv_photo410       \
-lopencv_stitching410   \
-lopencv_video410       \
-lopencv_videoio410

win32:CONFIG(debug, debug|release): LIBS += -L'D:/Program Files/opencv-4.1.0-x64-v15/install/x64/vc15/lib/' \
-lopencv_calib3d410d    \
-lopencv_core410d       \
-lopencv_dnn410d        \
-lopencv_features2d410d \
-lopencv_flann410d      \
-lopencv_highgui410d    \
-lopencv_imgcodecs410d  \
-lopencv_imgproc410d    \
-lopencv_ml410d         \
-lopencv_objdetect410d  \
-lopencv_photo410d      \
-lopencv_stitching410d  \
-lopencv_video410d      \
-lopencv_videoio410d


#包含torch
INCLUDEPATH += $$quote( D:/Program Files/libtorch1.7.1-x64/debug/include\ )
INCLUDEPATH += $$quote( D:/Program Files/libtorch1.7.1-x64/debug/include/torch/csrc/api/include\ )
win32:CONFIG(release, debug|release):\
LIBS += -L'D:/Program Files/libtorch1.7.1-x64/release/lib/' \
-lasmjit                      \
-lc10                         \
-lc10_cuda                    \
-lc10d                        \
-lcaffe2_detectron_ops_gpu    \
-lcaffe2_module_test_dynamic  \
-lcaffe2_nvrtc                \
-lclog                        \
-lcpuinfo                     \
-ldnnl                        \
-lfbgemm                      \
-lgloo                        \
-lgloo_cuda                   \
-llibprotobuf                 \
-llibprotobuf-lite            \
-llibprotoc                   \
-lmkldnn                      \
-ltorch                       \
-ltorch_cuda                  \
-ltorch_cpu                   \
-INCLUDE:?warp_size@cuda@at@@YAHXZ

else:win32:CONFIG(debug, debug|release):\
LIBS += -L'D:/Program Files/libtorch1.7.1-x64/debug/lib/' \
-lasmjit                      \
-lc10                         \
-lc10_cuda                    \
-lc10d                        \
-lcaffe2_detectron_ops_gpu    \
-lcaffe2_module_test_dynamic  \
-lcaffe2_nvrtc                \
-lclog                        \
-lcpuinfo                     \
-ldnnl                        \
-lfbgemm                      \
-lgloo                        \
-lgloo_cuda                   \
-llibprotobufd                \
-llibprotobuf-lited           \
-llibprotocd                  \
-lmkldnn                      \
-ltorch                       \
-ltorch_cuda                  \
-ltorch_cpu                   \
-INCLUDE:?warp_size@cuda@at@@YAHXZ

INCLUDEPATH += $$quote( D:\Program Files\qzxing-master\src\ )
win32:CONFIG(release, debug|release): LIBS += -L'D:\Program Files\qzxing-master\lib/'\
-lQZXing2


#RC_ICONS = car.ico
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    image.qrc

DISTFILES +=
