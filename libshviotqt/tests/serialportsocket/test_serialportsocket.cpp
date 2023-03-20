#include "mockserialport.h"
#include "mockrpcconnection.h"

#include <shv/iotqt/rpc/serialportsocket.h>
#include <shv/iotqt/rpc/socket.h>

#include <shv/chainpack/rpcmessage.h>

#include <necrolog.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

using namespace shv::iotqt::rpc;
using namespace shv::chainpack;
using namespace std;

int main(int argc, char** argv)
{
	//NecroLog::setTopicsLogThresholds("SerialPortSocket,WriteQueue");
	Exception::setAbortOnException(true);
	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("Send")
{
	RpcMessage rec_msg;
	MockRpcConnection conn;
	QObject::connect(&conn, &MockRpcConnection::rpcMessageReceived, [&rec_msg](const shv::chainpack::RpcMessage &msg) {
		nInfo() << "data read:" << msg.toCpon();
		rec_msg = msg;
	});
	auto *serial = new MockSerialPort("TestSend", &conn);
	//serial->open(QIODevice::ReadWrite);
	//nInfo() << "serial is open:" << serial->isOpen();
	//serial->write("abc", 2);
	auto *socket = new SerialPortSocket(serial);
	socket->setReceiveTimeout(0);
	conn.setSocket(socket);
	conn.connectToHost(QUrl());
	RpcRequest rq;
	rq.setRequestId(1);
	rq.setShvPath("foo/bar");
	rq.setMethod("baz");
	rq.setParams(RpcValue::List{0xa2, 0xa3, 0xa4, 0xa5, 0xaa, "\xa2\xa3\xa4\xa5\xaa"});
	//rq.setParams("\xa2\xa3\xa4\xa5\xaa");
	//rq.setParams("\xaa");
	nInfo() << "message chainpack:" << QByteArray::fromStdString(rq.value().toChainPack()).toHex().toStdString();
	conn.sendMessage(rq);
	nInfo() << "data writen:" << serial->writtenData().toHex().toStdString();
	auto data = serial->writtenData();
	serial->setDataToReceive(data);
	REQUIRE(rec_msg.value() == rq.value());

	// add some rubbish
	data = QByteArray("rubbish!@#$%^&*") + data;
	serial->setDataToReceive(data);
	REQUIRE(rec_msg.value() == rq.value());
}
