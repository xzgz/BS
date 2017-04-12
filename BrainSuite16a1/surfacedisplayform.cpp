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

#include <qdir.h>
#include <qfileinfo.h>
#include "surfacedisplayform.h"
#include "ui_surfacedisplayform.h"
#include <brainsuitewindow.h>
#include <surfacealpha.h>
#include <qcolordialog.h>
#include <dsglwidget.h>
#include <brainsuitesettings.h>
#include <brainsuitesettings.h>
#include <qfiledialog.h>
#include <surfacerecolorchoices.h>
#include <surflib.h>
#include <waitcursor.h>
#include <glviewcontroller.h>
#include <brainsuitesettings.h>
#include <brainsuiteqtsettings.h>

const char *SurfaceRecolorChoices::names[] = {
  "none",
  "curvature",
  "volume labels",
  "hemisphere labels",
  "primary image intensity",
  "volume blend",
  "vertex labels",
  "vertex attributes"
};

const size_t SurfaceRecolorChoices::nNames = sizeof(SurfaceRecolorChoices::names)/sizeof(SurfaceRecolorChoices::names[0]);

SurfaceDisplayForm::SurfaceDisplayForm(BrainSuiteWindow *brainSuiteWindow, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::SurfaceDisplayForm), brainSuiteWindow(brainSuiteWindow)
{
  ui->setupUi(this);
  ui->zoomLineEdit->setText("1.0");
  ui->openPushButton->setText("");
  ui->openPushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->savePushButton->setText("");
  ui->savePushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->removePushButton->setText("");
  ui->removePushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  DSPoint backgroundColor = brainSuiteWindow->getSurfaceBackgroundColor();
  ui->backgroundColorSelectionButton->setColor(QColor::fromRgbF(backgroundColor.x,backgroundColor.y,backgroundColor.z));
  ui->depthSortCheckBox->setChecked(BrainSuiteSettings::depthSortSurfaceTriangles);
  ui->colorSelectionButton->setColor(DSPoint(0.7f,0.7f,0.7f));
  for (size_t i=0;i<SurfaceRecolorChoices::nNames;i++)
  {
    ui->recolorChoiceComboBox->insertItem((int)i,SurfaceRecolorChoices::names[i],QVariant((unsigned int)i));  // using int because size_t is ambiguous for QVariant
  }
  ui->sliceOpacityLineEdit->setText(QString::number(BrainSuiteSettings::sliceOpacity,'g',3));
  ui->translucentSliceCheckBox->setChecked(BrainSuiteSettings::useSliceOpacity);
  updateSliceViewStatus();
  updateSurfaceSelection();
  updateGeneralCheckboxes();
}

SurfaceDisplayForm::~SurfaceDisplayForm()
{
  delete ui;
}

void SurfaceDisplayForm::updateGeneralCheckboxes()
{
  if (brainSuiteWindow)
  {
    ui->showFibersCheckBox->setChecked(BrainSuiteSettings::showFibers);
    ui->showSurfacesCheckBox->setChecked(BrainSuiteSettings::showSurfaces);
    ui->showTensorGlyphs->setChecked(BrainSuiteSettings::showTensors);
    ui->showODFGlpyhsCheckBox->setChecked(BrainSuiteSettings::showODFs);
    ui->lightingCheckBox->setChecked(BrainSuiteSettings::useLighting);
  }
}

bool SurfaceDisplayForm::findSelectedSurfaces(std::vector<SurfaceAlpha *> &surfaces) // finds the currently selected surface
{
  surfaces.clear();
  QList<QListWidgetItem *> selectionlist = ui->surfaceListWidget->selectedItems();
  for (QList<QListWidgetItem *>::iterator i=selectionlist.begin();i!=selectionlist.end();++i)
  {
    QListWidgetItem *item=*i;
    QVariant v=item->data(Qt::UserRole);
    SurfaceAlpha *surface = (SurfaceAlpha *) v.value<void *>();
    if (surface) surfaces.push_back(surface); // verify it still exists?
  }
  return surfaces.size()!=0;
}

void SurfaceDisplayForm::on_showSurfaceCheckBox_clicked()
{
  if (ui->showSurfaceCheckBox->checkState()==Qt::PartiallyChecked)
    ui->showSurfaceCheckBox->setCheckState(Qt::Checked);
  bool showState=ui->showSurfaceCheckBox->isChecked();
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    if (selectedSurface->show != showState) // in case they were already set, no need to redraw. [ this will probably not happen ]
    {
      selectedSurface->show = showState;
      updateView=true;
    }
  }
  if (updateView) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::updateSurfaceList()
{
  ui->surfaceListWidget->clear();
  if (brainSuiteWindow)
  {
    for (std::list<BrainSuiteDataManager::SurfaceHandle>::iterator siter=brainSuiteWindow->brainSuiteDataManager.surfaces.begin(); siter!=brainSuiteWindow->brainSuiteDataManager.surfaces.end(); siter++)
    {
      SurfaceAlpha *surface = siter->surface;
      if (!surface)
      {
        std::cerr<<"empty surface handle."<<std::endl;
        continue;
      }
      std::string sn = StrUtil::shortName(surface->filename);
      QString q=sn.c_str();
      QListWidgetItem *item = new QListWidgetItem(q);
      item->setData(Qt::UserRole,qVariantFromValue((void *) surface));
      ui->surfaceListWidget->addItem(item);
    }
  }
  ui->surfaceListWidget->clearSelection();
}

Qt::CheckState getCheckState(const int nSet, const int nTotal)
{
  if (nSet==0) return Qt::Unchecked;
  if (nSet==nTotal) return Qt::Checked;
  return Qt::PartiallyChecked;
}

inline bool operator!=(const DSPoint &r, const DSPoint &l)
{
  return (r.x!=l.x || r.y!=l.y || r.z!=l.z);
}

void SurfaceDisplayForm::selectShownSurfaces()
{
  std::vector<int> shown;
  for (int row=0;row<ui->surfaceListWidget->count();row++)
  {
    QListWidgetItem *item=ui->surfaceListWidget->item(row);
    if (!item) continue;
    QVariant v=item->data(Qt::UserRole);
    SurfaceAlpha *surface = (SurfaceAlpha *) v.value<void *>();
    if (surface && surface->show) shown.push_back(row);
  }
  bool first=false;
  if (shown.size())
  {
    ui->surfaceListWidget->clearSelection();
    for (auto row : shown)
    {
      QListWidgetItem *item=ui->surfaceListWidget->item(row);
      if (item)
      {
        if (!first) { first=true; ui->surfaceListWidget->setCurrentRow(row); }
        item->setSelected(true);
      }
    }
  }
}

void SurfaceDisplayForm::updateSurfaceSelection()
{
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0)
  {
    ui->showSurfaceCheckBox->setDisabled(true);
    ui->showAsWireframe->setDisabled(true);
    ui->vertexColorCheckBox->setDisabled(true);
    ui->solidColorCheckBox->setDisabled(true);
    ui->disableClippingCheckBox->setDisabled(true);
    ui->alphaLineEdit->setDisabled(true);
    ui->translucentCheckBox->setDisabled(true);
    ui->recolorSurfacePushButton->setDisabled(true);
    ui->removePushButton->setDisabled(true);
    ui->showSmoothedCheckBox->setDisabled(true);
  }
  else
  {
    ui->recolorSurfacePushButton->setEnabled(true);
    ui->showSurfaceCheckBox->setEnabled(true);
    ui->showAsWireframe->setEnabled(true);
    ui->vertexColorCheckBox->setEnabled(true);
    ui->solidColorCheckBox->setEnabled(true);
    ui->disableClippingCheckBox->setEnabled(true);
    ui->alphaLineEdit->setEnabled(true);
    ui->translucentCheckBox->setEnabled(true);
    ui->removePushButton->setEnabled(true);
    ui->showSmoothedCheckBox->setEnabled(true);
    int nSurf=0;
    int nShow=0;
    int nWire=0;
    int nColor=0;
    int nSolid=0;
    int nClip=0;
    int nTranslucent=0;
    int nAlpha=0;
    int nSmooth=0;
    float alpha=-1;
    DSPoint solidColor(-1,-1,-1);
    bool colorMatch=true;
    for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
    {
      SurfaceAlpha *selectedSurface = *i;
      if (!selectedSurface) continue;
      if (colorMatch)
      {
        if (solidColor.x<0) solidColor = selectedSurface->solidColor;
        else if (solidColor != selectedSurface->solidColor) colorMatch=false;
      }
      nSurf++;
      nShow += selectedSurface->show;
      nWire += selectedSurface->wireFrame;
      nColor += selectedSurface->useVertexColor;
      nSolid += selectedSurface->useSolidColor;
      nClip += selectedSurface->allowClip==false;
      nTranslucent += selectedSurface->translucent;
      if (alpha<0) { alpha = selectedSurface->alpha; }
      if (selectedSurface->alpha==alpha) nAlpha++;
      if (selectedSurface->displayAlternate) nSmooth++;
    }
    ui->colorSelectionButton->setColor(solidColor);
    ui->showSurfaceCheckBox->setCheckState(getCheckState(nShow,nSurf));
    ui->showAsWireframe->setCheckState(getCheckState(nWire,nSurf));
    ui->vertexColorCheckBox->setCheckState(getCheckState(nColor,nSurf));
    ui->solidColorCheckBox->setCheckState(getCheckState(nSolid,nSurf));
    ui->disableClippingCheckBox->setCheckState(getCheckState(nClip,nSurf));
    ui->translucentCheckBox->setCheckState(getCheckState(nTranslucent,nSurf));
    ui->showSmoothedCheckBox->setCheckState(getCheckState(nSmooth,nSurf));
    if (nAlpha==nSurf)
      ui->alphaLineEdit->setText(QString::number(alpha));
    else
      ui->alphaLineEdit->setText("");
  }
  std::ostringstream ostr;
  switch (selectedSurfaces.size())
  {
    case 0 :
      ostr<<"no surface selected."<<std::endl;
      break;
    case 1:
      {
        SurfaceAlpha *surface = selectedSurfaces[0];
        if (surface)
        {
          QFileInfo fileInfo(surface->filename.c_str());
          ostr<<StrUtil::shortName(surface->filename)<<"\n"
             <<fileInfo.canonicalPath().toStdString()<<'\n'
            <<"triangles:\t"<<surface->nt()<<"\n"
           <<"vertices:\t"<<surface->nv()<<"\n"
          <<"normals:\t"<<surface->vertexNormals.size()<<"\n"
          <<"uv map:  \t"<<surface->vertexUV.size()<<"\n"
          <<"vertex color:\t"<<surface->vertexColor.size()<<"\n"
          <<"vertex labels:\t"<<surface->vertexLabels.size()<<"\n";
        }
        else
        {
          std::cerr<<"surface handle was empty."<<std::endl;
          ostr<<"no surface selected."<<std::endl;
        }
      }
      break;
    default:
      {
        size_t nv=0;
        size_t nt=0;
        int nwire=0;
        int nshow=0;
        int count=0;
        int nvertcolor=0;
        int ntrans=0;
        SurfaceAlpha *firstSurface=0;
        float alpha=-1;
        bool commonAlpha=true;
        for (size_t i=0;i<selectedSurfaces.size();i++)
        {
          if (!selectedSurfaces[i]) continue;
          if (!firstSurface) firstSurface = selectedSurfaces[i];
          count++;
          nv += selectedSurfaces[i]->nv();
          nt += selectedSurfaces[i]->nt();
          if (selectedSurfaces[i]->wireFrame) nwire++;
          if (selectedSurfaces[i]->show) nshow++;
          if (selectedSurfaces[i]->useVertexColor) nvertcolor++;
          if (selectedSurfaces[i]->translucent) ntrans++;
          if (alpha==-1) { alpha = selectedSurfaces[i]->alpha; commonAlpha=true; }
          else if (alpha != selectedSurfaces[i]->alpha) { commonAlpha = false; }
        }
        ostr<<count<<" surfaces selected\n"
           <<"triangles:\t"<<nt<<"\n"
          <<"vertices:\t"<<nv<<"\n";
      }
      break;
  }
  if (ostr.str().empty()==false)
  {
    ui->surfaceInfoTextEdit->setText(ostr.str().c_str());
  }
}

void SurfaceDisplayForm::on_surfaceListWidget_itemSelectionChanged()
{
  updateSurfaceSelection();
}

void SurfaceDisplayForm::on_showAsWireframe_clicked()
{
  if (ui->showAsWireframe->checkState()==Qt::PartiallyChecked)
    ui->showAsWireframe->setCheckState(Qt::Checked);
  bool wireState=ui->showAsWireframe->isChecked();
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    if (selectedSurface->wireFrame != wireState) // in case they were already set, no need to redraw. [ this will probably not happen ]
    {
      selectedSurface->wireFrame = wireState;
      updateView=true;
    }
  }
  if (updateView) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_vertexColorCheckBox_clicked()
{
  if (ui->vertexColorCheckBox->checkState()==Qt::PartiallyChecked)
    ui->vertexColorCheckBox->setCheckState(Qt::Checked);
  bool vertexColorState=ui->vertexColorCheckBox->isChecked();
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    selectedSurface->useVertexColor = vertexColorState;
    selectedSurface->useSolidColor = vertexColorState^true;
    updateView=true;
  }
  ui->solidColorCheckBox->setChecked(vertexColorState^true);
  if (updateView) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_solidColorCheckBox_clicked()
{
  if (ui->solidColorCheckBox->checkState()==Qt::PartiallyChecked)
    ui->solidColorCheckBox->setCheckState(Qt::Checked);
  bool solidState=ui->solidColorCheckBox->isChecked();
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    selectedSurface->useVertexColor = solidState^true;
    selectedSurface->useSolidColor = solidState;
    updateView=true;
  }
  ui->vertexColorCheckBox->setChecked(solidState^true);
  if (updateView) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_disableClippingCheckBox_clicked()
{
  if (ui->disableClippingCheckBox->checkState()==Qt::PartiallyChecked)
    ui->disableClippingCheckBox->setCheckState(Qt::Checked);
  bool clipState=ui->disableClippingCheckBox->isChecked()==false;
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    if (selectedSurface->allowClip != clipState) // in case they were already set, no need to redraw. [ this will probably not happen ]
    {
      selectedSurface->allowClip = clipState;
      updateView=true;
    }
  }
  if (updateView) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_colorSelectionButton_clicked()
{
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0) return;

  QColor c=QColorDialog::getColor(ui->colorSelectionButton->getColor(),this,"New Surface Color");
  if (!c.isValid()) return;
  ui->colorSelectionButton->setColor(c);
  DSPoint newColor(c.redF(),c.greenF(),c.blueF());

  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    selectedSurface->useVertexColor = false;
    selectedSurface->useSolidColor = true;
    selectedSurface->solidColor = newColor;
  }
  ui->solidColorCheckBox->setChecked(true);
  ui->vertexColorCheckBox->setChecked(false);
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_translucentCheckBox_clicked()
{
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0) return;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!= selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    selectedSurface->translucent = ui->translucentCheckBox->isChecked();
    if (brainSuiteWindow) brainSuiteWindow->updateSurfaceView();
  }
}

void SurfaceDisplayForm::on_openPushButton_clicked()
{
  if (brainSuiteWindow)
    brainSuiteWindow->loadSurface();
}

void SurfaceDisplayForm::on_savePushButton_clicked()
{
  if (!brainSuiteWindow) return;
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0) return;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    std::string target = selectedSurface->filename.empty() ? BrainSuiteQtSettings::getCurrentDirectory() : selectedSurface->filename;
    QString filename = brainSuiteWindow->dsGetSaveFileNameQS("Save Surface","Surface Files (*.dfs *.obj)",target.c_str());
    if (filename.isEmpty()) break;
    selectedSurface->write(filename.toStdString());
    selectedSurface->filename=filename.toStdString();
  }
  updateSurfaceList();
}

void SurfaceDisplayForm::on_removePushButton_clicked()
{
  if (!brainSuiteWindow) return;
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0) return;

  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    brainSuiteWindow->brainSuiteDataManager.removeSurface(selectedSurface);
    updateView=true;
  }
  brainSuiteWindow->updateSurfaceDisplayLists();
  if (updateView)
    brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_showImageSlicesCheckBox_clicked()
{
  BrainSuiteSettings::renderSlices = ui->showImageSlicesCheckBox->isChecked();
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::updateSliceViewStatus()
{
  ui->showImageSlicesCheckBox->setChecked(BrainSuiteSettings::renderSlices);
  if (brainSuiteWindow && brainSuiteWindow->dsglWidget)
  {
    ui->showXYSliceCheckBox->setChecked(brainSuiteWindow->dsglWidget->surfaceViewProperties.showXYPlane);
    ui->showXZSliceCheckBox->setChecked(brainSuiteWindow->dsglWidget->surfaceViewProperties.showXZPlane);
    ui->showYZSliceCheckBox->setChecked(brainSuiteWindow->dsglWidget->surfaceViewProperties.showYZPlane);
  }
  else
  {
    ui->showXYSliceCheckBox->setChecked(true);
    ui->showXZSliceCheckBox->setChecked(true);
    ui->showYZSliceCheckBox->setChecked(true);
  }
}

void SurfaceDisplayForm::on_showXYSliceCheckBox_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->dsglWidget)
  {
    brainSuiteWindow->dsglWidget->surfaceViewProperties.showXYPlane = ui->showXYSliceCheckBox->isChecked();
    brainSuiteWindow->updateSurfaceView();
  }
}

void SurfaceDisplayForm::on_showXZSliceCheckBox_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->dsglWidget)
  {
    brainSuiteWindow->dsglWidget->surfaceViewProperties.showXZPlane = ui->showXZSliceCheckBox->isChecked();
    brainSuiteWindow->updateSurfaceView();
  }
}

void SurfaceDisplayForm::on_showYZSliceCheckBox_clicked()
{
  if (brainSuiteWindow && brainSuiteWindow->dsglWidget)
  {
    brainSuiteWindow->dsglWidget->surfaceViewProperties.showYZPlane = ui->showYZSliceCheckBox->isChecked();
    brainSuiteWindow->updateSurfaceView();
  }
}

void SurfaceDisplayForm::currentColorChanged(const QColor & c)
{
  ui->backgroundColorSelectionButton->setColor(c);
  brainSuiteWindow->setSurfaceBackgroundColor(DSPoint(c.redF(),c.greenF(),c.blueF()));
}

void SurfaceDisplayForm::on_backgroundColorSelectionButton_clicked()
{
  if (!brainSuiteWindow) return;
  QColor currentColor(ui->backgroundColorSelectionButton->getColor());
  QColorDialog dialog(currentColor,this);
  dialog.setWindowTitle("New Background Color");
  connect(&dialog, SIGNAL(currentColorChanged(const QColor&)), this, SLOT(currentColorChanged(const QColor&)));
  QColor nextColor;
  if (dialog.exec()==QDialog::Accepted)
  {
    nextColor=dialog.currentColor();
  }
  else
  {
    nextColor=currentColor;
  }
  ui->backgroundColorSelectionButton->setColor(nextColor);
  brainSuiteWindow->setSurfaceBackgroundColor(DSPoint(nextColor.redF(),nextColor.greenF(),nextColor.blueF()));
}

void SurfaceDisplayForm::on_depthSortCheckBox_clicked()
{
  BrainSuiteSettings::depthSortSurfaceTriangles ^= true;
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_alphaLineEdit_textEdited(const QString & /* arg1 */)
{
  float alpha = ui->alphaLineEdit->text().toFloat();
  if (alpha<0) alpha=0;
  if (alpha>1) alpha=1;
  if (!brainSuiteWindow) return;
  std::vector<SurfaceAlpha *> selectedSurfaces;
  findSelectedSurfaces(selectedSurfaces);
  if (selectedSurfaces.size()==0) return;

  bool updateView=false;
  for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
  {
    SurfaceAlpha *selectedSurface = *i;
    if (!selectedSurface) continue;
    selectedSurface->alpha = alpha;
    updateView=true;
  }
  if (updateView)
    brainSuiteWindow->updateSurfaceView();
}

bool surfaceRecolor(SurfaceAlpha *surface, const int recolorChoice, BrainSuiteWindow *brainSuiteWindow);
bool surfaceRecolorRequirementsOK(const int recolorChoice, BrainSuiteWindow *brainSuiteWindow);

void SurfaceDisplayForm::on_recolorSurfacePushButton_clicked()
{
  QVariant v=ui->recolorChoiceComboBox->itemData(ui->recolorChoiceComboBox->currentIndex());
  bool ok=false;
  int recolorChoice=v.toInt(&ok);
  if (!surfaceRecolorRequirementsOK(recolorChoice,brainSuiteWindow))
  {
    std::cerr<<"could not perform operation."<<std::endl;
    return;
  }
  if (ok)
  {
    std::vector<SurfaceAlpha *> selectedSurfaces;
    findSelectedSurfaces(selectedSurfaces);
    bool updateView=false;
    for (std::vector<SurfaceAlpha *>::iterator i=selectedSurfaces.begin(); i!=selectedSurfaces.end(); ++i)
    {
      SurfaceAlpha *selectedSurface = *i;
      if (!selectedSurface) continue;
      if (!surfaceRecolor(selectedSurface,recolorChoice,brainSuiteWindow))
        std::cerr<<"could not recolor "<<selectedSurface->filename<<std::endl;
      updateView=true;
    }
    if (updateView)
      brainSuiteWindow->updateSurfaceView();
  }
}

void SurfaceDisplayForm::on_showFibersCheckBox_clicked()
{
  brainSuiteWindow->setShowFibers(ui->showFibersCheckBox->isChecked());
}

void SurfaceDisplayForm::on_showTensorGlyphs_clicked()
{
  brainSuiteWindow->setShowTensors(ui->showTensorGlyphs->isChecked());
}

void SurfaceDisplayForm::on_showODFGlpyhsCheckBox_clicked()
{
  brainSuiteWindow->setShowODFs(ui->showODFGlpyhsCheckBox->isChecked());
}

void SurfaceDisplayForm::on_showSurfacesCheckBox_clicked()
{
  brainSuiteWindow->setShowSurfaces(ui->showSurfacesCheckBox->isChecked());
}

void SurfaceDisplayForm::on_lightingCheckBox_clicked()
{
  brainSuiteWindow->setShowLighting(ui->lightingCheckBox->isChecked());
}

void SurfaceDisplayForm::on_sliceOpacityLineEdit_textEdited(const QString &arg1)
{
  BrainSuiteSettings::sliceOpacity = arg1.toFloat();
  if (BrainSuiteSettings::sliceOpacity<0) BrainSuiteSettings::sliceOpacity=0;
  if (BrainSuiteSettings::sliceOpacity>1) BrainSuiteSettings::sliceOpacity=1;
  if (brainSuiteWindow) brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_translucentSliceCheckBox_clicked()
{
  BrainSuiteSettings::useSliceOpacity=ui->translucentSliceCheckBox->isChecked();
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void SurfaceDisplayForm::on_zoomLineEdit_textEdited(const QString &arg1)
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->dsglWidget->glViewController.properties.zoom = arg1.toFloat();
    brainSuiteWindow->updateSurfaceView();
  }
}

bool computeAlternateNormals(SurfaceAlpha *surface)
{
  if (!surface || (surface->nv()!=surface->alternateVertices.size())) return false;
  const size_t nt = surface->triangles.size();
  const size_t nv = surface->vertices.size();
  surface->vertexNormals.resize(nv);
  std::fill(surface->vertexNormals.begin(),surface->vertexNormals.end(),DSPoint(0,0,0));
  const DSPoint *vv = &surface->alternateVertices[0];
  const Triangle *tv = &surface->triangles[0];
  DSPoint *vn=&surface->vertexNormals[0];
  for (size_t n=0;n<nt;n++)
  {
    Triangle t  = tv[n];
    DSPoint v1    = vv[t.a];
    DSPoint v3    = vv[t.b];
    DSPoint v2    = vv[t.c];
    DSPoint tn = cross(v3-v1,v3-v2);
    vn[t.a] += tn;
    vn[t.b] += tn;
    vn[t.c] += tn;
  }
  for (size_t n=0;n<nv;n++)
  {
    const float m = vn[n].mag();
    if (m>0)
    {
      vn[n].x/=m;
      vn[n].y/=m;
      vn[n].z/=m;
    }
  }
  return true;
}

void laplaceSmoothAlternate(SurfaceAlpha &surface, const int n, const float alpha)
{
  const size_t nv = surface.vertices.size();
  if (surface.connectivity.size()!=nv) surface.computeConnectivity();
  Surface::VertexConnection *connex = &surface.connectivity[0];
  surface.alternateVertices = surface.vertices;
  DSPoint *point = &surface.alternateVertices[0];
  DSPoint *dst = point;
  const float a = alpha;
  const float b = 1-a;
  for (int k=0;k<n;k++)
  {
    for (size_t i=0;i<nv;i++)
    {
      Surface::VertexConnection &c = connex[i];
      const int N = c.size();
      DSPoint d(0,0,0);
      for (int j=0;j<N;j++)
      {
        d += point[c[j]];
      }
      dst[i] = b * point[i] + (a/N) * d;
    }
  }
}

void SurfaceDisplayForm::on_showSmoothedCheckBox_clicked()
{
  std::vector<SurfaceAlpha *> surfaces;
  findSelectedSurfaces(surfaces);
  bool updateView=false;
  WaitCursor wc(this);
  for (auto surface=surfaces.begin(); surface != surfaces.end(); surface++)
  {
    SurfaceAlpha *s=*surface;
    if (s->displayAlternate)
    {
      s->displayAlternate=false;
      s->computeNormals();
    }
    else
    {
      s->displayAlternate=true;
      if (s->alternateVertices.size()!=s->nv())
      {
        laplaceSmoothAlternate(*s,25,0.50f);
        ::computeAlternateNormals(s);
      }
      else
        ::computeAlternateNormals(s);
    }
    updateView=true;
  }
  if (updateView) brainSuiteWindow->updateSurfaceView();
}
