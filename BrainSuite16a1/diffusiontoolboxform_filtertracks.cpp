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

#include "diffusiontoolboxform.h"
#include "ui_diffusiontoolboxform.h"
#include <brainsuitewindow.h>
#include <brainsuitesettings.h>
#include <fibertrackset.h>
#include <qcolordialog.h>
#include <sphereroixmlreader.h>

static std::string xmlFileFilter="XML files (*.xml)";

// basic default colors for spheres
static DSPoint colorTable[]=
{
  DSPoint(1,0,0),
  DSPoint(0,0,1),
  DSPoint(0,1,0),
  DSPoint(1,0,1),
  DSPoint(1,1,0),
  DSPoint(0,1,1)
};

static const int nColors=sizeof(colorTable)/sizeof(colorTable[0]);

bool getSelectedRows(std::vector<bool> &selected, QTableWidget *tableWidget)
{
  QItemSelectionModel *select = tableWidget->selectionModel();
  if (!select) { return false; }
  if (!select->hasSelection()) { return false; };
  const size_t nROIs=selected.size();
  QModelIndexList indices=select->selectedRows();
  std::fill(selected.begin(),selected.end(),false);
  for (int i=0;i<indices.size();i++)
  {
    int row=indices.at(i).row();
    if (row>=0 && row <(int)nROIs) selected[row]=true;
  }
  return true;
}

static void makeReadOnly(QTableWidgetItem *item)
{
  item->setFlags(item->flags() & ~Qt::ItemIsEditable);
  item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
}

template<class ContainerType>
void readUniqueList(ContainerType &container, QLineEdit *lineEdit)
{
  std::istringstream istr(lineEdit->text().toStdString());
  for (;;)
  {
    char ch;
    while (!istr.eof()&&!::isdigit(istr.peek()))
    {
      istr.get(ch);
    }
    if (istr.eof()) break;
    int label=0;
    if (istr>>label)
    {
      container.push_back(label);
    }
  }
  std::sort(container.begin(),container.end());
  container = ContainerType(container.begin(),std::unique(container.begin(),container.end()));
}

inline DSPoint voxelToWorld(const Vol3DBase &vRef, IPoint3D point)
{
  float rx = vRef.rx; if (rx==0) rx = 1;
  float ry = vRef.ry; if (ry==0) ry = 1;
  float rz = vRef.rz; if (rz==0) rz = 1;
  return DSPoint((point.x-0.5f)*rx,(point.y-0.5f)*ry,(point.z-0.5f)*rz);
}

// track filter
void DiffusionToolboxForm::initTrackFilterPage()
{
  ui->addSpherePushButton->setText("");
  ui->removeSpherePushButton->setText("");
  ui->removeSpherePushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCloseButton));
  ui->loadROIsPushButton->setText("");
  ui->loadROIsPushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogOpenButton));
  ui->saveROIsPushButton->setText("");
  ui->saveROIsPushButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
  ui->tractLengthEdit->setText("0.0");
  ui->tractLengthSlider->setRange(0,250);
  ui->roiTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
  ui->roiTableWidget->setColumnCount(ROIColumns::NItems);
  ui->roiTableWidget->setShowGrid(false);
  QStringList headers;
  headers.push_back("On");
  headers.push_back("I/E");
  headers.push_back("And");
  headers.push_back("Col");
  headers.push_back("Position");
  headers.push_back("Radius (mm)");
  ui->roiTableWidget->setHorizontalHeaderLabels(headers);
  ui->roiTableWidget->setColumnWidth(ROIColumns::Active,24);
  ui->roiTableWidget->setColumnWidth(ROIColumns::Include,24);
  ui->roiTableWidget->setColumnWidth(ROIColumns::And,28);
  ui->roiTableWidget->setColumnWidth(ROIColumns::Color,28);
  ui->roiTableWidget->setColumnWidth(ROIColumns::Position,80);
  ui->roiTableWidget->setColumnWidth(ROIColumns::Radius,80);
  ui->roiTableWidget->verticalHeader()->hide();
  ui->sphereRadiusSlider->setRange(0,200);
  ui->sphereRadiusSlider->setValue(10);
  ui->sphereRadiusSlider->setVisible(false);
  ui->roiRadiusSliderLabel->setVisible(false);
  ui->mergeCortialROIsCheckBox->setCheckState(Qt::Checked);
  ui->showSphereIn3DViewCheckBox->setChecked(true);
  ui->showSphereInSliceViewCheckBox->setChecked(true);
  ui->showSphereAsWireframeCheckBox->setChecked(false);
  updateROISphereTable();
}

bool DiffusionToolboxForm::sphereWireframe()
{
  return ui->showSphereAsWireframeCheckBox->isChecked();
}

bool DiffusionToolboxForm::showSphereROIsInSliceView()
{
  return ui->showSphereInSliceViewCheckBox->isChecked();
}

bool DiffusionToolboxForm::showSphereROIs_in3D()
{
  return ui->showSphereIn3DViewCheckBox->isChecked();
}

void DiffusionToolboxForm::on_sphereRadiusSlider_valueChanged(int radius)
{
  QItemSelectionModel *select = ui->roiTableWidget->selectionModel();
  if (!select) { return; }
  if (!select->hasSelection()) { return; };
  QModelIndexList indices=select->selectedRows();
  for (int i=0;i<indices.size();i++)
  {
    int row=indices.at(i).row();
    if ((int)sphereROIs.size()<row) continue;
    sphereROIs[row].radius=radius;
  }
  updateROISphereTableRadii();
  applyCurrentFilters();
  updateImages();
}

void DiffusionToolboxForm::on_showSphereIn3DViewCheckBox_clicked()
{
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showSphereAsWireframeCheckBox_clicked()
{
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::moveSelectedROIto(const IPoint3D &position)
{
  int row = ui->roiTableWidget->currentRow();
  if (row<0||row>=(int)sphereROIs.size()) return;
  if ((int)sphereROIs.size()>row)
  {
    sphereROIs[row].voxelPosition=position;
    std::ostringstream voxelStream;
    voxelStream<<position.x<<' '<<position.y<<' '<<position.z;
    blocking=true;
    ui->roiTableWidget->item(row, ROIColumns::Position)->setText(voxelStream.str().c_str());
    blocking=false;
  }
  applyCurrentFilters();
}

void DiffusionToolboxForm::updateROISphereTableRadii()
{
  blocking=true;
  const int nRows = ui->roiTableWidget->rowCount();
  for (int i=0;i<nRows;i++)
  {
    if (i>=(int)sphereROIs.size()) break;
    ui->roiTableWidget->item(i,ROIColumns::Radius)->setText(QString::number(sphereROIs[i].radius));
  }
  blocking=false;
}

void DiffusionToolboxForm::updateROISphereTable()
{
  blocking=true;
  ui->roiTableWidget->clearContents();
  int nItems=0;
  for (auto sphereROI=sphereROIs.begin();sphereROI<sphereROIs.end();sphereROI++)
  {
    SphereROI roi=*sphereROI;
    std::ostringstream voxelStream;
    voxelStream<<roi.voxelPosition.x<<' '<<roi.voxelPosition.y<<' '<<roi.voxelPosition.z;
    ui->roiTableWidget->setRowCount(nItems+1);
    ui->roiTableWidget->setItem(nItems, ROIColumns::Active, new QTableWidgetItem());
    ui->roiTableWidget->item(nItems,ROIColumns::Active)->data(Qt::CheckStateRole);
    ui->roiTableWidget->item(nItems,ROIColumns::Active)->setCheckState(roi.active ? Qt::Checked : Qt::Unchecked);
    ui->roiTableWidget->setItem(nItems, ROIColumns::Include, new QTableWidgetItem());
    ui->roiTableWidget->item(nItems,ROIColumns::Include)->data(Qt::CheckStateRole);
    ui->roiTableWidget->item(nItems,ROIColumns::Include)->setCheckState(roi.include ? Qt::Checked : Qt::Unchecked);
    ui->roiTableWidget->setItem(nItems, ROIColumns::And, new QTableWidgetItem());
    ui->roiTableWidget->item(nItems,ROIColumns::And)->data(Qt::CheckStateRole);
    ui->roiTableWidget->item(nItems,ROIColumns::And)->setCheckState(roi.andop ? Qt::Checked : Qt::Unchecked);
    ui->roiTableWidget->setItem(nItems, ROIColumns::Color, new QTableWidgetItem(""));
    ui->roiTableWidget->item(nItems,ROIColumns::Color)->setBackgroundColor(QColor::fromRgbF(roi.color.x,roi.color.y,roi.color.z));
    ui->roiTableWidget->setItem(nItems, ROIColumns::Position, new QTableWidgetItem(voxelStream.str().c_str()));
    ui->roiTableWidget->setItem(nItems, ROIColumns::Radius, new QTableWidgetItem(QString::number(roi.radius)));
    makeReadOnly(ui->roiTableWidget->item(nItems,ROIColumns::Active));
    makeReadOnly(ui->roiTableWidget->item(nItems,ROIColumns::Include));
    makeReadOnly(ui->roiTableWidget->item(nItems,ROIColumns::And));
    makeReadOnly(ui->roiTableWidget->item(nItems,ROIColumns::Color));
    nItems++;
  }
  blocking=false;
}

void DiffusionToolboxForm::on_roiTableWidget_cellChanged(int row, int column)
{
  if (blocking) return;
  if (row<0 && row >= (int)sphereROIs.size()) return;
  blocking=true;
  QTableWidgetItem *item=ui->roiTableWidget->item(row,column);
  std::vector<bool> selected(sphereROIs.size(),false);
  getSelectedRows(selected,ui->roiTableWidget);
  switch (column)
  {
    case ROIColumns::Active:
      {
        const bool newState=(ui->roiTableWidget->item(row,ROIColumns::Active)->checkState() == Qt::Checked);
        sphereROIs[row].active=newState;
        if (selected[row])
        {
          for (size_t i=0;i<selected.size();i++)
          {
            if (selected[i])
            {
              sphereROIs[i].active=newState;
              ui->roiTableWidget->item(i,ROIColumns::Active)->setCheckState(newState ? Qt::Checked : Qt::Unchecked);
            }
          }
        }
        else
        {
          ui->roiTableWidget->clearSelection();
          ui->roiTableWidget->selectRow(row);
        }
        applyCurrentFilters();
        updateImages();
      }
      break;
    case ROIColumns::Include:
      {
        const bool newState=(ui->roiTableWidget->item(row,ROIColumns::Include)->checkState() == Qt::Checked);
        sphereROIs[row].include = newState;
        if (selected[row])
        {
          for (size_t i=0;i<selected.size();i++)
          {
            if (selected[i])
            {
              sphereROIs[i].include=newState;
              ui->roiTableWidget->item(i,ROIColumns::Include)->setCheckState(newState ? Qt::Checked : Qt::Unchecked);
            }
          }
        }
        else
        {
          ui->roiTableWidget->clearSelection();
          ui->roiTableWidget->selectRow(row);
        }
        applyCurrentFilters();
      }
      break;
    case ROIColumns::And:
      {
        const bool newState=(ui->roiTableWidget->item(row,ROIColumns::And)->checkState() == Qt::Checked);
        sphereROIs[row].andop = newState;
        if (selected[row])
        {
          for (size_t i=0;i<selected.size();i++)
          {
            if (selected[i])
            {
              sphereROIs[i].andop=newState;
              ui->roiTableWidget->item(i,ROIColumns::And)->setCheckState(newState ? Qt::Checked : Qt::Unchecked);
            }
          }
        }
        else
        {
          ui->roiTableWidget->clearSelection();
          ui->roiTableWidget->selectRow(row);
        }
        applyCurrentFilters();
      }
      break;
    case ROIColumns::Position:
      {
        std::istringstream istr(item->text().toStdString());
        int x,y,z;
        istr>>x>>y>>z;
        sphereROIs[row].voxelPosition=IPoint3D(x,y,z);
        std::cout<<x<<' '<<y<<' '<<z<<std::endl;
        applyCurrentFilters();
        updateImages();
      }
      break;
    case ROIColumns::Radius:
      {
        float r = item->text().toFloat();
        sphereROIs[row].radius=r;
        ui->sphereRadiusSlider->blockSignals(true);
        ui->sphereRadiusSlider->setValue(r);
        ui->sphereRadiusSlider->blockSignals(false);
        applyCurrentFilters();
        updateImages();
      }
      break;
  }
  blocking=false;
}

void DiffusionToolboxForm::increaseSphereRadius()
{
  QItemSelectionModel *select = ui->roiTableWidget->selectionModel();
  int n=ui->roiTableWidget->currentRow();
  if (!select||!select->hasSelection()||n<0) return;
  int radius=++sphereROIs[n].radius;
  ui->roiRadiusSliderLabel->setVisible(true);
  ui->sphereRadiusSlider->setVisible(true);
  ui->sphereRadiusSlider->setValue(radius);
  ui->roiTableWidget->item(n,ROIColumns::Radius)->setText(QString::number(radius));
  updateImages();
}

void DiffusionToolboxForm::decreaseSphereRadius()
{
  QItemSelectionModel *select = ui->roiTableWidget->selectionModel();
  int n=ui->roiTableWidget->currentRow();
  if (!select||!select->hasSelection()||n<0) return;
  int radius=--sphereROIs[n].radius;

  ui->roiRadiusSliderLabel->setVisible(true);
  ui->sphereRadiusSlider->setVisible(true);
  ui->sphereRadiusSlider->setValue(radius);
  ui->roiTableWidget->item(n,ROIColumns::Radius)->setText(QString::number(radius));
  updateImages();
}

void DiffusionToolboxForm::on_addSpherePushButton_clicked()
{
  SphereROI roi;
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume)
  {
    roi.voxelPosition=brainSuiteWindow->currentVolumePosition;
  }
  roi.color=colorTable[(sphereROIs.size())%nColors];
  roi.active=true;
  sphereROIs.push_back(roi);
  updateROISphereTable();
  ui->roiTableWidget->selectRow(sphereROIs.size()-1);
  applyCurrentFilters();
  updateImages();
}

void DiffusionToolboxForm::on_removeSpherePushButton_clicked()
{
  QItemSelectionModel *select = ui->roiTableWidget->selectionModel();
  if (!select) { return; }
  if (!select->hasSelection()) { return; };
  const size_t nROIs=sphereROIs.size();
  QModelIndexList indices=select->selectedRows();
  std::vector<bool> keep(nROIs,true);
  for (int i=0;i<indices.size();i++)
  {
    int row=indices.at(i).row();
    if (row>=0 && row <(int)sphereROIs.size()) keep[row]=false;
  }
  std::vector<SphereROI> newSphereROIs;
  for (size_t i=0;i<nROIs;i++)
    if (keep[i]) newSphereROIs.push_back(sphereROIs[i]);
  sphereROIs = newSphereROIs;
  updateROISphereTable();
  applyCurrentFilters();
}

void DiffusionToolboxForm::on_roiTableWidget_itemSelectionChanged()
{
  QItemSelectionModel *select = ui->roiTableWidget->selectionModel();
  int n=ui->roiTableWidget->currentRow();
  if (!select||!select->hasSelection()||n<0) { ui->sphereRadiusSlider->setVisible(false); ui->roiRadiusSliderLabel->setVisible(false); return; };
  int radius=sphereROIs[n].radius;
  ui->roiRadiusSliderLabel->setVisible(true);
  ui->sphereRadiusSlider->setVisible(true);
  ui->sphereRadiusSlider->setValue(radius);
}

void DiffusionToolboxForm::on_roiTableWidget_cellDoubleClicked(int row, int column)
{
  if (row<0 && row >= (int)sphereROIs.size()) return;
  switch (column)
  {
    case ROIColumns::Color:
      {
        QColor c=QColorDialog::getColor(QColor::fromRgbF(sphereROIs[row].color.x,sphereROIs[row].color.y,sphereROIs[row].color.z),this,"Default Track Color");
        if (c.isValid())
        {
          sphereROIs[row].color=DSPoint(c.redF(),c.greenF(),c.blueF());
          ui->roiTableWidget->item(row,ROIColumns::Color)->setBackgroundColor(QColor::fromRgbF(sphereROIs[row].color.x,sphereROIs[row].color.y,sphereROIs[row].color.z));
        }
        updateImages();
      }
      break;
    default: break;
  }
}

void DiffusionToolboxForm::on_excludeLabelListCheckBox_clicked()
{
  applyCurrentFilters();
}

void DiffusionToolboxForm::on_excludeLabelsLineEdit_textChanged()
{
  if (ui->excludeLabelListCheckBox->isChecked())
    applyCurrentFilters();
}

void DiffusionToolboxForm::on_saveROIsPushButton_clicked()
{
  if (brainSuiteWindow)
  {
    std::string ofname=brainSuiteWindow->dsGetSaveFileName("save fiber track ROI files", xmlFileFilter,"");
    if (ofname.empty()) return;
    std::ofstream ofile(ofname);
    if (!ofile)
    {
      std::cerr<<"unable to open "<<ofname<<" for writing"<<std::endl;
      return;
    }
    ofile<<"<?xml version='1.0' standalone='yes'?>\n"
        <<"<BrainSuiteROIs>\n";
    for (auto i=sphereROIs.begin();i!=sphereROIs.end();i++)
    {
      SphereROI &sphere=*i;
      ofile<<"<ROI ";
      ofile<<"voxelposition='"<<sphere.voxelPosition<<"' ";
      ofile<<"radius_mm='"<<sphere.radius<<"' ";
      ofile<<"active='"<<(sphere.active?'1':'0')<<"' ";
      ofile<<"include='"<<(sphere.include?'1':'0')<<"' ";
      ofile<<"and='"<<(sphere.andop?'1':'0')<<"' ";
      ofile<<"color='"<<sphere.color<<"' ";
      ofile<<"/>\n";
    }
    ofile<<"</BrainSuiteROIs>\n";
    std::cout<<"saved ROIs to "<<ofname<<std::endl;
  }
}

void DiffusionToolboxForm::on_loadROIsPushButton_clicked()
{
  std::string ifname=brainSuiteWindow->getOpenFilenameDS("load fiber track ROI file", xmlFileFilter);
  if (ifname.empty()) return;
  std::cout<<"loading "<<ifname<<std::endl;
  std::vector<SphereROI> newSpheres;
  if (SphereROIXMLReader::readROIXML(newSpheres,ifname))
  {
    sphereROIs=newSpheres;
    updateROISphereTable();
    updateImages();
    applyCurrentFilters();
  }
  else
    std::cerr<<"error reading "<<ifname<<std::endl;
}
