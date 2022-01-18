#include "parameterset.h"
#include "ui_parameterset.h"
#include"general/generalfunc.h"
#include<QMessageBox>
#include"general/xmloperate.h"

ParameterSet::ParameterSet(QWidget *parent) :
    QWidget(parent),
    m_Combobox_cur(nullptr),
    ui(new Ui::ParameterSet)
{
    ui->setupUi(this);
    //setWindowFlag(Qt::Window);
    //setWindowModality(Qt::WindowModal);
    //初始化变量类型
    parTypeList<<"IntPar"<<"FloatPar"<<"StringPar";
}

ParameterSet::~ParameterSet()
{
    delete ui;
}
//初始化参数
void ParameterSet::initPar(QString path)
{
    m_ConfigPath=path+"\\ParConfig.xml";
    readPar(m_ConfigPath);
    initUI();
}
void ParameterSet::closeEvent(QCloseEvent *event)
{
    close();
}

void ParameterSet::initUI()
{
    //初始化控件内容
    GeneralFunc::setTableWidgetHeader(ui->tblwgtPar,QStringList()<<tr(QString::fromLocal8Bit("变量名").toUtf8())
                                      <<tr(QString::fromLocal8Bit("变量值").toUtf8())
                                      <<tr(QString::fromLocal8Bit("变量类型").toUtf8()));
    showIntPar(m_IntParameter);
    showFloatPar(m_FloatParameter);
    showStringPar(m_StringParameter);
}
//读取参数
void ParameterSet::readPar(QString typePath)
{
    XmlOperate m_xml;
    m_xml.openXml(m_ConfigPath);
    QStringList parlist=m_xml.getChild(QStringList()<<parTypeList[0]);
    for (int i=0;i<parlist.size();i++)
    {
        m_IntParameter[parlist[i]]=m_xml.readNode(QStringList()<<parTypeList[0]<<parlist[i]).toInt();
    }
    parlist=m_xml.getChild(QStringList()<<parTypeList[1]);
    for (int i=0;i<parlist.size();i++)
    {
        m_FloatParameter[parlist[i]]=m_xml.readNode(QStringList()<<parTypeList[1]<<parlist[i]).toFloat();
    }
    parlist=m_xml.getChild(QStringList()<<parTypeList[2]);
    for (int i=0;i<parlist.size();i++)
    {
        m_StringParameter[parlist[i]]=m_xml.readNode(QStringList()<<parTypeList[2]<<parlist[i]);
    }
    m_xml.closeXml();
}

//显示Int参数
void ParameterSet::showIntPar(QMap<QString,int> map_int)
{
    //添加变量到列表
    QMap<QString, int>::iterator iter = map_int.begin();
    while (iter != map_int.end())
    {
        int row=ui->tblwgtPar->rowCount();
        ui->tblwgtPar->insertRow(row);
        QTableWidgetItem *item=new QTableWidgetItem(iter.key());
        ui->tblwgtPar->setItem(row,0,item);
        item=new QTableWidgetItem(QString::number(iter.value()));
        ui->tblwgtPar->setItem(row,1,item);
        item=new QTableWidgetItem("IntPar");
        ui->tblwgtPar->setItem(row,2,item);
        iter++;
    }
}
//显示float参数
void ParameterSet::showFloatPar(QMap<QString,float> map_float)
{
    //添加变量到列表
    QMap<QString, float>::iterator iter = map_float.begin();
    while (iter != map_float.end())
    {
        int row=ui->tblwgtPar->rowCount();
        ui->tblwgtPar->insertRow(row);
        QTableWidgetItem *item=new QTableWidgetItem(iter.key());
        ui->tblwgtPar->setItem(row,0,item);
        item=new QTableWidgetItem(QString::number(iter.value()));
        ui->tblwgtPar->setItem(row,1,item);
        item=new QTableWidgetItem("FloatPar");
        ui->tblwgtPar->setItem(row,2,item);
        iter++;
    }
}
//显示string参数
void ParameterSet::showStringPar(QMap<QString,QString> map_string)
{
    //添加变量到列表
    QMap<QString, QString>::iterator iter = map_string.begin();
    while (iter != map_string.end())
    {
        int row=ui->tblwgtPar->rowCount();
        ui->tblwgtPar->insertRow(row);
        QTableWidgetItem *item=new QTableWidgetItem(iter.key());
        ui->tblwgtPar->setItem(row,0,item);
        item=new QTableWidgetItem(iter.value());
        ui->tblwgtPar->setItem(row,1,item);
        item=new QTableWidgetItem("StringPar");
        ui->tblwgtPar->setItem(row,2,item);
        iter++;
    }
}

//写入参数
void ParameterSet::writePar()
{
    //写入参数
    XmlOperate m_xml;
    m_xml.openXml(m_ConfigPath);
    m_xml.clearXml();
    for( int i=0;i<ui->tblwgtPar->rowCount();i++)
    {
        QString section,keyText,valueText;
        keyText=ui->tblwgtPar->item(i,0)->text().remove(' ');
        valueText=ui->tblwgtPar->item(i,1)->text();
        section=ui->tblwgtPar->item(i,2)->text();
        m_xml.addNode(QStringList()<<section<<keyText,valueText);
    }
    m_xml.closeXml();
    emit signal_par_change();
}
void ParameterSet::on_pbtnEditPar_clicked()
{
    writePar();
}

void ParameterSet::on_pbtnAddPar_clicked()
{
    //增加参数
    int row=ui->tblwgtPar->rowCount();
    ui->tblwgtPar->insertRow(row);
    QTableWidgetItem *item=new QTableWidgetItem(QString("Mark"));
    ui->tblwgtPar->setItem(row,0,item);
    item=new QTableWidgetItem(QString("Values"));
    ui->tblwgtPar->setItem(row,1,item);
    item=new QTableWidgetItem("IntPar");
    ui->tblwgtPar->setItem(row,2,item);
    ui->tblwgtPar->scrollToBottom();

}

void ParameterSet::on_pbtnDeletePar_clicked()
{
    //删除参数,仅在列表中删除，保存之后生效
    int row=ui->tblwgtPar->currentRow();
    //removeRow
    ui->tblwgtPar->removeRow(row);
}
void ParameterSet::on_tblwgtPar_cellClicked(int row, int column)
{
    if(m_Combobox_cur!=nullptr)
    {
        ui->tblwgtPar->removeCellWidget(rowLast,2);
        delete  m_Combobox_cur;
        m_Combobox_cur=nullptr;
    }
    QString keyLast;
    if(column==2)
    {
        disconnect(textchange);
        rowLast=row;
        m_Combobox_cur=new QComboBox();
        keyLast=ui->tblwgtPar->item(row,column)->text();
        m_Combobox_cur->addItems(parTypeList);
        m_Combobox_cur->setCurrentText(keyLast);
        ui->tblwgtPar->setCellWidget(row,column,m_Combobox_cur);
        textchange=connect(m_Combobox_cur,&QComboBox::currentTextChanged,this,[=](QString text){
            QTableWidgetItem *itemtext=new QTableWidgetItem(text);
            ui->tblwgtPar->setItem(row,column,itemtext);
        });
    }
}
