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

#include <cstdlib>
#include <algorithm>
#include <string>
#include <connectivityrenderer.h>
#include <connectivitymap.h>
#include <glcompatibility.h>
#include <connectivityproperties.h>
#include <brainsuitesettings.h>
#include <qpainter.h>
#include <glframe2d.h>

float ConnectivityRenderer::arcWidthBase=1.0f;
float ConnectivityRenderer::arcWidthScale=5.0f;
float ConnectivityRenderer::textSize=0.10f;
float ConnectivityRenderer::rotationShift=(float)(-M_PI/2);

inline void dsColor(const DSPoint &c)
{
  ::glColor3f(c.x,c.y,c.z);
}


inline bool inRange(const int value, const int lower, const int upper)
{
  return (value>=lower && value < upper);
}

ConnectivityRenderer::ConnectivityRenderer() :
  clearColor(1,1,1),
  mainTextColor(0,0,0),
  nodeTextColor(0,0,0),
  highlightedTextColor(0,0,0),
  nodeHighlightColor(0,1,0),
  nodeColor(0,0.5f,0),
  linkColorA(0,0,1),
  linkColorB(1,0,0),
  highlight(-1), weightThreshold(0.05f), circleRadius(0.3f), textZoom(1.03f),
  drawArcs(true),
  showNodeNames(true),
  useAbbrevs(false),
  groupMode(false),
  dither(true),
  ditherDelta(0.1f),
  computing(false),
  retinaScale(1.0f)
{
  usePrintColors();
  slider.frameColor = mainTextColor;
}

void ConnectivityRenderer::drawQtText(QPainter &painter, std::string text, float x, float y, float angle, DSPoint color)//=DSPoint(1,1,1))//, void *font=0)
{
  painter.save();
  QColor c;
  c.setRgbF(color.x,color.y,color.z);
  painter.setPen(c);
  painter.translate(x,y);
  painter.rotate(angle * 180.0f/M_PI);
  painter.drawText(0,0,text.c_str());
  painter.restore();
}

void ConnectivityRenderer::drawNodeText(QPainter &painter, ConnectivityMap &connectivityMap)
{
  float centerx=width()/2;
  float centery=height()/2;
  float zoom=std::min(width(), height())*circleRadius;
  const bool useSubset = (connectivitySubset.size()>0);
  const size_t nElements = useSubset ? connectivitySubset.size() : connectivityMap.size();
  for (size_t i=0;i<nElements;i++)
  {
    int nodeID = (useSubset) ? connectivitySubset[i] : (int)i; // negatives may represent invalid connections...
    float theta=i*2*M_PI/nElements+rotationShift;
    float tx = centerx+textZoom*zoom*cos(theta);
    float ty = centery+textZoom*zoom*sin(theta);
    const bool selected=isSelected((int)i);
    if (showNodeNames)
    {
      if (selected)
      {
        dsColor(highlightedTextColor);
      }
      else
      {
        dsColor(nodeTextColor);
      }
      drawQtText(painter,connectivityMap.nodeNames[nodeID],tx,ty,theta, selected ? highlightedTextColor : nodeTextColor);
    }
  }
}

//TODO : remove con props
void ConnectivityRenderer::render(QPainter &painter, ConnectivityMap &connectivityMap,
                                    std::vector<LabelDetails> &labelDetails)
{
  const bool useSubset = (connectivitySubset.size()>0);
  const size_t nElements = useSubset ? connectivitySubset.size() : connectivityMap.size();
  rotationShift = -M_PI/2 + M_PI/(nElements);
  int sliderWidth=25;
  int sliderHeight=int(height()*0.3);
  int topMargin=10;
  float textHeight=18;
  float textSpacing=2 + textHeight;
  float textY=slider.y0+topMargin;
  glClearColor(0.5f, 0.0f, 0.5f, 0.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //Initialize OpenGL projection matrix
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glLoadIdentity();
  glEnable2D();
  glClear(GL_COLOR_BUFFER_BIT);
  slider.renderQt(painter,width()-sliderWidth-10, 0+topMargin, sliderWidth, sliderHeight, weightThreshold);
  if (computing)
  {
    drawQtText(painter,"Computing connectivity...", 10.0f, textY, 0, nodeTextColor);
  }
  else if (connectivityMap.size()>0)
  {
    drawLinks(painter,connectivityMap);
    drawNodes(painter,connectivityMap,labelDetails);
    dsColor(nodeTextColor);
    drawNodeText(painter,connectivityMap);
    textY = slider.y0+topMargin;
    dsColor(nodeTextColor);
    if (highlight>=0)
    {
      std::ostringstream ostr;
      int id = useSubset ? connectivitySubset[highlight] : highlight;
      ostr<<"Showing connectivity for "<<connectivityMap.nodeNames[id];
      drawQtText(painter,ostr.str(), 10.0f, textY, 0, nodeTextColor);
    }
    else
    {
      std::ostringstream ostr;
      if (useSubset)
        ostr<<"Showing connectivity for "<<subsetName;
      else
        ostr<<"Showing connectivity for all nodes";
      drawQtText(painter,ostr.str(), 10.0f, textY, 0, nodeTextColor);
    }
  }
  else
  {
    drawQtText(painter,"Connectivity has not been computed.", 10.0f, textY, 0, nodeTextColor);
  }
  textY += textSpacing;
  if (statusMessage.empty()==false)
  {
    drawQtText(painter,statusMessage, 10.0f, height()-2*textSpacing, 0, nodeTextColor);
  }
  {
    float sliderTextY=slider.y0+topMargin;
    drawQtText(painter,"Threshold ", slider.x0-90, sliderTextY, (float)(-2*M_PI),mainTextColor);//,defaultFontBitmap); // why???
    std::ostringstream ostr;
    ostr<<(int)(weightThreshold*100)<<"% ";
    drawQtText(painter,ostr.str(), slider.x0-40, sliderTextY+=textSpacing, (float)(-2*M_PI),mainTextColor);//,defaultFontBitmap);
  }
}
void ConnectivityRenderer::useScreenColors()
{
  clearColor=DSPoint(0,0,0);
  mainTextColor=DSPoint(1,1,1);
  nodeTextColor=DSPoint(0.7f,0.7f,0.7f);
  highlightedTextColor=DSPoint(1,1,1);
  nodeHighlightColor=DSPoint(0,1,0);
  nodeColor=DSPoint(0,0.75f,0);
  linkColorA=DSPoint(0,0,1);
  linkColorB=DSPoint(1,0,0);
  dither=(false);
  ditherDelta=(0.0f);
}

void ConnectivityRenderer::usePrintColors()
{
  clearColor=DSPoint(1,1,1);
  mainTextColor=DSPoint(0.3f,0.3f,0.3f);
  nodeTextColor=DSPoint(0.3f,0.3f,0.3f);
  highlightedTextColor=DSPoint(0,0,0);
  nodeHighlightColor=DSPoint(0,1,0);
  nodeColor=DSPoint(0,0.5f,0);
  linkColorA=DSPoint(0,0,1);
  linkColorB=DSPoint(1,0,0);
  dither=(false);
  ditherDelta=(0.1f);
}

bool ConnectivityRenderer::inSlider(float x, float y)
{
  return slider.inside(x,y);
}

void ConnectivityRenderer::moveSlider(float /*x*/, float y)
{
  float dy = slider.y1 - y;
  float range = (float)std::abs(slider.y1 - slider.y0);
  if (range>0)
    weightThreshold = dy/range;
  else
    weightThreshold = 0.0f;
  if (weightThreshold>1.0f) weightThreshold = 1.0f;
  else if (weightThreshold<0.0f) weightThreshold = 0.0f;
}

void ConnectivityRenderer::initialize()
{
}

void ConnectivityRenderer::drawArcQt(QPainter &painter, const float x, const float y, const float r, const float theta0, const float theta1)
{
  const float dTheta=theta1-theta0;
  const float arclength = std::ceil(dTheta * r);
  const int nSamples=(r<20) ? 20 : arclength;
  QPointF p0(x+r*cos(theta0),y+r*sin(theta0));
  for (int i=1;i<nSamples;i++)
  {
    const float angle=theta0 + (dTheta * i)/nSamples;
    QPointF p1(x+r*cos(angle),y+r*sin(angle));
    painter.drawLine(p0,p1);
    p0=p1;
  }
}

bool ConnectivityRenderer::isSelected(const int nodeID)
{
  return std::find(selection.begin(),selection.end(),nodeID)!=selection.end();
}

void ConnectivityRenderer::clearSelection()
{
  highlight = -1;
  selection.resize(0);
}

QColor colorFromPt(const DSPoint &p)
{
  return QColor(p.x*255,p.y*255,p.z*255);
}

void ConnectivityRenderer::drawNodes(QPainter &painter, ConnectivityMap &connectivityMap, std::vector<LabelDetails> &labelDetails)
{
  painter.save();
  float centerx=width()/2.0f;
  float centery=height()/2.0f;
  float zoom=std::min(width(), height())*circleRadius;
  float nodeSize = std::abs(1.5f*circleRadius*std::min(width(), height())/100);
  const bool useSubset = (connectivitySubset.size()>0);
  const size_t nElements = useSubset ? connectivitySubset.size() : connectivityMap.size();
  for (size_t i=0;i<nElements;i++)
  {
    float theta=(float)(i*2*M_PI/nElements+rotationShift);
    float x = centerx+zoom*cos(theta);
    float y = centery+zoom*sin(theta);
    if (BrainSuiteSettings::showConnectivityNodesUsingLabelColors)
    {
      int labelID=connectivityMap.getLabelID(i);
      QRgb rgb(labelDetails[labelID].color);
      QColor color(rgb);
      painter.setPen(color);
      QBrush qb(color);
      painter.setBrush(qb);
    }
    else
    {
      const bool selected=isSelected((int)i);
      QColor color=colorFromPt(selected ? nodeHighlightColor : nodeColor);
      painter.setPen(color);
    }
    painter.drawEllipse(QPoint(x,y),(int)nodeSize,(int)nodeSize);
  }
  painter.restore();
}

void ConnectivityRenderer::drawLinks(QPainter &painter, ConnectivityMap &connectivityMap)
{
  painter.save();
  const float maxVal = (connectivityMap.maxValue > 0) ? connectivityMap.maxValue : 1;
  int connectivityThreshold=(int)(maxVal*weightThreshold);
  if (connectivityThreshold<=0) connectivityThreshold = 1;
  const bool useSubset = (connectivitySubset.size()>0);
  const size_t nElements = useSubset ? connectivitySubset.size() : connectivityMap.size();

  float centerx=width()/2.0f;
  float centery=height()/2.0f;
  float zoom=std::min(width(), height())*circleRadius;

  for (size_t i=0;i<nElements;i++)
  {
    float thetaI=(float)(i*2*M_PI/nElements+rotationShift);
    float xI = centerx+zoom*cos(thetaI);
    float yI = centery+zoom*sin(thetaI);
    int nodeI = (useSubset) ? connectivitySubset[i] : (int)i;
    bool iIsSelected = isSelected((int)i);
    for (size_t j=i+1;j<nElements;j++)
    {
      bool jIsSelected = isSelected((int)j);
      int nodeJ = (useSubset) ? connectivitySubset[j] : (int)j;
      if (selection.size() && !iIsSelected && !jIsSelected) continue;
      if (groupMode&&selection.size()>1) if (!iIsSelected||!jIsSelected) continue;
      float value = connectivityMap.adjacencyMatrix(nodeI,nodeJ);
      if (value>=connectivityThreshold)
      {
        float thetaJ=(float)(j*2*M_PI/nElements+rotationShift);
        float xJ = centerx+zoom*cos(thetaJ);
        float yJ = centery+zoom*sin(thetaJ);
        float wc = (float)value / (float)maxVal;
        const float lineWidth=(arcWidthBase+wc*arcWidthBase*arcWidthScale);
        const DSPoint lineColor(wc*linkColorB+(1-wc)*linkColorA);
        QPen widthPen;
        widthPen.setWidth(lineWidth);
        widthPen.setColor(colorFromPt(lineColor));
        painter.setPen(widthPen);
        if (drawArcs)
        {
          float theta = 0.5f*(thetaI+thetaJ);
          float dTheta = 0.5f*std::abs(thetaI-thetaJ);
          float r = tan(dTheta);
          if (std::abs(r)>1000) // if the angle is ~90 degrees, r blows up
          {
            painter.drawLine(QPointF(xI,yI),QPointF(xJ,yJ));
          }
          else
          {
            float y = sin(dTheta)*tan(dTheta);
            float R = cos(dTheta)+y;
            float px=zoom*R*cos(theta)+centerx;
            float py=zoom*R*sin(theta)+centery;
            float phiI = atan2(yI-py, xI-px);
            float phiJ = atan2(yJ-py, xJ-px);
            if (phiJ<phiI) phiJ += (float)(2*M_PI);
            if (phiI<0)
            {
              phiI += (float)(2*M_PI);
              phiJ += (float)(2*M_PI);
            }
            float rad=std::abs(r*zoom);
            if (r>0)
            {
              drawArcQt(painter, px, py, rad, phiJ, (float)(phiI+2*M_PI));
            }
            else
            {
              drawArcQt(painter, px, py, rad, phiI, phiJ);
            }
          }
        }
        else
        {
          painter.drawLine(QPointF(xI,yI),QPointF(xJ,yJ));
        }
      }
    }
  }
  painter.restore();
}

void ConnectivityRenderer::glEnable2D()
{
  glClearColor(clearColor.x,clearColor.y,clearColor.z,0.0f);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  //Clear back buffer
  glClear(GL_COLOR_BUFFER_BIT);
  glGetIntegerv(GL_VIEWPORT, vPort);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, vPort[2], vPort[3], 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  vPort[2]/=retinaScale;
  vPort[3]/=retinaScale;
}

void ConnectivityRenderer::glDisable2D()
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

int ConnectivityRenderer::mapSector(ConnectivityMap &connectivityMap, float x, float y)
{
  const bool useSubset = (connectivitySubset.size()>0);
  const size_t nElements = useSubset ? connectivitySubset.size() : connectivityMap.size();

  rotationShift = (float)(-M_PI/2 + M_PI/(nElements));

  float centerx = width()/2.0f;
  float centery = height()/2.0f;
  float dx = x-centerx;
  float dy = y-centery;
  float angle = (float)(atan2(dy, dx)-rotationShift+(2*M_PI/nElements/2));
  if (angle<0) angle+=(float)(2*M_PI);
  if (angle>=2*M_PI) angle-=(float)(2*M_PI);
  float r2 = dx*dx+dy*dy;
  float zoom=0.5f * std::min(width(), height())*circleRadius;
  if (r2<(zoom*zoom)) return -1;
  return int(nElements*angle/(2*M_PI));
}

bool ConnectivityRenderer::getSelectedLabels(std::vector<int> &labelIDs, const ConnectivityMap &connectivityMap)
{
  const bool useSubset = (connectivitySubset.size()>0);
  labelIDs.resize(selection.size());
  for (size_t i=0;i<selection.size();i++)
  {
    int nodeIndex = selection[i];
    int nodeID = (useSubset) ? connectivitySubset[nodeIndex] : nodeIndex;
    labelIDs[i] = connectivityMap.getLabelID(nodeID);
  }
  return (labelIDs.size()>0);
}

// here the node ID is the row / column of the matrix
bool ConnectivityRenderer::getSelectedNodes(std::vector<int> &nodeIndices)
{
  const bool useSubset = (connectivitySubset.size()>0);
  nodeIndices.resize(selection.size());
  for (size_t i=0;i<selection.size();i++)
  {
    int nodeIndex = selection[i];
    nodeIndices[i] = (useSubset) ? connectivitySubset[nodeIndex] : nodeIndex;
  }
  return (nodeIndices.size()>0);
}


int ConnectivityRenderer::getSelectedLabel(ConnectivityMap &connectivityMap)
{
  const bool useSubset = (connectivitySubset.size()>0);
  int id = (useSubset) ? connectivitySubset[highlight] : highlight;
  return connectivityMap.getLabelID(id);
}

bool ConnectivityRenderer::makeSelection(ConnectivityMap &connectivityMap, float x, float y)
{
  bool stateChange=false;
  int sector = mapSector(connectivityMap,x,y);
  if (selection.size()==1 && selection[0]==sector) return false;
  if (sector>=0)
  {
    selection.resize(1);
    selection[0] = sector;
  }
  else
    selection.resize(0);
  highlight = sector;
  stateChange = true;
  return stateChange;
}

void ConnectivityRenderer::pickSubrange(ConnectivityMap &connectivityMap, const int lower, const int upper)
{
  connectivitySubset.resize(0);
  for (size_t i=0;i<connectivityMap.labelIDs.size();i++)
  {
    if (inRange(connectivityMap.labelIDs[i],lower,upper))
    {
      connectivitySubset.push_back((int)i);
    }
  }
}

void ConnectivityRenderer::sortOddEven(std::vector<int> &selectionVector, ConnectivityMap &connectivityMap)
{
  std::vector<int> left;
  std::vector<int> right;
  for (size_t i=0;i<selectionVector.size();i++)
  {
    int id = connectivityMap.labelIDs[selectionVector[i]];
    if (id&1)
      left.push_back(selectionVector[i]);
    else
      right.push_back(selectionVector[i]);
  }
  const size_t nLeft=left.size();
  for (size_t i=0;i<nLeft;i++)
    right.push_back(left[nLeft-i-1]);
  selectionVector = right;
}

void ConnectivityRenderer::connectivityShowAllBrain(ConnectivityMap &connectivityMap)
{
  subsetName="all brain areas";
  connectivitySubset.resize(0);
  for (size_t i=0;i<connectivityMap.labelIDs.size();i++)
  {
    if (inRange(connectivityMap.labelIDs[i],100,700)||inRange(connectivityMap.labelIDs[i],800,1000))
    {
      connectivitySubset.push_back((int)i);
    }
  }
  sortOddEven(connectivitySubset,connectivityMap);
  clearSelection();
  BrainSuiteSettings::showTrackSubset=false;
}

bool ConnectivityRenderer::addSecondSelection(ConnectivityMap &connectivityMap, float x, float y)
{
  bool stateChange=false;
  int sector = mapSector(connectivityMap,x,y);
  if (sector<0)
  {
    highlight = sector;
    if (selection.size()>0)
    {
      selection.resize(0);
      return true;
    }
    else
      return false;
  }
  if (isSelected(sector)) return false;
  if (selection.size()>1)
    selection.resize(1);
  selection.push_back(sector);
  highlight = sector;
  stateChange = true;
  return stateChange;
}


bool ConnectivityRenderer::addSelection(ConnectivityMap &connectivityMap, float x, float y)
{
  bool stateChange=false;
  int sector = mapSector(connectivityMap,x,y);
  if (sector<0)
  {
    highlight = sector;
    if (selection.size()>0)
    {
      selection.resize(0);
      return true;
    }
    else
      return false;
  }
  if (isSelected(sector)) return false;
  selection.push_back(sector);
  highlight = sector;
  stateChange = true;
  return stateChange;
}

