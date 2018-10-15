/***************************************************************************
 *   Copyright (C) 2003 by Tim Sutton                                      *
 *   tim@linfiniti.com                                                     *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#ifndef TypologicalAnalysisGUI_H
#define TypologicalAnalysisGUI_H

#include <ui_typologicalanalysisguibase.h>
#include <qgsmaptoolselect.h>
#include <qgsmaptool.h>
#include <qgsmapcanvas.h>

#include "typologicalanalysis.h"
#include "qgsmaptoolidentifyfeature.h"
#include "qgsfeature.h"
#include "qgsfeatureiterator.h"
#include "qgsmaptip.h"
#include "qgspythonrunner.h"
#include "qgspythonutils.h"
#include "qgsgeometry.h"

#include <QMap>
#include <QString>
#include <QVector>
#include <QDialog>

class QPushButton;
class QgsRubberBand;
class QgsMapTip;
class QGraphicsItem;


//map tool for selecting features
class QgsSelectFeatures : public QgsMapToolSelect {
    Q_OBJECT;


public:
  QgsSelectFeatures (QgsMapCanvas *canvas )
    : QgsMapToolSelect ( canvas )
  {
  }

  void canvasReleaseEvent( QMouseEvent *e ) override
  {
    QgsMapToolSelect::canvasReleaseEvent( e );
    emit mouseReleased();
  }

signals:
   void mouseReleased();
};



class TypologicalAnalysisGui : public QDialog, private Ui::TypologicalAnalysisGuiBase
{
    Q_OBJECT
  public:
    TypologicalAnalysisGui( QgsMapCanvas *canvas, QWidget* parent = 0, Qt::WindowFlags fl = 0 );
    ~TypologicalAnalysisGui();

    /** map for attribute names */
    QMap<int, QString> attrName;

    QMap<QString, QMap<QString, QString>>contextMap;

    /** struct for result feature vector */
    struct Feature {
        bool categoryScore;
        int similarityScore;                                                                           
        QgsFeature feature;
    };

    /** vector for successfully compared features with a score */
    QVector<Feature> featureVector;


  private:
    static const int context_id = 0;

    QgsMapCanvas* mCanvasGui;
    QgsSelectFeatures* mSelectFeatureTool;
    QgsVectorLayer* vLayer;

    QString mCategory;
    QString mContext;
    QgsFeature mSelectedFeature;
    QgsFeatureList mFeaturesWithinRadius;


    /** select features within the specified radius */
    void selectFeaturesWithinRadius(double radius);

    /** compare feature within radius with selected feature */
    void compareFeature(QgsFeature feature);

    /** create the result layer for similar objects */
    void createObjectResultLayer();

    /** trim question marks */
    QString trim(QString);

    /** function for creating text annotation for chosen object */
    void createTextAnnotation();

    /** search for roman places nearby */
    void searchForPlaces(QgsFeature feature);

    /** create places result layer  */
    void createPlacesResultLayer();

    /** copy attributes and fields to new layer */
    void copyAttributesAndFields(QgsVectorLayer *newLayer);

    /** save layer to hard disk */
    QgsVectorLayer* saveLayerToHardDisk(QgsVectorLayer *newLayer, QString layername);

    /** render categories by specified field */
    void renderCategories(QgsVectorLayer *layer, QString field, int colorIndex);

    /** get dating of spathae and gladii*/
    void getDating(QString swordCategory);

    /** render dating*/
    QgsVectorLayer* renderDating(QgsVectorLayer *layer, QString field, int colorIndex);

    /** fill filters in GUI */
    void fillFilter();

    /** set selected filters */
    void setFilters();

    /** filter selected layer */
    void filterByLayer(QgsVectorLayer *layer);

    /** transform geometry */
    QgsGeometry* getGeometry(QgsGeometry* geo, QgsVectorLayer* layer);

    /** filter */
    void filter(QgsFeatureIterator fIter);


  private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
    void setFeatureObjectTool();
    void setFeatureCityTool();
    void processFeature();
    void gladiusTypology();
    void spathaTypology();

};

#endif
