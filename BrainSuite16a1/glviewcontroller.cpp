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

#include <math.h>
#include <glviewcontroller.h>
#include <qgl.h>
#include <ArcBall.h>
#include <brainsuitesettings.h>

inline DSPoint getRotationAngles(Matrix3fT &m)
{
  const float d = -m.s.M12;
  const float absc = sqrtf(1-d*d);
  const float eps = 0.000001f;
  float a=0,b=0,c=0,e=0,f=0;
  if (absc>eps)
  {
    c =  absc;
    a =  m.s.M22 / c;
    b =  m.s.M02 / c;
    f =  m.s.M10 / c;
    e =  m.s.M11 / c;
  }
  else
  {
    c = 0;
    if (d>0)
    {
      const float AE2 = (m.s.M00 + m.s.M21);
      const float AF2 = (m.s.M01 + m.s.M20);
      const float BE2 = (m.s.M01 - m.s.M20);
      if (fabsf(AF2) > eps)
      {
        const float e_f = AE2 / AF2;//  f = 1/sqrtf(e_f^2 + 1)
        const float absF = 1.0f/sqrt(1.0f + e_f * e_f);
        f = absF;
        e = e_f * absF;
      }
      else
      {
        const float f_e = AF2 / AE2;//  f = 1/sqrtf(e_f^2 + 1)
        const float absE = 1.0f/sqrt(1.0f + f_e * f_e);
        e = absE;
        f = f_e * absE;
      }
      if (fabsf(BE2) > eps)
      {
        const float a_b = AE2 / BE2;
        const float absB = 1.0f/sqrt(1.0f + a_b * a_b);
        b = absB;
        a = a_b * absB;
      }
      else
      {
        const float b_a = BE2 / AE2;
        const float absA = 1.0f/sqrt(1.0f + b_a * b_a);
        a = absA;
        b = b_a * absA;
      }
    }
    else
    {
      const float AE2 = (m.s.M00 - m.s.M21);
      const float AF2 = (m.s.M01 - m.s.M20);
      const float BE2 = (m.s.M01 + m.s.M20);
      if (fabsf(AF2) > eps)
      {
        const float e_f = AE2 / AF2;//  f = 1/sqrtf(e_f^2 + 1)
        const float absF = 1.0f/sqrt(1.0f + e_f * e_f);
        f = absF;
        e = e_f * absF;
      }
      else
      {
        const float f_e = AF2 / AE2;//  f = 1/sqrtf(e_f^2 + 1)
        const float absE = 1.0f/sqrt(1.0f + f_e * f_e);
        e = absE;
        f = f_e * absE;
      }
      if (fabsf(BE2) > eps)
      {
        const float a_b = AE2 / BE2;
        const float absB = 1.0f/sqrt(1.0f + a_b * a_b);
        b = absB;
        a = a_b * absB;
      }
      else
      {
        const float b_a = BE2 / AE2;
        const float absA = 1.0f/sqrt(1.0f + b_a * b_a);
        a = absA;
        b = b_a * absA;
      }
    }
  }
  const float X = (float)(180.0f/M_PI);
  return DSPoint(X*atan2(d,c),X*atan2(b,a),X*atan2(f,e));
}

inline void rotZ(Matrix3fT &m, const float degrees)
{
  const float c = (float)cos(degrees * (M_PI/180.0f));
  const float s = (float)sin(degrees * (M_PI/180.0f));
  m.s.M00 = c; m.s.M01 =-s; m.s.M02 = 0;
  m.s.M10 = s; m.s.M11 = c; m.s.M12 = 0;
  m.s.M20 = 0; m.s.M21 = 0; m.s.M22 = 1;
}

inline void rotY(Matrix3fT &m, const float degrees)
{
  const float c = (float)cos(degrees * (M_PI/180.0f));
  const float s = (float)sin(degrees * (M_PI/180.0f));
  m.s.M00 = c; m.s.M01 = 0; m.s.M02 = s;
  m.s.M10 = 0; m.s.M11 = 1; m.s.M12 = 0;
  m.s.M20 =-s; m.s.M21 = 0; m.s.M22 = c;
}

inline void rotX(Matrix3fT &m, const float degrees)
{
  const float c = (float)cos(degrees * (M_PI/180.0f));
  const float s = (float)sin(degrees * (M_PI/180.0f));
  m.s.M00 = 1; m.s.M01 = 0; m.s.M02 = 0;
  m.s.M10 = 0; m.s.M11 = c; m.s.M12 =-s;
  m.s.M20 = 0; m.s.M21 = s; m.s.M22 = c;
}

inline std::ostream &operator<<(std::ostream &s, Matrix3fT &m)
{
  s<<m.s.M00<<' '<<m.s.M01<<' '<<m.s.M02<<'\n';
  s<<m.s.M10<<' '<<m.s.M11<<' '<<m.s.M12<<'\n';
  s<<m.s.M20<<' '<<m.s.M21<<' '<<m.s.M22<<'\n';
  return s;
}

inline std::ostream &operator<<(std::ostream &s, Matrix4fT &m)
{
  s<<m.s.M00<<' '<<m.s.M01<<' '<<m.s.M02<<' '<<m.s.M03<<'\n';
  s<<m.s.M10<<' '<<m.s.M11<<' '<<m.s.M12<<' '<<m.s.M13<<'\n';
  s<<m.s.M20<<' '<<m.s.M21<<' '<<m.s.M22<<' '<<m.s.M23<<'\n';
  s<<m.s.M30<<' '<<m.s.M31<<' '<<m.s.M32<<' '<<m.s.M33<<'\n';
  return s;
}

inline void setIdentity(Matrix3fT &m)
{
  m.s.M00 = 1; m.s.M01 = 0; m.s.M02 = 0;
  m.s.M10 = 0; m.s.M11 = 1; m.s.M12 = 0;
  m.s.M20 = 0; m.s.M21 = 0; m.s.M22 = 1;
}

inline void setIdentity(Matrix4fT &m)
{
  m.s.M00 = 1; m.s.M01 = 0; m.s.M02 = 0; m.s.M03 = 0;
  m.s.M10 = 0; m.s.M11 = 1; m.s.M12 = 0; m.s.M13 = 0;
  m.s.M20 = 0; m.s.M21 = 0; m.s.M22 = 1; m.s.M23 = 0;
  m.s.M30 = 0; m.s.M31 = 0; m.s.M32 = 0; m.s.M33 = 1;
}

inline void extend(Matrix4fT &m, Matrix3fT &s)
{
  m.s.M00 = s.s.M00; m.s.M01 = s.s.M01; m.s.M02 = s.s.M02; m.s.M03 = 0;
  m.s.M10 = s.s.M10; m.s.M11 = s.s.M11; m.s.M12 = s.s.M12; m.s.M13 = 0;
  m.s.M20 = s.s.M20; m.s.M21 = s.s.M21; m.s.M22 = s.s.M22; m.s.M23 = 0;
  m.s.M30 =       0; m.s.M31 =       0; m.s.M32 =       0; m.s.M33 = 1;
}

GLViewController::GLViewController() :
  clearColor(0.5625,0.5625,0.78125f),
  arcBall(100,100),
  light0(GL_LIGHT0),
  light1(GL_LIGHT1),
  light2(GL_LIGHT2),
  viewportXdim(1), viewportYdim(1),retinaScale(1.0f)
{
  //	Matrix4fSetIdentity(&Transform); // NEW: Final Transform
  setIdentity(Transform);
  Matrix3fSetIdentity(&LastRot);   // NEW: Last Rotation
  Matrix3fSetIdentity(&ThisRot);
  light0.enabled=true;
  light1.enabled=false;
  light2.enabled=false;
  light1.position=DSPoint(0.0,1.0,0.0);
  light1.diffuseColor=DSPoint(1,1,1);

}

void GLViewController::leftButtonDown(Point2D p)
{
  mouseDownPoint = p;
  LastRot = ThisRot;										// Set Last Static Rotation To Last Dynamic One
  Point2fT MousePt;
  MousePt.s.X = (GLfloat)p.x*retinaScale;
  MousePt.s.Y = (GLfloat)p.y*retinaScale;
  arcBall.click(&MousePt);							// Update Start Vector And Prepare For Dragging
}

bool GLViewController::mouseMove(uint32 nFlags, Point2D point)
{
  Point2D delta = point-mouseDownPoint;
  bool altKeyDown = nFlags & Qt::AltModifier;
  if (altKeyDown)
  {
    properties.xShift += (viewportXdim>0) ? delta.x / (float)viewportXdim : 0;
    properties.yShift -= (viewportYdim>0) ? delta.y / (float)viewportXdim : 0;
  }
  else if ((nFlags&Qt::ControlModifier)||(nFlags & Qt::MiddleButton ))
  {
    properties.zoom *= (((delta.y)>0) ? 1.1f : 1.0f/1.1f);
  }
  else
  {
    Quat4fT     ThisQuat;
    Point2fT MousePt;
    MousePt.s.X = (GLfloat)mouseDownPoint.x*retinaScale;
    MousePt.s.Y = (GLfloat)mouseDownPoint.y*retinaScale;
    arcBall.drag(&MousePt, &ThisQuat);						// Update End Vector And Get Rotation As Quaternion
    Matrix3fSetRotationFromQuat4f(&ThisRot, &ThisQuat);		// Convert Quaternion Into Matrix3fT
    Matrix3fMulMatrix3f(&ThisRot, &LastRot);				// Accumulate Last Rotation Into This One
    Matrix4fSetRotationFromMatrix3f(&Transform, &ThisRot);	// Set Our Final Transform's Rotation From This One
    DSPoint angles = getRotationAngles(ThisRot);
    properties.xAngle = angles.x;
    properties.yAngle = angles.y;
    properties.zAngle = angles.z;

  }
  mouseDownPoint = point;
  return true;
}

inline void makeRotationMatrix(Matrix3fT &m, const float degrees, const float x, const float y, const float z)
{
  const float s = (float)sin(degrees * M_PI/180.0f);
  const float c = (float)cos(degrees * M_PI/180.0f);
  m.s.M00 = x*x*(1-c)+c;		m.s.M01 = x*y*(1-c)-z*s;		m.s.M02 = x*z*(1-c)+y*s;
  m.s.M10 = x*y*(1-c)+z*s;	m.s.M11 = y*y*(1-c)+c;			m.s.M12 = y*z*(1-c)-x*s;
  m.s.M20 = x*z*(1-c)-y*s;	m.s.M11 = y*z*(1-c)+x*s;		m.s.M12 = z*z*(1-c)+c;
}

void GLViewController::rotation()
{
  glLoadIdentity();
  glMultMatrixf(Transform.M);
}

void GLViewController::axes()
{
  glBegin(GL_LINES);
  ::glColor3f(0,0,1);
  glVertex3f(0,0,0);
  glVertex3f(0,0,10);
  glColor3f(0,1,0);
  glVertex3f(0,0,0);
  glVertex3f(0,10,0);
  glColor3f(1,0,0);
  glVertex3f(0,0,0);
  glVertex3f(10,0,0);
  glEnd();
}

void GLViewController::lighting(bool on)
{
  if (on)
  {
    glEnable(GL_LIGHTING);
    light0.lightItUp();
    light1.lightItUp();
    light2.lightItUp();
  }
  else
  {
    glDisable(GL_LIGHTING);
  }
}

bool GLViewController::initialize()
{
  ::glClearColor(clearColor.x,clearColor.y,clearColor.z,1);
  ::glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  const float aspectRatio = (viewportYdim>0) ? (float)(viewportXdim)/float(viewportYdim) : 1.0f;
  GLfloat a = properties.extent * properties.zoom;
  GLfloat b = (float)(a * aspectRatio);
  GLfloat depth = 2500.0f * properties.zoom;
  float scale = properties.zoom/2.0f;
  float xShiftPixels = -properties.xShift * viewportYdim * scale; // shift is in screen pixels!
  float yShiftPixels = -properties.yShift * viewportXdim * scale;
  ::glMatrixMode(GL_PROJECTION);
  ::glLoadIdentity();
  ::glOrtho(xShiftPixels-b,xShiftPixels+b,yShiftPixels-a,yShiftPixels+a,-depth,depth);
  ::glViewport(0, 0, viewportXdim, viewportYdim);
  ::glMatrixMode(GL_MODELVIEW);
  ::glLoadIdentity();
  if (viewportXdim>0 && viewportYdim>0)
    arcBall.setBounds((float)viewportXdim,(float)viewportYdim);
  else if (viewportXdim>0)
    arcBall.setBounds((float)viewportXdim,(float)1);
  else if (viewportYdim>0)
    arcBall.setBounds((float)1,(float)viewportYdim);
  return true;
}

bool GLViewController::setup(const int viewportX, const int viewportY)
{
  viewportXdim = viewportX;
  viewportYdim = viewportY;
  return true;
}

void GLViewController::setRotation(const float xAngle, const float yAngle, const float zAngle)
{
  properties.xAngle = xAngle;
  properties.yAngle = yAngle;
  properties.zAngle = zAngle;
  Matrix3fT m;
  setIdentity(ThisRot);
  rotY(m,properties.yAngle); Matrix3fMulMatrix3f(&ThisRot,&m);
  rotX(m,properties.xAngle); Matrix3fMulMatrix3f(&ThisRot,&m);
  rotZ(m,properties.zAngle); Matrix3fMulMatrix3f(&ThisRot,&m);
  extend(Transform,ThisRot);
}
