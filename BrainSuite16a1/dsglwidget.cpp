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

#include <QtOpenGL>
#include <dsglwidget.h>
#include <dspoint.h>
#include <brainsuitewindow.h>
#include <surfacealpha.h>
#include <glviewcontroller.h>
#include <brainsuiterenderer.h>
#include <glglyphs.h>
#include <brainsuitesettings.h>
#include <toolmanager.h>
#include <tensorglyphrenderer.h>
#include <diffusiontoolboxform.h>
#include <brainsuitesettings.h>
#include <protocolcurveset.h>
#include <fibertrackset.h>
#include <curvetoolboxform.h>
#include <fiberrenderer.h>
#include <connectivityproperties.h>
#include <qdebug.h>
#include <DS/timer.h>
#include <qdebug.h>
#include <delineationtoolboxform.h>
#include <DS/timer.h>
#include <tensorglyphrenderer_t.h>
#include <hardirenderer.h>
#include <diffusioncolorops.h>

// TODO: replace gluunproject to remove dependencies on glut & glu
#ifdef Q_OS_LINUX
#include <GL/glut.h>
#endif
#ifdef Q_OS_MAC
#include <GLUT/glut.h>
#endif
#ifdef Q_OS_WIN
#include <GL/GLU.h>
#endif

extern FiberRenderer fiberRenderer;
extern HARDITool hardiTool;
extern HARDIRenderer hardiRenderer;

DSGLWidget::DSGLWidget(BrainSuiteWindow *parent_) :   QGLWidget(QGLFormat(QGL::SampleBuffers), parent_),
  hasMouse(false), brainSuiteWindow(parent_), tracing(false), blockSpin(false)
{
  BrainSuiteRenderer::initialize();
  setFormat(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer));
  glViewController.setRotation(270,180,0);
  fiberRenderer.setup(1.0f,BrainSuiteSettings::nCylinderPanels);
}

DSPoint DSGLWidget::getOGLPos(const QPoint &p)
{  
  GLfloat winX, winY, winZ;
  GLdouble posX, posY, posZ;
  winX = (float)p.x();
  winX = (float)p.x()*devicePixelRatio();
  winY = (float)viewport[3] - (float)p.y()*devicePixelRatio();
  glReadPixels( winX, int(winY), 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
  gluUnProject( winX, winY, winZ, modelview, projection, viewport, &posX, &posY, &posZ);
  return DSPoint((float)posX, (float)posY, (float)posZ);
}

DSPoint DSGLWidget::getBackgroundColor()
{
  return glViewController.clearColor;
}

void DSGLWidget::setBackgroundColor(DSPoint backgroundColor)
{
  glViewController.clearColor = backgroundColor;
  update();
}

void DSGLWidget::setRotation(const DSPoint &rotationAngles)
{
  glViewController.setRotation(rotationAngles.x,rotationAngles.y,rotationAngles.z);
  update();
}

class TriSortNode {
public:
  TriSortNode(const size_t id_=0, const float depth_=0) : id(id_), depth(depth_) {}
  size_t id;
  float depth;
};

inline float sortValue(Triangle &t, DSPoint *p, const DSPoint &view)
{
  return std::min(std::min(view.dot(p[t.a]),view.dot(p[t.b])),view.dot(p[t.c]));
}

inline bool operator<(const TriSortNode &lhs, const TriSortNode &rhs)
{
  return (lhs.depth < rhs.depth);
}

void drawSurfaceAlpha(SurfaceAlpha &surface, bool viewLighting, bool depthSort)
{
  const size_t nt = surface.triangles.size();
  const size_t nv = surface.vertices.size();
  bool sortTris = depthSort;
  DSPoint *verts = (surface.displayAlternate && surface.alternateVertices.size()==nv) ? &surface.alternateVertices[0] : &surface.vertices[0];
  if (nt>0)
  {
    if (surface.wireFrame)
    {
      ::glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
      sortTris = false;
    }
    else
      ::glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    if (nv!=surface.vertexColor.size())
    {
      surface.useVertexColor = false;
      surface.useSolidColor = true;
    }
    else
      if (surface.useVertexColor)
      {
        if (surface.alphaColor.size() !=surface.vertexColor.size())
        {
          surface.alphaColor.resize(surface.vertexColor.size());
          const size_t n = surface.alphaColor.size();
          for (size_t i=0;i<n;i++)
          {
            surface.alphaColor[i] = RGBAf(surface.vertexColor[i],0.5f);
          }
        }
      }
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3,GL_FLOAT,0,verts);

    if (viewLighting && (surface.vertexNormals.size()>0))
    {
      glEnable(GL_LIGHTING);
      glEnableClientState(GL_NORMAL_ARRAY);
      glNormalPointer(GL_FLOAT,0,&surface.vertexNormals[0]);
    }
    else
    {
      glDisable(GL_LIGHTING);
      glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (surface.useVertexColor&&surface.alphaColor.size()==nv)
    {
      if (surface.alphaColor.size()>0)
      {
        if (surface.alpha != surface.alphaColor[0].a)
        {
          const size_t nv = surface.alphaColor.size();
          RGBAf *p = &surface.alphaColor[0];
          const float alpha = surface.alpha;
          for (size_t i=0;i<nv;i++)
          {
            p[i].a = alpha;
          }
        }
        glEnable(GL_COLOR_MATERIAL);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4,GL_FLOAT,0,&surface.alphaColor[0]);
      }
    }
    else
    {
      glDisableClientState(GL_COLOR_ARRAY);
      glEnable(GL_COLOR_MATERIAL);
      glColor4f(surface.solidColor.x,surface.solidColor.y,surface.solidColor.z,surface.alpha);
    }
    if (sortTris)
    {
      GLfloat modelview[16];
      glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
      const DSPoint quickview(modelview[2],modelview[6],modelview[10]);
      std::vector<TriSortNode> tris(surface.nt());
      for (size_t i=0;i<surface.nt();i++)
        tris[i] = TriSortNode(i,sortValue(surface.triangles[i],verts,quickview));
      std::sort(tris.begin(),tris.end());
      std::vector<Triangle> tNew(surface.nt());
      for (size_t i=0;i<surface.nt();i++)
        tNew[i] = surface.triangles[tris[i].id];
      glDrawElements(GL_TRIANGLES,(GLsizei)surface.nt()*3,GL_UNSIGNED_INT,&tNew[0]);
    }
    else
    {
      glDrawElements(GL_TRIANGLES,(GLsizei)surface.nt()*3,GL_UNSIGNED_INT,&surface.triangles[0]);
    }
  }
}

class DrawIfTranslucent {
public:
  DrawIfTranslucent() : depthSort(false) {}
  bool depthSort;
  void drawSurface(SurfaceAlpha *surface)
  {
    if (surface && surface->show && surface->nv()>0)
    {
      if (surface->translucent)
      {
        ::glEnable(GL_ALPHA_TEST);
        ::glEnable(GL_DEPTH_TEST);
        ::glEnable (GL_BLEND);
        ::glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        ::glDepthFunc(GL_LEQUAL);
        BrainSuiteRenderer::setClip(surface->allowClip);
        drawSurfaceAlpha(*surface,true,depthSort);
        BrainSuiteRenderer::resetClip();
      }
    }
  }
  void operator()(BrainSuiteDataManager::SurfaceHandle &surfaceHandle)
  {
    drawSurface(surfaceHandle.surface);
  }
};

void drawSurface(BrainSuiteDataManager::SurfaceHandle &iter)
{
  SurfaceAlpha *surface = iter.surface;
  if (!surface) { std::cerr<<"null pointer in surface list..."<<std::endl; return; }
  if (surface->translucent) return;
  if (!surface->show) return;
  BrainSuiteRenderer::setClip(surface->allowClip);
  if (surface->wireFrame)
  {
    ::glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
  }
  else
  {
    ::glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  }
  DSPoint *verts = (surface->displayAlternate && surface->alternateVertices.size()==surface->nv()) ? &surface->alternateVertices[0] : &surface->vertices[0];

  glVertexPointer(3, GL_FLOAT, 0, verts); // Set The Vertex Pointer To Our Vertex Data
  if (surface->vertexNormals.size()==0)
  {
    surface->computeNormals();
    if (surface->vertexNormals.size()==0) return;
  }
  glNormalPointer(GL_FLOAT, 0, &surface->vertexNormals[0]);// Set The Vertex Normal Pointer To The Vertex Normal Buffer
  if (surface->useVertexColor&&surface->vertexColor.size()==surface->vertices.size())
  {
    glEnable(GL_COLOR_MATERIAL);
    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(3,GL_FLOAT,0,&surface->vertexColor[0]);
  }
  else
  {
    glDisableClientState(GL_COLOR_ARRAY);
    glEnable(GL_COLOR_MATERIAL);
    glColor3f(surface->solidColor.x,surface->solidColor.y,surface->solidColor.z);
  }
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_NORMAL_ARRAY);
  const int ntris = (int)(3*surface->nt());
  glDrawElements( GL_TRIANGLES, ntris, GL_UNSIGNED_INT, &surface->triangles[0]);
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  BrainSuiteRenderer::resetClip();
}

void DSGLWidget::paintOpaqueSurfaces()
{
  if (!brainSuiteWindow) return;
  std::list<BrainSuiteDataManager::SurfaceHandle> &surfaceHandles = brainSuiteWindow->brainSuiteDataManager.surfaces;
  for (std::list<BrainSuiteDataManager::SurfaceHandle>::iterator handle=surfaceHandles.begin(); handle!= surfaceHandles.end(); handle++)
    drawSurface(*handle);
}

void DSGLWidget::paintTranslucentSurfaces()
{
  if (!brainSuiteWindow) return;
  std::list<BrainSuiteDataManager::SurfaceHandle> &surfaceHandles = brainSuiteWindow->brainSuiteDataManager.surfaces;
  DrawIfTranslucent surfacePainter;
  surfacePainter.depthSort= BrainSuiteSettings::depthSortSurfaceTriangles;
  ::glEnable(GL_ALPHA_TEST);
  ::glEnable (GL_BLEND);
  ::glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  ::glDepthFunc(GL_LEQUAL);
  for (auto surfaceHandle=surfaceHandles.begin(); surfaceHandle!= surfaceHandles.end(); surfaceHandle++)
  {
    SurfaceAlpha *surface = surfaceHandle->surface;
    if (!surface) { continue; }
    if (!surface->translucent) continue;
    surfacePainter.drawSurface(surface);
  }
  glDisable (GL_BLEND);
}


inline DSPoint rgb2pt(const uint32 a)
{
  unsigned char *p = (unsigned char *)&a;
  return DSPoint(p[2]/255.0f,p[1]/255.0f,p[0]/255.0f);
}

void shiftTowardsCamera(const float viewShift)
{
  GLfloat modelview[16];
  glGetFloatv( GL_MODELVIEW_MATRIX, modelview );
  const DSPoint displace(modelview[2]*viewShift,modelview[6]*viewShift,modelview[10]*viewShift);
  ::glTranslatef(displace.x,displace.y,displace.z);
}

void DSGLWidget::paintTensors()
{
  BrainSuiteRenderer::setClip(BrainSuiteSettings::clipTensors);
  if (brainSuiteWindow->brainSuiteDataManager.volume && brainSuiteWindow->brainSuiteDataManager.volume->typeID()==SILT::Eigensystem3x3f)
  {
    Vol3D<EigenSystem3x3f> &vEig(*(Vol3D<EigenSystem3x3f> *)brainSuiteWindow->brainSuiteDataManager.volume);
    Vol3D<uint8> &vMask(brainSuiteWindow->brainSuiteDataManager.vMask);
    bool mask=true;
    if (BrainSuiteSettings::useColorFA)
    {
      MajorVectorFA colorOp(brainSuiteWindow->imageState.brightness.maxv);
      if (mask==false)
      {
        if (BrainSuiteSettings::drawTensorsXY) TensorGlyphRenderer::drawEllipsesXY(vEig,brainSuiteWindow->currentVolumePosition.z,colorOp);
        if (BrainSuiteSettings::drawTensorsXZ) TensorGlyphRenderer::drawEllipsesXZ(vEig,brainSuiteWindow->currentVolumePosition.y,colorOp);
        if (BrainSuiteSettings::drawTensorsYZ) TensorGlyphRenderer::drawEllipsesYZ(vEig,brainSuiteWindow->currentVolumePosition.x,colorOp);
      }
      else
      {
        if (BrainSuiteSettings::drawTensorsXY) TensorGlyphRenderer::drawEllipsesMaskedXY(vEig,vMask,brainSuiteWindow->currentVolumePosition.z,colorOp);
        if (BrainSuiteSettings::drawTensorsXZ) TensorGlyphRenderer::drawEllipsesMaskedXZ(vEig,vMask,brainSuiteWindow->currentVolumePosition.y,colorOp);
        if (BrainSuiteSettings::drawTensorsYZ) TensorGlyphRenderer::drawEllipsesMaskedYZ(vEig,vMask,brainSuiteWindow->currentVolumePosition.x,colorOp);
      }
    }
    else
    {
      MajorVector colorOp(brainSuiteWindow->imageState.brightness.maxv);
      if (mask==false)
      {
        if (BrainSuiteSettings::drawTensorsXY) TensorGlyphRenderer::drawEllipsesXY(vEig,brainSuiteWindow->currentVolumePosition.z,colorOp);
        if (BrainSuiteSettings::drawTensorsXZ) TensorGlyphRenderer::drawEllipsesXZ(vEig,brainSuiteWindow->currentVolumePosition.y,colorOp);
        if (BrainSuiteSettings::drawTensorsYZ) TensorGlyphRenderer::drawEllipsesYZ(vEig,brainSuiteWindow->currentVolumePosition.x,colorOp);
      }
      else
      {
        if (BrainSuiteSettings::drawTensorsXY) TensorGlyphRenderer::drawEllipsesMaskedXY(vEig,vMask,brainSuiteWindow->currentVolumePosition.z,colorOp);
        if (BrainSuiteSettings::drawTensorsXZ) TensorGlyphRenderer::drawEllipsesMaskedXZ(vEig,vMask,brainSuiteWindow->currentVolumePosition.y,colorOp);
        if (BrainSuiteSettings::drawTensorsYZ) TensorGlyphRenderer::drawEllipsesMaskedYZ(vEig,vMask,brainSuiteWindow->currentVolumePosition.x,colorOp);
      }
    }
  }
  BrainSuiteRenderer::resetClip();
}

void DSGLWidget::paintODFs()
{
  BrainSuiteRenderer::setClip(BrainSuiteSettings::clipODFs);
  hardiRenderer.drawMasked(brainSuiteWindow->currentVolumePosition,brainSuiteWindow->brainSuiteDataManager.vMask);
  BrainSuiteRenderer::resetClip();
}

inline DSPoint voxelToWorld(const Vol3DBase &vRef, IPoint3D point)
{
  float rx = vRef.rx; if (rx==0) rx = 1;
  float ry = vRef.ry; if (ry==0) ry = 1;
  float rz = vRef.rz; if (rz==0) rz = 1;
  return DSPoint(
        (point.x-0.5f)*rx,
        (point.y-0.5f)*ry,
        (point.z-0.5f)*rz);
}

void DSGLWidget::drawSphereROIs()
{
  Vol3DBase *volume = brainSuiteWindow->brainSuiteDataManager.volume;
  if (volume && brainSuiteWindow->toolManager.diffusionToolboxForm)
  {
    const float rx = volume->rx;
    const float ry = volume->ry;
    const float rz = volume->rz;
    DSPoint shift(rx/2.0f,ry/2.0f,rz/2.0f);
    if (brainSuiteWindow->toolManager.diffusionToolboxForm->showSphereROIs_in3D())
    {
      auto &sphereROIs=brainSuiteWindow->toolManager.diffusionToolboxForm->sphereROIs;
      bool wireframe=brainSuiteWindow->toolManager.diffusionToolboxForm->sphereWireframe();
      for (auto sphere=sphereROIs.begin();sphere!=sphereROIs.end();sphere++)
      {
        if (!sphere->active) continue;
        DSPoint pos=voxelToWorld(*volume,sphere->voxelPosition);
        BrainSuiteRenderer::drawTrackROISphere(pos+shift,sphere->radius,sphere->color,wireframe);
      }
    }
  }
}

void DSGLWidget::paintFibers()
{
  BrainSuiteRenderer::setClip(BrainSuiteSettings::clipFibers);
  if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset && BrainSuiteSettings::showTrackSubset)
    BrainSuiteRenderer::drawFiberTrackSet(*brainSuiteWindow->brainSuiteDataManager.fiberTrackSubset);
  else
    if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet)
      BrainSuiteRenderer::drawFiberTrackSet(*brainSuiteWindow->brainSuiteDataManager.fiberTrackSet);
  BrainSuiteRenderer::resetClip();
}

inline DSPoint lighter(DSPoint color, int f=150)
{
  QColor c=QColor::fromRgbF(color.x,color.y,color.z).light(f);
  return DSPoint(c.redF(),c.greenF(),c.blueF());
}

void DSGLWidget::paintCurves()
{
  if (brainSuiteWindow->brainSuiteDataManager.protocolCurveSet)
  {
    ::glPushMatrix();
    if (!BrainSuiteSettings::showCurvesAsTubes)
      shiftTowardsCamera(0.05f);
    const int selectedCurve = (brainSuiteWindow->toolManager.curveToolboxForm) ? brainSuiteWindow->toolManager.curveToolboxForm->currentCurveIndex() : -1;
    BrainSuiteRenderer::drawCurveset(*brainSuiteWindow->brainSuiteDataManager.protocolCurveSet,selectedCurve);
    if (brainSuiteWindow->toolManager.curveToolboxForm)
    {
      auto curve=brainSuiteWindow->toolManager.curveToolboxForm->currentCurve();
      if (curve && curve->points.size())
      {
        DSPoint color=lighter(brainSuiteWindow->toolManager.curveToolboxForm->currentCurve()->attributes.color);
        std::vector<DSPoint> endPoints(1);
        endPoints.push_back(curve->points.back());
        BrainSuiteRenderer::drawSphereStrip(endPoints,color,4.0*BrainSuiteSettings::curveTubeSize);
      }
    }
    ::glPopMatrix();
  }
}

inline DSPoint volumeOrigin(Vol3DBase *volume)
{
  const int cx = volume->cx;
  const int cy = volume->cy;
  const int cz = volume->cz;
  const float ox = 0;//volume->ox; ignoring volume position for now
  const float oy = 0;//volume->oy;
  const float oz = 0;//volume->oz;
  const float rx = volume->rx;
  const float ry = volume->ry;
  const float rz = volume->rz;
  return DSPoint(-ox-cx*rx/2.0f,-oy-cy*ry/2.0f,-oz-cz*rz/2.0f);
}

// TODO:move to renderer
void setClipping(const DSPoint &pointWC)
{
  BrainSuiteRenderer::setClippingPlane(BrainSuiteSettings::clipXPlane,GL_CLIP_PLANE0,DSPoint(1,0,0),pointWC,BrainSuiteSettings::clipXPlaneOffset);
  BrainSuiteRenderer::setClippingPlane(BrainSuiteSettings::clipYPlane,GL_CLIP_PLANE1,DSPoint(0,1,0),pointWC,BrainSuiteSettings::clipYPlaneOffset);
  BrainSuiteRenderer::setClippingPlane(BrainSuiteSettings::clipZPlane,GL_CLIP_PLANE2,DSPoint(0,0,1),pointWC,BrainSuiteSettings::clipZPlaneOffset);
}

void DSGLWidget::initializeGL()
{
  ::glClearDepth(1.0f);
  ::glEnable(GL_DEPTH_TEST);
  ::glDepthFunc(GL_LEQUAL);
  ::glEnable(GL_COLOR_MATERIAL);
  ::glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,0);//twoSidedLighting ? 1 : 0;
}

void DSGLWidget::resizeGL( int width, int height )
{
  glViewController.setup(width,height);
}

void DSGLWidget::setOrigin()
{
  {
    Vol3DBase *volume = brainSuiteWindow->brainSuiteDataManager.volume;
    SurfaceAlpha *firstSurface = brainSuiteWindow->brainSuiteDataManager.firstSurface();
    if (volume)
    {
      DSPoint origin=volumeOrigin(volume);
      glTranslatef(origin.x,origin.y,origin.z);
      IPoint3D position = brainSuiteWindow->currentVolumePosition;
      DSPoint pointWC(position.x * volume->rx + 0.5f,position.y * volume->ry + 0.5f,position.z * volume->rz + 0.5f);
      if (brainSuiteWindow->imageState.showCursor) BrainSuiteRenderer::drawCrossHairs(volume,position);
      setClipping(pointWC);
    }
    else if (firstSurface)
      glTranslatef(-firstSurface->center.x,-firstSurface->center.y,-firstSurface->center.z);
    else if (brainSuiteWindow->brainSuiteDataManager.protocolCurveSet && brainSuiteWindow->brainSuiteDataManager.protocolCurveSet->curves.size())
      glTranslatef(-brainSuiteWindow->brainSuiteDataManager.curveCentroid.x,-brainSuiteWindow->brainSuiteDataManager.curveCentroid.y,-brainSuiteWindow->brainSuiteDataManager.curveCentroid.z);
    else if (brainSuiteWindow->brainSuiteDataManager.fiberTrackSet && brainSuiteWindow->brainSuiteDataManager.fiberTrackSet->curves.size())
      glTranslatef(-brainSuiteWindow->brainSuiteDataManager.fiberCentroid.x,-brainSuiteWindow->brainSuiteDataManager.fiberCentroid.y,-brainSuiteWindow->brainSuiteDataManager.fiberCentroid.z);
  }
}

void DSGLWidget::paintGL()
{
  if (!brainSuiteWindow) return; // shouldn't ever happen, but just in case...
  glViewController.initialize();
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glPushMatrix();
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glLoadIdentity();
  glViewController.lighting(BrainSuiteSettings::useLighting);
  glViewController.rotation();
  setOrigin();
  if (BrainSuiteSettings::showTensors) paintTensors();
  if (BrainSuiteSettings::showODFs) paintODFs();
  if (BrainSuiteSettings::showFibers) paintFibers();
  if (BrainSuiteSettings::showFibers) drawSphereROIs();
  if (BrainSuiteSettings::showCurves) paintCurves();
  if (BrainSuiteSettings::renderSlices && BrainSuiteSettings::useSliceOpacity==false) paintSlices(false);
  if (BrainSuiteSettings::showSurfaces) paintOpaqueSurfaces();
  if (BrainSuiteSettings::showSurfaces) paintTranslucentSurfaces();
  if (BrainSuiteSettings::renderSlices && BrainSuiteSettings::useSliceOpacity==true) paintSlices(true);
  glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
  glGetDoublev( GL_PROJECTION_MATRIX, projection );
  glGetIntegerv( GL_VIEWPORT, viewport );
  ::glPopMatrix();
  ::glPopAttrib();
}
