#include "view.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <iostream>

View::View() : QWidget(nullptr) {
  InitMainWindow();
  CreateSocket();
}

void View::InitMainWindow() {
  InitTableWidget();
  InitButtons();
  InitLabels();

  MainWindowPlaceItems();

  setLayout(main_layout_);
  setWindowTitle("Client");
  this->resize(1000, 500);
}

void View::InitTableWidget() {
  table_widget_ = new QTableWidget(0, 3, this);

  table_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  table_widget_->setShowGrid(true);
  table_widget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  QStringList headers = {"filename", "link", "load time"};
  table_widget_->setHorizontalHeaderLabels(headers);
}

void View::InitButtons() {
  connect_button_ = new QPushButton("Connect", this);
  connect_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(connect_button_, &QPushButton::clicked, this,
          &View::OnConnectButtonClicked);

  send_button_ = new QPushButton("Send", this);
  send_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(send_button_, &QPushButton::clicked, this,
          &View::OnSendButtonClicked);

  download_button_ = new QPushButton("Download", this);
  download_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(download_button_, &QPushButton::clicked, this,
          &View::OnDownloadButtonClicked);
}

void View::InitLabels() {
  client_status_label_ = new QLabel(this);
  client_status_label_->setText(tr("Client status: Disconnected"));
}

void View::MainWindowPlaceItems() {
  main_layout_ = new QGridLayout(this);

  main_layout_->addWidget(table_widget_, 0, 0, 3, 3);

  main_layout_->addWidget(connect_button_, 0, 3, 1, 1);
  main_layout_->addWidget(send_button_, 1, 3, 1, 1);
  main_layout_->addWidget(download_button_, 2, 3, 1, 1);

  main_layout_->addWidget(client_status_label_, 3, 0, 1, 4);
}

void View::AddNewRow(const QString &filename, const QString &link,
                     const QString &load_date) {
  int new_row_cnt = table_widget_->rowCount();

  table_widget_->insertRow(new_row_cnt);

  table_widget_->setItem(new_row_cnt, 0, new QTableWidgetItem(filename));
  table_widget_->setItem(new_row_cnt, 1, new QTableWidgetItem(link));
  table_widget_->setItem(new_row_cnt, 2, new QTableWidgetItem(load_date));
}

void View::OverrideRow(const QString &filename, const QString &link,
                       const QString &load_date) {
  for (int row = 0; row < table_widget_->rowCount(); ++row) {
    QString curFilename = table_widget_->item(row, 0)->text();
    if (curFilename == filename) {
      table_widget_->removeRow(row);
      break;
    }
  }
  AddNewRow(filename, link, load_date);
}

void View::CreateSocket() {
  socket_ = new QTcpSocket(this);
  connect(socket_, &QTcpSocket::connected, this, &View::onConnected);
  connect(socket_, &QTcpSocket::errorOccurred, this, &View::onConnectionError);
  connect(socket_, &QTcpSocket::readyRead, this, &View::ReadServerMessage);
}

void View::onConnected() {
  is_connected_ = true;
  client_status_label_->setText("Client status: Connected");
}

void View::onConnectionError(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  QMessageBox::warning(
      this, "Error",
      "Failed to connect to the server: " + socket_->errorString());
  is_connected_ = false;
  client_status_label_->setText("Client status: Disconnected");
}

void View::OnConnectButtonClicked() {
  if (!isConnected()) {
    socket_->connectToHost("127.0.0.1", 1111);
  } else {
    QMessageBox::warning(this, "Warning", "You're already connected");
  }
}

void View::OnSendButtonClicked() {
  if (isConnected()) {
    QString file_path =
        QFileDialog::getOpenFileName(this, "Choose a file to upload");
    if (!file_path.isEmpty()) {
      QFile file(file_path);
      if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Warning", "Failed to open file");
        return;
      }
      SendFileToServer(file);
    } else {
      QMessageBox::warning(this, "Warning", "Can't open file");
    }
  } else {
    QMessageBox::warning(this, "Warning", "You're not connected to server");
  }
}

void View::OnDownloadButtonClicked() {
  // QString link = "http://www.google.com";
  // QDesktopServices::openUrl(QUrl(link));

  RequestFileFromServer("client.pro");
}

void View::SendFileToServer(QFile &file) {
  QDataStream out(socket_);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "UPLOAD_FILE";
  out << message;
  QString filename = QFileInfo(file).fileName();
  out << filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  QString send_date = QDateTime::currentDateTime().toString();
  out << send_date;
  std::cout << "send date: " << send_date.toStdString() << std::endl;

  int file_size = QFileInfo(file).size();
  out << file_size;
  std::cout << "size: " << file_size << std::endl;

  QByteArray file_data = file.readAll();
  std::cout << "file data size: " << file_data.size() << std::endl;
  out << file_data;
}

void View::ReadServerMessage() {
  QDataStream in(socket_);
  in.setVersion(QDataStream::Qt_6_6);

  QString message;
  in >> message;

  if (message == "SEND_FILE_FOR_U") {
    ReceiveFileFromServer(in);
  } else if (message == "ADD_NEW_FILE_TO_TABLE") {
    ReadFromServerForUpdateTable(in, message);
  } else if (message == "NEW_FILE" || message == "OVERRIDE") {
    ReadFromServerForUpdateTable(in, message);
  }
}

void View::ReceiveFileFromServer(QDataStream &in) {
  QString filename;
  in >> filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  int size = 0;
  in >> size;
  std::cout << "control size: " << size << std::endl;

  QFile received_file(filename);
  QByteArray file_data;

  if (received_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    in >> file_data;
    std::cout << "size: " << file_data.size() << std::endl;
    received_file.write(file_data);
    std::cout << "file received\n";
  }

  received_file.close();
}

void View::ReadFromServerForUpdateTable(QDataStream &in,
                                        const QString &message) {
  QString filename, load_time;
  in >> filename >> load_time;

  if (message == "NEW_FILE") {
    AddNewRow(filename, "link", load_time);
  } else if (message == "OVERRIDE") {
    OverrideRow(filename, "link", load_time);
  }
}

void View::RequestFileFromServer(const QString &filename) {
  QDataStream out(socket_);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "SEND_ME_FILE";
  out << message << filename;
}
