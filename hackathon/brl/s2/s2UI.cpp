#include "v3d_message.h"
#include <QWidget>
#include <QDialogButtonBox>
#include <QtGui>
#include <QtNetwork>
#include <stdlib.h>
#include <QThread>
#include "s2Controller.h"
#include "s2UI.h"
#include "s2plot.h"
#include "stackAnalyzer.h"

S2UI::S2UI(V3DPluginCallback2 &callback, QWidget *parent):   QDialog(parent)
{
    fileString =QString("");
    lastFile = QString("");
    allROILocations = new LandmarkList;
    cb = &callback;
    myStackAnalyzer = new StackAnalyzer(callback);
    s2Label = new QLabel(tr("smartScope 2"));
    s2LineEdit = new QLineEdit("01b");
    startPosMonButton = new QPushButton(tr("start monitor"));
    startSmartScanPB = new QPushButton(tr("SmartScan"));
    startSmartScanPB->setEnabled(false);
    startStackAnalyzerPB = new QPushButton(tr("start stack analyzer"));
    createROIMonitor();

    lhTabs = new QTabWidget();
    lhTabs->addTab(createS2Monitors(), "s2 Monitor");
    lhTabs->addTab(&myPosMon, "monCOM" );
    lhTabs->addTab(&myController, "s2COM");
    mainLayout = new QGridLayout();
    mainLayout->addWidget(s2Label, 0, 0);
    mainLayout->addWidget(s2LineEdit, 0, 1);
    createButtonBox1();
    mainLayout->addWidget(startS2PushButton, 1,0);
    mainLayout->addWidget(startScanPushButton, 1,1);
    mainLayout->addWidget(loadScanPushButton, 2,0);
    mainLayout->addWidget(startZStackPushButton,2,1);
    mainLayout->addWidget(startPosMonButton,3,0);
    mainLayout->addWidget(startSmartScanPB, 3,1,1,2);

    mainLayout->addWidget(lhTabs, 4,0, 4, 3);
    mainLayout->addWidget(createROIControls(), 0,5, 4,4);
    mainLayout->addWidget(roiGroupBox,4,5,7,4);
    mainLayout->addWidget(startStackAnalyzerPB, 8, 0,1,2);
    roiGroupBox->show();
    hookUpSignalsAndSlots();
    //workerThread = new QThread;
    //myStackAnalyzer->moveToThread(workerThread);
    posMonStatus = false;
    waitingForFile = false;
    smartScanStatus = 0;
    setLayout(mainLayout);
    setWindowTitle(tr("smartScope2 Interface"));
    //workerThread->start();


}

void S2UI::hookUpSignalsAndSlots(){
    //internal communication
    connect(startS2PushButton, SIGNAL(clicked()), this, SLOT(startS2()));
    connect(loadScanPushButton, SIGNAL(clicked()), this, SLOT(loadScan()));
    connect(startPosMonButton,SIGNAL(clicked()), this, SLOT(posMonButtonClicked()));
    connect(roiXEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiYEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiZEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiXWEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiYWEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiZWEdit, SIGNAL(textChanged(QString)), this, SLOT(updateROIPlot(QString)));
    connect(roiClearPB, SIGNAL(clicked()),this,SLOT(clearROIPlot()));

    // communication with myController to send commands
    connect(startScanPushButton, SIGNAL(clicked()), this, SLOT(startScan()));
    connect(&myController,SIGNAL(newBroadcast(QString)), this, SLOT(updateString(QString)));
    connect(centerGalvosPB, SIGNAL(clicked()), &myController, SLOT(centerGalvos()));
    connect(startZStackPushButton, SIGNAL(clicked()), &myController, SLOT(startZStack()));
    connect(startZStackPushButton, SIGNAL(clicked()), this, SLOT(startingZStack()));

    connect(startSmartScanPB, SIGNAL(clicked()), this, SLOT(startingSmartScan()));

    // communication with myPosMon to monitor parameters
    connect(&myPosMon, SIGNAL(newBroadcast(QString)), this, SLOT(updateString(QString)));
    connect(&myPosMon, SIGNAL(pmStatus(bool)), this, SLOT(pmStatusHandler(bool)));
    connect(&myPosMon, SIGNAL(newS2Parameter(QMap<int,S2Parameter>)), this, SLOT(updateS2Data(QMap<int,S2Parameter>)));
    connect(this, SIGNAL(startPM()), &myPosMon, SLOT(startPosMon()));
    connect(this, SIGNAL(stopPM()), &myPosMon, SLOT(stopPosMon()));


    // communication with  myStackAnalyzer
    connect(startStackAnalyzerPB, SIGNAL(clicked()),myStackAnalyzer, SLOT(loadScan()));
    connect(this, SIGNAL(newImageData(Image4DSimple*)), myStackAnalyzer, SLOT(processStack(Image4DSimple*)) );
    connect(myStackAnalyzer, SIGNAL(analysisDone(LandmarkList)), this, SLOT(handleNewLocation(LandmarkList)));
    connect(this, SIGNAL(moveToNext(LocationSimple)), &myController, SLOT(initROI(LocationSimple)));


}



QGroupBox *S2UI::createROIMonitor(){
    roiGroupBox = new QGroupBox(tr("ROI Monitor"));
    gl = new QGridLayout();
    roiGS = new QGraphicsScene();
    roiGS->setObjectName("roiGS");
    roiGV = new QGraphicsView();
    roiGV->setObjectName("roiGV");
    roiGV->setScene(roiGS);
    roiRect = QRectF(-400, -400, 800, 800);
    roiGS->addRect(roiRect,QPen::QPen(Qt::gray, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin), QBrush::QBrush(Qt::gray));
    newRect = roiGS->addRect(0,0,50,50);
    //roiGV->setViewportUpdateMode(QGraphicsView::FullViewportUpdate)  ;
    roiGV->adjustSize();
    roiGV->setDragMode(QGraphicsView::ScrollHandDrag);
    gl->addWidget(roiGV,0,0,4,4);
    roiClearPB = new QPushButton(tr("clear ROIs"));
    gl->addWidget(roiClearPB, 4,0);

    roiGroupBox->setLayout(gl);
}

void S2UI::updateROIPlot(QString ignore){
    //roiRect.moveLeft(roiXEdit->text().toFloat());
    //roiRect.setY(roiYEdit->text().toFloat());
    //qDebug()<<"y="<<roiYEdit->text().toFloat();
    roiGS->removeItem(newRect);
    newRect =  roiGS->addRect(roiXEdit->text().toFloat(),roiYEdit->text().toFloat(),roiXWEdit->text().toFloat(),roiYWEdit->text().toFloat());
    //newRect =  roiGS->addRect(uiS2ParameterMap[1].getCurrentValue()*10,uiS2ParameterMap[2].getCurrentValue()*10,uiS2ParameterMap[13].getCurrentValue(),uiS2ParameterMap[14].getCurrentValue());

}

void S2UI::createButtonBox1(){
    startS2PushButton = new QPushButton(tr("Start smartScope2"));
    startScanPushButton = new QPushButton(tr("single scan"));
    loadScanPushButton = new QPushButton(tr("load last scan"));
    startZStackPushButton = new QPushButton(tr("start z stack"));

}

QGroupBox *S2UI::createS2Monitors(){
    // add fields with data...  currently hardcoding the number of parameters...
    QGroupBox *gMonBox = new QGroupBox(tr("&smartScope Monitor"));

    QGridLayout *gbMon = new QGridLayout;

    for (int jj=0; jj<=19; jj++){
        QLabel * labeli = new QLabel(tr("test"));
        labeli->setText(QString::number(jj));
        labeli->setObjectName(QString::number(jj));
        labeli->setWordWrap(true);
        gbMon->addWidget(labeli, jj%11, jj/11);
    }
    gMonBox->setLayout(gbMon);
    return gMonBox;
}

QGroupBox *S2UI::createROIControls(){
    QGroupBox *gROIBox = new QGroupBox(tr("&ROI Controls"));
    gROIBox->setCheckable(true);
    gROIBox->setChecked(true);
    QLabel *roiXLabel = new QLabel(tr("ROI x ="));
    roiXEdit = new QLineEdit("0.0");
    roiXLabel->setBuddy(roiXEdit);
    roiXEdit->setObjectName("roiX");
    QLabel *roiYLabel = new QLabel(tr("ROI y ="));
    roiYEdit = new QLineEdit("0.0");
    roiYLabel->setBuddy(roiYEdit);
    roiYEdit->setObjectName("roiY");

    QLabel *roiZLabel = new QLabel(tr("ROI z ="));
    roiZEdit = new QLineEdit("0.0");
    roiZLabel->setBuddy(roiZEdit);
    roiZEdit->setObjectName("roiZ");

    QLabel *roiXWLabel = new QLabel(tr("size ="));
    roiXWEdit = new QLineEdit("0.0");
    roiXWLabel->setBuddy(roiXWEdit);
    roiXWEdit->setObjectName("roiXW");
    QLabel *roiYWLabel = new QLabel(tr(" size ="));
    roiYWEdit = new QLineEdit("0.0");
    roiYWLabel->setBuddy(roiYWEdit);
    roiYWEdit->setObjectName("roiYW");
    QLabel  *roiZWLabel = new QLabel(tr("size ="));
    roiZWEdit = new QLineEdit("0.0");
    roiZWLabel->setBuddy(roiZWEdit);
    roiZWEdit->setObjectName("roiZW");

    centerGalvosPB = new QPushButton(tr("center galvos"));


    QGridLayout *glROI = new QGridLayout;
    glROI->addWidget(roiXLabel, 1, 0);
    glROI->addWidget(roiXEdit, 1, 1);
    glROI->addWidget(roiXWLabel, 1, 2);
    glROI->addWidget(roiXWEdit, 1, 3);
    glROI->addWidget(roiYLabel, 2, 0);
    glROI->addWidget(roiYEdit, 2, 1);
    glROI->addWidget(roiYWLabel, 2, 2);
    glROI->addWidget(roiYWEdit, 2, 3);
    glROI->addWidget(roiZLabel, 3, 0);
    glROI->addWidget(roiZEdit, 3, 1);
    glROI->addWidget(roiZWLabel, 3, 2);
    glROI->addWidget(roiZWEdit, 3, 3);
    glROI->addWidget(centerGalvosPB, 4, 1, 1,2);

    gROIBox->setLayout(glROI);
    return gROIBox;
}



void S2UI::startS2()
{
    myController.initializeS2();
    myPosMon.initializeS2();
    startS2PushButton->setText("s2 running");// should check something..?
}

void S2UI::startScan()
{
    lastFile=getFileString();
    waitingForFile = true;
    QTimer::singleShot(0, &myController, SLOT(startScan()));
    roiGS->addRect(roiXEdit->text().toFloat(),roiYEdit->text().toFloat(),roiXWEdit->text().toFloat(),roiYWEdit->text().toFloat(), QPen::QPen(Qt::green, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin));

}


void S2UI::loadScan(){


    QString latestString = getFileString();
    //QString latestString =QString("/Volumes/mat/BRL/testData/ZSeries-01142016-0940-048/ZSeries-01142016-0940-048_Cycle00001_Ch2_000001.ome.tif");
    QFileInfo imageFileInfo = QFileInfo(latestString);
    if (imageFileInfo.isReadable()){
        //set up metadata in the eventual output image.
        // do this here to minimize chance of modified values...
        Image4DSimple  total4DImage;
        total4DImage.setFileName(latestString.toLocal8Bit().data());
        // THE VALUES below are 'live' values which could potentially change
        // while the image data is being read in.
        total4DImage.setRezX(uiS2ParameterMap[8].getCurrentValue());
        total4DImage.setRezY(uiS2ParameterMap[9].getCurrentValue());
        total4DImage.setRezZ(1.0);// HARDCODED Z RESOLUTION!  needs to be added to parameterMap



        v3dhandle newwin = cb->newImageWindow();
        Image4DSimple * pNewImage = cb->loadImage(latestString.toLatin1().data());
        QDir imageDir =  imageFileInfo.dir();
        QStringList filterList;
        filterList.append(QString("*Ch2*.tif"));
        imageDir.setNameFilters(filterList);
        QStringList fileList = imageDir.entryList();

        //get the parent dir and the list of ch1....ome.tif files
        //use this to id the number of images in the stack (in one channel?!)
        long x = pNewImage->getXDim();
        long y = pNewImage->getYDim();
        long nFrames = fileList.length();
        long pBytes = pNewImage->getUnitBytes();



        V3DLONG tunits = x*y*nFrames;
        unsigned short int * total1dData = new unsigned short int [tunits];
        V3DLONG totalImageIndex = 0;
        for (int f=0; f<nFrames; f++){
            qDebug()<<fileList[f];
            Image4DSimple * pNewImage = cb->loadImage(imageDir.absoluteFilePath(fileList[f]).toLatin1().data());
            if (pNewImage->valid()){
                unsigned short int * data1d = 0;
                data1d = new unsigned short int [x*y];
                data1d = (unsigned short int*)pNewImage->getRawData();
                for (V3DLONG i = 0; i< (x*y); i++){
                    total1dData[totalImageIndex]= data1d[i];
                    totalImageIndex++;
                }
            }else{
                qDebug()<<imageDir.absoluteFilePath(fileList[f])<<" failed!";
            }
        }
        total4DImage.setData((unsigned char*)total1dData, x, y, nFrames, 1, V3D_UINT16);
        cb->setImage(newwin, &total4DImage);
        cb->setImageName(newwin,QString("test"));
        cb->updateImageWindow(newwin);
        if (smartScanStatus ==1){
            total4DImage.setOriginX(scanList.value(scanNumber).x);// this is in pixels, using the expected origin
            total4DImage.setOriginY(scanList.value(scanNumber).y);
        emit newImageData(&total4DImage); // goes to stackAnalyzer
        return;}

    }else{
        qDebug()<<"invalid image";
    }
}

void S2UI::displayScan(){ // this will listen for a signal from myController
    //containing either a filename or  eventually an address

}



void S2UI::pmStatusHandler(bool pmStatus){
    posMonStatus = pmStatus;
    s2LineEdit->setText(tr("pmstatus updated"));
}

void S2UI::posMonButtonClicked(){
    // if it's not running, start it
    // and change button text to 'stop pos mon'
    if (!posMonStatus){
        emit startPM();
        s2LineEdit->setText(tr("Position Monitor started"));
        startPosMonButton->setText(tr("stop position monitor"));
        startSmartScanPB->setEnabled(true);
    }
    else{
        emit stopPM();
        startPosMonButton->setText(tr("start position monitor"));
        startSmartScanPB->setEnabled(false);

    }
    // if it's running, stop it
    // and change text to start pos mon


}
void S2UI::updateS2Data( QMap<int, S2Parameter> currentParameterMap){


    int minVal = 0;
    int maxVal = currentParameterMap.keys().length();
    for (int i= 0; i <maxVal ; i++){
        QString parameterStringi = currentParameterMap[i].getParameterName();
        float parameterValuei = currentParameterMap[i].getCurrentValue();
        QString iString = QString::number(i);
        if (currentParameterMap[i].getExpectedType().contains("string")){
            parameterStringi.append(" = ").append(currentParameterMap[i].getCurrentString());
        }
        if (currentParameterMap[i].getExpectedType().contains("float")){
            parameterStringi.append(" = ").append(QString::number(parameterValuei));
        }
        if (currentParameterMap[i].getExpectedType().contains("list")){
            QString fString = currentParameterMap[i].getCurrentString().split(".xml").first();
            parameterStringi.append(" = ").append(fString);
            updateFileString(fString);
        }
        QLabel* item = this->findChild<QLabel*>( iString);
        if (item){
            item->setText(parameterStringi.split("\\").last());
        }


    }

    checkParameters(currentParameterMap);

}

void S2UI::checkParameters(QMap<int, S2Parameter> currentParameterMap){
    int minVal = 0;
    int maxVal = currentParameterMap.keys().length();
    for (int i= 0; i <maxVal ; i++){
        if (currentParameterMap[i].getExpectedType().contains("float")){
            if (currentParameterMap[i].getCurrentValue() != uiS2ParameterMap[i].getCurrentValue())
                uiS2ParameterMap[i].setCurrentValue(currentParameterMap[i].getCurrentValue());
            if (i==18){
                //updateROIPlot(QString("ignore"));
                roiXEdit->setText(QString::number( uiS2ParameterMap[i].getCurrentValue()));
            }else if (i==19){
                roiYEdit->setText(QString::number( uiS2ParameterMap[i].getCurrentValue()));
            }else if (i==13){
                roiXWEdit->setText(QString::number( uiS2ParameterMap[i].getCurrentValue()));
            }else if (i==14){
                roiYWEdit->setText(QString::number( uiS2ParameterMap[i].getCurrentValue()));
            }
        }
    }
}



void S2UI::updateString(QString broadcastedString){
}


//  -------  smart scanning stuffs   --------
//  -----------------------------------------
void S2UI::startingSmartScan(){

    if (smartScanStatus==1){
        smartScanStatus=0;
        startSmartScanPB->setText("smartScan");
        allROILocations->clear();
        return;
    }
    smartScanStatus = 1;
    startSmartScanPB->setText("cancel smartScan");
    if (allROILocations->isEmpty()){
       s2LineEdit->setText("starting smartScan...");
        LocationSimple startLocation = LocationSimple(uiS2ParameterMap[18].getCurrentValue()/uiS2ParameterMap[8].getCurrentValue(),
                uiS2ParameterMap[19].getCurrentValue()/uiS2ParameterMap[9].getCurrentValue(),
                0);
        startLocation.mass = 0;
        allROILocations->append(startLocation);
        QTimer::singleShot(10,this, SLOT(smartScanHandler()));
    }
    // append text to noteTaker
    //

}

void S2UI::handleNewLocation(LandmarkList newLandmarks){
    qDebug()<<"got "<< newLandmarks.length()<<"  new landmarks associated with ROI "<<scanNumber;
    for (int i = 0; i<newLandmarks.length(); i++){
        qDebug()<<"x= "<<newLandmarks.value(i).x<<" y = "<<newLandmarks.value(i).y<<" z= "<<newLandmarks.value(i).z;
        allROILocations->append(newLandmarks.value(i));
    }
    scanNumber++;
smartScanHandler();

}
void S2UI::smartScanHandler(){
    if (smartScanStatus!=1){
        qDebug()<<"smartScan aborted";
        scanNumber = 0;
        return;
    }
    qDebug()<<"we now have a total of "<< allROILocations->length()<<" target ROIs...";
    for (int i = 0; i<allROILocations->length(); i++){
        qDebug()<<"x= "<<allROILocations->value(i).x<<" y = "<<allROILocations->value(i).y<<" z= "<<allROILocations->value(i).z;
    }

    if (!allROILocations->isEmpty()){
        LocationSimple nextLocation = allROILocations->first();
        allROILocations->removeFirst();
        moveToROI(nextLocation);
        waitingForFile = true;
        scanList.append(nextLocation);
        QTimer::singleShot(100, &myController, SLOT(startZStack())); //hardcoded delay here... not sure
        // how to make this more eventdriven. maybe  wait for move to finish.
        qDebug()<<"start next ROI at x = "<<nextLocation.x<<"  y = "<<nextLocation.y;
        s2LineEdit->setText(QString("start next ROI at x = ").append(QString::number(nextLocation.x)).append("  y = ").append(QString::number(nextLocation.y)));
    }

}


// ------------------------------------


void S2UI::startingZStack(){
    QTimer::singleShot(100, &myController, SLOT(startZStack()));
    qDebug()<<"start single z Stack";
    roiGS->addRect(roiXEdit->text().toFloat(),roiYEdit->text().toFloat(),roiXWEdit->text().toFloat(),roiYWEdit->text().toFloat(), QPen::QPen(Qt::blue, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

}

void S2UI::updateFileString(QString inputString){
    //this means a new file has been created.. it can be late by up to 1 full cycle of s2parametermap updating
    // but it guarantees that the acquisition is done
    // a separate poller of updated filename could be much faster.
    // final version will require much more rigorous timing- it's not clear how we'll parse out
    // the streamed image data into files...
    fileString = inputString;
    fileString.replace("\\AIBSDATA","\\Volumes").replace("\\","/").append("_Cycle00001_Ch2_000001.ome.tif");
    if ((QString::compare(fileString,lastFile, Qt::CaseInsensitive)!=0)&(waitingForFile)){
        waitingForFile = false;
        QTimer::singleShot(0, this, SLOT(loadScan()));

    }
    lastFile = fileString;
}

void S2UI::s2ROIMonitor(){

    if ((!allROILocations->isEmpty())&(!waitingForFile)){
        LocationSimple nextLocation = allROILocations->first();
        allROILocations->removeFirst();
        moveToROI(nextLocation);
        QTimer::singleShot(100, &myController, SLOT(startZStack()));

    }

    QTimer::singleShot(10, this, SLOT(s2ROIMonitor()));

}

void S2UI::clearROIPlot(){
    roiGS->clear();
    roiRect = QRectF(-400, -400, 800, 800);
    roiGS->addRect(roiRect,QPen::QPen(Qt::gray, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin), QBrush::QBrush(Qt::gray));
    newRect = roiGS->addRect(0,0,10,10);
}

QString S2UI::getFileString(){
    return fileString;
}




void S2UI::moveToROI(LocationSimple nextROI){
    // convert from pixels to microns:
    if( posMonStatus){
    float nextXMicrons = nextROI.x * uiS2ParameterMap[8].getCurrentValue();
    float nextYMicrons = nextROI.y* uiS2ParameterMap[9].getCurrentValue();
    // and now to galvo voltage:
    float nextGalvoX = nextXMicrons/uiS2ParameterMap[17].getCurrentValue();
    float nextGalvoY = nextYMicrons/uiS2ParameterMap[17].getCurrentValue();
    LocationSimple newLoc;
    newLoc.x = nextGalvoX;
    newLoc.y = nextGalvoY;
    roiGS->addRect(nextXMicrons,nextYMicrons,roiXWEdit->text().toFloat(),roiYWEdit->text().toFloat(), QPen::QPen(Qt::blue, 3, Qt::DashDotLine, Qt::RoundCap, Qt::RoundJoin));
    emit moveToNext(newLoc);
    }else{
        s2LineEdit->setText("start PosMon before moving galvos");
        smartScanStatus = -1;
    }
}

//  set filename in Image4DSimple* using  ->setFileName
//  BEFORE PASSING TO StackAnalyzer slot.


// get voltage-to-pixel-to-micron resolution and add methods to
// move to pan to micron positions using -sts currentScanCenter

// need an origin at the beginning

// need a new 'start smartscan' button as path to eventual handler

//

