# Copyright (C) 2016 The Regents of the University of California
#
# Created by David W. Shattuck, Ph.D.
#
# This file is part of BrainSuite16a1.
#
# BrainSuite16a1 is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
# USA.
#

BUILDVERSION=2777
VERSTR = '\\"$${BUILDVERSION}\\"'  # place quotes around the version string
DEFINES += BUILDVERSION=\"$${VERSTR}\" # create a VER macro containing the version string
message($$BUILDVERSION)

QT += core gui widgets
QT += opengl
QT += network
TARGET = BrainSuite16a1
TEMPLATE = app
DEFINES += "QT_BUILD"
DEFINES += "NOMINMAX"
RC_FILE = brainsuiteico.rc
CONFIG += c++11
DEFINES += PROGNAME='\\"$${TARGET}\\"'
QMAKE_MAC_SDK = macosx10.11

macx {
ICON = images/brainsuite_icon_512px.icns
LIBS += -lz -lm
QMAKE_CXXFLAGS += -std=c++11 -stdlib=libc++
}


unix:!macx:!symbian: LIBS += -L$$PWD/../lib/x86_64-pc-linux-gnu
unix:!macx:!symbian: LIBS += -lGLU -lz
unix:!macx:!symbian: QMAKE_CXXFLAGS+=-std=c++11
unix:INCLUDEPATH += /usr/local/include
unix:INCLUDEPATH += $$PWD/../3rdParty/include

win32 {
## Windows common build here
LIBS += -lopengl32 -lglu32
win32-msvc2010:PLATFORMTOOLSET=v100
win32-msvc2012:PLATFORMTOOLSET=v110
win32-msvc2013:PLATFORMTOOLSET=v120
INCLUDEPATH += $$PWD/../3rdParty/zlib-1.2.8
DEFINES += "_AFXDLL"
DEFINES += "ZLIB_WINAPI"
DEFINES += "_USE_MATH_DEFINES"
LIBDIR = $$PWD/../../lib/
!contains(QMAKE_HOST.arch, x86_64) {
## Windows x86 (32bit) specific build here
                message("x86 build")
                QMAKE_LFLAGS_RELEASE += /LARGEADDRESSAWARE
                DEFINES += "QT_X86_32"
                LIBS += -L$${LIBDIR}win32/$${PLATFORMTOOLSET}
                CONFIG(release, debug|release): LIBS += -lzlib_Win32_MD
                else:CONFIG(debug, debug|release): LIBS += -lzlib_Win32_MDd
        } else {
## Windows x64 (64bit) specific build here
                message("x86_64 build")
                DEFINES += "QT_X86_64"
                LIBS += -L$${LIBDIR}x64/$${PLATFORMTOOLSET}
                CONFIG(release, debug|release): LIBS += -lzlib_x64_MD
                else:CONFIG(debug, debug|release): LIBS += -lzlib_x64_MDd
        }
	QMAKE_LFLAGS_RELEASE += /LTCG
} else {
}

INCLUDEPATH += .
INCLUDEPATH += $$PWD/../include
INCLUDEPATH += $$PWD/../corticalextraction/include
INCLUDEPATH += $$PWD/../corticalextraction/skullfinder
INCLUDEPATH += $$PWD/../corticalextraction/pialmesh
INCLUDEPATH += $$PWD/../diffusion/include
INCLUDEPATH += $$PWD/../curvelib
INCLUDEPATH += $$PWD/../surflib
INCLUDEPATH += $$PWD/../3rdParty/include

SOURCES += main.cpp \
    aboutbrainsuitedialog.cpp \
    bfcguitool.cpp \
    bfcthreaddialog.cpp \
    biasfieldcorrectionthread.cpp \
    brainsuitedatamanager.cpp \
    brainsuiteinstalldialog.cpp \
    brainsuiteqtsettings.cpp \
    brainsuiterenderer.cpp \
    brainsuitesettings.cpp \
    brainsuitewindow.cpp \
    bseguitool.cpp \
    bsw_diffusion.cpp \
    bsw_filehandlers.cpp \
    bsw_initialization.cpp \
    bsw_keypress.cpp \
    cerebroguitool.cpp \
    cerebrumextractiondialog.cpp \
    cerebrumlabelerqt.cpp \
    clicklabel.cpp \
    colorselectionbutton.cpp \
    computeconnectivitythread.cpp \
    computeconnectivitythreaddialog.cpp \
    computerange.cpp \
    connectivityglwidget.cpp \
    connectivityrenderer.cpp \
    connectivityviewdialog.cpp \
    consoletext.cpp \
    cortexidentificationdialog.cpp \
    cortexmodelguitool.cpp \
    corticalextractiondata.cpp \
    corticalextractionsequencedialog_parameters.cpp \
    corticalextractionsequencedialog.cpp \
    corticalextractionsettings.cpp \
    curvetoolboxform.cpp \
    delineationtoolboxform.cpp \
    dewispdialog.cpp \
    dewispguitool.cpp \
    diffusiontoolboxform_filtertracks.cpp \
    diffusiontoolboxform.cpp \
    dsfilelabel.cpp \
    dsfilepathbutton.cpp \
    dsglslider.cpp \
    dsglviewdialog.cpp \
    dsglwidget_events.cpp \
    dsglwidget_slices.cpp \
    dsglwidget_toggles.cpp \
    dsglwidget.cpp \
    dsimage.cpp \
    dsimagerangecontrol.cpp \
    dslight.cpp \
    dsrangeslider.cpp \
    dstube.cpp \
    dualregistrationprocess.cpp \
    dualregistrationprocessdialog.cpp \
    extractionstages.cpp \
    fiberrenderer.cpp \
    filedownloader.cpp \
    floatslider.cpp \
    glframe2d.cpp \
    glglyphs.cpp \
    glviewcontroller.cpp \
    gotocoordinatedialog.cpp \
    hemisplitguitool.cpp \
    imagedisplaypropertiesform.cpp \
    imageframe.cpp \
    imagewidget.cpp \
    innercorticalsurfacedialog.cpp \
    innercorticalsurfaceguitool.cpp \
    labeldescriptions.cpp \
    latestversionmanifest.cpp \
    lutmenusystem.cpp \
    nonuniformitycorrectiondialog.cpp \
    pialsurfacedialog.cpp \
    pialsurfaceguitool.cpp \
    pialsurfacethread.cpp \
    pialsurfacethreaddialog.cpp \
    pialsurfacetool.cpp \
    processingthreaddialog.cpp \
    pvcguitool.cpp \
    registrationprocessdialog.cpp \
    scrubmaskdialog.cpp \
    scrubmaskguitool.cpp \
    skullandscalpdialog.cpp \
    skullfinderguitool.cpp \
    skullstrippingdialog.cpp \
    splithemispheresdialog.cpp \
    surfacedisplayform.cpp \
    surfaceinstances.cpp \
    surfacelabelprocess.cpp \
    surfacerecolor.cpp \
    svregguitool.cpp \
    svregistrationdialog.cpp \
    svregistrationtool.cpp \
    svregmanifest.cpp \
    tissueclassificationdialog.cpp \
    toolmanager.cpp \
    topologicalcorrectiondialog.cpp \
    topologycorrectionguitool.cpp \
    welcomesplashpage.cpp \
    buildversion.cpp

# 3rd Party Code
SOURCES += \
    ../3rdParty/src/arcball.cpp

# vol3dlib
SOURCES += \
  ../vol3d/codec32.cpp \
  ../vol3d/colormap.cpp \
  ../vol3d/graph.cpp \
  ../vol3d/morph32.cpp \
  ../vol3d/niftiparser.cpp \
  ../vol3d/runlengthsegmenter.cpp \
  ../vol3d/vol3dbase.cpp \
  ../vol3d/vol3dops.cpp \
  ../vol3d/vol3dquery.cpp \
  ../vol3d/volumeloader.cpp \
  ../vol3d/vol3dreorder.cpp \
  ../vol3d/volumescaler.cpp

# cortical extraction
SOURCES += \
  ../corticalextraction/bse/anisotropicdiffusionfilter.cpp \
  ../corticalextraction/bse/bsetool.cpp \
  ../corticalextraction/skullfinder/skullandscalptool.cpp \
  ../corticalextraction/bfc/bfcloop.cpp \
  ../corticalextraction/bfc/bfctool.cpp \
  ../corticalextraction/bfc/bfcutils.cpp \
  ../corticalextraction/bfc/biasdensitymodel.cpp \
  ../corticalextraction/bfc/biasfieldcorrector.cpp \
  ../corticalextraction/bfc/biasmodelfit.cpp \
  ../corticalextraction/bfc/clampcopy.cpp \
  ../corticalextraction/bfc/computebiaspoints.cpp \
  ../corticalextraction/bfc/histtest.cpp \
  ../corticalextraction/bfc/regcomputebiasfloat.cpp \
  ../corticalextraction/bfc/regcomputemaskedbiasfloat.cpp \
  ../corticalextraction/bfc/regremovebias.cpp \
  ../corticalextraction/bfc/regremovebiasmasked.cpp \
  ../corticalextraction/bfc/regularizedsplinef.cpp \
  ../corticalextraction/bfc/roihistogram.cpp \
  ../corticalextraction/pvc/gaussian.cpp \
  ../corticalextraction/pvc/partialvolumemodel.cpp \
  ../corticalextraction/pvc/pvctool.cpp \
  ../corticalextraction/cortex/cortexmodeler.cpp \
  ../corticalextraction/scrubmask/maskscrubber.cpp \
  ../corticalextraction/tca/slicesegmenter.cpp \
  ../corticalextraction/tca/tcafixbackground.cpp \
  ../corticalextraction/tca/tcafixchecker.cpp \
  ../corticalextraction/tca/tcafixforeground.cpp \
  ../corticalextraction/tca/topofixer.cpp \
  ../corticalextraction/tca/topologycorrector.cpp \
  ../corticalextraction/dewisp/dewisp.cpp \
  ../corticalextraction/dewisp/dewisptool.cpp \
  ../corticalextraction/dfs/marchingcubes.cpp \
  ../corticalextraction/dfs/marchingcubesinstances.cpp \
  ../corticalextraction/pialmesh/collisiondata.cpp \
  ../corticalextraction/pialmesh/collisionlist.cpp \
  ../corticalextraction/pialmesh/surfaceexpander.cpp \
  ../corticalextraction/hemisplit/brainsplitter.cpp \
  ../corticalextraction/hemisplit/edgemapper.cpp

# curves & surfaces
SOURCES += \
  ../curvelib/protocolcurveset.cpp \
  ../surflib/siltsurface.cpp \
  ../surflib/surflib.cpp

# diffusion tools
SOURCES += \
  ../diffusion/tracks/trackfilter.cpp \
	../diffusion/odfmax/harditool.cpp \
	../diffusion/odfmax/findodfmaximamanager.cpp \
	../diffusion/odfmax/findodfmaximathread.cpp \
	../diffusion/odfmax/odffactory.cpp \
  ../diffusion/harditool/hardirenderer.cpp \
  ../diffusion/harditool/odffactory_gl.cpp \
  ../diffusion/htrack/htracker.cpp \
  ../diffusion/htrack/fibertrackset.cpp \
  ../diffusion/htrack/fibertrackerthread.cpp \
  ../diffusion/conmat/connectivitymap.cpp \
  ../diffusion/conmat/connectivityproperties.cpp

FORMS += \
  aboutbrainsuitedialog.ui \
  bfcthreaddialog.ui \
  brainsuiteinstalldialog.ui \
  brainsuitewindow.ui \
  cerebrumextractiondialog.ui \
  computeconnectivitythreaddialog.ui \
  connectivityviewdialog.ui \
  cortexidentificationdialog.ui \
  corticalextractionsequencedialog.ui \
  curvetoolboxform.ui \
  delineationtoolboxform.ui \
  dewispdialog.ui \
  diffusiontoolboxform.ui \
  dsglviewdialog.ui \
  dualregistrationprocessdialog.ui \
  gotocoordinatedialog.ui \
  imagedisplaypropertiesform.ui \
  imageframe.ui \
  innercorticalsurfacedialog.ui \
  nonuniformitycorrectiondialog.ui \
  pialsurfacedialog.ui \
  pialsurfacethreaddialog.ui \
  processingthreaddialog.ui \
  registrationprocessdialog.ui \
  scrubmaskdialog.ui \
  skullandscalpdialog.ui \
  skullstrippingdialog.ui \
  splithemispheresdialog.ui \
  surfacedisplayform.ui \
  svregistrationdialog.ui \
  tissueclassificationdialog.ui \
  topologicalcorrectiondialog.ui \
  welcomesplashpage.ui \

HEADERS += \
    aboutbrainsuitedialog.h \
    bfcguitool.h \
    bfcthreaddialog.h \
    biasfieldcorrectionthread.h \
    brainsuitedatamanager.h \
    brainsuiteinstalldialog.h \
    brainsuitemanifest.h \
    brainsuiteqtsettings.h \
    brainsuiterenderer.h \
    brainsuitesettings.h \
    brainsuitewindow_t.h \
    brainsuitewindow.h \
    bseguitool.h \
    buildversion.h \
    cerebroguitool.h \
    cerebrumextractiondialog.h \
    cerebrumlabelerqt.h \
    clicklabel.h \
    colorops.h \
    colorselectionbutton.h \
    computeconnectivitythread.h \
    computeconnectivitythreaddialog.h \
    computerange.h \
    connectivityglwidget.h \
    connectivityrenderer.h \
    connectivityviewdialog.h \
    consoletext.h \
    cortexidentificationdialog.h \
    cortexmodelguitool.h \
    corticalextractiondata.h \
    corticalextractionsequencedialog.h \
    corticalextractionsettings.h \
    curvetoolboxform.h \
    cylinder.h \
    delineationtoolboxform.h \
    dewispdialog.h \
    dewispguitool.h \
    diffusioncolorops.h \
    diffusiontoolboxform.h \
    displaycode.h \
    dsfilelabel.h \
    dsfilepathbutton.h \
    dsglslider.h \
    dsglviewdialog.h \
    dsglwidget.h \
    dsimage.h \
    dsimagerangecontrol.h \
    dslight.h \
    dsrangeslider.h \
    dstube.h \
    dualregistrationprocess.h \
    dualregistrationprocessdialog.h \
    extractionguitool.h \
    extractionstages.h \
    fiberrenderer.h \
    filedownloader.h \
    floatslider.h \
    glcompatibility.h \
    glframe2d.h \
    glglyphs.h \
    glslicerender.h \
    glviewcontroller.h \
    gotocoordinatedialog.h \
    hardirenderer.h \
    hemisplitguitool.h \
    imagedisplaypropertiesform.h \
    imageframe.h \
    imagestate.h \
    imagewidget.h \
    innercorticalsurfacedialog.h \
    innercorticalsurfaceguitool.h \
    labelbrush.h \
    latestversionmanifest.h \
    lutmenusystem.h \
    nonuniformitycorrectiondialog.h \
    orientation.h \
    pialsurfacedialog.h \
    pialsurfaceguitool.h \
    pialsurfacethread.h \
    pialsurfacethreaddialog.h \
    pialsurfacetool.h \
    processingthreaddialog.h \
    pvcguitool.h \
    qtrendertemplates.h \
    quietset.h \
    registrationprocessdialog.h \
    scrubmaskdialog.h \
    scrubmaskguitool.h \
    skullandscalpdialog.h \
    skullfinderguitool.h \
    skullstrippingdialog.h \
    slicefill.h \
    splithemispheresdialog.h \
    surfacealpha.h \
    surfacedisplayform.h \
    surfaceinstances.h \
    surfacelabelprocess.h \
    surfacerecolor.h \
    surfacerecolorchoices.h \
    surfaceviewproperties.h \
    svregguitool.h \
    svregistrationdialog.h \
    svregistrationtool.h \
    svregmanifest.h \
    tensorglyphrenderer_t.h \
    tensorglyphrenderer.h \
    threaddialogstatus.h \
    thresholdtools.h \
    tissueclassificationdialog.h \
    toolmanager.h \
    topologicalcorrectiondialog.h \
    topologycorrectionguitool.h \
    undosystem.h \
    volumepainter.h \
    waitcursor.h \
    welcomesplashpage.h \
    ../include/allocator.h \
    ../include/argparser.h \
    ../include/autocrop.h \
    ../include/buildversiondef.h \
    ../include/byteorder.h \
    ../include/collector.h \
    ../include/colormap.h \
    ../include/commonerrors.h \
    ../include/connectionlist.h \
    ../include/dfsheader.h \
    ../include/dsnifti.h \
    ../include/dspoint.h \
    ../include/eigensystem3x3.h \
    ../include/endianswap.h \
    ../include/graph.h \
    ../include/histogramsmoother.h \
    ../include/ipoint3d.h \
    ../include/labeldescriptionset.h \
    ../include/labeldetails.h \
    ../include/libversiontext.h \
    ../include/marchingcubes_t.h \
    ../include/marchingcubes.h \
    ../include/marrhildrethedgedetector.h \
    ../include/mat3.h \
    ../include/niftiinfo.h \
    ../include/niftiparser.h \
    ../include/operators.h \
    ../include/point3d.h \
    ../include/rgb8.h \
    ../include/siltbyteswap.h \
    ../include/silttypes.h \
    ../include/slicet.h \
    ../include/smoothingkernel.h \
    ../include/strutil.h \
    ../include/subvol.h \
    ../include/tensor3x3.h \
    ../include/triangle.h \
    ../include/triple.h \
    ../include/uvpoint.h \
    ../include/vbit.h \
    ../include/vectorutil.h \
    ../include/vol3d_t.h \
    ../include/vol3d.h \
    ../include/vol3dbase.h \
    ../include/vol3dcompressed.h \
    ../include/vol3dcrop.h \
    ../include/vol3ddatatypes.h \
    ../include/vol3dhistogram.h \
    ../include/vol3dlib.h \
    ../include/vol3dops.h \
    ../include/vol3dquery.h \
    ../include/vol3dreorder.h \
    ../include/vol3dsimple.h \
    ../include/vol3dutils.h \
    ../include/volumedivisor.h \
    ../include/volumeloader.h \
    ../include/volumescaler.h \
    ../include/xmlbuffer.h \
    ../include/zstream.h

HEADERS += \
    ../corticalextraction/include/cortexmodeler.h \
    ../corticalextraction/include/maskscrubber.h \
    ../corticalextraction/include/topofixer.h \
    ../corticalextraction/include/dewisp.h \
    ../corticalextraction/include/dewisptool.h \
    ../corticalextraction/include/surfaceexpander.h \
    ../corticalextraction/include/brainsplitter.h \
    ../corticalextraction/include/BSE/bsetool.h \
    ../corticalextraction/include/BFC/bfcmessages.h \
    ../corticalextraction/include/BFC/bfctool.h \
    ../corticalextraction/include/BFC/biasdensitymodel.h \
    ../corticalextraction/include/BFC/biasfieldcorrector.h \
    ../corticalextraction/include/BFC/biasinfo.h \
    ../corticalextraction/include/BFC/clampcopy.h \
    ../corticalextraction/include/BFC/means3.h \
    ../corticalextraction/include/BFC/regularizedsplinef.h \
    ../corticalextraction/include/BFC/roihistogram.h \
    ../corticalextraction/include/BFC/weighting.h \
    ../corticalextraction/include/PVC/gaussian.h \
    ../corticalextraction/include/PVC/pvctool.h \
    ../corticalextraction/include/PVC/pvmodel.h \
    ../corticalextraction/include/PVC/tissuelabels.h \
    ../corticalextraction/include/TCA/dad.h \
    ../corticalextraction/include/TCA/edgetable.h \
    ../corticalextraction/include/TCA/grapht.h \
    ../corticalextraction/include/TCA/link.h \
    ../corticalextraction/include/TCA/linkbg.h \
    ../corticalextraction/include/TCA/maximalspanningtree.h \
    ../corticalextraction/include/TCA/grapht.h \
    ../corticalextraction/include/TCA/priorityqueue.h \
    ../corticalextraction/include/TCA/seeker.h \
    ../corticalextraction/include/TCA/slicelabel.h \
    ../corticalextraction/include/TCA/sliceops.h \
    ../corticalextraction/include/TCA/slicesegmenter.h \
    ../corticalextraction/include/TCA/slicetemplates.h \
    ../corticalextraction/include/TCA/tcafixbackground.h \
    ../corticalextraction/include/TCA/tcafixchecker.h \
    ../corticalextraction/include/TCA/tcafixforeground.h \
    ../corticalextraction/include/TCA/topologycorrector.h


HEADERS += ../diffusion/include/slice2d.h \
    ../diffusion/include/connectivityproperties.h

RESOURCES += \
    brainsuite.qrc

OTHER_FILES += \
    brainsuiteico.rc

DISTFILES += \
    images/brainsuite_icon_512px.icns
