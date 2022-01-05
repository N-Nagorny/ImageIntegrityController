#include "integrity.hpp"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <memory>

#include <QFileDialog>
#include <QMessageBox>
#include <QPixmap>
#include <QDebug>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(std::make_unique<Ui::MainWindow>())
{
  ui->setupUi(this);

  {
    QVector<unsigned int> ui_block_sides{32, 64, 128};
    QChar multiplication_sign{0x00D7};
    QList<QString> block_side_names;

    for (int i = 0; i < ui_block_sides.size(); ++i) {
      unsigned int block_side = ui_block_sides.at(i);
      block_sides[i] = block_side;
      block_side_names.append(
        QString::number(block_side) +
        multiplication_sign +
        QString::number(block_side)
      );
    }

    ui->block_size_combobox->addItems(block_side_names);
  }

  setSigningMode();

  QObject::connect(ui->choose_input_file_pushbutton, &QPushButton::clicked, this, &MainWindow::setInputFilePath);
  QObject::connect(ui->choose_output_folder_pushbutton, &QPushButton::clicked, this, &MainWindow::setOutputFolderPath);
  QObject::connect(ui->signing_radiobutton, &QRadioButton::clicked, this, &MainWindow::setSigningMode);
  QObject::connect(ui->verifying_radiobutton, &QRadioButton::clicked, this, &MainWindow::setVerifyingMode);
  QObject::connect(ui->perform_pushbutton, &QPushButton::clicked, this, &MainWindow::performAction);
}

void MainWindow::setSigningMode() {
  unsigned int min = 1;
  unsigned int max = 100;

  ui->spinbox_label->setText(
    QString("Noise margin factor (") +
    QString::number(min) +
    QString("-") +
    QString::number(max) +
    QString(") :")
  );

  ui->spinbox->setMinimum(min);
  ui->spinbox->setMaximum(max);
  ui->spinbox->setValue(min);
}

void MainWindow::setVerifyingMode() {
  unsigned int min = 1;
  unsigned int max = 64;

  ui->spinbox_label->setText(
    QString("Critical Hamming distance (") +
    QString::number(min) +
    QString("-") +
    QString::number(max) +
    QString(") :")
  );

  ui->spinbox->setMinimum(min);
  ui->spinbox->setMaximum(max);
  ui->spinbox->setValue(min);
}

void MainWindow::setInputFilePath() {
  QString input_file_path = QFileDialog::getOpenFileName(
    this,
    QObject::tr("Choose an image"),
    QDir::currentPath(),
    QObject::tr("Image Files (*.png *.pgm *.jpg *.jpeg *.bmp)")
  );
  ui->input_file_path_lineedit->setText(input_file_path);
}

void MainWindow::setOutputFolderPath() {
  QString output_folder_path = QFileDialog::getExistingDirectory(
    this,
    QObject::tr("Choose a folder"),
    QDir::currentPath()
  );
  ui->output_folder_path_lineedit->setText(output_folder_path);
}

unsigned int MainWindow::getSpinBoxValue() {
  return ui->spinbox->value();
}

QString MainWindow::getInputFilePath() {
  return ui->input_file_path_lineedit->text();
}

QString MainWindow::getOutputFolderPath() {
  return ui->output_folder_path_lineedit->text();
}

void MainWindow::showMessage(const QString& text) {
  QMessageBox messageBox;
  messageBox.critical(0, QObject::tr("Error"), text);
  messageBox.setFixedSize(500, 200);
}

void MainWindow::performAction() {
  QString input_file_path = getInputFilePath();
  QString output_folder_path = getOutputFolderPath();

  if (!QFileInfo::exists(input_file_path) ||
      !QFileInfo(input_file_path).isFile()) {
    showMessage(QObject::tr("Input file doesn't exist."));
    return;
  }

  if (!QFileInfo::exists(output_folder_path) ||
      !QFileInfo(output_folder_path).isDir()) {
    showMessage(QObject::tr("Output folder doesn't exist."));
    return;
  }

  unsigned int threshold = getSpinBoxValue();
  unsigned int block_side =
    block_sides.at(ui->block_size_combobox->currentIndex());

  try {
    if (ui->signing_radiobutton->isChecked()) {
      signImage(input_file_path, output_folder_path, block_side, threshold);
    }

    if (ui->verifying_radiobutton->isChecked()) {
      verifyImage(input_file_path, output_folder_path, block_side, threshold);
    }
  } catch (const std::exception& e) {
    showMessage(e.what());
  }
}

void MainWindow::signImage(const QString& input_file_path, const QString& output_folder_path, unsigned int block_side, unsigned int p) {
  cv::Mat input_image = cv::imread(input_file_path.toStdString());
  if (input_image.size() == cv::Size{0, 0}) {
    throw std::logic_error(input_file_path.toStdString() + std::string(" is not an image."));
  }

  QString input_file_basename = QFileInfo(input_file_path).baseName();
  QString output_file_path = QDir(output_folder_path).filePath(input_file_basename + "_signed" + ".png");
  if (QFileInfo::exists(output_file_path)) {
    QMessageBox::StandardButton ret = QMessageBox::question(
      this,
      QObject::tr("Are you sure?"),
      output_file_path + QObject::tr(" already exists. Do you want to overwrite it?"),
      QMessageBox::No | QMessageBox::Yes
    );
    if (ret == QMessageBox::No)
      return;
  }

  signMat(&input_image, block_side, p);
  cv::imwrite(output_file_path.toStdString(), input_image);
  ui->statusbar->showMessage("Signed file saved to " + output_file_path);

  unsigned int processed_x = input_image.cols / block_side * block_side;
  unsigned int processed_y = input_image.rows / block_side * block_side;

  cv::cvtColor(input_image, input_image, CV_BGR2RGB);
  cv::rectangle(input_image, cv::Rect(0, 0, processed_x, processed_y), cv::Scalar(255));
  QImage output_image =
    QImage(static_cast<const unsigned char*>(input_image.data), input_image.cols, input_image.rows, input_image.step, QImage::Format_RGB888);
  scene.addPixmap(QPixmap::fromImage(output_image));
  ui->graphicsview->setScene(&scene);
}

void MainWindow::verifyImage(const QString& input_file_path, const QString& output_folder_path, unsigned int block_side, unsigned int critical_distance) {
  cv::Mat input_image = cv::imread(input_file_path.toStdString());
  if (input_image.size() == cv::Size{0, 0}) {
    throw std::logic_error(input_file_path.toStdString() + std::string(" is not an image."));
  }

  QString input_file_basename = QFileInfo(input_file_path).baseName();
  QString output_file_path = QDir(output_folder_path).filePath(input_file_basename + "_verified" + ".png");
  if (QFileInfo::exists(output_file_path)) {
    QMessageBox::StandardButton ret = QMessageBox::question(
      this,
      QObject::tr("Are you sure?"),
      output_file_path + QObject::tr(" already exists. Do you want to overwrite it?"),
      QMessageBox::No | QMessageBox::Yes
    );
    if (ret == QMessageBox::No)
      return;
  }

  input_image = checkMat(input_image, block_side, critical_distance);
  cv::imwrite(output_file_path.toStdString(), input_image);
  ui->statusbar->showMessage("Verified file saved to " + output_file_path);

  unsigned int processed_x = input_image.cols / block_side * block_side;
  unsigned int processed_y = input_image.rows / block_side * block_side;

  cv::cvtColor(input_image, input_image, CV_BGR2RGB);
  cv::rectangle(input_image, cv::Rect(0, 0, processed_x, processed_y), cv::Scalar(255));
  QImage output_image =
    QImage(static_cast<const unsigned char*>(input_image.data), input_image.cols, input_image.rows, input_image.step, QImage::Format_RGB888);
  scene.addPixmap(QPixmap::fromImage(output_image));
  ui->graphicsview->setScene(&scene);
}
