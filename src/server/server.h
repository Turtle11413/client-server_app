#ifndef SERVER_H_
#define SERVER_H_

#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>

class Server : public QTcpServer {
  Q_OBJECT

public:
  Server();
  QTcpSocket *socket;

public slots:
  void incomingConnection(qintptr socketDescriptor);
  void slotReadyRead();

private:
  void sendToClient(QString str);

  QVector<QTcpSocket *> sockets_;
  QByteArray data_;
};

#endif // SERVER_H_
