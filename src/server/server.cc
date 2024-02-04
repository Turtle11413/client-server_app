#include "server.h"

#include <QDir>
#include <iostream>

Server::Server(const int &port) {
  server_ = new QTcpServer(this);
  if (server_->listen(QHostAddress::Any, port)) {
    std::cout << "\tServer is running\n"
              << "\tPort: " << port << std::endl;

    QDir().mkpath("./files/");
    FillFileMap();

    connect(server_, &QTcpServer::newConnection, this, &Server::NewConnection);
    connect(this, &Server::NewFileUploaded, this, &Server::UpdateClientTable);

  } else {
    std::cout << "Unable to start server\n";
  }
}

Server::~Server() {
  for (auto socket : client_sockets_) {
    socket->close();
    socket->deleteLater();
  }

  server_->close();
  server_->deleteLater();
}

void Server::FillFileMap() {
  QDir directory("files/");

  QFileInfoList files_list = directory.entryInfoList(QDir::Files);
  file_map_.clear();

  for (const QFileInfo &fileInfo : files_list) {
    QString filename = fileInfo.fileName();
    QString load_time = fileInfo.lastModified().toString(Qt::ISODate);

    file_map_.insert(filename, load_time);
  }
}

void Server::NewConnection() {
  while (server_->hasPendingConnections()) {
    QTcpSocket *socket = server_->nextPendingConnection();
    client_sockets_.insert(socket);

    connect(socket, &QTcpSocket::readyRead, this,
            &Server::ReadMessageFromClient);
    connect(socket, &QTcpSocket::disconnected, this,
            &Server::ClientDisconnected);

    if (!file_map_.empty()) {
      for (auto it = file_map_.begin(); it != file_map_.end(); ++it) {
        UpdateClientTable(it.key(), it.value());
      }
    }

    std::cout << "New client connected: " << socket->peerPort() << std::endl;
    std::cout << "size: " << client_sockets_.size() << std::endl;
  }
}

void Server::ClientDisconnected() {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  if (socket && client_sockets_.contains(socket)) {

    client_sockets_.remove(socket);
    socket->deleteLater();

    std::cout << "Client disconnected: " << socket->peerPort() << std::endl;
    std::cout << "size: " << client_sockets_.size() << std::endl;
  }
}

void Server::ReadMessageFromClient() {
  QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
  if (!socket) {
    return;
  }

  QDataStream in(socket);
  in.setVersion(QDataStream::Qt_6_6);

  QString message;
  in >> message;

  std::cout << "Message is: " << message.toStdString() << std::endl;

  if (message == "UPLOAD_FILE") {

    ReceiveFileFromClient(in, socket);
  } else if (message == "SEND_ME_FILE") {
    QString filename;
    in >> filename;
    SendFileToClient(filename, socket);
  }
}

bool Server::isExistsFile(const QString &filename) {
  return file_map_.contains(filename);
}

void Server::ReceiveFileFromClient(QDataStream &in, QTcpSocket *client) {
  std::cout << "Start receiving from client\n";

  QString filename;
  in >> filename;
  std::cout << "filename is: " << filename.toStdString() << std::endl;

  QString send_date;
  in >> send_date;
  std::cout << "date is: " << send_date.toStdString() << std::endl;

  qint64 size = 0;
  in >> size;

  QFile file("./files/" + filename);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    std::cerr << "Failed to open file for writing!";
    return;
  }

  QByteArray buffer;
  qint64 bytes_receive = 0;

  while (bytes_receive < size) {
    buffer = client->read(qMin(size - bytes_receive, qint64(8192)));
    bytes_receive += buffer.size();
    file.write(buffer);
    // std::cout << buffer.toStdString();
    if (buffer.isEmpty()) {
      if (!client->waitForReadyRead()) {
        std::cerr << "Error: waitForReadyRead() failed" << std::endl;
        file.close();
        return;
      }
    }
  }

  std::cout << "Send file size: " << bytes_receive << std::endl;

  file.close();

  emit NewFileUploaded(filename, send_date);

  file_map_.insert(filename, send_date);
}

void Server::SendFileToClient(const QString &filename, QTcpSocket *client) {
  QDataStream out(client);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "SEND_FILE_FOR_U";
  out << message;
  std::cout << message.toStdString() << std::endl;

  out << filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  QFile file("./files/" + filename);

  qint64 file_size = QFileInfo(file).size();
  out << file_size;
  std::cout << "size: " << file_size << std::endl;

  if (file.open(QIODevice::ReadOnly)) {

    QByteArray buffer;
    const int block_size = 8192;
    qint64 bytes_send = 0;

    while (bytes_send < file_size) {
      buffer = file.read(block_size);
      bytes_send += client->write(buffer);
      client->waitForBytesWritten();
    }

    file.close();
  }
}

void Server::UpdateClientTable(const QString &filename,
                               const QString &load_time) {
  QByteArray buffer;
  QDataStream out(&buffer, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_6_6);

  QString message;
  if (!isExistsFile(filename)) {
    message = "NEW_FILE";
  } else {
    message = "OVERRIDE";
  }

  out << message << filename << load_time;

  for (auto client : client_sockets_) {
    client->write(buffer);
  }
}
