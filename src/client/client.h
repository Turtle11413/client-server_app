#ifndef CLIENT_H_
#define CLIENT_H_

#include <QFile>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QTcpSocket>
#include <QTextBrowser>
#include <QWidget>

class Client : public QWidget {
  Q_OBJECT
 public:
  Client();
  ~Client();
  bool isConnected() const noexcept { return is_connected_; }

 public slots:
  void OnConnectButtonClicked();
  void OnSendButtonClicked();

 private:
  void InitMainWindow();
  void InitTableWidget();
  void InitButtons();
  void InitLabels();
  void MainWindowPlaceItems();
  void onConnected();
  void onConnectionError(QAbstractSocket::SocketError socketError);

  void ReadServerMessage();
  void RequestFileFromServer(const QString &filename);
  void ReceiveFileFromServer(QDataStream &in);
  void ReadFromServerForUpdateTable(QDataStream &in, const QString &message);
  void LoadTableFromServer(QDataStream &in);
  void DownloadLink(QTableWidgetItem *item);
  void SendFileToServer(QFile &file);

  void AddNewRow(const QString &filename, const QString &link,
                 const QString &load_date);
  void OverrideRow(const QString &filename, const QString &link,
                   const QString &load_date);

  void CreateSocket();

  QGridLayout *main_layout_;

  QTableWidget *table_widget_;

  QPushButton *connect_button_;
  QPushButton *send_button_;

  QLabel *client_status_label_;

  QTcpSocket *socket_;

  bool is_connected_ = false;
};

#endif  // CLIENT_H_
