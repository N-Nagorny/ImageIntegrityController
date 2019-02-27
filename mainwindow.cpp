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
    QList<QString> imgFormats;
    imgFormats.append(QString("PNG"));
    imgFormats.append(QString("PGM"));
    ui->comboBox->addItems(segSizes);
    ui->comboBox_2->addItems(imgFormats);
    ui->spinBox->setMinimum(1);
    ui->spinBox->setMaximum(100);
    ui->spinBox->setValue(1);
    QObject::connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(openImage())); //"Обзор..." button
    QObject::connect(ui->pushButton_3, SIGNAL(clicked()), this, SLOT(processBatch()));
    QObject::connect(ui->comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSegSide(int))); //"Обзор..." button
    QObject::connect(ui->comboBox_2, SIGNAL(currentIndexChanged(int)), this, SLOT(updateImgFormat(int)));
    QObject::connect(ui->spinBox, SIGNAL(valueChanged(int)), this, SLOT(updateSpinBoxValue(int))); //"Обзор..." button
}

void MainWindow::updateSegSide(int newValue) {
    switch(newValue) {
    case 0:
        seg_side = 32;
        break;
    case 1:
        seg_side = 64;
        break;
    case 2:
        seg_side = 128;
        break;
    }
}

void MainWindow::updateImgFormat(int newValue) {
    switch(newValue) {
    case 0:
        imgFormat = ImgFormat::PNG;
        break;
    case 1:
        imgFormat = ImgFormat::PGM;
        break;
    }
    qDebug() << static_cast<std::underlying_type<ImgFormat>::type>(imgFormat);
}

void MainWindow::updateSpinBoxValue(int newValue) {
    spinBoxValue = newValue;
}

void MainWindow::processBatch() {
    QString inputDir = QFileDialog::getExistingDirectory(this, tr("Select an input folder"),
                                                    QDir::currentPath(),
                                                    QFileDialog::ShowDirsOnly);
    QDir inputDirectory(inputDir);
    QString outputDir = QFileDialog::getExistingDirectory(this, tr("Select an output folder"),
                                                    QDir::currentPath(),
                                                    QFileDialog::ShowDirsOnly);
    QStringList images = inputDirectory.entryList(QStringList() << "*.jpg" << "*.JPG" << "*.png" << "*.PNG" << "*.pgm" << "*.PGM" << "*.jpeg" << ".JPEG" << "*.bmp" << "*.BMP", QDir::Files);
    for(int i = 0; i < images.size(); i++) {
        qDebug() << images.at(i).toStdString().c_str();
        origImage = imread((inputDir + '/' + images.at(i)).toStdString().c_str());
        QFileInfo info((inputDir + '/' + images.at(i)).toStdString().c_str());
        QString strNewName = outputDir + "/" + info.completeBaseName();
        if (ui->radioButton->isChecked()) {
            saveImage(KochEmbedder(seg_side, spinBoxValue, true), strNewName,imgFormat);
        }
        else if (ui->radioButton_2->isChecked()) {
            saveImage(KochExtractor(seg_side, spinBoxValue, true), strNewName,imgFormat);
        }
    }
}

//Open image function call
void MainWindow::openImage()
{
    //declare FileOpName as the choosen opened file name
    QString FileOpName = QFileDialog::getOpenFileName(this,
                                    tr("Open File"), QDir::currentPath(),tr("Image Files (*.png *.pgm *.jpg *.jpeg *.bmp)"));

    //Check if FileOpName exist or not
    //function to load the image whenever fName is not empty
    if( FileOpName.size() )
    {
        origImage = imread(FileOpName.toStdString().c_str());

        showImage(origImage);
        QFileInfo info(FileOpName);
        if (ui->radioButton->isChecked()) {
            QString strNewName = info.path() + "/" + info.completeBaseName() + "_emb";
            saveImage(KochEmbedder(seg_side, spinBoxValue, false), strNewName, imgFormat);
        }
        else if (ui->radioButton_2->isChecked()) {
            QString strNewName = info.path() + "/" + info.completeBaseName() + "_ext";
            saveImage(KochExtractor(seg_side, spinBoxValue, false),strNewName, imgFormat);
        }
    }
}

void MainWindow::showImage(Mat image) {
    Mat rgb;
    cvtColor(image, rgb, CV_BGR2RGB);

    QImage qimage = QImage((const unsigned char*)rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    scene.addPixmap(QPixmap::fromImage(qimage));
    ui->graphicsView->setScene(&scene);
}

void MainWindow::saveImage(Mat image, QString outputPath, ImgFormat format) {
    QString finalPath;
    Mat finalMat;
    switch(format) {
    case ImgFormat::PNG:
        finalPath = outputPath + ".png";
        finalMat = image;
        break;
    case ImgFormat::PGM:
        finalPath = outputPath + ".pgm";
        Mat grayImage;
        cvtColor(image, grayImage, CV_BGR2GRAY);
        finalMat = grayImage;
        break;
    }
    imwrite(finalPath.toStdString().c_str(), finalMat);
}

bool** MainWindow::calculateHashes(Mat image, unsigned int seg_side) {
    int Nc = image.cols * image.rows / (seg_side*seg_side);

    bool** calculatedHashes = new bool*[Nc];
    for (int i = 0; i < Nc; i++) {
        calculatedHashes[i] = new bool[64];
    }

    Mat grayImage, fgray;
    cvtColor(image, grayImage, CV_BGR2GRAY);

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
            x = x + seg_side;
            n = n + 1;
        }
        y = y + seg_side;
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
    delete [] segments;
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


Mat MainWindow::KochEmbedder(unsigned int seg_side, unsigned int P, bool is_batch_mode) {
    int x = seg_side;
    int y = seg_side;
    while (x + seg_side <= origImage.cols && y + seg_side <= origImage.rows) {
        x += seg_side;
        y += seg_side;
    }
    Rect Rec(0, 0, x, y);
    imagerd = origImage(Rec);

    bool** calculatedHashes = calculateHashes(imagerd, seg_side);
    Mat blue;
    vector<Mat> channels(3);
    split(imagerd, channels);
    blue = channels[0];

    Mat fimage;

    blue.convertTo(fimage, CV_64F/*, 1.0/255.0*/); // also scale to [0..1] range (not mandatory)


    int Nc = fimage.cols*fimage.rows / (seg_side*seg_side);

    y = 0;
    int n = 0;
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
                    if ((dctMat.at<double>(v1,u1) >= 0 && dctMat.at<double>(v2,u2) >= 0) ||
                        (dctMat.at<double>(v1,u1) <= 0 && dctMat.at<double>(v2,u2) <= 0)) {
                            dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) - 1.0;
                            dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)+ 1.0;
                    }
                    else if (dctMat.at<double>(v1,u1) < 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) +1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)+1.0;
                    }
                    else if (dctMat.at<double>(v2,u2) < 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) -1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2)- 1.0;
                    }
                }
            }
            else {
                while(abs(dctMat.at<double>(v1,u1)) - abs(dctMat.at<double>(v2,u2)) <= P) {
                    if ((dctMat.at<double>(v1,u1) >= 0 && dctMat.at<double>(v2,u2) >= 0) ||
                        (dctMat.at<double>(v1,u1) <= 0 && dctMat.at<double>(v2,u2) <= 0)) {
                            dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) + 1.0;
                            dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2) - 1.0;
                    }
                    else if (dctMat.at<double>(v1,u1) < 0) {
                        dctMat.at<double>(v1,u1) = dctMat.at<double>(v1,u1) -1.0;
                        dctMat.at<double>(v2,u2) = dctMat.at<double>(v2,u2) -1.0;
                    }
                    else if (dctMat.at<double>(v2,u2) < 0) {
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
    qDebug() << "deb1";
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
    tImage.copyTo(origImage(Rec));
    if (!is_batch_mode) {
        Mat rectangled = origImage.clone();
        rectangle(rectangled , Rec, Scalar(255), 1, 8, 0);
        showImage(rectangled);
    }
    for(int i = 0; i < Nc; i++) {
        delete [] calculatedHashes[i];
    }
    delete [] calculatedHashes;
    for(int i = 0; i < Nc; i++) {
        segments[i].release();
    }
    delete [] segments;
    return origImage;
}

int hDist(bool* v1, bool* v2, int length) {
    int distance = 0;
    for (int i = 0; i < length; i++) {
        if (v1[i] != v2[i])
            distance++;
    }
    return distance;
}

Mat MainWindow::KochExtractor(unsigned int seg_side, unsigned int T, bool is_batch_mode) {
    int x = seg_side;
    int y = seg_side;
    while (x + seg_side <= origImage.cols && y + seg_side <= origImage.rows) {
        x += seg_side;
        y += seg_side;
    }
    Rect Rec(0, 0, x, y);
    imagerd = origImage(Rec);
    qDebug() << imagerd.cols << ' ' << imagerd.rows;
    bool** calculatedHashes = calculateHashes(imagerd, seg_side);
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

    y = 0;
    int n = 0;
    x;
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
    tImage.copyTo(origImage(Rec));
    rectangle(origImage, Rec, Scalar(255), 1, 8, 0);
    if (!is_batch_mode)
        showImage(origImage);
    return origImage;
}

MainWindow::~MainWindow()
{
    delete ui;
}
