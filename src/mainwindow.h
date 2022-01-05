#pragma once

#include "ui_mainwindow.h"

#include <memory>

#include <QGraphicsScene>
#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget* parent = nullptr);
  virtual ~MainWindow() {};

private slots:
  unsigned int getSpinBoxValue();
  QString getOutputFolderPath();
  QString getInputFilePath();
  void setOutputFolderPath();
  void setInputFilePath();
  void setSigningMode();
  void setVerifyingMode();
  void performAction();

private:
  void showMessage(const QString& text);
  void signImage(const QString& input_file_path, const QString& output_folder_path, unsigned int block_side, unsigned int p);
  void verifyImage(const QString& input_file_path, const QString& output_folder_path, unsigned int block_side, unsigned int critical_distance);

  std::unique_ptr<Ui::MainWindow> ui;
  QGraphicsScene scene;
  std::map<int, unsigned int> block_sides;
};
