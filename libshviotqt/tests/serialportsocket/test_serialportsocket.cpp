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
	//NecroLog::setTopicsLogThresholds("SerialPortSocket");
	Exception::setAbortOnException(true);
	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("Send")
{
	RpcMessage rec_msg;
	MockRpcConnection conn;
	QObject::connect(&conn, &MockRpcConnection::rpcMessageReceived, [&rec_msg](const shv::chainpack::RpcMessage &msg) {
		//nInfo() << "data read:" << msg.toCpon();
		rec_msg = msg;
	});
	auto *serial = new MockSerialPort("TestSend", &conn);
	auto *socket = new SerialPortSocket(serial);
	socket->setReceiveTimeout(0);
	conn.setSocket(socket);
	conn.connectToHost(QUrl());
	RpcRequest rq;
	rq.setRequestId(1);
	rq.setShvPath("foo/bar");
	rq.setMethod("baz");
	RpcValue::List params = {
		"hello",
		"\xaa",
		"\xa2\xa3\xa4\xa5\xaa",
		"aa\xaa""aa",
		RpcValue::List{0xa2, 0xa3, 0xa4, 0xa5, 0xaa, "\xa2\xa3\xa4\xa5\xaa"},
	};
	for(const auto &p : params) {
		rq.setParams(p);
		//nInfo() << "==== message:" << rq.toCpon();
		//nInfo() << "message chainpack:" << QByteArray::fromStdString(rq.value().toChainPack()).toHex().toStdString();
		serial->clearWrittenData();
		rec_msg = {};
		conn.sendMessage(rq);
		auto data = serial->writtenData();
		//serial->setDataToReceive(data);
		//REQUIRE(rec_msg.value() == rq.value());

		vector<string> rubbish1 = {
			"",
			"#$%^&",
			"\xa2",
			"\xa2\xaa\xa4\xa2\xa4\xaa",
			"%^\xa2""rr",
		};
		for(const auto &leading_rubbish : rubbish1) {
			// add some rubbish
			auto data1 = QByteArray::fromStdString(leading_rubbish) + data;
			vector<string> rubbish2 = {
				"",
				"/*-+",
				"\xa3",
				"\xa2\xa3\xa4\xa5\xaa",
				"%^\xaa""rr",
			};
			for(const auto &extra_rubbish : rubbish2) {
				// add some rubbish
				auto data2 = data1 +  QByteArray::fromStdString(extra_rubbish);
				//nInfo() << n++ << "data sent:" << data2.toStdString();
				//nInfo().nospace() << "data sent dump:\n" << shv::chainpack::utils::hexDump(data2.constData(), data2.size());
				rec_msg = {};
				serial->setDataToReceive(data2);
				REQUIRE(socket->readMessageError() == SerialPortSocket::ReadMessageError::Ok);
				REQUIRE(rec_msg.value() == rq.value());
			}
		}
	}
}

DOCTEST_TEST_CASE("Send corrupted data")
{

}
