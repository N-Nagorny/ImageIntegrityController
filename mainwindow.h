#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;
using namespace std;
namespace Ui {
class MainWindow;
}

enum class ImgFormat {
    PNG,
    PGM
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:

private slots:
    void openImage();
    void processBatch();
    void updateSegSide(int newValue);
    void updateImgFormat(int newValue);
    void updateSpinBoxValue(int newValue);

private:
    Ui::MainWindow *ui;
    Mat   imagerd;
    Mat origImage;
    bool wmark[64] = {1,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,0,1,0,1,1,0,1,1,0,1,1,0,1,0,1,1,0,0,1,0,1,1,0,0,1,0};
    unsigned int seg_side = 32;
    unsigned int spinBoxValue = 1;
    ImgFormat imgFormat = ImgFormat::PNG;

    Mat KochEmbedder(unsigned int seg_side, unsigned int P);
    Mat KochExtractor(unsigned int seg_side, unsigned int T);
    bool** calculateHashes(Mat image, unsigned int seg_side);
    void showImage(Mat image);
    void saveImage(Mat image, QString outputPath, ImgFormat format);
};

#endif // MAINWINDOW_H
