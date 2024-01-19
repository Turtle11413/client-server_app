#include "server.h"

#include <iostream>

Server::Server() {
  if (this->listen(QHostAddress::Any, 2323)) {
    std::cout << "server: start\n";
  } else {
    std::cout << "server: error\n";
  }
}

void Server::incomingConnection(qintptr socketDescriptor) {
  socket = new QTcpSocket;
  socket->setSocketDescriptor(socketDescriptor);

  connect(socket, &QTcpSocket::readyRead, this, &Server::slotReadyRead);
  connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);

  sockets_.push_back(socket);
  std::cout << "client connected: " << socketDescriptor << std::endl;
}

void Server::slotReadyRead() {
  socket = (QTcpSocket *)sender();
  QDataStream in(socket);
  in.setVersion(QDataStream::Qt_6_6);

  if (in.status() == QDataStream::Ok) {
    std::cout << "read...\n";
    QString str;
    in >> str;
    std::cout << str.toStdString() << std::endl;
  } else {
    std::cout << "DataStream error\n";
  }
}

void Server::sendToClient(QString str) {
  data_.clear();

  QDataStream out(&data_, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_6_6);
  out << str;

  socket->write(data_);
}
