#include "client.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <iostream>

Client::Client() : QWidget(nullptr) {
  InitMainWindow();
  CreateSocket();
}

Client::~Client() {
  if (socket_->isOpen()) {
    socket_->close();
  }

  socket_->disconnect();
  socket_->deleteLater();
}

void Client::InitMainWindow() {
  InitTableWidget();
  InitButtons();
  InitLabels();

  MainWindowPlaceItems();

  setLayout(main_layout_);
  setWindowTitle("Client");
  this->resize(1000, 500);
}

void Client::InitTableWidget() {
  table_widget_ = new QTableWidget(0, 3, this);

  table_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  table_widget_->setShowGrid(true);
  table_widget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  QStringList headers = {"filename", "link", "load time"};
  table_widget_->setHorizontalHeaderLabels(headers);
}

void Client::InitButtons() {
  connect_button_ = new QPushButton("Connect", this);
  connect_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(connect_button_, &QPushButton::clicked, this,
          &Client::OnConnectButtonClicked);

  send_button_ = new QPushButton("Send", this);
  send_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(send_button_, &QPushButton::clicked, this,
          &Client::OnSendButtonClicked);

  download_button_ = new QPushButton("Download", this);
  download_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(download_button_, &QPushButton::clicked, this,
          &Client::OnDownloadButtonClicked);
}

void Client::InitLabels() {
  client_status_label_ = new QLabel(this);
  client_status_label_->setText(tr("Client status: Disconnected"));
}

void Client::MainWindowPlaceItems() {
  main_layout_ = new QGridLayout(this);

  main_layout_->addWidget(table_widget_, 0, 0, 3, 3);

  main_layout_->addWidget(connect_button_, 0, 3, 1, 1);
  main_layout_->addWidget(send_button_, 1, 3, 1, 1);
  main_layout_->addWidget(download_button_, 2, 3, 1, 1);

  main_layout_->addWidget(client_status_label_, 3, 0, 1, 4);
}

void Client::AddNewRow(const QString &filename, const QString &link,
                       const QString &load_date) {
  int new_row_cnt = table_widget_->rowCount();

  table_widget_->insertRow(new_row_cnt);

  table_widget_->setItem(new_row_cnt, 0, new QTableWidgetItem(filename));
  table_widget_->setItem(new_row_cnt, 1, new QTableWidgetItem(link));
  table_widget_->setItem(new_row_cnt, 2, new QTableWidgetItem(load_date));
}

void Client::OverrideRow(const QString &filename, const QString &link,
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

void Client::CreateSocket() {
  socket_ = new QTcpSocket(this);
  connect(socket_, &QTcpSocket::connected, this, &Client::onConnected);
  connect(socket_, &QTcpSocket::errorOccurred, this,
          &Client::onConnectionError);
  connect(socket_, &QTcpSocket::readyRead, this, &Client::ReadServerMessage);
}

void Client::onConnected() {
  is_connected_ = true;
  client_status_label_->setText("Client status: Connected");
}

void Client::onConnectionError(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  QMessageBox::warning(
      this, "Error",
      "Failed to connect to the server: " + socket_->errorString());
  is_connected_ = false;
  client_status_label_->setText("Client status: Disconnected");
}

void Client::OnConnectButtonClicked() {
  if (!isConnected()) {
    socket_->connectToHost("127.0.0.1", 1111);
  } else {
    QMessageBox::warning(this, "Warning", "You're already connected");
  }
}

void Client::OnSendButtonClicked() {
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
      file.close();
    } else {
      QMessageBox::warning(this, "Warning", "Can't open file");
    }
  } else {
    QMessageBox::warning(this, "Warning", "You're not connected to server");
  }
}

void Client::OnDownloadButtonClicked() {
  // QString link = "http://www.google.com";
  // QDesktopServices::openUrl(QUrl(link));

  RequestFileFromServer("maze.txt");
}

void Client::SendFileToServer(QFile &file) {
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

  while (!file.atEnd()) {
    QByteArray buffer = file.read(8192);
    out.writeRawData(buffer.constData(), buffer.size());
  }
}

void Client::ReadServerMessage() {
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

void Client::ReceiveFileFromServer(QDataStream &in) {
  QString filename;
  in >> filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  int size = 0;
  in >> size;
  std::cout << "control size: " << size << std::endl;

  QString file_path = QFileDialog::getExistingDirectory(this, tr("Save file"),
                                                        QDir::homePath());
  if (!file_path.isEmpty()) {
    QFile received_file(file_path + "/" + filename);
    QByteArray file_data;

    if (received_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      in >> file_data;
      std::cout << "size: " << file_data.size() << std::endl;
      received_file.write(file_data);
      std::cout << "file received\n";
    }

    received_file.close();
  }
}

void Client::ReadFromServerForUpdateTable(QDataStream &in,
                                          const QString &message) {
  QString filename, load_time;
  in >> filename >> load_time;

  if (message == "NEW_FILE") {
    AddNewRow(filename, "link", load_time);
  } else if (message == "OVERRIDE") {
    OverrideRow(filename, "link", load_time);
  }
}

void Client::RequestFileFromServer(const QString &filename) {
  QDataStream out(socket_);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "SEND_ME_FILE";
  out << message << filename;
}