/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/
//QGis includes
#include "typologicalanalysisgui.h"
#include "typologicalanalysis.h"
#include "qgscontexthelp.h"
#include "qgisinterface.h"
#include "qgis.h"
#include "qgsmapcanvas.h"
#include "qgsvectorlayer.h"
#include "qgsgeometry.h"
#include "qgspoint.h"
#include "qgsmaplayerregistry.h"
#include "qgsvectordataprovider.h"
#include "qgsvectorfilewriter.h"
#include "qgsstylev2.h"
#include "qgscategorizedsymbolrendererv2.h"
#include "qgsgraduatedsymbolrendererv2.h"
#include "qgsvectorcolorrampv2.h"
#include "qgstextannotationitem.h"
#include "qgsmaplayerlegend.h"
#include "qgsrasterlayer.h"
#include "qgsmaplayercombobox.h"

//qt includes
#include <QMessageBox>
#include <QMouseEvent>
#include <QColor>
#include <Qt>
#include <QtCore/qobject.h>     //for qobject_cast
#include <QSpinBox>
#include <QTextBrowser>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QVector>
#include <QVectorIterator>

//other includes
#include <string>
#include <stdio.h>
#include <vector>
#include <algorithm>



TypologicalAnalysisGui::TypologicalAnalysisGui( QgsMapCanvas *canvas, QWidget* parent, Qt::WindowFlags fl )
    : QDialog( parent, fl )
    , mCanvasGui( canvas )
{

    setupUi( this );
    mSelectFeatureTool = new QgsSelectFeatures(canvas);                                         //create feature select tool
    vLayer = qobject_cast<QgsVectorLayer *>(mCanvasGui->currentLayer());                        //get current active layer

    connect(select_object,SIGNAL(clicked()),this,SLOT(setFeatureObjectTool()));
    connect(select_object_city,SIGNAL(clicked()), this, SLOT(setFeatureCityTool()));
    connect(mSelectFeatureTool, SIGNAL( mouseReleased() ), this, SLOT(processFeature()));
    connect(gladii_typ, SIGNAL(clicked()), this, SLOT(gladiusTypology()));
    connect(spathae_typ, SIGNAL(clicked()), this, SLOT(spathaTypology()));

    present_object->hide();
    similarartifactsradio_button->setChecked(true);
    select_object_city->setDisabled(true);


    //create attribute map for names (not latitude, longitude), only chosen attributes
    attrName[0]="Fundort"; attrName[1]="Fundumstände"; attrName[2]="Klingenart";
    attrName[3]="Klingen-Konkordanz"; attrName[4]="Erhaltung der Klinge in mm";
    attrName[5]="Klingenzustand"; attrName[6]="Klingenquerschnitt";
    attrName[7]="Besonderheiten der Klinge"; attrName[8]="Schwertgriff";
    attrName[9]="Schwertscheide"; attrName[10]="Datierung"; attrName[11]="Inventar";
    attrName[12]="datzahl";

    fillFilter();

    //create maps for Umkreissuche, summarised in categories (military, people, nature, construction)
    QMap<QString, QString> military;
    QMap<QString, QString> people;
    QMap<QString, QString> nature;
    QMap<QString, QString> construction;

    military.insert("fort", "Kastell"); military.insert("station", "Standort");
    military.insert("wall", "Mauer"); military.insert("unknown", "Unbekannt");

    people.insert("estate", "Landgut"); people.insert("settlement", "Siedlung");
    people.insert("people", "Bevölkerung"); people.insert("unknown", "Unbekannt");
    people.insert("cemetary", "Friedhof"); people.insert("villa", "Villa");

    nature.insert("river", "Fluss"); nature.insert("forest", "Wald");
    nature.insert("region", "Gebiet"); nature.insert("mountain", "Berg");
    nature.insert("unknown", "Unbekannt");

    construction.insert("bridge", "Brücke");  construction.insert("aqueduct", "Aquädukt");
    construction.insert("mine", "Mine"); construction.insert("bath", "Bad");
    construction.insert("quarry", "Steinbruch");  construction.insert("temple", "Tempel");
    construction.insert("unknown", "Unbekannt");

    contextMap.insert("military", military);
    contextMap.insert("people", people);
    contextMap.insert("nature", nature);
    contextMap.insert("construction", construction);


}


TypologicalAnalysisGui::~TypologicalAnalysisGui() {

}


void TypologicalAnalysisGui::processFeature() {

    //get selected feature to set radius center point
    QgsFeatureList featureList = vLayer->selectedFeatures();

    if (featureList.empty()) {
        QMessageBox::warning(0, tr("Achtung!"), tr("Es wurde kein gültiges Objekt ausgewählt. Abbruch. Bitte neues Objekt auswählen"));
        mCanvasGui->setMapTool( mSelectFeatureTool );                                           //set feature select map tool on canvas
        return;
    }

    mSelectedFeature = featureList.first();                                                     //get selected feature
    QgsAttributes attr = mSelectedFeature.attributes();

    if (mCategory == "object") {
        double radius = radius_object->text().toDouble()*1000.0;

        //check if chosen category is empty
        if ((objectCat->currentText( )== "Klingenart" ||
             objectCat->currentText() == "Klingentyp" ||
             objectCat->currentText() == "Klingenart+Klingentyp") &&
                mSelectedFeature.attributes().at(2).toString().isEmpty()) {
            QMessageBox::information(0, "Hinweis!", "Das von Ihnen gewählte Objekt weist keine Daten in der von Ihnen gewählten Vergleichskategorie auf!");
            return;
        }
        if (objectCat->currentText() == "Schwertgriff" &&
                mSelectedFeature.attributes().at(8).toString().isEmpty()) {
            QMessageBox::information(0, "Hinweis!", "Das von Ihnen gewählte Objekt weist keine Daten in der von Ihnen gewählten Vergleichskategorie auf!");
            return;
        }
        if (objectCat->currentText() == "Schwertscheide" &&
                mSelectedFeature.attributes().at(9).toString().isEmpty()) {
            QMessageBox::information(0, "Hinweis!", "Das von Ihnen gewählte Objekt weist keine Daten in der von Ihnen gewählten Vergleichskategorie auf!");
            return;
        }

        selectFeaturesWithinRadius(radius);                                                     //select all features in the specified radius to continue processing
        foreach(QgsFeature f, mFeaturesWithinRadius) {
             compareFeature(f);                                                                 //compare feature with selected feature
        }
    }

    if (mCategory == "places") {
        double radius = radius_city->text().toDouble()*1000.0;
        QList<QgsMapLayer*> layerList = QgsMapLayerRegistry::instance()->mapLayersByName("Römische Plätze");
        vLayer = qobject_cast<QgsVectorLayer *>(layerList.first());                         //change active layer to Römische Plätze
        selectFeaturesWithinRadius(radius);

        if (mFeaturesWithinRadius.isEmpty()) {
            QMessageBox::warning(0, "Fehler!", "Es wurden keine römischen Plätze mit Ihren Kriterien in der Nähe des Objekts gefunden!");
            return;
        }

        foreach(QgsFeature f, mFeaturesWithinRadius) {
             searchForPlaces(f);                                                                //compare feature with selected feature
        }
    }

    //set presenting text for selected object
    QString text;
    for (int j = 0; j < attrName.count(); j++) {
        if (attr[j].operator ==("")) continue;
        QString name = attrName[j];
        QString value = attr[j].toString();
        if (j == attrName.count()-1) {
            text += name + ": " + value;
        }
        else {
            text += name + ": " + value + ", \n";
        }
    }

    present_object->show();                                                                     //show text browser
    present_object->setText(text);                                                              //set presenting text
    show();                                                                                     //show plugin widget
}


void TypologicalAnalysisGui::selectFeaturesWithinRadius(double radius) {

    //get geometry of selected feature and transform to the right coordinate system
    QgsGeometry *fGeom = mSelectedFeature.geometry();

    if ( mCanvasGui->mapSettings().hasCrsTransformEnabled() ) {
        try
        {
          QgsCoordinateTransform ct( vLayer->crs(), mCanvasGui->mapSettings().destinationCrs());
          fGeom->transform( ct );
        }
        catch ( QgsCsException &cse )
        {
          Q_UNUSED( cse );
          // catch exception for 'invalid' point and leave existing selection unchanged
          QgsLogger::warning( "Caught CRS exception " + QString( __FILE__ ) + ": " + QString::number( __LINE__ ) );
          return;
        }
     }

    //create buffer with radius
    QgsGeometry *bufferGeo = fGeom->buffer(radius*0.85, 160);
    QgsGeometry selectGeomTrans( *bufferGeo );

    //get features within the specified radius
    QgsFeatureIterator iter = vLayer->getFeatures(QgsFeatureRequest().setFilterRect(selectGeomTrans.boundingBox()).setFlags(QgsFeatureRequest::ExactIntersect));
    QgsFeature f;

    while(iter.nextFeature(f)) {
         mFeaturesWithinRadius.append(f);
    }
    delete bufferGeo;
}


void TypologicalAnalysisGui::setFeatureObjectTool() {
    hide();                                                                                         //hide interface to select point
    mCategory = "object";
    mCanvasGui->setMapTool( mSelectFeatureTool );                                                   //set feature select map tool on canvas
}


void TypologicalAnalysisGui::setFeatureCityTool() {
    hide();
    mCategory = "places";
    mCanvasGui->setMapTool( mSelectFeatureTool );
}


void TypologicalAnalysisGui::on_buttonBox_accepted() {
    if (mCategory == "object") {
        createObjectResultLayer();
    }
    if (mCategory == "places") {
        createPlacesResultLayer();
    }
    if (!was->text().isEmpty() && mCategory != "") {
        setFilters();
    }
    else {
        QMessageBox::warning(0, "Fehler!", "Bitte geben Sie einen Suchbegriff ein!");
        return;
    }
    delete mSelectFeatureTool;
    accept();
}


void TypologicalAnalysisGui::on_buttonBox_rejected() {
    if (vLayer->selectedFeatureCount() > 0) {
        vLayer->deselect(vLayer->selectedFeaturesIds());
         delete mSelectFeatureTool;
    }
    reject();
}


QString TypologicalAnalysisGui::trim(QString strToTrim) {
    if (strToTrim.contains("?")) {
        for (int i=0; i < strToTrim.count("?"); i++) {
            int pos = strToTrim.indexOf("?");
            strToTrim.remove(pos,1);
        }
    }
    return strToTrim;
}


void TypologicalAnalysisGui::copyAttributesAndFields(QgsVectorLayer *newLayer) {

    QgsVectorDataProvider *pr = newLayer->dataProvider();                                       //create data provider
    QgsFeatureList fList;
    QList<QgsField> fieldList;

    newLayer->startEditing();                                                                   //start editing
    if (mCategory == "object") {
        for (int j=0; j<mSelectedFeature.fields()->count();j++) {                               //get fields of the feature and append to list
            fieldList.append(mSelectedFeature.fields()->field(j));
        }
        QgsField similarityField("aehnlich", QVariant::Int);                                    //create new field for similarity score if Ähnliche Objekte
        fieldList.append(similarityField);                                                      //add this field to the list
    }

    if (mCategory == "places" || mCategory == "") {
        for (int j=0; j<mFeaturesWithinRadius.at(0).fields()->count();j++) {                    //get fields of the feature and append to list
            fieldList.append(mFeaturesWithinRadius.at(0).fields()->field(j));
        }
    }

    pr->addAttributes(fieldList);                                                               //add fields with the data provider

    //fill attribute fields with values of the corresponding features of the feature struct vector
    for (int i = 0; i < featureVector.count(); i++) {
        if (mSelectedFeature.isValid()) {
            if (featureVector.at(i).feature.id() == mSelectedFeature.id()) continue;            //exclude the feature selected by the user
        }

        QgsFeature feat;
        QgsFeature f = featureVector.at(i).feature;
        QgsAttributes attrs = featureVector.at(i).feature.attributes();

        QgsGeometry *geo = new QgsGeometry();
        geo = f.geometry();

        if (mCategory == "object") {
            int count = featureVector.at(i).similarityScore;                                    //add similarity score
            attrs.append(count);
        }
        QgsGeometry geom( *geo );
        feat.setGeometry(geom);

        feat.setAttributes(attrs);
        fList.append(feat);
    }
    pr->addFeatures(fList);                                                                     //add all features with the data provider
    newLayer->commitChanges();                                                                  //commit changes


}



QgsVectorLayer* TypologicalAnalysisGui::saveLayerToHardDisk(QgsVectorLayer *newLayer, QString layername) {
    QString filename;

    if (mCategory == "object" || mCategory == "places") {
        //create filename - consists of Fundort and unique feature ID for writing the vector layer to hard disk
        filename = QString(mSelectedFeature.attribute(0).toString() + "_" + QString::number(int(mSelectedFeature.id())));
    }
    else {
        filename = layername.replace(" ", "");
    }

    //write memory layer to hard disk
    QgsCoordinateReferenceSystem *crs = new QgsCoordinateReferenceSystem(newLayer->crs());
    QgsVectorFileWriter::WriterError error = QgsVectorFileWriter::writeAsVectorFormat(newLayer, QString(filename + ".shp"),
                                                                                      "utf-8", crs, QString("ESRI Shapefile"));
    if (error != QgsVectorFileWriter::NoError) {
        QMessageBox::warning(0, tr("Fehler!"), tr("Fehler beim Schreiben des Shapefiles!"));
    }
    delete crs;

    //load the saved layer to the map canvas
    if (mCategory == "object" || mCategory == "places") {
        QgsVectorLayer *layer = new QgsVectorLayer(QString(filename + ".shp"),
                                                   QString(layername + filename), "ogr");
        mCanvasGui->layers().append(layer);                                                     //add layer to layer set
        QgsMapLayerRegistry::instance()->addMapLayer(layer);                                    //add layer to map layer registry

        return layer;                                                                           //return the layer
    }
    else {
        QgsVectorLayer *layer = new QgsVectorLayer(QString(filename + ".shp"), QString(layername), "ogr");
        mCanvasGui->layers().append(layer);                                                     //add layer to layer set
        QgsMapLayerRegistry::instance()->addMapLayer(layer);                                    //add layer to map layer registry

        return layer;                                                                           //return the layer
    }
}



void TypologicalAnalysisGui::renderCategories(QgsVectorLayer *layer, QString field, int colorIndex) {

    QgsStyleV2 *style = QgsStyleV2::defaultStyle();                                             //set style to default
    QStringList defaultColorRampNames = style->colorRampNames();                                //get default color ramp names
    QgsSymbolV2 *symbol = QgsSymbolV2::defaultSymbol(layer->geometryType());                    //create default symbol with help of layer geometry (Point)
    QgsVectorColorRampV2 *ramp = style->colorRamp(defaultColorRampNames.at(colorIndex));        //create color ramp
    int index = layer->fieldNameIndex(field);                                                   //get index of similarity score field 'aehnlich'
    QList<QVariant> uniqueValueList;                                                            //list for unique values of similarity score field
    layer->uniqueValues(index, uniqueValueList);


    //create categories from field with the similarity score
    QgsCategoryList catList;

    for (int i = 0; i < uniqueValueList.count(); i++) {                                         //set color for each unique value inside the list
        QgsSymbolV2 *sym = symbol->clone();
        QVariant value = uniqueValueList[i];
        double d = i/(double)uniqueValueList.count();
        sym->setColor(ramp->color(d));
        catList.append(QgsRendererCategoryV2(value, sym, value.toString()));                    //append to category list
    }

    //render categories
    QgsCategorizedSymbolRendererV2 *renderer = new QgsCategorizedSymbolRendererV2(field, catList);
    renderer->sortByValue();
    layer->setRendererV2(renderer);
    layer->triggerRepaint();
}



QgsVectorLayer* TypologicalAnalysisGui::renderDating(QgsVectorLayer *layer, QString field, int colorIndex) {

    QgsStyleV2 *style = QgsStyleV2::defaultStyle();                                             //set style to default
    QStringList defaultColorRampNames = style->colorRampNames();                                //get default color ramp names
    QgsMarkerSymbolV2 *symbol = new QgsMarkerSymbolV2();                                        //create new marker symbol
    QgsVectorColorRampV2 *ramp = style->colorRamp(defaultColorRampNames.at(colorIndex));        //create color ramp
    symbol->setSize(3.400);

    //render dates with gradient
    QgsGraduatedSymbolRendererV2 *renderer = new QgsGraduatedSymbolRendererV2();
    renderer = renderer->createRenderer(layer, field, 6, QgsGraduatedSymbolRendererV2::Pretty, symbol, ramp);
    layer->setRendererV2(renderer);
    layer->triggerRepaint();
    delete symbol;
    return layer;
}



//create Ähnliche Objekte Layer with categorised symbols and map tips
void TypologicalAnalysisGui::createObjectResultLayer() {

    //create new layer in memory to work on
    QgsVectorLayer *newLayer = new QgsVectorLayer("Point", "Ähnliche Objekte", "memory");

    if (!newLayer->isValid()) {
        QgsDebugMsg("Layer ist nicht valide.");
        return;
    }

    QString layername = "Ähnliche Objekte ";
    copyAttributesAndFields(newLayer);
    createTextAnnotation();

    QgsVectorLayer *layer = saveLayerToHardDisk(newLayer, layername);
    if (!layer->isValid()) {
        QMessageBox::warning(0, "Fehler!", "Layer ist nicht valide!");
        return;
    }

    //create categorised renderer to categorise the similarity score
    renderCategories(layer, "aehnlich", 0);

    //create map tips for mouse hover
    QgsFields fields = layer->pendingFields();
    attrName[13]="Ähnlichkeitswert (ca.)";                                                        //add similarity score
    QString displayExpression;

    for(int j = 0; j < attrName.count(); j++) {
        QString name = attrName[j];
        QString value = '"'+ fields.at(j).name() + '"';
        displayExpression += "[% CASE WHEN " + value + " IS NOT NULL THEN '<b>" +
                name + "</b>: ' || " + value + "|| '<br />'END%]" + '\n';
    }

    displayExpression.toUtf8();
    layer->setDisplayField(displayExpression);
    layer->triggerRepaint();
    delete newLayer;
}



void TypologicalAnalysisGui::createPlacesResultLayer() {

    //create new layer in memory to work on
    QgsVectorLayer *newLayer = new QgsVectorLayer("Point", "Umkreis", "memory");

    if (!newLayer->isValid()) {
        QgsDebugMsg("Layer ist nicht valide.");
        return;
    }

    QString layername = "Umkreis";

    //create the attribute table in the new layer and fill attribute fields
    copyAttributesAndFields(newLayer);

    //create text annotation item for the chosen object
    createTextAnnotation();

    QgsVectorLayer *layer = saveLayerToHardDisk(newLayer, layername);
    if (!layer->isValid()) {
        QMessageBox::warning(0, "Fehler!", "Layer ist nicht valide!");
        return;
    }

    //create categorised renderer to categorise the similarity score
    renderCategories(layer, "featureTyp", 20);

    //create map tips for mouse hover
    QString displayExpression = "<b>Kontext</b>: " + placeCat->currentText() + "<br />" + '\n';
    displayExpression += "<b>[%featureTyp%]</b> <br />[%\"title\"%] <br />";
    displayExpression.toUtf8();
    layer->setDisplayField(displayExpression);
    layer->triggerRepaint();
    delete newLayer;
}



void TypologicalAnalysisGui::createTextAnnotation() {

    QString chosenFeatureAnnotation;

    for(int j = 0; j < attrName.count(); j++) {
        if (mSelectedFeature.attributes().at(j).toString().isEmpty()) continue;
        QString name = attrName[j];
        QString value = mSelectedFeature.attributes().at(j).toString();
        chosenFeatureAnnotation += "<b>" + name + "</b>" + ": " + value + "<br />";
    }

    chosenFeatureAnnotation.toUtf8();
    QTextDocument *document = new QTextDocument();
    document->setHtml(chosenFeatureAnnotation);
    QgsTextAnnotationItem *textItem = new QgsTextAnnotationItem(mCanvasGui);
    QgsPoint point;
    point.set(mSelectedFeature.geometry()->asPoint().x(), mSelectedFeature.geometry()->asPoint().y());
    textItem->setMapPosition(point);
    textItem->setFrameSize(QSize(300,200));
    textItem->setDocument(document);
    mCanvasGui->scene()->addItem(textItem);                                                     //add annotation to feature on screen
    delete document;
}



void TypologicalAnalysisGui::searchForPlaces(QgsFeature feature) {

    int index = feature.fieldNameIndex("featureTyp");                                           //get index of the feature type

    Feature place;
    place.feature = feature;

    if (placeCat->currentText() == "Militär") {
        QMap<QString, QString> military = contextMap.value("military");                         //get military (context) key value pair map

        QgsAttributes attr = feature.attributes();

            if (military.contains(attr.at(index).toString())) {                                 //if map contains feature type of current feature
                place.feature.setAttribute(index, military.value(attr.at(index).toString()));   //replace with value of map - german translation
                featureVector.append(place);                                                    //append feature to vector
                mContext = "military";                                                          //set context
            }
        }


    if (placeCat->currentText() == "Siedlungen und Bevölkerung") {
        QMap<QString, QString> people = contextMap.value("people");

        QgsAttributes attr = feature.attributes();

            if (people.contains(attr.at(index).toString())) {
                place.feature.setAttribute(index, people.value(attr.at(index).toString()));
                featureVector.append(place);
                mContext = "people";
            }
        }


    if (placeCat->currentText() == "natürliche Umgebung") {
        QMap<QString, QString> nature = contextMap.value("nature");

        QgsAttributes attr = feature.attributes();

            if (nature.contains(attr.at(index).toString())) {
                place.feature.setAttribute(index, nature.value(attr.at(index).toString()));
                featureVector.append(place);
                mContext = "nature";
            }
        }


    if (placeCat->currentText() == "Bauten") {
        QMap<QString, QString> construction = contextMap.value("construction");

        QgsAttributes attr = feature.attributes();

            if (construction.contains(attr.at(index).toString())) {
                place.feature.setAttribute(index, construction.value(attr.at(index).toString()));
                featureVector.append(place);
                mContext = "construction";
            }
        }

}



void TypologicalAnalysisGui::compareFeature(QgsFeature feature) {

    //get categories
    QgsAttributes attr = mSelectedFeature.attributes();
    QStringList help;

    if (attr[2].toString().contains(",")) {
        help = attr[2].toString().split(", ");
    }
    else {                                                                                      //append one value 2 times, because it could be Klingenart or Klingentyp in the database
        help.append(attr[2].toString());
        help.append(attr[2].toString());
    }

    QString kArt = trim(help[0]);
    QString kTyp = trim(help[1]);

    struct Feature compareFeature;
    compareFeature.feature = feature;
	compareFeature.categoryScore = false;
	compareFeature.similarityScore = 0;
    QString category = objectCat->currentText();
    QgsAttributes attributes = feature.attributes();

    if (category == "Klingenart") {
        if (attributes[2].toString().contains(kArt)) {
            compareFeature.categoryScore = true;                                                //defined user category was found!
            compareFeature.similarityScore = compareFeature.similarityScore+1;

            for (int i=1; i < attr.count(); i++) {                                              //start at Fundumstände
                if (attr[i].toString() == "" || i==2) continue;                                 //skip already checked fields

                //only one word
                if (!attr[i].toString().contains(" ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {                //if a match was found
                        compareFeature.similarityScore = compareFeature.similarityScore+1;      //increase similarity score
                    }
                }

                //only one value
                if (attr[i].toString().contains(" ") && !attr[i].toString().contains(", ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }

                //more than one value
                if (attr[i].toString().contains(", ")) {
                    QStringList valueList = attr[i].toString().split(", ");                     //split in tokens; 1 token = 1 value
                    foreach (QString value, valueList) {
                        if (attributes[i].toString().contains(value)) {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }
            }
            featureVector.push_back(compareFeature);                                            //push feature in the feature vector
        }

    }

    if (category == "Klingentyp") {
        if (attributes[2].toString().contains(kTyp)) {
            compareFeature.categoryScore = true;
            compareFeature.similarityScore = compareFeature.similarityScore+1;

            for (int i=1; i < attr.count(); i++) {
                if (attr[i].toString() == "" || i==2) continue;

                //only one word
                if (!attr[i].toString().contains(" ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }

                //only one value
                if (attr[i].toString().contains(" ") && !attr[i].toString().contains(", ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }

                //more than one value
                if (attr[i].toString().contains(", ")) {
                    QStringList valueList = attr[i].toString().split(", ");
                    foreach (QString value, valueList) {
                        if (attributes[i].toString().contains(value)) {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }
            }
            featureVector.push_back(compareFeature);
        }
    }

    if (category == "Klingenart+Klingentyp") {
        if (trim(attributes[2].toString()) == trim(attr[2].toString())) {
            compareFeature.categoryScore = true;
            compareFeature.similarityScore = compareFeature.similarityScore+1;

            for (int i=1; i < attr.count(); i++) {
                if (attr[i].toString() == "" || i==2) continue;

                //only one word
                if (!attr[i].toString().contains(" ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }

                //only one value
                if (attr[i].toString().contains(" ") && !attr[i].toString().contains(", ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }

                //more than one value
                if (attr[i].toString().contains(", ")) {
                    QStringList valueList = attr[i].toString().split(", ");
                    foreach (QString value, valueList) {
                        if (attributes[i].toString().contains(value)) {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }
            }
            featureVector.push_back(compareFeature);
        }
    }

    if (category == "Schwertgriff") {

            for (int i=1; i < attr.count(); i++) {
                if (attr[i].toString() == "") continue;

                //only one word
                if (!attr[i].toString().contains(" ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        if (i==8) {
                            compareFeature.categoryScore = true;
                        }
                        else {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }

                //only one value
                if (attr[i].toString().contains(" ") && !attr[i].toString().contains(", ")) {
                    if (attributes[i].toString().contains(attr[i].toString())) {
                        if (i==8) {
                            compareFeature.categoryScore = true;
                        }
                        else {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }

                //more than one value
                if (attr[i].toString().contains(", ")) {
                    QStringList valueList = attr[i].toString().split(", ");
                    foreach (QString value, valueList) {
                        if (attributes[i].toString().contains(value)) {
                            if (i==8) {
                                compareFeature.categoryScore = true;
                            }
                            else {
                                compareFeature.similarityScore = compareFeature.similarityScore+1;
                            }
                        }
                    }
                }
            }
            if (compareFeature.categoryScore == true) {
                featureVector.push_back(compareFeature);
            }
    }

    if (category == "Schwertscheide") {
        for (int i=1; i < attr.count(); i++) {
            if (attr[i].toString() == "" ) continue;

            //only one word
            if (!attr[i].toString().contains(" ")) {
                if (attributes[i].toString().contains(attr[i].toString())) {
                    if (i==9) {
                        compareFeature.categoryScore = true;                                    //if chosen category has a match
                    }
                    else {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }
            }

            //only one value
            if (attr[i].toString().contains(" ") && !attr[i].toString().contains(", ")) {
                if (attributes[i].toString().contains(attr[i].toString())) {
                    if (i==9) {
                        compareFeature.categoryScore = true;
                    }
                    else {
                        compareFeature.similarityScore = compareFeature.similarityScore+1;
                    }
                }
            }

            //more than one value
            if (attr[i].toString().contains(", ")) {
                QStringList valueList = attr[i].toString().split(", ");
                foreach (QString value, valueList) {
                    if (attributes[i].toString().contains(value)) {
                        if (i==9) {
                            compareFeature.categoryScore = true;
                        }
                        else {
                            compareFeature.similarityScore = compareFeature.similarityScore+1;
                        }
                    }
                }
            }
        }
        if (compareFeature.categoryScore == true) {                                             //if chosen category has a match, append to feature vector
            featureVector.push_back(compareFeature);
        }
    }
}


void TypologicalAnalysisGui::getDating(QString swordCategory) {

    QgsFeatureList fList;
    QList<QgsField> fieldList;
    QgsFeatureList gladii;
    QgsFeatureList spathae;

    QgsFeatureIterator fIter = vLayer->getFeatures();
    QgsFeature f;

    //sort gladii and spathae and append to list
    while(fIter.nextFeature(f)) {
        if (f.attribute(2).toString().contains("Gladius")) {
            gladii.append(f);
        }
        if (f.attribute(2).toString().contains("Spatha")) {
            spathae.append(f);
        }
    }


    if (swordCategory == "gladius") {
        QgsVectorLayer *newLayer = new QgsVectorLayer("Point", "GladiusTypologie", "memory");
        QgsVectorDataProvider *pr = newLayer->dataProvider();                                   //create data provider

        newLayer->startEditing();

        for (int j=0; j<f.fields()->count();j++) {                                              //get fields of the feature and append to list
            fieldList.append(f.fields()->field(j));
        }
        QgsField typologyStart("datStart", QVariant::Int);                                      //create new field for typology start date
        fieldList.append(typologyStart);                                                        //add this field to the list
        pr->addAttributes(fieldList);                                                           //add fields with the data provider

        foreach(f, gladii) {
            QgsFeature feat;
            QString date = f.attribute("datzahl").toString();
            QgsAttributes attrs = f.attributes();

            QgsGeometry *geo = f.geometry();
            QgsGeometry geom( *geo );
            feat.setGeometry(geom);                                                             //set the right geometry

            if (date.isEmpty()) continue;
            if (date.startsWith("-")) {                                                         //if dating is before christ
                attrs.append("0");                                                              //append 0 to attribute list
            }
            else {
                if (date.contains("-")) {                                                       //else if dating contains - but is not starting before christ
                    date = date.replace(date.indexOf("-"), date.length(), "");                  //use only the first value - replace rest of the string with ""
                    attrs.append(date.toInt());
                }
                else {                                                                          //else if there is only one date value
                    attrs.append(date.toInt());
                }
            }

            feat.setAttributes(attrs);
            fList.append(feat);
        }

        pr->addFeatures(fList);                                                                 //add all features with the data provider
        newLayer->commitChanges();


        QString layername = "Gladius Typologie";
        QgsVectorLayer *layer = saveLayerToHardDisk(newLayer, layername);
        if (!layer->isValid()) {
            QMessageBox::warning(0, "Fehler!", "Layer ist nicht valide!");
            return;
        }

        //add labels to layer - properties
        layer->setCustomProperty("labeling", "pal");
        layer->setCustomProperty("labeling/enabled", "true");
        layer->setCustomProperty("labeling/fontSize", "8");
        layer->setCustomProperty("labeling/fontStyle", "Normal");
        layer->setCustomProperty("labeling/fieldName", "klinge_ ar");

        layer = renderDating(layer, "datStart", 14);
        layer->triggerRepaint();
        delete newLayer;
    }

    if (swordCategory == "spatha") {
        QgsVectorLayer *newLayer = new QgsVectorLayer("Point", "SpathaTypologie", "memory");
        QgsVectorDataProvider *pr = newLayer->dataProvider();                                   //create data provider

        newLayer->startEditing();

        for (int j=0; j<f.fields()->count();j++) {                                              //get fields of the feature and append to list
            fieldList.append(f.fields()->field(j));
        }
        QgsField typologyStart("datStart", QVariant::Int);                                      //create new field for typology start date
        fieldList.append(typologyStart);                                                        //add this field to the list
        pr->addAttributes(fieldList);                                                           //add fields with the data provider



        foreach(f, spathae) {
            QString date = f.attribute("datzahl").toString();

            QgsFeature feat;
            QgsAttributes attrs = f.attributes();

            QgsGeometry *geo = f.geometry();
            QgsGeometry geom( *geo );
            feat.setGeometry(geom);

            if (date.isEmpty()) continue;

            if (date.startsWith("-")) {
                if (date.indexOf("-") != 0) {
                    date = date.replace(date.indexOf("-"), date.length(), "");
                    attrs.append(date.toInt());
                }
                attrs.append("0");
            }
            else {
                if (date.contains("-")) {
                    date = date.replace(date.indexOf("-"), date.length(), "");
                    attrs.append(date.toInt());
                }
                else {
                    attrs.append(date.toInt());
                }
            }

            feat.setAttributes(attrs);
            fList.append(feat);
        }

        pr->addFeatures(fList);                                                                 //add all features with the data provider
        newLayer->commitChanges();

        QString layername = "Spatha Typologie";
        QgsVectorLayer *layer = saveLayerToHardDisk(newLayer, layername);
        if (!layer->isValid()) {
            QMessageBox::warning(0, "Fehler!", "Layer ist nicht valide!");
            return;
        }


        layer->setCustomProperty("labeling", "pal");
        layer->setCustomProperty("labeling/enabled", "true");
        layer->setCustomProperty("labeling/fontSize", "8");
        layer->setCustomProperty("labeling/fontStyle", "Normal");
        layer->setCustomProperty("labeling/fieldName", "klinge_ ar");

        layer = renderDating(layer, "datStart", 15);
        layer->triggerRepaint();
        delete newLayer;
    }


}



void TypologicalAnalysisGui::gladiusTypology() {
    getDating("gladius");

}

void TypologicalAnalysisGui::spathaTypology() {
    getDating("spatha");
}


void TypologicalAnalysisGui::fillFilter() {
    wo_pro->addItem("");
    wo_layer->addItem("");
    was_comb->addItem("");

    QMap<QString, QgsMapLayer*> mapLayers = QgsMapLayerRegistry::instance()->mapLayers();
    QMap<QString, QgsMapLayer*>::iterator layer_it = mapLayers.begin();

    for (;layer_it != mapLayers.end(); ++layer_it) {
        QgsVectorLayer* vl = dynamic_cast<QgsVectorLayer*>(layer_it.value());
        if (vl) {
            if (vl->name() != "Römische Plätze" &&
                vl->name() != "Provinzen (117 n. Chr.)" &&
                vl->name() != "Provinzen (303-324 n. Chr.)" &&
                vl->name() != "Flüsse" &&
                vl->name() != "Deutschland" &&
                vl->name() != "Bundesländer" &&
                vl->name() != "Google Physical") {
                wo_layer->addItem(vl->name());
            }

            if (vl->name().contains("Provinzen")) {
                QgsFeatureIterator fIter = vl->getFeatures();
                QgsFeature f;

                while(fIter.nextFeature(f)) {
                    QString provinz = f.attribute("Provinz").toString();
                    wo_pro->addItem(provinz);
                }
            }

            if (vl->name() == "Bundesländer") {
                wo_bund->addItem("");
                QgsFeatureIterator fIter = vl->getFeatures();
                QgsFeature f;

                while(fIter.nextFeature(f)) {
                    QString bundesland = f.attribute("GEN").toString();
                    wo_bund->addItem(bundesland);
                }
            }
         }
     }


    for (int i = 0; i < attrName.count(); i++) {
        if (i != 10) {
            was_comb->addItem(attrName[i]);
        }
    }
}


void TypologicalAnalysisGui::setFilters() {

    if (wo_layer->currentText() == "") {                                                        //if no layer was selected always filter Fundobjekte-Layer
        filterByLayer(vLayer);
    }
    else {                                                                                      //if another layer is chosen which is not Fundobjekte
        QList<QgsMapLayer*> layerList = QgsMapLayerRegistry::instance()->mapLayersByName(wo_layer->currentText());
        QgsVectorLayer *woLayer = qobject_cast<QgsVectorLayer *>(layerList.first());       
        filterByLayer(woLayer);
    }

    if (featureVector.isEmpty()) {
        QMessageBox::warning(0, "Fehler!", "Keine Objekte gefunden!");
        return;
    }

    //create new layer in memory to work on
    QgsVectorLayer *newLayer = new QgsVectorLayer("Point", "Filter", "memory");

    if (!newLayer->isValid()) {
        QgsDebugMsg("Layer ist nicht valide.");
        return;
    }

    QString layername = "Filterergebnis";
    copyAttributesAndFields(newLayer);
    QgsVectorLayer *layer = saveLayerToHardDisk(newLayer, layername);
    if (!layer->isValid()) {
        QMessageBox::warning(0, "Fehler!", "Layer ist nicht valide!");
        return;
    }
}



void TypologicalAnalysisGui::filterByLayer(QgsVectorLayer *layer) {

    if (wo_bund->currentText() == "" && wo_pro->currentText() == "") {
        QgsFeatureIterator fIter = layer->getFeatures();
        filter(fIter);                                                                      //filter features/objects
    }

    else if (wo_bund->currentText() != "" && wo_pro->currentText() != "") {
        QMessageBox::warning(0, "Fehler!", "Es kann nicht nach römischen Provinzen und heutigen Regionen gefiltert werden!");
    }

    else if (wo_bund->currentText() != "") {
                                                                                            //if Bundesländer was selected
        QList<QgsMapLayer*> layerList = QgsMapLayerRegistry::instance()->mapLayersByName("Bundesländer");
        QgsVectorLayer *bundLayer = qobject_cast<QgsVectorLayer *>(layerList.first());      //get layer

        //get specified Bundesland
        QString bundesland = wo_bund->currentText();
        QgsFeatureIterator bundFeatIter = bundLayer->getFeatures();
        QgsFeature bundFeature;

        while (bundFeatIter.nextFeature(bundFeature)) {
            if (bundFeature.attribute("GEN").toString() == bundesland) {
                QgsGeometry *geoBund = bundFeature.geometry();
                //geoBund = getGeometry(geoBund, bundLayer);

                if ( mCanvasGui->mapSettings().hasCrsTransformEnabled() ) {
                    try
                    {
                      QgsCoordinateTransform ct( bundLayer->crs(), mCanvasGui->mapSettings().destinationCrs());
                      geoBund->transform( ct );
                    }
                    catch ( QgsCsException &cse )
                    {
                      Q_UNUSED( cse );
                      // catch exception for 'invalid' point and leave existing selection unchanged
                      QgsLogger::warning( "Caught CRS exception " + QString( __FILE__ ) + ": " + QString::number( __LINE__ ) );
                      return;
                    }
                 }

                QgsGeometry selectGeomTrans( *geoBund );

                QgsFeatureIterator features = layer->getFeatures();
                QgsFeature f;
                QgsFeatureIds ids;
                while (features.nextFeature(f)) {
                    if (selectGeomTrans.contains(f.geometry())) {
                        ids.insert(f.id());
                    }
                }

                //get features within the specified Bundesland
                QgsFeatureIterator fIter = layer->getFeatures(QgsFeatureRequest().setFilterFids(ids));
                filter(fIter);
            }
        }
    }

    else if (wo_pro->currentText() != "") {

        QMap<QString, QgsMapLayer*> layerList = QgsMapLayerRegistry::instance()->mapLayers();

        foreach(QgsMapLayer *pLayer, layerList) {
            if (!pLayer->name().contains("Provinzen")) continue;                             //if layer is not Provinzen

            QgsVectorLayer *proLayer = qobject_cast<QgsVectorLayer *>(pLayer);
            QgsFeatureIterator prFeatIter = proLayer->getFeatures();
            QgsFeature prFeature;
            QString provinz = wo_pro->currentText();

            while (prFeatIter.nextFeature(prFeature)) {
                if (prFeature.attribute("Provinz").toString() == provinz) {

                    QgsGeometry *geoPr = prFeature.geometry();

                    if ( mCanvasGui->mapSettings().hasCrsTransformEnabled() ) {
                        try
                        {
                          QgsCoordinateTransform ct( proLayer->crs(), mCanvasGui->mapSettings().destinationCrs());
                          geoPr->transform( ct );
                        }
                        catch ( QgsCsException &cse )
                        {
                          Q_UNUSED( cse );
                          // catch exception for 'invalid' point and leave existing selection unchanged
                          QgsLogger::warning( "Caught CRS exception " + QString( __FILE__ ) + ": " + QString::number( __LINE__ ) );
                          return;
                        }
                     }

                    QgsGeometry selectGeomTrans( *geoPr );

                    QgsFeatureIterator features = layer->getFeatures();
                    QgsFeature f;
                    QgsFeatureIds ids;
                    while (features.nextFeature(f)) {
                        if (selectGeomTrans.contains(f.geometry())) {
                            ids.insert(f.id());
                        }
                    }

                    //get features within the specified Bundesland
                    QgsFeatureIterator fIter = layer->getFeatures(QgsFeatureRequest().setFilterFids(ids));
                    filter(fIter);
                    break;
                }
            }
        }
    }
}


void TypologicalAnalysisGui::filter(QgsFeatureIterator fIter) {

    QgsFeature f;

    while(fIter.nextFeature(f)) {
        mFeaturesWithinRadius.append(f);
        Feature filterFeat;
        filterFeat.feature = f;
        if (was_comb->currentText() == "") {                                                    //search in all fields
            QgsAttributes attrs = f.attributes();
            int score = 0;
            for (int i = 0; i < attrs.count(); i++) {
                if (was->text().contains(" ")) {                                                //more than one word
                    QStringList tokens = was->text().split(" ");
                    for (int j = 0; j < tokens.count(); j++) {
                        if (attrs[i].toString().contains(tokens[j])) {                          //if match is found increase score
                            score++;
                        }
                    }
                    if (score != 0) {
                        featureVector.append(filterFeat);
                        break;
                    }
                }
                else {
                    if (attrs[i].toString().contains(was->text()) && score == 0) {
                        featureVector.append(filterFeat);
                        break;
                    }
                }
            }
        }
                                                                                                //search in specific field
        else {
            QString field = was_comb->currentText();
            int index = attrName.key(field);
            QgsAttributes attrs = f.attributes();
            int score = 0;
            if (was->text().contains(" ")) {                                                    //more than one word
                QStringList tokens = was->text().split(" ");
                for (int j = 0; j < tokens.count(); j++) {
                    if (attrs[index].toString().contains(tokens[j])) {
                        score++;
                    }
                }
                if (score != 0) {
                    featureVector.append(filterFeat);
                    break;
                }
            }
            else {
                if (attrs[index].toString().contains(was->text()) && score == 0) {
                    featureVector.append(filterFeat);
                    break;
                }
            }
        }

        if (wann->text() != "") {
            QStringList suche = wann->text().split("-");
            if (suche.isEmpty()) {
                QMessageBox::warning(0, "Fehler!", "Kein gültiger Datierungszeitraum ausgewählt.");
                return;
            }

            int anfSuch = suche[0].toInt();
            int endSuch = suche[1].toInt();

            if (featureVector.isEmpty()) continue;
            int i = featureVector.size()-1;

            QgsAttributes attrs = featureVector.at(i).feature.attributes();
            int index = attrName.key("datzahl");
            if (attrs.at(index).isNull()) {
                featureVector.remove(i);
            }
            if (attrs.at(index).toString().contains("-")) {
                if (!attrs.at(index).toString().startsWith("-")) {

                    QStringList dat = attrs.at(index).toString().split("-");
                    int anf = dat[0].toInt();
                    int end = dat[1].toInt();

                    if (anf >= anfSuch && anf <= endSuch && 
                        end >= anfSuch && end <= endSuch) {
                    }
                    else {
                        featureVector.remove(i);
                    }
                }

                else {
                    int anf = 0;
                    if (attrs.at(index).toString().lastIndexOf("-") != 0) {
                        QStringList dat = attrs.at(index).toString().split("-");
                        int end = dat[2].toInt();

                        if (anf >= anfSuch && anf <= endSuch) {
                            if (end >= anfSuch && end <= endSuch) continue;
                        }
                        else {
                            featureVector.remove(i);
                        }
                    }
                }
            }
            else {
                int dat = attrs.at(index).toInt();                                             //only one date or none
                if (dat == anfSuch || dat == endSuch) {
                    continue;
                }
                if (dat > anfSuch && dat < endSuch) {
                    continue;
                }
                else {
                     featureVector.remove(i);
                }
            }
        }
    }
}

