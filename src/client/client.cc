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
  connect_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  connect(connect_button_, &QPushButton::clicked, this,
          &Client::OnConnectButtonClicked);

  send_button_ = new QPushButton("Send", this);
  send_button_->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
  connect(send_button_, &QPushButton::clicked, this,
          &Client::OnSendButtonClicked);
}

void Client::InitLabels() {
  client_status_label_ = new QLabel(this);
  client_status_label_->setText(tr("Client status: Disconnected"));
  client_status_label_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void Client::MainWindowPlaceItems() {
  main_layout_ = new QGridLayout(this);

  main_layout_->addWidget(table_widget_, 0, 0, 3, 3);

  main_layout_->addWidget(connect_button_, 0, 3, 1, 1);
  main_layout_->addWidget(send_button_, 1, 3, 2, 1);

  main_layout_->addWidget(client_status_label_, 3, 0, 1, 3);
}

void Client::AddNewRow(const QString &filename, const QString &link,
                       const QString &load_date) {
  int new_row_cnt = table_widget_->rowCount();

  table_widget_->insertRow(new_row_cnt);

  table_widget_->setItem(new_row_cnt, 0, new QTableWidgetItem(filename));

  QTableWidgetItem *link_item = new QTableWidgetItem(link);
  link_item->setForeground(Qt::blue);
  link_item->setFlags(link_item->flags() | Qt::ItemIsSelectable);
  table_widget_->setItem(new_row_cnt, 1, link_item);
  connect(table_widget_, &QTableWidget::itemClicked, this,
          &Client::DownloadLink);

  table_widget_->setItem(new_row_cnt, 2, new QTableWidgetItem(load_date));
}

void Client::DownloadLink(QTableWidgetItem *item) {
  QString link = item->text();
  QString filename = QFileInfo(QUrl(link).path()).fileName();
  RequestFileFromServer(filename);
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
      this, "Warning",
      "Failed to connect to the server: " + socket_->errorString());
  is_connected_ = false;
  client_status_label_->setText("Client status: Disconnected");

  for (int row = table_widget_->rowCount() - 1; row >= 0; --row) {
    table_widget_->removeRow(row);
  }
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

void Client::SendFileToServer(QFile &file) {
  QDataStream out(socket_);
  QString message = "UPLOAD_FILE";
  out << message;

  QString filename = QFileInfo(file).fileName();
  out << filename;

  QString send_date = QDateTime::currentDateTime().toString();
  out << send_date;

  qint64 file_size = QFileInfo(file).size();
  out << file_size;

  QByteArray buffer;
  const int block_size = 8192;
  qint64 bytes_send = 0;

  while (bytes_send < file_size) {
    buffer = file.read(block_size);
    bytes_send += socket_->write(buffer);
    socket_->waitForBytesWritten();
  }

  std::cout << "Send file size: " << bytes_send << std::endl;
}

void Client::ReadServerMessage() {
  QDataStream in(socket_);
  in.setVersion(QDataStream::Qt_6_6);

  QString message;
  in >> message;

  if (message == "SEND_FILE_FOR_U") {
    ReceiveFileFromServer(in);
  } else if (message == "NEW_FILE" || message == "OVERRIDE") {
    ReadFromServerForUpdateTable(in, message);
  } else if (message == "LOAD_TABLE") {
    LoadTableFromServer(in);
  }
}

void Client::ReceiveFileFromServer(QDataStream &in) {
  QString filename;
  in >> filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  qint64 file_size = 0;
  in >> file_size;
  std::cout << "control size: " << file_size << std::endl;

  QString file_path = QFileDialog::getExistingDirectory(this, tr("Save file"),
                                                        QDir::homePath());
  if (!file_path.isEmpty()) {
    QFile received_file(file_path + "/" + filename);
    QByteArray file_data;

    if (received_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QByteArray buffer;
      qint64 bytes_receive = 0;

      while (bytes_receive < file_size) {
        buffer = socket_->read(qMin(file_size - bytes_receive, qint64(8192)));
        bytes_receive += buffer.size();
        received_file.write(buffer);
        // std::cout << buffer.toStdString();
        if (buffer.isEmpty()) {
          if (!socket_->waitForReadyRead()) {
            QMessageBox::warning(this, "Erorr",
                                 "Error: waitForReadyRead() failed");
            received_file.close();

            return;
          }
        }
      }
    }

    received_file.close();

    QFile log_file(file_path + "/log.txt");
    if (log_file.open(QIODevice::WriteOnly | QIODevice::Append)) {
      QTextStream out(&log_file);
      out << "File: " << filename << " downloaded at: "
          << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
      log_file.close();
    } else {
      qDebug() << "Error: Failed to open log file for writing";
    }
  } else {
    QMessageBox::warning(this, "Error",
                         "Error: Failed to open received file for writing");
  }
}

void Client::ReadFromServerForUpdateTable(QDataStream &in,
                                          const QString &message) {
  QString filename, load_time;
  in >> filename >> load_time;

  QString link = "http://localhost:1111/files/" + filename;

  if (message == "NEW_FILE") {
    AddNewRow(filename, link, load_time);
  } else if (message == "OVERRIDE") {
    OverrideRow(filename, link, load_time);
  }
}

void Client::LoadTableFromServer(QDataStream &in) {
  quint16 size = 0;
  in >> size;

  for (quint16 i = 0; i < size; ++i) {
    QString filename;
    QString load_date;
    in >> load_date >> filename;
    QString link = "http://localhost:1111/files/" + filename;

    AddNewRow(filename, link, load_date);
  }
}

void Client::RequestFileFromServer(const QString &filename) {
  QDataStream out(socket_);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "SEND_ME_FILE";
  out << message << filename;
}
