/***************************************************************************
  typologicalanalysis.cpp
  The typological analysis plugin
  -------------------
         begin                : 01.02.2015
         copyright            : (C) Julia Philipps and 01.02.2015
         email                : philipj0@smail.uni-koeln.de

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//
// QGIS Specific includes
//

#include <qgisinterface.h>
#include <qgisgui.h>
#include <qgsmessagebar.h>
#include <qgsmapcanvas.h>
#include <qgsmaplayer.h>
#include <qgsdatasourceuri.h>
#include <qgsmaplayerregistry.h>
#include <qgsvectorlayer.h>

#include <stdio.h>

#include "typologicalanalysis.h"
#include "typologicalanalysisgui.h"

//
// Qt4 Related Includes
//

#include <QAction>
#include <QToolBar>
#include <QTextCodec>
#include <QDialogButtonBox>
#include <QRadioButton>

static const QString sName = QObject::tr( "Typological Analysis" );
static const QString sDescription = QObject::tr( "The typological analysis plugin" );
static const QString sCategory = QObject::tr( "Vector" );
static const QString sPluginVersion = QObject::tr( "Version 0.1" );
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;
static const QString sPluginIcon = ":/typologicalanalysis/typologicalanalysis.png";

//////////////////////////////////////////////////////////////////////
//
// THE FOLLOWING METHODS ARE MANDATORY FOR ALL PLUGINS
//
//////////////////////////////////////////////////////////////////////

/**
 * Constructor for the plugin. The plugin is passed a pointer
 * an interface object that provides access to exposed functions in QGIS.
 * @param theQGisInterface - Pointer to the QGIS interface object
 */
TypologicalAnalysis::TypologicalAnalysis( QgisInterface * theQgisInterface ):
    QgisPlugin( sName, sDescription, sCategory, sPluginVersion, sPluginType ),
    mQGisIface( theQgisInterface )
{
    //encoding for message boxes, etc. to support german language
    QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));
}

TypologicalAnalysis::~TypologicalAnalysis()
{
}

/*
 * Initialize the GUI interface for the plugin - this is only called once when the plugin is
 * added to the plugin registry in the QGIS application.
 */
void TypologicalAnalysis::initGui()
{

  // Create the action for tool
  mQActionPointer = new QAction( QIcon( ":/typologicalanalysis/typologicalanalysis.png" ), tr( "Typological Analysis" ), this );
  mQActionPointer->setObjectName( "mQActionPointer" );
  // Set the what's this text
  mQActionPointer->setWhatsThis( tr( "Das Plugin führt Typologische Analysen durch." ) );
  // Connect the action to the run
  connect( mQActionPointer, SIGNAL( triggered() ), this, SLOT( run() ) );
  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon( mQActionPointer );
  mQGisIface->addPluginToMenu( tr( "&Typological Analysis" ), mQActionPointer );

}

//method defined in interface
void TypologicalAnalysis::help()
{

}

// Slot called when the menu item is triggered
// If you created more menu items / toolbar buttons in initiGui, you should
// create a separate handler for each action - this single run() method will
// not be enough
void TypologicalAnalysis::run()
{
    TypologicalAnalysisGui *gui = new TypologicalAnalysisGui( mQGisIface->mapCanvas(), mQGisIface->mainWindow(), QgisGui::ModalDialogFlags );
    gui->setAttribute( Qt::WA_DeleteOnClose );

    mCanvas = mQGisIface->mapCanvas();

    if (mCanvas->layerCount() > 0 && mCanvas->currentLayer() != NULL) {
        QgsMapLayer *vLayer = mCanvas->currentLayer();

        if (!vLayer->isValid() || !vLayer->name().operator ==("Fundobjekte")) {
            mQGisIface->messageBar()->pushMessage( tr( "Layer Fundobjekte wurde nicht gefunden" ), tr( "Das Plugin benötigt das Fundobjekte Layer" ), QgsMessageBar::INFO, mQGisIface->messageTimeout() );
        }
        else {
            gui->show();
        }
    }
    else {
        mQGisIface->messageBar()->pushMessage( tr( "Keine gültigen Layer" ), tr( "Das Plugin benötigt das Fundobjekte Layer" ), QgsMessageBar::INFO, mQGisIface->messageTimeout() );
    }

}


// Unload the plugin by cleaning up the GUI
void TypologicalAnalysis::unload()
{
  // remove the GUI
  mQGisIface->removePluginMenu( "&Typological Analysis", mQActionPointer );
  mQGisIface->removeToolBarIcon( mQActionPointer );
  delete mQActionPointer;
}





//////////////////////////////////////////////////////////////////////////
//
//
//  THE FOLLOWING CODE IS AUTOGENERATED BY THE PLUGIN BUILDER SCRIPT
//    YOU WOULD NORMALLY NOT NEED TO MODIFY THIS, AND YOUR PLUGIN
//      MAY NOT WORK PROPERLY IF YOU MODIFY THIS INCORRECTLY
//
//
//////////////////////////////////////////////////////////////////////////


/**
 * Required extern functions needed  for every plugin
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory( QgisInterface * theQgisInterfacePointer )
{
  return new TypologicalAnalysis( theQgisInterfacePointer );
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the category
QGISEXTERN QString category()
{
  return sCategory;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

QGISEXTERN QString icon()
{
  return sPluginIcon;
}

// Delete ourself
QGISEXTERN void unload( QgisPlugin * thePluginPointer )
{
  delete thePluginPointer;
}
