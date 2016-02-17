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
#include "lrreportdesignwidget.h"
#include "serializators/lrxmlreader.h"
#include "serializators/lrxmlwriter.h"
#include "lrreportengine_p.h"
#include "lrbasedesignintf.h"

#include <QDebug>
#include <QObject>
#include <QDockWidget>
#include <QtXml>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QApplication>


namespace LimeReport {

//GraphicsViewZoom
GraphicsViewZoom::GraphicsViewZoom(QGraphicsView* view)
  : QObject(view), m_view(view)
{
  m_view->viewport()->installEventFilter(this);
  m_view->setMouseTracking(true);
  m_modifiers = Qt::ControlModifier;
  m_zoomFactorBase = 1.0015;
}

void GraphicsViewZoom::gentleZoom(double factor) {
  m_view->scale(factor, factor);
  m_view->centerOn(m_targetScenePos);
  QPointF delta_viewport_pos = m_targetViewportPos - QPointF(m_view->viewport()->width() / 2.0,
                                                             m_view->viewport()->height() / 2.0);
  QPointF viewport_center = m_view->mapFromScene(m_targetScenePos) - delta_viewport_pos;
  m_view->centerOn(m_view->mapToScene(viewport_center.toPoint()));
  emit zoomed();
}

void GraphicsViewZoom::setModifiers(Qt::KeyboardModifiers modifiers) {
  m_modifiers = modifiers;
}

void GraphicsViewZoom::setZoomFactorBase(double value) {
  m_zoomFactorBase = value;
}

bool GraphicsViewZoom::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::MouseMove) {
    QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
    QPointF delta = m_targetViewportPos - mouse_event->pos();
    if (qAbs(delta.x()) > 5 || qAbs(delta.y()) > 5) {
      m_targetViewportPos = mouse_event->pos();
      m_targetScenePos = m_view->mapToScene(mouse_event->pos());
    }
  } else if (event->type() == QEvent::Wheel) {
    QWheelEvent* wheel_event = static_cast<QWheelEvent*>(event);
    if (QApplication::keyboardModifiers() == m_modifiers) {
      if (wheel_event->orientation() == Qt::Vertical) {
        double angle = wheel_event->delta();
        double factor = qPow(m_zoomFactorBase, angle);
        gentleZoom(factor);
        return true;
      }
    }
  }
  Q_UNUSED(object)
  return false;
}

// ReportDesignIntf
ReportDesignWidget* ReportDesignWidget::m_instance=0;

ReportDesignWidget::ReportDesignWidget(ReportEnginePrivate *report, QMainWindow *mainWindow, QWidget *parent) :
    QWidget(parent), m_mainWindow(mainWindow)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(1,1,1,1);
    m_view = new QGraphicsView(qobject_cast<QWidget*>(this));
    m_view->setBackgroundBrush(QBrush(Qt::gray));
    mainLayout->addWidget(m_view);
    setLayout(mainLayout);

    if (!report) {
        m_report=new ReportEnginePrivate(this);
        m_report->setObjectName("report");
        m_report->appendPage("page1");
    }
    else {
        m_report=report;
        if (!m_report->pageCount()) m_report->appendPage("page1");
    }

    setActivePage(m_report->pageAt(0));
    foreach(QGraphicsItem* item, activePage()->selectedItems()){
        item->setSelected(false);
    }
    connect(m_report,SIGNAL(pagesLoadFinished()),this,SLOT(slotPagesLoadFinished()));
    connect(m_report,SIGNAL(cleared()),this,SIGNAL(cleared()));
    m_view->scale(0.5,0.5);
    m_instance=this;
    //m_view->viewport()->installEventFilter(this);
    m_zoomer = new GraphicsViewZoom(m_view);
}

ReportDesignWidget::~ReportDesignWidget()
{ m_instance=0;}

void ReportDesignWidget::setActivePage(PageDesignIntf *page)
{
    m_view->setScene(page);
    connect(page,SIGNAL(itemInserted(LimeReport::PageDesignIntf*,QPointF,QString)),this,SIGNAL(itemInserted(LimeReport::PageDesignIntf*,QPointF,QString)));
    connect(page,SIGNAL(itemInsertCanceled(QString)),this,SIGNAL(itemInsertCanceled(QString)));
    connect(page,SIGNAL(itemPropertyChanged(QString,QString,QVariant,QVariant)),this,SIGNAL(itemPropertyChanged(QString,QString,QVariant,QVariant)));
    connect(page,SIGNAL(selectionChanged()),this,SLOT(slotSelectionChanged()));
    connect(page,SIGNAL(insertModeStarted()),this,SIGNAL(insertModeStarted()));
    connect(page,SIGNAL(commandHistoryChanged()),this,SIGNAL(commandHistoryChanged()));
    connect(page,SIGNAL(sceneRectChanged(QRectF)),this,SLOT(slotSceneRectChanged(QRectF)));

    connect(page,SIGNAL(itemAdded(LimeReport::PageDesignIntf*,LimeReport::BaseDesignIntf*)),
            this, SIGNAL(itemAdded(LimeReport::PageDesignIntf*,LimeReport::BaseDesignIntf*)));
    connect(page,SIGNAL(itemRemoved(LimeReport::PageDesignIntf*,LimeReport::BaseDesignIntf*)),
            this,SIGNAL(itemDeleted(LimeReport::PageDesignIntf*,LimeReport::BaseDesignIntf*)));
    connect(page,SIGNAL(bandAdded(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)),
            this, SIGNAL(bandAdded(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)));
    connect(page, SIGNAL(bandRemoved(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)),
            this, SIGNAL(bandDeleted(LimeReport::PageDesignIntf*,LimeReport::BandDesignIntf*)));



    m_view->centerOn(0, 0);
    emit activePageChanged();
}

void ReportDesignWidget::createStartPage()
{
    PageDesignIntf* reportPage = m_report->appendPage("page1");
    setActivePage(reportPage);
}

void ReportDesignWidget::removeDatasource(const QString &datasourceName)
{
    m_report->dataManager()->removeDatasource(datasourceName);
}

void ReportDesignWidget::addBand(const QString &bandType)
{
    activePage()->addBand(bandType);
}

void ReportDesignWidget::addBand(BandDesignIntf::BandsType bandType)
{
    activePage()->addBand(bandType);
}

void ReportDesignWidget::startInsertMode(const QString &itemType)
{
    activePage()->startInsertMode(itemType);
}

void ReportDesignWidget::startEditMode()
{
    activePage()->startEditMode();
}

PageDesignIntf * ReportDesignWidget::activePage()
{
    return qobject_cast<PageDesignIntf*>(m_view->scene());
}

QList<QGraphicsItem *> ReportDesignWidget::selectedItems(){
    return m_view->scene()->selectedItems();
}

void ReportDesignWidget::deleteItem(QGraphicsItem *item){
    activePage()->removeReportItem(dynamic_cast<BaseDesignIntf*>(item));
}

void ReportDesignWidget::deleteSelectedItems(){
    activePage()->deleteSelected(true);
}

QStringList ReportDesignWidget::datasourcesNames(){
    return m_report->dataManager()->dataSourceNames();
}

void ReportDesignWidget::slotItemSelected(BaseDesignIntf *item){
    emit itemSelected(item);
}

void ReportDesignWidget::saveToFile(const QString &fileName){
    m_report->saveToFile(fileName);
}

bool ReportDesignWidget::save()
{
    if (!m_report->reportFileName().isEmpty()){
        return m_report->saveToFile();
    }
    else {
        m_report->emitSaveReport();
        if (m_report->isSaved()) return true;
        return m_report->saveToFile(QFileDialog::getSaveFileName(this,tr("Report file name"),"","Report files (*.lrxml);; All files (*)"));
    }
}

void ReportDesignWidget::loadFromFile(const QString &fileName)
{
    m_report->loadFromFile(fileName);    
    setActivePage(m_report->pageAt(0));
}

void ReportDesignWidget::scale(qreal sx, qreal sy)
{
    m_view->scale(sx,sy);
}

QString ReportDesignWidget::reportFileName()
{
    return m_report->reportFileName();
}

bool ReportDesignWidget::isNeedToSave()
{
    return m_report->isNeedToSave();
}

bool ReportDesignWidget::emitLoadReport()
{
    return m_report->emitLoadReport();
}

void ReportDesignWidget::updateSize()
{
    activePage()->slotUpdateItemSize();
}

void ReportDesignWidget::undo()
{
    activePage()->undo();
}

void ReportDesignWidget::redo()
{
    activePage()->redo();
}

void ReportDesignWidget::copy()
{
    activePage()->copy();
}

void ReportDesignWidget::paste()
{
    activePage()->paste();
}

void ReportDesignWidget::cut()
{
    activePage()->cut();
}

void ReportDesignWidget::brinToFront()
{
    activePage()->bringToFront();
}

void ReportDesignWidget::sendToBack()
{
    activePage()->sendToBack();
}

void ReportDesignWidget::alignToLeft()
{
    activePage()->alignToLeft();
}

void ReportDesignWidget::alignToRight()
{
    activePage()->alignToRigth();
}

void ReportDesignWidget::alignToVCenter()
{
    activePage()->alignToVCenter();
}

void ReportDesignWidget::alignToTop()
{
    activePage()->alignToTop();
}

void ReportDesignWidget::alignToBottom()
{
    activePage()->alignToBottom();
}

void ReportDesignWidget::alignToHCenter()
{
    activePage()->alignToHCenter();
}

void ReportDesignWidget::sameHeight()
{
    activePage()->sameHeight();
}

void ReportDesignWidget::sameWidth()
{
    activePage()->sameWidth();
}

void ReportDesignWidget::editLayoutMode(bool value)
{
    if (value ){
        activePage()->pageItem()->setItemMode(activePage()->pageItem()->itemMode() | LayoutEditMode);
    } else if (activePage()->pageItem()->itemMode() & LayoutEditMode){
        activePage()->pageItem()->setItemMode(activePage()->pageItem()->itemMode() ^ LayoutEditMode);
    }
}

void ReportDesignWidget::addHLayout()
{
    activePage()->addHLayout();
}

void ReportDesignWidget::setFont(const QFont& font)
{
    activePage()->setFont(font);
}

void ReportDesignWidget::setTextAlign(const bool& horizontalAlign, const Qt::AlignmentFlag& alignment)
{
    //activePage()->setTextAlign(alignment);
    activePage()->changeSelectedGrpoupTextAlignPropperty(horizontalAlign, alignment);
}

void ReportDesignWidget::setBorders(const BaseDesignIntf::BorderLines& borders)
{
    activePage()->setBorders(borders);
}

bool ReportDesignWidget::isCanUndo()
{
    return activePage()->isCanUndo();
}

bool ReportDesignWidget::isCanRedo()
{
    return activePage()->isCanRedo();
}

void ReportDesignWidget::slotSelectionChanged()
{
    QGraphicsScene* page=dynamic_cast<QGraphicsScene*>(sender());
    if (page){
        if (page->selectedItems().count()==1){
            BaseDesignIntf* item = dynamic_cast<BaseDesignIntf*>(page->selectedItems().at(0));
            if (item)
              emit (itemSelected(item));
        }
        else if (page->selectedItems().count()>1){
            emit (multiItemSelected());
        }
    }
}

DataSourceManager* ReportDesignWidget::dataManager()
{
    return m_report->dataManager();
}

void ReportDesignWidget::slotPagesLoadFinished()
{
    setActivePage(m_report->pageAt(0));
    emit loaded();
}

void ReportDesignWidget::slotDatasourceCollectionLoaded(const QString & /*collectionName*/)
{
}

void ReportDesignWidget::slotSceneRectChanged(QRectF)
{
    m_view->centerOn(0,0);
}

bool ReportDesignWidget::eventFilter(QObject *target, QEvent *event)
{
    if (event->type() == QEvent::Wheel){
        QWheelEvent* we = dynamic_cast<QWheelEvent*>(event);
        if (QApplication::keyboardModifiers()==Qt::ControlModifier){
            if(we->delta()<0) scale(1.2,1.2);
            else scale(1/1.2,1/1.2);
        }
    }
    return QWidget::eventFilter(target,event);
}

void ReportDesignWidget::clear()
{
    m_report->clearReport();
}

}



