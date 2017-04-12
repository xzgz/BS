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
#include "harditool.h"
#include <DS/timer.h>
#include <htracker.h>
#include <fibertrackset.h>
#include <waitcursor.h>
#include <trackfilter.h>
#include <connectivityviewdialog.h>
#include <cmath>
#include <qcolordialog.h>
#include <qevent.h>
#include <brainsuiteqtsettings.h>
#include <qfiledialog.h>
#include <findodfmaximathread.h>
#include <processingthreaddialog.h>
#include <qthread.h>
#include <qdebug.h>
#include <fibertrackerthread.h>
#include <brainsuitewindow_t.h>
#include <hardirenderer.h>
#include <findodfmaximamanager.h>
#include <alignfull.h>
#include <vectorinterpolator.h>
#include <rungekutta4.h>

extern HARDITool hardiTool;
extern HARDIRenderer hardiRenderer;

class ODFProcessingThread : public ProcessingThread {
public:
  ODFProcessingThread(ProcessingThreadDialog *parent, FindODFMaximaManager &manager) : ProcessingThread(parent,"Find ODF Maxima"), manager(manager) {}
  FindODFMaximaManager &manager;
  int progressPosition(){ return manager.currentPosition(); }
  int progressMaximum() { return manager.nSlices(); }
  bool stop() { manager.stop(); return true; }
  void run()  { manager.launch(0); }
};

class FiberTrackingThread : public ProcessingThread {
public:
  FiberTrackingThread(ProcessingThreadDialog *parent, FiberTrackerThreadManager &manager) : ProcessingThread(parent,"Fiber Tracking"), manager(manager) {}
  FiberTrackerThreadManager &manager;
  int progressPosition(){ return manager.currentSlice(); }
  int progressMaximum() { return manager.nSlices(); }
  bool stop() { manager.stop(); return true; }
  void run()  { manager.launch(0); }
};

inline float fractionalAnisotropy(const EigenSystem3x3f &e)
{
  return sqrtf(0.5 * (square(e.l0-e.l1)+square(e.l1-e.l2)+square(e.l2-e.l0)) /(e.l0*e.l0+e.l1*e.l1+e.l2*e.l2));
}

//TODO: move to separate file
float computeGlyphMaxEigenvector(const Vol3DBase *vol, const float evLimit, const float percentile, const size_t nBins)
// returns ratio of maximum tensor radius / voxel size
{
  float result=1;
  if (vol->typeID()!=SILT::Eigensystem3x3f) return result;
  Vol3D<EigenSystem3x3f> &v =*(Vol3D<EigenSystem3x3f> *)vol;
  const int ds = v.size();
  if (!ds) return result;
  float maxEV = std::abs(v[0].l0);
  int mid=0;
  for (int i=1;i<ds;i++)
  {
    if (std::abs(v[i].l0)<evLimit)
      if (std::abs(v[i].l0) > maxEV) { mid=i; maxEV = std::abs(v[i].l0); }
  }
  if (maxEV<=0) return result;
  std::vector<int> count(nBins); // quantize it
  size_t nv=0;
  const float scale=nBins/maxEV;
  for (int i=0;i<ds;i++)
  {
    const float ev0=std::abs(v[i].l0);
    if (ev0==0) continue;
    nv++;
    const size_t v=(size_t)(ev0*scale);
    const size_t id=(v<nBins) ? v : nBins-1;
    count[id]++;
  }
  float thresh=nv*percentile;
  size_t ct=0;
  size_t cut=nBins;
  for (size_t i=0;i<nBins;i++)
  {
    ct += count[i];
    if (ct>thresh)
    {
      cut=i;
      break;
    }
  }
  float scaledMaxEV=maxEV;
  if (cut<nBins)
  {
    float scale=(float)cut/(float)nBins;
    scaledMaxEV=maxEV*scale;
  }
  return scaledMaxEV;
}

float computeGlyphScaleFactor(Vol3DBase *vol)
// returns ratio of maximum tensor radius / voxel size
{
  if (vol->typeID()!=SILT::Eigensystem3x3f) return 1.0f;
  const int nBins=100000;
  float result=1;
  Vol3D<EigenSystem3x3f> &v =*(Vol3D<EigenSystem3x3f> *)vol;
  const int ds = v.size();
  if (ds<=0) return result;
  float upperLimit=10e30;
  int pass=1;
  const float percentile=0.95f;
  float maximumEV=computeGlyphMaxEigenvector(vol,upperLimit,percentile,nBins);
  while (maximumEV<=0)
  {
    pass++;
    upperLimit/=nBins;
    maximumEV=computeGlyphMaxEigenvector(vol,upperLimit,percentile,nBins);
    if (pass>10) { maximumEV=1; break; }
  }
  float mv = std::max(std::max(v.rx,v.ry),v.rz);
  if (mv<=0) mv = 1.0f;
  result = mv/maximumEV;
  return result;
}

void setSliderPosition(QSlider *slider, int pos)
{
  slider->setTracking(false);
  slider->setSliderPosition(pos);
  slider->setTracking(true);
}

DiffusionToolboxForm::DiffusionToolboxForm(BrainSuiteWindow *brainSuiteWindow, QWidget *parent) :
  QWidget(parent),
  sphereColor(1,0.3f,0.3f),
  sphere2Color(0.3f,0.3f,1.0f),
  ui(new Ui::DiffusionToolboxForm),
  brainSuiteWindow(brainSuiteWindow),
  tubeSliderScale(100.0f),
  ellipseRadiusSliderScale(10.0f),
  odfScaleSliderScale(100.0f),
  downsampleFactor(0),
  initializing(false),
  blocking(false)
{
  ui->setupUi(this);
  initializing=true;
  ui->downsampleFactorEditBox->setText("100");
  ui->downsampleTractsCheckBox->setChecked(false);
  // diffusion tensor display
  ui->ellipseScaleSlider->setRange(0,10000);
  ui->glyphQualityComboBox->blockSignals(true);
  ui->glyphQualityComboBox->insertItem(0,"cuboid (6 sides)");
  ui->glyphQualityComboBox->insertItem(1,"low (20 sides)");
  ui->glyphQualityComboBox->insertItem(2,"medium (80 sides)");
  ui->glyphQualityComboBox->insertItem(3,"high (320 sides)");
  ui->glyphQualityComboBox->blockSignals(false);
  updateDiffusionProperties(); // from BrainSuiteSettings
  // HARDI ODF
  ui->coefficientSlider->blockSignals(true);
  ui->coefficientSlider->setRange(1,hardiTool.filecount);
  ui->coefficientSlider->setValue(hardiTool.odfFactory.nCoefficients);
  ui->coefficientSlider->blockSignals(false);
  ui->coefficientText->setText(QString::number(hardiTool.odfFactory.nCoefficients));

  ui->odfScaleSlider->setTracking(false);
  ui->odfScaleSlider->setRange(0,1000);
  ui->odfScaleSlider->setValue(1.0 * odfScaleSliderScale);
  ui->odfScaleSlider->setTracking(true);

  ui->samplingSlider->setTracking(false);
  ui->samplingSlider->setValue(hardiTool.samplingRes);
  ui->samplingSlider->setRange(3,50);
  ui->samplingSlider->setTracking(true);
  ui->samplingText->setText(QString::number(hardiTool.samplingRes));

  ui->windowSlider->setTracking(false);
  ui->windowSlider->setRange(0,100);
  ui->windowSlider->setValue(hardiRenderer.radius);
  ui->windowSlider->setTracking(true);
  updateGlyphs(true);
  ui->odfScaleLineEdit->setText(QString::number(hardiTool.odfFactory.scale));
  ui->samplingText->setText(QString::number(hardiTool.samplingRes));
  ui->windowSizeText->setText(QString::number(hardiRenderer.radius));
  ui->coefficientText->setText(QString::number(hardiTool.odfFactory.nCoefficients));

  ui->odfMinMaxCheckBox->setChecked(hardiTool.odfFactory.normalize);

  ui->odfSamplingLineEdit->setText(QString::number(20));
  ui->tractSeedingLineEdit->setText(QString::number(1.0f));
  ui->stepsizeLineEdit->setText(QString::number(0.25f));
  ui->maxStepsLineEdit->setText(QString::number(500));
  ui->angleThresholdLineEdit->setText(QString::number(10.0f));
  ui->faThresholdLineEdit->setText(QString::number(0.05f));
  ui->gfaThresholdLineEdit->setText(QString::number(0.01f));
  ui->tractographyStatusBox->setText("");
  ui->renderFibersAsTubesCheckBox->setChecked(BrainSuiteSettings::showFibersAsTubes);
  ui->tubeRadiusEditBox->setText(QString::number(BrainSuiteSettings::tubeSize,'f',2));
  ui->tubeRadiusSlider->setRange(0,250);
  ui->tubeRadiusSlider->setSliderPosition(BrainSuiteSettings::tubeSize*tubeSliderScale);
  ui->renderFibersAsTubesCheckBox->setChecked(BrainSuiteSettings::showFibersAsTubes);

  ui->enableTrackClippingCheckBox->setChecked(BrainSuiteSettings::clipFibers);

  initTrackFilterPage();
  ui->lineWidthLineEdit->setText(QString::number(BrainSuiteSettings::fiberLineWidth));
  ui->trackDisplayColorModelComboBox->insertItem(0,"Solid");
  ui->trackDisplayColorModelComboBox->insertItem(1,"Average Angle");
  ui->trackDisplayColorModelComboBox->insertItem(2,"Local Angle");
  ui->trackDisplayColorModelComboBox->setCurrentIndex(1);
  ui->trackDisplayColorSelectionButton->setColor(BrainSuiteSettings::defaultFiberColor);
  ui->multithreadCheckBox->setChecked(true);
  initializing=false;
}

DiffusionToolboxForm::~DiffusionToolboxForm()
{
  delete ui;
}

void DiffusionToolboxForm::updateImages()
{
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateImages();
    brainSuiteWindow->updateSurfaceView();
  }
}

void DiffusionToolboxForm::setSamplingSlider(const int n)
{
  on_samplingSlider_valueChanged(n);
}

void DiffusionToolboxForm::setODFWindow(const int n)
{
  on_windowSlider_valueChanged(n);
}

void DiffusionToolboxForm::setODFSampling(const int n)
{
  ui->odfSamplingLineEdit->setText(QString::number(n));
}
void DiffusionToolboxForm::setTrackSeeding(const float f)
{
  ui->tractSeedingLineEdit->setText(QString::number(f));
}
void DiffusionToolboxForm::setStepsize(const float f)
{
  ui->stepsizeLineEdit->setText(QString::number(f));
}
void DiffusionToolboxForm::setMaxSteps(const int n)
{
  ui->maxStepsLineEdit->setText(QString::number(n));
}
void DiffusionToolboxForm::setAngleThreshold(const float f)
{
  ui->angleThresholdLineEdit->setText(QString::number(f));
}
void DiffusionToolboxForm::setFAThreshold(const float f)
{
  ui->faThresholdLineEdit->setText(QString::number(f));
}

void DiffusionToolboxForm::setGFAThreshold(const float f)
{
  ui->gfaThresholdLineEdit->setText(QString::number(f));
}

QSize DiffusionToolboxForm::sizeHint() const
{
  return QSize(320,600);
}

bool DiffusionToolboxForm::selectDTITracking()
{
  ui->computeFromODFsCheckBox->setCheckState(Qt::Unchecked);
  ui->computeFromTensorsCheckBox->setCheckState(Qt::Checked);
  return true;
}

bool DiffusionToolboxForm::selectODFTracking()
{
  ui->computeFromODFsCheckBox->setCheckState(Qt::Checked);
  ui->computeFromTensorsCheckBox->setCheckState(Qt::Unchecked);
  return true;
}

void DiffusionToolboxForm::resetDiffusionScale()
{
  if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume
      && brainSuiteWindow->brainSuiteDataManager.volume->typeID()==SILT::Eigensystem3x3f)
  {
    const float radius = computeGlyphScaleFactor(brainSuiteWindow->brainSuiteDataManager.volume);
    BrainSuiteSettings::ellipseRadius = radius;
    float maxR = 10*radius;
    ui->ellipseScaleSlider->setTracking(false);
    ui->ellipseScaleSlider->setValue(ellipseRadiusSliderScale * BrainSuiteSettings::ellipseRadius);
    ui->ellipseScaleSlider->setTracking(true);
    ellipseRadiusSliderScale=10000.0f/maxR;
  }
}

void DiffusionToolboxForm::updateDiffusionProperties()
{
  if (ui->computeFromTensorsCheckBox->isChecked()==false && ui->computeFromODFsCheckBox->isChecked()==false)
  {
    if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume
        && brainSuiteWindow->brainSuiteDataManager.volume->typeID()==SILT::Eigensystem3x3f)
    {
      ui->computeFromTensorsCheckBox->setChecked(true);
    }
    else
    {
      if (hardiTool.vODF.size()>0)
        ui->computeFromTensorsCheckBox->setChecked(true);
    }
  }
  ui->showTensorGlyphsCheckBox->setChecked(BrainSuiteSettings::showTensors);
  ui->showTensorsUsingColorFA->setChecked(BrainSuiteSettings::useColorFA);
  ui->ellipseScaleSlider->setTracking(false);
  ui->ellipseScaleSlider->setValue(ellipseRadiusSliderScale * BrainSuiteSettings::ellipseRadius);
  ui->ellipseScaleSlider->setTracking(true);
  ui->showDTI_XYPlaneCheckBox->setChecked(BrainSuiteSettings::drawTensorsXY);
  ui->showDTI_XZPlaneCheckBox->setChecked(BrainSuiteSettings::drawTensorsXZ);
  ui->showDTI_YZPlaneCheckBox->setChecked(BrainSuiteSettings::drawTensorsYZ);
  ui->ellipseScaleText->setText(QString::number(BrainSuiteSettings::ellipseRadius));
  ui->glyphQualityComboBox->blockSignals(true);
  ui->glyphQualityComboBox->setCurrentIndex(BrainSuiteSettings::glyphQuality);
  ui->glyphQualityComboBox->blockSignals(false);

  // HARDI
  ui->showODFGlyphsCheckBox->setChecked(BrainSuiteSettings::showODFs);
  ui->clipGlyphsCheckBox->setChecked(BrainSuiteSettings::clipODFs);
  ui->showXYPlaneCheckBox->setChecked(hardiRenderer.drawXY);
  ui->showXZPlaneCheckBox->setChecked(hardiRenderer.drawXZ);
  ui->showYZPlaneCheckBox->setChecked(hardiRenderer.drawYZ);
  ui->flipXCheckBox->setChecked(hardiTool.odfFactory.flipX);
  ui->flipYCheckBox->setChecked(hardiTool.odfFactory.flipY);
  ui->flipZCheckBox->setChecked(hardiTool.odfFactory.flipZ);
  if (ui->coefficientSlider->maximum()!=hardiTool.filecount)
  {
    ui->coefficientSlider->blockSignals(true);
    ui->coefficientSlider->setRange(1,hardiTool.filecount);
    ui->coefficientSlider->setValue(hardiTool.odfFactory.nCoefficients);
    ui->coefficientSlider->blockSignals(false);
  }
  ui->windowSlider->blockSignals(true);
  ui->windowSlider->setValue(hardiRenderer.radius);
  ui->windowSlider->blockSignals(false);
  ui->coefficientText->setText(QString::number(hardiTool.odfFactory.nCoefficients));
}

void DiffusionToolboxForm::showDTIPage()
{
  ui->toolBox->setCurrentIndex(0);
}

void DiffusionToolboxForm::showHARDIPage()
{
  ui->toolBox->setCurrentIndex(1);
}

void DiffusionToolboxForm::showTractographyPage()
{
  ui->toolBox->setCurrentIndex(2);
}

void DiffusionToolboxForm::showTrackDisplayPage()
{
  ui->toolBox->setCurrentIndex(3);
}

void DiffusionToolboxForm::showTrackFilteringPage()
{
  ui->toolBox->setCurrentIndex(3);
}

void DiffusionToolboxForm::showConnectivityPage()
{
  ui->toolBox->setCurrentIndex(3);
}

void DiffusionToolboxForm::mouseMoveTo(ImageWidget * /*port*/, const IPoint3D &position, QMouseEvent * event)
{
  bool ctrl=(event) ? ((event->modifiers()&Qt::ControlModifier)!=0) : false;
  if (ctrl)
    moveSelectedROIto(position);
}

void DiffusionToolboxForm::on_ellipseScaleSlider_valueChanged(int value)
{
  BrainSuiteSettings::ellipseRadius = value/ellipseRadiusSliderScale;
  int p = BrainSuiteSettings::ellipseRadius*ellipseRadiusSliderScale;
  ui->ellipseScaleSlider->setSliderPosition(p);
  ui->ellipseScaleText->setText(QString::number(BrainSuiteSettings::ellipseRadius));
  brainSuiteWindow->updateSurfaceView();
}

bool DiffusionToolboxForm::updateGlyphs(bool recomputeBasis)
{
  if (brainSuiteWindow)
  {
    hardiRenderer.updateGlyphs(brainSuiteWindow->brainSuiteDataManager.vMask,recomputeBasis);
    return true;
  }
  return false;
}

void DiffusionToolboxForm::on_samplingSlider_valueChanged(int value)
{
  hardiTool.samplingRes = value;
  updateGlyphs(true);
  ui->samplingText->setText(QString::number(hardiTool.samplingRes));
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_windowSlider_valueChanged(int value)
{
  hardiRenderer.radius = value;
  updateGlyphs(false);
  ui->windowSizeText->setText(QString::number(hardiRenderer.radius));
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_coefficientSlider_valueChanged(int value)
{
  hardiTool.setMaxCoefficient(value);
  updateGlyphs(true);
  ui->coefficientText->setText(QString::number(hardiTool.odfFactory.nCoefficients));
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_flipXCheckBox_clicked()
{
  hardiTool.odfFactory.flipX = ui->flipXCheckBox->isChecked();
  updateGlyphs(true);
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_flipYCheckBox_clicked()
{
  hardiTool.odfFactory.flipY = ui->flipYCheckBox->isChecked();
  updateGlyphs(true);
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_flipZCheckBox_clicked()
{
  hardiTool.odfFactory.flipZ = ui->flipZCheckBox->isChecked();
  updateGlyphs(true);
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showXYPlaneCheckBox_clicked()
{
  hardiRenderer.drawXY=ui->showXYPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showXZPlaneCheckBox_clicked()
{
  hardiRenderer.drawXZ=ui->showXZPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showYZPlaneCheckBox_clicked()
{
  hardiRenderer.drawYZ=ui->showYZPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showODFGlyphsCheckBox_clicked()
{
  if (brainSuiteWindow)
  {
    BrainSuiteSettings::showODFs =ui->showODFGlyphsCheckBox->isChecked();
    brainSuiteWindow->updateSurfaceView();
  }
}

bool DiffusionToolboxForm::computeFiberTracks(FiberTrackSet &fiberTrackSet, Vol3D<EigenSystem3x3f> &vEigIn, const Vol3D<uint8> &vMask,
                                              const float vStep, float stepsize, int nSteps, const float angleThresh)
{
  WaitCursor wc(this);
  Timer timer;
  timer.start();
  const float fibertrackDotProductThresh=std::cos(angleThresh*M_PI/180);//angleThresh
  if (ui->multithreadCheckBox->isChecked())
  {
    FiberTrackerThreadManager fttm(fiberTrackSet,vEigIn,vMask,vStep,stepsize,nSteps,angleThresh);
    ProcessingThreadDialog dialog(this);
    if (dialog.launch(new FiberTrackingThread(&dialog,fttm))==QDialog::Rejected)
    {
    }
    else
    {
      if (ui->tractographySortTracksCheckBox->isChecked())
      {
        fiberTrackSet.sortFiberTracks();
      }
    }
  }
  else
  {
    const float xStepV=vStep;
    const float yStepV=vStep;
    const float zStepV=vStep;
    HTrackerBase *tracker=new HTracker<VectorInterpolator<AlignFull>,RungeKutta4>();
    if (!tracker) return false;
    tracker->stepsize = stepsize;
    tracker->nSteps = nSteps;
    tracker->voxels = true;
    EigenSystem3x3f blank;
    Vol3D<EigenSystem3x3f> vEig;
    vEig.makeCompatible(vEigIn);
    const int cx = vEig.cx;
    const int cy = vEig.cy;
    const int cz = vEig.cz;
    bool mask = vMask.isCompatible(vEig);
    if (mask)
    {
      const int ds = vMask.size();
      for (int i=0;i<ds;i++) vEig[i] = (vMask[i] && vEigIn[i].l0!=0) ? vEigIn[i] : blank;
    }
    else
    {
      const int ds = vMask.size();
      for (int i=0;i<ds;i++) vEig[i] = (vEigIn[i].l0!=0) ? vEigIn[i] : blank;
    }
    const float xstep=vEig.rx*xStepV;
    const float ystep=vEig.ry*yStepV;
    const float zstep=vEig.rz*zStepV;
    int checked=0;
    const float zMin=0;
    const float zMax=(float)(cz-1);
    ui->tractographyStatusBox->setText("computing fiber tracks");
    ui->tractographyStatusBox->repaint();
    for (float z=zMin;z<=zMax;z+=zstep)
    {
      int nc = 0;
      for (float y=0;y<cy;y+=ystep)
      {
        for (float x=0;x<cx;x+=xstep)
        {
          IPoint3D ip((int)ceil(x),(int)ceil(y),(int)ceil(z));
          if (mask && !vMask(ip)) continue;
          std::list<DSPoint> tract;
          tracker->voxels = true;
          DSPoint	seedPoint = DSPoint((float)x,(float)y,(float)z);
          checked++;
          int seedIndex=-1;
          tracker->track(tract,seedIndex,vEig,seedPoint,fibertrackDotProductThresh);
          if (tract.size()>1)
          {
            nc++;
            fiberTrackSet.newCurve();
            fiberTrackSet.current().attributes.color = DSPoint(1,0,0);
            fiberTrackSet.current().seedIndex=seedIndex;
            std::vector<DSPoint> &points(fiberTrackSet.current().points);
            fiberTrackSet.current().points.reserve(tract.size());
            for (auto i : tract) points.push_back(i);
            DSPoint color;
            for (size_t i=1;i<points.size();i++)
            {
              DSPoint dp=points[i]-points[i-1];
              color += abs(dp);
            }
            fiberTrackSet.current().attributes.color = color.unit();
          }
        }
      }
      if (nc>0)
      {
        std::ostringstream ostr; ostr<<"slice "<<z<<" : "<<nc<<" curves";
        ui->tractographyStatusBox->setText(ostr.str().c_str());
        ui->tractographyStatusBox->repaint();
      }
    }
  }
  timer.stop();
  std::ostringstream ostr;
  ostr<<"made "<<fiberTrackSet.curves.size()<<" tracts in "<<timer.elapsed();
  ui->tractographyStatusBox->setText(ostr.str().c_str());
  ui->tractographyStatusBox->repaint();
  return true;
}

bool DiffusionToolboxForm::sampleODFs(Vol3D<EigenSystem3x3f> &vEig, HARDITool &hardiTool, Vol3D<uint8> &vMask)
{
  WaitCursor wc(this);
  if (!hardiTool.filecount) { ui->tractographyStatusBox->setText("No ODF files have been loaded."); return false; }
  vEig.makeCompatible(hardiTool.vODF);
  const int cx=vEig.cx;
  const int cy=vEig.cy;
  const int cz=vEig.cz;
  bool useMask = vMask.isCompatible(vEig);
  Timer t; t.start();
  ui->tractographyStatusBox->setText("computing ODF maxima");
  ui->tractographyStatusBox->repaint();
  if (ui->multithreadCheckBox->isChecked())
  {
    ProcessingThreadDialog dialog(this);
    FindODFMaximaManager mrManager(vEig,vMask,hardiTool);
    if (dialog.launch(new ODFProcessingThread(&dialog,mrManager))==QDialog::Rejected)
    {
      vEig.setsize(0,0,0);
      return false;
    }
  }
  else
  {
    for (int z=0;z<cz;z++)
    {
      for (int y=0;y<cy;y++)
      {
        for (int x=0;x<cx;x++)
          if (!useMask || vMask(x,y,z))
          {
            vEig(x,y,z)=hardiTool.findODFMaxima(x,y,z);
          }
      }
    }
  }
  t.stop();
  std::ostringstream ostr;
  ostr<<"odf maxima computation took "<<t.elapsed();
  ui->tractographyStatusBox->setText(ostr.str().c_str());
  return true;
}

DSPoint computeCenter(FiberTrackSet *fiberTrackSet)
{
// use centroid of points rather than centroid of segments
  if (!fiberTrackSet) return DSPoint(0,0,0);
  DSPoint fiberCenter=DSPoint(0,0,0);
  for (std::vector<FiberTrack>::const_iterator fiber=fiberTrackSet->curves.begin();fiber!=fiberTrackSet->curves.end();fiber++)
  {
    DSPoint cavg(0,0,0);
    for (std::vector<DSPoint>::const_iterator point=fiber->points.begin();point!=fiber->points.end();point++)
    {
      cavg+=*point;
    }
    fiberCenter += cavg / (float)fiber->points.size();
  }
  return fiberCenter/(float)fiberTrackSet->curves.size();
}

float DiffusionToolboxForm::getLengthThreshold()
{
  return ui->tractLengthEdit->text().toFloat();
}

bool DiffusionToolboxForm::computeODFTracks()
{
  if (brainSuiteWindow->brainSuiteDataManager.vODFEig.size()==0) { on_findODFMaximaPushButton_clicked(); }
  if (brainSuiteWindow->brainSuiteDataManager.vODFEig.size()==0) return false;
  Vol3D<uint8> vMask;
  const float magnitudeThreshold=ui->gfaThresholdLineEdit->text().toFloat();
  if (magnitudeThreshold>0)
  {
    vMask.makeCompatible(brainSuiteWindow->brainSuiteDataManager.vODFEig);
    const int ds=brainSuiteWindow->brainSuiteDataManager.vODFEig.size();
    for (int i=0;i<ds;i++)
      vMask[i] = (brainSuiteWindow->brainSuiteDataManager.vODFEig[i].l0>=magnitudeThreshold) ? 255 : 0;
  }
  else
    vMask.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
  FiberTrackSet *fiberTrackSet = new FiberTrackSet;
  if (fiberTrackSet)
  {
    const float stepsize=ui->stepsizeLineEdit->text().toFloat();
    const int nSteps=ui->maxStepsLineEdit->text().toInt();
    float angleThreshold=ui->angleThresholdLineEdit->text().toFloat();
    float seedingRate = ui->tractSeedingLineEdit->text().toFloat();
    if (seedingRate<=0.1f) seedingRate=0.1f;
    if (seedingRate>5.0f) seedingRate=5.0f;
    ui->tractSeedingLineEdit->setText(QString::number(seedingRate));
    if (computeFiberTracks(*fiberTrackSet,brainSuiteWindow->brainSuiteDataManager.vODFEig,vMask,1.0f/seedingRate,stepsize,nSteps,angleThreshold))
    {
      if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet)
        delete brainSuiteWindow->brainSuiteDataManager.fiberTrackSet;
      brainSuiteWindow->brainSuiteDataManager.fiberTrackSet = fiberTrackSet;
      applyCurrentFilters();
      brainSuiteWindow->initializeSurfaceView();
      brainSuiteWindow->updateSurfaceView();
    }
    else
      delete fiberTrackSet;
    brainSuiteWindow->brainSuiteDataManager.fiberCentroid=computeCenter(fiberTrackSet);
    return true;
  }
  return false;
}

void DiffusionToolboxForm::on_computeTractsPushButton_clicked()
{
  if (ui->computeFromODFsCheckBox->isChecked())
  {
    if (brainSuiteWindow->brainSuiteDataManager.vODFEig.size()==0) { on_findODFMaximaPushButton_clicked(); }
    if (brainSuiteWindow->brainSuiteDataManager.vODFEig.size()==0) return;
    Vol3D<uint8> vMask;
    const float magnitudeThreshold=ui->gfaThresholdLineEdit->text().toFloat();
    if (magnitudeThreshold>0)
    {
      vMask.makeCompatible(brainSuiteWindow->brainSuiteDataManager.vODFEig);
      const int ds=brainSuiteWindow->brainSuiteDataManager.vODFEig.size();
      for (int i=0;i<ds;i++)
        vMask[i] = (brainSuiteWindow->brainSuiteDataManager.vODFEig[i].l0>=magnitudeThreshold) ? 255 : 0;
    }
    else
      vMask.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
    FiberTrackSet *fiberTractSet = new FiberTrackSet;
    if (fiberTractSet)
    {
      const float stepsize=ui->stepsizeLineEdit->text().toFloat();
      const int nSteps=ui->maxStepsLineEdit->text().toInt();
      float angleThreshold=ui->angleThresholdLineEdit->text().toFloat();
      float seedingRate = ui->tractSeedingLineEdit->text().toFloat();
      if (seedingRate<=0.1f) seedingRate=0.1f;
      if (seedingRate>5.0f) seedingRate=5.0f;
      ui->tractSeedingLineEdit->setText(QString::number(seedingRate));
      if (computeFiberTracks(*fiberTractSet,brainSuiteWindow->brainSuiteDataManager.vODFEig,vMask,1.0f/seedingRate,stepsize,nSteps,angleThreshold))
      {
        if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet)
          delete brainSuiteWindow->brainSuiteDataManager.fiberTrackSet;
        brainSuiteWindow->brainSuiteDataManager.fiberTrackSet = fiberTractSet;
        applyCurrentFilters();
        brainSuiteWindow->initializeSurfaceView();
        brainSuiteWindow->updateSurfaceView();
      }
      else
        delete fiberTractSet;
      brainSuiteWindow->brainSuiteDataManager.fiberCentroid=computeCenter(fiberTractSet);
    }
  }
  else if (ui->computeFromTensorsCheckBox->isChecked())
  {
    if (brainSuiteWindow && brainSuiteWindow->brainSuiteDataManager.volume
        && brainSuiteWindow->brainSuiteDataManager.volume->typeID()==SILT::Eigensystem3x3f)
    {
      Vol3D<EigenSystem3x3f> &vEig=*(Vol3D<EigenSystem3x3f> *)brainSuiteWindow->brainSuiteDataManager.volume;
      Vol3D<uint8> vMask;
      Vol3D<float32> vFA;
      const float faThreshold=ui->faThresholdLineEdit->text().toFloat();
      if (faThreshold>0)
      {
        if (brainSuiteWindow->brainSuiteDataManager.vMask.isCompatible(vEig))
          vMask.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
        else
        {
          vMask.makeCompatible(vEig);
          vMask.set(255);
        }
        const int ds=vEig.size();
        vFA.makeCompatible(vEig);
        for (int i=0;i<ds;i++)
        {
          vFA[i]=fractionalAnisotropy(vEig[i]);
          if (vMask[i])
            vMask[i] = (fractionalAnisotropy(vEig[i])>=faThreshold) ? 255 : 0;
        }
      }
      else
        vMask.copy(brainSuiteWindow->brainSuiteDataManager.vMask);
      FiberTrackSet *fiberTractSet = new FiberTrackSet;
      if (fiberTractSet)
      {
        const float stepsize=ui->stepsizeLineEdit->text().toFloat();
        const int nSteps=ui->maxStepsLineEdit->text().toInt();
        float angleThreshold=ui->angleThresholdLineEdit->text().toFloat();
        float seedingRate = ui->tractSeedingLineEdit->text().toFloat();
        if (seedingRate<=0.1f) seedingRate=0.1f;
        if (seedingRate>5.0f) seedingRate=5.0f;
        ui->tractSeedingLineEdit->setText(QString::number(seedingRate));
        if (computeFiberTracks(*fiberTractSet,vEig,vMask,1.0f/seedingRate,stepsize,nSteps,angleThreshold))
        {
          if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet)
            delete brainSuiteWindow->brainSuiteDataManager.fiberTrackSet;
          brainSuiteWindow->brainSuiteDataManager.fiberTrackSet = fiberTractSet;
          applyCurrentFilters();
          brainSuiteWindow->initializeSurfaceView();
          brainSuiteWindow->updateSurfaceView();
        }
        else
          delete fiberTractSet;
        brainSuiteWindow->brainSuiteDataManager.fiberCentroid=computeCenter(fiberTractSet);
      }
    }
  }
}

void DiffusionToolboxForm::on_findODFMaximaPushButton_clicked()
{
  if (brainSuiteWindow)
  {
    int samplingRes=ui->odfSamplingLineEdit->text().toInt();
    if (samplingRes<3) { samplingRes=3; ui->odfSamplingLineEdit->setText(QString::number(samplingRes)); }
    if (samplingRes>100) { samplingRes=100; ui->odfSamplingLineEdit->setText(QString::number(samplingRes)); }
    hardiTool.odfFactory.makeBasis(samplingRes);
    sampleODFs(brainSuiteWindow->brainSuiteDataManager.vODFEig,hardiTool,brainSuiteWindow->brainSuiteDataManager.vMask);
  }
}

void DiffusionToolboxForm::on_showDTI_XYPlaneCheckBox_clicked()
{
  BrainSuiteSettings::drawTensorsXY=ui->showDTI_XYPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showDTI_XZPlaneCheckBox_clicked()
{
  BrainSuiteSettings::drawTensorsXZ=ui->showDTI_XZPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showDTI_YZPlaneCheckBox_clicked()
{
  BrainSuiteSettings::drawTensorsYZ=ui->showDTI_YZPlaneCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_tubeRadiusSlider_valueChanged(int scaledRadius)
{
  if (!initializing)
  {
    BrainSuiteSettings::tubeSize = scaledRadius/tubeSliderScale;
    ui->tubeRadiusEditBox->setText(QString::number(BrainSuiteSettings::tubeSize,'f',2));
    if (BrainSuiteSettings::showFibersAsTubes)
      brainSuiteWindow->updateSurfaceView();
  }
}

void DiffusionToolboxForm::on_tubeRadiusEditBox_textEdited(const QString &arg1)
{
  BrainSuiteSettings::tubeSize = arg1.toFloat();
  initializing=true;
  ui->tubeRadiusSlider->setSliderPosition(BrainSuiteSettings::tubeSize*tubeSliderScale);
  initializing=false;
  if (BrainSuiteSettings::showFibersAsTubes)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_renderFibersAsTubesCheckBox_clicked()
{
  BrainSuiteSettings::showFibersAsTubes = ui->renderFibersAsTubesCheckBox->isChecked();
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_tractLengthSlider_valueChanged(int lengthThreshold)
{
  if (!initializing)
  {
    QString s;
    float f =lengthThreshold;
    s.setNum(f,'f',1);
    ui->tractLengthEdit->setText(s);
    applyCurrentFilters();
  }
}

void DiffusionToolboxForm::on_tractLengthEdit_textChanged(const QString &arg1)
{
  initializing=true;
  float lengthThreshold = arg1.toFloat();
  ui->tractLengthSlider->setTracking(false);
  ui->tractLengthSlider->setSliderPosition(lengthThreshold);
  ui->tractLengthSlider->setTracking(true);
  initializing=false;
  applyCurrentFilters();
}

float length(std::vector<DSPoint> &curve)
{
  const size_t n=curve.size();
  if (n<2) return 0.0f;
  float sum=0.0f;
  for (size_t i=1;i<n;i++)
  {
    sum += (curve[i]-curve[i-1]).mag();
  }
  return sum;
}

bool downsample(std::vector<DSPoint> &newCurve, std::vector<DSPoint> &curve, const int nPoints)
// downsample the curve into nPoints points, unless it already has length <= nPoints
// this method uses uniform speed parameterization, and is fast but non-ideal
// TODO: replice with splines
{
  const size_t nc = curve.size();
  if ((int)nc<=nPoints) { newCurve = curve; return false; }
  const float len = ::length(curve);
  if (len<=0.0f) return false;
  newCurve.resize(nPoints);
  newCurve[0] = curve[0];
  float sum=0.0f;
  int nextIdx = 1;
  float nextPos = nextIdx * len / (nPoints - 1);
  for (size_t i=1;i<nc;i++)
  {
    sum += (curve[i]-curve[i-1]).mag();
    if (sum>=nextPos)
    {
      newCurve[nextIdx] = curve[i];
      nextIdx++;
      if (nextIdx==nPoints-1)
      {
        newCurve[nextIdx] = curve.back();
        break;
      }
      nextPos = nextIdx * len / (nPoints - 1);
    }
  }
  return true;
}

inline DSPoint voxelToWorld(const Vol3DBase &vRef, IPoint3D point)
{
  float rx = vRef.rx; if (rx==0) rx = 1;
  float ry = vRef.ry; if (ry==0) ry = 1;
  float rz = vRef.rz; if (rz==0) rz = 1;
  return DSPoint((point.x-0.5f)*rx,(point.y-0.5f)*ry,(point.z-0.5f)*rz);
}

template<class ContainerType>
void readUniqueList(ContainerType &container, QTextEdit *lineEdit)
{
  std::istringstream istr(lineEdit->toPlainText().toStdString());
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

FiberTrackSet *filterBySphereROIs(FiberTrackSet *fiberTrackSet, Vol3DBase *volume, std::vector<SphereROI> &sphereROIs)
{
  if (!fiberTrackSet) return 0;
  FiberTrackSet *newSubset=0;
  if (sphereROIs.size())
  {
    for (auto sphere=sphereROIs.begin();sphere!=sphereROIs.end();sphere++)
    {
      SphereROI roi=*sphere;
      if (roi.active && roi.andop)
      {
        FiberTrackSet *source = (newSubset) ? newSubset : fiberTrackSet;
        FiberTrackSet *dest = 0;
        DSPoint wc = (volume) ? voxelToWorld(*volume,roi.voxelPosition) : roi.voxelPosition;
        if (roi.include)
          dest = TrackFilter::sphericalFilter(source, wc, roi.radius);
        else
          dest = TrackFilter::sphericalFilterExclude(source, wc, roi.radius);
        if (newSubset) delete newSubset;
        newSubset = dest;
      }
    }
    FiberTrackSet *source = (newSubset) ? newSubset : fiberTrackSet;
    std::vector<bool> keepList;
    for (auto sphere=sphereROIs.begin();sphere!=sphereROIs.end();sphere++)
    {
      SphereROI roi=*sphere;
      if (roi.active && !roi.andop)
      {
        DSPoint wc = (volume) ? voxelToWorld(*volume,roi.voxelPosition) : roi.voxelPosition;
        if (roi.include)
        {
          TrackFilter::sphericalFilterIndex(keepList,source, wc, roi.radius);
        }
      }
    }
    if (keepList.size()>0)
    {
      FiberTrackSet *dest = TrackFilter::reduceTrackSet(source,keepList);
      if (newSubset) delete newSubset;
      newSubset = dest;
    }
  }
  return newSubset;
}

void DiffusionToolboxForm::applyCurrentFilters()
{
  Timer t; t.start();
  if (!brainSuiteWindow->brainSuiteDataManager.fiberTrackSet)
  {
    ui->fiberCountDisplayLabel->setText("diplaying 0/0 fibers");
    ui->fiberCountDisplayLabel2->setText("diplaying 0/0 fibers");
    return;
  }
  FiberTrackSet *newSubset=0;
  bool filterLength=false;
  float lengthThreshold = 0.0f;

  lengthThreshold=ui->tractLengthEdit->text().toFloat();
  filterLength = lengthThreshold>0;

  Vol3DBase *labelVolume = brainSuiteWindow->brainSuiteDataManager.vLabel;
  if (filterLength)
  {
    FiberTrackSet *lengthSubset = TrackFilter::filterTracksByLength(newSubset ? newSubset : brainSuiteWindow->brainSuiteDataManager.fiberTrackSet,lengthThreshold);
    if (lengthSubset)
    {
      delete newSubset; newSubset = lengthSubset;
    }
  }
  if (ui->excludeLabelListCheckBox->isChecked() && labelVolume)
  {
    std::vector<int> excludeLabels;
    ::readUniqueList(excludeLabels,ui->excludeLabelsLineEdit);
    if (excludeLabels.size())
    {
      FiberTrackSet *filteredTracks=TrackFilter::filterExcludedLabels((newSubset) ? newSubset : brainSuiteWindow->brainSuiteDataManager.fiberTrackSet,labelVolume,excludeLabels);
      if (filteredTracks)
      {
        if (newSubset) delete newSubset;
        newSubset = filteredTracks;
      }
    }
  }
  if (sphereROIs.size())
  {
    FiberTrackSet *filteredTracks=::filterBySphereROIs(
                                    (newSubset) ? newSubset : brainSuiteWindow->brainSuiteDataManager.fiberTrackSet,
                                    brainSuiteWindow->brainSuiteDataManager.volume,
                                    sphereROIs);
    if (filteredTracks)
    {
      if (newSubset) delete newSubset;
      newSubset = filteredTracks;
    }
  }
  if (brainSuiteWindow->toolManager.connectivityViewDialog)
  {
    if (brainSuiteWindow->toolManager.connectivityViewDialog->isVisible())
    {
      FiberTrackSet *circleFilteredSubset=brainSuiteWindow->toolManager.connectivityViewDialog->applyFilter(newSubset ? newSubset : brainSuiteWindow->brainSuiteDataManager.fiberTrackSet);
      if (circleFilteredSubset)
      {
        delete newSubset;
        newSubset=circleFilteredSubset;
        circleFilteredSubset=0;
      }
    }
  }
  if (downsampleFactor>0)
  {
    Timer t; t.start();
    FiberTrackSet *downsampledSet=new FiberTrackSet;
    FiberTrackSet *sourceSet=newSubset ? newSubset : brainSuiteWindow->brainSuiteDataManager.fiberTrackSet;
    const size_t nf = sourceSet->curves.size();
    downsampledSet->curves.resize(nf);
    for (size_t i=0;i<nf;i++)
    {
      downsampledSet->curves[i].attributes = sourceSet->curves[i].attributes;
      downsample(downsampledSet->curves[i].points,sourceSet->curves[i].points,downsampleFactor);
    }
    delete newSubset;
    newSubset=downsampledSet;
  }
  delete brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset;
  brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset=newSubset;
  BrainSuiteSettings::showTrackSubset = (newSubset!=0);
  QString totalFibers=QString::number(brainSuiteWindow->brainSuiteDataManager.fiberTrackSet->curves.size());
  QString nc = QString::number((BrainSuiteSettings::showTrackSubset) ?
                                 brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset->curves.size() :
                                 brainSuiteWindow->brainSuiteDataManager.fiberTrackSet->curves.size());
  t.stop();
  brainSuiteWindow->updateSurfaceView();
  QString s=QString("diplaying %1/%2 fibers").arg(nc,totalFibers);
  ui->fiberCountDisplayLabel->setText(s);
  ui->fiberCountDisplayLabel2->setText(s);
}

void DiffusionToolboxForm::computeConnectivity()
{
  brainSuiteWindow->showDialog(brainSuiteWindow->toolManager.connectivityViewDialog);
  if (brainSuiteWindow->toolManager.connectivityViewDialog)
  {
    brainSuiteWindow->toolManager.connectivityViewDialog->computeConnectivity(ui->mergeCortialROIsCheckBox->isChecked());
  }
}

void DiffusionToolboxForm::on_computeConnectivityPushButton_clicked()
{
  computeConnectivity();
}

void DiffusionToolboxForm::on_clipGlyphsCheckBox_clicked()
{
  BrainSuiteSettings::clipODFs = ui->clipGlyphsCheckBox->checkState();
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_glyphQualityComboBox_currentIndexChanged(int index)
{
  BrainSuiteSettings::glyphQuality = index;
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showTensorGlyphsCheckBox_clicked()
{
  BrainSuiteSettings::showTensors=ui->showTensorGlyphsCheckBox->isChecked();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateSurfaceView();
  }
}

void DiffusionToolboxForm::on_showTensorsUsingColorFA_clicked()
{
  BrainSuiteSettings::useColorFA=ui->showTensorsUsingColorFA->isChecked();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateImages();
    brainSuiteWindow->updateSurfaceView();
  }
}

void DiffusionToolboxForm::on_clipTensorGlyphsCheckBox_clicked()
{
  BrainSuiteSettings::clipTensors=ui->clipTensorGlyphsCheckBox->isChecked();
  if (brainSuiteWindow)
  {
    brainSuiteWindow->updateSurfaceView();
  }
}

void DiffusionToolboxForm::on_downsampleFactorEditBox_textEdited(const QString &arg1)
{
  if (ui->downsampleTractsCheckBox->isChecked())
  {
    downsampleFactor = arg1.toInt();
    applyCurrentFilters();
  }
  else
    downsampleFactor=0;
}

void DiffusionToolboxForm::on_downsampleTractsCheckBox_clicked()
{
  if (ui->downsampleTractsCheckBox->isChecked())
  {
    downsampleFactor = ui->downsampleFactorEditBox->text().toInt();
  }
  else
    downsampleFactor=0;
  applyCurrentFilters();
}

void DiffusionToolboxForm::on_odfScaleLineEdit_textEdited(const QString &arg1)
{
  float value = arg1.toFloat();
  float f = hardiRenderer.setScale(value);
  int p = f*odfScaleSliderScale;
  setSliderPosition(ui->odfScaleSlider,p);
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_odfScaleSlider_valueChanged(int value)
{
  float f = hardiRenderer.setScale(value/odfScaleSliderScale);
  int p = f*odfScaleSliderScale;
  ui->odfScaleSlider->setSliderPosition(p);
  ui->odfScaleLineEdit->setText(QString::number(hardiTool.odfFactory.scale));
  brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_trackDisplayColorSelectionButton_clicked()
{
  QColor c=QColorDialog::getColor(ui->trackDisplayColorSelectionButton->getColor(),this,"Default Track Color");
  if (!c.isValid()) return;
  ui->trackDisplayColorSelectionButton->setColor(c);
  BrainSuiteSettings::defaultFiberColor=DSPoint(c.redF(),c.greenF(),c.blueF());
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_showSphereInSliceViewCheckBox_clicked()
{
  updateImages();
}

void DiffusionToolboxForm::on_odfMinMaxCheckBox_clicked()
{
  hardiTool.odfFactory.normalize = ui->odfMinMaxCheckBox->isChecked();
  updateGlyphs(false);
  updateImages();
}

void DiffusionToolboxForm::on_trackDisplayColorModelComboBox_activated(int index)
{
  switch (index)
  {
    case 0 : BrainSuiteSettings::trackColorModel=BrainSuiteSettings::SingleColor; break;
    case 1 : BrainSuiteSettings::trackColorModel=BrainSuiteSettings::AverageAngle; break;
    case 2 : BrainSuiteSettings::trackColorModel=BrainSuiteSettings::LocalAngle; break;
    case 3 : BrainSuiteSettings::trackColorModel=BrainSuiteSettings::FiberLength; break;
    default : BrainSuiteSettings::trackColorModel=BrainSuiteSettings::AverageAngle; break;
  }
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_lineWidthLineEdit_textEdited(const QString &arg1)
{
  BrainSuiteSettings::fiberLineWidth=arg1.toInt();
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}

void DiffusionToolboxForm::on_enableTrackClippingCheckBox_toggled(bool checked)
{
  BrainSuiteSettings::clipFibers=checked;
  if (brainSuiteWindow)
    brainSuiteWindow->updateSurfaceView();
}
