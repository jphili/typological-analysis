/***************************************************************************
    typologicalanalysis.h
    -------------------
    begin                : Jan 21, 2004
    copyright            : (C) 2004 by Tim Sutton
    email                : tim@linfiniti.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TypologicalAnalysis_H
#define TypologicalAnalysis_H

//QT4 includes
#include <QObject>

//QGIS includes
#include "../qgisplugin.h"
#include "qgsmapcanvas.h"

//forward declarations
class QAction;
class QToolBar;
class QgisInterface;

/**
* \class Plugin
* \brief [name] plugin for QGIS for analysing typological developments
* [description]
*/
class TypologicalAnalysis: public QObject, public QgisPlugin
{
    Q_OBJECT
  public:

    //////////////////////////////////////////////////////////////////////
    //
    //                MANDATORY PLUGIN METHODS FOLLOW
    //
    //////////////////////////////////////////////////////////////////////

    /**
    * Constructor for a plugin. The QgisInterface pointer is passed by
    * QGIS when it attempts to instantiate the plugin.
    * @param theInterface Pointer to the QgisInterface object.
     */
    TypologicalAnalysis( QgisInterface * theInterface );
    //! Destructor
    virtual ~TypologicalAnalysis();

  public slots:
    //! init the gui
    virtual void initGui();
    //! Show the dialog box
    void run();
    //! unload the plugin
    void unload();
    //! show the help document
    void help();

  private:

    ////////////////////////////////////////////////////////////////////
    //
    // MANDATORY PLUGIN PROPERTY DECLARATIONS  .....
    //
    ////////////////////////////////////////////////////////////////////

    int mPluginType;
    //! Pointer to the QGIS interface object
    QgisInterface *mQGisIface;
    //!pointer to the qaction for this plugin
    QAction * mQActionPointer;
    ////////////////////////////////////////////////////////////////////
    //
    // ADD YOUR OWN PROPERTY DECLARATIONS AFTER THIS POINT.....
    //
    ////////////////////////////////////////////////////////////////////

    QgsMapCanvas *mCanvas;


};

#endif //TypologicalAnalysis_H
