#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <fstream>
#include <set>
#include <QDebug>
#include <QStringList>

using namespace cv;
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QList<QString> segSizes;
    segSizes.append(QString("32×32"));
    segSizes.append(QString("64×64"));
    segSizes.append(QString("128×128"));
    ui->comboBox->addItems(segSizes);
    ui->spinBox->setMinimum(1);
    ui->spinBox->setMaximum(100);
    ui->spinBox->setValue(1);
    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openImage())); //"Обзор..." button
    QObject::connect(ui->pushButton_2, SIGNAL(clicked()), this, SLOT(process())); //"Обзор..." button
    QObject::connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(printCom())); //"Обзор..." button
}

void MainWindow::printCom() {
    qDebug() << ui->comboBox->currentIndex();
}

void dumpMat(Mat* mat) {
    qDebug() << mat->type();
    ofstream myfile("output",ofstream::out);
    myfile << *mat;
    myfile.close();
}

//Open image function call
void MainWindow::openImage()
{
    //declare FileOpName as the choosen opened file name
    FileOpName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath(),tr("Image Files (*.png *.jpg *.jpeg *.bmp)"));

    //Check if FileOpName exist or not
    //function to load the image whenever fName is not empty
    if( FileOpName.size() )
    {
        imagerd = imread(FileOpName.toStdString().c_str());

        Mat rgbImage;

        cvtColor(imagerd, rgbImage, CV_BGR2RGB);
        QImage image = QImage((const unsigned char*)rgbImage.data, rgbImage.rows, rgbImage.cols, rgbImage.step, QImage::Format_RGB888);
        QGraphicsScene* scene = new QGraphicsScene();
        scene->addPixmap(QPixmap::fromImage(image));
        ui->graphicsView->setScene(scene);
//        ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
    }
}

void MainWindow::process() {
    if (ui->radioButton->isChecked()) {
        if (ui->comboBox->currentIndex() == 0) {
            KochEmbedder(32, ui->spinBox->value());
        }
        else if (ui->comboBox->currentIndex() == 1) {
            KochEmbedder(64, ui->spinBox->value());
        }
        else if (ui->comboBox->currentIndex() == 2) {
            KochEmbedder(128, ui->spinBox->value());
        }
    }
    else if (ui->radioButton_2->isChecked()) {
        if (ui->comboBox->currentIndex() == 0) {
            KochExtractor(32, ui->spinBox->value());
        }
        else if (ui->comboBox->currentIndex() == 1) {
            KochExtractor(64, ui->spinBox->value());
        }
        else if (ui->comboBox->currentIndex() == 2) {
            KochExtractor(128, ui->spinBox->value());
        }
    }
}

bool** MainWindow::calculateHashes(unsigned int seg_side) {
    int Nc = imagerd.cols*imagerd.rows / (seg_side*seg_side);

    bool** calculatedHashes = new bool*[Nc];
    for (int i = 0; i < Nc; i++) {
        calculatedHashes[i] = new bool[64];
    }

    Mat grayImage, fgray;
    cvtColor(imagerd, grayImage, CV_BGR2GRAY);

    grayImage.convertTo(fgray, CV_64F/*, 1.0/255.0*/); // also scale to [0..1] range (not mandatory)

    int y = 0;
    int n = 0;
    int x;
    Mat* segments = new Mat[Nc];
    for (int i = 0; i < Nc; i++) {
        segments[i] = Mat(seg_side,seg_side,CV_64F);
    }
    while (y < fgray.rows) {
        x = 0;
        while (x < fgray.cols) {
            for (int i = 0; i < seg_side; i++) {
                for (int j = 0; j < seg_side; j++) {
                    segments[n].at<double>(i, j) = fgray.at<double>(x + i, y + j);
                }
            }
            x = x+ seg_side;
            n = n + 1;
        }
        y = y+ seg_side;
    }

    for (int c = 0; c < Nc; c++) {
        Mat dctGray;
        dct(segments[c],dctGray);
        double average = 0;
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (i != 0 && j != 0)
                    average += dctGray.at<double>(i, j);
            }
        }
        average /= 63;
//        for (int i = 0; i < 64; i++)
//            calculatedHashes[c][i] = wmark[i];
        for (int i = 0; i < 8; i++) {
            for (int j = 0; j < 8; j++) {
                if (segments[c].at<double>(i, j) > average)
                    calculatedHashes[c][i*8 + j] = 1;
                else calculatedHashes[c][i*8 + j] = 0;
            }
        }
    }

    return calculatedHashes;
}

//bool** MainWindow::calculateHashes(unsigned int seg_side) {
//    int Nc = imagerd.cols*imagerd.rows / (seg_side*seg_side);

//    bool** calculatedHashes = new bool*[Nc];
//    for (int i = 0; i < Nc; i++) {
//        calculatedHashes[i] = new bool[64];
//    }

//    Mat grayImage, fgray;
//    cvtColor(imagerd, grayImage, CV_BGR2GRAY);

//    grayImage.convertTo(fgray, CV_64F/*, 1.0/255.0*/); // also scale to [0..1] range (not mandatory)

//    int y = 0;
//    int n = 0;
//    int x;
//    Mat* segments = new Mat[Nc];
//    for (int i = 0; i < Nc; i++) {
//        segments[i] = Mat(seg_side,seg_side,CV_64F);
//    }
//    while (y < fgray.rows) {
//        x = 0;
//        while (x < fgray.cols) {
//            for (int i = 0; i < seg_side; i++) {
//                for (int j = 0; j < seg_side; j++) {
//                    segments[n].at<double>(i, j) = fgray.at<double>(x + i, y + j);
//                }
//            }
//            x = x+ seg_side;
//            n = n + 1;
//        }
//        y = y+ seg_side;
//    }

//    for (int c = 0; c < Nc; c++) {
//        Mat sizedSeg;
//        cv::resize(segments[c], sizedSeg, Size(8, 8));
//        double average = 0;
//        for (int i = 0; i < sizedSeg.cols; i++) {
//            for (int j = 0; j < sizedSeg.cols; j++) {
//                    average += segments[c].at<double>(i, j);
//            }
//        }
//        average /= 64;
////        for (int i = 0; i < 64; i++)
////            calculatedHashes[c][i] = wmark[i];
//        for (int i = 0; i < sizedSeg.cols; i++) {
//            for (int j = 0; j < sizedSeg.cols; j++) {
//                if (segments[c].at<double>(i, j) > average)
//                    calculatedHashes[c][i*8 + j] = 1;
//                else calculatedHashes[c][i*8 + j] = 0;
//            }
//        }
//    }

//    return calculatedHashes;
//}

//bool** MainWindow::calculateHashes(unsigned int seg_side) {
//    int Nc = imagerd.cols*imagerd.rows / (seg_side*seg_side);

//    bool** calculatedHashes = new bool*[Nc];
//    for (int i = 0; i < Nc; i++) {
//        calculatedHashes[i] = wmark;
//    }
//    return calculatedHashes;
//}


void MainWindow::KochEmbedder(unsigned int seg_side, unsigned int P) {
    bool** calculatedHashes = calculateHashes(seg_side);
    Mat blue;
    vector<Mat> channels(3);
    split(imagerd, channels);
    blue = channels[0];

    Mat fimage;

    blue.convertTo(fimage, CV_64F/*, 1.0/255.0*/); // also scale to [0..1] range (not mandatory)


    int Nc = fimage.cols*fimage.rows / (seg_side*seg_side);

    int y = 0;
    int n = 0;
    int x;
    Mat* segments = new Mat[Nc];
    for (int i = 0; i < Nc; i++) {
        segments[i] = Mat(seg_side, seg_side, CV_64F);
    }
    while (y < fimage.rows) {
        x = 0;
        while (x < fimage.cols) {
            for (int i = 0; i < seg_side; i++) {
                for (int j = 0; j < seg_side; j++) {
                    segments[n].at<double>(i, j) = fimage.at<double>(x + i, y + j);
                }
            }
            x = x + seg_side;
            n = n + 1;
        }
        y = y + seg_side;
    }


    for (int c = 0; c < Nc; c++) {
        Mat dctMat;
        dct(segments[c],dctMat);

        int start_u = 0;
        int start_v = seg_side - 1;
        int u1 = start_u;
        int v1 = start_v;
        int end_u = seg_side - 1;
        int end_v = 0;
        int diag = 0;
        for (int i = 0; i < 64; i++) {
            if (u1 + 1 > end_u || v1 -1 < end_v) {
                if (diag == 0)
                    diag = 1;
                else if (diag > 0)
                    diag = -diag;
                else if (diag < 0)
                    diag = -diag + 1;

                if (diag > 0) {
                    start_u = 0 + diag * 1;
                    start_v = seg_side - 1;
                    end_u = seg_side - 1;
                    end_v = 0 + diag * 1;
                }
                else if (diag < 0) {
                    start_u = 0;
                    start_v = seg_side - 1 + diag * 1;
                    end_u = seg_side - 1 + diag * 1;
                    end_v = 0;
                }

                u1 = start_u;
                v1 = start_v;
            }
            int u2 = u1 + 1;
            int v2 = v1 - 1;
            if (calculatedHashes[c][i] == true) {
                int n = 0;
                while(abs(dctMat.at<double>(v1,u1))-abs(dctMat.at<double>(v2,u2)) + P >= 0) {
                    if ((dctMat.at<double>(v1,u1) >0 && dctMat.at<double>(v2,u2) > 0) || (dctMat.at<double>(v1,u1)<0 &&dctMat.at<double>(v2,u2)<0)) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) - 1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)+ 1.0;
                    }
                    else if (dctMat.at<double>(v1,u1) <= 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) +1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)+1.0;
                    }
                    else if (dctMat.at<double>(v2,u2) <= 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) -1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)- 1.0;
                    }
                }
            }
            else {
                //qDebug() << "flase";
                while(abs(dctMat.at<double>(v1,u1)) - abs(dctMat.at<double>(v2,u2)) <= P) {
                    if ((dctMat.at<double>(v1,u1) > 0 && dctMat.at<double>(v2,u2) > 0) ||
                            (dctMat.at<double>(v1,u1) <0 && dctMat.at<double>(v2,u2) < 0)) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) + 1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2) - 1.0;
                    }
                    else if (dctMat.at<double>(v1,u1) <= 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) -1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2) -1.0;
                    }
                    else if (dctMat.at<double>(v2,u2) <= 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) +1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)+1.0;
                    }
                }
            }
            u1 = u1 + 2;
            v1 = v1 - 2;
        }
        dct(dctMat,segments[c], DCT_INVERSE);
        dctMat.release();
    }
    y = 0;
    n = 0;
    Mat out = Mat(imagerd.rows, imagerd.cols,CV_64F);

    while (y < imagerd.rows) {
        x = 0;
        while (x < imagerd.cols) {
            for (int i = 0; i < seg_side; i++) {
                for (int j = 0; j < seg_side; j++) {
                    out.at<double>(x + i, y+ j) = segments[n].at<double>(i, j);
                }
            }
            x = x+ seg_side;

            n = n + 1;
        }
        y = y+ seg_side;
    }

//    Mat idctMat;
//    dct(out,idctMat, DCT_INVERSE);
    Mat tmpImage, cImage, tImage;

    out.convertTo(tmpImage, CV_8U/*, 255.0*/); // also scale to [0..1] range (not mandatory)
    channels[0] = tmpImage;
    merge(channels, tImage);
    cvtColor(tImage, cImage, CV_BGR2RGB);
    QImage image = QImage((const unsigned char*)cImage.data, cImage.rows, cImage.cols, cImage.step, QImage::Format_RGB888);
    QGraphicsScene* scene = new QGraphicsScene();
    scene->addPixmap(QPixmap::fromImage(image));
    ui->graphicsView->setScene(scene);
//    ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
    imwrite("outEmb.png", tImage);
}

int hDist(bool* v1, bool* v2, int length) {
    int distance = 0;
    for (int i = 0; i < length; i++) {
        if (v1[i] != v2[i])
            distance++;
    }
    return distance;
}

void MainWindow::KochExtractor(unsigned int seg_side, unsigned int T) {
    bool** calculatedHashes = calculateHashes(seg_side);
    int Nc = imagerd.cols* imagerd.rows / (seg_side*seg_side);
    bool** extractedHashes = new bool*[Nc];
    for (int i = 0; i < Nc; i++) {
        extractedHashes[i] = new bool[64];
    }
    Mat blue;
    vector<Mat> channels(3);
    split(imagerd, channels);
    blue = channels[0];

    Mat fimage;

    blue.convertTo(fimage, CV_64F/*, 1.0/255.0*/); // also scale to [0..1] range (not mandatory)

//    Mat dctMat;
//    dct(fimage,dctMat);

    int y = 0;
    int n = 0;
    int x;
    Mat* segments = new Mat[Nc];
    for (int i = 0; i < Nc; i++) {
        segments[i] = Mat(seg_side,seg_side,CV_64F);
    }
    while (y < fimage.rows) {
        x = 0;
        while (x < fimage.cols) {
            for (int i = 0; i < seg_side; i++) {
                for (int j = 0; j < seg_side; j++) {
                    segments[n].at<double>(i, j) = fimage.at<double>(x + i, y + j);
                }
            }
            x = x+ seg_side;
            n = n + 1;
        }
        y = y+ seg_side;
    }

    std::vector<bool> wmark_e;
    for (int c = 0; c < Nc; c++) {
        Mat dctMat;
        dct(segments[c],dctMat);
        int start_u = 0;
        int start_v = seg_side - 1;
        int u1 = start_u;
        int v1 = start_v;
        int end_u = seg_side - 1;
        int end_v = 0;
        int diag = 0;
        for (int ii = 0; ii < 64; ii++) {
            if (u1 + 1 > end_u || v1 -1 < end_v) {
                if (diag == 0)
                    diag = 1;
                else if (diag > 0)
                    diag = -diag;
                else if (diag < 0)
                    diag = -diag + 1;

                if (diag > 0) {
                    start_u = 0 + diag * 1;
                    start_v = seg_side - 1;
                    end_u = seg_side - 1;
                    end_v = 0 + diag * 1;
                }
                else if (diag < 0) {
                    start_u = 0;
                    start_v = seg_side - 1 + diag * 1;
                    end_u = seg_side - 1 + diag * 1;
                    end_v = 0;
                }

                u1 = start_u;
                v1 = start_v;
            }
            int u2 = u1 + 1;
            int v2 = v1 - 1;
//            extractedHashes[c][ii] = wmark[ii];
            if ((abs(dctMat.at<double>(v1,u1)) - abs(dctMat.at<double>(v2,u2))) > 0)
                extractedHashes[c][ii] = false;
            else extractedHashes[c][ii] = true;

            u1 = u1 + 2;
            v1 = v1 - 2;
        }
        dct(dctMat,segments[c], DCT_INVERSE);
//        ofstream myfile("output",ofstream::out);
//        for (bool i : wmark_e) {
//            myfile << (int)i;

//        }

//        myfile.close();
    }

    std::set<int> modifiedSegs;
    for (int c = 0; c < Nc; c++) {
        qDebug() << c << ": " << hDist(calculatedHashes[c], extractedHashes[c], 64);
        if (hDist(calculatedHashes[c], extractedHashes[c], 64) > T) {
            modifiedSegs.insert(modifiedSegs.end(), c);
        }
    }
    y = 0;
    n = 0;
    Mat out = Mat(imagerd.rows, imagerd.cols,CV_64F);

    while (y < out.rows) {
        x = 0;
        while (x < out.cols) {
            for (int i = 0; i < seg_side; i++) {
                for (int j = 0; j < seg_side; j++) {
                    out.at<double>(x + i, y+ j) = segments[n].at<double>(i, j);
                }
            }
            x = x + seg_side;

            n = n + 1;
        }
        y = y + seg_side;
    }

//    Mat idctMat;
//    dct(out,idctMat, DCT_INVERSE);
    Mat tmpImage, cImage, tImage;

    y = 0;
    n = 0;

    while (y < out.rows) {
        x = 0;
        while (x < out.cols) {
            if (modifiedSegs.find(n) != modifiedSegs.end()) {
                for (int i = 0; i < seg_side; i++)
                    for (int j = 0; j < seg_side; j++)
                        out.at<double>(x + i, y+ j) = 0.0;
            }

            x = x + seg_side;
            n = n + 1;
        }
        y = y + seg_side;
    }
    out.convertTo(tmpImage, CV_8U);

    channels[0] = tmpImage;
    merge(channels, tImage);
    cvtColor(tImage, cImage, CV_BGR2RGB);
    QImage image = QImage((const unsigned char*)cImage.data, cImage.rows, cImage.cols, cImage.step, QImage::Format_RGB888);
    QGraphicsScene* scene = new QGraphicsScene();
    scene->addPixmap(QPixmap::fromImage(image));
    ui->graphicsView->setScene(scene);
//ui->graphicsView->fitInView( scene->sceneRect(), Qt::KeepAspectRatio );
    imwrite("outExt.png", tImage);
}

MainWindow::~MainWindow()
{
    delete ui;
}
