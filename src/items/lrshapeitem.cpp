/***************************************************************************
 *   This file is part of the Lime Report project                          *
 *   Copyright (C) 2015 by Alexander Arin                                  *
 *   arin_a@bk.ru                                                          *
 *                                                                         *
 **                   GNU General Public License Usage                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 **                  GNU Lesser General Public License                    **
 *                                                                         *
 *   This library is free software: you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library.                                      *
 *   If not, see <http://www.gnu.org/licenses/>.                           *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 ****************************************************************************/
#include "lrshapeitem.h"
#include "lrdesignelementsfactory.h"
#include <QStyleOptionGraphicsItem>
#include <QPainter>

namespace{

const QString xmlTag = "ShapeItem";

LimeReport::BaseDesignIntf * createShapeItem(QObject* owner, LimeReport::BaseDesignIntf*  parent){
    return new LimeReport::ShapeItem(owner,parent);
}
bool registred = LimeReport::DesignElementsFactory::instance().registerCreator(
                     xmlTag, LimeReport::ItemAttribs(QObject::tr("Shape Item"),"Item"), createShapeItem
                 );
}


namespace LimeReport{

ShapeItem::ShapeItem(QObject *owner, QGraphicsItem *parent)
    :ItemDesignIntf(xmlTag,owner,parent),
      m_shape(HorizontalLine),
      m_shapeColor(Qt::black),
      m_shapeBrushColor(Qt::black),
      m_shapeBrushType(Qt::NoBrush),
      m_lineWidth(1),
      m_penStyle(Qt::SolidLine),
      m_opacity(100)
{
}

void ShapeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();

    QPen pen(m_shapeColor);
    pen.setWidthF(m_lineWidth);
    pen.setStyle(m_penStyle);
    painter->setPen(pen);
    QBrush brush(m_shapeBrushColor,m_shapeBrushType);

    painter->setBrush(brush);
    painter->setBackground(QBrush(Qt::NoBrush));
    painter->setOpacity(qreal(m_opacity)/100);

    switch (m_shape){
    case HorizontalLine:
        painter->drawLine(0,rect().height()/2,rect().right(),rect().height()/2);
        break;
    case VerticalLine:
        painter->drawLine(rect().width()/2,0,rect().width()/2,rect().height());
        break;
    case Ellipse:
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawEllipse(rect());
        break;
    case Rectangle:
        painter->setRenderHint(QPainter::Antialiasing);
        painter->drawRect(rect());
        break;
    }
    painter->restore();
    ItemDesignIntf::paint(painter,option,widget);

}

void ShapeItem::setShapeColor(QColor value)
{
    if ((value!=m_shapeColor)){
        QColor oldValue = m_shapeColor;
        m_shapeColor=value;
        update();
        notify("shapeColor",oldValue,value);
    }
}

void ShapeItem::setShapeBrushColor(QColor value)
{
    if (value!=m_shapeBrushColor){
        QColor oldValue = m_shapeBrushColor;
        m_shapeBrushColor=value;
        update();
        notify("shapeBrushColor",oldValue,value);
    }
}

void ShapeItem::setShapeBrushType(Qt::BrushStyle value)
{
    if (m_shapeBrushType!=value){
        Qt::BrushStyle oldValue = m_shapeBrushType;
        m_shapeBrushType=value;
        update(rect());
        notify("shapeBrush",QBrush(oldValue),QBrush(value));
    }
}

void ShapeItem::setShapeType(ShapeItem::ShapeType value)
{
    if (m_shape!=value){
        ShapeType oldValue = m_shape;
        m_shape=value;
        update();
        notify("shape",oldValue,value);
    }
}

void ShapeItem::setLineWidth(qreal value)
{
    if (m_lineWidth!=value){
        qreal oldValue = m_lineWidth;
        m_lineWidth=value;
        update();
        notify("lineWidth",oldValue,value);

    }
}

Qt::PenStyle ShapeItem::penStyle() const
{
    return m_penStyle;
}

void ShapeItem::setPenStyle(const Qt::PenStyle &value)
{
    if (m_penStyle!=value){
        Qt::PenStyle oldValue = m_penStyle;
        m_penStyle = value;
        update();
        notify("penStyle",(int)oldValue,(int)value);
    }
}

BaseDesignIntf *ShapeItem::createSameTypeItem(QObject *owner, QGraphicsItem *parent)
{
    return new ShapeItem(owner,parent);
}
int ShapeItem::opacity() const
{
    return m_opacity;
}

void ShapeItem::setOpacity(int opacity)
{
    if (m_opacity!=opacity){
        if (opacity < 0) {
            m_opacity = 0;
        }
        else if (opacity > 100) {
            m_opacity = 100;
        }
        else {
            m_opacity =  opacity;
        }
        update();
    }
}



}
