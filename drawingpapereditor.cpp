﻿#include "drawingpapereditor.h"
#include <QPainter>


QtDrawingPaperEditor::QtDrawingPaperEditor(QWidget *parent)
    : QWidget(parent)
{
    this->setMouseTracking(true);
    QSizePolicy sizepolicy=this->sizePolicy();
    sizepolicy.setHorizontalPolicy(QSizePolicy::Expanding);
    sizepolicy.setVerticalPolicy(QSizePolicy::Expanding);
    this->setSizePolicy(sizepolicy);
    editor_bg_color_ = QColor(54, 54, 54);
    editor_paper_bg_color_ = QColor(27, 27, 27);
    is_mouse_mid_btn_down_ = false;
    is_mouse_right_btn_down_ = false;
    UpdatePaperWidthOfPerPixel();
    paper_x_ = this->x();
    paper_y_ = this->y();
    paper_width_ = this->width();
    paper_height_ = this->height();
    scale_value_ = 1.0;
    UpdatePaperWidthOfPerPixel();
    CentenThePaper();
}

QtDrawingPaperEditor::~QtDrawingPaperEditor()
{
}
void QtDrawingPaperEditor::initUI()
{
    paper_x_ = this->x();
    paper_y_ = this->y();
    paper_width_ = this->width();
    paper_height_ = this->height();
    scale_value_ = 1.0;
    UpdatePaperWidthOfPerPixel();
    CentenThePaper();
}
void QtDrawingPaperEditor::initImage(QImage image)
{
    show_image=image.copy(QRect(0,0,image.width(),image.height()));
    update();
    //initUI();
}


void QtDrawingPaperEditor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    //draw bg
    QRect frame_rect = this->rect();
    painter.fillRect(frame_rect, QBrush(editor_bg_color_));
    DrawPaper(painter);
}

void QtDrawingPaperEditor::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    //if(event->buttons() & Qt::LeftButton)
    //计算点击得坐标点
    mouse_down_pos_ = event->pos();
    orgin_dif_x_ = mouse_down_pos_.x() - paper_x_;
    orgin_dif_y_ = mouse_down_pos_.y() - paper_y_;
    if(event->buttons() & Qt::LeftButton)
    {
        if(orgin_dif_x_/scale_value_<0||orgin_dif_y_/scale_value_>this->rect().height()
                ||orgin_dif_x_/scale_value_>this->rect().width()||orgin_dif_y_/scale_value_<0)return;
        emit signal_PressPoint(camname,QPoint(orgin_dif_x_/scale_value_,orgin_dif_y_/scale_value_));
    }
    else if(event->buttons() & Qt::MidButton)
    {
        is_mouse_mid_btn_down_ = true;
    }
    else if(event->buttons() & Qt::RightButton)
    {
        initUI();
        is_mouse_right_btn_down_ = true;
    }
}

void QtDrawingPaperEditor::mouseMoveEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if(is_mouse_mid_btn_down_ == true)
    {
        paper_x_ = event->pos().x() - orgin_dif_x_;
        paper_y_ = event->pos().y() - orgin_dif_y_;
        update();
    }

        /*mouse_down_pos_ = event->pos();
        orgin_dif_x_ = mouse_down_pos_.x() - paper_x_;
        orgin_dif_y_ = mouse_down_pos_.y() - paper_y_;
        if(orgin_dif_x_/scale_value_<0||orgin_dif_y_/scale_value_>this->rect().height()
                ||orgin_dif_x_/scale_value_>this->rect().width()||orgin_dif_y_/scale_value_<0)return;
        emit signal_PressPoint(camname,QPoint(orgin_dif_x_/scale_value_,orgin_dif_y_/scale_value_));*/
}

void QtDrawingPaperEditor::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    is_mouse_mid_btn_down_ = false;
    is_mouse_right_btn_down_ = false;
}

void QtDrawingPaperEditor::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    initUI();
    UpdatePaperWidthOfPerPixel();
    CentenThePaper();
}

void QtDrawingPaperEditor::enterEvent(QEvent *event)
{
    Q_UNUSED(event)
}

void QtDrawingPaperEditor::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
}

void QtDrawingPaperEditor::wheelEvent(QWheelEvent *event)
{
    Q_UNUSED(event)
    QPoint numPixels = event->pixelDelta();
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numPixels.isNull())
    {
        OnWheelValueChanged(event->pos(),numPixels);
    }
    else if (!numDegrees.isNull())
    {
        QPoint numSteps = numDegrees / 15;
        OnWheelValueChanged(event->pos(), numSteps);
    }
    event->accept();
}

//scale_value : 0.5~ 20
//paper_width :
//widget_width :
double QtDrawingPaperEditor::CalcPaperWidthOfPerPixel(double scale_value,
                                                        int paper_width,
                                                        int widget_width)
{
    if(scale_value>20)scale_value=20.0;
    int paper_width_of_final_show =
                paper_width ;//+ static_cast<int>(static_cast<double>(paper_width )* 0.2);
    int scaled_widget_width = qRound(static_cast<double>(widget_width) * scale_value);
    double paper_width_of_per_pixel =
                                    static_cast<double>(paper_width_of_final_show) /
                                    static_cast<double>(scaled_widget_width);
    //limit readable per-pixel value
    if(paper_width_of_per_pixel < 0.0005) paper_width_of_per_pixel = 0.0005;
    return paper_width_of_per_pixel;
}

//re-calc  "paper_width_of_per_pixel" only when : init, resize, scale_value changed
void QtDrawingPaperEditor::UpdatePaperWidthOfPerPixel()
{
    if(this->height() >= this->width())
    {
        paper_width_of_per_pixel_ = CalcPaperWidthOfPerPixel(scale_value_, paper_width_, this->width());
    }
    else
    {
        paper_width_of_per_pixel_ = CalcPaperWidthOfPerPixel(scale_value_, paper_width_, this->width());
    }
}

int QtDrawingPaperEditor::PaperWidth2DrawWidth(int paper_width)
{
    double draw_width = static_cast<double>(paper_width) / paper_width_of_per_pixel_;
    return qRound(draw_width);
}

int QtDrawingPaperEditor::DrawWidth2PaperWidth(int draw_width)
{
    double paper_width = static_cast<double>(draw_width) * paper_width_of_per_pixel_;
    return static_cast<int>(qRound(paper_width));
}

void QtDrawingPaperEditor::OnWheelValueChanged(QPoint mouse_pos, QPoint step)
{
    //if mouse point in paper
    if(draw_paper_rect_.contains(mouse_pos))
    {
        QPoint before_resize_mouse_point_at_paper =  MousePoint2PaperPoint(mouse_pos);
        int temp_paper_point_x = DrawWidth2PaperWidth(before_resize_mouse_point_at_paper.x());
        int temp_paper_point_y = DrawWidth2PaperWidth(before_resize_mouse_point_at_paper.y());
        //resize
        int step_value =  step.y();
        scale_value_ += static_cast<double>(step_value) /20.0;
        if(scale_value_ > SCALE_VALUE_MAX) scale_value_ = SCALE_VALUE_MAX;
        if(scale_value_ < SCALE_VALUE_MIN) scale_value_ = SCALE_VALUE_MIN;
        UpdatePaperWidthOfPerPixel();
        int temp_draw_point_x = PaperWidth2DrawWidth(temp_paper_point_x);
        int temp_draw_point_y = PaperWidth2DrawWidth(temp_paper_point_y);
        QPoint after_resize_mouse_point_at_paper(temp_draw_point_x, temp_draw_point_y);
        QPoint should_move_length = after_resize_mouse_point_at_paper - before_resize_mouse_point_at_paper;
        paper_x_ -= should_move_length.x();
        paper_y_ -= should_move_length.y();
        update();
    }
    else
    { //else using center resize
        int old_width = draw_paper_rect_.width();
        int old_height = draw_paper_rect_.height();
        //resize
        int step_value =  step.y();
        scale_value_ += static_cast<double>(step_value) /20.0;
        if(scale_value_ > SCALE_VALUE_MAX) scale_value_ = SCALE_VALUE_MAX;
        if(scale_value_ < SCALE_VALUE_MIN) scale_value_ = SCALE_VALUE_MIN;
        UpdatePaperWidthOfPerPixel();
        int new_width = PaperWidth2DrawWidth(paper_width_);
        int new_height = PaperWidth2DrawWidth(paper_height_);
        int adjusted_height = new_height - old_height;
        int adjusted_width = new_width - old_width;
        paper_x_ -= adjusted_width/2;
        paper_y_ -= adjusted_height/2;
        update();
    }
}

void QtDrawingPaperEditor::DrawPaper(QPainter &painter)
{
    int draw_width = PaperWidth2DrawWidth(paper_width_);
    int draw_height = PaperWidth2DrawWidth(paper_height_);
    draw_paper_rect_.setX(paper_x_);
    draw_paper_rect_.setY(paper_y_);
    draw_paper_rect_.setWidth(draw_width);
    draw_paper_rect_.setHeight(draw_height);
    painter.fillRect(draw_paper_rect_, QBrush(editor_paper_bg_color_));
    QPixmap pixmap=QPixmap::fromImage(show_image);
    painter.drawPixmap(draw_paper_rect_, pixmap);
}

QPoint QtDrawingPaperEditor::MousePoint2PaperPoint(QPoint point)
{
    QPoint ret;
    ret.setX(point.x() - paper_x_);
    ret.setY(point.y() - paper_y_);
    return ret;
}

void QtDrawingPaperEditor::CentenThePaper()
{
    //scale_value_ = 0.9;
    UpdatePaperWidthOfPerPixel();
    int adjust_distance_x = (this->width() - PaperWidth2DrawWidth(paper_width_)) / 2;
    int adjust_distance_y = (this->height() - PaperWidth2DrawWidth(paper_height_)) / 2;
    paper_x_ = adjust_distance_x;
    paper_y_ = adjust_distance_y;
    update();
}



