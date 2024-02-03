#include "server.h"

#include <QDir>
#include <iostream>

Server::Server(const int &port) {
  server_ = new QTcpServer(this);
  if (server_->listen(QHostAddress::Any, port)) {
    std::cout << "\tServer is running\n"
              << "\tPort: " << port << std::endl;

    connect(server_, &QTcpServer::newConnection, this, &Server::NewConnection);
    connect(this, &Server::NewFileUploaded, this, &Server::UpdateClientTable);

    QDir().mkpath("./files/");

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

void Server::NewConnection() {
  while (server_->hasPendingConnections()) {
    QTcpSocket *socket = server_->nextPendingConnection();
    client_sockets_.insert(socket);

    connect(socket, &QTcpSocket::readyRead, this,
            &Server::ReadMessageFromClient);
    connect(socket, &QTcpSocket::disconnected, this,
            &Server::ClientDisconnected);

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
  in.setFloatingPointPrecision(QDataStream::SinglePrecision);
  in.setByteOrder(QDataStream::BigEndian);

  QString message;
  in >> message;

  std::cout << message.toStdString() << std::endl;

  if (message == "UPLOAD_FILE") {
    ReceiveFileFromClient(in);
  } else if (message == "SEND_ME_FILE") {
    QString filename;
    in >> filename;
    SendFileToClient(filename, socket);
  }
}

bool Server::isExistsFile(const QString &filename) {
  return file_map_.contains(filename);
}

void Server::ReceiveFileFromClient(QDataStream &in) {
  QString filename;
  in >> filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  QString load_time;
  in >> load_time;
  std::cout << "load_time: " << load_time.toStdString() << std::endl;

  int file_size;
  in >> file_size;
  std::cout << "control size: " << file_size << std::endl;

  QFile loadedFile("./files/" + filename);

  if (loadedFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {

    while (file_size > 0 && !in.atEnd()) {
      QByteArray buffer;
      buffer.resize(file_size);
      in.readRawData(buffer.data(), file_size);
      loadedFile.write(buffer);
      file_size -= buffer.size();
    }
    // QByteArray file_data;
    // file_data.resize(file_size);

    // in.readRawData(file_data.data(), file_size);
    std::cout << "received file size: " << QFileInfo(loadedFile).size()
              << std::endl;

    // loadedFile.write(file_data);
    std::cout << "file received\n";
  }

  loadedFile.close();

  emit NewFileUploaded(filename, load_time);

  file_map_.insert(filename, load_time);
}

void Server::SendFileToClient(const QString &filename, QTcpSocket *client) {
  QByteArray buffer;
  QDataStream out(&buffer, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_6_6);

  QString message = "SEND_FILE_FOR_U";
  out << message;
  std::cout << message.toStdString() << std::endl;

  out << filename;
  std::cout << "filename: " << filename.toStdString() << std::endl;

  QFile file("./files/" + filename);

  int file_size = QFileInfo(file).size();
  out << file_size;
  std::cout << "size: " << file_size << std::endl;

  if (file.open(QIODevice::ReadOnly)) {

    QByteArray file_data = file.readAll();
    std::cout << "file data size: " << file_data.size() << std::endl;
    out << file_data;
    file.close();

    client->write(buffer);
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
