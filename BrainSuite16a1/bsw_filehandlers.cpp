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

#include "brainsuitewindow.h"
#include "ui_brainsuitewindow.h"
#include <brainsuitewindow_t.h>
#include <qinputdialog.h>
#include <waitcursor.h>
#include <surfacealpha.h>
#include <brainsuiteqtsettings.h>
#include <brainsuitesettings.h>
#include <diffusiontoolboxform.h>
#include <imagedisplaypropertiesform.h>
#include <volumeloader.h>
#include <harditool.h>
#include <fibertrackset.h>
#include <colorops.h>
#include <delineationtoolboxform.h>
#include <colormap.h>
#include <dsglwidget.h>
#include <imageframe.h>
#include <imagewidget.h>
#include <cerebrumextractiondialog.h>
#include <corticalextractionsequencedialog.h>
#include <skullstrippingdialog.h>
#include <hardirenderer.h>
#include <nonuniformitycorrectiondialog.h>
#include <surfacedisplayform.h>
#include <connectivityviewdialog.h>
#include <dsglviewdialog.h>
#include <qevent.h>
#include <qmimedata.h>
#include <protocolcurveset.h>
#include <curvetoolboxform.h>
#include <qmessagebox.h>
#include <colormap.h>

extern ColorMap lutList;
extern HARDIRenderer hardiRenderer;
extern ColorMap lutList;
extern HARDITool hardiTool;

DSPoint computeCenter(FiberTrackSet *fiberTrackSet);
std::string imageVolumeFilter="3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;All Files (*)";
std::string labelFilter="3D Image Volume Files (*.label.img *.label.nii *.label.img.gz *.label.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;All Files (*)";
std::string maskFilter="3D Mask Images (*.mask.img *.mask.nii *.mask.img.gz *.mask.nii.gz);;3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz);;All Files (*)";
std::string analyzeOnlyImageFilter="3D Analyze Image Volume Files (*.img *.img.gz)";

bool BrainSuiteWindow::setCurrentDirectory(std::string directory)
{
  QFileInfo fi(directory.c_str());
  BrainSuiteQtSettings::setCurrentDirectory(fi.canonicalPath().toStdString());
  return true;
}

void BrainSuiteWindow::on_action_Surface_triggered()
{
  loadSurface();
  return;
}

// TODO: move settings back to BrainSuiteQTSettings
void BrainSuiteWindow::addRecentFile(std::string ifname)
{
  if (disableRecentFileList) return;
  if (ifname.empty()) return;
  QString filename(ifname.c_str());
  QSettings settings("BrainSuite",BrainSuiteQtSettings::brainSuiteQtKey);
  QStringList files = settings.value("recentFileList").toStringList();
  files.removeAll(filename);
  files.prepend(filename);
  const int MaxRecentFiles = (int)recentFileActions.size();
  while (files.size() > MaxRecentFiles)
    files.removeLast();
  settings.setValue("recentFileList", files);
  updateRecentFileActions();
}

void BrainSuiteWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData()->hasFormat("text/uri-list"))
  {
    event->acceptProposedAction();
  }
}

void BrainSuiteWindow::dropEvent(QDropEvent *event)
{
  if (event->mimeData()->hasUrls())
  {
    disablePathChecking=true;
    QList<QUrl> list = event->mimeData()->urls();
    for (QList<QUrl>::iterator i=list.begin(); i!=list.end();i++)
    {
      loadFile(i->toLocalFile().toStdString());
    }
    disablePathChecking=false;
    checkAutosavePath();
  }
}

void BrainSuiteWindow::openRecentFile()
{
  QAction *action = qobject_cast<QAction *>(sender());
  if (action)
  {
    loadFile(action->data().toString().toStdString());
  }
}


bool BrainSuiteWindow::loadFile(std::string ifname)
{
  ifname=QDir::toNativeSeparators(QDir::cleanPath(ifname.c_str())).toStdString();
  WaitCursor waitCursor(this);
  std::string filetype = StrUtil::getExt(StrUtil::gzStrip(ifname));
  if (StrUtil::eqnocase(filetype,"dfs")) return loadSurface(ifname);
  if (StrUtil::eqnocase(filetype,"obj")) return loadSurface(ifname);
  if (StrUtil::eqnocase(filetype,"img")||StrUtil::eqnocase(filetype,"nii")||StrUtil::eqnocase(filetype,"hdr")) return loadVolume(ifname);
  if (StrUtil::eqnocase(filetype,"trk")) return loadFiberTrackSet(ifname);
  if (StrUtil::eqnocase(filetype,"dft")) return loadFiberTrackSet(ifname);
  if (StrUtil::eqnocase(filetype,"dfc")) return loadProtocolCurveset(ifname);
  if (StrUtil::eqnocase(filetype,"ucf")) return loadCurveSet(ifname);
  if (StrUtil::eqnocase(filetype,"odf")) return loadHARDISHC(ifname);
  if (StrUtil::eqnocase(filetype,"shc")) return loadHARDISHC(ifname);
  if (StrUtil::eqnocase(filetype,"bst")) return loadBSTFile(ifname);
  if (StrUtil::eqnocase(filetype,"lut")) return lutList.loadCustomLUT(ifname);
  std::cerr<<"Files of type "<<filetype.c_str()<<" are not supported at this time."<<std::endl;
  return false;
}

std::string BrainSuiteWindow::getOpenFilenameDS(std::string caption, std::string filter)
{
  QString s=QFileDialog::getOpenFileName(this,
                                         caption.c_str(),
                                         BrainSuiteQtSettings::getCurrentDirectory().c_str(),
                                         filter.c_str(),
                                         0,
                                         fileDialogOptions);
  if (s.isEmpty()) return std::string();
  return QDir::toNativeSeparators(s).toStdString();
}

std::string BrainSuiteWindow::getOpenImageFilename()
{
  return getOpenFilenameDS("Open Image Volume",imageVolumeFilter);
}

bool BrainSuiteWindow::loadOverlay1Volume()
{
  std::string filename = getOpenImageFilename();
  if (filename.empty()) return false;
  WaitCursor waitCursor(this);
  return loadOverlay1Volume(filename);
}

bool BrainSuiteWindow::loadOverlay2Volume()
{
  std::string filename = getOpenImageFilename();
  if (filename.empty()) return false;
  WaitCursor waitCursor(this);
  return loadOverlay2Volume(filename);
}

bool BrainSuiteWindow::loadLabelVolume()
{
  std::string filename = getOpenFilenameDS("Open Label Volume", labelFilter);
  if (filename.empty()) return false;
  return loadLabelVolume(filename);
}

bool BrainSuiteWindow::saveLabelVolume(std::string labelFilename)
{
  if (!brainSuiteDataManager.vLabel) return false;
  return brainSuiteDataManager.vLabel->write(labelFilename);
}

QString BrainSuiteWindow::dsGetSaveFileNameQS(QString title, QString filter, QString /*defaultName*/)
{
  return QDir::toNativeSeparators(
        QFileDialog::getSaveFileName(this,title,
                                     BrainSuiteQtSettings::getCurrentDirectory().c_str(),
                                     filter,0,fileDialogOptions));
}

std::string BrainSuiteWindow::dsGetSaveFileName(std::string title, std::string filter, std::string defaultName)
{
  return dsGetSaveFileNameQS(title.c_str(),filter.c_str(),defaultName.c_str()).toStdString();
}

bool BrainSuiteWindow::saveLabelVolume()
{
  if (!brainSuiteDataManager.vLabel) return false;
  std::string labelFilename=dsGetSaveFileName("Save Label Volume",labelFilter,brainSuiteDataManager.vLabel->filename);
  if (labelFilename.empty()) return false;
  return saveLabelVolume(labelFilename);
}

bool BrainSuiteWindow::loadMaskVolume(std::string maskFilename)
{
  WaitCursor wc(this);

  bool state=(brainSuiteDataManager.vMask.read(maskFilename));
  updateImages();
  updateImageNames();
  if (!state) std::cout<<"loading mask failed. "<<maskFilename<<std::endl;
  return state;
}

bool BrainSuiteWindow::loadMaskVolume()
{
  std::string filename = getOpenFilenameDS("Open Mask Volume",maskFilter);
  if (filename.empty()) return false;
  return loadMaskVolume(filename);
}

void BrainSuiteWindow::on_action_SaveMaskVolume_triggered()
{
  saveMaskVolume();
}

bool BrainSuiteWindow::saveMaskVolume()
{
  if (!brainSuiteDataManager.vMask.size()) return false;
  std::string filename=dsGetSaveFileName("Save Mask Volume","3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)","");
  if (filename.empty()) return false;
  return saveMaskVolume(filename);
}

bool BrainSuiteWindow::loadCustomCerebrumMask()
{
  std::string filename = getOpenFilenameDS("Open Image Volume",maskFilter);
  if (filename.empty()==false)
  {
    return loadCustomCerebrumMask(filename);
  }
  return false;
}

void BrainSuiteWindow::on_actionOpen_connectivity_matrix_triggered()
{
  std::string filename = getOpenFilenameDS("Open Connectivity Matrix","Tab separated value files (*.tsv)");
  if (filename.empty()) return;
  brainSuiteDataManager.connectivityMap.readTSV(filename);
  showDialog(toolManager.connectivityViewDialog);
}

void BrainSuiteWindow::on_actionLoad_Custom_Cerebrum_Mask_triggered()
{
  loadCustomCerebrumMask();
}

SurfaceAlpha *BrainSuiteWindow::loadSurface(std::string ifname)
{
	WaitCursor wc(this);
	SurfaceAlpha *surface = new SurfaceAlpha;
	if (!surface) return 0;
	if (surface->read(ifname))
	{
		setCurrentDirectory(surface->filename);
		addRecentFile(surface->filename);
		setWindowTitle(QDir::toNativeSeparators(QDir::cleanPath(surface->filename.c_str())));
		if (!dsglWidget)
			initializeSurfaceView();
		DSPoint faceColor(0.7f,0.7f,0.7f);
		surface->solidColor = faceColor;
		brainSuiteDataManager.surfaces.push_back(surface);
		showSurfaceDisplayProperties();
		updateSurfaceDisplayLists();
		updateSurfaceView();
		return surface;
	}
	else
	{
    std::cerr<<"error reading "<<ifname<<std::endl;
		delete surface;
		return 0;
	}
}

bool BrainSuiteWindow::loadSurface()
{
	bool status = true;
	QStringList filenames = QFileDialog::getOpenFileNames(this,
                                          "Open Surface Files",
                                          BrainSuiteQtSettings::getCurrentDirectory().c_str(),
                                          "Surface Files (*.dfs *.dfs.gz *.obj)",
                                          0,fileDialogOptions);
	for (QStringList::const_iterator constIterator = filenames.constBegin(); constIterator != filenames.constEnd(); ++constIterator)
	{
		if (!loadSurface((*constIterator).toStdString())) status = false;
	}
	return status;
}

bool BrainSuiteWindow::loadProtocolCurveset()
{
	std::string filename = getOpenFilenameDS("Open Protocol Curve Set","Curve Files (*.dfc)");
	WaitCursor waitCursor(this);
	return (filename.empty()==false) ? loadProtocolCurveset(filename) : false;
}

void BrainSuiteWindow::loadVolume(Vol3DBase *newVolume, bool autoScale)
{
	if (!newVolume) return;
	QFileInfo fileInfo(newVolume->filename.c_str());
	setWindowTitle(QDir::toNativeSeparators(QDir::cleanPath(newVolume->filename.c_str())));
	if (ports.size()==0)
    initializeWindows();
	brainSuiteDataManager.addVolume(newVolume);
	if (fileInfo.exists())
	{
		setWindowFilePath(newVolume->filename.c_str());
		setCurrentDirectory(newVolume->filename.c_str());
		addRecentFile(newVolume->filename);
	}
  if (newVolume->typeID()==SILT::Eigensystem3x3f)
	{
		BrainSuiteSettings::showTensors=true;
		showDiffusionToolbox();
		if (toolManager.diffusionToolboxForm)
		{
			toolManager.diffusionToolboxForm->resetDiffusionScale();
			toolManager.diffusionToolboxForm->updateDiffusionProperties();
			toolManager.diffusionToolboxForm->showDTIPage();
		}
	}
  updateVolumeView(autoScale);
  toolManager.imageDisplayPropertiesForm->setImageVolumeSliderRange();
}

bool BrainSuiteWindow::loadVolume(std::string filename)
{
	WaitCursor wc(this);
	Vol3DBase *newVolume = VolumeLoader::load(filename);
  if (!newVolume) { std::cerr<<"error loading "<<filename<<std::endl; return false; }
  loadVolume(newVolume,true);
	checkAutosavePath();
	return true;
}

bool BrainSuiteWindow::loadVolume()
{
	std::string filename=getOpenImageFilename();
	if (filename.empty()) return false;
	WaitCursor waitCursor(this);
	return loadVolume(filename);
}

bool BrainSuiteWindow::loadOverlay1Volume(Vol3DBase *volume, bool autoscale)
{
  if (volume)
  {
    delete brainSuiteDataManager.imageOverlay1Volume;
    brainSuiteDataManager.imageOverlay1Volume = volume;
    if (autoscale && !BrainSuiteSettings::dontAutoScale) autoscaleOverlay1();
    updateImages();
    updateImageNames();
    if (autoscale) updateImageDisplaySliders();
    toolManager.imageDisplayPropertiesForm->setImageOverlay1SliderRange();
    return true;
  }
  else
    return false;
}

bool BrainSuiteWindow::loadOverlay1Volume(std::string filename, bool noScale)
{
	WaitCursor wc(this);
	Vol3DBase *newVolume = VolumeLoader::load(filename);
  if (!newVolume) { std::cerr<<"error loading "<<filename<<std::endl; return false; }
  return loadOverlay1Volume(newVolume,noScale==false);
}

bool BrainSuiteWindow::loadOverlay2Volume(Vol3DBase *volume, bool autoscale)
{
  if (volume)
  {
    delete brainSuiteDataManager.imageOverlay2Volume;
    brainSuiteDataManager.imageOverlay2Volume = volume;
    if (autoscale && !BrainSuiteSettings::dontAutoScale) autoscaleOverlay2();
    updateImages();
    updateImageNames();
    updateImageDisplaySliders();
    toolManager.imageDisplayPropertiesForm->setImageOverlay2SliderRange();
    return true;
  }
  else
    return false;
}

bool BrainSuiteWindow::loadOverlay2Volume(std::string filename)
{
	WaitCursor wc(this);
	Vol3DBase *newVolume = VolumeLoader::load(filename);
  if (!newVolume) { std::cerr<<"error loading "<<filename<<std::endl; return false; }
  return loadOverlay2Volume(newVolume,true);
}

bool BrainSuiteWindow::loadLabelVolume(std::string labelFilename)
{
	WaitCursor wc(this);
	Vol3DBase *newVolume = VolumeLoader::load(labelFilename);
	if (newVolume)
	{
		delete brainSuiteDataManager.vLabel;
		brainSuiteDataManager.vLabel=newVolume;
	}
	else
	{
    std::cerr<<"unable to load "<<labelFilename<<std::endl;
	}
  updateImages();
  updateImageNames();
  return newVolume!=nullptr;
}

bool BrainSuiteWindow::saveMaskVolume(std::string maskFilename)
{
	if (!brainSuiteDataManager.vMask.size()) return false;
	return brainSuiteDataManager.vMask.write(maskFilename);
}


bool BrainSuiteWindow::loadHARDISHC(std::string hardiFilename)
{
	if (hardiTool.loadSHClist(hardiFilename))
	{
		showDiffusionToolbox();
		Vol3D<float32> *newVolume = new Vol3D<float32>;
		if (newVolume)
		{
			newVolume->copy(hardiTool.vGFA);
			delete brainSuiteDataManager.volume;
			brainSuiteDataManager.volume = newVolume;
			brainSuiteDataManager.volume->origin.x=0; // kludge
			brainSuiteDataManager.volume->origin.y=0;
			brainSuiteDataManager.volume->origin.z=0;
      loadVolume(newVolume,true);
			BrainSuiteSettings::showODFs=true;
			if (toolManager.diffusionToolboxForm)
			{
				toolManager.diffusionToolboxForm->updateDiffusionProperties();
				toolManager.diffusionToolboxForm->showHARDIPage();
				toolManager.diffusionToolboxForm->selectODFTracking();
				ui->diffusionToolboxDockWidget->raise();
			}
		}
		addRecentFile(hardiFilename);
		return true;
	}
	return false;
}

bool BrainSuiteWindow::loadFiberTrackSet(std::string tractFilename)
{
	if (!brainSuiteDataManager.fiberTrackSet)
		brainSuiteDataManager.fiberTrackSet = new FiberTrackSet;
	if (!brainSuiteDataManager.fiberTrackSet)			return false;
	if (!brainSuiteDataManager.fiberTrackSet->read(tractFilename))
	{
		std::cerr<<"could not read "<<tractFilename<<std::endl;
		return false;
	}
	brainSuiteDataManager.fiberCentroid=computeCenter(brainSuiteDataManager.fiberTrackSet);
  showDiffusionToolbox();
  if (toolManager.diffusionToolboxForm)
  {
    toolManager.diffusionToolboxForm->showTrackDisplayPage();
		toolManager.diffusionToolboxForm->applyCurrentFilters();
  }
	initializeSurfaceView();
	addRecentFile(tractFilename);
	updateSurfaceView();
	setCurrentDirectory(tractFilename);
	setWindowTitle(QDir::toNativeSeparators(QDir::cleanPath(tractFilename.c_str())));
	return true;
}


bool BrainSuiteWindow::loadLabelDescriptionFile(std::string labelDescriptionFilename)
{
	if (!brainSuiteDataManager.labelDescriptionSet.read(labelDescriptionFilename))
		return false;
	for (size_t i=0;i<brainSuiteDataManager.labelDescriptionSet.labelDetails.size();i++)
	{
		if (0)
		{
			uint32 rgb = brainSuiteDataManager.labelDescriptionSet.labelDetails[i].color;
			brainSuiteDataManager.labelDescriptionSet.labelDetails[i].color = ((rgb&255)<<16)|((rgb>>16)&255)|(rgb&0xFF00);
		}
		LabelOp::colorTable[i] = brainSuiteDataManager.labelDescriptionSet.labelDetails[i].color;
	}
  if (toolManager.delineationToolboxForm)
	{
		toolManager.delineationToolboxForm->updateLabelSelectionComboBox();
		toolManager.delineationToolboxForm->updateColorSelectionButton();
	}
	updateImages();
	return true;
}


bool BrainSuiteWindow::saveConnectivityMatrix(std::string ofname)
{
  std::ofstream ofile(ofname.c_str());
  if (!ofile) return false;
  size_t nElements = brainSuiteDataManager.connectivityMap.size();
  ofile<<'0';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<'\t'<<brainSuiteDataManager.connectivityMap.getLabelID(i);
  }
  ofile<<'\n';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<brainSuiteDataManager.connectivityMap.getLabelID(i);
    for (size_t j=0;j<nElements;j++)
    {
      float value = brainSuiteDataManager.connectivityMap.adjacencyMatrix((int)i,(int)j);
      ofile<<'\t'<<value;
    }
    ofile<<'\n';
  }
  return true;
}

bool BrainSuiteWindow::saveConnectivityMatrix()
{
  QString filename = dsGetSaveFileNameQS("Save Connectivity Matrix","Tab separated values (*.tsv)","");
  std::ofstream ofile(filename.toStdString().c_str());
  if (!ofile) return false;
  size_t nElements = brainSuiteDataManager.connectivityMap.size();
  ofile<<'0';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<'\t'<<brainSuiteDataManager.connectivityMap.getLabelID(i);
  }
  ofile<<'\n';
  for (size_t i=0;i<nElements;i++)
  {
    ofile<<brainSuiteDataManager.connectivityMap.getLabelID(i);
    for (size_t j=0;j<nElements;j++)
    {
      float value = brainSuiteDataManager.connectivityMap.adjacencyMatrix((int)i,(int)j);
      ofile<<'\t'<<value;
    }
    ofile<<'\n';
  }
  return true;
}


void BrainSuiteWindow::on_actionSave_Surface_View_Screenshow_triggered()
{
  if (!dsglWidget) return;
  QImage qpm = dsglWidget->grabFrameBuffer();
  QString filename = dsGetSaveFileNameQS(("Save Screenshot"),("image format (*.jpg *.png)"),"");
  if (filename.isEmpty()) return;
  if (qpm.save(filename))
    std::cout<<"saved file."<<filename.toStdString()<<std::endl;
  else
    std::cerr<<"unable to save file "<<filename.toStdString()<<std::endl;
}

void BrainSuiteWindow::on_actionSave_Custom_Sized_Rendering_triggered()
{
  QInputDialog qd(this);
  qd.setLabelText("Enter the output image dimensions: ");
  qd.setTextValue("4096 4096");
  qd.setInputMode(QInputDialog::TextInput);
  if (qd.exec()==QInputDialog::Accepted)
  {
    std::istringstream istr(qd.textValue().toStdString());
    int w=100,h=100;
    istr>>w>>h;
    QPixmap qpm = dsglWidget->renderPixmap(w,h);
    std::cout<<"rendered "<<qpm.width()<<'x'<<qpm.height()<<std::endl;
    QString filename = dsGetSaveFileNameQS(("Save Screenshot"),("image format (*.jpg *.png)"),"");
    if (filename.isEmpty()) return;
    if (qpm.save(filename))
      std::cout<<"saved file."<<filename.toStdString()<<std::endl;
    else
      std::cerr<<"unable to save file "<<filename.toStdString()<<std::endl;
  }
}

void BrainSuiteWindow::savePortScreenShot(int portNo)
{
  if ((int)ports.size()<(portNo+1)) return;
  if (!ports[portNo]->image()) return;
  QString filename = dsGetSaveFileNameQS(("Save Image Screenshot"),("image format (*.jpg *.png)"),"");
  if (filename.isEmpty()) return;
  if (ports[portNo]->image()->blendBitmap.image.save(filename))
    std::cout<<"saved file."<<filename.toStdString()<<std::endl;
  else
    std::cerr<<"unable to save file "<<filename.toStdString()<<std::endl;
}

void BrainSuiteWindow::on_actionSave_Axial_Image_Screenshot_triggered()
{
  savePortScreenShot(1);
}

void BrainSuiteWindow::on_actionSave_Coronal_Image_Screenshot_triggered()
{
  savePortScreenShot(0);
}

void BrainSuiteWindow::on_actionSave_Sagittal_Image_Screenshot_triggered()
{
  savePortScreenShot(2);
}

void BrainSuiteWindow::setCerebrumAtlasFile()
{
  std::string filename = getOpenFilenameDS("Open Image Volume",analyzeOnlyImageFilter);
  if (filename.empty()) return;
  QFileInfo info(filename.c_str());
  if (info.exists())
  {
    QString filename = QDir::toNativeSeparators(QDir::cleanPath(info.filePath()));
    ces.cbm.atlasFilename = filename.toStdString();
    if (toolManager.cerebrumExtractionDialog)
      toolManager.cerebrumExtractionDialog->updateDialogFromCES();
    if (toolManager.corticalExtractionSequenceDialog)
      toolManager.corticalExtractionSequenceDialog->updateCerebroParameters();
  }
}

void BrainSuiteWindow::setCerebrumAtlasLabelFile()
{
  std::string filename = getOpenFilenameDS("Open Atlas Label Volume",analyzeOnlyImageFilter);
  if (filename.empty()) return;
  QFileInfo info(filename.c_str());
  if (info.exists())
  {
    QString filename = QDir::toNativeSeparators(QDir::cleanPath(info.filePath()));
    ces.cbm.labelFilename = filename.toStdString();
    if (toolManager.cerebrumExtractionDialog)
      toolManager.cerebrumExtractionDialog->updateDialogFromCES();
    if (toolManager.corticalExtractionSequenceDialog)
      toolManager.corticalExtractionSequenceDialog->updateCerebroParameters();
  }
}

std::string BrainSuiteWindow::getDirectory(std::string currentPath, std::string queryText)
{
  QString directory = QFileDialog::getExistingDirectory (this, queryText.c_str(), currentPath.c_str(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
  if (directory.isEmpty()) return std::string();
  QFileInfo info(directory);
  if (info.exists() && info.isDir())
  {
    QString directory = QDir::toNativeSeparators(QDir::cleanPath(info.filePath()));
    return directory.toStdString();
  }
  return std::string();
}

void BrainSuiteWindow::setCerebrumTempDirectory()
{
  std::string directory=getDirectory(QDir::homePath().toStdString(),"Select a temp directory for Cerebrum Extraction");
  if (directory.empty()==false)
  {
    ces.cbm.tempDirectory = directory;
    if (toolManager.cerebrumExtractionDialog)
      toolManager.cerebrumExtractionDialog->updateDialogFromCES();
    if (toolManager.corticalExtractionSequenceDialog)
      toolManager.corticalExtractionSequenceDialog->updateCerebroParameters();
  }

}

void BrainSuiteWindow::on_actionSave_Surfaceview_Screenshot_triggered()
{
  QImage im=dsglWidget->grabFrameBuffer();
  std::string filter=
      "image files (*.jpg *.png *.gif)";
  QString filename=dsGetSaveFileNameQS("Save Surfaceview Screenshot",filter.c_str(),"");
  if (filename.size())
    im.save(filename);
}

void BrainSuiteWindow::on_actionSave_Connectivity_Matrix_triggered()
{
  if (brainSuiteDataManager.connectivityMap.size())
    saveConnectivityMatrix();
  else
    std::cerr<<"connectivity matrix has not been computed"<<std::endl;
}

void BrainSuiteWindow::on_actionAdd_Custom_SVReg_Atlas_Directory_triggered()
{
  std::string customSVRegPath=getOpenFilenameDS("Select Custom Atlas Directory","SVReg Atlas Manifest Files (*.xml)");

}

std::string listFilter="Plain Text List Files (*.list *.txt);;All Files (*)";

void BrainSuiteWindow::on_actionLabel_Description_File_triggered()
{
  loadLabelDescriptionFile();
}

bool BrainSuiteWindow::loadLabelDescriptionFile()
{
  std::string filename = getOpenFilenameDS("Open Label Description File","Label Description Files (*.xml *.lbl)");
  WaitCursor waitCursor(this);
  return loadLabelDescriptionFile(filename);
}

bool BrainSuiteWindow::saveLabelDescriptionFile()
{
  QString filename = dsGetSaveFileNameQS("Save Label Description File","Label Description Files (*.xml *.lbl)","");
  WaitCursor waitCursor(this);
  return saveLabelDescriptionFile(filename.toStdString());
}

bool BrainSuiteWindow::saveLabelDescriptionFile(std::string labelDescriptionFilename)
{
  std::cout<<"saving "<<labelDescriptionFilename<<std::endl;
  return brainSuiteDataManager.labelDescriptionSet.write(labelDescriptionFilename);
}

bool BrainSuiteWindow::loadBSTFile()
{
  std::string filename = getOpenFilenameDS("Open BrainSuite Set File","BrainSuite Set Files (*.bst)");
  WaitCursor waitCursor(this);
  return loadBSTFile(filename);
}

#include <colormap.h>

extern ColorMap lutList;

bool BrainSuiteWindow::loadBSTFile(std::string bstFilename)
{
  disablePathChecking=true;
  bool loadedVolume=false;
  WaitCursor wc(this);
  QFileInfo fileInfo(bstFilename.c_str());
  std::ifstream ifile(bstFilename.c_str());
  if (!ifile) return false;
  addRecentFile(fileInfo.absoluteFilePath().toStdString()	);
  disableRecentFileList=true;
  char buffer[1024];
  std::string path(fileInfo.canonicalPath().toStdString());
  path+='/';
  bool shouldUpdate=false;
  bool updateDiffusionProperties=false;
  for (int i=0;i<10000;i++)
  {
    ifile.getline(buffer,sizeof(buffer));
    if (ifile.gcount()>0)
    {
      std::istringstream istr(buffer);
      std::string key,srcString;
      istr>>key>>std::ws>>srcString;
      if (StrUtil::eqnocase(key,"path"))
      {
        path = srcString;
        continue;
      }
      std::string filepath=srcString;
      if (filepath.empty()==false)
      {
        filepath=(filepath.at(0)=='/'||filepath.at(0)=='\\') ? filepath : path+filepath;
      }
      filepath = QDir::toNativeSeparators(QDir::cleanPath(filepath.c_str())).toStdString();
      if (StrUtil::eqnocase(key,"volume"))
      {
        std::cout<<"loading volume "<<filepath<<std::endl;
        loadVolume(filepath);
        loadedVolume=true;
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"label"))
      {
        std::cout<<"loading label "<<filepath<<std::endl;
        loadLabelVolume(filepath);
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"labeltext"))
      {
        std::cout<<"loading labeldesc "<<filepath<<std::endl;
        loadLabelDescriptionFile(filepath);
      }
      else if (StrUtil::eqnocase(key,"labeldesc"))
      {
        std::cout<<"loading labeldesc "<<filepath<<std::endl;
        loadLabelDescriptionFile(filepath);
      }
      else if (StrUtil::eqnocase(key,"overlay"))
      {
        std::cout<<"loading overlay "<<filepath<<std::endl;
        loadOverlay1Volume(filepath);
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"overlay2"))
      {
        std::cout<<"loading overlay2 "<<filepath<<std::endl;
        loadOverlay2Volume(filepath);
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"mask"))
      {
        std::cout<<"loading mask "<<filepath<<std::endl;
        loadMaskVolume(filepath);
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"surface"))
      {
        std::cout<<"loading surface "<<filepath<<std::endl;
        loadSurface(filepath); // add attributes as an argument
        shouldUpdate=true; //updateImages();
        updateSurfaceView();
      }
      else if (StrUtil::eqnocase(key,"curveset"))
      {
        std::cout<<"loading curveset "<<filepath<<std::endl;
        loadCurveSet(filepath); // add attributes as an argument
      }
      else if (StrUtil::eqnocase(key,"tractset"))
      {
        std::cout<<"loading tractset "<<filepath<<std::endl;
        loadFiberTrackSet(filepath); // add attributes as an argument
      }
      else if (StrUtil::eqnocase(key,"SHC"))
      {
        std::cout<<"loading SHC List "<<filepath<<std::endl;
        loadHARDISHC(path + srcString);
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"connectivity_matrix"))
      {
        //							std::cout<<"loading Connectivity Matrix "<<filepath<<std::endl;
        //							connectivityMap.readTSV(filename.toStdString());
        //							showDialog(toolManager.connectivityViewDialog);
      }
      else if (StrUtil::eqnocase(key,"compute_odf_tracks"))
      {
        if (toolManager.diffusionToolboxForm)
        {
          std::cout<<"computing tracks"<<std::endl;
          toolManager.diffusionToolboxForm->showTractographyPage();
          toolManager.diffusionToolboxForm->computeODFTracks();
        }
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"save_tracks"))
      {
        std::string name=path+srcString;
        std::cout<<"saving tracks: "<<name<<std::endl;
        if (!brainSuiteDataManager.fiberTrackSet->write(name))
        {
          std::cerr<<"unable to save "<<name<<std::endl;
        }
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"set_BSE_diffusion_iterations"))
      {
        ces.bse.diffusionIterations = QString(srcString.c_str()).toInt();
        if (toolManager.skullStrippingDialog)
          toolManager.skullStrippingDialog->updateDialogFromCES();
        if (toolManager.corticalExtractionSequenceDialog)
          toolManager.corticalExtractionSequenceDialog->updateBSEParameters();
      }
      else if (StrUtil::eqnocase(key,"set_BSE_diffusion_constant"))
      {
        ces.bse.diffusionConstant = QString(srcString.c_str()).toFloat();
        if (toolManager.skullStrippingDialog)
          toolManager.skullStrippingDialog->updateDialogFromCES();
        if (toolManager.corticalExtractionSequenceDialog)
          toolManager.corticalExtractionSequenceDialog->updateBSEParameters();
      }
      else if (StrUtil::eqnocase(key,"set_BSE_edge_constant"))
      {
        ces.bse.edgeConstant = QString(srcString.c_str()).toFloat();
        if (toolManager.skullStrippingDialog)
          toolManager.skullStrippingDialog->updateDialogFromCES();
        if (toolManager.corticalExtractionSequenceDialog)
          toolManager.corticalExtractionSequenceDialog->updateBSEParameters();
      }
      else if (StrUtil::eqnocase(key,"set_BFC_iterative"))
      {
        ces.bfc.iterative = srcString.empty() ? true : (QString(srcString.c_str()).toInt()!=0);
        if (toolManager.nonuniformityCorrectionDialog)
          toolManager.nonuniformityCorrectionDialog->updateDialogFromCES();
        if (toolManager.corticalExtractionSequenceDialog)
          toolManager.corticalExtractionSequenceDialog->updateBFCParameters();
      }
      else if (StrUtil::eqnocase(key,"run_cortical_extraction"))
      {
        showDialog(toolManager.corticalExtractionSequenceDialog);
        if (toolManager.corticalExtractionSequenceDialog)
          toolManager.corticalExtractionSequenceDialog->runAll();
      }
      else if (StrUtil::eqnocase(key,"set_sampling_slider"))
      {
        if (toolManager.diffusionToolboxForm)
        {
          viewSettings.blockSync = true;
          toolManager.diffusionToolboxForm->setSamplingSlider(atoi(srcString.c_str()));
          viewSettings.blockSync = false;
          updateDiffusionProperties=true;
        }
      }
      else if (StrUtil::eqnocase(key,"set_ODF_window"))
      {
        if (toolManager.diffusionToolboxForm)
        {
          viewSettings.blockSync = true;
          toolManager.diffusionToolboxForm->setODFWindow(atoi(srcString.c_str()));
          viewSettings.blockSync = false;
          updateDiffusionProperties=true;
        }
      }
      else if (StrUtil::eqnocase(key,"set_track_seed"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setTrackSeeding(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"set_stepsize"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setStepsize(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"set_max_steps"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setMaxSteps(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"set_angle_thresh"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setAngleThreshold(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"set_FA_thresh"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setFAThreshold(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"set_ODF_sampling"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setODFSampling(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"show_ODF_axial"))
      {
        viewSettings.blockSync = true;
        hardiRenderer.drawXY=(QString(srcString.c_str()).toInt()!=0);
        updateDiffusionProperties=true;
        viewSettings.blockSync = false;
      }
      else if (StrUtil::eqnocase(key,"show_ODF_coronal"))
      {
        viewSettings.blockSync = true;
        hardiRenderer.drawXZ=(QString(srcString.c_str()).toInt()!=0);
        updateDiffusionProperties=true;
        viewSettings.blockSync = false;
      }
      else if (StrUtil::eqnocase(key,"show_ODF_sagittal"))
      {
        viewSettings.blockSync = true;
        hardiRenderer.drawYZ=(QString(srcString.c_str()).toInt()!=0);
        updateDiffusionProperties=true;
        viewSettings.blockSync = false;
      }
      else if (StrUtil::eqnocase(key,"hide_surfaces"))
      {
        BrainSuiteSettings::showSurfaces=false;
        if (toolManager.surfaceDisplayForm) toolManager.surfaceDisplayForm->updateGeneralCheckboxes();
      }
      else if (StrUtil::eqnocase(key,"hide_volume"))
      {
        setShowVolume(false);
      }
      else if (StrUtil::eqnocase(key,"hide_overlay1"))
      {
        setShowOverlay1(false);
      }
      else if (StrUtil::eqnocase(key,"hide_overlay2"))
      {
        setShowOverlay2(false);
      }
      else if (StrUtil::eqnocase(key,"hide_labels"))
      {
        setShowLabels(false);
      }
      else if (StrUtil::eqnocase(key,"hide_mask"))
      {
        setShowMask(false);
      }
      else if (StrUtil::eqnocase(key,"set_GFA_thresh"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->setGFAThreshold(QString(srcString.c_str()).toFloat());
      }
      else if (StrUtil::eqnocase(key,"compute_connectivity"))
      {
        if (toolManager.diffusionToolboxForm)
          toolManager.diffusionToolboxForm->computeConnectivity();
      }
      else if (StrUtil::eqnocase(key,"save_connectivity"))
      {
        std::string name=path+srcString;
        std::cout<<"saving connectivity: "<<name<<std::endl;
        if (brainSuiteDataManager.connectivityMap.size())
        {
          if (!saveConnectivityMatrix(name))
            std::cerr<<"connectivity matrix could not be saved"<<std::endl;

        }
        else
          std::cerr<<"connectivity matrix has not been computed"<<std::endl;
        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"set_voxel_position"))
      {
        int x,y,z;
        x=::atoi(srcString.c_str());
        istr>>y>>z;
        std::cout<<"setting voxel position "<<x<<','<<y<<','<<z<<std::endl;
        viewSettings.blockSync = true;
        setVolumePosition(IPoint3D(x,y,z),true);
        viewSettings.blockSync = false;

        shouldUpdate=true; //updateImages();
      }
      else if (StrUtil::eqnocase(key,"set_zoom_level"))
      {
        viewSettings.blockSync = true;
        setZoomLevel(atof(srcString.c_str()));
        viewSettings.blockSync = false;
        shouldUpdate=true;
      }
      else if (StrUtil::eqnocase(key,"set_view_shift"))
      {
        if (dsglWidget)
        {
          float x=::atof(srcString.c_str());
          float y;
          istr>>y;
          dsglWidget->glViewController.properties.xShift=x;
          dsglWidget->glViewController.properties.yShift=y;
          std::cout<<"setting voxel shift "<<x<<','<<y<<std::endl;
          shouldUpdate=true; //updateImages();
        }
      }//set_view_zoom
      else if (StrUtil::eqnocase(key,"set_view_zoom"))
      {
        if (dsglWidget)
        {
          viewSettings.blockSync = true;
          dsglWidget->glViewController.properties.zoom=atof(srcString.c_str());
          viewSettings.blockSync = false;
          shouldUpdate=true;
        }
      }
      else if (StrUtil::eqnocase(key,"set_view_rotation"))
      {
        float x=::atof(srcString.c_str());
        float y,z;
        istr>>y>>z;
        viewSettings.blockSync = true;
        dsglWidget->glViewController.setRotation(x,y,z);
        viewSettings.blockSync = false;
      }
      else if (StrUtil::eqnocase(key,"exit"))
      {
        if (toolManager.connectivityViewDialog)
          toolManager.connectivityViewDialog->close();
        QApplication::quit();
      }
      else if (StrUtil::eqnocase(key,"popout_surfaceview"))
      {
        popoutSurfaceView=true;
        ui->action_Popout_Surface_View->setChecked(popoutSurfaceView);
        initializeSurfaceView();
      }
      else if (StrUtil::eqnocase(key,"set_surfaceview_res"))
      {
        int x=::atoi(srcString.c_str());
        int y;
        istr>>y;
        if (toolManager.dsglViewDialog)
          toolManager.dsglViewDialog->resize(x,y);
      }
      else if (StrUtil::eqnocase(key,"set_background"))
      {
        float r=::atof(srcString.c_str());
        float g,b;
        istr>>g>>b;
        if (dsglWidget)
          dsglWidget->setBackgroundColor(DSPoint(r,g,b));
      }
      else if (StrUtil::eqnocase(key,"save_surfaceview_screenshot"))
      {
        if (dsglWidget && srcString.empty()==false)
        {
          QImage image=dsglWidget->grabFrameBuffer();
          if (image.save(srcString.c_str()))
            std::cout<<"saved file "<<srcString<<std::endl;
          else
            std::cerr<<"unable to save file "<<srcString<<std::endl;
        }
      }
      else if (StrUtil::eqnocase(key,"load_extraction_settings"))
      {
      }
      else if (StrUtil::eqnocase(key,"LUT"))
      {
        lutList.loadCustomLUT(filepath);
      }
      else if (StrUtil::eqnocase(key,"set_LUT1"))
      {
        auto it = lutList.map.find(srcString);
        if (it != lutList.map.end())
        {
          imageLUT = it->second;
          updateImageDisplaySliders();
        }
        else
          std::cerr<<"LUT "<<srcString<<" is not loaded."<<std::endl;
      }
      else if (StrUtil::eqnocase(key,"set_LUT2"))
      {
        auto it = lutList.map.find(srcString);
        if (it != lutList.map.end())
        {
          imageOverlay1LUT = it->second;
          updateImageDisplaySliders();
        }
        else
          std::cerr<<"LUT "<<srcString<<" is not loaded."<<std::endl;
      }
      else if (StrUtil::eqnocase(key,"set_LUT3"))
      {
        auto it = lutList.map.find(srcString);
        if (it != lutList.map.end())
        {
          imageOverlay2LUT = it->second;
          updateImageDisplaySliders();
        }
        else
          std::cerr<<"LUT "<<srcString<<" is not loaded."<<std::endl;
      }
      else if (StrUtil::eqnocase(key,"set_LUT1_symmetric"))
      {
        if (toolManager.imageDisplayPropertiesForm)
          toolManager.imageDisplayPropertiesForm->setLUT1symmetric(true);
      }
      else if (StrUtil::eqnocase(key,"set_LUT2_symmetric"))
      {
        if (toolManager.imageDisplayPropertiesForm)
          toolManager.imageDisplayPropertiesForm->setLUT2symmetric(true);
      }
      else if (StrUtil::eqnocase(key,"set_LUT3_symmetric"))
      {
        if (toolManager.imageDisplayPropertiesForm)
          toolManager.imageDisplayPropertiesForm->setLUT3symmetric(true);
      }
    }
  }
  if (shouldUpdate) updateImages();
  if (updateDiffusionProperties)
  {
    if (toolManager.diffusionToolboxForm)
      toolManager.diffusionToolboxForm->updateDiffusionProperties();
  }
  setWindowTitle(QDir::toNativeSeparators(QDir::cleanPath(fileInfo.absoluteFilePath())));
  setCurrentDirectory(fileInfo.absoluteFilePath().toStdString());
  disableRecentFileList=false;
  disablePathChecking=false;
  if (loadedVolume) checkAutosavePath();
  return true;
}

DSPoint computeCentroid(ProtocolCurveSet *curveSet)
{ // computed from vertices
  if (!curveSet) return DSPoint(0,0,0);
  DSPoint curveCenter=DSPoint(0,0,0);
  for (std::vector<ProtocolCurve>::const_iterator curve=curveSet->curves.begin();curve!=curveSet->curves.end();curve++)
  {
    DSPoint cavg(0,0,0);
    for (std::vector<DSPoint>::const_iterator point=curve->points.begin();point!=curve->points.end();point++)
    {
      cavg+=*point;
    }
    curveCenter += cavg / (float)curve->points.size();
  }
  return curveCenter/(float)curveSet->curves.size();
}


bool BrainSuiteWindow::loadProtocolCurveset(std::string ifname)
{
  WaitCursor wc(this);
  if (!brainSuiteDataManager.protocolCurveSet)
    brainSuiteDataManager.protocolCurveSet = new ProtocolCurveSet;
  if (!brainSuiteDataManager.protocolCurveSet) return false;
  if (!brainSuiteDataManager.protocolCurveSet->read(ifname)) return false;
  addRecentFile(ifname);
  if (toolManager.curveToolboxForm)
    toolManager.curveToolboxForm->updateCurveList();
  brainSuiteDataManager.curveCentroid=computeCentroid(brainSuiteDataManager.protocolCurveSet);
  if (!dsglWidget)
    initializeSurfaceView();
  else
    updateSurfaceView();
  setCurrentDirectory(ifname);
  on_action_Curve_Tool_triggered();
  return true;
}


bool BrainSuiteWindow::saveCurveSet(std::string curvesetFilename)
{
  if (brainSuiteDataManager.protocolCurveSet==0) return false;
  if (curvesetFilename.empty()) return false;
  return  brainSuiteDataManager.protocolCurveSet->writeDFC(curvesetFilename);
}

bool BrainSuiteWindow::saveCurveSet()
{
  if (brainSuiteDataManager.protocolCurveSet==0) return false;
  std::string filename = dsGetSaveFileName("Save Curve Set","Curve Files (*.dfc)","");
  if (filename.empty()) return false;
  return saveCurveSet(filename);
}

bool BrainSuiteWindow::loadCurveSet(std::string curvesetFilename)
{
  WaitCursor waitCursor(this);
  return loadProtocolCurveset(curvesetFilename);
}

bool BrainSuiteWindow::loadCurveSet()
{
  std::string filename = getOpenFilenameDS("Open Curve Set","Curve Files (*.dfc *.ucf)");
  return (filename.empty()==false) ? loadProtocolCurveset(filename) : false;
}

bool BrainSuiteWindow::loadCustomCerebrumMask(std::string maskFilename)
{
  if (corticalExtractionData.vCerebrumMask.read(maskFilename))
  {
    return true;
  }
  else
    return false;
}

bool BrainSuiteWindow::loadCurveProtocol(std::string curveProtocolFilename)
{
  WaitCursor waitCursor(this);
  if (!brainSuiteDataManager.protocolCurveSet)
    brainSuiteDataManager.protocolCurveSet = new ProtocolCurveSet;
  if (!brainSuiteDataManager.protocolCurveSet) return false;
  if (!brainSuiteDataManager.protocolCurveSet->readXML(curveProtocolFilename)) return false;
  if (toolManager.curveToolboxForm)
  {
    toolManager.curveToolboxForm->updateCurveList();
    toolManager.curveToolboxForm->updateProtocol();
  }
  brainSuiteDataManager.curveCentroid=DSPoint(0,0,0);
  setCurrentDirectory(curveProtocolFilename);
  return true;
}

bool BrainSuiteWindow::loadCurveProtocol()
{
  std::string filename = getOpenFilenameDS("Open Curve Protocol","Curve Protocl Files (*.xml)");
  return (filename.empty()==false) ? loadCurveProtocol(filename) : false;
}

void BrainSuiteWindow::on_action_OpenCurveset_triggered()
{
  loadCurveSet();
}

void BrainSuiteWindow::on_action_Open_Overlay_Volume_triggered()
{
  loadOverlay1Volume();
}

void BrainSuiteWindow::on_action_Overlay2_Volume_triggered()
{
  loadOverlay2Volume();
}

void BrainSuiteWindow::on_action_Mask_Volume_triggered()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  loadMaskVolume();
}

void BrainSuiteWindow::on_action_Label_Volume_triggered()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  loadLabelVolume();
}

std::string getBasename(std::string filename)
{
  std::string basename=::StrUtil::extStrip(filename,"gz");
  if (StrUtil::hasExtension(basename,".hdr")) basename=StrUtil::extStrip(basename,"hdr");
  else if (StrUtil::hasExtension(basename,".img")) basename=StrUtil::extStrip(basename,"img");
  else if (StrUtil::hasExtension(basename,".nii")) basename=StrUtil::extStrip(basename,"nii");
  return basename;
}
void BrainSuiteWindow::on_actionColormap_LUT_triggered()
{
  std::string ifname=getOpenFilenameDS("Load Colormap LUT","LUT Files (*.lut)");
  if (!ifname.empty())
  {
    lutList.loadCustomLUT(ifname);
  }
}

bool BrainSuiteWindow::loadUCF(std::string ifname)
{
  WaitCursor wc(this);
  if (!brainSuiteDataManager.protocolCurveSet)
    brainSuiteDataManager.protocolCurveSet = new ProtocolCurveSet;
  if (!brainSuiteDataManager.protocolCurveSet) return false;
  if (!brainSuiteDataManager.protocolCurveSet->readUCF(ifname)) return false;
  addRecentFile(ifname);
  if (!dsglWidget)
    initializeSurfaceView();
  return true;
}


void BrainSuiteWindow::on_actionImage_Volume_triggered()
{
  QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
  loadVolume();
  return;
}


bool BrainSuiteWindow::loadHARDISHC()
{
  std::string filename = getOpenFilenameDS("Open HARDI SHC Volumes","HARDI SHC List Files (*.odf *.shc)");
  WaitCursor waitCursor(this);
  loadHARDISHC(filename);
  return false;
}

void BrainSuiteWindow::on_action_HARDI_SHC_Data_triggered()
{
  loadHARDISHC();
}

void BrainSuiteWindow::on_action_Fiber_Tract_Set_triggered()
{
  loadFiberTrackSet();
}

bool BrainSuiteWindow::loadFiberTrackSet()
{
  std::string filename = getOpenFilenameDS("Open Fiber Trakt Set","Track Files (*.trk *.dfc *.dft)");
  WaitCursor waitCursor(this);
  return loadFiberTrackSet(filename);
}


void BrainSuiteWindow::on_action_BST_File_triggered()
{
  loadBSTFile();
}

bool BrainSuiteWindow::loadExtractionStudy(CorticalExtractionData::Stages state)
{
  QFileDialog fileDialog;
  fileDialog.setFileMode(QFileDialog::DirectoryOnly);
  std::string ifname = getOpenFilenameDS("Open Image Volume","3D Image Volume Files (*.img *.nii *.img.gz *.nii.gz)");
  if (ifname.empty()) return false;
  WaitCursor waitCursor(this);
  std::cout<<ifname<<std::endl;
  std::string basename = getBasename(ifname);
  std::cout<<"working with "<<basename<<" as basename."<<std::endl;

  std::ostringstream errstring;
  if (corticalExtractionData.innerCorticalSurface)
    brainSuiteDataManager.removeSurface(corticalExtractionData.innerCorticalSurface);
  if (corticalExtractionData.pialSurface)
    brainSuiteDataManager.removeSurface(corticalExtractionData.pialSurface);
  if (!corticalExtractionData.load(basename,errstring,state))
  {
    std::string error;
    if (errstring.str().empty())
      error="An unspecified error occurred while trying to load previous study.";
    else
      error="An error occurred while trying to load previous study:\n\n" + errstring.str();
    QMessageBox msgBox(this);
    msgBox.setText(error.c_str());
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    return false;
  }
  bool showResults=true;
  if (corticalExtractionData.vBFC)
  {
    brainSuiteDataManager.addVolume(corticalExtractionData.vBFC);
    corticalExtractionData.vBFC=0;
    updateVolumeView(true);
  }
  if (corticalExtractionData.innerCorticalSurface)
    brainSuiteDataManager.addSurface(corticalExtractionData.innerCorticalSurface,false);
  if (corticalExtractionData.pialSurface)
    brainSuiteDataManager.addSurface(corticalExtractionData.pialSurface,false);
  updateSurfaceDisplayLists();
  if (showResults)
  {
    QMessageBox msgBox(this);
    msgBox.setText(("Loaded previous study:\n"+corticalExtractionData.loadStream.str()).c_str());
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
  }
  return true;
}

