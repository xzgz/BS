// Copyright (C) 2016 The Regents of the University of California
//
// Created by David W. Shattuck, Ph.D.
//
// This file is part of BrainSuite16a1.
//
// BrainSuite16a1 is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; version 2.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.
//

#include "delineationtoolboxform.h"
#include "ui_delineationtoolboxform.h"
#include <brainsuitewindow.h>
#include <DS/morph32.h>
#include <displaycode.h>
#include <qevent.h>
#include <imagewidget.h>
#include <thresholdtools.h>
#include <undosystem.h>
#include <marchingcubes_t.h>
#include <waitcursor.h>
#include <slicefill.h>
#include <vol3dhistogram.h>
#include <surfacerecolorchoices.h>
#include <dsglwidget.h>
#include <surfacealpha.h>
#include <surflib.h>
#include <surfacedisplayform.h>
#include <colorops.h>
#include <qcolordialog.h>
#include <volumepainter.h>

bool surfaceRecolor(SurfaceAlpha *surface, const int recolorChoice, BrainSuiteWindow *brainSuiteWindow);
std::string tsvFilter="Tab separated value file (*.tsv)";

// TODO: This toolbox handles multiple tools, and would be better if it each
// tool was a separate class, e.g., a MaskingTool class that would provide thresholds,
// morphology, and other operations.

// move to DS Qt helper class
void applyFilterToTableWidget(QTableWidget *tableWidget, const QString &filter)
{
  if (!tableWidget) return;
  for (int row=0;row<tableWidget->rowCount();row++)
  {
    bool match = false;
    for (int col=0;col<tableWidget->columnCount();col++)
    {
      QTableWidgetItem *item = tableWidget->item(row,col);
      if (item->text().contains(filter))
      {
        match = true;
        break;
      }
    }
    tableWidget->setRowHidden( row, !match );
  }
}

class UndoWrapper {
public:
  UndoSystem<uint8,50>  undoSystem;
};

static inline int labelValue(Vol3DBase *v, const IPoint3D &p)
{
  switch (v->typeID())
  {
    case SILT::Uint8 : return (*(Vol3D<uint8> *)v)(p.x,p.y,p.z); break;
    case SILT::Sint8 : return (*(Vol3D<sint8> *)v)(p.x,p.y,p.z); break;
    case SILT::Uint16: return (*(Vol3D<uint16> *)v)(p.x,p.y,p.z); break;
    case SILT::Sint16: return (*(Vol3D<sint16> *)v)(p.x,p.y,p.z); break;
    case SILT::Uint32: return (*(Vol3D<uint32> *)v)(p.x,p.y,p.z); break;
    case SILT::Sint32: return (*(Vol3D<sint32> *)v)(p.x,p.y,p.z); break;
    default: return 0;
  }
  return 0;
}

DelineationToolboxForm::DelineationToolboxForm(BrainSuiteWindow *brainSuiteWindow, QWidget *parent) :
  QWidget(parent),
  brushXY(Orientation::XY),
  brushXZ(Orientation::XZ),
  brushYZ(Orientation::YZ),
  ui(new Ui::DelineationToolboxForm),
  brainSuiteWindow(brainSuiteWindow),
  currentLabelID(0), lockCombobox(false),
  paintedSomething(false),  sortColumn(-1), sortAscending(true)
{
  ui->setupUi(this);
  maskToolundoSystem = new UndoWrapper;
  ui->brushSizeSpinBox->setValue(3);
  ui->lowerCondtionalThresholdEditBox->setText("0");
  ui->upperCondtionalThresholdEditBox->setText("10000");
  ui->thresholdSlider->setRange(0,255);
  ui->thresholdSlider->setValue(0);
  ui->autoUpdateCheckbox->setChecked(true);
  ui->redoButton->setVisible(false);
  ui->thresholdEditBox->setText("0");
  updateUndoButton();
  ui->loadMaskButton->setText("");
  ui->loadMaskButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->saveMaskButton->setText("");
  ui->saveMaskButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->loadLabelVolumeButton->setText("");
  ui->loadLabelVolumeButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->saveLabelVolumeButton->setText("");
  ui->saveLabelVolumeButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->maskModeComboBox->insertItem(DrawEverywhere,"draw everywhere");
  ui->maskModeComboBox->insertItem(DrawInsideMask,"draw inside masked area");
  ui->maskModeComboBox->insertItem(DrawOutsideMask,"draw outside masked area");
  {
    ui->labelTableWidget->setColumnCount(4);
    ui->labelTableWidget->setShowGrid(false);
    QStringList headers;
    headers.push_back("ID");
    headers.push_back("description");
    headers.push_back("voxels");
    headers.push_back("volume (cc)");
    ui->labelTableWidget->setHorizontalHeaderLabels(headers);
    ui->labelTableWidget->setColumnWidth(0,40);
    ui->labelTableWidget->setColumnWidth(2,75);
    ui->labelTableWidget->setColumnWidth(3,75);
    ui->labelTableWidget->verticalHeader()->hide();
    connect(ui->labelTableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)),    this, SLOT(slot_labelTableHeaderClicked(int)));
  }
  {
    ui->labelMaskTableWidget->setColumnCount(4);
    ui->labelMaskTableWidget->setShowGrid(false);
    QStringList headers;
    headers.push_back("ID");
    headers.push_back("description");
    headers.push_back("voxels");
    headers.push_back("volume (cc)");
    ui->labelMaskTableWidget->setHorizontalHeaderLabels(headers);
    ui->labelMaskTableWidget->setColumnWidth(0,40);
    ui->labelMaskTableWidget->setColumnWidth(2,75);
    ui->labelMaskTableWidget->setColumnWidth(3,75);
    ui->labelMaskTableWidget->verticalHeader()->hide();
    connect(ui->labelMaskTableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)),    this, SLOT(slot_labelMaskTableHeaderClicked(int)));
  }
  ui->labelBrushSizeSpinBox->setValue(3); // TODO: move to default value class
  for (size_t i=0;i<SurfaceRecolorChoices::nNames;i++)
  {
    ui->recolorChoiceComboBox->insertItem((int)i,SurfaceRecolorChoices::names[i],QVariant((unsigned int)i));  // using int because size_t is ambiguous for QVariant
  }
  updateLabelSelectionComboBox();
  chooseLabel(1);
}

DelineationToolboxForm::~DelineationToolboxForm()
{
  delete ui;
}

MaskStates DelineationToolboxForm::getMaskState() { return (MaskStates)ui->maskModeComboBox->currentIndex(); }

void DelineationToolboxForm::slot_labelTableHeaderClicked(int columnClicked)
{
  if (sortColumn<0)
  {
    sortColumn=columnClicked;
  }
  else
  {
    if (sortColumn==columnClicked) sortAscending^=true;
  }
  sortColumn=columnClicked;
  ui->labelTableWidget->sortItems(columnClicked,sortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
  for (auto i=0;i<4;i++)
  {
    auto *item = ui->labelTableWidget->horizontalHeaderItem(i);
    if (i==columnClicked)
      item->setIcon(style()->standardIcon(sortAscending ? QStyle::SP_ArrowDown : QStyle::SP_ArrowUp));
    else
      item->setIcon(QIcon());
  }
}

void DelineationToolboxForm::slot_labelMaskTableHeaderClicked(int clicky)
{
  ui->labelMaskTableWidget->sortItems(clicky);
}

void DelineationToolboxForm::setThresholdRange(float minv, float maxv, int nlevels)
{
  ui->thresholdSlider->setFloatRange(minv,maxv,nlevels);
}

void DelineationToolboxForm::setThresholdRange(int /*minv*/, int maxv)
{
  ui->thresholdSlider->setIntRange(0,maxv);
}

float DelineationToolboxForm::getThreshold()
{
  return ui->thresholdSlider->valuef();
}

QSize DelineationToolboxForm::sizeHint() const
{
  return QSize(300,560);
}

bool DelineationToolboxForm::isEditingMask()
{
  return (isVisible() && ui->editMaskButton->isChecked());
}

bool DelineationToolboxForm::isEditingLabels()
{
  return (isVisible() && ui->editLabelButton->isChecked());
}

void DelineationToolboxForm::disableEdits()
{
  ui->editMaskButton->setChecked(false);
}

bool DelineationToolboxForm::alwaysEditMask()
{
  return (isVisible() && ui->alwaysDrawButton->isChecked());
}

bool DelineationToolboxForm::alwaysEditLabels()
{
  return (isVisible() && ui->alwaysDrawLabelButton->isChecked());
}

void DelineationToolboxForm::on_loadMaskButton_clicked()
{
  brainSuiteWindow->loadMaskVolume();
}

bool DelineationToolboxForm::toggleEditState()
{
  switch (ui->toolBox->currentIndex())
  {
    case 0:
      ui->editMaskButton->setChecked(!ui->editMaskButton->isChecked());
      on_editMaskButton_clicked();
      break;
    case 1:
      ui->editLabelButton->setChecked(!ui->editLabelButton->isChecked());
      on_editLabelButton_clicked();
      break;
  }
  return true;
}

EditDSBitmap *DelineationToolboxForm::editBitmap(Orientation::Code orientation)
{
  switch (ui->toolBox->currentIndex())
  {
    case 0 :
      if (!isEditingMask()) return 0;
      break;
    case 1 :
      if (!isEditingLabels()) return 0;
      break;
    default: break;
  }

  switch (orientation)
  {
    case Orientation::XY : return &brushXY.bitmap;
    case Orientation::XZ : return &brushXZ.bitmap;
    case Orientation::YZ : return &brushYZ.bitmap;
    default: break;
  }
  return 0;
}

void DelineationToolboxForm::mousePressed(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  mouseMoveTo(port,position,event);
}

template <class T>
inline void volPaintT(Vol3D<T> &v, const IPoint3D point, LabelBrush &brush, const int brushRadius, const int labelID, bool use3D)
{
  brush.resize(brushRadius,use3D);
  IPoint3D *b = &brush.points[0];
  const size_t n = brush.points.size();
  const int cx = v.cx;
  const int cy = v.cy;
  const int cz = v.cz;
  for (size_t i=0;i<n;i++)
  {
    const IPoint3D p(point.x+b[i].x,point.y+b[i].y,point.z+b[i].z);
    if ((p.x>=0)&&(p.x<cx)&&(p.y>=0)&&(p.y<cy)&&(p.z>=0)&&(p.z<cz))
      v(p.x,p.y,p.z)=labelID;
  }
}

void DelineationToolboxForm::updateUndoButton()
{
  ui->undoButton->setEnabled(maskToolundoSystem->undoSystem.current()!=0);
}

bool DelineationToolboxForm::setUndoPoint(IPoint3D &point, std::string description)
{
  if (BrainSuiteSettings::maskUndoDisabled) return false;
  ::WaitCursor wc(this);
  bool flag=maskToolundoSystem->undoSystem.set(brainSuiteWindow->brainSuiteDataManager.vMask,point,description);
  if (flag)
  {
    brainSuiteWindow->setMaskUndoDescription(description);
  }
  updateUndoButton();
  return flag;
}

bool DelineationToolboxForm::setUndoPoint(std::string description)
{
  return setUndoPoint(brainSuiteWindow->currentVolumePosition,description);
}

void DelineationToolboxForm::mouseMoveTo(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  switch (ui->toolBox->currentIndex())
  {
    case 0: maskToolMouseMoveTo(port, position, event); break;
    case 1: labelPainterMouseMoveTo(port, position, event); break;
    default: break;
  }
}

void DelineationToolboxForm::maskToolMouseMoveTo(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  if ((event && event->modifiers() & Qt::ControlModifier) || alwaysEditMask())
  {
    if (brainSuiteWindow->brainSuiteDataManager.vMask.size())
    {
      if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
      {
        int labelID = (event->buttons() & Qt::LeftButton) ? 255 : 0;
        if (port)
        {
          IPoint3D paintPoint = position;
          int n = ui->brushSizeSpinBox->text().toInt();
          bool use3D = ui->use3DBrushCheckBox->isChecked();
          if (event->type()==QMouseEvent::MouseButtonPress) // only set the point if it was a mouse click
          {
            setUndoPoint(paintPoint,"mask edit");
          }
          switch (port->orientation)
          {
            case Orientation::XY :
              volPaintT(brainSuiteWindow->brainSuiteDataManager.vMask,paintPoint,brushXY,n,labelID,use3D);
              break;
            case Orientation::XZ :
              volPaintT(brainSuiteWindow->brainSuiteDataManager.vMask,paintPoint,brushXZ,n,labelID,use3D);
              break;
            case Orientation::YZ :
              volPaintT(brainSuiteWindow->brainSuiteDataManager.vMask,paintPoint,brushYZ,n,labelID,use3D);
              break;
            case Orientation::Oblique :
              break;
          }
        }
        brainSuiteWindow->updateImages();
      }
      else if (event->buttons() & Qt::RightButton)
      {
        brainSuiteWindow->brainSuiteDataManager.vMask(position)=0;
        brainSuiteWindow->updateImages();
      }
    }
  }
}

template <class PaintStrategy>
void volumePaintermaskedDT(Vol3DBase *vol, Vol3D<uint8> &vMask, const IPoint3D point, LabelBrush &brush, const int brushRadius, bool brush3D,
                             const int labelID, const MaskStates maskState)
{
  if (!vol) return;
  PaintStrategy pm;
  switch (vol->typeID())
  {
    case SILT::Sint8  : pm(*(Vol3D<sint8> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    case SILT::Uint8  : pm(*(Vol3D<uint8> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    case SILT::Sint16 : pm(*(Vol3D<sint16> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    case SILT::Uint16 : pm(*(Vol3D<uint16> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    case SILT::Sint32 : pm(*(Vol3D<sint32> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    case SILT::Uint32 : pm(*(Vol3D<uint32> *)vol,vMask,point,brush,brushRadius,brush3D,labelID,maskState); break;
    default: break;
  }
}


bool DelineationToolboxForm::newVolume()
{

  switch (ui->toolBox->currentIndex())
  {
    case 0:
      break;
    case 1:
      on_newLabelVolumeButton_clicked();
      return true;
      break;
  }
  return false;
}

void DelineationToolboxForm::labelPainterMouseMoveTo(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  if (event && (event->modifiers() & Qt::ControlModifier || alwaysEditLabels()))
  {
    if (brainSuiteWindow->brainSuiteDataManager.vLabel)
    {
      if (event->buttons() & (Qt::LeftButton|Qt::RightButton))
      {
        if (port)
        {
          int labelID=currentLabel();
          int paintLabelID = labelID;
          MaskStates maskState = getMaskState();
          Vol3DBase *labelVolume = brainSuiteWindow->brainSuiteDataManager.vLabel;
          Vol3D<uint8> &maskVolume = brainSuiteWindow->brainSuiteDataManager.vMask;
          bool clobber = ui->overwriteCurrentLabelsCheckBox->isChecked();
          IPoint3D paintPoint = position;
          int n = ui->labelBrushSizeSpinBox->text().toInt();
          bool use3D = ui->use3DLabelBrushCheckBox->isChecked();
          LabelBrush *brush = 0;
          switch (port->orientation)
          {
            case Orientation::XY :
              brush = &brushXY;
              break;
            case Orientation::XZ :
              brush = &brushXZ;
              break;
            case Orientation::YZ :
              brush = &brushYZ;
              break;
            case Orientation::Oblique :
              break;
          }
          if (brush)
          {
            paintedSomething=true;
            if (clobber)
            {
              if (event->buttons() & Qt::LeftButton)
                volumePaintermaskedDT<PaintMasked>(labelVolume,maskVolume,paintPoint,*brush,n,use3D,paintLabelID,maskState);
              else
                volumePaintermaskedDT<PaintMasked>(labelVolume,maskVolume,paintPoint,*brush,n,use3D,0,maskState);
            }
            else
            {
              if (event->buttons() & Qt::LeftButton)
                volumePaintermaskedDT<PaintMaskedNoClobber>(labelVolume,maskVolume,paintPoint,*brush,n,use3D,paintLabelID,maskState);
              else
              {
                volumePaintermaskedDT<PaintMaskedNoClobberErase>(labelVolume,maskVolume,paintPoint,*brush,n,use3D,paintLabelID,maskState);
              }
            }
          }
        }
        brainSuiteWindow->updateImages();
      }
    }
  }
}

void DelineationToolboxForm::mouseReleased(ImageWidget *port, const IPoint3D &position, QMouseEvent *event)
{
  mouseMoveTo(port,position,event);
}

void DelineationToolboxForm::on_dilateCubeButton_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vMask.size())
  {
    Morph32 m32;
    if (ui->conditionalDilationCheckBox->isChecked())
    {
      setUndoPoint("conditional dilate cube");
      Vol3D<VBit> mask;
      mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
      Vol3D<VBit> maskInside;
      maskInside.copy(mask);
      m32.dilateC(mask);
      setDiff(mask,maskInside);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
      const double thresholdValueMin = ui->lowerCondtionalThresholdEditBox->text().toDouble();
      const double thresholdValueMax = ui->upperCondtionalThresholdEditBox->text().toDouble();
      Vol3D<uint8> vOutline;
      mask.decode(vOutline);
      ThresholdTools::conditionalThreshold(vOutline,brainSuiteWindow->brainSuiteDataManager.volume,thresholdValueMin,thresholdValueMax);
      mask.encode(vOutline);
      opOr(mask,maskInside);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    }
    else
    {
      setUndoPoint("dilate cube");
      Vol3D<VBit> mask;
      mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
      m32.dilateC(mask);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    }
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_dilateDiamondButton_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vMask.size())
  {
    Morph32 m32;
    if (ui->conditionalDilationCheckBox->isChecked())
    {
      setUndoPoint("conditional dilate diamond");
      Vol3D<VBit> mask;
      mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
      Vol3D<VBit> maskInside;
      maskInside.copy(mask);
      m32.dilateR(mask);
      setDiff(mask,maskInside);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
      const double thresholdValueMin = ui->lowerCondtionalThresholdEditBox->text().toDouble();
      const double thresholdValueMax = ui->upperCondtionalThresholdEditBox->text().toDouble();
      Vol3D<uint8> vOutline;
      mask.decode(vOutline);
      ThresholdTools::conditionalThreshold(vOutline,brainSuiteWindow->brainSuiteDataManager.volume,thresholdValueMin,thresholdValueMax);
      mask.encode(vOutline);
      opOr(mask,maskInside);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    }
    else
    {
      setUndoPoint("dilate diamond");
      Vol3D<VBit> mask;
      mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
      m32.dilateR(mask);
      mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    }
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_erodeCubeButton_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vMask.size())
  {
    setUndoPoint("erode cube");
    Vol3D<VBit> mask;
    mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
    Morph32 m32;
    m32.erodeC(mask);
    mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_erodeDiamondButton_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.vMask.size())
  {
    setUndoPoint("erode diamond");
    Vol3D<VBit> mask;
    mask.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
    Morph32 m32;
    m32.erodeR(mask);
    mask.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_brushSizeSpinBox_valueChanged(int /*arg1*/)
{
  on_editMaskButton_clicked();
}

void DelineationToolboxForm::on_use3DBrushCheckBox_clicked()
{

}

void DelineationToolboxForm::on_saveMaskButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->saveMaskVolume();
}

void DelineationToolboxForm::on_thresholdEditBox_textEdited(const QString &/*arg1*/)
{
  const double thresholdValue = ui->thresholdEditBox->text().toDouble();
  ui->thresholdSlider->blockSignals(true);
  ui->thresholdSlider->setFloatValue(thresholdValue);
  ui->thresholdSlider->blockSignals(false);
  thresholdIfAutomatic();
}

void DelineationToolboxForm::thresholdIfAutomatic()
{
  if (ui->autoUpdateCheckbox->isChecked())
  {
    threshold(true);
  }
}

void DelineationToolboxForm::on_autoUpdateCheckbox_clicked()
{
  thresholdIfAutomatic();
}

bool DelineationToolboxForm::threshold(bool saveUndoPoint)
{
  if (!brainSuiteWindow) return false;
  Vol3DBase *vol = brainSuiteWindow->brainSuiteDataManager.volume;
  if (!vol) return false;
  Vol3D<uint8> &mask = brainSuiteWindow->brainSuiteDataManager.vMask;
  const double thresholdValue = ui->thresholdEditBox->text().toDouble();
  if (mask.start()==0)
  {
    mask.makeCompatible(*vol);
  }
  else
  {
    if (saveUndoPoint)
      setUndoPoint("threshold");
  }
  if (mask.start()!=0)
  {
    ThresholdTools::threshold(mask,vol,thresholdValue);
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
  return true;
}

void DelineationToolboxForm::on_thresholdButton_clicked()
{
  threshold(true);
}

void DelineationToolboxForm::on_thresholdSlider_valueChanged(int /*value*/)
{
  float f = ui->thresholdSlider->valuef();
  ui->thresholdEditBox->setText(QString::number(f));
  if (ui->autoUpdateCheckbox->isChecked())
  {
    threshold(false);
  }
}

void DelineationToolboxForm::on_invertButton_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vMask.size()) return;

  Vol3D<uint8> &mask = brainSuiteWindow->brainSuiteDataManager.vMask;
  setUndoPoint("invert mask");
  if (mask.start()==0) return;
  const int ds = mask.size();
  for (int i=0;i<ds;i++) mask[i] = 255 - mask[i];
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void DelineationToolboxForm::on_applyMasktoImage_clicked()
{
  if (!brainSuiteWindow||!brainSuiteWindow->brainSuiteDataManager.volume) return;
  brainSuiteWindow->brainSuiteDataManager.volume->maskWith( brainSuiteWindow->brainSuiteDataManager.vMask);
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void DelineationToolboxForm::undoLastAction()
{
  if (!maskToolundoSystem->undoSystem.current()) return;
  IPoint3D p = maskToolundoSystem->undoSystem.current()->point;
  if (maskToolundoSystem->undoSystem.undo(brainSuiteWindow->brainSuiteDataManager.vMask))
  {
    brainSuiteWindow->currentVolumePosition = p;
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_fillBackgroundButton_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vMask.size()) return;
  setUndoPoint("fill background");
  Vol3D<VBit> vb;
  RunLengthSegmenter rls;
  vb.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
  rls.segmentBG(vb);
  vb.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void DelineationToolboxForm::on_selectForeground_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vMask.size()) return;
  setUndoPoint("select foreground");
  Vol3D<VBit> vb;
  RunLengthSegmenter rls;
  vb.encode(brainSuiteWindow->brainSuiteDataManager.vMask);
  rls.segmentFG(vb);
  vb.decode(brainSuiteWindow->brainSuiteDataManager.vMask);
  brainSuiteWindow->updateView(DisplayCode::Dirty|DisplayCode::MaskOn);
}

void DelineationToolboxForm::updateEditBitmaps()
{
  switch (ui->toolBox->currentIndex())
  {
    case 0:
      if (ui->editMaskButton->isChecked())
      {
        int brushSize = ui->brushSizeSpinBox->text().toInt();
        bool use3D = ui->use3DBrushCheckBox->isChecked();
        brushXY.resize(brushSize,use3D);
        brushXZ.resize(brushSize,use3D);
        brushYZ.resize(brushSize,use3D);
      }
      if (brainSuiteWindow) brainSuiteWindow->updateImages();
      break;
    case 1:
      if (ui->editLabelButton->isChecked())
      {
        int brushSize = ui->labelBrushSizeSpinBox->text().toInt();
        bool use3D = ui->use3DLabelBrushCheckBox->isChecked();
        brushXY.resize(brushSize,use3D);
        brushXZ.resize(brushSize,use3D);
        brushYZ.resize(brushSize,use3D);
      }
      if (brainSuiteWindow) brainSuiteWindow->updateImages();
      break;
  }
}

void DelineationToolboxForm::on_editMaskButton_clicked()
{
  updateEditBitmaps();
}

void DelineationToolboxForm::on_undoButton_clicked()
{
  undoLastAction();
  updateUndoButton();
}

void DelineationToolboxForm::on_redoButton_clicked()
{
  if (maskToolundoSystem->undoSystem.redo(brainSuiteWindow->brainSuiteDataManager.vMask))
  {
    IPoint3D p = maskToolundoSystem->undoSystem.current()->point;
    brainSuiteWindow->currentVolumePosition = p;
    brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
  }
}

void DelineationToolboxForm::on_thresholdSlider_sliderReleased()
{

}

void DelineationToolboxForm::on_thresholdSlider_sliderPressed()
{
  if (ui->autoUpdateCheckbox->isChecked())
    threshold(true);
}

void DelineationToolboxForm::on_quickSurfaceButton_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vMask.size()) return;
  {
    MarchingCubes tessellator;
    WaitCursor waitCursor(this);
    SurfaceAlpha *maskSurface=new SurfaceAlpha;
    if (!maskSurface) return;
    int n = 10;
    float alpha = 0.5f;
    float c = 5.0f;
    tessellator.cubes(brainSuiteWindow->brainSuiteDataManager.vMask,*maskSurface);
    maskSurface->computeNormals();
    SurfLib::computeCurvature(*(maskSurface));
    SurfLib::laplaceSmoothCurvatureWeighted(*(maskSurface),n,alpha,c);
    maskSurface->computeNormals();
    maskSurface->filename = "[new surface]";
    QVariant v=ui->recolorChoiceComboBox->itemData(ui->recolorChoiceComboBox->currentIndex());
    bool ok=false;
    int recolorChoice=v.toInt(&ok);
    if (ok)
    {
      surfaceRecolor(maskSurface, recolorChoice, brainSuiteWindow);
    }
    brainSuiteWindow->addSurface(maskSurface,true);
  }
}

int increaseSpinBox(QSpinBox *spinbox)
{
  int n = spinbox->text().toInt();
  if (n>=25) { n+=5; if (n%5) n -= (n%5); }
  else if (n>=0) n++;
  else n=0;
  spinbox->setValue(n);
  return n;
}

int decreaseSpinBox(QSpinBox *spinbox)
{
  int n = spinbox->text().toInt();
  if (n>25) { n=(n%5) ? n-(n%5) :  n-5; }
  else if (n>0) n--;
  else n=0;
  spinbox->setValue(n);
  return n;
}

void DelineationToolboxForm::decreaseBrushSize()
{
  switch (ui->toolBox->currentIndex())
  {
    case 0 : decreaseSpinBox(ui->brushSizeSpinBox); break;
    case 1 : decreaseSpinBox(ui->labelBrushSizeSpinBox); break;
    default: return;
  }
  brainSuiteWindow->updateView(DisplayCode::RedrawView);
}

void DelineationToolboxForm::increaseBrushSize()
{
  switch (ui->toolBox->currentIndex())
  {
    case 0 : increaseSpinBox(ui->brushSizeSpinBox); break;
    case 1 : increaseSpinBox(ui->labelBrushSizeSpinBox); break;
    default: return;
  }
  brainSuiteWindow->updateView(DisplayCode::RedrawView);
}

void DelineationToolboxForm::showMaskTool()
{
  ui->toolBox->setCurrentIndex(0);
}

void DelineationToolboxForm::showPainterTool()
{
  ui->toolBox->setCurrentIndex(1);
}

void DelineationToolboxForm::on_editLabelButton_clicked()
{
  updateEditBitmaps();
}

void DelineationToolboxForm::on_labelBrushSizeSpinBox_valueChanged(int )
{
  on_editLabelButton_clicked();
}

void DelineationToolboxForm::on_labelSelectionComboBox_currentIndexChanged(int index)
{
  if (lockCombobox==false)
    chooseLabel(ui->labelSelectionComboBox->itemData(index).toInt());
}

inline bool sameDimensions(Vol3D<uint8> &v1, Vol3DBase *vp)
{
  return (vp->cx==v1.cx)&&(vp->cy==v1.cy)&&(vp->cz==v1.cz);
}

void makeReadOnly(QTableWidgetItem *item)
{
  if (item)
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
}

void DelineationToolboxForm::on_computeVolumesPushButton_clicked()
{
  if (!brainSuiteWindow) return;
  Vol3DBase *vLabel=brainSuiteWindow->brainSuiteDataManager.vLabel;
  Vol3D<uint8> &vMask=brainSuiteWindow->brainSuiteDataManager.vMask;
  Vol3DHistogram hist;
  if (!vLabel) return;
  bool success=false;
  if (sameDimensions(vMask,vLabel)&&brainSuiteWindow->imageState.viewMask)
  {
    success=hist.compute(vLabel);
  }
  else
  {
    success=hist.compute(vLabel);
  }
  if (!success)
  {
    std::string info="data type " + vLabel->datatypeName() + " is not supported for this operation";
    std::cerr<<info<<std::endl;
    return;
  }
  float voxsize = vLabel->rx * vLabel->ry * vLabel->rz/1000.0f;
  int nItems = 0;
  for (int i=0;i<=hist.tmax;i++)
  {
    const int count = hist.count[i-hist.tmin];
    if (count>0)
    {
      ui->labelTableWidget->setRowCount(nItems+1);
      ui->labelTableWidget->setItem(nItems, 0, new QTableWidgetItem());
      ui->labelTableWidget->item(nItems,0)->setData(Qt::EditRole,i);
      ui->labelTableWidget->setItem(nItems, 1, new QTableWidgetItem(brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].descriptor.c_str()));
      ui->labelTableWidget->setItem(nItems, 2, new QTableWidgetItem());
      ui->labelTableWidget->item(nItems,2)->setData(Qt::EditRole,count);
      ui->labelTableWidget->setItem(nItems, 3, new QTableWidgetItem());
      ui->labelTableWidget->item(nItems,3)->setData(Qt::EditRole,count*voxsize);
      for (int i=0;i<4;i++) makeReadOnly(ui->labelTableWidget->item(nItems,i));
      nItems++;
    }
  }
  QString filter=ui->roiFilterLineEdit->text();
  if (filter.isEmpty()==false)
    ::applyFilterToTableWidget(ui->labelTableWidget,filter);
}

void DelineationToolboxForm::on_colorSelectionButton_clicked()
{
  QColor c=QColorDialog::getColor(ui->colorSelectionButton->getColor(),this,"New Label Color");
  if (!c.isValid()) return;
  ui->colorSelectionButton->setColor(c);
  QRgb rgb = c.rgb()&RGB_MASK;
  LabelOp::colorTable[currentLabel()] = rgb;
  brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[currentLabel()].color = rgb;
  if (brainSuiteWindow)
    brainSuiteWindow->updateImages();
}

bool DelineationToolboxForm::incrementLabel() { return chooseLabel(currentLabel()+1); }
bool DelineationToolboxForm::decrementLabel() { return chooseLabel(currentLabel()-1); }

bool DelineationToolboxForm::chooseLabel(const int labelID)
{
  currentLabelID=labelID;
  if (currentLabelID<0) currentLabelID = 0;
  if (currentLabelID>=(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
    currentLabelID=(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size()-1;
  ui->labelIDEditBox->setText(QString::number(currentLabelID));
  LabelDetails &details(brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[currentLabelID]);
  ui->labelDescriptionEditBox->setText(details.descriptor.c_str()); // 	std::string descriptor;
  ui->labelAbbrevEditBox->setText(details.tag.c_str());
  lockCombobox=true;
  ui->labelSelectionComboBox->setCurrentIndex(ui->labelSelectionComboBox->findData(labelID));
  lockCombobox=false;
  QRgb rgb(details.color);
  QColor c(rgb);
  ui->colorSelectionButton->setColor(c);
  return true;
}

void DelineationToolboxForm::updateColorSelectionButton()
{
  if (!brainSuiteWindow) return;
  int CL = currentLabel();
  if (CL>=0 && CL < (int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
  {
    QColor color=brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[CL].color;
    ui->colorSelectionButton->setColor(color);
  }
}

void DelineationToolboxForm::updateLabelSelectionComboBox()
{
  lockCombobox=true;
  ui->labelSelectionComboBox->clear();
  for (size_t i=0,id=0;i<brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size();i++)
  {
    if (brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].descriptor.empty()) continue;
    std::ostringstream ostr;
    ostr<<i<<" - "<<brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].descriptor;
    ui->labelSelectionComboBox->insertItem((int)id++,ostr.str().c_str(),QVariant((int)i));
  }
  ui->labelSelectionComboBox->setCurrentIndex(ui->labelSelectionComboBox->findData(currentLabel()));
  lockCombobox=false;
}

class VolumeLabelFill3D {
public:
  Vol3D<uint8> fillmask;
  Vol3D<VBit> fillmaskBit;
  RunLengthSegmenter fillRLS;
  template <class T>
  void volfill3D(Vol3D<T> &volume, IPoint3D point, const int labelID)
  {
    T label = volume(point.x,point.y,point.z);
    fillmask.makeCompatible(volume);
    const int ds = volume.size();
    for (int i=0;i<ds;i++)
      fillmask[i] = (volume[i]==label) ? 255 : 0;
    fillmaskBit.encode(fillmask);
    fillRLS.segmentFG(fillmaskBit);
    int currentLabelID = fillRLS.labelID(point.x,point.y,point.z);
    if (currentLabelID>=0)
    {
      RegionInfo *ri = &fillRLS.regionInfo[0];
      const size_t nr = fillRLS.regionInfo.size();
      for (size_t i=0;i<nr;i++)
      {
        ri[i].selected = (ri[i].label==currentLabelID) ? 1 : 0;
      }
      fillRLS.label32FG(fillmaskBit);
      fillmaskBit.decode(fillmask);
      for (int i=0;i<ds;i++) if (fillmask[i]) volume[i] = labelID;
    }
  }
  template <class T>
  void volfill3Dmasked(Vol3D<T> &volume, Vol3D<uint8> &vMask, IPoint3D point, const int labelID, bool invertMask)
  {
    if (vMask.isCompatible(volume)==false)
    {
      volfill3D(volume, point, labelID);
      return;
    }
    T label = volume(point.x,point.y,point.z);
    fillmask.makeCompatible(volume);
    const int ds = volume.size();
    if (invertMask)
    {
      for (int i=0;i<ds;i++)
        fillmask[i] = ((volume[i]==label) && (vMask[i]==0)) ? 255 : 0;
    }
    else
    {
      for (int i=0;i<ds;i++)
        fillmask[i] = (volume[i]==label) ? vMask[i] : 0;
    }
    fillmaskBit.encode(fillmask);
    fillRLS.segmentFG(fillmaskBit);
    int currentLabelID = fillRLS.labelID(point.x,point.y,point.z);
    if (currentLabelID>=0)
    {
      RegionInfo *ri = &fillRLS.regionInfo[0];
      const size_t nr = fillRLS.regionInfo.size();
      for (size_t i=0;i<nr;i++)
      {
        ri[i].selected = (ri[i].label==currentLabelID) ? 1 : 0;
      }
      fillRLS.label32FG(fillmaskBit);
      fillmaskBit.decode(fillmask);
      for (int i=0;i<ds;i++) if (fillmask[i]) volume[i] = labelID;
    }
  }
};

void DelineationToolboxForm::on_fill3DButton_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vLabel) return;
  if (!brainSuiteWindow->activeImageWidget()) return;
  Vol3D<uint8> &vMask = brainSuiteWindow->brainSuiteDataManager.vMask;
  Vol3DBase *vLabel = brainSuiteWindow->brainSuiteDataManager.vLabel;
  MaskStates maskMode = (MaskStates)ui->maskModeComboBox->currentIndex();
  if (vMask.size()==0) maskMode = DrawEverywhere;
  IPoint3D point = brainSuiteWindow->currentVolumePosition;
  int labelID = currentLabel();
  bool changed = true;
  VolumeLabelFill3D filler;
  switch (maskMode)
  {
    case DrawEverywhere:
      switch (vLabel->typeID())
      {
        case SILT::Sint16 : filler.volfill3D(*(Vol3D<uint16>*)vLabel,point,labelID); break;
        case SILT::Uint16 : filler.volfill3D(*(Vol3D<uint16>*)vLabel,point,labelID); break;
        case SILT::Uint8  : filler.volfill3D(*(Vol3D<uint8 >*)vLabel,point,labelID); break;
        default:
          changed = false;
      }
      break;
    case DrawInsideMask:
      switch (vLabel->typeID())
      {
        case SILT::Sint16 : filler.volfill3Dmasked(*(Vol3D<uint16>*)vLabel,vMask,point,labelID,false); break;
        case SILT::Uint16 : filler.volfill3Dmasked(*(Vol3D<uint16>*)vLabel,vMask,point,labelID,false); break;
        case SILT::Uint8  : filler.volfill3Dmasked(*(Vol3D<uint8 >*)vLabel,vMask,point,labelID,false); break;
        default:
          changed = false;
      }
      break;
    case DrawOutsideMask:
      switch (vLabel->typeID())
      {
        case SILT::Sint16 : filler.volfill3Dmasked(*(Vol3D<uint16>*)vLabel,vMask,point,labelID,true); break;
        case SILT::Uint16 : filler.volfill3Dmasked(*(Vol3D<uint16>*)vLabel,vMask,point,labelID,true); break;
        case SILT::Uint8  : filler.volfill3Dmasked(*(Vol3D<uint8 >*)vLabel,vMask,point,labelID,true); break;
        default:
          changed = false;
      }
      break;
  }
  if (changed) brainSuiteWindow->updateView(DisplayCode::ShowLabels||DisplayCode::Dirty);
}


void DelineationToolboxForm::on_fill2DButton_clicked()
{
  if (!brainSuiteWindow) return;
  if (!brainSuiteWindow->brainSuiteDataManager.vLabel) return;
  if (!brainSuiteWindow->activeImageWidget()) return;
  Vol3D<uint8> &vMask = brainSuiteWindow->brainSuiteDataManager.vMask;
  Vol3DBase *vLabel = brainSuiteWindow->brainSuiteDataManager.vLabel;
  MaskStates maskMode = (MaskStates)ui->maskModeComboBox->currentIndex();
  if (vMask.size()==0) maskMode = DrawEverywhere;
  Orientation::Code orientation = brainSuiteWindow->activeImageWidget()->orientation;
  IPoint3D point = brainSuiteWindow->currentVolumePosition;
  int labelID = currentLabel();
  SliceFill sliceFill;
  bool changed=true;
  switch (maskMode)
  {
    case DrawInsideMask :
      {
        switch (orientation)
        {
          case Orientation::XY : sliceFill.floodFillMaskedXY(vLabel,vMask,point,labelID,false); break;
          case Orientation::XZ : sliceFill.floodFillMaskedXZ(vLabel,vMask,point,labelID,false); break;
          case Orientation::YZ : sliceFill.floodFillMaskedYZ(vLabel,vMask,point,labelID,false); break;
          default: changed=false;
        }
      }
      break;
    case DrawOutsideMask :
      {
        switch (orientation)
        {
          case Orientation::XY : sliceFill.floodFillMaskedXY(vLabel,vMask,point,labelID,true); break;
          case Orientation::XZ : sliceFill.floodFillMaskedXZ(vLabel,vMask,point,labelID,true); break;
          case Orientation::YZ : sliceFill.floodFillMaskedYZ(vLabel,vMask,point,labelID,true); break;
          default: changed=false;
        }
      }
    default:
      {
        switch (orientation)
        {
          case Orientation::XY : sliceFill.floodFillXY(vLabel,point,labelID); break;
          case Orientation::XZ : sliceFill.floodFillXZ(vLabel,point,labelID); break;
          case Orientation::YZ : sliceFill.floodFillYZ(vLabel,point,labelID); break;
          default: changed=false;
        }
      }
      break;
  }
  if (changed) brainSuiteWindow->updateView(DisplayCode::ShowLabels||DisplayCode::Dirty);
}

void DelineationToolboxForm::on_loadLabelDescriptionsButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadLabelDescriptionFile();
}

void DelineationToolboxForm::on_saveLabelDescriptions_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->saveLabelDescriptionFile();
}

void DelineationToolboxForm::on_newLabelVolumeButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->newLabelVolume();
}

void DelineationToolboxForm::on_loadLabelVolumeButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadLabelVolume();
}

void DelineationToolboxForm::on_saveLabelVolumeButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->saveLabelVolume();
}

void DelineationToolboxForm::on_eyedropButton_clicked()
{
  if (brainSuiteWindow)
  {
    if (brainSuiteWindow->brainSuiteDataManager.vLabel)
      chooseLabel(labelValue(brainSuiteWindow->brainSuiteDataManager.vLabel,brainSuiteWindow->currentVolumePosition));
  }
}

void DelineationToolboxForm::on_labelIDEditBox_textEdited(const QString &arg1)
{
  chooseLabel(arg1.toInt());
}

void DelineationToolboxForm::on_labelDescriptionEditBox_textEdited(const QString &arg1)
{
  if (currentLabelID>=0&&currentLabelID<(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
  {
    brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[currentLabelID].descriptor = arg1.toStdString();
    updateLabelSelectionComboBox();
  }
}

void DelineationToolboxForm::on_labelAbbrevEditBox_textEdited(const QString &arg1)
{
  if (currentLabelID>=0&&currentLabelID<(int)brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails.size())
  {
    brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[currentLabelID].tag = arg1.toStdString();
  }
}

void DelineationToolboxForm::on_toolBox_currentChanged(int /*index*/)
{
  updateEditBitmaps();
}

void DelineationToolboxForm::on_labelTableWidget_doubleClicked(const QModelIndex &index)
{
  int n=index.row();
  auto widget=ui->labelTableWidget->item(n,0);
  if (widget)
  {
    int labelID = widget->data(Qt::EditRole).toInt();
    if (brainSuiteWindow)
      brainSuiteWindow->moveToFirstVoxelLabeled(labelID);
  }
}

void DelineationToolboxForm::on_saveROIDetailsTextPushButton_clicked()
{
  if (brainSuiteWindow)
  {
    std::string ofname=brainSuiteWindow->dsGetSaveFileName("Save ROI Details",tsvFilter,"");
    if (ofname.empty()) return;
    std::ofstream ofile(ofname.c_str());
    if (!ofile) { return; }
    int nRows = ui->labelTableWidget->rowCount();
    int nCols = ui->labelTableWidget->columnCount();
    for (int i=0;i<nRows;i++)
    {
      for (int j=0;j<nCols;j++)
      {
        auto widget=ui->labelTableWidget->item(i,j);
        if (j>0) ofile<<'\t';
        if (widget)
        {
          ofile<<widget->text().toStdString();
        }
      }
      ofile<<"\n";
    }
  }
}

template <class T>
void makeMask(Vol3D<uint8> &vMask,Vol3D<T> &vLabel, std::vector<uint8> &labelmap)
{
  vMask.makeCompatible(vLabel);
  const size_t ds=vMask.size();
  for (size_t i=0;i<ds;i++)
    vMask[i] = labelmap[vLabel[i]];
}

template <class T>
void makeMask(Vol3D<uint8> &vMask,Vol3D<T> &vLabel, int labelID)
{
  vMask.makeCompatible(vLabel);
  const size_t ds=vMask.size();
  for (size_t i=0;i<ds;i++)
    vMask[i] = (vLabel[i]==labelID) ? 255 : 0;
}


void DelineationToolboxForm::on_makeMaskFromLabelsPushButton_clicked()
{
  if (!brainSuiteWindow->brainSuiteDataManager.vLabel) return;
  QItemSelectionModel *select = ui->labelMaskTableWidget->selectionModel();
  if (!select) { return ; }
  if (!select->hasSelection()) { return ; };
  QModelIndexList indices=select->selectedRows();
  std::vector<uint8> labelmap(65536,0);
  for (int i=0;i<indices.size();i++)
  {
    int row=indices.at(i).row();
    if (row>=0)
    {
      if (ui->labelMaskTableWidget->isRowHidden(row)) continue; // TODO : need to unselect hidden rows elsewhere in the code
      QTableWidgetItem *item=ui->labelMaskTableWidget->item(row,0);
      if (item)
      {
        int idx=item->data(Qt::EditRole).toInt();
        if (idx>=0 && idx < 65536) labelmap[idx]=255;
      }
    }
  }
  switch (brainSuiteWindow->brainSuiteDataManager.vLabel->typeID())
  {
    case SILT::Sint8 :   makeMask(brainSuiteWindow->brainSuiteDataManager.vMask,*(Vol3D<uint8> *)brainSuiteWindow->brainSuiteDataManager.vLabel,labelmap); break;
    case SILT::Uint8 :   makeMask(brainSuiteWindow->brainSuiteDataManager.vMask,*(Vol3D<uint8> *)brainSuiteWindow->brainSuiteDataManager.vLabel,labelmap); break;
    case SILT::Sint16:   makeMask(brainSuiteWindow->brainSuiteDataManager.vMask,*(Vol3D<uint16> *)brainSuiteWindow->brainSuiteDataManager.vLabel,labelmap); break;
    case SILT::Uint16:   makeMask(brainSuiteWindow->brainSuiteDataManager.vMask,*(Vol3D<uint16> *)brainSuiteWindow->brainSuiteDataManager.vLabel,labelmap); break;
    default: break;
  }
  brainSuiteWindow->updateView(DisplayCode::MaskOn|DisplayCode::Dirty);
}

void DelineationToolboxForm::on_updateMaskLabelListPushButton_clicked()
{
  if (!brainSuiteWindow) return;
  Vol3DBase *vLabel=brainSuiteWindow->brainSuiteDataManager.vLabel;
  if (!vLabel) return;
  Vol3DHistogram hist;
  bool success=hist.compute(vLabel);
  if (!success)
  {
    std::string info="data type " + vLabel->datatypeName() + " is not supported for this operation";
    std::cerr<<info<<std::endl;
    return;
  }
  float voxsize = vLabel->rx * vLabel->ry * vLabel->rz/1000.0f;
  int nItems = 0;
  ui->labelMaskTableWidget->setSortingEnabled(false);
  for (int i=0;i<=hist.tmax;i++)
  {
    const int count = hist.count[i-hist.tmin];
    if (count>0)
    {
      ui->labelMaskTableWidget->setRowCount(nItems+1);
      ui->labelMaskTableWidget->setItem(nItems, 0, new QTableWidgetItem());
      ui->labelMaskTableWidget->item(nItems,0)->setData(Qt::EditRole,i);
      ui->labelMaskTableWidget->setItem(nItems, 1, new QTableWidgetItem(brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].descriptor.c_str()));
      ui->labelMaskTableWidget->setItem(nItems, 2, new QTableWidgetItem());
      ui->labelMaskTableWidget->item(nItems,2)->setData(Qt::EditRole,count);
      ui->labelMaskTableWidget->setItem(nItems, 3, new QTableWidgetItem());
      ui->labelMaskTableWidget->item(nItems,3)->setData(Qt::EditRole,count*voxsize);
      for (int i=0;i<4;i++) makeReadOnly(ui->labelMaskTableWidget->item(nItems,i));
      nItems++;
    }
  }
}

template <class T>
SurfaceAlpha *makeSurface_t(Vol3D<T> &vLabel, const int labelID)
{
  MarchingCubes tessellator;
  SurfaceAlpha *labelSurface=new SurfaceAlpha;
  if (!labelSurface) return 0;
  int n = 10;
  float alpha = 0.5f;
  float c = 5.0f;
  tessellator.cubes(vLabel,*labelSurface,EQ<T>((T)labelID));
  labelSurface->computeNormals();
  SurfLib::computeCurvature(*(labelSurface));
  SurfLib::laplaceSmoothCurvatureWeighted(*(labelSurface),n,alpha,c);
  labelSurface->computeNormals();
  return labelSurface;
}

SurfaceAlpha *makeSurface(Vol3DBase *vLabel, int labelID)
{
  if (!vLabel) return 0;
  switch (vLabel->typeID())
  {
    case SILT::Sint8  : return makeSurface_t(*(Vol3D<uint8 >*)vLabel,labelID); break;
    case SILT::Uint8  : return makeSurface_t(*(Vol3D<uint8 >*)vLabel,labelID); break;
    case SILT::Sint16 : return makeSurface_t(*(Vol3D<uint16>*)vLabel,labelID); break;
    case SILT::Uint16 : return makeSurface_t(*(Vol3D<uint16>*)vLabel,labelID); break;
    default: break;
  }
  return 0;
}
inline DSPoint rgb2pt(const uint32 a)
{
  unsigned char *p = (unsigned char *)&a;
  return DSPoint(p[2]/255.0f,p[1]/255.0f,p[0]/255.0f);
}

void DelineationToolboxForm::on_makeSurfacesFromLabelsPushButton_clicked()
{
  WaitCursor waitCursor(this);
  if (!brainSuiteWindow->brainSuiteDataManager.vLabel) return;
  QItemSelectionModel *select = ui->labelMaskTableWidget->selectionModel();
  if (!select) { return ; }
  if (!select->hasSelection()) { return ; };
  QModelIndexList indices=select->selectedRows();
  std::vector<int> labelIDs;
  for (int i=0;i<indices.size();i++)
  {
    int row=indices.at(i).row();
    if (row>=0)
    {
      if (ui->labelMaskTableWidget->isRowHidden(row)) continue; // TODO : need to unselect hidden rows elsewhere in the code
      QTableWidgetItem *item=ui->labelMaskTableWidget->item(row,0);
      if (item)
      {
        int idx=item->data(Qt::EditRole).toInt();
        if (idx>=0 && idx < 65536) labelIDs.push_back(idx);
      }
    }
  }
  Vol3DBase *vLabel=brainSuiteWindow->brainSuiteDataManager.vLabel;
  for (int i : labelIDs)
  {
    SurfaceAlpha *labelSurface=makeSurface(vLabel,i);
    if (labelSurface)
    {
      labelSurface->filename = brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].descriptor;
      labelSurface->useSolidColor = true;
      labelSurface->useVertexColor = false;
      labelSurface->solidColor =rgb2pt(brainSuiteWindow->brainSuiteDataManager.labelDescriptionSet.labelDetails[i].color);
      brainSuiteWindow->addSurface(labelSurface,true);
    }
  }
}

void DelineationToolboxForm::on_roiFilterLineEdit_textEdited(const QString &filter)
{
  ::applyFilterToTableWidget(ui->labelTableWidget,filter);
}

void DelineationToolboxForm::on_labelMaskLineEdit_textEdited(const QString &filter)
{
  ::applyFilterToTableWidget(ui->labelMaskTableWidget,filter);
  on_labelMaskTableWidget_itemSelectionChanged();
}

void DelineationToolboxForm::on_labelMaskTableWidget_itemSelectionChanged()
{
  if (ui->updateMaskOnSelectionChangeCheckBox->isChecked())
  {
    on_makeMaskFromLabelsPushButton_clicked();
  }
}

