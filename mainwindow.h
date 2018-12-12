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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
protected:

private slots:
    void openImage();
    void process();
    void printCom();
private:
    Ui::MainWindow *ui;
    Mat   imagerd; //declare imagerd as IplImage
    QString     FileOpName; //declare FileOpName as IplImage
    bool wmark[64] = {1,0,0,1,1,0,1,1,1,0,1,0,1,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,0,0,0,0,0,1,1,1,1,0,1,0,1,1,0,1,1,0,1,1,0,1,0,1,1,0,0,1,0,1,1,0,0,1,0};

    void KochEmbedder(unsigned int seg_side, unsigned int P);
    void KochExtractor(unsigned int seg_side, unsigned int T);
    bool** calculateHashes(unsigned int seg_side);


};

#endif // MAINWINDOW_H
